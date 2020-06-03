#include "fast_double_parser.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

static inline uint64_t rng(uint64_t h) {
  h ^= h >> 33;
  h *= UINT64_C(0xff51afd7ed558ccd);
  h ^= h >> 33;
  h *= UINT64_C(0xc4ceb9fe1a85ec53);
  h ^= h >> 33;
  return h;
}
enum {
  FAST_PATH = 0,
  ZERO_PATH = 1,
  SLOW_PATH = 2,
  SLOWER_PATH = 3,
  FAILURE = 4,
  COULD_NOT_ROUND = 5,
  EXPONENT_FAILURE = 6,
  EARLY_STRTOD = 7
};

size_t compute_float_64_stats(int64_t power, uint64_t i) {

  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
    return FAST_PATH;
  }
  if (i == 0) {
    return ZERO_PATH;
  }

  // We are going to need to do some 64-bit arithmetic to get a more precise
  // product. We use a table lookup approach.
  fast_double_parser::components c = fast_double_parser::power_of_ten_components
      [power -
       FASTFLOAT_SMALLEST_POWER]; // safe because
                                  // power >= FASTFLOAT_SMALLEST_POWER
                                  // and power <= FASTFLOAT_LARGEST_POWER
  // we recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  uint64_t factor_mantissa = c.mantissa;
  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = fast_double_parser::leading_zeroes(i);
  i <<= lz;
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  fast_double_parser::value128 product =
      fast_double_parser::full_multiplication(i, factor_mantissa);
  uint64_t lower = product.low;
  uint64_t upper = product.high;
  // We know that upper has at most one leading zero because
  // both i and  factor_mantissa have a leading one. This means
  // that the result is at least as large as ((1<<63)*(1<<63))/(1<<64).

  // As long as the first 9 bits of "upper" are not "1", then we
  // know that we have an exact computed value for the leading
  // 55 bits because any imprecision would play out as a +1, in
  // the worst case.
  // We expect this next branch to be rarely taken (say 1% of the time).
  // When (upper & 0x1FF) == 0x1FF, it can be common for
  // lower + i < lower to be true (proba. much higher than 1%).
  int answer = SLOW_PATH;
  if (unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
    uint64_t factor_mantissa_low =
        fast_double_parser::mantissa_128[power - FASTFLOAT_SMALLEST_POWER];
    // next, we compute the 64-bit x 128-bit multiplication, getting a 192-bit
    // result (three 64-bit values)
    product = fast_double_parser::full_multiplication(i, factor_mantissa_low);
    uint64_t product_low = product.low;
    uint64_t product_middle2 = product.high;
    uint64_t product_middle1 = lower;
    uint64_t product_high = upper;
    uint64_t product_middle = product_middle1 + product_middle2;
    if (product_middle < product_middle1) {
      product_high++; // overflow carry
    }
    // we want to check whether mantissa *i + i would affect our result
    // This does happen, e.g. with 7.3177701707893310e+15
    if (((product_middle + 1 == 0) && ((product_high & 0x1FF) == 0x1FF) &&
         (product_low + i < product_low))) { // let us be prudent and bail out.
      return FAILURE;
    }
    upper = product_high;
    lower = product_middle;
    answer = SLOWER_PATH;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += 1 ^ upperbit;
  // Here we have mantissa < (1<<54).

  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  if (unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
               ((mantissa & 3) == 1))) {
    return COULD_NOT_ROUND;
  }
  mantissa += mantissa & 1;
  mantissa >>= 1;
  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    lz--; // undo previous addition
  }
  mantissa &= ~(1ULL << 52);
  uint64_t real_exponent = c.exp - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    return EXPONENT_FAILURE;
  }
  mantissa |= real_exponent << 52;
  double d;
  memcpy(&d, &mantissa, sizeof(d));
  // printf("answer = %zu\n", answer);
  return answer;
}

