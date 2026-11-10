// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Toggles the LED at the longest period of the timer, gives a PWM on pin 3,
// and prints how long ago the counter was last at its bottom: 20 ms after the
// start, as the period is about 33 ms and has not ended yet. Each call that
// can refuse is checked, and a refusal printed with its reason.

#include <TimerTwo.h>
#include <stdint.h>

namespace {

constexpr uint8_t kLedPin{LED_BUILTIN};
constexpr uint32_t kBaudRate{115200};

void OnTick() {
  digitalWrite(kLedPin, digitalRead(kLedPin) == LOW ? HIGH : LOW);
}

// Prints why a call was refused and stops there, as the rest would not work.
void Check(timer_two::Status status, __FlashStringHelper const* call) {
  if (!status.IsOk()) {
    Serial.print(call);
    Serial.print(F(": "));
    Serial.println(status.GetErrorText());
    while (true) {
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(kBaudRate);
  pinMode(kLedPin, OUTPUT);
  auto& timer = timer_two::timer2;
  Check(timer.Begin(timer_two::Timer2::kPeriodMax, OnTick), F("Begin"));
  Check(timer.EnablePwm(127), F("EnablePwm"));
  Check(timer.Start(), F("Start"));

  delay(20);
  timer_two::Result<timer_two::TimeType> const elapsed{timer.Read()};
  Check(elapsed.GetError(), F("Read"));
  Serial.print(F("Since the bottom: "));
  Serial.print(elapsed.GetValue());
  Serial.println(F(" us"));
}

void loop() {}
