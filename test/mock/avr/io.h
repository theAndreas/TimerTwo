// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The registers of Timer/Counter2 of the ATmega328P, for the tests on the
// host: each a byte the tests read and set, with the numbers of its bits as
// avr-libc names them. TCNT2 counts as the timer does in mode 5, phase
// correct PWM: one step on each read while a clock is selected, up from 0 to
// OCR2A and down again, so that what waits for the counter to move sees it
// move.

#ifndef TIMERTWO_TEST_MOCK_AVR_IO_H_
#define TIMERTWO_TEST_MOCK_AVR_IO_H_

#include <stdint.h>

// avr-libc's names of the bits, which are macros there; constants by the same
// names are all the library needs.
// NOLINTBEGIN(readability-identifier-naming)
constexpr int WGM20{0};
constexpr int WGM21{1};
constexpr int COM2B0{4};
constexpr int COM2B1{5};
constexpr int COM2A0{6};
constexpr int COM2A1{7};
constexpr int CS20{0};
constexpr int CS21{1};
constexpr int CS22{2};
constexpr int WGM22{3};
constexpr int TOIE2{0};
constexpr int TOV2{0};
// NOLINTEND(readability-identifier-naming)

// TCNT2, which counts on each read while TCCR2B selects a clock.
class MockCounter {
 public:
  // The value the counter has now, after which it steps.
  operator uint8_t();  // NOLINT(google-explicit-constructor)
  // Sets the counter, as a write to TCNT2 does; it goes on upwards.
  MockCounter& operator=(uint8_t value);

  // Sets the counter and its direction, for a test; it steps on each read.
  void Set(uint8_t value, bool counting_up);

  // Lets the counter step only on every reads-th read, as a prescaler makes
  // the CPU read the same value several times.
  void SetReadsPerStep(uint8_t reads);

 private:
  uint8_t value_{0};
  bool counting_up_{true};
  uint8_t reads_per_step_{1};
  uint8_t reads_{0};
};

// avr-libc's names of the registers, globals as they are on the board.
// NOLINTBEGIN(readability-identifier-naming)
extern uint8_t volatile TCCR2A;
extern uint8_t volatile TCCR2B;
extern uint8_t volatile OCR2A;
extern uint8_t volatile OCR2B;
extern uint8_t volatile TIMSK2;
// What was last written to TIFR2: on the board a 1 clears its flag, so a test
// sees which flags were cleared.
extern uint8_t volatile TIFR2;
extern MockCounter TCNT2;
// NOLINTEND(readability-identifier-naming)

#endif  // TIMERTWO_TEST_MOCK_AVR_IO_H_
