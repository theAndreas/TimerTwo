// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// TimerTwo at whatever clock F_CPU is: CMakeLists.txt builds these tests at
// 1, 16 and 20 MHz. Each expectation is computed from F_CPU in 64 bits, apart
// from how TimerTwo computes it. As in TimerTwoTest.cpp, each test needs a
// process of its own.

#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <array>

#include "StandardTypes.h"
#include "TimerTwo.h"

namespace {

constexpr uint64_t kClock{F_CPU};
constexpr uint64_t kMicrosecondsPerSecond{1000000};

// Mode 5 counts up to the top and down again, 2 * 256 steps at the most, of
// the largest prescaler.
constexpr uint64_t kLongestPeriodInClocks{uint64_t{2} * 256 * 1024};

// The prescaler TCCR2B selects, 0 while the timer does not run.
uint64_t Prescaler() {
  constexpr std::array<uint64_t, 8> kPrescalers{0,  1,   8,   32,
                                                64, 128, 256, 1024};
  return kPrescalers.at(TCCR2B & 0x07U);
}

// The period OCR2A and TCCR2B make, in clocks.
uint64_t PeriodInClocks() { return uint64_t{2} * OCR2A * Prescaler(); }

class ClockTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_EQ(Timer2.getState(), TimerTwo::STATE_INIT)
        << "Each test needs a process of its own; run them through ctest.";
  }
};

TEST_F(ClockTest, TheLongestPeriodIsTheLastThatFits) {
  uint64_t const longest{
      ((kLongestPeriodInClocks * kMicrosecondsPerSecond) - 1) / kClock};

  EXPECT_EQ(Timer2.getPeriodMax(), longest);
}

TEST_F(ClockTest, TheLongestPeriodIsTakenAndOneMoreRefused) {
  ASSERT_EQ(Timer2.init(Timer2.getPeriodMax()), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);
  EXPECT_EQ(Prescaler(), 1024U);

  EXPECT_EQ(Timer2.setPeriod(
                static_cast<TimerTwo::TimeType>(Timer2.getPeriodMax() + 1U)),
            E_NOT_OK);
}

// Each period is met to less than a step of the counter up and one down, and
// the two clocks that dividing by 2 and by the prescaler may cut.
TEST_F(ClockTest, EachPeriodIsMetWithinAStep) {
  ASSERT_EQ(Timer2.init(), E_OK);
  // The prescaler is in TCCR2B while the timer runs.
  ASSERT_EQ(Timer2.start(), E_OK);
  constexpr std::array<uint32_t, 8> kPeriods{2,    3,     10,    100,
                                             1000, 10000, 20000, 26214};

  for (uint32_t const period : kPeriods) {
    ASSERT_EQ(Timer2.setPeriod(static_cast<TimerTwo::TimeType>(period)), E_OK)
        << period << " us";
    uint64_t const wanted{period * kClock / kMicrosecondsPerSecond};

    EXPECT_LE(PeriodInClocks(), wanted) << period << " us";
    EXPECT_LT(wanted - PeriodInClocks(), (2 * Prescaler()) + 2)
        << period << " us";
  }
}

TEST_F(ClockTest, APeriodShorterThanAStepIsRefused) {
  // One step up and one down at no prescaler: two clocks.
  constexpr bool kOneMicrosecondIsAStep{2 * kMicrosecondsPerSecond <= kClock};

  EXPECT_EQ(Timer2.init(1U), kOneMicrosecondIsAStep ? E_OK : E_NOT_OK);
}

TEST_F(ClockTest, ReadIsTheTimeOfTheStepsCounted) {
  ASSERT_EQ(Timer2.init(1000U), E_OK);
  ASSERT_EQ(Timer2.start(), E_OK);
  TCNT2.Set(50, true);
  TimerTwo::TimeType microseconds{0};

  ASSERT_EQ(Timer2.read(microseconds), E_OK);

  EXPECT_EQ(microseconds, 50 * Prescaler() * kMicrosecondsPerSecond / kClock);
}

}  // namespace
