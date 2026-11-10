// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// avr-libc's access to flash, for the tests on the host, which has no
// separate flash: PROGMEM is nothing, and a read from flash is a read.

#ifndef TIMERTWO_TEST_MOCK_AVR_PGMSPACE_H_
#define TIMERTWO_TEST_MOCK_AVR_PGMSPACE_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PROGMEM
#define PGM_P const char*

// avr-libc's names, which the library calls.
// NOLINTNEXTLINE(readability-identifier-naming)
inline uint8_t pgm_read_byte(void const* address) {
  return *static_cast<uint8_t const*>(address);
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline uint16_t pgm_read_word(void const* address) {
  uint16_t value{0};
  memcpy(&value, address, sizeof(value));
  return value;
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline uint32_t pgm_read_dword(void const* address) {
  uint32_t value{0};
  memcpy(&value, address, sizeof(value));
  return value;
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline void* memcpy_P(void* ram, void const* flash, size_t count) {
  return memcpy(ram, flash, count);
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline void const* pgm_read_ptr(void const* address) {
  return *static_cast<void const* const*>(address);
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline int memcmp_P(void const* ram, void const* flash, size_t count) {
  return memcmp(ram, flash, count);
}

// NOLINTNEXTLINE(readability-identifier-naming)
inline size_t strlen_P(char const* flash) { return strlen(flash); }

#endif  // TIMERTWO_TEST_MOCK_AVR_PGMSPACE_H_
