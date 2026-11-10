// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// avr-libc's ISR(), for the tests on the host: the handler is a function of C
// linkage by the vector's name, which a test calls in place of the
// interrupt.

#ifndef TIMERTWO_TEST_MOCK_AVR_INTERRUPT_H_
#define TIMERTWO_TEST_MOCK_AVR_INTERRUPT_H_

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ISR(vector) extern "C" void vector()

// The handler of Timer2's overflow, which a test calls.
extern "C" void TIMER2_OVF_vect();  // NOLINT(readability-identifier-naming)

#endif  // TIMERTWO_TEST_MOCK_AVR_INTERRUPT_H_
