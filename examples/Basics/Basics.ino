// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Toggles the LED every millisecond from the overflow interrupt, and gives a
// PWM of half the period on pin 3, whose frequency follows the period.

#include <TimerTwo.h>
#include <stdint.h>

namespace {

constexpr uint8_t kLedPin{LED_BUILTIN};

void OnTick() {
  digitalWrite(kLedPin, digitalRead(kLedPin) == LOW ? HIGH : LOW);
}

}  // namespace

void setup() {
  pinMode(kLedPin, OUTPUT);
  // 1000 microseconds, and the callback. Nothing to do on a refusal here: the
  // period is in range, and the timer is begun once.
  timer_two::Discard(timer_two::timer2.Begin(1000, OnTick));
  timer_two::Discard(timer_two::timer2.EnablePwm(128));
  timer_two::Discard(timer_two::timer2.Start());
}

void loop() {}
