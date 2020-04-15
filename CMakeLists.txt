cmake_policy(SET CMP0048 NEW)
project(fast_double_parser LANGUAGES CXX VERSION 0.0.0.0)
cmake_minimum_required(VERSION 3.11)

add_subdirectory(benchmarks/dependencies/abseil-cpp)
add_subdirectory(benchmarks/dependencies/double-conversion)

set(headers include/fast_double_parser.h)
set(unit_src tests/unit.cpp)
set(stats_src tests/stats.cpp)

set(benchmark_src benchmarks/benchmark.cpp)

add_executable(unit ${unit_src})
target_include_directories(unit PUBLIC include)

add_executable(stats ${stats_src})
target_include_directories(stats PUBLIC include)

add_executable(benchmark ${benchmark_src})
target_link_libraries(benchmark PUBLIC double-conversion absl_strings)
target_include_directories(benchmark PUBLIC include)