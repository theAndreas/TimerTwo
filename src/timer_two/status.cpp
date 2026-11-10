// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "timer_two/status.h"

#include <avr/pgmspace.h>
#include <stdint.h>

namespace timer_two {
namespace {

// The texts and the table of them lie in flash: a switch that returned F("...")
// for each error would be turned by avr-gcc into a table of pointers that the
// Uno copies into RAM.
char const kTextOk[] PROGMEM = "ok";
char const kTextNotBegun[] PROGMEM = "not begun";
char const kTextBegunAlready[] PROGMEM = "begun already";
char const kTextRunning[] PROGMEM = "running";
char const kTextNotRunning[] PROGMEM = "not running";
char const kTextNotStopped[] PROGMEM = "not stopped";
char const kTextPeriodOutOfRange[] PROGMEM = "period out of range";
char const kTextNoCallback[] PROGMEM = "no callback";
char const kTextUnknownError[] PROGMEM = "unknown error";

// In the order of Error. An array, as PROGMEM takes one.
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
char const* const kTexts[] PROGMEM = {
    kTextOk,         kTextNotBegun,   kTextBegunAlready,     kTextRunning,
    kTextNotRunning, kTextNotStopped, kTextPeriodOutOfRange, kTextNoCallback,
};

constexpr uint8_t kTextCount{sizeof(kTexts) / sizeof(kTexts[0])};

static_assert(kTextCount == static_cast<uint8_t>(Error::kNoCallback) + 1,
              "every error has a text");

}  // namespace

__FlashStringHelper const* Status::GetErrorText() const {
  uint8_t const index{static_cast<uint8_t>(error_)};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  void const* const text{index < kTextCount ? pgm_read_ptr(&kTexts[index])
                                            : kTextUnknownError};
  // The type Serial.print() takes for a text in flash, as F() makes it.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<__FlashStringHelper const*>(text);
}

}  // namespace timer_two
