# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The tests on the host: the library against a mock of the Arduino core, on GoogleTest. The
# library is built as gnu++11, as avr-gcc builds it for Arduino; only the tests are C++17,
# which GoogleTest needs. The warnings and the presets' instrumentation come from
# cmake/checks.cmake.
#
#   cmake --workflow --preset default    # the tests
#   cmake --workflow --preset sanitize   # under ASan and UBSan
#   tools/coverage.sh                    # the tests, and what they reach

cmake_minimum_required(VERSION 3.25)
project(TimerTwo LANGUAGES CXX)
enable_testing()
# For clangd, which reads it through .clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/checks.cmake)
find_package(GTest REQUIRED)
include(GoogleTest)

# Recursive, as a module has a folder of its own (docs/CodingStyle.md F-12). A test is named as
# the class it tests and Test, or a family of functions and _test (Testing.md TS-5); the mock
# and the cases of what must not compile are no tests of their own.
file(GLOB_RECURSE library_sources CONFIGURE_DEPENDS src/timer_two/*.cpp)
file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS test/*Test.cpp test/*_test.cpp)
list(FILTER test_sources EXCLUDE REGEX "/test/(mock|compile_fail)/")

# One library and one test program per variant: a switch that changes what is compiled makes
# a build of its own. A variant is added with its defines, as add_variant(checked DEFINES
# TIMERTWO_CHECK_ACCESS=1), and in tools/project.sh for the examples. CLOCK is F_CPU, the
# Uno's 16 MHz unless given; TESTS the tests it runs, all of test/ unless given.
function(add_variant name)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "CLOCK" "DEFINES;TESTS")
  if(NOT arg_CLOCK)
    set(arg_CLOCK 16000000uL)
  endif()
  if(NOT arg_TESTS)
    set(arg_TESTS ${test_sources})
  endif()

  add_library(timer_two_${name} STATIC ${library_sources} test/mock/Arduino.cpp
                                       test/mock/avr/io.cpp)
  target_include_directories(timer_two_${name} PUBLIC src test/mock)
  # The CPU's clock, which the board's build defines.
  target_compile_definitions(timer_two_${name} PUBLIC F_CPU=${arg_CLOCK} ${arg_DEFINES})
  target_compile_options(timer_two_${name} PRIVATE -std=gnu++11 ${CHECK_WARNINGS}
                                                  ${CHECK_INSTRUMENT})
  target_link_options(timer_two_${name} PUBLIC ${CHECK_INSTRUMENT})

  add_executable(timer_two_test_${name} ${arg_TESTS})
  target_compile_features(timer_two_test_${name} PRIVATE cxx_std_17)
  # The root, so that a test includes its helpers as "test/...", as Google wants an include
  # to name the directory.
  target_include_directories(timer_two_test_${name} PRIVATE ${CMAKE_SOURCE_DIR})
  target_compile_options(timer_two_test_${name} PRIVATE ${CHECK_WARNINGS}
                                                       ${CHECK_INSTRUMENT})
  target_link_libraries(timer_two_test_${name} PRIVATE timer_two_${name}
                                                      GTest::gtest_main)
  # A test that waits for a counter that stands would hang the run: it fails after 10 s.
  gtest_discover_tests(timer_two_test_${name} TEST_PREFIX ${name}/ PROPERTIES TIMEOUT 10)
endfunction()

add_variant(default)
# The clocks where TimerTwo computed with a quotient of F_CPU that was cut or 0 (OIL 2): the
# tests that hold at any clock, as TimeType is 32 bits below 8 MHz.
add_variant(1mhz CLOCK 1000000uL TESTS test/ClockTest.cpp)
add_variant(20mhz CLOCK 20000000uL TESTS test/ClockTest.cpp)

add_compile_fail_tests(DIRECTORY ${CMAKE_SOURCE_DIR}/test/compile_fail STANDARD gnu++11
                       INCLUDES ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/test/mock)
