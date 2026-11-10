// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file
 * \brief The return type and the bit macros that TimerTwo shares with the
 *        author's older libraries.
 *
 * The same file comes with those libraries, under the same guard, so that a
 * sketch that includes two of them gets its definitions once. Version 2.0.0
 * replaces it by types of the namespace timer_two.
 */

// The guard the other copies of this file have; renamed, a sketch with one of
// them would define E_OK twice.
#ifndef _STANDARD_TYPES_H_  // NOLINT(build/header_guard)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _STANDARD_TYPES_H_

/*! \brief A switch of the configuration that is on. */
#define STD_ON 1u
/*! \brief A switch of the configuration that is off. */
#define STD_OFF 0u

/*! \brief The character that ends a string. */
#define STD_NULL_CHARACTER '\0'

/*!
 * \brief Sets bit `Bit` of `Var` to `Value`, 0 or 1.
 * \param Var the variable or register
 * \param Bit the number of the bit
 * \param Value 0 or 1
 */
#define writeBit(Var, Bit, Value) \
  ((Var) = static_cast<uint8_t>(((Var) & ~(1u << (Bit))) | ((Value) << (Bit))))

/*!
 * \brief Reads the group of bits `BitGroupMask` of `Var`, shifted down by
 *        `BitGroupPosition`, and stores it in `Var`.
 * \param Var the variable or register
 * \param BitGroupMask the bits of the group, in place
 * \param BitGroupPosition the number of the group's lowest bit
 */
#define readBitGroup(Var, BitGroupMask, BitGroupPosition) \
  (Var = ((Var & ((uint8_t)BitGroupMask)) >> BitGroupPosition))

/*!
 * \brief Sets the group of bits `BitGroupMask` of `Var` to `Value`.
 * \param Var the variable or register
 * \param BitGroupMask the bits of the group, in place
 * \param BitGroupPosition the number of the group's lowest bit
 * \param Value the value of the group, not shifted
 */
#define writeBitGroup(Var, BitGroupMask, BitGroupPosition, Value) \
  ((Var) = static_cast<uint8_t>(                                  \
       ((Var) & ~static_cast<unsigned>(BitGroupMask)) |           \
       ((static_cast<unsigned>(Value) << (BitGroupPosition)) &    \
        static_cast<unsigned>(BitGroupMask))))

/*! \brief Whether a function did what it was asked. */
typedef enum {
  E_OK = 0,    /*!< It did. */
  E_NOT_OK = 1 /*!< It did not, and changed nothing or not all. */
} StdReturnType;

#endif  // _STANDARD_TYPES_H_  NOLINT(build/header_guard)
