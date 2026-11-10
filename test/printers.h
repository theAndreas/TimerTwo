// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// How GoogleTest prints an Error, a Status and a State that an expectation
// did not meet: by name, rather than as the bytes of an object.

#ifndef TIMERTWO_TEST_PRINTERS_H_
#define TIMERTWO_TEST_PRINTERS_H_

#include <avr/pgmspace.h>

#include <ostream>

#include "timer_two/status.h"
#include "timer_two/timer.h"

namespace timer_two {

// The text of an error, as the library has it in flash, which the host reads
// as RAM.
inline char const* TextOf(Error error) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<char const*>(Status{error}.GetErrorText());
}

// GoogleTest finds these by their name, in the namespace of the type.
// NOLINTNEXTLINE(readability-identifier-naming)
inline void PrintTo(Error error, std::ostream* out) { *out << TextOf(error); }

// NOLINTNEXTLINE(readability-identifier-naming)
inline void PrintTo(Status const& status, std::ostream* out) {
  *out << TextOf(status.GetError());
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline void PrintTo(State state, std::ostream* out) {
  switch (state) {
    case State::kNotBegun:
      *out << "not begun";
      break;
    case State::kIdle:
      *out << "idle";
      break;
    case State::kRunning:
      *out << "running";
      break;
    case State::kStopped:
      *out << "stopped";
      break;
  }
}

}  // namespace timer_two

#endif  // TIMERTWO_TEST_PRINTERS_H_
