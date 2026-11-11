// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Timer, a periodic interrupt, a PWM and a time readout on an 8-bit
 *        Timer/Counter of the AVR in phase correct PWM with OCRxA as its top.
 *
 * The counter runs up to the top and down again in mode 5: the period sets
 * the top, OCRxA, and the prescaler; the overflow at the bottom calls the
 * callback; and OCxB gives the PWM, at a duty that is a share of the top.
 * Which Timer/Counter it is, is a parameter of the template: timer2.h hands in
 * Timer/Counter2 of the ATmega328P, the tests a mock of it.
 */

#ifndef TIMERTWO_SRC_TIMER_TWO_TIMER_H_
#define TIMERTWO_SRC_TIMER_TWO_TIMER_H_

#include <Arduino.h>
#include <avr/io.h>
#include <stdint.h>

#include "timer_two/attributes.h"
#include "timer_two/status.h"

namespace timer_two {

/*!
 * \brief A time in microseconds: 32 bits below 8 MHz, where the longest period
 *        does not fit into 16.
 */
#if F_CPU < 8000000UL
using TimeType = uint32_t;
#else
using TimeType = uint16_t;
#endif

/*! \brief The function the overflow interrupt calls. */
using Callback = void (*)();

/*! \brief Where a timer is in its life. */
enum class State : uint8_t {
  kNotBegun, /*!< Begin() comes first. */
  kIdle,     /*!< Begun, not started. */
  kRunning,  /*!< Counting. */
  kStopped   /*!< Stopped by Stop(); Resume() goes on, Start() from 0. */
};

namespace internal {

/*! \cond */
// The values of the clock select bits CSx2..CSx0, and the bits themselves.
enum class ClockSelect : uint8_t {
  kNoClock = 0,
  kDivide1,
  kDivide8,
  kDivide32,
  kDivide64,
  kDivide128,
  kDivide256,
  kDivide1024
};
constexpr uint8_t kClockSelectMask{0x07U};

// The counter has 256 values.
constexpr uint32_t kCounterValues{256U};
constexpr uint32_t kClocksPerMillisecond{F_CPU / 1000UL};
constexpr uint32_t kMicrosecondsPerMillisecond{1000U};
// The duty is a share of 255 of the top, rounded.
constexpr uint16_t kDutyMax{255U};
constexpr uint16_t kDutyRounding{kDutyMax / 2U};

// The prescalers as shifts: 1, 8, 32, 64, 128, 256 and 1024.
constexpr uint8_t kShiftDivide8{3U};
constexpr uint8_t kShiftDivide32{5U};
constexpr uint8_t kShiftDivide64{6U};
constexpr uint8_t kShiftDivide128{7U};
constexpr uint8_t kShiftDivide256{8U};
constexpr uint8_t kShiftDivide1024{10U};

// The shift of the prescaler clock_select selects; a switch rather than a
// table, which avr-gcc would copy to RAM.
inline uint8_t ShiftOf(ClockSelect clock_select) {
  switch (clock_select) {
    case ClockSelect::kDivide8:
      return kShiftDivide8;
    case ClockSelect::kDivide32:
      return kShiftDivide32;
    case ClockSelect::kDivide64:
      return kShiftDivide64;
    case ClockSelect::kDivide128:
      return kShiftDivide128;
    case ClockSelect::kDivide256:
      return kShiftDivide256;
    case ClockSelect::kDivide1024:
      return kShiftDivide1024;
    case ClockSelect::kNoClock:
    case ClockSelect::kDivide1:
    default:
      return 0U;
  }
}

// The top for a period, and in clock_select the smallest prescaler that lets
// it fit the counter; 0 for a period too short for one step, or too long. The
// counter runs up to the top and down again, so half the period is counted.
// Multiplied before divided, so that a clock below 2 MHz keeps its cycles; up
// to Timer::kPeriodMax the product is below 2^29.
inline uint8_t TopOf(TimeType period, ClockSelect& clock_select) {
  uint32_t const half{(kClocksPerMillisecond * period) /
                      (2U * kMicrosecondsPerMillisecond)};
  for (uint8_t select{static_cast<uint8_t>(ClockSelect::kDivide1)};
       select <= static_cast<uint8_t>(ClockSelect::kDivide1024); ++select) {
    uint32_t const top{half >> ShiftOf(static_cast<ClockSelect>(select))};
    if (top < kCounterValues) {
      clock_select = static_cast<ClockSelect>(select);
      return static_cast<uint8_t>(top);
    }
  }
  clock_select = ClockSelect::kNoClock;
  return 0U;
}
/*! \endcond */

}  // namespace internal

/*!
 * \brief A periodic interrupt, a PWM and a time readout on an 8-bit
 *        Timer/Counter in phase correct PWM, mode 5.
 *
 * Begin() first, then Start(). A function that can refuse returns a Status,
 * and leaves the timer as it was when it refuses. A sketch uses the one that
 * timer2.h gives, timer2.
 *
 * \tparam Registers the Timer/Counter: static functions `Tccra()`, `Tccrb()`,
 *                   `Ocra()`, `Ocrb()`, `Timsk()`, `Tifr()` and `Tcnt()` that
 *                   return its registers, and `kPwmPin`, the Arduino pin of
 *                   OCxB. The names of the bits are those of Timer/Counter2, as
 *                   all of the AVR's 8-bit timers share their places.
 */
template <typename Registers>
class Timer {
 public:
  /*!
   * \brief The longest period, in microseconds: the last whose half fits 256
   *        steps of the largest prescaler. 32767 at 16 MHz.
   */
  static constexpr TimeType kPeriodMax{static_cast<TimeType>(
      (((internal::kCounterValues << internal::kShiftDivide1024) * 2U *
        internal::kMicrosecondsPerMillisecond) -
       1U) /
      internal::kClocksPerMillisecond)};

