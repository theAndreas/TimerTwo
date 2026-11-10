// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief Attributes of a later standard, as macros for every standard, and
 *        Discard() for a result one of them marks.
 *
 * The library is written for C++11 and later. Where a later standard has an
 * attribute, the macro is that attribute; before it, the compiler's own
 * where GCC and clang have one, else nothing.
 */

#ifndef TIMERTWO_SRC_TIMER_TWO_ATTRIBUTES_H_
#define TIMERTWO_SRC_TIMER_TWO_ATTRIBUTES_H_

/*!
 * \def TIMER_TWO_NODISCARD
 * \brief Marks a function whose result must not be ignored.
 *
 * Stands in front of the declaration: `TIMER_TWO_NODISCARD int Read();`. A call
 * that drops the result is a warning, and an error in CI.
 *
 * \note For a function only. On a type, as a class whose every result must be
 *       used, `[[nodiscard]]` of C++17 works, GCC's attribute before it does
 *       not.
 */
// By __cplusplus, not by __has_cpp_attribute: GCC and clang have
// [[nodiscard]] in C++11 as well, as an extension that -Wpedantic reports.
#if __cplusplus >= 201703L
#define TIMER_TWO_NODISCARD [[nodiscard]]
#elif defined(__GNUC__)
// GCC, avr-gcc and clang, which defines __GNUC__ as well.
#define TIMER_TWO_NODISCARD __attribute__((warn_unused_result))
#else
#define TIMER_TWO_NODISCARD
#endif

namespace timer_two {

/*!
 * \brief Drops a result that must not be ignored, where dropping it is meant.
 *
 * For a result TIMER_TWO_NODISCARD marks: the call says that it is
 * dropped on purpose, and a comment beside it why:
 * `Discard(Send(frame));  // sent again on the next tick`. Not a cast to
 * `void`: before C++17, GCC's `warn_unused_result` warns through one and clang
 * does not, so a cast that passes on the host fails the build for the target.
 *
 * \tparam Value The type of the result.
 */
template <typename Value>
void Discard(Value const& /*value*/) noexcept {}

}  // namespace timer_two

#endif  // TIMERTWO_SRC_TIMER_TWO_ATTRIBUTES_H_
