// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief TimerTwo, a periodic interrupt, a PWM and a time readout on
 *        Timer/Counter2 of the ATmega328P.
 *
 * An adaption of the Arduino library TimerOne to the 8-bit Timer2. The timer
 * runs in mode 5, phase correct PWM, with OCR2A as its top: the period sets
 * OCR2A and the prescaler, the overflow at the bottom calls the callback, and
 * OC2B on Arduino pin 3 gives the PWM. This breaks analogWrite() on pin 3, and
 * tone(), which uses Timer2 as well.
 */

#ifndef TIMERTWO_SRC_TIMERTWO_H_
#define TIMERTWO_SRC_TIMERTWO_H_

#include <avr/interrupt.h>
#include <avr/io.h>

#include "Arduino.h"
#include "StandardTypes.h"

/*! \brief The width of Timer2's counter, in bits. */
#define TIMERTWO_NUMBER_OF_BITS 8u
/*! \brief The number of values of Timer2's counter. */
#define TIMERTWO_RESOLUTION (1uL << TIMERTWO_NUMBER_OF_BITS)

/*! \brief The Arduino pin of OC2A, PB3, which is not offered: see PwmPinType.
 */
#define TIMERTWO_A_ARDUINO_PIN 11u
/*! \brief The Arduino pin of OC2B, PD3. */
#define TIMERTWO_B_ARDUINO_PIN 3u

/*! \brief The position of the clock select bits CS22..CS20 in TCCR2B. */
#define TIMERTWO_REG_CS_GP 0u
/*! \brief The mask of the clock select bits CS22..CS20 in TCCR2B. */
#define TIMERTWO_REG_CS_GM B111

/*! \brief The largest prescaler of Timer2. */
#define TIMERTWO_MAX_PRESCALER 1024u

/*!
 * \brief Timer/Counter2 as a periodic interrupt, a PWM on pin 3 and a time
 *        readout.
 *
 * There is one, Timer2; init() first, then start(). A function that can
 * refuse returns E_NOT_OK, and leaves the timer as it was, unless it says
 * otherwise.
 */
class TimerTwo {
 public:
  /*! \brief The function the overflow interrupt calls. */
  typedef void (*TimerIsrCallbackF_void)(void);

  /*!
   * \brief A time in microseconds: 32 bits below 8 MHz, where the longest
   *        period does not fit into 16.
   */
#if F_CPU < 8000000uL
  using TimeType = uint32_t;
#else
  using TimeType = uint16_t;
#endif

  /*! \brief Where the timer is in its life. */
  enum StateType {
    STATE_INIT,    /*!< Not initialized: init() comes first. */
    STATE_IDLE,    /*!< Initialized, not started. */
    STATE_RUNNING, /*!< Counting. */
    STATE_STOPPED  /*!< Stopped by stop(). */
  };

  /*! \brief The values of the clock select bits CS22..CS20 of TCCR2B. */
  enum ClockSelectType {
    REG_CS_NO_CLOCK,     /*!< No clock: the counter stands. */
    REG_CS_NO_PRESCALER, /*!< The CPU's clock. */
    REG_CS_PRESCALE_8,   /*!< The CPU's clock / 8. */
    REG_CS_PRESCALE_32,  /*!< The CPU's clock / 32. */
    REG_CS_PRESCALE_64,  /*!< The CPU's clock / 64. */
    REG_CS_PRESCALE_128, /*!< The CPU's clock / 128. */
    REG_CS_PRESCALE_256, /*!< The CPU's clock / 256. */
    REG_CS_PRESCALE_1024 /*!< The CPU's clock / 1024. */
  };

  /*!
   * \brief The pins TimerTwo gives a PWM on.
   *
   * Only pin 3, OC2B: in mode 5 OCR2A is the top of the counter, so the duty
   * of OC2A on pin 11 cannot be set without changing the period.
   */
  enum PwmPinType {
    PWM_PIN_3 = TIMERTWO_B_ARDUINO_PIN /*!< Arduino pin 3, OC2B, PD3. */
  };

 private:
  TimerTwo();
  ~TimerTwo();
  TimerTwo(TimerTwo const&);

  TimerIsrCallbackF_void TimerIsrOverflowCallback;
  StateType State;
  ClockSelectType ClockSelectBitGroup;
  // The duty setPwmDuty() was given, which setPeriod() applies to a new top.
  byte PwmDutyCycle;