  /*! \brief The period Begin() sets if none is given: 1 ms. */
  static constexpr TimeType kDefaultPeriod{1000U};

  /*!
   * \brief Sets the timer up in mode 5, with a period and a callback; it does
   *        not count before Start().
   * \param period the period in microseconds, up to kPeriodMax
   * \param callback what the overflow interrupt calls, or nullptr for none
   * \return Error::kBegunAlready, or Error::kPeriodOutOfRange as SetPeriod()
   */
  TIMER_TWO_NODISCARD Status Begin(TimeType period = kDefaultPeriod,
                                   Callback callback = nullptr) {
    internal::ClockSelect clock_select{internal::ClockSelect::kNoClock};
    uint8_t const top{internal::TopOf(period, clock_select)};
    if (state_ != State::kNotBegun) {
      return Error::kBegunAlready;
    }
    if ((period > kPeriodMax) || (top == 0U)) {
      return Error::kPeriodOutOfRange;
    }
    // Mode 5: WGMx2 and WGMx0; no clock and no output yet.
    Registers::Tccra() = static_cast<uint8_t>(1U << WGM20);
    Registers::Tccrb() = static_cast<uint8_t>(1U << WGM22);
    state_ = State::kIdle;
    ApplyPeriod(top, clock_select);
    if (callback != nullptr) {
      Discard(AttachInterrupt(callback));  // not nullptr, so it attaches
    }
    return Error::kOk;
  }

  /*!
   * \brief Sets the period of the interrupt and of the PWM; the duty of the PWM
   *        keeps its share of it.
   * \param period the period in microseconds, up to kPeriodMax
   * \return Error::kNotBegun, or Error::kPeriodOutOfRange for a period too
   *         long, or too short for one step of the counter: 0, or 1 below 2 MHz
   */
  TIMER_TWO_NODISCARD Status SetPeriod(TimeType period) {
    internal::ClockSelect clock_select{internal::ClockSelect::kNoClock};
    uint8_t const top{internal::TopOf(period, clock_select)};
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    // A top of 0 would stop the counter at 0.
    if ((period > kPeriodMax) || (top == 0U)) {
      return Error::kPeriodOutOfRange;
    }
    ApplyPeriod(top, clock_select);
    return Error::kOk;
  }

