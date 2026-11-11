// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Timer on the registers of Timer2 in test/mock/avr/io.h, at the Uno's 16 MHz:
// each test makes a timer of its own, on registers set back to those after a
// reset.

#include "timer_two/timer.h"

#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <array>

#include "Arduino.h"
#include "test/printers.h"  // IWYU pragma: keep
#include "timer_two/status.h"
#include "timer_two/timer2.h"

namespace timer_two {
namespace {

// The bits of TCCR2B that select the clock, and their values.
constexpr uint8_t kClockSelectMask{0x07};
constexpr uint8_t kDivide1{1};
constexpr uint8_t kDivide8{2};
constexpr uint8_t kDivide32{3};
constexpr uint8_t kDivide64{4};
constexpr uint8_t kDivide128{5};
constexpr uint8_t kDivide256{6};
constexpr uint8_t kDivide1024{7};

int overflow_calls{0};

void CountOverflow() { ++overflow_calls; }

bool BitIsSet(uint8_t reg, unsigned bit) {
  return ((static_cast<unsigned>(reg) >> bit) & 1U) != 0;
}

uint8_t ClockSelect() {
  return static_cast<uint8_t>(TCCR2B & kClockSelectMask);
}

class TimerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    TCCR2A = 0;
    TCCR2B = 0;
    OCR2A = 0;
    OCR2B = 0;
    TIMSK2 = 0;
    TIFR2 = 0;
    TCNT2.Set(0, true);
    mock_pin_mode.fill(INPUT);
    overflow_calls = 0;
  }

  Timer2& timer() { return timer_; }

 private:
  Timer2 timer_;
};

TEST_F(TimerTest, BeginSetsPhaseCorrectPwmWithoutAClock) {
  TCCR2A = 0xFF;
  TCCR2B = 0xFF;

  EXPECT_EQ(timer().Begin(), Error::kOk);

  EXPECT_EQ(timer().GetState(), State::kIdle);
  EXPECT_TRUE(BitIsSet(TCCR2A, WGM20));
  EXPECT_FALSE(BitIsSet(TCCR2A, WGM21));
  EXPECT_TRUE(BitIsSet(TCCR2B, WGM22));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
  EXPECT_EQ(ClockSelect(), 0);
}

TEST_F(TimerTest, BeginIsRefusedTheSecondTime) {
  ASSERT_EQ(timer().Begin(), Error::kOk);

  EXPECT_EQ(timer().Begin(), Error::kBegunAlready);
}

TEST_F(TimerTest, BeginRefusesAPeriodAboveTheLongestAndStaysNotBegun) {
  EXPECT_EQ(timer().Begin(static_cast<TimeType>(Timer2::kPeriodMax + 1U)),
            Error::kPeriodOutOfRange);

  EXPECT_EQ(timer().GetState(), State::kNotBegun);
}

TEST_F(TimerTest, BeginRefusesAPeriodOfZeroAndStaysNotBegun) {
  EXPECT_EQ(timer().Begin(0U, CountOverflow), Error::kPeriodOutOfRange);

  EXPECT_EQ(timer().GetState(), State::kNotBegun);
  EXPECT_EQ(timer().GetCallback(), nullptr);
  EXPECT_EQ(timer().Start(), Error::kNotBegun);
}

TEST_F(TimerTest, TheLongestPeriodIsAbout33Milliseconds) {
  // 256 steps up and down at 16 MHz / 1024.
  EXPECT_EQ(Timer2::kPeriodMax, 32767U);
}

