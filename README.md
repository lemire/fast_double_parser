# fast_double_parser
Fast function to parse strings containing decimal numbers into double-precision (binary64) floating-point values.  That is, given the string "1.0e10", it should return a 64-bit floating-point value equal to 10. We do not sacrifice accuracy. The function will match exactly (down the smallest bit) the result of a standard function like strtod.

## Why should I expect this function to be faster?

Parsing strings into binary numbers (IEEE 754) is surprisingly difficult. Parsing a single number can take hundreds of instructions and CPU cycles, if not thousands. It is relatively easy to parse numbers faster if you sacrifice accuracy (e.g., tolerate 1 ULP errors), but we are interested in "perfect" parsing.

Instead of trying to solve the general problem, we cover what we believe are the most common scenarios, providing really fast parsing. We fall back on the standard library for the difficult cases. We believe that, in this manner, we achieve the best performance on the most important cases. 

We expect string numbers to follow [RFC 7159](https://tools.ietf.org/html/rfc7159).

## Requirements

You should be able to just drop  the header file into your project, it is a header-only library.

If you want to run our benchmarks, you should have

- Linux or macOS 
- A recent C++ compiler with make
- A recent cmake (one dependency requires cmake 3.5 or better) is necessary for the benchmarks 

## Usage (benchmarks)

```
make bench
```

To run unit tests, type `make unit && ./unit`.

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

## Credit

Contributions are invited.

This is joint work with Michael Eisel.
