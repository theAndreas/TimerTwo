// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The header a sketch includes, before any other of the library.
 *
 * It includes all of the library. arduino-cli finds a library only by a
 * header that lies directly in its `src/`, not by one in a folder below, and
 * it goes through a sketch's includes in order. Once this header has found
 * the library, `src/` is on the include path, and a header below it can be
 * included by its path in every file after it.
 *
 * \code{.cpp}
 * #include <TimerTwo.h>
 *
 * void OnTick() { ... }  // every millisecond
 *
 * void setup() {
 *   timer_two::Discard(timer_two::timer2.Begin(1000, OnTick));
 *   timer_two::Discard(timer_two::timer2.Start());
 * }
 * \endcode
 */

// The name is the library's, as Arduino expects, not lower case as Google's.
// The export pragmas tell clang-tidy's include-cleaner that this header
// provides what it includes.

#ifndef TIMERTWO_SRC_TIMERTWO_H_
#define TIMERTWO_SRC_TIMERTWO_H_

#include "timer_two/attributes.h"  // IWYU pragma: export
#include "timer_two/status.h"      // IWYU pragma: export
#include "timer_two/timer.h"       // IWYU pragma: export
#include "timer_two/timer2.h"      // IWYU pragma: export

#endif  // TIMERTWO_SRC_TIMERTWO_H_
