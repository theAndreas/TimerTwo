// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// TimerTwo against the registers of test/mock/avr/io.h, at the Uno's 16 MHz.
//
// Timer2 is the one TimerTwo, and init() takes it only once: a test cannot
// begin from a new one. ctest runs each test in a process of its own, as
// gtest_discover_tests() adds them, so each begins from the state after a
// reset; run as one program, all but the first fail in SetUp(). Version 2.0.0
// makes the timer a class a test can construct (OIL 3).

#include "TimerTwo.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <array>

#include "Arduino.h"
#include "StandardTypes.h"

namespace {

// The bits of TCCR2B that select the clock, and their values.
constexpr uint8_t kClockSelectMask{0x07};
constexpr uint8_t kNoPrescaler{1};
constexpr uint8_t kPrescale8{2};
constexpr uint8_t kPrescale32{3};
constexpr uint8_t kPrescale64{4};
constexpr uint8_t kPrescale128{5};
constexpr uint8_t kPrescale256{6};
constexpr uint8_t kPrescale1024{7};

int overflow_calls{0};

void CountOverflow() { ++overflow_calls; }

bool BitIsSet(uint8_t reg, unsigned bit) {
  return ((static_cast<unsigned>(reg) >> bit) & 1U) != 0;
}

uint8_t ClockSelect() {
  return static_cast<uint8_t>(TCCR2B & kClockSelectMask);
}

class TimerTwoTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_EQ(Timer2.getState(), TimerTwo::STATE_INIT)
        << "Each test needs a process of its own; run them through ctest.";
  }
};

TEST_F(TimerTwoTest, InitSetsPhaseCorrectPwmWithoutAClock) {
  TCCR2A = 0xFF;
  TCCR2B = 0xFF;

  EXPECT_EQ(Timer2.init(), E_OK);

  EXPECT_EQ(Timer2.getState(), TimerTwo::STATE_IDLE);
  EXPECT_TRUE(BitIsSet(TCCR2A, WGM20));
  EXPECT_FALSE(BitIsSet(TCCR2A, WGM21));
  EXPECT_TRUE(BitIsSet(TCCR2B, WGM22));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
  EXPECT_EQ(ClockSelect(), 0);
}

TEST_F(TimerTwoTest, InitIsRefusedTheSecondTime) {
  ASSERT_EQ(Timer2.init(), E_OK);

  EXPECT_EQ(Timer2.init(), E_NOT_OK);
}

TEST_F(TimerTwoTest, InitRefusesAPeriodAboveTheLongest) {
  EXPECT_EQ(
      Timer2.init(static_cast<TimerTwo::TimeType>(Timer2.getPeriodMax() + 1U)),
      E_NOT_OK);
}

TEST_F(TimerTwoTest, TheLongestPeriodIsAbout33Milliseconds) {
  // 256 steps up and down at 16 MHz / 1024.
  EXPECT_EQ(Timer2.getPeriodMax(), 32767U);
}

// The period is 2 * OCR2A * prescaler / 16 MHz: up to OCR2A and down again.
TEST_F(TimerTwoTest, APeriodSetsTheTopAndTheSmallestPrescaler) {
  ASSERT_EQ(Timer2.init(10U), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);
  EXPECT_EQ(OCR2A, 80);
  EXPECT_EQ(ClockSelect(), kNoPrescaler);

  EXPECT_EQ(Timer2.setPeriod(100U), E_OK);
  EXPECT_EQ(OCR2A, 100);
  EXPECT_EQ(ClockSelect(), kPrescale8);

  EXPECT_EQ(Timer2.setPeriod(1000U), E_OK);
  EXPECT_EQ(OCR2A, 250);
  EXPECT_EQ(ClockSelect(), kPrescale32);

  EXPECT_EQ(Timer2.setPeriod(2000U), E_OK);
  EXPECT_EQ(OCR2A, 250);
  EXPECT_EQ(ClockSelect(), kPrescale64);

  EXPECT_EQ(Timer2.setPeriod(4000U), E_OK);
  EXPECT_EQ(OCR2A, 250);
  EXPECT_EQ(ClockSelect(), kPrescale128);

  EXPECT_EQ(Timer2.setPeriod(8000U), E_OK);
  EXPECT_EQ(OCR2A, 250);
  EXPECT_EQ(ClockSelect(), kPrescale256);

  EXPECT_EQ(Timer2.setPeriod(Timer2.getPeriodMax()), E_OK);
  EXPECT_EQ(OCR2A, 255);
  EXPECT_EQ(ClockSelect(), kPrescale1024);
}