// The period is 2 * OCR2A * prescaler / 16 MHz: up to OCR2A and down again.
TEST_F(TimerTest, APeriodSetsTheTopAndTheSmallestPrescaler) {
  ASSERT_EQ(timer().Begin(10U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  EXPECT_EQ(OCR2A, 80);
  EXPECT_EQ(ClockSelect(), kDivide1);

  struct Case {
    TimeType period;
    uint8_t top;
    uint8_t clock_select;
  };
  std::array<Case, 6> const cases{{{100U, 100, kDivide8},
                                   {1000U, 250, kDivide32},
                                   {2000U, 250, kDivide64},
                                   {4000U, 250, kDivide128},
                                   {8000U, 250, kDivide256},
                                   {32767U, 255, kDivide1024}}};
  for (auto const& c : cases) {
    EXPECT_EQ(timer().SetPeriod(c.period), Error::kOk) << c.period << " us";
    EXPECT_EQ(OCR2A, c.top) << c.period << " us";
    EXPECT_EQ(ClockSelect(), c.clock_select) << c.period << " us";
  }
}

TEST_F(TimerTest, SetPeriodIsRefusedBeforeBegin) {
  EXPECT_EQ(timer().SetPeriod(1000U), Error::kNotBegun);

  EXPECT_EQ(OCR2A, 0);
}

TEST_F(TimerTest, SetPeriodRefusesAPeriodAboveTheLongest) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  EXPECT_EQ(timer().SetPeriod(static_cast<TimeType>(Timer2::kPeriodMax + 1U)),
            Error::kPeriodOutOfRange);
  EXPECT_EQ(OCR2A, 250);
}

TEST_F(TimerTest, SetPeriodRefusesAPeriodOfZero) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().SetPeriod(0U), Error::kPeriodOutOfRange);

  EXPECT_EQ(OCR2A, 250);
  EXPECT_EQ(ClockSelect(), kDivide32);
}

TEST_F(TimerTest, SetPeriodKeepsTheShareOfTheDuty) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().EnablePwm(255), Error::kOk);

  ASSERT_EQ(timer().SetPeriod(100U), Error::kOk);

  EXPECT_EQ(OCR2A, 100);
  EXPECT_EQ(OCR2B, 100);
}

TEST_F(TimerTest, StartSelectsTheClockFromZero) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  TCNT2.Set(42, false);

  EXPECT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().GetState(), State::kRunning);
  EXPECT_EQ(ClockSelect(), kDivide32);
  EXPECT_EQ(static_cast<uint8_t>(TCNT2), 0);
}

TEST_F(TimerTest, StartWithoutCallbackLeavesTheInterruptOff) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
}

TEST_F(TimerTest, StartWithCallbackEnablesTheInterrupt) {
  ASSERT_EQ(timer().Begin(1000U, CountOverflow), Error::kOk);
  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK2, TOIE2));
}

// The core's init() runs Timer2 before setup(), so its overflow flag is set:
// enabled with it, the interrupt would come at once.
TEST_F(TimerTest, StartClearsTheOverflowFlagBeforeItEnablesTheInterrupt) {
  ASSERT_EQ(timer().Begin(1000U, CountOverflow), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  // Written with a 1, which clears it.
  EXPECT_TRUE(BitIsSet(TIFR2, TOV2));
}

TEST_F(TimerTest, AttachInterruptWhileRunningClearsTheOverflowFlag) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_FALSE(BitIsSet(TIFR2, TOV2));

  ASSERT_EQ(timer().AttachInterrupt(CountOverflow), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIFR2, TOV2));
}

TEST_F(TimerTest, StartIsRefusedBeforeBegin) {
  EXPECT_EQ(timer().Start(), Error::kNotBegun);

  EXPECT_EQ(ClockSelect(), 0);
}

TEST_F(TimerTest, StartIsRefusedWhileRunning) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().Start(), Error::kRunning);
}

TEST_F(TimerTest, StartAfterStopBeginsFromZero) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().Stop(), Error::kOk);
  TCNT2.Set(42, true);

  EXPECT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(static_cast<uint8_t>(TCNT2), 0);
}

