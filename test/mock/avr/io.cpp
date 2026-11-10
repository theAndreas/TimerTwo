// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "avr/io.h"

#include <stdint.h>

uint8_t volatile TCCR2A{0};
uint8_t volatile TCCR2B{0};
uint8_t volatile OCR2A{0};
uint8_t volatile OCR2B{0};
uint8_t volatile TIMSK2{0};
MockCounter TCNT2;

namespace {

// The clock select bits CS22..CS20 of TCCR2B.
constexpr uint8_t kClockSelectMask{0x07};

}  // namespace

MockCounter::operator uint8_t() {
  uint8_t const now{value_};
  if ((TCCR2B & kClockSelectMask) != 0) {
    uint8_t const top{OCR2A};
    if (counting_up_ && value_ >= top) {
      counting_up_ = false;
    } else if (!counting_up_ && value_ == 0) {
      counting_up_ = true;
    }
    if (top != 0) {
      value_ = counting_up_ ? static_cast<uint8_t>(value_ + 1)
                            : static_cast<uint8_t>(value_ - 1);
    }
  }
  return now;
}

MockCounter& MockCounter::operator=(uint8_t value) {
  Set(value, true);
  return *this;
}

void MockCounter::Set(uint8_t value, bool counting_up) {
  value_ = value;
  counting_up_ = counting_up;
}
