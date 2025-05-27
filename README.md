# fast_double_parser: 4x faster than strtod
![MSYS2-CI](https://github.com/lemire/fast_double_parser/workflows/MSYS2-CI/badge.svg)[![Ubuntu 22.04](https://github.com/lemire/fast_double_parser/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/lemire/fast_double_parser/actions/workflows/ubuntu.yml)

###  :exclamation: :exclamation: :exclamation: :exclamation: We encourage users to adopt [fast_float](https://github.com/fastfloat/fast_float?tab=readme-ov-file#you-may-also-enforce-the-json-format-rfc-8259) library instead of fast_double_parser. It has more functionality. It provides full support for the JSON syntax and the same great speed. The fast_float library is part of most major Web browsers, GCC and so forth: it is well tested and supported. :exclamation: :exclamation: :exclamation: :exclamation:


## Introduction


Fast function to parse ASCII strings containing decimal numbers into double-precision (binary64) floating-point values.  That is, given the string "1.0e10", it should return a 64-bit floating-point value equal to 10000000000. We do not sacrifice accuracy. The function will match exactly (down the smallest bit) the result of a standard function like `strtod`.

We support all major compilers: Visual Studio, GNU GCC, LLVM Clang. We require C++11.

The core of this library was ported to Go by Nigel Tao and is now a standard float-parsing routine in Go (`strconv.ParseFloat`). 


## Reference

- Daniel Lemire, [Number Parsing at a Gigabyte per Second](https://arxiv.org/abs/2101.11408), Software: Practice and Experience 51 (8), 2021.



## Usage

You should be able to just drop  the header file into your project, it is a header-only library.


The current API is simple enough:

```C++
#include "fast_double_parser.h" // the file is in the include directory


double x;
char * string = ...
const char * endptr = fast_double_parser::parse_number(string, &x);
```

You must check the value returned (`endptr`): if it is `nullptr`, then the function refused to parse the input.
Otherwise, we return a pointer (`const char *`) to the end of the parsed string. The provided
pointer (`string`) should point at the beginning of the number: if you must skip whitespace characters,
it is your responsibility to do so.


We expect string numbers to follow [RFC 7159](https://tools.ietf.org/html/rfc7159) (JSON standard). In particular,
the parser will reject overly large values that would not fit in binary64. It will not accept
NaN or infinite values.

It works much like the C standard function `strtod` expect that the parsing is locale-independent. E.g., it will parse 0.5 as 1/2, but it will not parse 0,5 as
1/2 even if you are under a French system. Locale independence is by design (it is not a limitation). Like the standard C functions, it expects that the string
representation of your number ends with a non-number character (e.g., a null character, a space, a colon, etc.). If you wish the specify the end point of the string, as is common in C++, please consider the [fast_float](https://github.com/lemire/fast_float) C++ library instead.


We assume that the rounding mode is set to nearest, the default setting (`std::fegetround() == FE_TONEAREST`). It is uncommon to have a different setting.

## What if I prefer another API?

The [fast_float](https://github.com/lemire/fast_float) offers an API resembling that of the C++17 `std::from_chars` functions. In particular, you can specify the beginning and the end of the string.
Furthermore [fast_float](https://github.com/lemire/fast_float) supports both 32-bit and 64-bit floating-point numbers. The  [fast_float](https://github.com/lemire/fast_float) library is part of Apache Arrow, GCC 12, Safari/WebKit and other important systems.

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
NaN or infinite values. It will refuse to parse `001` or `0.` as these are invalid number strings as
per the [JSON specification](https://tools.ietf.org/html/rfc7159). Users who prefer a more
lenient C++ parser may consider the [fast_float](https://github.com/lemire/fast_float) C++ library.

The parsing is locale-independent. E.g., it will parse 0.5 as 1/2, but it will not parse 0,5 as
1/2 even if you are under a French system.


## Requirements


If you want to run our benchmarks, you should have

- Windows, Linux or macOS; presumably other systems can be supported as well
- A recent C++ compiler
- A recent cmake (cmake 3.11 or better) is necessary for the benchmarks

This code falls back on your platform's `strdtod_l` /`_strtod_l` implementation for numbers with a long decimal mantissa (more than 19 digits).

## Usage (benchmarks)

```
git clone https://github.com/lemire/fast_double_parser.git
cd fast_double_parser
mkdir build
cd build
cmake .. -DFAST_DOUBLE_BENCHMARKS=ON
cmake --build . --config Release  
ctest .
./benchmark
```
Under Windows, the last line should be `./Release/benchmark.exe`.

Be mindful that the benchmarks include the abseil library which is not supported everywhere.

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


## Ports and users

- The algorithm is part of the [LLVM standard libraries](https://github.com/llvm/llvm-project/commit/87c016078ad72c46505461e4ff8bfa04819fe7ba). 
- This library has been ported to Go and integrated in the Go standard library.
- It is part of the [Microsoft LightGBM machine-learning framework](https://github.com/microsoft/LightGBM).
- It is part of the database engine [noisepage](https://github.com/cmu-db/noisepage).
- The library has been reimplemented in [Google wuffs](https://github.com/google/wuffs/).
- [There is a Julia port](https://github.com/JuliaData/Parsers.jl).
- [There is a Rust port](https://github.com/ezrosent/frawk/tree/master/src/runtime/float_parse).
- [There is a Java port](https://github.com/wrandelshofer/FastDoubleParser).
- [There is a C# port](https://github.com/CarlVerret/csFastFloat).
- [Bazel Central Registry](https://registry.bazel.build/modules/fast_double_parser).

## Credit

Contributions are invited.

This is based on an original idea by Michael Eisel (joint work).

## License

You may use this code under either the Apache License 2.0 or
the Boost Software License 1.0.
