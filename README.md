# fast_double_parser
Fast function to parse strings containing decimal numbers into double-precision (binary64) floating-point values.  That is, given the string "1.0e10", it should return a 64-bit floating-point value equal to 10. We do not sacrifice accuracy. The function will match exactly (down the smallest bit) the result of a standard function like strtod.

## Why should I expect this function to be faster?

Parsing strings into binary numbers (IEEE 754) is surprisingly difficult. Parsing a single number can take hundreds of instructions and CPU cycles, if not thousands.

Instead of trying to solve the general problem, we cover what we believe are the most common scenarios, providing really fast parsing. We fall back on the standard library for the difficult cases.

We expect string numbers to follow [RFC 7159](https://tools.ietf.org/html/rfc7159).

## Requirements

You should be able to just drop  the header file into your project, it is a header-only library.

If you want to run our benchmarks, you should have

- Linux or macOS 
- A recent cmake (one dependency requires cmake 3.5 or better)
- A recent compiler 
- git

## Usage (benchmarks)

```
make
./benchmark
./benchmark data/canada.txt
```

## Credit

Contributions are invited.

This is joint work with Michael Eisel.