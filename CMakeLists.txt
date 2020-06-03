cmake_policy(SET CMP0048 NEW)
project(fast_double_parser LANGUAGES CXX VERSION 0.0.0.0)
cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

option(FAST_DOUBLE_PARSER_SANITIZE "Sanitize addresses" OFF)

add_subdirectory(benchmarks/dependencies/abseil-cpp)
add_subdirectory(benchmarks/dependencies/double-conversion)

set(headers include/fast_double_parser.h)
set(unit_src tests/unit.cpp)
set(stats_src tests/stats.cpp)

set(benchmark_src benchmarks/benchmark.cpp)

add_executable(unit ${unit_src})
if(FAST_DOUBLE_PARSER_SANITIZE)
  target_compile_options(unit PUBLIC -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_options(unit PUBLIC -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  # Ubuntu bug for GCC 5.0+ (safe for all versions)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(unit PUBLIC -fuse-ld=gold)
  endif()
endif()

target_include_directories(unit PUBLIC include)
enable_testing()
add_test(unit unit)

add_executable(stats ${stats_src})
target_include_directories(stats PUBLIC include)

add_executable(benchmark ${benchmark_src})
target_link_libraries(benchmark PUBLIC double-conversion absl_strings)
target_include_directories(benchmark PUBLIC include)