// parse the number at p
int parse_number_stats(const char *p) {
  bool found_minus = (*p == '-');
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
    if (!fast_double_parser::is_integer(
            *p)) { // a negative sign must be followed by an integer
      return false;
    }
  }
  const char *const start_digits = p;

  uint64_t i;      // an unsigned int avoids signed overflows (which are bad)
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    if (fast_double_parser::
            is_integer(*p)) {
      return false;
    }
    i = 0;
  } else {
    if (!(fast_double_parser::is_integer(*p))) { // must start with an integer
      return false;
    }
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (fast_double_parser::is_integer(*p)) {
      digit = *p - '0';
      // a multiplication by 10 is cheaper than an arbitrary integer
      // multiplication
      i = 10 * i + digit; // might overflow, we will handle the overflow later
      ++p;
    }
  }
  int64_t exponent = 0;
  const char *first_after_period = NULL;
  if ('.' == *p) {
    ++p;
    first_after_period = p;
    if (fast_double_parser::is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // might overflow + multiplication by 10 is likely
                          // cheaper than arbitrary mult.
      // we will handle the overflow later
    } else {
      return false;
    }
    while (fast_double_parser::is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
                          // because we have parse_highprecision_float later.
    }
    exponent = first_after_period - p;
  }
  int digit_count =
      p - start_digits - 1; // used later to guard against overflows
  int64_t exp_number = 0;   // exponential part
  if (('e' == *p) || ('E' == *p)) {
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!fast_double_parser::is_integer(*p)) {
      return false;
    }
    unsigned char digit = *p - '0';
    exp_number = digit;
    p++;
    if (fast_double_parser::is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (fast_double_parser::is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (fast_double_parser::is_integer(*p)) {
      if (exp_number > 0x100000000) { // we need to check for overflows
                                      // we refuse to parse this
        return false;
      }
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    exponent += (neg_exp ? -exp_number : exp_number);
  }
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon.
  if (unlikely((digit_count >= 19))) { // this is uncommon
    // It is possible that the integer had an overflow.
    // We have to handle the case where we have 0.0000somenumber.
    const char *start = start_digits;
    while ((*start == '0') || (*start == '.')) {
      start++;
    }
    // we over-decrement by one when there is a '.'
    digit_count -= (start - start_digits);
    if (digit_count >= 19) {
      // Chances are good that we had an overflow!
      // We start anew.
      // This will happen in the following examples:
      // 10000000000000000000000000000000000000000000e+308
      // 3.1415926535897932384626433832795028841971693993751
      //
      return EARLY_STRTOD;
    }
  }
  if (unlikely(exponent < FASTFLOAT_SMALLEST_POWER) ||
      (exponent > FASTFLOAT_LARGEST_POWER)) {
    // this is almost never going to get called!!!
    // exponent could be as low as 325
    return EARLY_STRTOD;
  }
  // from this point forward, exponent >= FASTFLOAT_SMALLEST_POWER and
  // exponent <= FASTFLOAT_LARGEST_POWER
  return compute_float_64_stats(exponent, i);
}

void random_floats(bool ininterval) {
  printf("** Generating random floats ");
  if (ininterval)
    printf("in interval [0,1]\n");
  else
    printf(" (all normals)\n");
  size_t counters[] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint64_t offset = 1190;
  size_t howmany = 10000000;
  for (size_t i = 1; i <= howmany; i++) {
    if ((i % 100000) == 0) {
      printf(".");
      fflush(NULL);
    }
    uint64_t x = rng(i + offset);
    double d;

    if (ininterval) {
      x &= 9007199254740991;
      d = (double)x / 9007199254740992.;
    } else {
      ::memcpy(&d, &x, sizeof(double));
      // paranoid
      while ((!std::isnormal(d)) || std::isnan(d) || std::isinf(d)) {
        offset++;
        x = rng(i + offset);
        ::memcpy(&d, &x, sizeof(double));
      }
    }
    std::string s(64, '\0');
    auto written = std::snprintf(&s[0], s.size(), "%.*e", DBL_DIG + 1, d);
    s.resize(written);
    int state = parse_number_stats(s.data());
    counters[state] += 1;
  }
  size_t count = howmany;
  printf("==========\n");
  printf("fast path %zu (%.5f %%) \n", counters[FAST_PATH],
         counters[FAST_PATH] * 100. / count);
  printf("zero path %zu (%.5f %%) \n", counters[ZERO_PATH],
         counters[ZERO_PATH] * 100. / count);
  printf("slow path %zu  (%.5f %%) \n", counters[SLOW_PATH],
         counters[SLOW_PATH] * 100. / count);
  printf("slower path %zu (%.5f %%) \n", counters[SLOWER_PATH],
         counters[SLOWER_PATH] * 100. / count);
  printf("failure %zu (%.5f %%) \n", counters[FAILURE],
         counters[FAILURE] * 100. / count);
  printf("could not round %zu (%.5f %%) \n", counters[COULD_NOT_ROUND],
         counters[COULD_NOT_ROUND] * 100. / count);
  printf("exponent failure %zu  (%.5f %%) \n", counters[EXPONENT_FAILURE],
         counters[EXPONENT_FAILURE] * 100. / count);
  printf("early bail %zu (%.5f %%) \n", counters[EARLY_STRTOD],
         counters[EARLY_STRTOD] * 100. / count);
}

void fileload(char *filename) {
  size_t counters[] = {0, 0, 0, 0, 0, 0, 0, 0};

  std::ifstream inputfile(filename);
  if (!inputfile) {
    std::cerr << "can't open " << filename << std::endl;
    return;
  }
  std::string line;
  size_t count = 0;
  while (getline(inputfile, line)) {
    int state = parse_number_stats(line.data());
    counters[state] += 1;
    count++;
  }
  std::cout << "read " << count << " lines " << std::endl;

  printf("==========\n");
  printf("fast path %zu (%.5f %%) \n", counters[FAST_PATH],
         counters[FAST_PATH] * 100. / count);
  printf("zero path %zu (%.5f %%) \n", counters[ZERO_PATH],
         counters[ZERO_PATH] * 100. / count);
  printf("slow path %zu  (%.5f %%) \n", counters[SLOW_PATH],
         counters[SLOW_PATH] * 100. / count);
  printf("slower path %zu (%.5f %%) \n", counters[SLOWER_PATH],
         counters[SLOWER_PATH] * 100. / count);
  printf("failure %zu (%.5f %%) \n", counters[FAILURE],
         counters[FAILURE] * 100. / count);
  printf("could not round %zu (%.5f %%) \n", counters[COULD_NOT_ROUND],
         counters[COULD_NOT_ROUND] * 100. / count);
  printf("exponent failure %zu  (%.5f %%) \n", counters[EXPONENT_FAILURE],
         counters[EXPONENT_FAILURE] * 100. / count);
  printf("early bail %zu (%.5f %%) \n", counters[EARLY_STRTOD],
         counters[EARLY_STRTOD] * 100. / count);
}

int main(int argc, char **argv) {
  if (argc == 1) {
    random_floats(false);
    random_floats(true);
    std::cout << "You can also provide a filename: it should contain one "
                 "string per line corresponding to a number"
              << std::endl;
  } else {
    fileload(argv[1]);
  }

  return EXIT_SUCCESS;
}
