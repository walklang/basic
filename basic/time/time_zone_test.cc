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

#include "basic/time/internal/cctz/include/cctz/time_zone.h"

#include "gtest/gtest.h"
#include "basic/time/internal/test_util.h"
#include "basic/time/time.h"

namespace cctz = basic::time_internal::cctz;

namespace {

TEST(TimeZone, ValueSemantics) {
  basic::TimeZone tz;
  basic::TimeZone tz2 = tz;  // Copy-construct
  EXPECT_EQ(tz, tz2);
  tz2 = tz;  // Copy-assign
  EXPECT_EQ(tz, tz2);
}

TEST(TimeZone, Equality) {
  basic::TimeZone a, b;
  EXPECT_EQ(a, b);
  EXPECT_EQ(a.name(), b.name());

  basic::TimeZone implicit_utc;
  basic::TimeZone explicit_utc = basic::UTCTimeZone();
  EXPECT_EQ(implicit_utc, explicit_utc);
  EXPECT_EQ(implicit_utc.name(), explicit_utc.name());

  basic::TimeZone la = basic::time_internal::LoadTimeZone("America/Los_Angeles");
  basic::TimeZone nyc = basic::time_internal::LoadTimeZone("America/New_York");
  EXPECT_NE(la, nyc);
}

TEST(TimeZone, CCTZConversion) {
  const cctz::time_zone cz = cctz::utc_time_zone();
  const basic::TimeZone tz(cz);
  EXPECT_EQ(cz, cctz::time_zone(tz));
}

TEST(TimeZone, DefaultTimeZones) {
  basic::TimeZone tz;
  EXPECT_EQ("UTC", basic::TimeZone().name());
  EXPECT_EQ("UTC", basic::UTCTimeZone().name());
}

TEST(TimeZone, FixedTimeZone) {
  const basic::TimeZone tz = basic::FixedTimeZone(123);
  const cctz::time_zone cz = cctz::fixed_time_zone(cctz::seconds(123));
  EXPECT_EQ(tz, basic::TimeZone(cz));
}

TEST(TimeZone, LocalTimeZone) {
  const basic::TimeZone local_tz = basic::LocalTimeZone();
  basic::TimeZone tz = basic::time_internal::LoadTimeZone("localtime");
  EXPECT_EQ(tz, local_tz);
}

TEST(TimeZone, NamedTimeZones) {
  basic::TimeZone nyc = basic::time_internal::LoadTimeZone("America/New_York");
  EXPECT_EQ("America/New_York", nyc.name());
  basic::TimeZone syd = basic::time_internal::LoadTimeZone("Australia/Sydney");
  EXPECT_EQ("Australia/Sydney", syd.name());
  basic::TimeZone fixed = basic::FixedTimeZone((((3 * 60) + 25) * 60) + 45);
  EXPECT_EQ("Fixed/UTC+03:25:45", fixed.name());
}

TEST(TimeZone, Failures) {
  basic::TimeZone tz = basic::time_internal::LoadTimeZone("America/Los_Angeles");
  EXPECT_FALSE(LoadTimeZone("Invalid/TimeZone", &tz));
  EXPECT_EQ(basic::UTCTimeZone(), tz);  // guaranteed fallback to UTC

  // Ensures that the load still fails on a subsequent attempt.
  tz = basic::time_internal::LoadTimeZone("America/Los_Angeles");
  EXPECT_FALSE(LoadTimeZone("Invalid/TimeZone", &tz));
  EXPECT_EQ(basic::UTCTimeZone(), tz);  // guaranteed fallback to UTC

  // Loading an empty std::string timezone should fail.
  tz = basic::time_internal::LoadTimeZone("America/Los_Angeles");
  EXPECT_FALSE(LoadTimeZone("", &tz));
  EXPECT_EQ(basic::UTCTimeZone(), tz);  // guaranteed fallback to UTC
}

}  // namespace
