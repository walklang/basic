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

#include "basic/time/time.h"

#if defined(_MSC_VER)
#include <winsock2.h>  // for timeval
#endif

#include <chrono>  // NOLINT(build/c++11)
#include <cstring>
#include <ctime>
#include <iomanip>
#include <limits>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "basic/time/clock.h"
#include "basic/time/internal/test_util.h"

namespace {

#if defined(GTEST_USES_SIMPLE_RE) && GTEST_USES_SIMPLE_RE
const char kZoneAbbrRE[] = ".*";  // just punt
#else
const char kZoneAbbrRE[] = "[A-Za-z]{3,4}|[-+][0-9]{2}([0-9]{2})?";
#endif

// This helper is a macro so that failed expectations show up with the
// correct line numbers.
#define EXPECT_CIVIL_INFO(ci, y, m, d, h, min, s, off, isdst)      \
  do {                                                             \
    EXPECT_EQ(y, ci.cs.year());                                    \
    EXPECT_EQ(m, ci.cs.month());                                   \
    EXPECT_EQ(d, ci.cs.day());                                     \
    EXPECT_EQ(h, ci.cs.hour());                                    \
    EXPECT_EQ(min, ci.cs.minute());                                \
    EXPECT_EQ(s, ci.cs.second());                                  \
    EXPECT_EQ(off, ci.offset);                                     \
    EXPECT_EQ(isdst, ci.is_dst);                                   \
    EXPECT_THAT(ci.zone_abbr, testing::MatchesRegex(kZoneAbbrRE)); \
  } while (0)

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

TEST(Time, ConstExpr) {
  constexpr basic::Time t0 = basic::UnixEpoch();
  static_assert(t0 == basic::Time(), "UnixEpoch");
  constexpr basic::Time t1 = basic::InfiniteFuture();
  static_assert(t1 != basic::Time(), "InfiniteFuture");
  constexpr basic::Time t2 = basic::InfinitePast();
  static_assert(t2 != basic::Time(), "InfinitePast");
  constexpr basic::Time t3 = basic::FromUnixNanos(0);
  static_assert(t3 == basic::Time(), "FromUnixNanos");
  constexpr basic::Time t4 = basic::FromUnixMicros(0);
  static_assert(t4 == basic::Time(), "FromUnixMicros");
  constexpr basic::Time t5 = basic::FromUnixMillis(0);
  static_assert(t5 == basic::Time(), "FromUnixMillis");
  constexpr basic::Time t6 = basic::FromUnixSeconds(0);
  static_assert(t6 == basic::Time(), "FromUnixSeconds");
  constexpr basic::Time t7 = basic::FromTimeT(0);
  static_assert(t7 == basic::Time(), "FromTimeT");
}

TEST(Time, ValueSemantics) {
  basic::Time a;      // Default construction
  basic::Time b = a;  // Copy construction
  EXPECT_EQ(a, b);
  basic::Time c(a);  // Copy construction (again)
  EXPECT_EQ(a, b);
  EXPECT_EQ(a, c);
  EXPECT_EQ(b, c);
  b = c;       // Assignment
  EXPECT_EQ(a, b);
  EXPECT_EQ(a, c);
  EXPECT_EQ(b, c);
}

TEST(Time, UnixEpoch) {
  const auto ci = basic::UTCTimeZone().At(basic::UnixEpoch());
  EXPECT_EQ(basic::CivilSecond(1970, 1, 1, 0, 0, 0), ci.cs);
  EXPECT_EQ(basic::ZeroDuration(), ci.subsecond);
  EXPECT_EQ(basic::Weekday::thursday, basic::GetWeekday(ci.cs));
}

TEST(Time, Breakdown) {
  basic::TimeZone tz = basic::time_internal::LoadTimeZone("America/New_York");
  basic::Time t = basic::UnixEpoch();

  // The Unix epoch as seen in NYC.
  auto ci = tz.At(t);
  EXPECT_CIVIL_INFO(ci, 1969, 12, 31, 19, 0, 0, -18000, false);
  EXPECT_EQ(basic::ZeroDuration(), ci.subsecond);
  EXPECT_EQ(basic::Weekday::wednesday, basic::GetWeekday(ci.cs));

  // Just before the epoch.
  t -= basic::Nanoseconds(1);
  ci = tz.At(t);
  EXPECT_CIVIL_INFO(ci, 1969, 12, 31, 18, 59, 59, -18000, false);
  EXPECT_EQ(basic::Nanoseconds(999999999), ci.subsecond);
  EXPECT_EQ(basic::Weekday::wednesday, basic::GetWeekday(ci.cs));

  // Some time later.
  t += basic::Hours(24) * 2735;
  t += basic::Hours(18) + basic::Minutes(30) + basic::Seconds(15) +
       basic::Nanoseconds(9);
  ci = tz.At(t);
  EXPECT_CIVIL_INFO(ci, 1977, 6, 28, 14, 30, 15, -14400, true);
  EXPECT_EQ(8, ci.subsecond / basic::Nanoseconds(1));
  EXPECT_EQ(basic::Weekday::tuesday, basic::GetWeekday(ci.cs));
}

TEST(Time, AdditiveOperators) {
  const basic::Duration d = basic::Nanoseconds(1);
  const basic::Time t0;
  const basic::Time t1 = t0 + d;

  EXPECT_EQ(d, t1 - t0);
  EXPECT_EQ(-d, t0 - t1);
  EXPECT_EQ(t0, t1 - d);

  basic::Time t(t0);
  EXPECT_EQ(t0, t);
  t += d;
  EXPECT_EQ(t0 + d, t);
  EXPECT_EQ(d, t - t0);
  t -= d;
  EXPECT_EQ(t0, t);

  // Tests overflow between subseconds and seconds.
  t = basic::UnixEpoch();
  t += basic::Milliseconds(500);
  EXPECT_EQ(basic::UnixEpoch() + basic::Milliseconds(500), t);
  t += basic::Milliseconds(600);
  EXPECT_EQ(basic::UnixEpoch() + basic::Milliseconds(1100), t);
  t -= basic::Milliseconds(600);
  EXPECT_EQ(basic::UnixEpoch() + basic::Milliseconds(500), t);
  t -= basic::Milliseconds(500);
  EXPECT_EQ(basic::UnixEpoch(), t);
}

TEST(Time, RelationalOperators) {
  constexpr basic::Time t1 = basic::FromUnixNanos(0);
  constexpr basic::Time t2 = basic::FromUnixNanos(1);
  constexpr basic::Time t3 = basic::FromUnixNanos(2);

  static_assert(basic::Time() == t1, "");
  static_assert(t1 == t1, "");
  static_assert(t2 == t2, "");
  static_assert(t3 == t3, "");

  static_assert(t1 < t2, "");
  static_assert(t2 < t3, "");
  static_assert(t1 < t3, "");

  static_assert(t1 <= t1, "");
  static_assert(t1 <= t2, "");
  static_assert(t2 <= t2, "");
  static_assert(t2 <= t3, "");
  static_assert(t3 <= t3, "");
  static_assert(t1 <= t3, "");

  static_assert(t2 > t1, "");
  static_assert(t3 > t2, "");
  static_assert(t3 > t1, "");

  static_assert(t2 >= t2, "");
  static_assert(t2 >= t1, "");
  static_assert(t3 >= t3, "");
  static_assert(t3 >= t2, "");
  static_assert(t1 >= t1, "");
  static_assert(t3 >= t1, "");
}

TEST(Time, Infinity) {
  constexpr basic::Time ifuture = basic::InfiniteFuture();
  constexpr basic::Time ipast = basic::InfinitePast();

  static_assert(ifuture == ifuture, "");
  static_assert(ipast == ipast, "");
  static_assert(ipast < ifuture, "");
  static_assert(ifuture > ipast, "");

  // Arithmetic saturates
  EXPECT_EQ(ifuture, ifuture + basic::Seconds(1));
  EXPECT_EQ(ifuture, ifuture - basic::Seconds(1));
  EXPECT_EQ(ipast, ipast + basic::Seconds(1));
  EXPECT_EQ(ipast, ipast - basic::Seconds(1));

  EXPECT_EQ(basic::InfiniteDuration(), ifuture - ifuture);
  EXPECT_EQ(basic::InfiniteDuration(), ifuture - ipast);
  EXPECT_EQ(-basic::InfiniteDuration(), ipast - ifuture);
  EXPECT_EQ(-basic::InfiniteDuration(), ipast - ipast);

  constexpr basic::Time t = basic::UnixEpoch();  // Any finite time.
  static_assert(t < ifuture, "");
  static_assert(t > ipast, "");
}

TEST(Time, FloorConversion) {
#define TEST_FLOOR_CONVERSION(TO, FROM) \
  EXPECT_EQ(1, TO(FROM(1001)));         \
  EXPECT_EQ(1, TO(FROM(1000)));         \
  EXPECT_EQ(0, TO(FROM(999)));          \
  EXPECT_EQ(0, TO(FROM(1)));            \
  EXPECT_EQ(0, TO(FROM(0)));            \
  EXPECT_EQ(-1, TO(FROM(-1)));          \
  EXPECT_EQ(-1, TO(FROM(-999)));        \
  EXPECT_EQ(-1, TO(FROM(-1000)));       \
  EXPECT_EQ(-2, TO(FROM(-1001)));

  TEST_FLOOR_CONVERSION(basic::ToUnixMicros, basic::FromUnixNanos);
  TEST_FLOOR_CONVERSION(basic::ToUnixMillis, basic::FromUnixMicros);
  TEST_FLOOR_CONVERSION(basic::ToUnixSeconds, basic::FromUnixMillis);
  TEST_FLOOR_CONVERSION(basic::ToTimeT, basic::FromUnixMillis);

#undef TEST_FLOOR_CONVERSION

  // Tests ToUnixNanos.
  EXPECT_EQ(1, basic::ToUnixNanos(basic::UnixEpoch() + basic::Nanoseconds(3) / 2));
  EXPECT_EQ(1, basic::ToUnixNanos(basic::UnixEpoch() + basic::Nanoseconds(1)));
  EXPECT_EQ(0, basic::ToUnixNanos(basic::UnixEpoch() + basic::Nanoseconds(1) / 2));
  EXPECT_EQ(0, basic::ToUnixNanos(basic::UnixEpoch() + basic::Nanoseconds(0)));
  EXPECT_EQ(-1,
            basic::ToUnixNanos(basic::UnixEpoch() - basic::Nanoseconds(1) / 2));
  EXPECT_EQ(-1, basic::ToUnixNanos(basic::UnixEpoch() - basic::Nanoseconds(1)));
  EXPECT_EQ(-2,
            basic::ToUnixNanos(basic::UnixEpoch() - basic::Nanoseconds(3) / 2));

  // Tests ToUniversal, which uses a different epoch than the tests above.
  EXPECT_EQ(1,
            basic::ToUniversal(basic::UniversalEpoch() + basic::Nanoseconds(101)));
  EXPECT_EQ(1,
            basic::ToUniversal(basic::UniversalEpoch() + basic::Nanoseconds(100)));
  EXPECT_EQ(0,
            basic::ToUniversal(basic::UniversalEpoch() + basic::Nanoseconds(99)));
  EXPECT_EQ(0,
            basic::ToUniversal(basic::UniversalEpoch() + basic::Nanoseconds(1)));
  EXPECT_EQ(0,
            basic::ToUniversal(basic::UniversalEpoch() + basic::Nanoseconds(0)));
  EXPECT_EQ(-1,
            basic::ToUniversal(basic::UniversalEpoch() + basic::Nanoseconds(-1)));
  EXPECT_EQ(-1,
            basic::ToUniversal(basic::UniversalEpoch() + basic::Nanoseconds(-99)));
  EXPECT_EQ(
      -1, basic::ToUniversal(basic::UniversalEpoch() + basic::Nanoseconds(-100)));
  EXPECT_EQ(
      -2, basic::ToUniversal(basic::UniversalEpoch() + basic::Nanoseconds(-101)));

  // Tests ToTimespec()/TimeFromTimespec()
  const struct {
    basic::Time t;
    timespec ts;
  } to_ts[] = {
      {basic::FromUnixSeconds(1) + basic::Nanoseconds(1), {1, 1}},
      {basic::FromUnixSeconds(1) + basic::Nanoseconds(1) / 2, {1, 0}},
      {basic::FromUnixSeconds(1) + basic::Nanoseconds(0), {1, 0}},
      {basic::FromUnixSeconds(0) + basic::Nanoseconds(0), {0, 0}},
      {basic::FromUnixSeconds(0) - basic::Nanoseconds(1) / 2, {-1, 999999999}},
      {basic::FromUnixSeconds(0) - basic::Nanoseconds(1), {-1, 999999999}},
      {basic::FromUnixSeconds(-1) + basic::Nanoseconds(1), {-1, 1}},
      {basic::FromUnixSeconds(-1) + basic::Nanoseconds(1) / 2, {-1, 0}},
      {basic::FromUnixSeconds(-1) + basic::Nanoseconds(0), {-1, 0}},
      {basic::FromUnixSeconds(-1) - basic::Nanoseconds(1) / 2, {-2, 999999999}},
  };
  for (const auto& test : to_ts) {
    EXPECT_THAT(basic::ToTimespec(test.t), TimespecMatcher(test.ts));
  }
  const struct {
    timespec ts;
    basic::Time t;
  } from_ts[] = {
      {{1, 1}, basic::FromUnixSeconds(1) + basic::Nanoseconds(1)},
      {{1, 0}, basic::FromUnixSeconds(1) + basic::Nanoseconds(0)},
      {{0, 0}, basic::FromUnixSeconds(0) + basic::Nanoseconds(0)},
      {{0, -1}, basic::FromUnixSeconds(0) - basic::Nanoseconds(1)},
      {{-1, 999999999}, basic::FromUnixSeconds(0) - basic::Nanoseconds(1)},
      {{-1, 1}, basic::FromUnixSeconds(-1) + basic::Nanoseconds(1)},
      {{-1, 0}, basic::FromUnixSeconds(-1) + basic::Nanoseconds(0)},
      {{-1, -1}, basic::FromUnixSeconds(-1) - basic::Nanoseconds(1)},
      {{-2, 999999999}, basic::FromUnixSeconds(-1) - basic::Nanoseconds(1)},
  };
  for (const auto& test : from_ts) {
    EXPECT_EQ(test.t, basic::TimeFromTimespec(test.ts));
  }

  // Tests ToTimeval()/TimeFromTimeval() (same as timespec above)
  const struct {
    basic::Time t;
    timeval tv;
  } to_tv[] = {
      {basic::FromUnixSeconds(1) + basic::Microseconds(1), {1, 1}},
      {basic::FromUnixSeconds(1) + basic::Microseconds(1) / 2, {1, 0}},
      {basic::FromUnixSeconds(1) + basic::Microseconds(0), {1, 0}},
      {basic::FromUnixSeconds(0) + basic::Microseconds(0), {0, 0}},
      {basic::FromUnixSeconds(0) - basic::Microseconds(1) / 2, {-1, 999999}},
      {basic::FromUnixSeconds(0) - basic::Microseconds(1), {-1, 999999}},
      {basic::FromUnixSeconds(-1) + basic::Microseconds(1), {-1, 1}},
      {basic::FromUnixSeconds(-1) + basic::Microseconds(1) / 2, {-1, 0}},
      {basic::FromUnixSeconds(-1) + basic::Microseconds(0), {-1, 0}},
      {basic::FromUnixSeconds(-1) - basic::Microseconds(1) / 2, {-2, 999999}},
  };
  for (const auto& test : to_tv) {
    EXPECT_THAT(ToTimeval(test.t), TimevalMatcher(test.tv));
  }
  const struct {
    timeval tv;
    basic::Time t;
  } from_tv[] = {
      {{1, 1}, basic::FromUnixSeconds(1) + basic::Microseconds(1)},
      {{1, 0}, basic::FromUnixSeconds(1) + basic::Microseconds(0)},
      {{0, 0}, basic::FromUnixSeconds(0) + basic::Microseconds(0)},
      {{0, -1}, basic::FromUnixSeconds(0) - basic::Microseconds(1)},
      {{-1, 999999}, basic::FromUnixSeconds(0) - basic::Microseconds(1)},
      {{-1, 1}, basic::FromUnixSeconds(-1) + basic::Microseconds(1)},
      {{-1, 0}, basic::FromUnixSeconds(-1) + basic::Microseconds(0)},
      {{-1, -1}, basic::FromUnixSeconds(-1) - basic::Microseconds(1)},
      {{-2, 999999}, basic::FromUnixSeconds(-1) - basic::Microseconds(1)},
  };
  for (const auto& test : from_tv) {
    EXPECT_EQ(test.t, basic::TimeFromTimeval(test.tv));
  }

  // Tests flooring near negative infinity.
  const int64_t min_plus_1 = std::numeric_limits<int64_t>::min() + 1;
  EXPECT_EQ(min_plus_1, basic::ToUnixSeconds(basic::FromUnixSeconds(min_plus_1)));
  EXPECT_EQ(std::numeric_limits<int64_t>::min(),
            basic::ToUnixSeconds(
                basic::FromUnixSeconds(min_plus_1) - basic::Nanoseconds(1) / 2));

  // Tests flooring near positive infinity.
  EXPECT_EQ(std::numeric_limits<int64_t>::max(),
            basic::ToUnixSeconds(basic::FromUnixSeconds(
                std::numeric_limits<int64_t>::max()) + basic::Nanoseconds(1) / 2));
  EXPECT_EQ(std::numeric_limits<int64_t>::max(),
            basic::ToUnixSeconds(
                basic::FromUnixSeconds(std::numeric_limits<int64_t>::max())));
  EXPECT_EQ(std::numeric_limits<int64_t>::max() - 1,
            basic::ToUnixSeconds(basic::FromUnixSeconds(
                std::numeric_limits<int64_t>::max()) - basic::Nanoseconds(1) / 2));
}

TEST(Time, RoundtripConversion) {
#define TEST_CONVERSION_ROUND_TRIP(SOURCE, FROM, TO, MATCHER) \
  EXPECT_THAT(TO(FROM(SOURCE)), MATCHER(SOURCE))

  // FromUnixNanos() and ToUnixNanos()
  int64_t now_ns = basic::GetCurrentTimeNanos();
  TEST_CONVERSION_ROUND_TRIP(-1, basic::FromUnixNanos, basic::ToUnixNanos,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(0, basic::FromUnixNanos, basic::ToUnixNanos,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(1, basic::FromUnixNanos, basic::ToUnixNanos,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(now_ns, basic::FromUnixNanos, basic::ToUnixNanos,
                             testing::Eq)
      << now_ns;

  // FromUnixMicros() and ToUnixMicros()
  int64_t now_us = basic::GetCurrentTimeNanos() / 1000;
  TEST_CONVERSION_ROUND_TRIP(-1, basic::FromUnixMicros, basic::ToUnixMicros,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(0, basic::FromUnixMicros, basic::ToUnixMicros,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(1, basic::FromUnixMicros, basic::ToUnixMicros,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(now_us, basic::FromUnixMicros, basic::ToUnixMicros,
                             testing::Eq)
      << now_us;

  // FromUnixMillis() and ToUnixMillis()
  int64_t now_ms = basic::GetCurrentTimeNanos() / 1000000;
  TEST_CONVERSION_ROUND_TRIP(-1, basic::FromUnixMillis, basic::ToUnixMillis,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(0, basic::FromUnixMillis, basic::ToUnixMillis,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(1, basic::FromUnixMillis, basic::ToUnixMillis,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(now_ms, basic::FromUnixMillis, basic::ToUnixMillis,
                             testing::Eq)
      << now_ms;

  // FromUnixSeconds() and ToUnixSeconds()
  int64_t now_s = std::time(nullptr);
  TEST_CONVERSION_ROUND_TRIP(-1, basic::FromUnixSeconds, basic::ToUnixSeconds,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(0, basic::FromUnixSeconds, basic::ToUnixSeconds,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(1, basic::FromUnixSeconds, basic::ToUnixSeconds,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(now_s, basic::FromUnixSeconds, basic::ToUnixSeconds,
                             testing::Eq)
      << now_s;

  // FromTimeT() and ToTimeT()
  time_t now_time_t = std::time(nullptr);
  TEST_CONVERSION_ROUND_TRIP(-1, basic::FromTimeT, basic::ToTimeT, testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(0, basic::FromTimeT, basic::ToTimeT, testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(1, basic::FromTimeT, basic::ToTimeT, testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(now_time_t, basic::FromTimeT, basic::ToTimeT,
                             testing::Eq)
      << now_time_t;

  // TimeFromTimeval() and ToTimeval()
  timeval tv;
  tv.tv_sec = -1;
  tv.tv_usec = 0;
  TEST_CONVERSION_ROUND_TRIP(tv, basic::TimeFromTimeval, basic::ToTimeval,
                             TimevalMatcher);
  tv.tv_sec = -1;
  tv.tv_usec = 999999;
  TEST_CONVERSION_ROUND_TRIP(tv, basic::TimeFromTimeval, basic::ToTimeval,
                             TimevalMatcher);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  TEST_CONVERSION_ROUND_TRIP(tv, basic::TimeFromTimeval, basic::ToTimeval,
                             TimevalMatcher);
  tv.tv_sec = 0;
  tv.tv_usec = 1;
  TEST_CONVERSION_ROUND_TRIP(tv, basic::TimeFromTimeval, basic::ToTimeval,
                             TimevalMatcher);
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  TEST_CONVERSION_ROUND_TRIP(tv, basic::TimeFromTimeval, basic::ToTimeval,
                             TimevalMatcher);

  // TimeFromTimespec() and ToTimespec()
  timespec ts;
  ts.tv_sec = -1;
  ts.tv_nsec = 0;
  TEST_CONVERSION_ROUND_TRIP(ts, basic::TimeFromTimespec, basic::ToTimespec,
                             TimespecMatcher);
  ts.tv_sec = -1;
  ts.tv_nsec = 999999999;
  TEST_CONVERSION_ROUND_TRIP(ts, basic::TimeFromTimespec, basic::ToTimespec,
                             TimespecMatcher);
  ts.tv_sec = 0;
  ts.tv_nsec = 0;
  TEST_CONVERSION_ROUND_TRIP(ts, basic::TimeFromTimespec, basic::ToTimespec,
                             TimespecMatcher);
  ts.tv_sec = 0;
  ts.tv_nsec = 1;
  TEST_CONVERSION_ROUND_TRIP(ts, basic::TimeFromTimespec, basic::ToTimespec,
                             TimespecMatcher);
  ts.tv_sec = 1;
  ts.tv_nsec = 0;
  TEST_CONVERSION_ROUND_TRIP(ts, basic::TimeFromTimespec, basic::ToTimespec,
                             TimespecMatcher);

  // FromUDate() and ToUDate()
  double now_ud = basic::GetCurrentTimeNanos() / 1000000;
  TEST_CONVERSION_ROUND_TRIP(-1.5, basic::FromUDate, basic::ToUDate,
                             testing::DoubleEq);
  TEST_CONVERSION_ROUND_TRIP(-1, basic::FromUDate, basic::ToUDate,
                             testing::DoubleEq);
  TEST_CONVERSION_ROUND_TRIP(-0.5, basic::FromUDate, basic::ToUDate,
                             testing::DoubleEq);
  TEST_CONVERSION_ROUND_TRIP(0, basic::FromUDate, basic::ToUDate,
                             testing::DoubleEq);
  TEST_CONVERSION_ROUND_TRIP(0.5, basic::FromUDate, basic::ToUDate,
                             testing::DoubleEq);
  TEST_CONVERSION_ROUND_TRIP(1, basic::FromUDate, basic::ToUDate,
                             testing::DoubleEq);
  TEST_CONVERSION_ROUND_TRIP(1.5, basic::FromUDate, basic::ToUDate,
                             testing::DoubleEq);
  TEST_CONVERSION_ROUND_TRIP(now_ud, basic::FromUDate, basic::ToUDate,
                             testing::DoubleEq)
      << std::fixed << std::setprecision(17) << now_ud;

  // FromUniversal() and ToUniversal()
  int64_t now_uni = ((719162LL * (24 * 60 * 60)) * (1000 * 1000 * 10)) +
                    (basic::GetCurrentTimeNanos() / 100);
  TEST_CONVERSION_ROUND_TRIP(-1, basic::FromUniversal, basic::ToUniversal,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(0, basic::FromUniversal, basic::ToUniversal,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(1, basic::FromUniversal, basic::ToUniversal,
                             testing::Eq);
  TEST_CONVERSION_ROUND_TRIP(now_uni, basic::FromUniversal, basic::ToUniversal,
                             testing::Eq)
      << now_uni;

#undef TEST_CONVERSION_ROUND_TRIP
}

template <typename Duration>
std::chrono::system_clock::time_point MakeChronoUnixTime(const Duration& d) {
  return std::chrono::system_clock::from_time_t(0) + d;
}

TEST(Time, FromChrono) {
  EXPECT_EQ(basic::FromTimeT(-1),
            basic::FromChrono(std::chrono::system_clock::from_time_t(-1)));
  EXPECT_EQ(basic::FromTimeT(0),
            basic::FromChrono(std::chrono::system_clock::from_time_t(0)));
  EXPECT_EQ(basic::FromTimeT(1),
            basic::FromChrono(std::chrono::system_clock::from_time_t(1)));

  EXPECT_EQ(
      basic::FromUnixMillis(-1),
      basic::FromChrono(MakeChronoUnixTime(std::chrono::milliseconds(-1))));
  EXPECT_EQ(basic::FromUnixMillis(0),
            basic::FromChrono(MakeChronoUnixTime(std::chrono::milliseconds(0))));
  EXPECT_EQ(basic::FromUnixMillis(1),
            basic::FromChrono(MakeChronoUnixTime(std::chrono::milliseconds(1))));

  // Chrono doesn't define exactly its range and precision (neither does
  // basic::Time), so let's simply test +/- ~100 years to make sure things work.
  const auto century_sec = 60 * 60 * 24 * 365 * int64_t{100};
  const auto century = std::chrono::seconds(century_sec);
  const auto chrono_future = MakeChronoUnixTime(century);
  const auto chrono_past = MakeChronoUnixTime(-century);
  EXPECT_EQ(basic::FromUnixSeconds(century_sec),
            basic::FromChrono(chrono_future));
  EXPECT_EQ(basic::FromUnixSeconds(-century_sec), basic::FromChrono(chrono_past));

  // Roundtrip them both back to chrono.
  EXPECT_EQ(chrono_future,
            basic::ToChronoTime(basic::FromUnixSeconds(century_sec)));
  EXPECT_EQ(chrono_past,
            basic::ToChronoTime(basic::FromUnixSeconds(-century_sec)));
}

TEST(Time, ToChronoTime) {
  EXPECT_EQ(std::chrono::system_clock::from_time_t(-1),
            basic::ToChronoTime(basic::FromTimeT(-1)));
  EXPECT_EQ(std::chrono::system_clock::from_time_t(0),
            basic::ToChronoTime(basic::FromTimeT(0)));
  EXPECT_EQ(std::chrono::system_clock::from_time_t(1),
            basic::ToChronoTime(basic::FromTimeT(1)));

  EXPECT_EQ(MakeChronoUnixTime(std::chrono::milliseconds(-1)),
            basic::ToChronoTime(basic::FromUnixMillis(-1)));
  EXPECT_EQ(MakeChronoUnixTime(std::chrono::milliseconds(0)),
            basic::ToChronoTime(basic::FromUnixMillis(0)));
  EXPECT_EQ(MakeChronoUnixTime(std::chrono::milliseconds(1)),
            basic::ToChronoTime(basic::FromUnixMillis(1)));

  // Time before the Unix epoch should floor, not trunc.
  const auto tick = basic::Nanoseconds(1) / 4;
  EXPECT_EQ(std::chrono::system_clock::from_time_t(0) -
                std::chrono::system_clock::duration(1),
            basic::ToChronoTime(basic::UnixEpoch() - tick));
}

TEST(Time, TimeZoneAt) {
  const basic::TimeZone nyc =
      basic::time_internal::LoadTimeZone("America/New_York");
  const std::string fmt = "%a, %e %b %Y %H:%M:%S %z (%Z)";

  // A non-transition where the civil time is unique.
  basic::CivilSecond nov01(2013, 11, 1, 8, 30, 0);
  const auto nov01_ci = nyc.At(nov01);
  EXPECT_EQ(basic::TimeZone::TimeInfo::UNIQUE, nov01_ci.kind);
  EXPECT_EQ("Fri,  1 Nov 2013 08:30:00 -0400 (EDT)",
            basic::FormatTime(fmt, nov01_ci.pre, nyc));
  EXPECT_EQ(nov01_ci.pre, nov01_ci.trans);
  EXPECT_EQ(nov01_ci.pre, nov01_ci.post);
  EXPECT_EQ(nov01_ci.pre, basic::FromCivil(nov01, nyc));

  // A Spring DST transition, when there is a gap in civil time
  // and we prefer the later of the possible interpretations of a
  // non-existent time.
  basic::CivilSecond mar13(2011, 3, 13, 2, 15, 0);
  const auto mar_ci = nyc.At(mar13);
  EXPECT_EQ(basic::TimeZone::TimeInfo::SKIPPED, mar_ci.kind);
  EXPECT_EQ("Sun, 13 Mar 2011 03:15:00 -0400 (EDT)",
            basic::FormatTime(fmt, mar_ci.pre, nyc));
  EXPECT_EQ("Sun, 13 Mar 2011 03:00:00 -0400 (EDT)",
            basic::FormatTime(fmt, mar_ci.trans, nyc));
  EXPECT_EQ("Sun, 13 Mar 2011 01:15:00 -0500 (EST)",
            basic::FormatTime(fmt, mar_ci.post, nyc));
  EXPECT_EQ(mar_ci.trans, basic::FromCivil(mar13, nyc));

  // A Fall DST transition, when civil times are repeated and
  // we prefer the earlier of the possible interpretations of an
  // ambiguous time.
  basic::CivilSecond nov06(2011, 11, 6, 1, 15, 0);
  const auto nov06_ci = nyc.At(nov06);
  EXPECT_EQ(basic::TimeZone::TimeInfo::REPEATED, nov06_ci.kind);
  EXPECT_EQ("Sun,  6 Nov 2011 01:15:00 -0400 (EDT)",
            basic::FormatTime(fmt, nov06_ci.pre, nyc));
  EXPECT_EQ("Sun,  6 Nov 2011 01:00:00 -0500 (EST)",
            basic::FormatTime(fmt, nov06_ci.trans, nyc));
  EXPECT_EQ("Sun,  6 Nov 2011 01:15:00 -0500 (EST)",
            basic::FormatTime(fmt, nov06_ci.post, nyc));
  EXPECT_EQ(nov06_ci.pre, basic::FromCivil(nov06, nyc));

  // Check that (time_t) -1 is handled correctly.
  basic::CivilSecond minus1(1969, 12, 31, 18, 59, 59);
  const auto minus1_cl = nyc.At(minus1);
  EXPECT_EQ(basic::TimeZone::TimeInfo::UNIQUE, minus1_cl.kind);
  EXPECT_EQ(-1, basic::ToTimeT(minus1_cl.pre));
  EXPECT_EQ("Wed, 31 Dec 1969 18:59:59 -0500 (EST)",
            basic::FormatTime(fmt, minus1_cl.pre, nyc));
  EXPECT_EQ("Wed, 31 Dec 1969 23:59:59 +0000 (UTC)",
            basic::FormatTime(fmt, minus1_cl.pre, basic::UTCTimeZone()));
}

// FromCivil(CivilSecond(year, mon, day, hour, min, sec), UTCTimeZone())
// has a specialized fastpath implementation, which we exercise here.
TEST(Time, FromCivilUTC) {
  const basic::TimeZone utc = basic::UTCTimeZone();
  const std::string fmt = "%a, %e %b %Y %H:%M:%S %z (%Z)";
  const int kMax = std::numeric_limits<int>::max();
  const int kMin = std::numeric_limits<int>::min();
  basic::Time t;

  // 292091940881 is the last positive year to use the fastpath.
  t = basic::FromCivil(
      basic::CivilSecond(292091940881, kMax, kMax, kMax, kMax, kMax), utc);
  EXPECT_EQ("Fri, 25 Nov 292277026596 12:21:07 +0000 (UTC)",
            basic::FormatTime(fmt, t, utc));
  t = basic::FromCivil(
      basic::CivilSecond(292091940882, kMax, kMax, kMax, kMax, kMax), utc);
  EXPECT_EQ("infinite-future", basic::FormatTime(fmt, t, utc));  // no overflow

  // -292091936940 is the last negative year to use the fastpath.
  t = basic::FromCivil(
      basic::CivilSecond(-292091936940, kMin, kMin, kMin, kMin, kMin), utc);
  EXPECT_EQ("Fri,  1 Nov -292277022657 10:37:52 +0000 (UTC)",
            basic::FormatTime(fmt, t, utc));
  t = basic::FromCivil(
      basic::CivilSecond(-292091936941, kMin, kMin, kMin, kMin, kMin), utc);
  EXPECT_EQ("infinite-past", basic::FormatTime(fmt, t, utc));  // no underflow

  // Check that we're counting leap years correctly.
  t = basic::FromCivil(basic::CivilSecond(1900, 2, 28, 23, 59, 59), utc);
  EXPECT_EQ("Wed, 28 Feb 1900 23:59:59 +0000 (UTC)",
            basic::FormatTime(fmt, t, utc));
  t = basic::FromCivil(basic::CivilSecond(1900, 3, 1, 0, 0, 0), utc);
  EXPECT_EQ("Thu,  1 Mar 1900 00:00:00 +0000 (UTC)",
            basic::FormatTime(fmt, t, utc));
  t = basic::FromCivil(basic::CivilSecond(2000, 2, 29, 23, 59, 59), utc);
  EXPECT_EQ("Tue, 29 Feb 2000 23:59:59 +0000 (UTC)",
            basic::FormatTime(fmt, t, utc));
  t = basic::FromCivil(basic::CivilSecond(2000, 3, 1, 0, 0, 0), utc);
  EXPECT_EQ("Wed,  1 Mar 2000 00:00:00 +0000 (UTC)",
            basic::FormatTime(fmt, t, utc));
}

TEST(Time, ToTM) {
  const basic::TimeZone utc = basic::UTCTimeZone();

  // Compares the results of ToTM() to gmtime_r() for lots of times over the
  // course of a few days.
  const basic::Time start =
      basic::FromCivil(basic::CivilSecond(2014, 1, 2, 3, 4, 5), utc);
  const basic::Time end =
      basic::FromCivil(basic::CivilSecond(2014, 1, 5, 3, 4, 5), utc);
  for (basic::Time t = start; t < end; t += basic::Seconds(30)) {
    const struct tm tm_bt = ToTM(t, utc);
    const time_t tt = basic::ToTimeT(t);
    struct tm tm_lc;
#ifdef _WIN32
    gmtime_s(&tm_lc, &tt);
#else
    gmtime_r(&tt, &tm_lc);
#endif
    EXPECT_EQ(tm_lc.tm_year, tm_bt.tm_year);
    EXPECT_EQ(tm_lc.tm_mon, tm_bt.tm_mon);
    EXPECT_EQ(tm_lc.tm_mday, tm_bt.tm_mday);
    EXPECT_EQ(tm_lc.tm_hour, tm_bt.tm_hour);
    EXPECT_EQ(tm_lc.tm_min, tm_bt.tm_min);
    EXPECT_EQ(tm_lc.tm_sec, tm_bt.tm_sec);
    EXPECT_EQ(tm_lc.tm_wday, tm_bt.tm_wday);
    EXPECT_EQ(tm_lc.tm_yday, tm_bt.tm_yday);
    EXPECT_EQ(tm_lc.tm_isdst, tm_bt.tm_isdst);

    ASSERT_FALSE(HasFailure());
  }

  // Checks that the tm_isdst field is correct when in standard time.
  const basic::TimeZone nyc =
      basic::time_internal::LoadTimeZone("America/New_York");
  basic::Time t = basic::FromCivil(basic::CivilSecond(2014, 3, 1, 0, 0, 0), nyc);
  struct tm tm = ToTM(t, nyc);
  EXPECT_FALSE(tm.tm_isdst);

  // Checks that the tm_isdst field is correct when in daylight time.
  t = basic::FromCivil(basic::CivilSecond(2014, 4, 1, 0, 0, 0), nyc);
  tm = ToTM(t, nyc);
  EXPECT_TRUE(tm.tm_isdst);

  // Checks overflow.
  tm = ToTM(basic::InfiniteFuture(), nyc);
  EXPECT_EQ(std::numeric_limits<int>::max() - 1900, tm.tm_year);
  EXPECT_EQ(11, tm.tm_mon);
  EXPECT_EQ(31, tm.tm_mday);
  EXPECT_EQ(23, tm.tm_hour);
  EXPECT_EQ(59, tm.tm_min);
  EXPECT_EQ(59, tm.tm_sec);
  EXPECT_EQ(4, tm.tm_wday);
  EXPECT_EQ(364, tm.tm_yday);
  EXPECT_FALSE(tm.tm_isdst);

  // Checks underflow.
  tm = ToTM(basic::InfinitePast(), nyc);
  EXPECT_EQ(std::numeric_limits<int>::min(), tm.tm_year);
  EXPECT_EQ(0, tm.tm_mon);
  EXPECT_EQ(1, tm.tm_mday);
  EXPECT_EQ(0, tm.tm_hour);
  EXPECT_EQ(0, tm.tm_min);
  EXPECT_EQ(0, tm.tm_sec);
  EXPECT_EQ(0, tm.tm_wday);
  EXPECT_EQ(0, tm.tm_yday);
  EXPECT_FALSE(tm.tm_isdst);
}

TEST(Time, FromTM) {
  const basic::TimeZone nyc =
      basic::time_internal::LoadTimeZone("America/New_York");

  // Verifies that tm_isdst doesn't affect anything when the time is unique.
  struct tm tm;
  std::memset(&tm, 0, sizeof(tm));
  tm.tm_year = 2014 - 1900;
  tm.tm_mon = 6 - 1;
  tm.tm_mday = 28;
  tm.tm_hour = 1;
  tm.tm_min = 2;
  tm.tm_sec = 3;
  tm.tm_isdst = -1;
  basic::Time t = FromTM(tm, nyc);
  EXPECT_EQ("2014-06-28T01:02:03-04:00", basic::FormatTime(t, nyc));  // DST
  tm.tm_isdst = 0;
  t = FromTM(tm, nyc);
  EXPECT_EQ("2014-06-28T01:02:03-04:00", basic::FormatTime(t, nyc));  // DST
  tm.tm_isdst = 1;
  t = FromTM(tm, nyc);
  EXPECT_EQ("2014-06-28T01:02:03-04:00", basic::FormatTime(t, nyc));  // DST

  // Adjusts tm to refer to an ambiguous time.
  tm.tm_year = 2014 - 1900;
  tm.tm_mon = 11 - 1;
  tm.tm_mday = 2;
  tm.tm_hour = 1;
  tm.tm_min = 30;
  tm.tm_sec = 42;
  tm.tm_isdst = -1;
  t = FromTM(tm, nyc);
  EXPECT_EQ("2014-11-02T01:30:42-04:00", basic::FormatTime(t, nyc));  // DST
  tm.tm_isdst = 0;
  t = FromTM(tm, nyc);
  EXPECT_EQ("2014-11-02T01:30:42-05:00", basic::FormatTime(t, nyc));  // STD
  tm.tm_isdst = 1;
  t = FromTM(tm, nyc);
  EXPECT_EQ("2014-11-02T01:30:42-04:00", basic::FormatTime(t, nyc));  // DST

  // Adjusts tm to refer to a skipped time.
  tm.tm_year = 2014 - 1900;
  tm.tm_mon = 3 - 1;
  tm.tm_mday = 9;
  tm.tm_hour = 2;
  tm.tm_min = 30;
  tm.tm_sec = 42;
  tm.tm_isdst = -1;
  t = FromTM(tm, nyc);
  EXPECT_EQ("2014-03-09T03:30:42-04:00", basic::FormatTime(t, nyc));  // DST
  tm.tm_isdst = 0;
  t = FromTM(tm, nyc);
  EXPECT_EQ("2014-03-09T01:30:42-05:00", basic::FormatTime(t, nyc));  // STD
  tm.tm_isdst = 1;
  t = FromTM(tm, nyc);
  EXPECT_EQ("2014-03-09T03:30:42-04:00", basic::FormatTime(t, nyc));  // DST
}

TEST(Time, TMRoundTrip) {
  const basic::TimeZone nyc =
      basic::time_internal::LoadTimeZone("America/New_York");

  // Test round-tripping across a skipped transition
  basic::Time start = basic::FromCivil(basic::CivilHour(2014, 3, 9, 0), nyc);
  basic::Time end = basic::FromCivil(basic::CivilHour(2014, 3, 9, 4), nyc);
  for (basic::Time t = start; t < end; t += basic::Minutes(1)) {
    struct tm tm = ToTM(t, nyc);
    basic::Time rt = FromTM(tm, nyc);
    EXPECT_EQ(rt, t);
  }

  // Test round-tripping across an ambiguous transition
  start = basic::FromCivil(basic::CivilHour(2014, 11, 2, 0), nyc);
  end = basic::FromCivil(basic::CivilHour(2014, 11, 2, 4), nyc);
  for (basic::Time t = start; t < end; t += basic::Minutes(1)) {
    struct tm tm = ToTM(t, nyc);
    basic::Time rt = FromTM(tm, nyc);
    EXPECT_EQ(rt, t);
  }

  // Test round-tripping of unique instants crossing a day boundary
  start = basic::FromCivil(basic::CivilHour(2014, 6, 27, 22), nyc);
  end = basic::FromCivil(basic::CivilHour(2014, 6, 28, 4), nyc);
  for (basic::Time t = start; t < end; t += basic::Minutes(1)) {
    struct tm tm = ToTM(t, nyc);
    basic::Time rt = FromTM(tm, nyc);
    EXPECT_EQ(rt, t);
  }
}

TEST(Time, Range) {
  // The API's documented range is +/- 100 billion years.
  const basic::Duration range = basic::Hours(24) * 365.2425 * 100000000000;

  // Arithmetic and comparison still works at +/-range around base values.
  basic::Time bases[2] = {basic::UnixEpoch(), basic::Now()};
  for (const auto base : bases) {
    basic::Time bottom = base - range;
    EXPECT_GT(bottom, bottom - basic::Nanoseconds(1));
    EXPECT_LT(bottom, bottom + basic::Nanoseconds(1));
    basic::Time top = base + range;
    EXPECT_GT(top, top - basic::Nanoseconds(1));
    EXPECT_LT(top, top + basic::Nanoseconds(1));
    basic::Duration full_range = 2 * range;
    EXPECT_EQ(full_range, top - bottom);
    EXPECT_EQ(-full_range, bottom - top);
  }
}

TEST(Time, Limits) {
  // It is an implementation detail that Time().rep_ == ZeroDuration(),
  // and that the resolution of a Duration is 1/4 of a nanosecond.
  const basic::Time zero;
  const basic::Time max =
      zero + basic::Seconds(std::numeric_limits<int64_t>::max()) +
      basic::Nanoseconds(999999999) + basic::Nanoseconds(3) / 4;
  const basic::Time min =
      zero + basic::Seconds(std::numeric_limits<int64_t>::min());

  // Some simple max/min bounds checks.
  EXPECT_LT(max, basic::InfiniteFuture());
  EXPECT_GT(min, basic::InfinitePast());
  EXPECT_LT(zero, max);
  EXPECT_GT(zero, min);
  EXPECT_GE(basic::UnixEpoch(), min);
  EXPECT_LT(basic::UnixEpoch(), max);

  // Check sign of Time differences.
  EXPECT_LT(basic::ZeroDuration(), max - zero);
  EXPECT_LT(basic::ZeroDuration(),
            zero - basic::Nanoseconds(1) / 4 - min);  // avoid zero - min

  // Arithmetic works at max - 0.25ns and min + 0.25ns.
  EXPECT_GT(max, max - basic::Nanoseconds(1) / 4);
  EXPECT_LT(min, min + basic::Nanoseconds(1) / 4);
}

TEST(Time, ConversionSaturation) {
  const basic::TimeZone utc = basic::UTCTimeZone();
  basic::Time t;

  const auto max_time_t = std::numeric_limits<time_t>::max();
  const auto min_time_t = std::numeric_limits<time_t>::min();
  time_t tt = max_time_t - 1;
  t = basic::FromTimeT(tt);
  tt = basic::ToTimeT(t);
  EXPECT_EQ(max_time_t - 1, tt);
  t += basic::Seconds(1);
  tt = basic::ToTimeT(t);
  EXPECT_EQ(max_time_t, tt);
  t += basic::Seconds(1);  // no effect
  tt = basic::ToTimeT(t);
  EXPECT_EQ(max_time_t, tt);

  tt = min_time_t + 1;
  t = basic::FromTimeT(tt);
  tt = basic::ToTimeT(t);
  EXPECT_EQ(min_time_t + 1, tt);
  t -= basic::Seconds(1);
  tt = basic::ToTimeT(t);
  EXPECT_EQ(min_time_t, tt);
  t -= basic::Seconds(1);  // no effect
  tt = basic::ToTimeT(t);
  EXPECT_EQ(min_time_t, tt);

  const auto max_timeval_sec =
      std::numeric_limits<decltype(timeval::tv_sec)>::max();
  const auto min_timeval_sec =
      std::numeric_limits<decltype(timeval::tv_sec)>::min();
  timeval tv;
  tv.tv_sec = max_timeval_sec;
  tv.tv_usec = 999998;
  t = basic::TimeFromTimeval(tv);
  tv = ToTimeval(t);
  EXPECT_EQ(max_timeval_sec, tv.tv_sec);
  EXPECT_EQ(999998, tv.tv_usec);
  t += basic::Microseconds(1);
  tv = ToTimeval(t);
  EXPECT_EQ(max_timeval_sec, tv.tv_sec);
  EXPECT_EQ(999999, tv.tv_usec);
  t += basic::Microseconds(1);  // no effect
  tv = ToTimeval(t);
  EXPECT_EQ(max_timeval_sec, tv.tv_sec);
  EXPECT_EQ(999999, tv.tv_usec);

  tv.tv_sec = min_timeval_sec;
  tv.tv_usec = 1;
  t = basic::TimeFromTimeval(tv);
  tv = ToTimeval(t);
  EXPECT_EQ(min_timeval_sec, tv.tv_sec);
  EXPECT_EQ(1, tv.tv_usec);
  t -= basic::Microseconds(1);
  tv = ToTimeval(t);
  EXPECT_EQ(min_timeval_sec, tv.tv_sec);
  EXPECT_EQ(0, tv.tv_usec);
  t -= basic::Microseconds(1);  // no effect
  tv = ToTimeval(t);
  EXPECT_EQ(min_timeval_sec, tv.tv_sec);
  EXPECT_EQ(0, tv.tv_usec);

  const auto max_timespec_sec =
      std::numeric_limits<decltype(timespec::tv_sec)>::max();
  const auto min_timespec_sec =
      std::numeric_limits<decltype(timespec::tv_sec)>::min();
  timespec ts;
  ts.tv_sec = max_timespec_sec;
  ts.tv_nsec = 999999998;
  t = basic::TimeFromTimespec(ts);
  ts = basic::ToTimespec(t);
  EXPECT_EQ(max_timespec_sec, ts.tv_sec);
  EXPECT_EQ(999999998, ts.tv_nsec);
  t += basic::Nanoseconds(1);
  ts = basic::ToTimespec(t);
  EXPECT_EQ(max_timespec_sec, ts.tv_sec);
  EXPECT_EQ(999999999, ts.tv_nsec);
  t += basic::Nanoseconds(1);  // no effect
  ts = basic::ToTimespec(t);
  EXPECT_EQ(max_timespec_sec, ts.tv_sec);
  EXPECT_EQ(999999999, ts.tv_nsec);

  ts.tv_sec = min_timespec_sec;
  ts.tv_nsec = 1;
  t = basic::TimeFromTimespec(ts);
  ts = basic::ToTimespec(t);
  EXPECT_EQ(min_timespec_sec, ts.tv_sec);
  EXPECT_EQ(1, ts.tv_nsec);
  t -= basic::Nanoseconds(1);
  ts = basic::ToTimespec(t);
  EXPECT_EQ(min_timespec_sec, ts.tv_sec);
  EXPECT_EQ(0, ts.tv_nsec);
  t -= basic::Nanoseconds(1);  // no effect
  ts = basic::ToTimespec(t);
  EXPECT_EQ(min_timespec_sec, ts.tv_sec);
  EXPECT_EQ(0, ts.tv_nsec);

  // Checks how TimeZone::At() saturates on infinities.
  auto ci = utc.At(basic::InfiniteFuture());
  EXPECT_CIVIL_INFO(ci, std::numeric_limits<int64_t>::max(), 12, 31, 23,
                            59, 59, 0, false);
  EXPECT_EQ(basic::InfiniteDuration(), ci.subsecond);
  EXPECT_EQ(basic::Weekday::thursday, basic::GetWeekday(ci.cs));
  EXPECT_EQ(365, basic::GetYearDay(ci.cs));
  EXPECT_STREQ("-00", ci.zone_abbr);  // artifact of TimeZone::At()
  ci = utc.At(basic::InfinitePast());
  EXPECT_CIVIL_INFO(ci, std::numeric_limits<int64_t>::min(), 1, 1, 0, 0,
                            0, 0, false);
  EXPECT_EQ(-basic::InfiniteDuration(), ci.subsecond);
  EXPECT_EQ(basic::Weekday::sunday, basic::GetWeekday(ci.cs));
  EXPECT_EQ(1, basic::GetYearDay(ci.cs));
  EXPECT_STREQ("-00", ci.zone_abbr);  // artifact of TimeZone::At()

  // Approach the maximal Time value from below.
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 4, 15, 30, 6), utc);
  EXPECT_EQ("292277026596-12-04T15:30:06+00:00",
            basic::FormatTime(basic::RFC3339_full, t, utc));
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 4, 15, 30, 7), utc);
  EXPECT_EQ("292277026596-12-04T15:30:07+00:00",
            basic::FormatTime(basic::RFC3339_full, t, utc));
  EXPECT_EQ(
      basic::UnixEpoch() + basic::Seconds(std::numeric_limits<int64_t>::max()), t);

  // Checks that we can also get the maximal Time value for a far-east zone.
  const basic::TimeZone plus14 = basic::FixedTimeZone(14 * 60 * 60);
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 5, 5, 30, 7), plus14);
  EXPECT_EQ("292277026596-12-05T05:30:07+14:00",
            basic::FormatTime(basic::RFC3339_full, t, plus14));
  EXPECT_EQ(
      basic::UnixEpoch() + basic::Seconds(std::numeric_limits<int64_t>::max()), t);

  // One second later should push us to infinity.
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 4, 15, 30, 8), utc);
  EXPECT_EQ("infinite-future", basic::FormatTime(basic::RFC3339_full, t, utc));

  // Approach the minimal Time value from above.
  t = basic::FromCivil(basic::CivilSecond(-292277022657, 1, 27, 8, 29, 53), utc);
  EXPECT_EQ("-292277022657-01-27T08:29:53+00:00",
            basic::FormatTime(basic::RFC3339_full, t, utc));
  t = basic::FromCivil(basic::CivilSecond(-292277022657, 1, 27, 8, 29, 52), utc);
  EXPECT_EQ("-292277022657-01-27T08:29:52+00:00",
            basic::FormatTime(basic::RFC3339_full, t, utc));
  EXPECT_EQ(
      basic::UnixEpoch() + basic::Seconds(std::numeric_limits<int64_t>::min()), t);

  // Checks that we can also get the minimal Time value for a far-west zone.
  const basic::TimeZone minus12 = basic::FixedTimeZone(-12 * 60 * 60);
  t = basic::FromCivil(basic::CivilSecond(-292277022657, 1, 26, 20, 29, 52),
                      minus12);
  EXPECT_EQ("-292277022657-01-26T20:29:52-12:00",
            basic::FormatTime(basic::RFC3339_full, t, minus12));
  EXPECT_EQ(
      basic::UnixEpoch() + basic::Seconds(std::numeric_limits<int64_t>::min()), t);

  // One second before should push us to -infinity.
  t = basic::FromCivil(basic::CivilSecond(-292277022657, 1, 27, 8, 29, 51), utc);
  EXPECT_EQ("infinite-past", basic::FormatTime(basic::RFC3339_full, t, utc));
}

// In zones with POSIX-style recurring rules we use special logic to
// handle conversions in the distant future.  Here we check the limits
// of those conversions, particularly with respect to integer overflow.
TEST(Time, ExtendedConversionSaturation) {
  const basic::TimeZone syd =
      basic::time_internal::LoadTimeZone("Australia/Sydney");
  const basic::TimeZone nyc =
      basic::time_internal::LoadTimeZone("America/New_York");
  const basic::Time max =
      basic::FromUnixSeconds(std::numeric_limits<int64_t>::max());
  basic::TimeZone::CivilInfo ci;
  basic::Time t;

  // The maximal time converted in each zone.
  ci = syd.At(max);
  EXPECT_CIVIL_INFO(ci, 292277026596, 12, 5, 2, 30, 7, 39600, true);
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 5, 2, 30, 7), syd);
  EXPECT_EQ(max, t);
  ci = nyc.At(max);
  EXPECT_CIVIL_INFO(ci, 292277026596, 12, 4, 10, 30, 7, -18000, false);
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 4, 10, 30, 7), nyc);
  EXPECT_EQ(max, t);

  // One second later should push us to infinity.
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 5, 2, 30, 8), syd);
  EXPECT_EQ(basic::InfiniteFuture(), t);
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 4, 10, 30, 8), nyc);
  EXPECT_EQ(basic::InfiniteFuture(), t);

  // And we should stick there.
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 5, 2, 30, 9), syd);
  EXPECT_EQ(basic::InfiniteFuture(), t);
  t = basic::FromCivil(basic::CivilSecond(292277026596, 12, 4, 10, 30, 9), nyc);
  EXPECT_EQ(basic::InfiniteFuture(), t);

