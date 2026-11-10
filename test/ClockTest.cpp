// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Timer at whatever clock F_CPU is: CMakeLists.txt builds these tests at 1,
// 16 and 20 MHz. Each expectation is computed from F_CPU in 64 bits, apart
// from how Timer computes it.

#include <avr/io.h>
#include <gtest/gtest.h>
#include <stdint.h>

#include <array>

#include "test/printers.h"  // IWYU pragma: keep
#include "timer_two/status.h"
#include "timer_two/timer.h"
#include "timer_two/timer2.h"

namespace timer_two {
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
    TCCR2A = 0;
    TCCR2B = 0;
    OCR2A = 0;
    TCNT2.Set(0, true);
  }

  Timer2& timer() { return timer_; }

 private:
  Timer2 timer_;
};

TEST_F(ClockTest, TheLongestPeriodIsTheLastThatFits) {
  uint64_t const longest{
      ((kLongestPeriodInClocks * kMicrosecondsPerSecond) - 1) / kClock};

  EXPECT_EQ(Timer2::kPeriodMax, longest);
}

TEST_F(ClockTest, TheLongestPeriodIsTakenAndOneMoreRefused) {
  ASSERT_EQ(timer().Begin(Timer2::kPeriodMax), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  EXPECT_EQ(Prescaler(), 1024U);

  EXPECT_EQ(timer().SetPeriod(static_cast<TimeType>(Timer2::kPeriodMax + 1U)),
            Error::kPeriodOutOfRange);
}

// Each period is met to less than a step of the counter up and one down, and
// the two clocks that dividing by 2 and by the prescaler may cut.
TEST_F(ClockTest, EachPeriodIsMetWithinAStep) {
  ASSERT_EQ(timer().Begin(), Error::kOk);
  // The prescaler is in TCCR2B while the timer runs.
  ASSERT_EQ(timer().Start(), Error::kOk);
  constexpr std::array<uint32_t, 8> kPeriods{2,    3,     10,    100,
                                             1000, 10000, 20000, 26214};

  for (uint32_t const period : kPeriods) {
    ASSERT_EQ(timer().SetPeriod(static_cast<TimeType>(period)), Error::kOk)
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

  EXPECT_EQ(timer().Begin(1U),
            kOneMicrosecondIsAStep ? Error::kOk : Error::kPeriodOutOfRange);
}

TEST_F(ClockTest, ReadIsTheTimeOfTheStepsCounted) {
  ASSERT_EQ(timer().Begin(1000U), Error::kOk);
  ASSERT_EQ(timer().Start(), Error::kOk);
  TCNT2.Set(50, true);

  Result<TimeType> const result{timer().Read()};

  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(),
            50 * Prescaler() * kMicrosecondsPerSecond / kClock);
}

}  // namespace
}  // namespace timer_two