TEST_F(TimerTwoTest, SetPeriodRefusesAPeriodAboveTheLongest) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);

  EXPECT_EQ(Timer2.setPeriod(
                static_cast<TimerTwo::TimeType>(Timer2.getPeriodMax() + 1U)),
            E_NOT_OK);
  EXPECT_EQ(OCR2A, 250);
}

TEST_F(TimerTwoTest, StartSelectsTheClockFromZero) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);
  TCNT2.Set(42, false);

  EXPECT_EQ(Timer2.start(), E_OK);

  EXPECT_EQ(Timer2.getState(), TimerTwo::STATE_RUNNING);
  EXPECT_EQ(ClockSelect(), kPrescale32);
  EXPECT_EQ(static_cast<uint8_t>(TCNT2), 0);
}

TEST_F(TimerTwoTest, StartWithoutCallbackLeavesTheInterruptOff) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);

  ASSERT_EQ(Timer2.start(), E_OK);

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
}

TEST_F(TimerTwoTest, StartWithCallbackEnablesTheInterrupt) {
  ASSERT_EQ(Timer2.init(1000U, CountOverflow), E_OK);
  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));

  ASSERT_EQ(Timer2.start(), E_OK);

  EXPECT_TRUE(BitIsSet(TIMSK2, TOIE2));
}

TEST_F(TimerTwoTest, StartIsRefusedBeforeInit) {
  EXPECT_EQ(Timer2.start(), E_NOT_OK);
  EXPECT_EQ(ClockSelect(), 0);
}

TEST_F(TimerTwoTest, StartIsRefusedWhileRunning) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);

  EXPECT_EQ(Timer2.start(), E_NOT_OK);
}

TEST_F(TimerTwoTest, TheOverflowCallsTheCallback) {
  ASSERT_EQ(Timer2.init(1000U, CountOverflow), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);

  TIMER2_OVF_vect();
  TIMER2_OVF_vect();

  EXPECT_EQ(overflow_calls, 2);
}

TEST_F(TimerTwoTest, StopDeselectsTheClock) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);

  Timer2.stop();

  EXPECT_EQ(Timer2.getState(), TimerTwo::STATE_STOPPED);
  EXPECT_EQ(ClockSelect(), 0);
}

TEST_F(TimerTwoTest, ResumeSelectsTheClockAgain) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);
  Timer2.stop();

  EXPECT_EQ(Timer2.resume(), E_OK);

  EXPECT_EQ(ClockSelect(), kPrescale32);
}

TEST_F(TimerTwoTest, ResumeIsRefusedUnlessStopped) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);

  EXPECT_EQ(Timer2.resume(), E_NOT_OK);
}

TEST_F(TimerTwoTest, EnablePwmDrivesPin3WithTheDutyOfTheTop) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);

  EXPECT_EQ(Timer2.enablePwm(TimerTwo::PWM_PIN_3, 128), E_OK);

  EXPECT_EQ(mock_pin_mode[3], OUTPUT);
  EXPECT_TRUE(BitIsSet(TCCR2A, COM2B1));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B0));
  // Half of the top, 250.
  EXPECT_EQ(OCR2B, 125);
}

TEST_F(TimerTwoTest, EnablePwmIsRefusedBeforeInit) {
  EXPECT_EQ(Timer2.enablePwm(TimerTwo::PWM_PIN_3, 128), E_NOT_OK);

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
}

