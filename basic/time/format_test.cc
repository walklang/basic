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

#include <cstdint>
#include <limits>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "basic/time/internal/test_util.h"
#include "basic/time/time.h"

using testing::HasSubstr;

namespace {

// A helper that tests the given format specifier by itself, and with leading
// and trailing characters.  For example: TestFormatSpecifier(t, "%a", "Thu").
void TestFormatSpecifier(basic::Time t, basic::TimeZone tz,
                         const std::string& fmt, const std::string& ans) {
  EXPECT_EQ(ans, basic::FormatTime(fmt, t, tz));
  EXPECT_EQ("xxx " + ans, basic::FormatTime("xxx " + fmt, t, tz));
  EXPECT_EQ(ans + " yyy", basic::FormatTime(fmt + " yyy", t, tz));
  EXPECT_EQ("xxx " + ans + " yyy",
            basic::FormatTime("xxx " + fmt + " yyy", t, tz));
}

//
// Testing FormatTime()
//

TEST(FormatTime, Basics) {
  basic::TimeZone tz = basic::UTCTimeZone();
  basic::Time t = basic::FromTimeT(0);

  // Starts with a couple basic edge cases.
  EXPECT_EQ("", basic::FormatTime("", t, tz));
  EXPECT_EQ(" ", basic::FormatTime(" ", t, tz));
  EXPECT_EQ("  ", basic::FormatTime("  ", t, tz));
  EXPECT_EQ("xxx", basic::FormatTime("xxx", t, tz));
  std::string big(128, 'x');
  EXPECT_EQ(big, basic::FormatTime(big, t, tz));
  // Cause the 1024-byte buffer to grow.
  std::string bigger(100000, 'x');
  EXPECT_EQ(bigger, basic::FormatTime(bigger, t, tz));

  t += basic::Hours(13) + basic::Minutes(4) + basic::Seconds(5);
  t += basic::Milliseconds(6) + basic::Microseconds(7) + basic::Nanoseconds(8);
  EXPECT_EQ("1970-01-01", basic::FormatTime("%Y-%m-%d", t, tz));
  EXPECT_EQ("13:04:05", basic::FormatTime("%H:%M:%S", t, tz));
  EXPECT_EQ("13:04:05.006", basic::FormatTime("%H:%M:%E3S", t, tz));
  EXPECT_EQ("13:04:05.006007", basic::FormatTime("%H:%M:%E6S", t, tz));
  EXPECT_EQ("13:04:05.006007008", basic::FormatTime("%H:%M:%E9S", t, tz));
}

TEST(FormatTime, LocaleSpecific) {
  const basic::TimeZone tz = basic::UTCTimeZone();
  basic::Time t = basic::FromTimeT(0);

  TestFormatSpecifier(t, tz, "%a", "Thu");
  TestFormatSpecifier(t, tz, "%A", "Thursday");
  TestFormatSpecifier(t, tz, "%b", "Jan");
  TestFormatSpecifier(t, tz, "%B", "January");

  // %c should at least produce the numeric year and time-of-day.
  const std::string s =
      basic::FormatTime("%c", basic::FromTimeT(0), basic::UTCTimeZone());
  EXPECT_THAT(s, HasSubstr("1970"));
  EXPECT_THAT(s, HasSubstr("00:00:00"));

  TestFormatSpecifier(t, tz, "%p", "AM");
  TestFormatSpecifier(t, tz, "%x", "01/01/70");
  TestFormatSpecifier(t, tz, "%X", "00:00:00");
}

TEST(FormatTime, ExtendedSeconds) {
  const basic::TimeZone tz = basic::UTCTimeZone();

  // No subseconds.
  basic::Time t = basic::FromTimeT(0) + basic::Seconds(5);
  EXPECT_EQ("05", basic::FormatTime("%E*S", t, tz));
  EXPECT_EQ("05.000000000000000", basic::FormatTime("%E15S", t, tz));

  // With subseconds.
  t += basic::Milliseconds(6) + basic::Microseconds(7) + basic::Nanoseconds(8);
  EXPECT_EQ("05.006007008", basic::FormatTime("%E*S", t, tz));
  EXPECT_EQ("05", basic::FormatTime("%E0S", t, tz));
  EXPECT_EQ("05.006007008000000", basic::FormatTime("%E15S", t, tz));

  // Times before the Unix epoch.
  t = basic::FromUnixMicros(-1);
  EXPECT_EQ("1969-12-31 23:59:59.999999",
            basic::FormatTime("%Y-%m-%d %H:%M:%E*S", t, tz));

  // Here is a "%E*S" case we got wrong for a while.  While the first
  // instant below is correctly rendered as "...:07.333304", the second
  // one used to appear as "...:07.33330499999999999".
  t = basic::FromUnixMicros(1395024427333304);
  EXPECT_EQ("2014-03-17 02:47:07.333304",
            basic::FormatTime("%Y-%m-%d %H:%M:%E*S", t, tz));
  t += basic::Microseconds(1);
  EXPECT_EQ("2014-03-17 02:47:07.333305",
            basic::FormatTime("%Y-%m-%d %H:%M:%E*S", t, tz));
}

TEST(FormatTime, RFC1123FormatPadsYear) {  // locale specific
  basic::TimeZone tz = basic::UTCTimeZone();

  // A year of 77 should be padded to 0077.
  basic::Time t = basic::FromCivil(basic::CivilSecond(77, 6, 28, 9, 8, 7), tz);
  EXPECT_EQ("Mon, 28 Jun 0077 09:08:07 +0000",
            basic::FormatTime(basic::RFC1123_full, t, tz));
  EXPECT_EQ("28 Jun 0077 09:08:07 +0000",
            basic::FormatTime(basic::RFC1123_no_wday, t, tz));
}

TEST(FormatTime, InfiniteTime) {
  basic::TimeZone tz = basic::time_internal::LoadTimeZone("America/Los_Angeles");

  // The format and timezone are ignored.
  EXPECT_EQ("infinite-future",
            basic::FormatTime("%H:%M blah", basic::InfiniteFuture(), tz));
  EXPECT_EQ("infinite-past",
            basic::FormatTime("%H:%M blah", basic::InfinitePast(), tz));
}

//
// Testing ParseTime()
//

TEST(ParseTime, Basics) {
  basic::Time t = basic::FromTimeT(1234567890);
  std::string err;

  // Simple edge cases.
  EXPECT_TRUE(basic::ParseTime("", "", &t, &err)) << err;
  EXPECT_EQ(basic::UnixEpoch(), t);  // everything defaulted
  EXPECT_TRUE(basic::ParseTime(" ", " ", &t, &err)) << err;
  EXPECT_TRUE(basic::ParseTime("  ", "  ", &t, &err)) << err;
  EXPECT_TRUE(basic::ParseTime("x", "x", &t, &err)) << err;
  EXPECT_TRUE(basic::ParseTime("xxx", "xxx", &t, &err)) << err;

  EXPECT_TRUE(basic::ParseTime("%Y-%m-%d %H:%M:%S %z",
                              "2013-06-28 19:08:09 -0800", &t, &err))
      << err;
  const auto ci = basic::FixedTimeZone(-8 * 60 * 60).At(t);
  EXPECT_EQ(basic::CivilSecond(2013, 6, 28, 19, 8, 9), ci.cs);
  EXPECT_EQ(basic::ZeroDuration(), ci.subsecond);
}

TEST(ParseTime, NullErrorString) {
  basic::Time t;
  EXPECT_FALSE(basic::ParseTime("%Q", "invalid format", &t, nullptr));
  EXPECT_FALSE(basic::ParseTime("%H", "12 trailing data", &t, nullptr));
  EXPECT_FALSE(
      basic::ParseTime("%H out of range", "42 out of range", &t, nullptr));
}

TEST(ParseTime, WithTimeZone) {
  const basic::TimeZone tz =
      basic::time_internal::LoadTimeZone("America/Los_Angeles");
  basic::Time t;
  std::string e;

  // We can parse a std::string without a UTC offset if we supply a timezone.
  EXPECT_TRUE(
      basic::ParseTime("%Y-%m-%d %H:%M:%S", "2013-06-28 19:08:09", tz, &t, &e))
      << e;
  auto ci = tz.At(t);
  EXPECT_EQ(basic::CivilSecond(2013, 6, 28, 19, 8, 9), ci.cs);
  EXPECT_EQ(basic::ZeroDuration(), ci.subsecond);

  // But the timezone is ignored when a UTC offset is present.
  EXPECT_TRUE(basic::ParseTime("%Y-%m-%d %H:%M:%S %z",
                              "2013-06-28 19:08:09 +0800", tz, &t, &e))
      << e;
  ci = basic::FixedTimeZone(8 * 60 * 60).At(t);
  EXPECT_EQ(basic::CivilSecond(2013, 6, 28, 19, 8, 9), ci.cs);
  EXPECT_EQ(basic::ZeroDuration(), ci.subsecond);
}

TEST(ParseTime, ErrorCases) {
  basic::Time t = basic::FromTimeT(0);
  std::string err;

  EXPECT_FALSE(basic::ParseTime("%S", "123", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

  // Can't parse an illegal format specifier.
  err.clear();
  EXPECT_FALSE(basic::ParseTime("%Q", "x", &t, &err)) << err;
  // Exact contents of "err" are platform-dependent because of
  // differences in the strptime implementation between OSX and Linux.
  EXPECT_FALSE(err.empty());

  // Fails because of trailing, unparsed data "blah".
  EXPECT_FALSE(basic::ParseTime("%m-%d", "2-3 blah", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

  // Feb 31 requires normalization.
  EXPECT_FALSE(basic::ParseTime("%m-%d", "2-31", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Out-of-range"));

  // Check that we cannot have spaces in UTC offsets.
  EXPECT_TRUE(basic::ParseTime("%z", "-0203", &t, &err)) << err;
  EXPECT_FALSE(basic::ParseTime("%z", "- 2 3", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
  EXPECT_TRUE(basic::ParseTime("%Ez", "-02:03", &t, &err)) << err;
  EXPECT_FALSE(basic::ParseTime("%Ez", "- 2: 3", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));

  // Check that we reject other malformed UTC offsets.
  EXPECT_FALSE(basic::ParseTime("%Ez", "+-08:00", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
  EXPECT_FALSE(basic::ParseTime("%Ez", "-+08:00", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));

  // Check that we do not accept "-0" in fields that allow zero.
  EXPECT_FALSE(basic::ParseTime("%Y", "-0", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
  EXPECT_FALSE(basic::ParseTime("%E4Y", "-0", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
  EXPECT_FALSE(basic::ParseTime("%H", "-0", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
  EXPECT_FALSE(basic::ParseTime("%M", "-0", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
  EXPECT_FALSE(basic::ParseTime("%S", "-0", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
  EXPECT_FALSE(basic::ParseTime("%z", "+-000", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
  EXPECT_FALSE(basic::ParseTime("%Ez", "+-0:00", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
  EXPECT_FALSE(basic::ParseTime("%z", "-00-0", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Illegal trailing data"));
  EXPECT_FALSE(basic::ParseTime("%Ez", "-00:-0", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Illegal trailing data"));
}

TEST(ParseTime, ExtendedSeconds) {
  std::string err;
  basic::Time t;

  // Here is a "%E*S" case we got wrong for a while.  The fractional
  // part of the first instant is less than 2^31 and was correctly
  // parsed, while the second (and any subsecond field >=2^31) failed.
  t = basic::UnixEpoch();
  EXPECT_TRUE(basic::ParseTime("%E*S", "0.2147483647", &t, &err)) << err;
  EXPECT_EQ(basic::UnixEpoch() + basic::Nanoseconds(214748364) +
                basic::Nanoseconds(1) / 2,
            t);
  t = basic::UnixEpoch();
  EXPECT_TRUE(basic::ParseTime("%E*S", "0.2147483648", &t, &err)) << err;
  EXPECT_EQ(basic::UnixEpoch() + basic::Nanoseconds(214748364) +
                basic::Nanoseconds(3) / 4,
            t);

  // We should also be able to specify long strings of digits far
  // beyond the current resolution and have them convert the same way.
  t = basic::UnixEpoch();
  EXPECT_TRUE(basic::ParseTime(
      "%E*S", "0.214748364801234567890123456789012345678901234567890123456789",
      &t, &err))
      << err;
  EXPECT_EQ(basic::UnixEpoch() + basic::Nanoseconds(214748364) +
                basic::Nanoseconds(3) / 4,
            t);
}

TEST(ParseTime, ExtendedOffsetErrors) {
  std::string err;
  basic::Time t;

  // %z against +-HHMM.
  EXPECT_FALSE(basic::ParseTime("%z", "-123", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

  // %z against +-HH.
  EXPECT_FALSE(basic::ParseTime("%z", "-1", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));

  // %Ez against +-HH:MM.
  EXPECT_FALSE(basic::ParseTime("%Ez", "-12:3", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

  // %Ez against +-HHMM.
  EXPECT_FALSE(basic::ParseTime("%Ez", "-123", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Illegal trailing data"));

  // %Ez against +-HH.
  EXPECT_FALSE(basic::ParseTime("%Ez", "-1", &t, &err)) << err;
  EXPECT_THAT(err, HasSubstr("Failed to parse"));
}

TEST(ParseTime, InfiniteTime) {
  basic::Time t;
  std::string err;
  EXPECT_TRUE(basic::ParseTime("%H:%M blah", "infinite-future", &t, &err));
  EXPECT_EQ(basic::InfiniteFuture(), t);

  // Surrounding whitespace.
  EXPECT_TRUE(basic::ParseTime("%H:%M blah", "  infinite-future", &t, &err));
  EXPECT_EQ(basic::InfiniteFuture(), t);
  EXPECT_TRUE(basic::ParseTime("%H:%M blah", "infinite-future  ", &t, &err));
  EXPECT_EQ(basic::InfiniteFuture(), t);
  EXPECT_TRUE(basic::ParseTime("%H:%M blah", "  infinite-future  ", &t, &err));
  EXPECT_EQ(basic::InfiniteFuture(), t);

  EXPECT_TRUE(basic::ParseTime("%H:%M blah", "infinite-past", &t, &err));
  EXPECT_EQ(basic::InfinitePast(), t);

  // Surrounding whitespace.
  EXPECT_TRUE(basic::ParseTime("%H:%M blah", "  infinite-past", &t, &err));
  EXPECT_EQ(basic::InfinitePast(), t);
  EXPECT_TRUE(basic::ParseTime("%H:%M blah", "infinite-past  ", &t, &err));
  EXPECT_EQ(basic::InfinitePast(), t);
  EXPECT_TRUE(basic::ParseTime("%H:%M blah", "  infinite-past  ", &t, &err));
  EXPECT_EQ(basic::InfinitePast(), t);

  // "infinite-future" as literal std::string
  basic::TimeZone tz = basic::UTCTimeZone();
  EXPECT_TRUE(basic::ParseTime("infinite-future %H:%M", "infinite-future 03:04",
                              &t, &err));
  EXPECT_NE(basic::InfiniteFuture(), t);
  EXPECT_EQ(3, tz.At(t).cs.hour());
  EXPECT_EQ(4, tz.At(t).cs.minute());

  // "infinite-past" as literal std::string
  EXPECT_TRUE(
      basic::ParseTime("infinite-past %H:%M", "infinite-past 03:04", &t, &err));
  EXPECT_NE(basic::InfinitePast(), t);
  EXPECT_EQ(3, tz.At(t).cs.hour());
  EXPECT_EQ(4, tz.At(t).cs.minute());

  // The input doesn't match the format.
  EXPECT_FALSE(basic::ParseTime("infinite-future %H:%M", "03:04", &t, &err));
  EXPECT_FALSE(basic::ParseTime("infinite-past %H:%M", "03:04", &t, &err));
}

TEST(ParseTime, FailsOnUnrepresentableTime) {
  const basic::TimeZone utc = basic::UTCTimeZone();
  basic::Time t;
  EXPECT_FALSE(
      basic::ParseTime("%Y-%m-%d", "-292277022657-01-27", utc, &t, nullptr));
  EXPECT_TRUE(
      basic::ParseTime("%Y-%m-%d", "-292277022657-01-28", utc, &t, nullptr));
  EXPECT_TRUE(
      basic::ParseTime("%Y-%m-%d", "292277026596-12-04", utc, &t, nullptr));
  EXPECT_FALSE(
      basic::ParseTime("%Y-%m-%d", "292277026596-12-05", utc, &t, nullptr));
}

//
// Roundtrip test for FormatTime()/ParseTime().
//

TEST(FormatParse, RoundTrip) {
  const basic::TimeZone lax =
      basic::time_internal::LoadTimeZone("America/Los_Angeles");
  const basic::Time in =
      basic::FromCivil(basic::CivilSecond(1977, 6, 28, 9, 8, 7), lax);
  const basic::Duration subseconds = basic::Nanoseconds(654321);
  std::string err;

  // RFC3339, which renders subseconds.
  {
    basic::Time out;
    const std::string s =
        basic::FormatTime(basic::RFC3339_full, in + subseconds, lax);
    EXPECT_TRUE(basic::ParseTime(basic::RFC3339_full, s, &out, &err))
        << s << ": " << err;
    EXPECT_EQ(in + subseconds, out);  // RFC3339_full includes %Ez
  }

  // RFC1123, which only does whole seconds.
  {
    basic::Time out;
    const std::string s = basic::FormatTime(basic::RFC1123_full, in, lax);
    EXPECT_TRUE(basic::ParseTime(basic::RFC1123_full, s, &out, &err))
        << s << ": " << err;
    EXPECT_EQ(in, out);  // RFC1123_full includes %z
  }

  // `basic::FormatTime()` falls back to strftime() for "%c", which appears to
  // work. On Windows, `basic::ParseTime()` falls back to std::get_time() which
  // appears to fail on "%c" (or at least on the "%c" text produced by
  // `strftime()`). This makes it fail the round-trip test.
  //
  // Under the emscripten compiler `basic::ParseTime() falls back to
  // `strptime()`, but that ends up using a different definition for "%c"
  // compared to `strftime()`, also causing the round-trip test to fail
  // (see https://github.com/kripken/emscripten/pull/7491).
#if !defined(_MSC_VER) && !defined(__EMSCRIPTEN__)
  // Even though we don't know what %c will produce, it should roundtrip,
  // but only in the 0-offset timezone.
  {
    basic::Time out;
    const std::string s = basic::FormatTime("%c", in, basic::UTCTimeZone());
    EXPECT_TRUE(basic::ParseTime("%c", s, &out, &err)) << s << ": " << err;
    EXPECT_EQ(in, out);
  }
#endif  // !_MSC_VER && !__EMSCRIPTEN__
}

TEST(FormatParse, RoundTripDistantFuture) {
  const basic::TimeZone tz = basic::UTCTimeZone();
  const basic::Time in =
      basic::FromUnixSeconds(std::numeric_limits<int64_t>::max());
  std::string err;

  basic::Time out;
  const std::string s = basic::FormatTime(basic::RFC3339_full, in, tz);
  EXPECT_TRUE(basic::ParseTime(basic::RFC3339_full, s, &out, &err))
      << s << ": " << err;
  EXPECT_EQ(in, out);
}

TEST(FormatParse, RoundTripDistantPast) {
  const basic::TimeZone tz = basic::UTCTimeZone();
  const basic::Time in =
      basic::FromUnixSeconds(std::numeric_limits<int64_t>::min());
  std::string err;

  basic::Time out;
  const std::string s = basic::FormatTime(basic::RFC3339_full, in, tz);
  EXPECT_TRUE(basic::ParseTime(basic::RFC3339_full, s, &out, &err))
      << s << ": " << err;
  EXPECT_EQ(in, out);
}

}  // namespace