  /*!
   * \brief Starts the counter at 0, and the interrupt if a callback is set.
   * \return Error::kNotBegun, or Error::kRunning
   */
  TIMER_TWO_NODISCARD Status Start() {
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    if (state_ == State::kRunning) {
      return Error::kRunning;
    }
    Registers::Tcnt() = 0U;
    SelectClock(clock_select_);
    if (callback_ != nullptr) {
      // Off the bottom first, as version 1 did; one step takes at most 1024
      // clocks, 64 us at 16 MHz.
      while (Registers::Tcnt() == 0U) {
      }
      EnableOverflowInterrupt();
    }
    state_ = State::kRunning;
    return Error::kOk;
  }

  /*!
   * \brief Stops the counter where it is.
   * \return Error::kNotRunning
   */
  TIMER_TWO_NODISCARD Status Stop() {
    if (state_ != State::kRunning) {
      return Error::kNotRunning;
    }
    SelectClock(internal::ClockSelect::kNoClock);
    state_ = State::kStopped;
    return Error::kOk;
  }

  /*!
   * \brief Lets the counter go on from where Stop() stopped it.
   * \return Error::kNotStopped
   */
  TIMER_TWO_NODISCARD Status Resume() {
    if (state_ != State::kStopped) {
      return Error::kNotStopped;
    }
    SelectClock(clock_select_);
    state_ = State::kRunning;
    return Error::kOk;
  }

  /*!
   * \brief Sets the function the overflow interrupt calls, and enables the
   *        interrupt if the timer runs or is stopped, so that Resume() goes on
   *        with it.
   * \param callback the function
   * \return Error::kNoCallback for nullptr
   */
  TIMER_TWO_NODISCARD Status AttachInterrupt(Callback callback) {
    if (callback == nullptr) {
      return Error::kNoCallback;
    }
    // The AVR writes the pointer a byte at a time: the interrupt must not call
    // through half of the new one.
    ClearBit(Registers::Timsk(), TOIE2);
    callback_ = callback;
    // Stopped, the counter stands and comes to no bottom before Resume().
    bool const has_counted{(state_ == State::kRunning) ||
                           (state_ == State::kStopped)};
    if (has_counted) {
      EnableOverflowInterrupt();
    }
    return Error::kOk;
  }

  /*!
   * \brief Disables the overflow interrupt and forgets its callback, so that
   *        Start() does not enable it again.
   */
  void DetachInterrupt() {
    ClearBit(Registers::Timsk(), TOIE2);
    // After the interrupt is off, which calls through it.
    callback_ = nullptr;
  }

  /*!
   * \brief Starts the PWM on `Registers::kPwmPin`, pin 3 for Timer2, with a
   *        duty.
   *
   * It breaks analogWrite() on that pin.
   *
   * \param duty the duty, 0 for always low to 255 for always high
   * \return Error::kNotBegun
   */
  TIMER_TWO_NODISCARD Status EnablePwm(uint8_t duty) {
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    pinMode(Registers::kPwmPin, OUTPUT);
    SetBit(Registers::Tccra(), COM2B1);
    return SetPwmDuty(duty);
  }

  /*!
   * \brief Sets the duty of the PWM, which EnablePwm() started or is to start.
   * \param duty the duty, 0 for always low to 255 for always high
   * \return Error::kNotBegun
   */
  TIMER_TWO_NODISCARD Status SetPwmDuty(uint8_t duty) {
    if (state_ == State::kNotBegun) {
      return Error::kNotBegun;
    }
    duty_ = duty;
    Registers::Ocrb() = CompareValue();
    return Error::kOk;
  }

  /*!
   * \brief Stops the PWM; the pin is a plain output again, at the level of its
   *        bit in its PORT register.
   */
  void DisablePwm() { ClearBit(Registers::Tccra(), COM2B1); }