TEST_F(TimerTwoTest, SetPwmDutySetsTheCompareValue) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);
  ASSERT_EQ(Timer2.enablePwm(TimerTwo::PWM_PIN_3, 128), E_OK);

  EXPECT_EQ(Timer2.setPwmDuty(TimerTwo::PWM_PIN_3, 64), E_OK);

  EXPECT_EQ(OCR2B, 62);
}

TEST_F(TimerTwoTest, SetPwmDutyIsRefusedBeforeInit) {
  OCR2B = 7;

  EXPECT_EQ(Timer2.setPwmDuty(TimerTwo::PWM_PIN_3, 64), E_NOT_OK);

  EXPECT_EQ(OCR2B, 7);
}

TEST_F(TimerTwoTest, DisablePwmDisconnectsPin3) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);
  ASSERT_EQ(Timer2.enablePwm(TimerTwo::PWM_PIN_3, 128), E_OK);

  EXPECT_EQ(Timer2.disablePwm(TimerTwo::PWM_PIN_3), E_OK);

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
}

TEST_F(TimerTwoTest, AttachInterruptRefusesNullptr) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);

  EXPECT_EQ(Timer2.attachInterrupt(nullptr), E_NOT_OK);
  EXPECT_EQ(Timer2.getTimerIsrCallbackFunction(), nullptr);
}

TEST_F(TimerTwoTest, AttachInterruptWhileRunningEnablesTheInterrupt) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);

  EXPECT_EQ(Timer2.attachInterrupt(CountOverflow), E_OK);

  EXPECT_TRUE(BitIsSet(TIMSK2, TOIE2));
  TIMER2_OVF_vect();
  EXPECT_EQ(overflow_calls, 1);
}

TEST_F(TimerTwoTest, DetachInterruptDisablesTheInterrupt) {
  ASSERT_EQ(Timer2.init(1000U, CountOverflow), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);

  Timer2.detachInterrupt();

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
}

TEST_F(TimerTwoTest, ReadIsRefusedUnlessRunning) {
  ASSERT_EQ(Timer2.init(100U), E_OK);
  TimerTwo::TimeType microseconds{7};

  EXPECT_EQ(Timer2.read(microseconds), E_NOT_OK);

  EXPECT_EQ(microseconds, 7U);
}

// At 100 us the prescaler is 8: a step of the counter is 0.5 us.
TEST_F(TimerTwoTest, ReadCountsTheStepsUpwards) {
  ASSERT_EQ(Timer2.init(100U), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);
  TCNT2.Set(64, true);
  TimerTwo::TimeType microseconds{0};

  EXPECT_EQ(Timer2.read(microseconds), E_OK);

  EXPECT_EQ(microseconds, 32U);
}

TEST_F(TimerTwoTest, ReadCountsTheStepsBackDownFromTheTop) {
  ASSERT_EQ(Timer2.init(100U), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);
  // 100 steps up and 92 down.
  TCNT2.Set(8, false);
  TimerTwo::TimeType microseconds{0};

  EXPECT_EQ(Timer2.read(microseconds), E_OK);

  EXPECT_EQ(microseconds, 96U);
}

// 16 steps of the counter are as many microseconds as the prescaler divides
// by: 16 MHz / 16.
TEST_F(TimerTwoTest, ReadScalesByEachPrescaler) {
  ASSERT_EQ(Timer2.init(10U), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);
  struct Case {
    TimerTwo::TimeType period;
    TimerTwo::TimeType sixteen_steps;
  };
  std::array<Case, 7> const cases{{{10U, 1U},
                                   {100U, 8U},
                                   {1000U, 32U},
                                   {2000U, 64U},
                                   {4000U, 128U},
                                   {8000U, 256U},
                                   {32767U, 1024U}}};

  for (auto const& c : cases) {
    ASSERT_EQ(Timer2.setPeriod(c.period), E_OK);
    TCNT2.Set(16, true);
    TimerTwo::TimeType microseconds{0};

    EXPECT_EQ(Timer2.read(microseconds), E_OK);

    EXPECT_EQ(microseconds, c.sixteen_steps) << "period " << c.period;
  }
}

}  // namespace
