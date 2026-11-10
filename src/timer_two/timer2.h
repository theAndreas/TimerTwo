// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief timer2, Timer/Counter2 of the ATmega328P as a Timer.
 *
 * It breaks analogWrite() on pins 3 and 11, and tone(), which use Timer2 as
 * well. Pin 11, OC2A, gives no PWM: OCR2A is the top of the counter.
 */

#ifndef TIMERTWO_SRC_TIMER_TWO_TIMER2_H_
#define TIMERTWO_SRC_TIMER_TWO_TIMER2_H_

#include <avr/io.h>
#include <stdint.h>

#include "timer_two/timer.h"

namespace timer_two {

/*!
 * \brief The registers of Timer/Counter2, for Timer.
 *
 * Each returns the register as the vendor's header declares it, so that the
 * compiler writes it where the datasheet says.
 */
struct Timer2Registers {
  /*! \brief The Arduino pin of OC2B, PD3, which gives the PWM. */
  static constexpr uint8_t kPwmPin{3};

  /*! \brief TCCR2A. */
  static auto Tccra() -> decltype((TCCR2A)) { return TCCR2A; }
  /*! \brief TCCR2B. */
  static auto Tccrb() -> decltype((TCCR2B)) { return TCCR2B; }
  /*! \brief OCR2A, the top. */
  static auto Ocra() -> decltype((OCR2A)) { return OCR2A; }
  /*! \brief OCR2B, the compare value of the PWM. */
  static auto Ocrb() -> decltype((OCR2B)) { return OCR2B; }
  /*! \brief TIMSK2. */
  static auto Timsk() -> decltype((TIMSK2)) { return TIMSK2; }
  /*! \brief TIFR2, whose flags are cleared by writing 1. */
  static auto Tifr() -> decltype((TIFR2)) { return TIFR2; }
  /*! \brief TCNT2, the counter. */
  static auto Tcnt() -> decltype((TCNT2)) { return TCNT2; }
};

/*! \brief The type of timer2, for its constants: `Timer2::kPeriodMax`. */
using Timer2 = Timer<Timer2Registers>;

/*!
 * \brief Timer/Counter2; its overflow interrupt calls the callback.
 *
 * Made when compiling, by the constexpr constructor: there is no order of
 * construction to mind, and it can be used from the first line of setup().
 */
// The one object a sketch shares with the interrupt, as Serial is.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern Timer2 timer2;

}  // namespace timer_two

#endif  // TIMERTWO_SRC_TIMER_TWO_TIMER2_H_
