#!/bin/bash -eu

# Copy all fuzzer executable to $OUT/
$CXX $CFLAGS $LIB_FUZZING_ENGINE \
  $SRC/fast_double_parser/.clusterfuzzlite/parse_number_fuzzer.cpp \
  -o $OUT/parse_number_fuzzer \
  -I$SRC/fast_double_parser/include
