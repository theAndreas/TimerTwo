// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Arduino.h"

#include <stdint.h>

#include <array>

std::array<uint8_t, 20> mock_pin_mode{};

void pinMode(uint8_t pin, uint8_t mode) {
  if (pin < mock_pin_mode.size()) {
    mock_pin_mode[pin] = mode;
  }
}
