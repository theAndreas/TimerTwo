# Copyright 2026 Andreas Burnickl
# SPDX-License-Identifier: GPL-3.0-or-later
#
# What every build on the host is checked with, for CMakeLists.txt to include: the warnings,
# the instrumentation of the presets sanitize and coverage, and the tests of what must not
# compile. This file comes from the template and is brought up to date with `copier update`;
# what the project builds is in CMakeLists.txt.
#
#   CHECK_WARNINGS     the warnings, for target_compile_options(); errors only where
#                      CMAKE_COMPILE_WARNING_AS_ERROR is on, as the ci- presets set it
#   CHECK_INSTRUMENT   --coverage, or the sanitizers, for the compile and the link options
#   add_compile_fail_tests(DIRECTORY dir STANDARD std INCLUDES dir...)

option(TIMERTWO_COVERAGE "Build with --coverage, for gcovr" OFF)
option(TIMERTWO_SANITIZE "Build with AddressSanitizer and UBSan" OFF)

# -Wconversion and -Wsign-conversion above all: an int has 16 bits on the AVR, and a
# conversion that is harmless on the host can lose a value there. Braced initialization finds
# a narrowing only where a variable is initialized, not in the arithmetic after it. -Wshadow,
# as a name that hides another is read as the other. No -Werror: CMAKE_COMPILE_WARNING_AS_ERROR
# decides, on in CI and under `make check`, off while somebody is in the middle of debugging.
set(CHECK_WARNINGS -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow)

set(CHECK_INSTRUMENT)
if(TIMERTWO_COVERAGE)
  list(APPEND CHECK_INSTRUMENT --coverage)
endif()
if(TIMERTWO_SANITIZE)
  list(APPEND CHECK_INSTRUMENT -fsanitize=address,undefined
       -fno-sanitize-recover=all -fno-omit-frame-pointer)
endif()

# What must not compile: each .cpp file in DIRECTORY uses the library wrongly, and its test
# passes only if the compiler stops it with the message of the static_assert that its line
# "// Expected: ..." names. A static_assert that went missing, or another error, fails it. The
# compiler is called directly, with -fsyntax-only and in STANDARD, as avr-gcc builds a sketch:
# no target, so that tests run in parallel never build in the same tree at once.
function(add_compile_fail_tests)
  cmake_parse_arguments(PARSE_ARGV 0 arg "" "DIRECTORY;STANDARD" "INCLUDES")
  set(includes)
  foreach(dir IN LISTS arg_INCLUDES)
    list(APPEND includes -I${dir})
  endforeach()
  file(GLOB cases CONFIGURE_DEPENDS ${arg_DIRECTORY}/*.cpp)
  foreach(case IN LISTS cases)
    get_filename_component(name ${case} NAME_WE)
    file(STRINGS ${case} expected REGEX "^// Expected: " LIMIT_COUNT 1)
    string(REGEX REPLACE "^// Expected: " "" expected "${expected}")
    if(expected STREQUAL "")
      message(FATAL_ERROR "${case} names no message: a line \"// Expected: \".")
    endif()
    add_test(NAME compile_fail/${name}
             COMMAND ${CMAKE_CXX_COMPILER} -std=${arg_STANDARD} -fsyntax-only
                     ${includes} ${case})
    # GCC says "static assertion failed: <message>", Clang "static assertion failed due to
    # requirement '...': <message>"; both on one line.
    set_tests_properties(
      compile_fail/${name}
      PROPERTIES PASS_REGULAR_EXPRESSION
                 "static assertion failed[^\n]*${expected}")
  endforeach()
endfunction()