TEST_F(TimerTest, TheOverflowCallsTheCallback) {
  ASSERT_EQ(timer().Begin(1000U, CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  timer().OnOverflow();
  timer().OnOverflow();

  EXPECT_EQ(overflow_calls, 2);
}

TEST_F(TimerTest, TheOverflowWithoutCallbackCallsNothing) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  timer().OnOverflow();

  EXPECT_EQ(overflow_calls, 0);
}

TEST_F(TimerTest, StopDeselectsTheClock) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().Stop(), Error::kOk);

  EXPECT_EQ(timer().GetState(), State::kStopped);
  EXPECT_EQ(ClockSelect(), 0);
}

TEST_F(TimerTest, StopIsRefusedUnlessRunning) {
  EXPECT_EQ(timer().Stop(), Error::kNotRunning);
  EXPECT_EQ(timer().GetState(), State::kNotBegun);

  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  EXPECT_EQ(timer().Stop(), Error::kNotRunning);
  EXPECT_EQ(timer().GetState(), State::kIdle);
}

TEST_F(TimerTest, ResumeLetsTheTimerRunAgain) {
  ASSERT_EQ(timer().Begin(100U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().Stop(), Error::kOk);

  EXPECT_EQ(timer().Resume(), Error::kOk);

  EXPECT_EQ(timer().GetState(), State::kRunning);
  EXPECT_EQ(ClockSelect(), kDivide8);
  EXPECT_TRUE(timer().Read().IsOk());
}

TEST_F(TimerTest, ResumeIsRefusedUnlessStopped) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  EXPECT_EQ(timer().Resume(), Error::kNotStopped);

  ASSERT_EQ(timer().Start(), Error::kOk);
  EXPECT_EQ(timer().Resume(), Error::kNotStopped);
}

TEST_F(TimerTest, EnablePwmDrivesPin3WithTheDutyOfTheTop) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  EXPECT_EQ(timer().EnablePwm(128), Error::kOk);

  EXPECT_EQ(mock_pin_mode[3], OUTPUT);
  EXPECT_TRUE(BitIsSet(TCCR2A, COM2B1));
  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B0));
  // 128 / 255 of the top, 250, rounded.
  EXPECT_EQ(OCR2B, 125);
}

TEST_F(TimerTest, EnablePwmIsRefusedBeforeBegin) {
  EXPECT_EQ(timer().EnablePwm(128), Error::kNotBegun);

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
  EXPECT_EQ(mock_pin_mode[3], INPUT);
}

TEST_F(TimerTest, AFullDutyIsTheTopAndNoDutyTheBottom) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  // OCR2B at the top keeps the output high all the time, at the bottom low.
  ASSERT_EQ(timer().EnablePwm(255), Error::kOk);
  EXPECT_EQ(OCR2B, OCR2A);
  ASSERT_EQ(timer().SetPwmDuty(0), Error::kOk);
  EXPECT_EQ(OCR2B, 0);
}

TEST_F(TimerTest, SetPwmDutySetsTheCompareValue) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().EnablePwm(128), Error::kOk);

  EXPECT_EQ(timer().SetPwmDuty(64), Error::kOk);

  // 64 / 255 of the top, 250, rounded.
  EXPECT_EQ(OCR2B, 63);
}

TEST_F(TimerTest, SetPwmDutyIsRefusedBeforeBegin) {
  OCR2B = 7;

  EXPECT_EQ(timer().SetPwmDuty(64), Error::kNotBegun);

  EXPECT_EQ(OCR2B, 7);
}

TEST_F(TimerTest, DisablePwmDisconnectsPin3) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().EnablePwm(128), Error::kOk);

  timer().DisablePwm();

  EXPECT_FALSE(BitIsSet(TCCR2A, COM2B1));
}

TEST_F(TimerTest, AttachInterruptRefusesNullptr) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  EXPECT_EQ(timer().AttachInterrupt(nullptr), Error::kNoCallback);

  EXPECT_EQ(timer().GetCallback(), nullptr);
}

