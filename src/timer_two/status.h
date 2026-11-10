// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Why a call did not do what it was asked: Error, and Status and Result,
 *        which a function returns.
 */

#ifndef TIMERTWO_SRC_TIMER_TWO_STATUS_H_
#define TIMERTWO_SRC_TIMER_TWO_STATUS_H_

#include <stdint.h>

// The core's name, reserved as it is: GetErrorText() returns this very type.
class __FlashStringHelper;  // NOLINT(bugprone-reserved-identifier)

namespace timer_two {

/*!
 * \brief Why a call did not do what it was asked; one byte, so that it returns
 *        in a register.
 */
enum class Error : uint8_t {
  kOk = 0,           /*!< No error. */
  kNotBegun,         /*!< Begin() has not been called. */
  kBegunAlready,     /*!< Begin() has been called already. */
  kRunning,          /*!< The timer runs already. */
  kNotRunning,       /*!< The timer does not run. */
  kNotStopped,       /*!< The timer was not stopped by Stop(). */
  kPeriodOutOfRange, /*!< The period is not in range, see SetPeriod(). */
  kNoCallback        /*!< The callback is nullptr. */
};

/*!
 * \brief What a function that can refuse returns when it has no value to
 *        return.
 *
 * A sketch asks IsOk(), or compares with an error:
 *
 * \code{.cpp}
 * if (timer_two::timer2.SetPeriod(50000) ==
 * timer_two::Error::kPeriodOutOfRange) {
 *   ...
 * }
 * \endcode
 *
 * \note It has no conversion to `bool`, on purpose: with `StdReturnType` of
 *       version 1, `E_OK` was 0, so `if (Timer2.start())` was true on an error.
 */
class Status {
 public:
  /*!
   * \brief Makes a status of `error`, ok when none is given.
   *
   * Implicit, so that a function returns an error as it is.
   *
   * \param error the error, or Error::kOk
   */
  constexpr Status(  // NOLINT(google-explicit-constructor)
      Error error = Error::kOk)
      : error_{error} {}

  /*! \brief Why the call failed, or Error::kOk. */
  constexpr Error GetError() const { return error_; }

  /*!
   * \brief The error in a few words, in flash, for `Serial.print()`:
   *        `"period out of range"` for Error::kPeriodOutOfRange.
   *
   * A sketch that never calls it pays nothing, as the linker drops it with its
   * texts.
   *
   * \return the text; `"unknown error"` for a byte cast to Error that is none
   *         of its values
   */
  __FlashStringHelper const* GetErrorText() const;

  /*! \brief Whether the call did what it was asked. */
  constexpr bool IsOk() const { return error_ == Error::kOk; }

  /*!
   * \brief Whether the call failed for this reason.
   * \param other the error to compare with
   */
  constexpr bool operator==(Error other) const { return error_ == other; }

  /*!
   * \brief Whether the call did not fail for this reason.
   * \param other the error to compare with
   */
  constexpr bool operator!=(Error other) const { return error_ != other; }

 private:
  // A sketch reaches the error only through this class, so that it could carry
  // more than the enum one day without a sketch changing.
  Error error_;
};

/*!
 * \brief What a function that reads a value returns: the value, or why there is
 *        none.
 *
 * After an error the value is 0, or what `Value()` is, never undefined.
 *
 * \tparam Value the type of the value
 */
template <typename Value>
class Result {
 public:
  /*!
   * \brief Makes a result that holds `value`.
   *
   * Implicit, so that a function returns its value as it is.
   *
   * \param value the value
   */
  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr Result(Value value)  // NOLINT(runtime/explicit)
      : value_{value}, error_{Error::kOk} {}

  /*!
   * \brief Makes a result that holds why there is no value.
   *
   * Implicit, so that a function returns its error as it is.
   *
   * \param error the error
   */
  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr Result(Error error)  // NOLINT(runtime/explicit)
      : value_{}, error_{error} {}

  /*! \brief The value; 0, or what `Value()` is, after an error. */
  constexpr Value GetValue() const { return value_; }

  /*! \brief Why there is no value, or Error::kOk. */
  constexpr Error GetError() const { return error_; }

  /*! \copydoc Status::GetErrorText() */
  __FlashStringHelper const* GetErrorText() const {
    return Status{error_}.GetErrorText();
  }

  /*! \brief Whether there is a value. */
  constexpr bool IsOk() const { return error_ == Error::kOk; }

  /*! \copydoc Status::operator==(Error) const */
  constexpr bool operator==(Error other) const { return error_ == other; }

  /*! \copydoc Status::operator!=(Error) const */
  constexpr bool operator!=(Error other) const { return error_ != other; }

 private:
  // Side by side rather than in a union: a value and its error still return in
  // registers on the AVR, and the value is never left undefined.
  Value value_;
  Error error_;
};

}  // namespace timer_two

#endif  // TIMERTWO_SRC_TIMER_TWO_STATUS_H_