  // All the way up to a saturated date/time, without overflow.
  t = basic::FromCivil(basic::CivilSecond::max(), syd);
  EXPECT_EQ(basic::InfiniteFuture(), t);
  t = basic::FromCivil(basic::CivilSecond::max(), nyc);
  EXPECT_EQ(basic::InfiniteFuture(), t);
}

TEST(Time, FromCivilAlignment) {
  const basic::TimeZone utc = basic::UTCTimeZone();
  const basic::CivilSecond cs(2015, 2, 3, 4, 5, 6);
  basic::Time t = basic::FromCivil(cs, utc);
  EXPECT_EQ("2015-02-03T04:05:06+00:00", basic::FormatTime(t, utc));
  t = basic::FromCivil(basic::CivilMinute(cs), utc);
  EXPECT_EQ("2015-02-03T04:05:00+00:00", basic::FormatTime(t, utc));
  t = basic::FromCivil(basic::CivilHour(cs), utc);
  EXPECT_EQ("2015-02-03T04:00:00+00:00", basic::FormatTime(t, utc));
  t = basic::FromCivil(basic::CivilDay(cs), utc);
  EXPECT_EQ("2015-02-03T00:00:00+00:00", basic::FormatTime(t, utc));
  t = basic::FromCivil(basic::CivilMonth(cs), utc);
  EXPECT_EQ("2015-02-01T00:00:00+00:00", basic::FormatTime(t, utc));
  t = basic::FromCivil(basic::CivilYear(cs), utc);
  EXPECT_EQ("2015-01-01T00:00:00+00:00", basic::FormatTime(t, utc));
}

