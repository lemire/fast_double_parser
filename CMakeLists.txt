cmake_minimum_required(VERSION 3.11)
cmake_policy(SET CMP0048 NEW)

project(fast_double_parser LANGUAGES CXX VERSION 0.0.0.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

option(FAST_DOUBLE_PARSER_SANITIZE "Sanitize addresses" OFF)

set(headers include/fast_double_parser.h)
set(unit_src tests/unit.cpp)
set(bogus_src tests/bogus.cpp)
set(rebogus_src tests/bogus.cpp)

set(benchmark_src benchmarks/benchmark.cpp)


add_library(fast_double_parser INTERFACE)
target_include_directories(fast_double_parser
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

include(GNUInstallDirs)
install(FILES ${headers} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(TARGETS fast_double_parser EXPORT fast_double_parser-targets)
install(
    EXPORT fast_double_parser-targets
    DESTINATION "share/fast_double_parser"
    NAMESPACE fast_double_parser::
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/fast_double_parser-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/fast_double_parser-config.cmake"
  INSTALL_DESTINATION "share/fast_double_parser"
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/fast_double_parser-config.cmake"
  DESTINATION "share/fast_double_parser"
)

option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    add_executable(unit ${unit_src} ${bogus_src} ${rebogus_src})

    if(FAST_DOUBLE_PARSER_SANITIZE)
      target_compile_options(unit PUBLIC -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
      target_link_options(unit PUBLIC -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
      # Ubuntu bug for GCC 5.0+ (safe for all versions)
      if (CMAKE_COMPILER_IS_GNUCC AND NOT APPLE)
        target_link_libraries(unit PUBLIC -fuse-ld=gold)
      endif()
    endif()
    target_link_libraries(unit PRIVATE fast_double_parser)

    enable_testing()
    add_test(unit unit)
endif()

option(FAST_DOUBLE_BENCHMARKS "include benchmarks" OFF)



if(FAST_DOUBLE_BENCHMARKS)

    include(FetchContent)
    include(ExternalProject)

    set(ABSL_ENABLE_INSTALL ON)
    set(ABSL_RUN_TEST OFF CACHE INTERNAL "")
    set(ABSL_USE_GOOGLETEST_HEAD OFF CACHE INTERNAL "")

    FetchContent_Declare(abseil 
         GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git 
         GIT_TAG "20210324.2")
    FetchContent_GetProperties(abseil)
    if(NOT abseil_POPULATED)
        set(BUILD_TESTING OFF)
        FetchContent_Populate(abseil)
        add_subdirectory(${abseil_SOURCE_DIR} ${abseil_BINARY_DIR})
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${abseil_SOURCE_DIR}/absl/copts)
        include(${abseil_SOURCE_DIR}/absl/copts/AbseilConfigureCopts.cmake)
    endif()

    FetchContent_Declare(doubleconversion 
        GIT_REPOSITORY https://github.com/google/double-conversion.git 
        GIT_TAG "v3.1.5")
    FetchContent_GetProperties(doubleconversion)
    FetchContent_MakeAvailable(doubleconversion)

    add_executable(benchmark ${benchmark_src})
    target_link_libraries(benchmark PUBLIC double-conversion absl::strings)
    target_include_directories(benchmark PUBLIC include)
endif(FAST_DOUBLE_BENCHMARKS)