  /*!
   * \brief The time since the counter was last at its bottom, in steps of the
   *        prescaled clock.
   *
   * Waits for one step of the counter, at most 1024 clocks, to see whether it
   * counts up or down.
   *
   * \return the time in microseconds, or Error::kNotRunning
   */
  TIMER_TWO_NODISCARD Result<TimeType> Read() {
    if (state_ != State::kRunning) {
      return Error::kNotRunning;
    }
    uint16_t steps{Registers::Tcnt()};
    uint8_t next{Registers::Tcnt()};
    while (next == steps) {
      next = Registers::Tcnt();
    }
    // Counting down: up to the top, and back to where it is.
    if (next < steps) {
      steps = static_cast<uint16_t>((2U * Registers::Ocra()) - steps);
    }
    // In clocks, then in microseconds: multiplied before divided, so that no
    // step is lost. At most 2 * 255 steps of 1024 clocks, times 1000, fit 32
    // bits; the time is below kPeriodMax, which TimeType holds.
    uint32_t const clocks{static_cast<uint32_t>(steps)
                          << internal::ShiftOf(clock_select_)};
    return static_cast<TimeType>(
        (clocks * internal::kMicrosecondsPerMillisecond) /
        internal::kClocksPerMillisecond);
  }

  /*! \brief Where the timer is in its life. */
  State GetState() const { return state_; }

  /*! \brief The function the overflow interrupt calls, or nullptr. */
  Callback GetCallback() const { return callback_; }

  /*!
   * \brief Calls the callback; for the overflow interrupt, which timer2.cpp
   *        defines.
   */
  void OnOverflow() {
    Callback const callback{callback_};
    if (callback != nullptr) {
      callback();
    }
  }

 private:
  // The compare value of duty_ at the top, rounded: 0 is the bottom, which
  // keeps the output low, and 255 the top, which keeps it high. In 16 bits
  // unsigned: 255 * 255 does not fit the AVR's int.
  uint8_t CompareValue() const {
    // The outer cast is redundant on the AVR, where the product is unsigned
    // int, but not on the host, where it is int.
    // NOLINTNEXTLINE(readability-redundant-casting)
    uint16_t const product{static_cast<uint16_t>(
        static_cast<uint16_t>(Registers::Ocra()) * duty_)};
    return static_cast<uint8_t>((product + internal::kDutyRounding) /
                                internal::kDutyMax);
  }

  void ApplyPeriod(uint8_t top, internal::ClockSelect clock_select) {
    clock_select_ = clock_select;
    Registers::Ocra() = top;
    Registers::Ocrb() = CompareValue();
    if (state_ == State::kRunning) {
      SelectClock(clock_select_);
    }
  }

  // The flag of an overflow stays set until the interrupt runs, even while it
  // is disabled: the core's init() runs Timer2 for analogWrite() before
  // setup(), and an earlier run leaves it as well. Enabled with it set, the
  // interrupt would call the callback at once rather than after a period. A
  // flag is cleared by writing 1 to it, and only it: no read first.
  static void EnableOverflowInterrupt() {
    Registers::Tifr() = static_cast<uint8_t>(1U << TOV2);
    SetBit(Registers::Timsk(), TOIE2);
  }

  static void SelectClock(internal::ClockSelect clock_select) {
    Registers::Tccrb() = static_cast<uint8_t>(
        (static_cast<unsigned>(Registers::Tccrb()) &
         ~static_cast<unsigned>(internal::kClockSelectMask)) |
        static_cast<unsigned>(clock_select));
  }

  template <typename Register>
  static void SetBit(Register& reg, uint8_t bit) {
    reg = static_cast<uint8_t>(static_cast<unsigned>(reg) | (1U << bit));
  }

  template <typename Register>
  static void ClearBit(Register& reg, uint8_t bit) {
    reg = static_cast<uint8_t>(static_cast<unsigned>(reg) & ~(1U << bit));
  }

  // volatile, as the overflow interrupt reads it: the store must not move
  // past the enabling of the interrupt.
  Callback volatile callback_{nullptr};
  State state_{State::kNotBegun};
  internal::ClockSelect clock_select_{internal::ClockSelect::kNoClock};
  // The duty SetPwmDuty() was given, which a new period applies to its top.
  uint8_t duty_{0};
};

/*! \cond */
// The definitions C++11 needs for a static constexpr member that is used by
// reference.
template <typename Registers>
constexpr TimeType Timer<Registers>::kPeriodMax;
template <typename Registers>
constexpr TimeType Timer<Registers>::kDefaultPeriod;
/*! \endcond */

}  // namespace timer_two

#endif  // TIMERTWO_SRC_TIMER_TWO_TIMER_H_
