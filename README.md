# fast_double_parser
[![Build Status](https://cloud.drone.io/api/badges/lemire/fast_double_parser/status.svg)](https://cloud.drone.io/lemire/fast_double_parser) [![Build status](https://ci.appveyor.com/api/projects/status/y7215jgem4ggswnj/branch/master?svg=true)](https://ci.appveyor.com/project/lemire/fast-double-parser/branch/master)


Fast function to parse strings containing decimal numbers into double-precision (binary64) floating-point values.  That is, given the string "1.0e10", it should return a 64-bit floating-point value equal to 10000000000. We do not sacrifice accuracy. The function will match exactly (down the smallest bit) the result of a standard function like strtod.

We support all major compilers: Visual Studio, GNU GCC, LLVM Clang. We require C++11.

## Why should I expect this function to be faster?

Parsing strings into binary numbers (IEEE 754) is surprisingly difficult. Parsing a single number can take hundreds of instructions and CPU cycles, if not thousands. It is relatively easy to parse numbers faster if you sacrifice accuracy (e.g., tolerate 1 ULP errors), but we are interested in "perfect" parsing.

Instead of trying to solve the general problem, we cover what we believe are the most common scenarios, providing really fast parsing. We fall back on the standard library for the difficult cases. We believe that, in this manner, we achieve the best performance on some of the most important cases. 

We have benchmarked our parser on a collection of strings from a sample geojson file (canada.json). Here are some of our results:


| parser                                | MB/s |
| ------------------------------------- | ---- |
| fast_double_parser                    | 660 MB/s  |
| abseil, from_chars                    | 330 MB/s |
| double_conversion                     | 250 MB/s |
| strtod                    | 70 MB/s |

(configuration: Apple clang version 11.0.0, I7-7700K)

We expect string numbers to follow [RFC 7159](https://tools.ietf.org/html/rfc7159). In particular,
the parser will reject overly large values that would not fit in binary64. It will not produce
NaN or infinite values.

## Requirements

You should be able to just drop  the header file into your project, it is a header-only library.

If you want to run our benchmarks, you should have

- Windows, Linux or macOS; presumably other systems can be supported as well
- A recent C++ compiler
- A recent cmake (cmake 3.11 or better) is necessary for the benchmarks 

## Usage (benchmarks)

```
cd build
cmake ..
cmake --build . --config Release  
ctest .
./benchmark
```
Under Windows, the last like should be `./Release/benchmark.exe`.


## Sample results


```
$ ./benchmark 
parsing random integers in the range [0,1)


=== trial 1 ===
fast_double_parser  460.64 MB/s
strtod         186.90 MB/s
abslfromch     168.61 MB/s
absl           140.62 MB/s
double-conv    206.15 MB/s


=== trial 2 ===
fast_double_parser  449.76 MB/s
strtod         174.59 MB/s
abslfromch     152.68 MB/s
absl           157.52 MB/s
double-conv    193.97 MB/s


```

```
$ ./benchmark benchmarks/data/canada.txt
read 111126 lines 


=== trial 1 ===
fast_double_parser  662.01 MB/s
strtod         69.73 MB/s
abslfromch     341.74 MB/s
absl           325.23 MB/s
double-conv    249.68 MB/s


=== trial 2 ===
fast_double_parser  611.56 MB/s
strtod         69.53 MB/s
abslfromch     330.00 MB/s
absl           328.45 MB/s
double-conv    243.90 MB/s
```

## API

The current API is simple enough:

```C++
#include "fast_double_parser.h" // the file is in the include directory


double x;
char * string = ...
bool isok = fast_double_parser::parse_number(string, &x);
```

You must check the value of the boolean (`isok`): if it is false, then the function refused to parse.


## GCC and x86

We rely on the compiler rounding to the nearest even when doing floating-point operations (divisions and multiplications). Under x86 (32-bit), GNU GCC does not always evaluate divisions between two floats using round-to-even. It may be necessary to compile the code with the `-mpc64` flag. Demonstrations:

```C++
/*************
# g++ -o round round.cpp   && ./round 
x/y                          = 0.501782303179999944
expected x/y (round to even) = 0.501782303180000055
                               Equal
                              Not equal
FE_DOWNWARD  : 0.501782303179999944
FE_TONEAREST : 0.501782303179999944
FE_TOWARDZERO: 0.501782303179999944
FE_UPWARD    : 0.501782303180000056

************/

#include <fenv.h>

#include <iostream>

#pragma STDC FENV_ACCESS ON

#include <stdio.h>
int main() {
  double x = 50178230318.0;
  double y = 100000000000.0;
  double ratio = x/y;
  printf("x/y                          = %18.18f\n", x / y);
  printf("expected x/y (round to even) = %18.18f\n", 0.501782303180000055);
  printf("                              ");
  bool is_correct = (50178230318.0 / 100000000000.0 == 0.501782303180000055);
  std::cout << (is_correct ? " Equal" : "Not equal") << std::endl;
  printf("                              ");
  is_correct = (ratio == 0.501782303180000055);
  std::cout << (is_correct ? " Equal" : "Not equal") << std::endl;
  fesetround(FE_DOWNWARD);
  printf("FE_DOWNWARD  : %18.18f\n", x / y);

  fesetround(FE_TONEAREST);
  printf("FE_TONEAREST : %18.18f\n", x / y);

  fesetround(FE_TOWARDZERO);
  printf("FE_TOWARDZERO: %18.18f\n", x / y);

  fesetround(FE_UPWARD);
  printf("FE_UPWARD    : %18.18f\n", x / y);

  fesetround(FE_TONEAREST);

  return 0;
}
```


## Credit

Contributions are invited.

This is based on an original idea by Michael Eisel.