TEST_F(TimerTest, AttachInterruptWhileRunningEnablesTheInterrupt) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_EQ(timer().AttachInterrupt(CountOverflow), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK2, TOIE2));
  timer().OnOverflow();
  EXPECT_EQ(overflow_calls, 1);
}

TEST_F(TimerTest, AttachInterruptBeforeStartLeavesTheInterruptOff) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);

  EXPECT_EQ(timer().AttachInterrupt(CountOverflow), Error::kOk);

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
  EXPECT_EQ(timer().GetCallback(), &CountOverflow);
}

// OIL 7: the interrupt stayed off, and the callback was never called.
TEST_F(TimerTest, AttachInterruptWhileStoppedCallsTheCallbackAfterResume) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  ASSERT_EQ(timer().Stop(), Error::kOk);

  ASSERT_EQ(timer().AttachInterrupt(CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Resume(), Error::kOk);

  EXPECT_TRUE(BitIsSet(TIMSK2, TOIE2));
}

TEST_F(TimerTest, DetachInterruptDisablesTheInterruptAndForgetsTheCallback) {
  ASSERT_EQ(timer().Begin(1000U, CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);

  timer().DetachInterrupt();

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
  EXPECT_EQ(timer().GetCallback(), nullptr);
}

TEST_F(TimerTest, StartAfterDetachInterruptLeavesTheInterruptOff) {
  ASSERT_EQ(timer().Begin(1000U, CountOverflow), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  timer().DetachInterrupt();
  ASSERT_EQ(timer().Stop(), Error::kOk);

  ASSERT_EQ(timer().Start(), Error::kOk);

  EXPECT_FALSE(BitIsSet(TIMSK2, TOIE2));
}

TEST_F(TimerTest, ReadIsRefusedUnlessRunning) {
  ASSERT_EQ(timer().Begin(100U), Error::kOk);

  Result<TimeType> const result{timer().Read()};

  EXPECT_EQ(result.GetError(), Error::kNotRunning);
  EXPECT_EQ(result.GetValue(), 0U);
}

// At 100 us the prescaler is 8: a step of the counter is 0.5 us.
TEST_F(TimerTest, ReadCountsTheStepsUpwards) {
  ASSERT_EQ(timer().Begin(100U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT2.Set(50, true);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 25U);
}

TEST_F(TimerTest, ReadCountsTheStepsBackDownFromTheTop) {
  ASSERT_EQ(timer().Begin(100U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  // 100 steps up and 92 down.
  TCNT2.Set(8, false);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 96U);
}

TEST_F(TimerTest, ReadWaitsForTheCounterToStep) {
  ASSERT_EQ(timer().Begin(100U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT2.Set(50, false);
  // The prescaler of 8 lets the CPU read each value several times.
  TCNT2.SetReadsPerStep(4);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  // 100 steps up and 50 down, of 0.5 us.
  EXPECT_EQ(result.GetValue(), 75U);
}

// 16 steps of the counter are as many microseconds as the prescaler divides
// by: 16 MHz / 16.
TEST_F(TimerTest, ReadScalesByEachPrescaler) {
  ASSERT_EQ(timer().Begin(10U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  struct Case {
    TimeType period;
    TimeType sixteen_steps;
  };
  std::array<Case, 7> const cases{{{10U, 1U},
                                   {100U, 8U},
                                   {1000U, 32U},
                                   {2000U, 64U},
                                   {4000U, 128U},
                                   {8000U, 256U},
                                   {32767U, 1024U}}};

  for (auto const& c : cases) {
    ASSERT_EQ(timer().SetPeriod(c.period), Error::kOk);
    TCNT2.Set(16, true);

    Result<TimeType> const result{timer().Read()};

    ASSERT_TRUE(result.IsOk()) << "period " << c.period;
    EXPECT_EQ(result.GetValue(), c.sixteen_steps) << "period " << c.period;
  }
}

}  // namespace
}  // namespace timer_two
