// Copyright 2026 Andreas Burnickl
// SPDX-License-Identifier: GPL-3.0-or-later

#include "timer_two/status.h"

#include <gtest/gtest.h>
#include <stdint.h>

#include <string>

#include "test/printers.h"  // IWYU pragma: keep

namespace timer_two {
namespace {

TEST(StatusTest, IsOkWithoutAnError) {
  Status const status;

  EXPECT_TRUE(status.IsOk());
  EXPECT_EQ(status.GetError(), Error::kOk);
}

TEST(StatusTest, CarriesItsError) {
  Status const status{Error::kNotRunning};

  EXPECT_FALSE(status.IsOk());
  EXPECT_EQ(status.GetError(), Error::kNotRunning);
  EXPECT_TRUE(status == Error::kNotRunning);
  EXPECT_TRUE(status != Error::kRunning);
}

TEST(StatusTest, IsAConstantExpression) {
  static_assert(!Status{Error::kNoCallback}.IsOk(), "made when compiling");
}

TEST(StatusTest, HasATextForEachError) {
  EXPECT_EQ(std::string{TextOf(Error::kOk)}, "ok");
  EXPECT_EQ(std::string{TextOf(Error::kNotBegun)}, "not begun");
  EXPECT_EQ(std::string{TextOf(Error::kBegunAlready)}, "begun already");
  EXPECT_EQ(std::string{TextOf(Error::kRunning)}, "running");
  EXPECT_EQ(std::string{TextOf(Error::kNotRunning)}, "not running");
  EXPECT_EQ(std::string{TextOf(Error::kNotStopped)}, "not stopped");
  EXPECT_EQ(std::string{TextOf(Error::kPeriodOutOfRange)},
            "period out of range");
  EXPECT_EQ(std::string{TextOf(Error::kNoCallback)}, "no callback");
}

TEST(StatusTest, HasATextForAByteThatIsNoError) {
  // A byte that is none of the values of Error, on purpose.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  Error const no_error{static_cast<Error>(200)};

  EXPECT_EQ(std::string{TextOf(no_error)}, "unknown error");
}

TEST(ResultTest, HoldsAValue) {
  Result<uint16_t> const result{uint16_t{42}};

  EXPECT_TRUE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 42U);
  EXPECT_EQ(result.GetError(), Error::kOk);
}

TEST(ResultTest, HoldsAnErrorAndAValueOfZero) {
  Result<uint16_t> const result{Error::kNotRunning};

  EXPECT_FALSE(result.IsOk());
  EXPECT_EQ(result.GetValue(), 0U);
  EXPECT_TRUE(result == Error::kNotRunning);
  EXPECT_TRUE(result != Error::kOk);
  EXPECT_EQ(std::string{reinterpret_cast<char const*>(result.GetErrorText())},
            "not running");
}

}  // namespace
}  // namespace timer_two