TEST(Time, LegacyDateTime) {
  const basic::TimeZone utc = basic::UTCTimeZone();
  const std::string ymdhms = "%Y-%m-%d %H:%M:%S";
  const int kMax = std::numeric_limits<int>::max();
  const int kMin = std::numeric_limits<int>::min();
  basic::Time t;

  t = basic::FromDateTime(std::numeric_limits<basic::civil_year_t>::max(),
                         kMax, kMax, kMax, kMax, kMax, utc);
  EXPECT_EQ("infinite-future",
            basic::FormatTime(ymdhms, t, utc));  // no overflow
  t = basic::FromDateTime(std::numeric_limits<basic::civil_year_t>::min(),
                         kMin, kMin, kMin, kMin, kMin, utc);
  EXPECT_EQ("infinite-past",
            basic::FormatTime(ymdhms, t, utc));  // no overflow

  // Check normalization.
  EXPECT_TRUE(basic::ConvertDateTime(2013, 10, 32, 8, 30, 0, utc).normalized);
  t = basic::FromDateTime(2015, 1, 1, 0, 0, 60, utc);
  EXPECT_EQ("2015-01-01 00:01:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, 1, 1, 0, 60, 0, utc);
  EXPECT_EQ("2015-01-01 01:00:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, 1, 1, 24, 0, 0, utc);
  EXPECT_EQ("2015-01-02 00:00:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, 1, 32, 0, 0, 0, utc);
  EXPECT_EQ("2015-02-01 00:00:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, 13, 1, 0, 0, 0, utc);
  EXPECT_EQ("2016-01-01 00:00:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, 13, 32, 60, 60, 60, utc);
  EXPECT_EQ("2016-02-03 13:01:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, 1, 1, 0, 0, -1, utc);
  EXPECT_EQ("2014-12-31 23:59:59", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, 1, 1, 0, -1, 0, utc);
  EXPECT_EQ("2014-12-31 23:59:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, 1, 1, -1, 0, 0, utc);
  EXPECT_EQ("2014-12-31 23:00:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, 1, -1, 0, 0, 0, utc);
  EXPECT_EQ("2014-12-30 00:00:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, -1, 1, 0, 0, 0, utc);
  EXPECT_EQ("2014-11-01 00:00:00", basic::FormatTime(ymdhms, t, utc));
  t = basic::FromDateTime(2015, -1, -1, -1, -1, -1, utc);
  EXPECT_EQ("2014-10-29 22:58:59", basic::FormatTime(ymdhms, t, utc));
}

TEST(Time, NextTransitionUTC) {
  const auto tz = basic::UTCTimeZone();
  basic::TimeZone::CivilTransition trans;

  auto t = basic::InfinitePast();
  EXPECT_FALSE(tz.NextTransition(t, &trans));

  t = basic::InfiniteFuture();
  EXPECT_FALSE(tz.NextTransition(t, &trans));
}

TEST(Time, PrevTransitionUTC) {
  const auto tz = basic::UTCTimeZone();
  basic::TimeZone::CivilTransition trans;

  auto t = basic::InfiniteFuture();
  EXPECT_FALSE(tz.PrevTransition(t, &trans));

  t = basic::InfinitePast();
  EXPECT_FALSE(tz.PrevTransition(t, &trans));
}

TEST(Time, NextTransitionNYC) {
  const auto tz = basic::time_internal::LoadTimeZone("America/New_York");
  basic::TimeZone::CivilTransition trans;

  auto t = basic::FromCivil(basic::CivilSecond(2018, 6, 30, 0, 0, 0), tz);
  EXPECT_TRUE(tz.NextTransition(t, &trans));
  EXPECT_EQ(basic::CivilSecond(2018, 11, 4, 2, 0, 0), trans.from);
  EXPECT_EQ(basic::CivilSecond(2018, 11, 4, 1, 0, 0), trans.to);

  t = basic::InfiniteFuture();
  EXPECT_FALSE(tz.NextTransition(t, &trans));

  t = basic::InfinitePast();
  EXPECT_TRUE(tz.NextTransition(t, &trans));
  if (trans.from == basic::CivilSecond(1918, 03, 31, 2, 0, 0)) {
    // It looks like the tzdata is only 32 bit (probably macOS),
    // which bottoms out at 1901-12-13T20:45:52+00:00.
    EXPECT_EQ(basic::CivilSecond(1918, 3, 31, 3, 0, 0), trans.to);
  } else {
    EXPECT_EQ(basic::CivilSecond(1883, 11, 18, 12, 3, 58), trans.from);
    EXPECT_EQ(basic::CivilSecond(1883, 11, 18, 12, 0, 0), trans.to);
  }
}

TEST(Time, PrevTransitionNYC) {
  const auto tz = basic::time_internal::LoadTimeZone("America/New_York");
  basic::TimeZone::CivilTransition trans;

  auto t = basic::FromCivil(basic::CivilSecond(2018, 6, 30, 0, 0, 0), tz);
  EXPECT_TRUE(tz.PrevTransition(t, &trans));
  EXPECT_EQ(basic::CivilSecond(2018, 3, 11, 2, 0, 0), trans.from);
  EXPECT_EQ(basic::CivilSecond(2018, 3, 11, 3, 0, 0), trans.to);

  t = basic::InfinitePast();
  EXPECT_FALSE(tz.PrevTransition(t, &trans));

  t = basic::InfiniteFuture();
  EXPECT_TRUE(tz.PrevTransition(t, &trans));
  // We have a transition but we don't know which one.
}

}  // namespace
