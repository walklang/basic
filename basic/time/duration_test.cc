// Copyright 2017 The Basic Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <chrono>  // NOLINT(build/c++11)
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <random>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "basic/time/time.h"

namespace {

constexpr int64_t kint64max = std::numeric_limits<int64_t>::max();
constexpr int64_t kint64min = std::numeric_limits<int64_t>::min();

// Approximates the given number of years. This is only used to make some test
// code more readable.
basic::Duration ApproxYears(int64_t n) { return basic::Hours(n) * 365 * 24; }

// A gMock matcher to match timespec values. Use this matcher like:
// timespec ts1, ts2;
// EXPECT_THAT(ts1, TimespecMatcher(ts2));
MATCHER_P(TimespecMatcher, ts, "") {
  if (ts.tv_sec == arg.tv_sec && ts.tv_nsec == arg.tv_nsec)
    return true;
  *result_listener << "expected: {" << ts.tv_sec << ", " << ts.tv_nsec << "} ";
  *result_listener << "actual: {" << arg.tv_sec << ", " << arg.tv_nsec << "}";
  return false;
}

// A gMock matcher to match timeval values. Use this matcher like:
// timeval tv1, tv2;
// EXPECT_THAT(tv1, TimevalMatcher(tv2));
MATCHER_P(TimevalMatcher, tv, "") {
  if (tv.tv_sec == arg.tv_sec && tv.tv_usec == arg.tv_usec)
    return true;
  *result_listener << "expected: {" << tv.tv_sec << ", " << tv.tv_usec << "} ";
  *result_listener << "actual: {" << arg.tv_sec << ", " << arg.tv_usec << "}";
  return false;
}

TEST(Duration, ConstExpr) {
  constexpr basic::Duration d0 = basic::ZeroDuration();
  static_assert(d0 == basic::ZeroDuration(), "ZeroDuration()");
  constexpr basic::Duration d1 = basic::Seconds(1);
  static_assert(d1 == basic::Seconds(1), "Seconds(1)");
  static_assert(d1 != basic::ZeroDuration(), "Seconds(1)");
  constexpr basic::Duration d2 = basic::InfiniteDuration();
  static_assert(d2 == basic::InfiniteDuration(), "InfiniteDuration()");
  static_assert(d2 != basic::ZeroDuration(), "InfiniteDuration()");
}

TEST(Duration, ValueSemantics) {
  // If this compiles, the test passes.
  constexpr basic::Duration a;      // Default construction
  constexpr basic::Duration b = a;  // Copy construction
  constexpr basic::Duration c(b);   // Copy construction (again)

  basic::Duration d;
  d = c;  // Assignment
}

TEST(Duration, Factories) {
  constexpr basic::Duration zero = basic::ZeroDuration();
  constexpr basic::Duration nano = basic::Nanoseconds(1);
  constexpr basic::Duration micro = basic::Microseconds(1);
  constexpr basic::Duration milli = basic::Milliseconds(1);
  constexpr basic::Duration sec = basic::Seconds(1);
  constexpr basic::Duration min = basic::Minutes(1);
  constexpr basic::Duration hour = basic::Hours(1);

  EXPECT_EQ(zero, basic::Duration());
  EXPECT_EQ(zero, basic::Seconds(0));
  EXPECT_EQ(nano, basic::Nanoseconds(1));
  EXPECT_EQ(micro, basic::Nanoseconds(1000));
  EXPECT_EQ(milli, basic::Microseconds(1000));
  EXPECT_EQ(sec, basic::Milliseconds(1000));
  EXPECT_EQ(min, basic::Seconds(60));
  EXPECT_EQ(hour, basic::Minutes(60));

  // Tests factory limits
  const basic::Duration inf = basic::InfiniteDuration();

  EXPECT_GT(inf, basic::Seconds(kint64max));
  EXPECT_LT(-inf, basic::Seconds(kint64min));
  EXPECT_LT(-inf, basic::Seconds(-kint64max));

  EXPECT_EQ(inf, basic::Minutes(kint64max));
  EXPECT_EQ(-inf, basic::Minutes(kint64min));
  EXPECT_EQ(-inf, basic::Minutes(-kint64max));
  EXPECT_GT(inf, basic::Minutes(kint64max / 60));
  EXPECT_LT(-inf, basic::Minutes(kint64min / 60));
  EXPECT_LT(-inf, basic::Minutes(-kint64max / 60));

  EXPECT_EQ(inf, basic::Hours(kint64max));
  EXPECT_EQ(-inf, basic::Hours(kint64min));
  EXPECT_EQ(-inf, basic::Hours(-kint64max));
  EXPECT_GT(inf, basic::Hours(kint64max / 3600));
  EXPECT_LT(-inf, basic::Hours(kint64min / 3600));
  EXPECT_LT(-inf, basic::Hours(-kint64max / 3600));
}

TEST(Duration, ToConversion) {
#define TEST_DURATION_CONVERSION(UNIT)                                  \
  do {                                                                  \
    const basic::Duration d = basic::UNIT(1.5);                           \
    constexpr basic::Duration z = basic::ZeroDuration();                  \
    constexpr basic::Duration inf = basic::InfiniteDuration();            \
    constexpr double dbl_inf = std::numeric_limits<double>::infinity(); \
    EXPECT_EQ(kint64min, basic::ToInt64##UNIT(-inf));                    \
    EXPECT_EQ(-1, basic::ToInt64##UNIT(-d));                             \
    EXPECT_EQ(0, basic::ToInt64##UNIT(z));                               \
    EXPECT_EQ(1, basic::ToInt64##UNIT(d));                               \
    EXPECT_EQ(kint64max, basic::ToInt64##UNIT(inf));                     \
    EXPECT_EQ(-dbl_inf, basic::ToDouble##UNIT(-inf));                    \
    EXPECT_EQ(-1.5, basic::ToDouble##UNIT(-d));                          \
    EXPECT_EQ(0, basic::ToDouble##UNIT(z));                              \
    EXPECT_EQ(1.5, basic::ToDouble##UNIT(d));                            \
    EXPECT_EQ(dbl_inf, basic::ToDouble##UNIT(inf));                      \
  } while (0)

  TEST_DURATION_CONVERSION(Nanoseconds);
  TEST_DURATION_CONVERSION(Microseconds);
  TEST_DURATION_CONVERSION(Milliseconds);
  TEST_DURATION_CONVERSION(Seconds);
  TEST_DURATION_CONVERSION(Minutes);
  TEST_DURATION_CONVERSION(Hours);

#undef TEST_DURATION_CONVERSION
}

template <int64_t N>
void TestToConversion() {
  constexpr basic::Duration nano = basic::Nanoseconds(N);
  EXPECT_EQ(N, basic::ToInt64Nanoseconds(nano));
  EXPECT_EQ(0, basic::ToInt64Microseconds(nano));
  EXPECT_EQ(0, basic::ToInt64Milliseconds(nano));
  EXPECT_EQ(0, basic::ToInt64Seconds(nano));
  EXPECT_EQ(0, basic::ToInt64Minutes(nano));
  EXPECT_EQ(0, basic::ToInt64Hours(nano));
  const basic::Duration micro = basic::Microseconds(N);
  EXPECT_EQ(N * 1000, basic::ToInt64Nanoseconds(micro));
  EXPECT_EQ(N, basic::ToInt64Microseconds(micro));
  EXPECT_EQ(0, basic::ToInt64Milliseconds(micro));
  EXPECT_EQ(0, basic::ToInt64Seconds(micro));
  EXPECT_EQ(0, basic::ToInt64Minutes(micro));
  EXPECT_EQ(0, basic::ToInt64Hours(micro));
  const basic::Duration milli = basic::Milliseconds(N);
  EXPECT_EQ(N * 1000 * 1000, basic::ToInt64Nanoseconds(milli));
  EXPECT_EQ(N * 1000, basic::ToInt64Microseconds(milli));
  EXPECT_EQ(N, basic::ToInt64Milliseconds(milli));
  EXPECT_EQ(0, basic::ToInt64Seconds(milli));
  EXPECT_EQ(0, basic::ToInt64Minutes(milli));
  EXPECT_EQ(0, basic::ToInt64Hours(milli));
  const basic::Duration sec = basic::Seconds(N);
  EXPECT_EQ(N * 1000 * 1000 * 1000, basic::ToInt64Nanoseconds(sec));
  EXPECT_EQ(N * 1000 * 1000, basic::ToInt64Microseconds(sec));
  EXPECT_EQ(N * 1000, basic::ToInt64Milliseconds(sec));
  EXPECT_EQ(N, basic::ToInt64Seconds(sec));
  EXPECT_EQ(0, basic::ToInt64Minutes(sec));
  EXPECT_EQ(0, basic::ToInt64Hours(sec));
  const basic::Duration min = basic::Minutes(N);
  EXPECT_EQ(N * 60 * 1000 * 1000 * 1000, basic::ToInt64Nanoseconds(min));
  EXPECT_EQ(N * 60 * 1000 * 1000, basic::ToInt64Microseconds(min));
  EXPECT_EQ(N * 60 * 1000, basic::ToInt64Milliseconds(min));
  EXPECT_EQ(N * 60, basic::ToInt64Seconds(min));
  EXPECT_EQ(N, basic::ToInt64Minutes(min));
  EXPECT_EQ(0, basic::ToInt64Hours(min));
  const basic::Duration hour = basic::Hours(N);
  EXPECT_EQ(N * 60 * 60 * 1000 * 1000 * 1000, basic::ToInt64Nanoseconds(hour));
  EXPECT_EQ(N * 60 * 60 * 1000 * 1000, basic::ToInt64Microseconds(hour));
  EXPECT_EQ(N * 60 * 60 * 1000, basic::ToInt64Milliseconds(hour));
  EXPECT_EQ(N * 60 * 60, basic::ToInt64Seconds(hour));
  EXPECT_EQ(N * 60, basic::ToInt64Minutes(hour));
  EXPECT_EQ(N, basic::ToInt64Hours(hour));
}

TEST(Duration, ToConversionDeprecated) {
  TestToConversion<43>();
  TestToConversion<1>();
  TestToConversion<0>();
  TestToConversion<-1>();
  TestToConversion<-43>();
}

template <int64_t N>
void TestFromChronoBasicEquality() {
  using std::chrono::nanoseconds;
  using std::chrono::microseconds;
  using std::chrono::milliseconds;
  using std::chrono::seconds;
  using std::chrono::minutes;
  using std::chrono::hours;

  static_assert(basic::Nanoseconds(N) == basic::FromChrono(nanoseconds(N)), "");
  static_assert(basic::Microseconds(N) == basic::FromChrono(microseconds(N)), "");
  static_assert(basic::Milliseconds(N) == basic::FromChrono(milliseconds(N)), "");
  static_assert(basic::Seconds(N) == basic::FromChrono(seconds(N)), "");
  static_assert(basic::Minutes(N) == basic::FromChrono(minutes(N)), "");
  static_assert(basic::Hours(N) == basic::FromChrono(hours(N)), "");
}

TEST(Duration, FromChrono) {
  TestFromChronoBasicEquality<-123>();
  TestFromChronoBasicEquality<-1>();
  TestFromChronoBasicEquality<0>();
  TestFromChronoBasicEquality<1>();
  TestFromChronoBasicEquality<123>();

  // Minutes (might, depending on the platform) saturate at +inf.
  const auto chrono_minutes_max = std::chrono::minutes::max();
  const auto minutes_max = basic::FromChrono(chrono_minutes_max);
  const int64_t minutes_max_count = chrono_minutes_max.count();
  if (minutes_max_count > kint64max / 60) {
    EXPECT_EQ(basic::InfiniteDuration(), minutes_max);
  } else {
    EXPECT_EQ(basic::Minutes(minutes_max_count), minutes_max);
  }

  // Minutes (might, depending on the platform) saturate at -inf.
  const auto chrono_minutes_min = std::chrono::minutes::min();
  const auto minutes_min = basic::FromChrono(chrono_minutes_min);
  const int64_t minutes_min_count = chrono_minutes_min.count();
  if (minutes_min_count < kint64min / 60) {
    EXPECT_EQ(-basic::InfiniteDuration(), minutes_min);
  } else {
    EXPECT_EQ(basic::Minutes(minutes_min_count), minutes_min);
  }

  // Hours (might, depending on the platform) saturate at +inf.
  const auto chrono_hours_max = std::chrono::hours::max();
  const auto hours_max = basic::FromChrono(chrono_hours_max);
  const int64_t hours_max_count = chrono_hours_max.count();
  if (hours_max_count > kint64max / 3600) {
    EXPECT_EQ(basic::InfiniteDuration(), hours_max);
  } else {
    EXPECT_EQ(basic::Hours(hours_max_count), hours_max);
  }

  // Hours (might, depending on the platform) saturate at -inf.
  const auto chrono_hours_min = std::chrono::hours::min();
  const auto hours_min = basic::FromChrono(chrono_hours_min);
  const int64_t hours_min_count = chrono_hours_min.count();
  if (hours_min_count < kint64min / 3600) {
    EXPECT_EQ(-basic::InfiniteDuration(), hours_min);
  } else {
    EXPECT_EQ(basic::Hours(hours_min_count), hours_min);
  }
}

template <int64_t N>
void TestToChrono() {
  using std::chrono::nanoseconds;
  using std::chrono::microseconds;
  using std::chrono::milliseconds;
  using std::chrono::seconds;
  using std::chrono::minutes;
  using std::chrono::hours;

  EXPECT_EQ(nanoseconds(N), basic::ToChronoNanoseconds(basic::Nanoseconds(N)));
  EXPECT_EQ(microseconds(N), basic::ToChronoMicroseconds(basic::Microseconds(N)));
  EXPECT_EQ(milliseconds(N), basic::ToChronoMilliseconds(basic::Milliseconds(N)));
  EXPECT_EQ(seconds(N), basic::ToChronoSeconds(basic::Seconds(N)));

  constexpr auto basic_minutes = basic::Minutes(N);
  auto chrono_minutes = minutes(N);
  if (basic_minutes == -basic::InfiniteDuration()) {
    chrono_minutes = minutes::min();
  } else if (basic_minutes == basic::InfiniteDuration()) {
    chrono_minutes = minutes::max();
  }
  EXPECT_EQ(chrono_minutes, basic::ToChronoMinutes(basic_minutes));

  constexpr auto basic_hours = basic::Hours(N);
  auto chrono_hours = hours(N);
  if (basic_hours == -basic::InfiniteDuration()) {
    chrono_hours = hours::min();
  } else if (basic_hours == basic::InfiniteDuration()) {
    chrono_hours = hours::max();
  }
  EXPECT_EQ(chrono_hours, basic::ToChronoHours(basic_hours));
}

TEST(Duration, ToChrono) {
  using std::chrono::nanoseconds;
  using std::chrono::microseconds;
  using std::chrono::milliseconds;
  using std::chrono::seconds;
  using std::chrono::minutes;
  using std::chrono::hours;

  TestToChrono<kint64min>();
  TestToChrono<-1>();
  TestToChrono<0>();
  TestToChrono<1>();
  TestToChrono<kint64max>();

  // Verify truncation toward zero.
  const auto tick = basic::Nanoseconds(1) / 4;
  EXPECT_EQ(nanoseconds(0), basic::ToChronoNanoseconds(tick));
  EXPECT_EQ(nanoseconds(0), basic::ToChronoNanoseconds(-tick));
  EXPECT_EQ(microseconds(0), basic::ToChronoMicroseconds(tick));
  EXPECT_EQ(microseconds(0), basic::ToChronoMicroseconds(-tick));
  EXPECT_EQ(milliseconds(0), basic::ToChronoMilliseconds(tick));
  EXPECT_EQ(milliseconds(0), basic::ToChronoMilliseconds(-tick));
  EXPECT_EQ(seconds(0), basic::ToChronoSeconds(tick));
  EXPECT_EQ(seconds(0), basic::ToChronoSeconds(-tick));
  EXPECT_EQ(minutes(0), basic::ToChronoMinutes(tick));
  EXPECT_EQ(minutes(0), basic::ToChronoMinutes(-tick));
  EXPECT_EQ(hours(0), basic::ToChronoHours(tick));
  EXPECT_EQ(hours(0), basic::ToChronoHours(-tick));

  // Verifies +/- infinity saturation at max/min.
  constexpr auto inf = basic::InfiniteDuration();
  EXPECT_EQ(nanoseconds::min(), basic::ToChronoNanoseconds(-inf));
  EXPECT_EQ(nanoseconds::max(), basic::ToChronoNanoseconds(inf));
  EXPECT_EQ(microseconds::min(), basic::ToChronoMicroseconds(-inf));
  EXPECT_EQ(microseconds::max(), basic::ToChronoMicroseconds(inf));
  EXPECT_EQ(milliseconds::min(), basic::ToChronoMilliseconds(-inf));
  EXPECT_EQ(milliseconds::max(), basic::ToChronoMilliseconds(inf));
  EXPECT_EQ(seconds::min(), basic::ToChronoSeconds(-inf));
  EXPECT_EQ(seconds::max(), basic::ToChronoSeconds(inf));
  EXPECT_EQ(minutes::min(), basic::ToChronoMinutes(-inf));
  EXPECT_EQ(minutes::max(), basic::ToChronoMinutes(inf));
  EXPECT_EQ(hours::min(), basic::ToChronoHours(-inf));
  EXPECT_EQ(hours::max(), basic::ToChronoHours(inf));
}

TEST(Duration, FactoryOverloads) {
  enum E { kOne = 1 };
#define TEST_FACTORY_OVERLOADS(NAME)                                          \
  EXPECT_EQ(1, NAME(kOne) / NAME(kOne));                                      \
  EXPECT_EQ(1, NAME(static_cast<int8_t>(1)) / NAME(1));                       \
  EXPECT_EQ(1, NAME(static_cast<int16_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<int32_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<int64_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<uint8_t>(1)) / NAME(1));                      \
  EXPECT_EQ(1, NAME(static_cast<uint16_t>(1)) / NAME(1));                     \
  EXPECT_EQ(1, NAME(static_cast<uint32_t>(1)) / NAME(1));                     \
  EXPECT_EQ(1, NAME(static_cast<uint64_t>(1)) / NAME(1));                     \
  EXPECT_EQ(NAME(1) / 2, NAME(static_cast<float>(0.5)));                      \
  EXPECT_EQ(NAME(1) / 2, NAME(static_cast<double>(0.5)));                     \
  EXPECT_EQ(1.5, basic::FDivDuration(NAME(static_cast<float>(1.5)), NAME(1))); \
  EXPECT_EQ(1.5, basic::FDivDuration(NAME(static_cast<double>(1.5)), NAME(1)));

  TEST_FACTORY_OVERLOADS(basic::Nanoseconds);
  TEST_FACTORY_OVERLOADS(basic::Microseconds);
  TEST_FACTORY_OVERLOADS(basic::Milliseconds);
  TEST_FACTORY_OVERLOADS(basic::Seconds);
  TEST_FACTORY_OVERLOADS(basic::Minutes);
  TEST_FACTORY_OVERLOADS(basic::Hours);

#undef TEST_FACTORY_OVERLOADS

  EXPECT_EQ(basic::Milliseconds(1500), basic::Seconds(1.5));
  EXPECT_LT(basic::Nanoseconds(1), basic::Nanoseconds(1.5));
  EXPECT_GT(basic::Nanoseconds(2), basic::Nanoseconds(1.5));

  const double dbl_inf = std::numeric_limits<double>::infinity();
  EXPECT_EQ(basic::InfiniteDuration(), basic::Nanoseconds(dbl_inf));
  EXPECT_EQ(basic::InfiniteDuration(), basic::Microseconds(dbl_inf));
  EXPECT_EQ(basic::InfiniteDuration(), basic::Milliseconds(dbl_inf));
  EXPECT_EQ(basic::InfiniteDuration(), basic::Seconds(dbl_inf));
  EXPECT_EQ(basic::InfiniteDuration(), basic::Minutes(dbl_inf));
  EXPECT_EQ(basic::InfiniteDuration(), basic::Hours(dbl_inf));
  EXPECT_EQ(-basic::InfiniteDuration(), basic::Nanoseconds(-dbl_inf));
  EXPECT_EQ(-basic::InfiniteDuration(), basic::Microseconds(-dbl_inf));
  EXPECT_EQ(-basic::InfiniteDuration(), basic::Milliseconds(-dbl_inf));
  EXPECT_EQ(-basic::InfiniteDuration(), basic::Seconds(-dbl_inf));
  EXPECT_EQ(-basic::InfiniteDuration(), basic::Minutes(-dbl_inf));
  EXPECT_EQ(-basic::InfiniteDuration(), basic::Hours(-dbl_inf));
}

TEST(Duration, InfinityExamples) {
  // These examples are used in the documentation in time.h. They are
  // written so that they can be copy-n-pasted easily.

  constexpr basic::Duration inf = basic::InfiniteDuration();
  constexpr basic::Duration d = basic::Seconds(1);  // Any finite duration

  EXPECT_TRUE(inf == inf + inf);
  EXPECT_TRUE(inf == inf + d);
  EXPECT_TRUE(inf == inf - inf);
  EXPECT_TRUE(-inf == d - inf);

  EXPECT_TRUE(inf == d * 1e100);
  EXPECT_TRUE(0 == d / inf);  // NOLINT(readability/check)

  // Division by zero returns infinity, or kint64min/MAX where necessary.
  EXPECT_TRUE(inf == d / 0);
  EXPECT_TRUE(kint64max == d / basic::ZeroDuration());
}

TEST(Duration, InfinityComparison) {
  const basic::Duration inf = basic::InfiniteDuration();
  const basic::Duration any_dur = basic::Seconds(1);

  // Equality
  EXPECT_EQ(inf, inf);
  EXPECT_EQ(-inf, -inf);
  EXPECT_NE(inf, -inf);
  EXPECT_NE(any_dur, inf);
  EXPECT_NE(any_dur, -inf);

  // Relational
  EXPECT_GT(inf, any_dur);
  EXPECT_LT(-inf, any_dur);
  EXPECT_LT(-inf, inf);
  EXPECT_GT(inf, -inf);
}

TEST(Duration, InfinityAddition) {
  const basic::Duration sec_max = basic::Seconds(kint64max);
  const basic::Duration sec_min = basic::Seconds(kint64min);
  const basic::Duration any_dur = basic::Seconds(1);
  const basic::Duration inf = basic::InfiniteDuration();

  // Addition
  EXPECT_EQ(inf, inf + inf);
  EXPECT_EQ(inf, inf + -inf);
  EXPECT_EQ(-inf, -inf + inf);
  EXPECT_EQ(-inf, -inf + -inf);

  EXPECT_EQ(inf, inf + any_dur);
  EXPECT_EQ(inf, any_dur + inf);
  EXPECT_EQ(-inf, -inf + any_dur);
  EXPECT_EQ(-inf, any_dur + -inf);

  // Interesting case
  basic::Duration almost_inf = sec_max + basic::Nanoseconds(999999999);
  EXPECT_GT(inf, almost_inf);
  almost_inf += -basic::Nanoseconds(999999999);
  EXPECT_GT(inf, almost_inf);

  // Addition overflow/underflow
  EXPECT_EQ(inf, sec_max + basic::Seconds(1));
  EXPECT_EQ(inf, sec_max + sec_max);
  EXPECT_EQ(-inf, sec_min + -basic::Seconds(1));
  EXPECT_EQ(-inf, sec_min + -sec_max);

  // For reference: IEEE 754 behavior
  const double dbl_inf = std::numeric_limits<double>::infinity();
  EXPECT_TRUE(std::isinf(dbl_inf + dbl_inf));
  EXPECT_TRUE(std::isnan(dbl_inf + -dbl_inf));  // We return inf
  EXPECT_TRUE(std::isnan(-dbl_inf + dbl_inf));  // We return inf
  EXPECT_TRUE(std::isinf(-dbl_inf + -dbl_inf));
}

TEST(Duration, InfinitySubtraction) {
  const basic::Duration sec_max = basic::Seconds(kint64max);
  const basic::Duration sec_min = basic::Seconds(kint64min);
  const basic::Duration any_dur = basic::Seconds(1);
  const basic::Duration inf = basic::InfiniteDuration();

  // Subtraction
  EXPECT_EQ(inf, inf - inf);
  EXPECT_EQ(inf, inf - -inf);
  EXPECT_EQ(-inf, -inf - inf);
  EXPECT_EQ(-inf, -inf - -inf);

  EXPECT_EQ(inf, inf - any_dur);
  EXPECT_EQ(-inf, any_dur - inf);
  EXPECT_EQ(-inf, -inf - any_dur);
  EXPECT_EQ(inf, any_dur - -inf);

  // Subtraction overflow/underflow
  EXPECT_EQ(inf, sec_max - -basic::Seconds(1));
  EXPECT_EQ(inf, sec_max - -sec_max);
  EXPECT_EQ(-inf, sec_min - basic::Seconds(1));
  EXPECT_EQ(-inf, sec_min - sec_max);

  // Interesting case
  basic::Duration almost_neg_inf = sec_min;
  EXPECT_LT(-inf, almost_neg_inf);
  almost_neg_inf -= -basic::Nanoseconds(1);
  EXPECT_LT(-inf, almost_neg_inf);

  // For reference: IEEE 754 behavior
  const double dbl_inf = std::numeric_limits<double>::infinity();
  EXPECT_TRUE(std::isnan(dbl_inf - dbl_inf));  // We return inf
  EXPECT_TRUE(std::isinf(dbl_inf - -dbl_inf));
  EXPECT_TRUE(std::isinf(-dbl_inf - dbl_inf));
  EXPECT_TRUE(std::isnan(-dbl_inf - -dbl_inf));  // We return inf
}

TEST(Duration, InfinityMultiplication) {
  const basic::Duration sec_max = basic::Seconds(kint64max);
  const basic::Duration sec_min = basic::Seconds(kint64min);
  const basic::Duration inf = basic::InfiniteDuration();

#define TEST_INF_MUL_WITH_TYPE(T)                                     \
  EXPECT_EQ(inf, inf * static_cast<T>(2));                            \
  EXPECT_EQ(-inf, inf * static_cast<T>(-2));                          \
  EXPECT_EQ(-inf, -inf * static_cast<T>(2));                          \
  EXPECT_EQ(inf, -inf * static_cast<T>(-2));                          \
  EXPECT_EQ(inf, inf * static_cast<T>(0));                            \
  EXPECT_EQ(-inf, -inf * static_cast<T>(0));                          \
  EXPECT_EQ(inf, sec_max * static_cast<T>(2));                        \
  EXPECT_EQ(inf, sec_min * static_cast<T>(-2));                       \
  EXPECT_EQ(inf, (sec_max / static_cast<T>(2)) * static_cast<T>(3));  \
  EXPECT_EQ(-inf, sec_max * static_cast<T>(-2));                      \
  EXPECT_EQ(-inf, sec_min * static_cast<T>(2));                       \
  EXPECT_EQ(-inf, (sec_min / static_cast<T>(2)) * static_cast<T>(3));

  TEST_INF_MUL_WITH_TYPE(int64_t);  // NOLINT(readability/function)
  TEST_INF_MUL_WITH_TYPE(double);   // NOLINT(readability/function)

#undef TEST_INF_MUL_WITH_TYPE

  const double dbl_inf = std::numeric_limits<double>::infinity();
  EXPECT_EQ(inf, inf * dbl_inf);
  EXPECT_EQ(-inf, -inf * dbl_inf);
  EXPECT_EQ(-inf, inf * -dbl_inf);
  EXPECT_EQ(inf, -inf * -dbl_inf);

  const basic::Duration any_dur = basic::Seconds(1);
  EXPECT_EQ(inf, any_dur * dbl_inf);
  EXPECT_EQ(-inf, -any_dur * dbl_inf);
  EXPECT_EQ(-inf, any_dur * -dbl_inf);
  EXPECT_EQ(inf, -any_dur * -dbl_inf);

  // Fixed-point multiplication will produce a finite value, whereas floating
  // point fuzziness will overflow to inf.
  EXPECT_NE(basic::InfiniteDuration(), basic::Seconds(1) * kint64max);
  EXPECT_EQ(inf, basic::Seconds(1) * static_cast<double>(kint64max));
  EXPECT_NE(-basic::InfiniteDuration(), basic::Seconds(1) * kint64min);
  EXPECT_EQ(-inf, basic::Seconds(1) * static_cast<double>(kint64min));

  // Note that sec_max * or / by 1.0 overflows to inf due to the 53-bit
  // limitations of double.
  EXPECT_NE(inf, sec_max);
  EXPECT_NE(inf, sec_max / 1);
  EXPECT_EQ(inf, sec_max / 1.0);
  EXPECT_NE(inf, sec_max * 1);
  EXPECT_EQ(inf, sec_max * 1.0);
}

TEST(Duration, InfinityDivision) {
  const basic::Duration sec_max = basic::Seconds(kint64max);
  const basic::Duration sec_min = basic::Seconds(kint64min);
  const basic::Duration inf = basic::InfiniteDuration();

  // Division of Duration by a double
#define TEST_INF_DIV_WITH_TYPE(T)            \
  EXPECT_EQ(inf, inf / static_cast<T>(2));   \
  EXPECT_EQ(-inf, inf / static_cast<T>(-2)); \
  EXPECT_EQ(-inf, -inf / static_cast<T>(2)); \
  EXPECT_EQ(inf, -inf / static_cast<T>(-2));

  TEST_INF_DIV_WITH_TYPE(int64_t);  // NOLINT(readability/function)
  TEST_INF_DIV_WITH_TYPE(double);   // NOLINT(readability/function)

#undef TEST_INF_DIV_WITH_TYPE

  // Division of Duration by a double overflow/underflow
  EXPECT_EQ(inf, sec_max / 0.5);
  EXPECT_EQ(inf, sec_min / -0.5);
  EXPECT_EQ(inf, ((sec_max / 0.5) + basic::Seconds(1)) / 0.5);
  EXPECT_EQ(-inf, sec_max / -0.5);
  EXPECT_EQ(-inf, sec_min / 0.5);
  EXPECT_EQ(-inf, ((sec_min / 0.5) - basic::Seconds(1)) / 0.5);

  const double dbl_inf = std::numeric_limits<double>::infinity();
  EXPECT_EQ(inf, inf / dbl_inf);
  EXPECT_EQ(-inf, inf / -dbl_inf);
  EXPECT_EQ(-inf, -inf / dbl_inf);
  EXPECT_EQ(inf, -inf / -dbl_inf);

  const basic::Duration any_dur = basic::Seconds(1);
  EXPECT_EQ(basic::ZeroDuration(), any_dur / dbl_inf);
  EXPECT_EQ(basic::ZeroDuration(), any_dur / -dbl_inf);
  EXPECT_EQ(basic::ZeroDuration(), -any_dur / dbl_inf);
  EXPECT_EQ(basic::ZeroDuration(), -any_dur / -dbl_inf);
}

TEST(Duration, InfinityModulus) {
  const basic::Duration sec_max = basic::Seconds(kint64max);
  const basic::Duration any_dur = basic::Seconds(1);
  const basic::Duration inf = basic::InfiniteDuration();

  EXPECT_EQ(inf, inf % inf);
  EXPECT_EQ(inf, inf % -inf);
  EXPECT_EQ(-inf, -inf % -inf);
  EXPECT_EQ(-inf, -inf % inf);

  EXPECT_EQ(any_dur, any_dur % inf);
  EXPECT_EQ(any_dur, any_dur % -inf);
  EXPECT_EQ(-any_dur, -any_dur % inf);
  EXPECT_EQ(-any_dur, -any_dur % -inf);

  EXPECT_EQ(inf, inf % -any_dur);
  EXPECT_EQ(inf, inf % any_dur);
  EXPECT_EQ(-inf, -inf % -any_dur);
  EXPECT_EQ(-inf, -inf % any_dur);

  // Remainder isn't affected by overflow.
  EXPECT_EQ(basic::ZeroDuration(), sec_max % basic::Seconds(1));
  EXPECT_EQ(basic::ZeroDuration(), sec_max % basic::Milliseconds(1));
  EXPECT_EQ(basic::ZeroDuration(), sec_max % basic::Microseconds(1));
  EXPECT_EQ(basic::ZeroDuration(), sec_max % basic::Nanoseconds(1));
  EXPECT_EQ(basic::ZeroDuration(), sec_max % basic::Nanoseconds(1) / 4);
}

TEST(Duration, InfinityIDiv) {
  const basic::Duration sec_max = basic::Seconds(kint64max);
  const basic::Duration any_dur = basic::Seconds(1);
  const basic::Duration inf = basic::InfiniteDuration();
  const double dbl_inf = std::numeric_limits<double>::infinity();

  // IDivDuration (int64_t return value + a remainer)
  basic::Duration rem = basic::ZeroDuration();
  EXPECT_EQ(kint64max, basic::IDivDuration(inf, inf, &rem));
  EXPECT_EQ(inf, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(kint64max, basic::IDivDuration(-inf, -inf, &rem));
  EXPECT_EQ(-inf, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(kint64max, basic::IDivDuration(inf, any_dur, &rem));
  EXPECT_EQ(inf, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(0, basic::IDivDuration(any_dur, inf, &rem));
  EXPECT_EQ(any_dur, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(kint64max, basic::IDivDuration(-inf, -any_dur, &rem));
  EXPECT_EQ(-inf, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(0, basic::IDivDuration(-any_dur, -inf, &rem));
  EXPECT_EQ(-any_dur, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(kint64min, basic::IDivDuration(-inf, inf, &rem));
  EXPECT_EQ(-inf, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(kint64min, basic::IDivDuration(inf, -inf, &rem));
  EXPECT_EQ(inf, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(kint64min, basic::IDivDuration(-inf, any_dur, &rem));
  EXPECT_EQ(-inf, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(0, basic::IDivDuration(-any_dur, inf, &rem));
  EXPECT_EQ(-any_dur, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(kint64min, basic::IDivDuration(inf, -any_dur, &rem));
  EXPECT_EQ(inf, rem);

  rem = basic::ZeroDuration();
  EXPECT_EQ(0, basic::IDivDuration(any_dur, -inf, &rem));
  EXPECT_EQ(any_dur, rem);

  // IDivDuration overflow/underflow
  rem = any_dur;
  EXPECT_EQ(kint64max,
            basic::IDivDuration(sec_max, basic::Nanoseconds(1) / 4, &rem));
  EXPECT_EQ(sec_max - basic::Nanoseconds(kint64max) / 4, rem);

  rem = any_dur;
  EXPECT_EQ(kint64max,
            basic::IDivDuration(sec_max, basic::Milliseconds(1), &rem));
  EXPECT_EQ(sec_max - basic::Milliseconds(kint64max), rem);

  rem = any_dur;
  EXPECT_EQ(kint64max,
            basic::IDivDuration(-sec_max, -basic::Milliseconds(1), &rem));
  EXPECT_EQ(-sec_max + basic::Milliseconds(kint64max), rem);

  rem = any_dur;
  EXPECT_EQ(kint64min,
            basic::IDivDuration(-sec_max, basic::Milliseconds(1), &rem));
  EXPECT_EQ(-sec_max - basic::Milliseconds(kint64min), rem);

  rem = any_dur;
  EXPECT_EQ(kint64min,
            basic::IDivDuration(sec_max, -basic::Milliseconds(1), &rem));
  EXPECT_EQ(sec_max + basic::Milliseconds(kint64min), rem);

  //
  // operator/(Duration, Duration) is a wrapper for IDivDuration().
  //

  // IEEE 754 says inf / inf should be nan, but int64_t doesn't have
  // nan so we'll return kint64max/kint64min instead.
  EXPECT_TRUE(std::isnan(dbl_inf / dbl_inf));
  EXPECT_EQ(kint64max, inf / inf);
  EXPECT_EQ(kint64max, -inf / -inf);
  EXPECT_EQ(kint64min, -inf / inf);
  EXPECT_EQ(kint64min, inf / -inf);

  EXPECT_TRUE(std::isinf(dbl_inf / 2.0));
  EXPECT_EQ(kint64max, inf / any_dur);
  EXPECT_EQ(kint64max, -inf / -any_dur);
  EXPECT_EQ(kint64min, -inf / any_dur);
  EXPECT_EQ(kint64min, inf / -any_dur);

  EXPECT_EQ(0.0, 2.0 / dbl_inf);
  EXPECT_EQ(0, any_dur / inf);
  EXPECT_EQ(0, any_dur / -inf);
  EXPECT_EQ(0, -any_dur / inf);
  EXPECT_EQ(0, -any_dur / -inf);
  EXPECT_EQ(0, basic::ZeroDuration() / inf);

  // Division of Duration by a Duration overflow/underflow
  EXPECT_EQ(kint64max, sec_max / basic::Milliseconds(1));
  EXPECT_EQ(kint64max, -sec_max / -basic::Milliseconds(1));
  EXPECT_EQ(kint64min, -sec_max / basic::Milliseconds(1));
  EXPECT_EQ(kint64min, sec_max / -basic::Milliseconds(1));
}

TEST(Duration, InfinityFDiv) {
  const basic::Duration any_dur = basic::Seconds(1);
  const basic::Duration inf = basic::InfiniteDuration();
  const double dbl_inf = std::numeric_limits<double>::infinity();

  EXPECT_EQ(dbl_inf, basic::FDivDuration(inf, inf));
  EXPECT_EQ(dbl_inf, basic::FDivDuration(-inf, -inf));
  EXPECT_EQ(dbl_inf, basic::FDivDuration(inf, any_dur));
  EXPECT_EQ(0.0, basic::FDivDuration(any_dur, inf));
  EXPECT_EQ(dbl_inf, basic::FDivDuration(-inf, -any_dur));
  EXPECT_EQ(0.0, basic::FDivDuration(-any_dur, -inf));

  EXPECT_EQ(-dbl_inf, basic::FDivDuration(-inf, inf));
  EXPECT_EQ(-dbl_inf, basic::FDivDuration(inf, -inf));
  EXPECT_EQ(-dbl_inf, basic::FDivDuration(-inf, any_dur));
  EXPECT_EQ(0.0, basic::FDivDuration(-any_dur, inf));
  EXPECT_EQ(-dbl_inf, basic::FDivDuration(inf, -any_dur));
  EXPECT_EQ(0.0, basic::FDivDuration(any_dur, -inf));
}

TEST(Duration, DivisionByZero) {
  const basic::Duration zero = basic::ZeroDuration();
  const basic::Duration inf = basic::InfiniteDuration();
  const basic::Duration any_dur = basic::Seconds(1);
  const double dbl_inf = std::numeric_limits<double>::infinity();
  const double dbl_denorm = std::numeric_limits<double>::denorm_min();

  // IEEE 754 behavior
  double z = 0.0, two = 2.0;
  EXPECT_TRUE(std::isinf(two / z));
  EXPECT_TRUE(std::isnan(z / z));  // We'll return inf

  // Operator/(Duration, double)
  EXPECT_EQ(inf, zero / 0.0);
  EXPECT_EQ(-inf, zero / -0.0);
  EXPECT_EQ(inf, any_dur / 0.0);
  EXPECT_EQ(-inf, any_dur / -0.0);
  EXPECT_EQ(-inf, -any_dur / 0.0);
  EXPECT_EQ(inf, -any_dur / -0.0);

  // Tests dividing by a number very close to, but not quite zero.
  EXPECT_EQ(zero, zero / dbl_denorm);
  EXPECT_EQ(zero, zero / -dbl_denorm);
  EXPECT_EQ(inf, any_dur / dbl_denorm);
  EXPECT_EQ(-inf, any_dur / -dbl_denorm);
  EXPECT_EQ(-inf, -any_dur / dbl_denorm);
  EXPECT_EQ(inf, -any_dur / -dbl_denorm);

  // IDiv
  basic::Duration rem = zero;
  EXPECT_EQ(kint64max, basic::IDivDuration(zero, zero, &rem));
  EXPECT_EQ(inf, rem);

  rem = zero;
  EXPECT_EQ(kint64max, basic::IDivDuration(any_dur, zero, &rem));
  EXPECT_EQ(inf, rem);

  rem = zero;
  EXPECT_EQ(kint64min, basic::IDivDuration(-any_dur, zero, &rem));
  EXPECT_EQ(-inf, rem);

  // Operator/(Duration, Duration)
  EXPECT_EQ(kint64max, zero / zero);
  EXPECT_EQ(kint64max, any_dur / zero);
  EXPECT_EQ(kint64min, -any_dur / zero);

  // FDiv
  EXPECT_EQ(dbl_inf, basic::FDivDuration(zero, zero));
  EXPECT_EQ(dbl_inf, basic::FDivDuration(any_dur, zero));
  EXPECT_EQ(-dbl_inf, basic::FDivDuration(-any_dur, zero));
}

TEST(Duration, NaN) {
  // Note that IEEE 754 does not define the behavior of a nan's sign when it is
  // copied, so the code below allows for either + or - InfiniteDuration.
#define TEST_NAN_HANDLING(NAME, NAN)           \
  do {                                         \
    const auto inf = basic::InfiniteDuration(); \
    auto x = NAME(NAN);                        \
    EXPECT_TRUE(x == inf || x == -inf);        \
    auto y = NAME(42);                         \
    y *= NAN;                                  \
    EXPECT_TRUE(y == inf || y == -inf);        \
    auto z = NAME(42);                         \
    z /= NAN;                                  \
    EXPECT_TRUE(z == inf || z == -inf);        \
  } while (0)

  const double nan = std::numeric_limits<double>::quiet_NaN();
  TEST_NAN_HANDLING(basic::Nanoseconds, nan);
  TEST_NAN_HANDLING(basic::Microseconds, nan);
  TEST_NAN_HANDLING(basic::Milliseconds, nan);
  TEST_NAN_HANDLING(basic::Seconds, nan);
  TEST_NAN_HANDLING(basic::Minutes, nan);
  TEST_NAN_HANDLING(basic::Hours, nan);

  TEST_NAN_HANDLING(basic::Nanoseconds, -nan);
  TEST_NAN_HANDLING(basic::Microseconds, -nan);
  TEST_NAN_HANDLING(basic::Milliseconds, -nan);
  TEST_NAN_HANDLING(basic::Seconds, -nan);
  TEST_NAN_HANDLING(basic::Minutes, -nan);
  TEST_NAN_HANDLING(basic::Hours, -nan);

#undef TEST_NAN_HANDLING
}

TEST(Duration, Range) {
  const basic::Duration range = ApproxYears(100 * 1e9);
  const basic::Duration range_future = range;
  const basic::Duration range_past = -range;

  EXPECT_LT(range_future, basic::InfiniteDuration());
  EXPECT_GT(range_past, -basic::InfiniteDuration());

  const basic::Duration full_range = range_future - range_past;
  EXPECT_GT(full_range, basic::ZeroDuration());
  EXPECT_LT(full_range, basic::InfiniteDuration());

  const basic::Duration neg_full_range = range_past - range_future;
  EXPECT_LT(neg_full_range, basic::ZeroDuration());
  EXPECT_GT(neg_full_range, -basic::InfiniteDuration());

  EXPECT_LT(neg_full_range, full_range);
  EXPECT_EQ(neg_full_range, -full_range);
}

TEST(Duration, RelationalOperators) {
#define TEST_REL_OPS(UNIT)               \
  static_assert(UNIT(2) == UNIT(2), ""); \
  static_assert(UNIT(1) != UNIT(2), ""); \
  static_assert(UNIT(1) < UNIT(2), "");  \
  static_assert(UNIT(3) > UNIT(2), "");  \
  static_assert(UNIT(1) <= UNIT(2), ""); \
  static_assert(UNIT(2) <= UNIT(2), ""); \
  static_assert(UNIT(3) >= UNIT(2), ""); \
  static_assert(UNIT(2) >= UNIT(2), "");

  TEST_REL_OPS(basic::Nanoseconds);
  TEST_REL_OPS(basic::Microseconds);
  TEST_REL_OPS(basic::Milliseconds);
  TEST_REL_OPS(basic::Seconds);
  TEST_REL_OPS(basic::Minutes);
  TEST_REL_OPS(basic::Hours);

#undef TEST_REL_OPS
}

TEST(Duration, Addition) {
#define TEST_ADD_OPS(UNIT)                  \
  do {                                      \
    EXPECT_EQ(UNIT(2), UNIT(1) + UNIT(1));  \
    EXPECT_EQ(UNIT(1), UNIT(2) - UNIT(1));  \
    EXPECT_EQ(UNIT(0), UNIT(2) - UNIT(2));  \
    EXPECT_EQ(UNIT(-1), UNIT(1) - UNIT(2)); \
    EXPECT_EQ(UNIT(-2), UNIT(0) - UNIT(2)); \
    EXPECT_EQ(UNIT(-2), UNIT(1) - UNIT(3)); \
    basic::Duration a = UNIT(1);             \
    a += UNIT(1);                           \
    EXPECT_EQ(UNIT(2), a);                  \
    a -= UNIT(1);                           \
    EXPECT_EQ(UNIT(1), a);                  \
  } while (0)

  TEST_ADD_OPS(basic::Nanoseconds);
  TEST_ADD_OPS(basic::Microseconds);
  TEST_ADD_OPS(basic::Milliseconds);
  TEST_ADD_OPS(basic::Seconds);
  TEST_ADD_OPS(basic::Minutes);
  TEST_ADD_OPS(basic::Hours);

#undef TEST_ADD_OPS

  EXPECT_EQ(basic::Seconds(2), basic::Seconds(3) - 2 * basic::Milliseconds(500));
  EXPECT_EQ(basic::Seconds(2) + basic::Milliseconds(500),
            basic::Seconds(3) - basic::Milliseconds(500));

  EXPECT_EQ(basic::Seconds(1) + basic::Milliseconds(998),
            basic::Milliseconds(999) + basic::Milliseconds(999));

  EXPECT_EQ(basic::Milliseconds(-1),
            basic::Milliseconds(998) - basic::Milliseconds(999));

  // Tests fractions of a nanoseconds. These are implementation details only.
  EXPECT_GT(basic::Nanoseconds(1), basic::Nanoseconds(1) / 2);
  EXPECT_EQ(basic::Nanoseconds(1),
            basic::Nanoseconds(1) / 2 + basic::Nanoseconds(1) / 2);
  EXPECT_GT(basic::Nanoseconds(1) / 4, basic::Nanoseconds(0));
  EXPECT_EQ(basic::Nanoseconds(1) / 8, basic::Nanoseconds(0));

  // Tests subtraction that will cause wrap around of the rep_lo_ bits.
  basic::Duration d_7_5 = basic::Seconds(7) + basic::Milliseconds(500);
  basic::Duration d_3_7 = basic::Seconds(3) + basic::Milliseconds(700);
  basic::Duration ans_3_8 = basic::Seconds(3) + basic::Milliseconds(800);
  EXPECT_EQ(ans_3_8, d_7_5 - d_3_7);

  // Subtracting min_duration
  basic::Duration min_dur = basic::Seconds(kint64min);
  EXPECT_EQ(basic::Seconds(0), min_dur - min_dur);
  EXPECT_EQ(basic::Seconds(kint64max), basic::Seconds(-1) - min_dur);
}

TEST(Duration, Negation) {
  // By storing negations of various values in constexpr variables we
  // verify that the initializers are constant expressions.
  constexpr basic::Duration negated_zero_duration = -basic::ZeroDuration();
  EXPECT_EQ(negated_zero_duration, basic::ZeroDuration());

  constexpr basic::Duration negated_infinite_duration =
      -basic::InfiniteDuration();
  EXPECT_NE(negated_infinite_duration, basic::InfiniteDuration());
  EXPECT_EQ(-negated_infinite_duration, basic::InfiniteDuration());

  // The public APIs to check if a duration is infinite depend on using
  // -InfiniteDuration(), but we're trying to test operator- here, so we
  // need to use the lower-level internal query IsInfiniteDuration.
  EXPECT_TRUE(
      basic::time_internal::IsInfiniteDuration(negated_infinite_duration));

  // The largest Duration is kint64max seconds and kTicksPerSecond - 1 ticks.
  // Using the basic::time_internal::MakeDuration API is the cleanest way to
  // construct that Duration.
  constexpr basic::Duration max_duration = basic::time_internal::MakeDuration(
      kint64max, basic::time_internal::kTicksPerSecond - 1);
  constexpr basic::Duration negated_max_duration = -max_duration;
  // The largest negatable value is one tick above the minimum representable;
  // it's the negation of max_duration.
  constexpr basic::Duration nearly_min_duration =
      basic::time_internal::MakeDuration(kint64min, int64_t{1});
  constexpr basic::Duration negated_nearly_min_duration = -nearly_min_duration;

  EXPECT_EQ(negated_max_duration, nearly_min_duration);
  EXPECT_EQ(negated_nearly_min_duration, max_duration);
  EXPECT_EQ(-(-max_duration), max_duration);

  constexpr basic::Duration min_duration =
      basic::time_internal::MakeDuration(kint64min);
  constexpr basic::Duration negated_min_duration = -min_duration;
  EXPECT_EQ(negated_min_duration, basic::InfiniteDuration());
}

TEST(Duration, AbsoluteValue) {
  EXPECT_EQ(basic::ZeroDuration(), AbsDuration(basic::ZeroDuration()));
  EXPECT_EQ(basic::Seconds(1), AbsDuration(basic::Seconds(1)));
  EXPECT_EQ(basic::Seconds(1), AbsDuration(basic::Seconds(-1)));

  EXPECT_EQ(basic::InfiniteDuration(), AbsDuration(basic::InfiniteDuration()));
  EXPECT_EQ(basic::InfiniteDuration(), AbsDuration(-basic::InfiniteDuration()));

  basic::Duration max_dur =
      basic::Seconds(kint64max) + (basic::Seconds(1) - basic::Nanoseconds(1) / 4);
  EXPECT_EQ(max_dur, AbsDuration(max_dur));

  basic::Duration min_dur = basic::Seconds(kint64min);
  EXPECT_EQ(basic::InfiniteDuration(), AbsDuration(min_dur));
  EXPECT_EQ(max_dur, AbsDuration(min_dur + basic::Nanoseconds(1) / 4));
}

TEST(Duration, Multiplication) {
#define TEST_MUL_OPS(UNIT)                                    \
  do {                                                        \
    EXPECT_EQ(UNIT(5), UNIT(2) * 2.5);                        \
    EXPECT_EQ(UNIT(2), UNIT(5) / 2.5);                        \
    EXPECT_EQ(UNIT(-5), UNIT(-2) * 2.5);                      \
    EXPECT_EQ(UNIT(-5), -UNIT(2) * 2.5);                      \
    EXPECT_EQ(UNIT(-5), UNIT(2) * -2.5);                      \
    EXPECT_EQ(UNIT(-2), UNIT(-5) / 2.5);                      \
    EXPECT_EQ(UNIT(-2), -UNIT(5) / 2.5);                      \
    EXPECT_EQ(UNIT(-2), UNIT(5) / -2.5);                      \
    EXPECT_EQ(UNIT(2), UNIT(11) % UNIT(3));                   \
    basic::Duration a = UNIT(2);                               \
    a *= 2.5;                                                 \
    EXPECT_EQ(UNIT(5), a);                                    \
    a /= 2.5;                                                 \
    EXPECT_EQ(UNIT(2), a);                                    \
    a %= UNIT(1);                                             \
    EXPECT_EQ(UNIT(0), a);                                    \
    basic::Duration big = UNIT(1000000000);                    \
    big *= 3;                                                 \
    big /= 3;                                                 \
    EXPECT_EQ(UNIT(1000000000), big);                         \
    EXPECT_EQ(-UNIT(2), -UNIT(2));                            \
    EXPECT_EQ(-UNIT(2), UNIT(2) * -1);                        \
    EXPECT_EQ(-UNIT(2), -1 * UNIT(2));                        \
    EXPECT_EQ(-UNIT(-2), UNIT(2));                            \
    EXPECT_EQ(2, UNIT(2) / UNIT(1));                          \
    basic::Duration rem;                                       \
    EXPECT_EQ(2, basic::IDivDuration(UNIT(2), UNIT(1), &rem)); \
    EXPECT_EQ(2.0, basic::FDivDuration(UNIT(2), UNIT(1)));     \
  } while (0)

  TEST_MUL_OPS(basic::Nanoseconds);
  TEST_MUL_OPS(basic::Microseconds);
  TEST_MUL_OPS(basic::Milliseconds);
  TEST_MUL_OPS(basic::Seconds);
  TEST_MUL_OPS(basic::Minutes);
  TEST_MUL_OPS(basic::Hours);

#undef TEST_MUL_OPS

  // Ensures that multiplication and division by 1 with a maxed-out durations
  // doesn't lose precision.
  basic::Duration max_dur =
      basic::Seconds(kint64max) + (basic::Seconds(1) - basic::Nanoseconds(1) / 4);
  basic::Duration min_dur = basic::Seconds(kint64min);
  EXPECT_EQ(max_dur, max_dur * 1);
  EXPECT_EQ(max_dur, max_dur / 1);
  EXPECT_EQ(min_dur, min_dur * 1);
  EXPECT_EQ(min_dur, min_dur / 1);

  // Tests division on a Duration with a large number of significant digits.
  // Tests when the digits span hi and lo as well as only in hi.
  basic::Duration sigfigs = basic::Seconds(2000000000) + basic::Nanoseconds(3);
  EXPECT_EQ(basic::Seconds(666666666) + basic::Nanoseconds(666666667) +
                basic::Nanoseconds(1) / 2,
            sigfigs / 3);
  sigfigs = basic::Seconds(7000000000LL);
  EXPECT_EQ(basic::Seconds(2333333333) + basic::Nanoseconds(333333333) +
                basic::Nanoseconds(1) / 4,
            sigfigs / 3);

  EXPECT_EQ(basic::Seconds(7) + basic::Milliseconds(500), basic::Seconds(3) * 2.5);
  EXPECT_EQ(basic::Seconds(8) * -1 + basic::Milliseconds(300),
            (basic::Seconds(2) + basic::Milliseconds(200)) * -3.5);
  EXPECT_EQ(-basic::Seconds(8) + basic::Milliseconds(300),
            (basic::Seconds(2) + basic::Milliseconds(200)) * -3.5);
  EXPECT_EQ(basic::Seconds(1) + basic::Milliseconds(875),
            (basic::Seconds(7) + basic::Milliseconds(500)) / 4);
  EXPECT_EQ(basic::Seconds(30),
            (basic::Seconds(7) + basic::Milliseconds(500)) / 0.25);
  EXPECT_EQ(basic::Seconds(3),
            (basic::Seconds(7) + basic::Milliseconds(500)) / 2.5);

  // Tests division remainder.
  EXPECT_EQ(basic::Nanoseconds(0), basic::Nanoseconds(7) % basic::Nanoseconds(1));
  EXPECT_EQ(basic::Nanoseconds(0), basic::Nanoseconds(0) % basic::Nanoseconds(10));
  EXPECT_EQ(basic::Nanoseconds(2), basic::Nanoseconds(7) % basic::Nanoseconds(5));
  EXPECT_EQ(basic::Nanoseconds(2), basic::Nanoseconds(2) % basic::Nanoseconds(5));

  EXPECT_EQ(basic::Nanoseconds(1), basic::Nanoseconds(10) % basic::Nanoseconds(3));
  EXPECT_EQ(basic::Nanoseconds(1),
            basic::Nanoseconds(10) % basic::Nanoseconds(-3));
  EXPECT_EQ(basic::Nanoseconds(-1),
            basic::Nanoseconds(-10) % basic::Nanoseconds(3));
  EXPECT_EQ(basic::Nanoseconds(-1),
            basic::Nanoseconds(-10) % basic::Nanoseconds(-3));

  EXPECT_EQ(basic::Milliseconds(100),
            basic::Seconds(1) % basic::Milliseconds(300));
  EXPECT_EQ(
      basic::Milliseconds(300),
      (basic::Seconds(3) + basic::Milliseconds(800)) % basic::Milliseconds(500));

  EXPECT_EQ(basic::Nanoseconds(1), basic::Nanoseconds(1) % basic::Seconds(1));
  EXPECT_EQ(basic::Nanoseconds(-1), basic::Nanoseconds(-1) % basic::Seconds(1));
  EXPECT_EQ(0, basic::Nanoseconds(-1) / basic::Seconds(1));  // Actual -1e-9

  // Tests identity a = (a/b)*b + a%b
#define TEST_MOD_IDENTITY(a, b) \
  EXPECT_EQ((a), ((a) / (b))*(b) + ((a)%(b)))

  TEST_MOD_IDENTITY(basic::Seconds(0), basic::Seconds(2));
  TEST_MOD_IDENTITY(basic::Seconds(1), basic::Seconds(1));
  TEST_MOD_IDENTITY(basic::Seconds(1), basic::Seconds(2));
  TEST_MOD_IDENTITY(basic::Seconds(2), basic::Seconds(1));

  TEST_MOD_IDENTITY(basic::Seconds(-2), basic::Seconds(1));
  TEST_MOD_IDENTITY(basic::Seconds(2), basic::Seconds(-1));
  TEST_MOD_IDENTITY(basic::Seconds(-2), basic::Seconds(-1));

  TEST_MOD_IDENTITY(basic::Nanoseconds(0), basic::Nanoseconds(2));
  TEST_MOD_IDENTITY(basic::Nanoseconds(1), basic::Nanoseconds(1));
  TEST_MOD_IDENTITY(basic::Nanoseconds(1), basic::Nanoseconds(2));
  TEST_MOD_IDENTITY(basic::Nanoseconds(2), basic::Nanoseconds(1));

  TEST_MOD_IDENTITY(basic::Nanoseconds(-2), basic::Nanoseconds(1));
  TEST_MOD_IDENTITY(basic::Nanoseconds(2), basic::Nanoseconds(-1));
  TEST_MOD_IDENTITY(basic::Nanoseconds(-2), basic::Nanoseconds(-1));

  // Mixed seconds + subseconds
  basic::Duration mixed_a = basic::Seconds(1) + basic::Nanoseconds(2);
  basic::Duration mixed_b = basic::Seconds(1) + basic::Nanoseconds(3);

  TEST_MOD_IDENTITY(basic::Seconds(0), mixed_a);
  TEST_MOD_IDENTITY(mixed_a, mixed_a);
  TEST_MOD_IDENTITY(mixed_a, mixed_b);
  TEST_MOD_IDENTITY(mixed_b, mixed_a);

  TEST_MOD_IDENTITY(-mixed_a, mixed_b);
  TEST_MOD_IDENTITY(mixed_a, -mixed_b);
  TEST_MOD_IDENTITY(-mixed_a, -mixed_b);

#undef TEST_MOD_IDENTITY
}

TEST(Duration, Truncation) {
  const basic::Duration d = basic::Nanoseconds(1234567890);
  const basic::Duration inf = basic::InfiniteDuration();
  for (int unit_sign : {1, -1}) {  // sign shouldn't matter
    EXPECT_EQ(basic::Nanoseconds(1234567890),
              Trunc(d, unit_sign * basic::Nanoseconds(1)));
    EXPECT_EQ(basic::Microseconds(1234567),
              Trunc(d, unit_sign * basic::Microseconds(1)));
    EXPECT_EQ(basic::Milliseconds(1234),
              Trunc(d, unit_sign * basic::Milliseconds(1)));
    EXPECT_EQ(basic::Seconds(1), Trunc(d, unit_sign * basic::Seconds(1)));
    EXPECT_EQ(inf, Trunc(inf, unit_sign * basic::Seconds(1)));

    EXPECT_EQ(basic::Nanoseconds(-1234567890),
              Trunc(-d, unit_sign * basic::Nanoseconds(1)));
    EXPECT_EQ(basic::Microseconds(-1234567),
              Trunc(-d, unit_sign * basic::Microseconds(1)));
    EXPECT_EQ(basic::Milliseconds(-1234),
              Trunc(-d, unit_sign * basic::Milliseconds(1)));
    EXPECT_EQ(basic::Seconds(-1), Trunc(-d, unit_sign * basic::Seconds(1)));
    EXPECT_EQ(-inf, Trunc(-inf, unit_sign * basic::Seconds(1)));
  }
}

TEST(Duration, Flooring) {
  const basic::Duration d = basic::Nanoseconds(1234567890);
  const basic::Duration inf = basic::InfiniteDuration();
  for (int unit_sign : {1, -1}) {  // sign shouldn't matter
    EXPECT_EQ(basic::Nanoseconds(1234567890),
              basic::Floor(d, unit_sign * basic::Nanoseconds(1)));
    EXPECT_EQ(basic::Microseconds(1234567),
              basic::Floor(d, unit_sign * basic::Microseconds(1)));
    EXPECT_EQ(basic::Milliseconds(1234),
              basic::Floor(d, unit_sign * basic::Milliseconds(1)));
    EXPECT_EQ(basic::Seconds(1), basic::Floor(d, unit_sign * basic::Seconds(1)));
    EXPECT_EQ(inf, basic::Floor(inf, unit_sign * basic::Seconds(1)));

    EXPECT_EQ(basic::Nanoseconds(-1234567890),
              basic::Floor(-d, unit_sign * basic::Nanoseconds(1)));
    EXPECT_EQ(basic::Microseconds(-1234568),
              basic::Floor(-d, unit_sign * basic::Microseconds(1)));
    EXPECT_EQ(basic::Milliseconds(-1235),
              basic::Floor(-d, unit_sign * basic::Milliseconds(1)));
    EXPECT_EQ(basic::Seconds(-2), basic::Floor(-d, unit_sign * basic::Seconds(1)));
    EXPECT_EQ(-inf, basic::Floor(-inf, unit_sign * basic::Seconds(1)));
  }
}

TEST(Duration, Ceiling) {
  const basic::Duration d = basic::Nanoseconds(1234567890);
  const basic::Duration inf = basic::InfiniteDuration();
  for (int unit_sign : {1, -1}) {  // // sign shouldn't matter
    EXPECT_EQ(basic::Nanoseconds(1234567890),
              basic::Ceil(d, unit_sign * basic::Nanoseconds(1)));
    EXPECT_EQ(basic::Microseconds(1234568),
              basic::Ceil(d, unit_sign * basic::Microseconds(1)));
    EXPECT_EQ(basic::Milliseconds(1235),
              basic::Ceil(d, unit_sign * basic::Milliseconds(1)));
    EXPECT_EQ(basic::Seconds(2), basic::Ceil(d, unit_sign * basic::Seconds(1)));
    EXPECT_EQ(inf, basic::Ceil(inf, unit_sign * basic::Seconds(1)));

    EXPECT_EQ(basic::Nanoseconds(-1234567890),
              basic::Ceil(-d, unit_sign * basic::Nanoseconds(1)));
    EXPECT_EQ(basic::Microseconds(-1234567),
              basic::Ceil(-d, unit_sign * basic::Microseconds(1)));
    EXPECT_EQ(basic::Milliseconds(-1234),
              basic::Ceil(-d, unit_sign * basic::Milliseconds(1)));
    EXPECT_EQ(basic::Seconds(-1), basic::Ceil(-d, unit_sign * basic::Seconds(1)));
    EXPECT_EQ(-inf, basic::Ceil(-inf, unit_sign * basic::Seconds(1)));
  }
}

TEST(Duration, RoundTripUnits) {
  const int kRange = 100000;

#define ROUND_TRIP_UNIT(U, LOW, HIGH)          \
  do {                                         \
    for (int64_t i = LOW; i < HIGH; ++i) {     \
      basic::Duration d = basic::U(i);           \
      if (d == basic::InfiniteDuration())       \
        EXPECT_EQ(kint64max, d / basic::U(1));  \
      else if (d == -basic::InfiniteDuration()) \
        EXPECT_EQ(kint64min, d / basic::U(1));  \
      else                                     \
        EXPECT_EQ(i, basic::U(i) / basic::U(1)); \
    }                                          \
  } while (0)

  ROUND_TRIP_UNIT(Nanoseconds, kint64min, kint64min + kRange);
  ROUND_TRIP_UNIT(Nanoseconds, -kRange, kRange);
  ROUND_TRIP_UNIT(Nanoseconds, kint64max - kRange, kint64max);

  ROUND_TRIP_UNIT(Microseconds, kint64min, kint64min + kRange);
  ROUND_TRIP_UNIT(Microseconds, -kRange, kRange);
  ROUND_TRIP_UNIT(Microseconds, kint64max - kRange, kint64max);

  ROUND_TRIP_UNIT(Milliseconds, kint64min, kint64min + kRange);
  ROUND_TRIP_UNIT(Milliseconds, -kRange, kRange);
  ROUND_TRIP_UNIT(Milliseconds, kint64max - kRange, kint64max);

  ROUND_TRIP_UNIT(Seconds, kint64min, kint64min + kRange);
  ROUND_TRIP_UNIT(Seconds, -kRange, kRange);
  ROUND_TRIP_UNIT(Seconds, kint64max - kRange, kint64max);

  ROUND_TRIP_UNIT(Minutes, kint64min / 60, kint64min / 60 + kRange);
  ROUND_TRIP_UNIT(Minutes, -kRange, kRange);
  ROUND_TRIP_UNIT(Minutes, kint64max / 60 - kRange, kint64max / 60);

  ROUND_TRIP_UNIT(Hours, kint64min / 3600, kint64min / 3600 + kRange);
  ROUND_TRIP_UNIT(Hours, -kRange, kRange);
  ROUND_TRIP_UNIT(Hours, kint64max / 3600 - kRange, kint64max / 3600);

#undef ROUND_TRIP_UNIT
}

TEST(Duration, TruncConversions) {
  // Tests ToTimespec()/DurationFromTimespec()
  const struct {
    basic::Duration d;
    timespec ts;
  } to_ts[] = {
      {basic::Seconds(1) + basic::Nanoseconds(1), {1, 1}},
      {basic::Seconds(1) + basic::Nanoseconds(1) / 2, {1, 0}},
      {basic::Seconds(1) + basic::Nanoseconds(0), {1, 0}},
      {basic::Seconds(0) + basic::Nanoseconds(0), {0, 0}},
      {basic::Seconds(0) - basic::Nanoseconds(1) / 2, {0, 0}},
      {basic::Seconds(0) - basic::Nanoseconds(1), {-1, 999999999}},
      {basic::Seconds(-1) + basic::Nanoseconds(1), {-1, 1}},
      {basic::Seconds(-1) + basic::Nanoseconds(1) / 2, {-1, 1}},
      {basic::Seconds(-1) + basic::Nanoseconds(0), {-1, 0}},
      {basic::Seconds(-1) - basic::Nanoseconds(1) / 2, {-1, 0}},
  };
  for (const auto& test : to_ts) {
    EXPECT_THAT(basic::ToTimespec(test.d), TimespecMatcher(test.ts));
  }
  const struct {
    timespec ts;
    basic::Duration d;
  } from_ts[] = {
      {{1, 1}, basic::Seconds(1) + basic::Nanoseconds(1)},
      {{1, 0}, basic::Seconds(1) + basic::Nanoseconds(0)},
      {{0, 0}, basic::Seconds(0) + basic::Nanoseconds(0)},
      {{0, -1}, basic::Seconds(0) - basic::Nanoseconds(1)},
      {{-1, 999999999}, basic::Seconds(0) - basic::Nanoseconds(1)},
      {{-1, 1}, basic::Seconds(-1) + basic::Nanoseconds(1)},
      {{-1, 0}, basic::Seconds(-1) + basic::Nanoseconds(0)},
      {{-1, -1}, basic::Seconds(-1) - basic::Nanoseconds(1)},
      {{-2, 999999999}, basic::Seconds(-1) - basic::Nanoseconds(1)},
  };
  for (const auto& test : from_ts) {
    EXPECT_EQ(test.d, basic::DurationFromTimespec(test.ts));
  }

  // Tests ToTimeval()/DurationFromTimeval() (same as timespec above)
  const struct {
    basic::Duration d;
    timeval tv;
  } to_tv[] = {
      {basic::Seconds(1) + basic::Microseconds(1), {1, 1}},
      {basic::Seconds(1) + basic::Microseconds(1) / 2, {1, 0}},
      {basic::Seconds(1) + basic::Microseconds(0), {1, 0}},
      {basic::Seconds(0) + basic::Microseconds(0), {0, 0}},
      {basic::Seconds(0) - basic::Microseconds(1) / 2, {0, 0}},
      {basic::Seconds(0) - basic::Microseconds(1), {-1, 999999}},
      {basic::Seconds(-1) + basic::Microseconds(1), {-1, 1}},
      {basic::Seconds(-1) + basic::Microseconds(1) / 2, {-1, 1}},
      {basic::Seconds(-1) + basic::Microseconds(0), {-1, 0}},
      {basic::Seconds(-1) - basic::Microseconds(1) / 2, {-1, 0}},
  };
  for (const auto& test : to_tv) {
    EXPECT_THAT(basic::ToTimeval(test.d), TimevalMatcher(test.tv));
  }
  const struct {
    timeval tv;
    basic::Duration d;
  } from_tv[] = {
      {{1, 1}, basic::Seconds(1) + basic::Microseconds(1)},
      {{1, 0}, basic::Seconds(1) + basic::Microseconds(0)},
      {{0, 0}, basic::Seconds(0) + basic::Microseconds(0)},
      {{0, -1}, basic::Seconds(0) - basic::Microseconds(1)},
      {{-1, 999999}, basic::Seconds(0) - basic::Microseconds(1)},
      {{-1, 1}, basic::Seconds(-1) + basic::Microseconds(1)},
      {{-1, 0}, basic::Seconds(-1) + basic::Microseconds(0)},
      {{-1, -1}, basic::Seconds(-1) - basic::Microseconds(1)},
      {{-2, 999999}, basic::Seconds(-1) - basic::Microseconds(1)},
  };
  for (const auto& test : from_tv) {
    EXPECT_EQ(test.d, basic::DurationFromTimeval(test.tv));
  }
}

TEST(Duration, SmallConversions) {
  // Special tests for conversions of small durations.

  EXPECT_EQ(basic::ZeroDuration(), basic::Seconds(0));
  // TODO(bww): Is the next one OK?
  EXPECT_EQ(basic::ZeroDuration(), basic::Seconds(0.124999999e-9));
  EXPECT_EQ(basic::Nanoseconds(1) / 4, basic::Seconds(0.125e-9));
  EXPECT_EQ(basic::Nanoseconds(1) / 4, basic::Seconds(0.250e-9));
  EXPECT_EQ(basic::Nanoseconds(1) / 2, basic::Seconds(0.375e-9));
  EXPECT_EQ(basic::Nanoseconds(1) / 2, basic::Seconds(0.500e-9));
  EXPECT_EQ(basic::Nanoseconds(3) / 4, basic::Seconds(0.625e-9));
  EXPECT_EQ(basic::Nanoseconds(3) / 4, basic::Seconds(0.750e-9));
  EXPECT_EQ(basic::Nanoseconds(1), basic::Seconds(0.875e-9));
  EXPECT_EQ(basic::Nanoseconds(1), basic::Seconds(1.000e-9));

  EXPECT_EQ(basic::ZeroDuration(), basic::Seconds(-0.124999999e-9));
  EXPECT_EQ(-basic::Nanoseconds(1) / 4, basic::Seconds(-0.125e-9));
  EXPECT_EQ(-basic::Nanoseconds(1) / 4, basic::Seconds(-0.250e-9));
  EXPECT_EQ(-basic::Nanoseconds(1) / 2, basic::Seconds(-0.375e-9));
  EXPECT_EQ(-basic::Nanoseconds(1) / 2, basic::Seconds(-0.500e-9));
  EXPECT_EQ(-basic::Nanoseconds(3) / 4, basic::Seconds(-0.625e-9));
  EXPECT_EQ(-basic::Nanoseconds(3) / 4, basic::Seconds(-0.750e-9));
  EXPECT_EQ(-basic::Nanoseconds(1), basic::Seconds(-0.875e-9));
  EXPECT_EQ(-basic::Nanoseconds(1), basic::Seconds(-1.000e-9));

  timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 0;
  EXPECT_THAT(ToTimespec(basic::Nanoseconds(0)), TimespecMatcher(ts));
  // TODO(bww): Are the next three OK?
  EXPECT_THAT(ToTimespec(basic::Nanoseconds(1) / 4), TimespecMatcher(ts));
  EXPECT_THAT(ToTimespec(basic::Nanoseconds(2) / 4), TimespecMatcher(ts));
  EXPECT_THAT(ToTimespec(basic::Nanoseconds(3) / 4), TimespecMatcher(ts));
  ts.tv_nsec = 1;
  EXPECT_THAT(ToTimespec(basic::Nanoseconds(4) / 4), TimespecMatcher(ts));
  EXPECT_THAT(ToTimespec(basic::Nanoseconds(5) / 4), TimespecMatcher(ts));
  EXPECT_THAT(ToTimespec(basic::Nanoseconds(6) / 4), TimespecMatcher(ts));
  EXPECT_THAT(ToTimespec(basic::Nanoseconds(7) / 4), TimespecMatcher(ts));
  ts.tv_nsec = 2;
  EXPECT_THAT(ToTimespec(basic::Nanoseconds(8) / 4), TimespecMatcher(ts));

  timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  EXPECT_THAT(ToTimeval(basic::Nanoseconds(0)), TimevalMatcher(tv));
  // TODO(bww): Is the next one OK?
  EXPECT_THAT(ToTimeval(basic::Nanoseconds(999)), TimevalMatcher(tv));
  tv.tv_usec = 1;
  EXPECT_THAT(ToTimeval(basic::Nanoseconds(1000)), TimevalMatcher(tv));
  EXPECT_THAT(ToTimeval(basic::Nanoseconds(1999)), TimevalMatcher(tv));
  tv.tv_usec = 2;
  EXPECT_THAT(ToTimeval(basic::Nanoseconds(2000)), TimevalMatcher(tv));
}

void VerifySameAsMul(double time_as_seconds, int* const misses) {
  auto direct_seconds = basic::Seconds(time_as_seconds);
  auto mul_by_one_second = time_as_seconds * basic::Seconds(1);
  if (direct_seconds != mul_by_one_second) {
    if (*misses > 10) return;
    ASSERT_LE(++(*misses), 10) << "Too many errors, not reporting more.";
    EXPECT_EQ(direct_seconds, mul_by_one_second)
        << "given double time_as_seconds = " << std::setprecision(17)
        << time_as_seconds;
  }
}

// For a variety of interesting durations, we find the exact point
// where one double converts to that duration, and the very next double
// converts to the next duration.  For both of those points, verify that
// Seconds(point) returns the same duration as point * Seconds(1.0)
TEST(Duration, ToDoubleSecondsCheckEdgeCases) {
  constexpr uint32_t kTicksPerSecond = basic::time_internal::kTicksPerSecond;
  constexpr auto duration_tick = basic::time_internal::MakeDuration(0, 1u);
  int misses = 0;
  for (int64_t seconds = 0; seconds < 99; ++seconds) {
    uint32_t tick_vals[] = {0, +999, +999999, +999999999, kTicksPerSecond - 1,
                            0, 1000, 1000000, 1000000000, kTicksPerSecond,
                            1, 1001, 1000001, 1000000001, kTicksPerSecond + 1,
                            2, 1002, 1000002, 1000000002, kTicksPerSecond + 2,
                            3, 1003, 1000003, 1000000003, kTicksPerSecond + 3,
                            4, 1004, 1000004, 1000000004, kTicksPerSecond + 4,
                            5, 6,    7,       8,          9};
    for (uint32_t ticks : tick_vals) {
      basic::Duration s_plus_t = basic::Seconds(seconds) + ticks * duration_tick;
      for (basic::Duration d : {s_plus_t, -s_plus_t}) {
        basic::Duration after_d = d + duration_tick;
        EXPECT_NE(d, after_d);
        EXPECT_EQ(after_d - d, duration_tick);

        double low_edge = ToDoubleSeconds(d);
        EXPECT_EQ(d, basic::Seconds(low_edge));

        double high_edge = ToDoubleSeconds(after_d);
        EXPECT_EQ(after_d, basic::Seconds(high_edge));

        for (;;) {
          double midpoint = low_edge + (high_edge - low_edge) / 2;
          if (midpoint == low_edge || midpoint == high_edge) break;
          basic::Duration mid_duration = basic::Seconds(midpoint);
          if (mid_duration == d) {
            low_edge = midpoint;
          } else {
            EXPECT_EQ(mid_duration, after_d);
            high_edge = midpoint;
          }
        }
        // Now low_edge is the highest double that converts to Duration d,
        // and high_edge is the lowest double that converts to Duration after_d.
        VerifySameAsMul(low_edge, &misses);
        VerifySameAsMul(high_edge, &misses);
      }
    }
  }
}

TEST(Duration, ToDoubleSecondsCheckRandom) {
  std::random_device rd;
  std::seed_seq seed({rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()});
  std::mt19937_64 gen(seed);
  // We want doubles distributed from 1/8ns up to 2^63, where
  // as many values are tested from 1ns to 2ns as from 1sec to 2sec,
  // so even distribute along a log-scale of those values, and
  // exponentiate before using them.  (9.223377e+18 is just slightly
  // out of bounds for basic::Duration.)
  std::uniform_real_distribution<double> uniform(std::log(0.125e-9),
                                                 std::log(9.223377e+18));
  int misses = 0;
  for (int i = 0; i < 1000000; ++i) {
    double d = std::exp(uniform(gen));
    VerifySameAsMul(d, &misses);
    VerifySameAsMul(-d, &misses);
  }
}

TEST(Duration, ConversionSaturation) {
  basic::Duration d;

  const auto max_timeval_sec =
      std::numeric_limits<decltype(timeval::tv_sec)>::max();
  const auto min_timeval_sec =
      std::numeric_limits<decltype(timeval::tv_sec)>::min();
  timeval tv;
  tv.tv_sec = max_timeval_sec;
  tv.tv_usec = 999998;
  d = basic::DurationFromTimeval(tv);
  tv = ToTimeval(d);
  EXPECT_EQ(max_timeval_sec, tv.tv_sec);
  EXPECT_EQ(999998, tv.tv_usec);
  d += basic::Microseconds(1);
  tv = ToTimeval(d);
  EXPECT_EQ(max_timeval_sec, tv.tv_sec);
  EXPECT_EQ(999999, tv.tv_usec);
  d += basic::Microseconds(1);  // no effect
  tv = ToTimeval(d);
  EXPECT_EQ(max_timeval_sec, tv.tv_sec);
  EXPECT_EQ(999999, tv.tv_usec);

  tv.tv_sec = min_timeval_sec;
  tv.tv_usec = 1;
  d = basic::DurationFromTimeval(tv);
  tv = ToTimeval(d);
  EXPECT_EQ(min_timeval_sec, tv.tv_sec);
  EXPECT_EQ(1, tv.tv_usec);
  d -= basic::Microseconds(1);
  tv = ToTimeval(d);
  EXPECT_EQ(min_timeval_sec, tv.tv_sec);
  EXPECT_EQ(0, tv.tv_usec);
  d -= basic::Microseconds(1);  // no effect
  tv = ToTimeval(d);
  EXPECT_EQ(min_timeval_sec, tv.tv_sec);
  EXPECT_EQ(0, tv.tv_usec);

  const auto max_timespec_sec =
      std::numeric_limits<decltype(timespec::tv_sec)>::max();
  const auto min_timespec_sec =
      std::numeric_limits<decltype(timespec::tv_sec)>::min();
  timespec ts;
  ts.tv_sec = max_timespec_sec;
  ts.tv_nsec = 999999998;
  d = basic::DurationFromTimespec(ts);
  ts = basic::ToTimespec(d);
  EXPECT_EQ(max_timespec_sec, ts.tv_sec);
  EXPECT_EQ(999999998, ts.tv_nsec);
  d += basic::Nanoseconds(1);
  ts = basic::ToTimespec(d);
  EXPECT_EQ(max_timespec_sec, ts.tv_sec);
  EXPECT_EQ(999999999, ts.tv_nsec);
  d += basic::Nanoseconds(1);  // no effect
  ts = basic::ToTimespec(d);
  EXPECT_EQ(max_timespec_sec, ts.tv_sec);
  EXPECT_EQ(999999999, ts.tv_nsec);

  ts.tv_sec = min_timespec_sec;
  ts.tv_nsec = 1;
  d = basic::DurationFromTimespec(ts);
  ts = basic::ToTimespec(d);
  EXPECT_EQ(min_timespec_sec, ts.tv_sec);
  EXPECT_EQ(1, ts.tv_nsec);
  d -= basic::Nanoseconds(1);
  ts = basic::ToTimespec(d);
  EXPECT_EQ(min_timespec_sec, ts.tv_sec);
  EXPECT_EQ(0, ts.tv_nsec);
  d -= basic::Nanoseconds(1);  // no effect
  ts = basic::ToTimespec(d);
  EXPECT_EQ(min_timespec_sec, ts.tv_sec);
  EXPECT_EQ(0, ts.tv_nsec);
}

TEST(Duration, FormatDuration) {
  // Example from Go's docs.
  EXPECT_EQ("72h3m0.5s",
            basic::FormatDuration(basic::Hours(72) + basic::Minutes(3) +
                                 basic::Milliseconds(500)));
  // Go's largest time: 2540400h10m10.000000000s
  EXPECT_EQ("2540400h10m10s",
            basic::FormatDuration(basic::Hours(2540400) + basic::Minutes(10) +
                                 basic::Seconds(10)));

  EXPECT_EQ("0", basic::FormatDuration(basic::ZeroDuration()));
  EXPECT_EQ("0", basic::FormatDuration(basic::Seconds(0)));
  EXPECT_EQ("0", basic::FormatDuration(basic::Nanoseconds(0)));

  EXPECT_EQ("1ns", basic::FormatDuration(basic::Nanoseconds(1)));
  EXPECT_EQ("1us", basic::FormatDuration(basic::Microseconds(1)));
  EXPECT_EQ("1ms", basic::FormatDuration(basic::Milliseconds(1)));
  EXPECT_EQ("1s", basic::FormatDuration(basic::Seconds(1)));
  EXPECT_EQ("1m", basic::FormatDuration(basic::Minutes(1)));
  EXPECT_EQ("1h", basic::FormatDuration(basic::Hours(1)));

  EXPECT_EQ("1h1m", basic::FormatDuration(basic::Hours(1) + basic::Minutes(1)));
  EXPECT_EQ("1h1s", basic::FormatDuration(basic::Hours(1) + basic::Seconds(1)));
  EXPECT_EQ("1m1s", basic::FormatDuration(basic::Minutes(1) + basic::Seconds(1)));

  EXPECT_EQ("1h0.25s",
            basic::FormatDuration(basic::Hours(1) + basic::Milliseconds(250)));
  EXPECT_EQ("1m0.25s",
            basic::FormatDuration(basic::Minutes(1) + basic::Milliseconds(250)));
  EXPECT_EQ("1h1m0.25s",
            basic::FormatDuration(basic::Hours(1) + basic::Minutes(1) +
                                 basic::Milliseconds(250)));
  EXPECT_EQ("1h0.0005s",
            basic::FormatDuration(basic::Hours(1) + basic::Microseconds(500)));
  EXPECT_EQ("1h0.0000005s",
            basic::FormatDuration(basic::Hours(1) + basic::Nanoseconds(500)));

  // Subsecond special case.
  EXPECT_EQ("1.5ns", basic::FormatDuration(basic::Nanoseconds(1) +
                                          basic::Nanoseconds(1) / 2));
  EXPECT_EQ("1.25ns", basic::FormatDuration(basic::Nanoseconds(1) +
                                           basic::Nanoseconds(1) / 4));
  EXPECT_EQ("1ns", basic::FormatDuration(basic::Nanoseconds(1) +
                                        basic::Nanoseconds(1) / 9));
  EXPECT_EQ("1.2us", basic::FormatDuration(basic::Microseconds(1) +
                                          basic::Nanoseconds(200)));
  EXPECT_EQ("1.2ms", basic::FormatDuration(basic::Milliseconds(1) +
                                          basic::Microseconds(200)));
  EXPECT_EQ("1.0002ms", basic::FormatDuration(basic::Milliseconds(1) +
                                             basic::Nanoseconds(200)));
  EXPECT_EQ("1.00001ms", basic::FormatDuration(basic::Milliseconds(1) +
                                              basic::Nanoseconds(10)));
  EXPECT_EQ("1.000001ms",
            basic::FormatDuration(basic::Milliseconds(1) + basic::Nanoseconds(1)));

  // Negative durations.
  EXPECT_EQ("-1ns", basic::FormatDuration(basic::Nanoseconds(-1)));
  EXPECT_EQ("-1us", basic::FormatDuration(basic::Microseconds(-1)));
  EXPECT_EQ("-1ms", basic::FormatDuration(basic::Milliseconds(-1)));
  EXPECT_EQ("-1s", basic::FormatDuration(basic::Seconds(-1)));
  EXPECT_EQ("-1m", basic::FormatDuration(basic::Minutes(-1)));
  EXPECT_EQ("-1h", basic::FormatDuration(basic::Hours(-1)));

  EXPECT_EQ("-1h1m",
            basic::FormatDuration(-(basic::Hours(1) + basic::Minutes(1))));
  EXPECT_EQ("-1h1s",
            basic::FormatDuration(-(basic::Hours(1) + basic::Seconds(1))));
  EXPECT_EQ("-1m1s",
            basic::FormatDuration(-(basic::Minutes(1) + basic::Seconds(1))));

  EXPECT_EQ("-1ns", basic::FormatDuration(basic::Nanoseconds(-1)));
  EXPECT_EQ("-1.2us", basic::FormatDuration(
                          -(basic::Microseconds(1) + basic::Nanoseconds(200))));
  EXPECT_EQ("-1.2ms", basic::FormatDuration(
                          -(basic::Milliseconds(1) + basic::Microseconds(200))));
  EXPECT_EQ("-1.0002ms", basic::FormatDuration(-(basic::Milliseconds(1) +
                                                basic::Nanoseconds(200))));
  EXPECT_EQ("-1.00001ms", basic::FormatDuration(-(basic::Milliseconds(1) +
                                                 basic::Nanoseconds(10))));
  EXPECT_EQ("-1.000001ms", basic::FormatDuration(-(basic::Milliseconds(1) +
                                                  basic::Nanoseconds(1))));

  //
  // Interesting corner cases.
  //

  const basic::Duration qns = basic::Nanoseconds(1) / 4;
  const basic::Duration max_dur =
      basic::Seconds(kint64max) + (basic::Seconds(1) - qns);
  const basic::Duration min_dur = basic::Seconds(kint64min);

  EXPECT_EQ("0.25ns", basic::FormatDuration(qns));
  EXPECT_EQ("-0.25ns", basic::FormatDuration(-qns));
  EXPECT_EQ("2562047788015215h30m7.99999999975s",
            basic::FormatDuration(max_dur));
  EXPECT_EQ("-2562047788015215h30m8s", basic::FormatDuration(min_dur));

  // Tests printing full precision from units that print using FDivDuration
  EXPECT_EQ("55.00000000025s", basic::FormatDuration(basic::Seconds(55) + qns));
  EXPECT_EQ("55.00000025ms",
            basic::FormatDuration(basic::Milliseconds(55) + qns));
  EXPECT_EQ("55.00025us", basic::FormatDuration(basic::Microseconds(55) + qns));
  EXPECT_EQ("55.25ns", basic::FormatDuration(basic::Nanoseconds(55) + qns));

  // Formatting infinity
  EXPECT_EQ("inf", basic::FormatDuration(basic::InfiniteDuration()));
  EXPECT_EQ("-inf", basic::FormatDuration(-basic::InfiniteDuration()));

  // Formatting approximately +/- 100 billion years
  const basic::Duration huge_range = ApproxYears(100000000000);
  EXPECT_EQ("876000000000000h", basic::FormatDuration(huge_range));
  EXPECT_EQ("-876000000000000h", basic::FormatDuration(-huge_range));

  EXPECT_EQ("876000000000000h0.999999999s",
            basic::FormatDuration(huge_range +
                                 (basic::Seconds(1) - basic::Nanoseconds(1))));
  EXPECT_EQ("876000000000000h0.9999999995s",
            basic::FormatDuration(
                huge_range + (basic::Seconds(1) - basic::Nanoseconds(1) / 2)));
  EXPECT_EQ("876000000000000h0.99999999975s",
            basic::FormatDuration(
                huge_range + (basic::Seconds(1) - basic::Nanoseconds(1) / 4)));

  EXPECT_EQ("-876000000000000h0.999999999s",
            basic::FormatDuration(-huge_range -
                                 (basic::Seconds(1) - basic::Nanoseconds(1))));
  EXPECT_EQ("-876000000000000h0.9999999995s",
            basic::FormatDuration(
                -huge_range - (basic::Seconds(1) - basic::Nanoseconds(1) / 2)));
  EXPECT_EQ("-876000000000000h0.99999999975s",
            basic::FormatDuration(
                -huge_range - (basic::Seconds(1) - basic::Nanoseconds(1) / 4)));
}

TEST(Duration, ParseDuration) {
  basic::Duration d;

  // No specified unit. Should only work for zero and infinity.
  EXPECT_TRUE(basic::ParseDuration("0", &d));
  EXPECT_EQ(basic::ZeroDuration(), d);
  EXPECT_TRUE(basic::ParseDuration("+0", &d));
  EXPECT_EQ(basic::ZeroDuration(), d);
  EXPECT_TRUE(basic::ParseDuration("-0", &d));
  EXPECT_EQ(basic::ZeroDuration(), d);

  EXPECT_TRUE(basic::ParseDuration("inf", &d));
  EXPECT_EQ(basic::InfiniteDuration(), d);
  EXPECT_TRUE(basic::ParseDuration("+inf", &d));
  EXPECT_EQ(basic::InfiniteDuration(), d);
  EXPECT_TRUE(basic::ParseDuration("-inf", &d));
  EXPECT_EQ(-basic::InfiniteDuration(), d);
  EXPECT_FALSE(basic::ParseDuration("infBlah", &d));

  // Illegal input forms.
  EXPECT_FALSE(basic::ParseDuration("", &d));
  EXPECT_FALSE(basic::ParseDuration("0.0", &d));
  EXPECT_FALSE(basic::ParseDuration(".0", &d));
  EXPECT_FALSE(basic::ParseDuration(".", &d));
  EXPECT_FALSE(basic::ParseDuration("01", &d));
  EXPECT_FALSE(basic::ParseDuration("1", &d));
  EXPECT_FALSE(basic::ParseDuration("-1", &d));
  EXPECT_FALSE(basic::ParseDuration("2", &d));
  EXPECT_FALSE(basic::ParseDuration("2 s", &d));
  EXPECT_FALSE(basic::ParseDuration(".s", &d));
  EXPECT_FALSE(basic::ParseDuration("-.s", &d));
  EXPECT_FALSE(basic::ParseDuration("s", &d));
  EXPECT_FALSE(basic::ParseDuration(" 2s", &d));
  EXPECT_FALSE(basic::ParseDuration("2s ", &d));
  EXPECT_FALSE(basic::ParseDuration(" 2s ", &d));
  EXPECT_FALSE(basic::ParseDuration("2mt", &d));
  EXPECT_FALSE(basic::ParseDuration("1e3s", &d));

  // One unit type.
  EXPECT_TRUE(basic::ParseDuration("1ns", &d));
  EXPECT_EQ(basic::Nanoseconds(1), d);
  EXPECT_TRUE(basic::ParseDuration("1us", &d));
  EXPECT_EQ(basic::Microseconds(1), d);
  EXPECT_TRUE(basic::ParseDuration("1ms", &d));
  EXPECT_EQ(basic::Milliseconds(1), d);
  EXPECT_TRUE(basic::ParseDuration("1s", &d));
  EXPECT_EQ(basic::Seconds(1), d);
  EXPECT_TRUE(basic::ParseDuration("2m", &d));
  EXPECT_EQ(basic::Minutes(2), d);
  EXPECT_TRUE(basic::ParseDuration("2h", &d));
  EXPECT_EQ(basic::Hours(2), d);

  // Huge counts of a unit.
  EXPECT_TRUE(basic::ParseDuration("9223372036854775807us", &d));
  EXPECT_EQ(basic::Microseconds(9223372036854775807), d);
  EXPECT_TRUE(basic::ParseDuration("-9223372036854775807us", &d));
  EXPECT_EQ(basic::Microseconds(-9223372036854775807), d);

  // Multiple units.
  EXPECT_TRUE(basic::ParseDuration("2h3m4s", &d));
  EXPECT_EQ(basic::Hours(2) + basic::Minutes(3) + basic::Seconds(4), d);
  EXPECT_TRUE(basic::ParseDuration("3m4s5us", &d));
  EXPECT_EQ(basic::Minutes(3) + basic::Seconds(4) + basic::Microseconds(5), d);
  EXPECT_TRUE(basic::ParseDuration("2h3m4s5ms6us7ns", &d));
  EXPECT_EQ(basic::Hours(2) + basic::Minutes(3) + basic::Seconds(4) +
                basic::Milliseconds(5) + basic::Microseconds(6) +
                basic::Nanoseconds(7),
            d);

  // Multiple units out of order.
  EXPECT_TRUE(basic::ParseDuration("2us3m4s5h", &d));
  EXPECT_EQ(basic::Hours(5) + basic::Minutes(3) + basic::Seconds(4) +
                basic::Microseconds(2),
            d);

  // Fractional values of units.
  EXPECT_TRUE(basic::ParseDuration("1.5ns", &d));
  EXPECT_EQ(1.5 * basic::Nanoseconds(1), d);
  EXPECT_TRUE(basic::ParseDuration("1.5us", &d));
  EXPECT_EQ(1.5 * basic::Microseconds(1), d);
  EXPECT_TRUE(basic::ParseDuration("1.5ms", &d));
  EXPECT_EQ(1.5 * basic::Milliseconds(1), d);
  EXPECT_TRUE(basic::ParseDuration("1.5s", &d));
  EXPECT_EQ(1.5 * basic::Seconds(1), d);
  EXPECT_TRUE(basic::ParseDuration("1.5m", &d));
  EXPECT_EQ(1.5 * basic::Minutes(1), d);
  EXPECT_TRUE(basic::ParseDuration("1.5h", &d));
  EXPECT_EQ(1.5 * basic::Hours(1), d);

  // Huge fractional counts of a unit.
  EXPECT_TRUE(basic::ParseDuration("0.4294967295s", &d));
  EXPECT_EQ(basic::Nanoseconds(429496729) + basic::Nanoseconds(1) / 2, d);
  EXPECT_TRUE(basic::ParseDuration("0.429496729501234567890123456789s", &d));
  EXPECT_EQ(basic::Nanoseconds(429496729) + basic::Nanoseconds(1) / 2, d);

  // Negative durations.
  EXPECT_TRUE(basic::ParseDuration("-1s", &d));
  EXPECT_EQ(basic::Seconds(-1), d);
  EXPECT_TRUE(basic::ParseDuration("-1m", &d));
  EXPECT_EQ(basic::Minutes(-1), d);
  EXPECT_TRUE(basic::ParseDuration("-1h", &d));
  EXPECT_EQ(basic::Hours(-1), d);

  EXPECT_TRUE(basic::ParseDuration("-1h2s", &d));
  EXPECT_EQ(-(basic::Hours(1) + basic::Seconds(2)), d);
  EXPECT_FALSE(basic::ParseDuration("1h-2s", &d));
  EXPECT_FALSE(basic::ParseDuration("-1h-2s", &d));
  EXPECT_FALSE(basic::ParseDuration("-1h -2s", &d));
}

TEST(Duration, FormatParseRoundTrip) {
#define TEST_PARSE_ROUNDTRIP(d)                \
  do {                                         \
    std::string s = basic::FormatDuration(d);   \
    basic::Duration dur;                        \
    EXPECT_TRUE(basic::ParseDuration(s, &dur)); \
    EXPECT_EQ(d, dur);                         \
  } while (0)

  TEST_PARSE_ROUNDTRIP(basic::Nanoseconds(1));
  TEST_PARSE_ROUNDTRIP(basic::Microseconds(1));
  TEST_PARSE_ROUNDTRIP(basic::Milliseconds(1));
  TEST_PARSE_ROUNDTRIP(basic::Seconds(1));
  TEST_PARSE_ROUNDTRIP(basic::Minutes(1));
  TEST_PARSE_ROUNDTRIP(basic::Hours(1));
  TEST_PARSE_ROUNDTRIP(basic::Hours(1) + basic::Nanoseconds(2));

  TEST_PARSE_ROUNDTRIP(basic::Nanoseconds(-1));
  TEST_PARSE_ROUNDTRIP(basic::Microseconds(-1));
  TEST_PARSE_ROUNDTRIP(basic::Milliseconds(-1));
  TEST_PARSE_ROUNDTRIP(basic::Seconds(-1));
  TEST_PARSE_ROUNDTRIP(basic::Minutes(-1));
  TEST_PARSE_ROUNDTRIP(basic::Hours(-1));

  TEST_PARSE_ROUNDTRIP(basic::Hours(-1) + basic::Nanoseconds(2));
  TEST_PARSE_ROUNDTRIP(basic::Hours(1) + basic::Nanoseconds(-2));
  TEST_PARSE_ROUNDTRIP(basic::Hours(-1) + basic::Nanoseconds(-2));

  TEST_PARSE_ROUNDTRIP(basic::Nanoseconds(1) +
                       basic::Nanoseconds(1) / 4);  // 1.25ns

  const basic::Duration huge_range = ApproxYears(100000000000);
  TEST_PARSE_ROUNDTRIP(huge_range);
  TEST_PARSE_ROUNDTRIP(huge_range + (basic::Seconds(1) - basic::Nanoseconds(1)));

#undef TEST_PARSE_ROUNDTRIP
}

}  // namespace
