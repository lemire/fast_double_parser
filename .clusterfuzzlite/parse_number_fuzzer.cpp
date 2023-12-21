#include <fast_double_parser.h>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  std::string fuzz_input(reinterpret_cast<const char *>(data), size);

  double x;
  fast_double_parser::parse_number(fuzz_input.c_str(), &x);

  return 0;
}