  byte getPrescaleShiftScale();
  byte getTimerCycles(TimeType, ClockSelectType&);
  byte getPwmCompareValue();

 public:
  /*! \brief The one TimerTwo, which Timer2 refers to. */
  static TimerTwo& getInstance();

  /*! \brief Where the timer is in its life. */
  StateType getState() const { return State; }
  /*! \brief The function the overflow interrupt calls, or nullptr. */
  TimerIsrCallbackF_void getTimerIsrCallbackFunction() const {
    return TimerIsrOverflowCallback;
  }
  /*!
   * \brief The longest period setPeriod() takes, in microseconds: the last one
   *        whose half fits 256 steps of the largest prescaler.
   */
  TimeType getPeriodMax() {
    return static_cast<TimeType>(
        (TIMERTWO_RESOLUTION * TIMERTWO_MAX_PRESCALER * 2uL * 1000uL - 1uL) /
        (F_CPU / 1000uL));
  }

  /*!
   * \brief Sets the timer up in mode 5, with a period and a callback; the timer
   *        does not count before start().
   * \param Microseconds the period, up to getPeriodMax()
   * \param sTimerOverflowCallback what the overflow interrupt calls, or nullptr
   *                               for none
   * \return E_NOT_OK if the timer was initialized already, or setPeriod() would
   *         refuse the period; the timer is left as it was then.
   */
  StdReturnType init(TimeType Microseconds = 1000uL,
                     TimerIsrCallbackF_void sTimerOverflowCallback = nullptr);
  /*!
   * \brief Sets the period of the interrupt and of the PWM.
   *
   * The duty of the PWM is a share of the period, and keeps its share.
   * \param Microseconds the period, up to getPeriodMax()
   * \return E_NOT_OK if the period is too long, or too short for one step of
   *         the counter at the CPU's clock: 0, or 1 below 2 MHz.
   */
  StdReturnType setPeriod(TimeType Microseconds);
  /*!
   * \brief Starts the PWM on a pin, with a duty.
   * \param PwmPin the pin
   * \param DutyCycle the duty, 0 for always low to 255 for always high
   * \return E_NOT_OK if the timer is not initialized.
   */
  StdReturnType enablePwm(PwmPinType PwmPin, byte DutyCycle);
  /*!
   * \brief Stops the PWM on a pin; the pin is a plain output again, at the
   *        level of its bit in PORTD.
   * \param PwmPin the pin
   * \return E_NOT_OK for a pin TimerTwo has no PWM on.
   */
  StdReturnType disablePwm(PwmPinType PwmPin);
  /*!
   * \brief Sets the duty of the PWM on a pin that enablePwm() started.
   * \param PwmPin the pin
   * \param DutyCycle the duty, 0 for always low to 255 for always high
   * \return E_NOT_OK if the timer is not initialized.
   */
  StdReturnType setPwmDuty(PwmPinType PwmPin, byte DutyCycle);
  /*!
   * \brief Starts the counter at 0, and the interrupt if a callback is set.
   * \return E_NOT_OK if the timer is not initialized, or runs already.
   */
  StdReturnType start();
  /*! \brief Stops the counter where it is, if it runs. */
  void stop();
  /*!
   * \brief Lets the counter go on from where stop() stopped it, and the timer
   *        runs again.
   * \return E_NOT_OK if the timer was not stopped.
   */
  StdReturnType resume();
  /*!
   * \brief Sets the function the overflow interrupt calls, and enables the
   *        interrupt if the timer runs.
   * \param TimerOverflowCallback the function
   * \return E_NOT_OK for nullptr.
   */
  StdReturnType attachInterrupt(TimerIsrCallbackF_void TimerOverflowCallback);
  /*!
   * \brief Disables the overflow interrupt and forgets its callback, so that
   *        start() does not enable it again.
   */
  void detachInterrupt();
  /*!
   * \brief The time since the counter was last at its bottom.
   *
   * Waits for one tick of the counter, to see whether it counts up or down.
   * \param[out] Microseconds the time, in steps of the prescaled clock
   * \return E_NOT_OK if the timer does not run.
   */
  StdReturnType read(TimeType& Microseconds);
  /*! \brief Calls the callback; for the interrupt. */
  void callTimerIsrOverflowCallback() { TimerIsrOverflowCallback(); }
};

/*! \brief The one TimerTwo. */
extern TimerTwo& Timer2;

#endif  // TIMERTWO_SRC_TIMERTWO_H_
