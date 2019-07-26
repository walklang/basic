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

#include "basic/strings/match.h"

#include "gtest/gtest.h"

namespace {

TEST(MatchTest, StartsWith) {
  const std::string s1("123\0abc", 7);
  const basic::string_view a("foobar");
  const basic::string_view b(s1);
  const basic::string_view e;
  EXPECT_TRUE(basic::StartsWith(a, a));
  EXPECT_TRUE(basic::StartsWith(a, "foo"));
  EXPECT_TRUE(basic::StartsWith(a, e));
  EXPECT_TRUE(basic::StartsWith(b, s1));
  EXPECT_TRUE(basic::StartsWith(b, b));
  EXPECT_TRUE(basic::StartsWith(b, e));
  EXPECT_TRUE(basic::StartsWith(e, ""));
  EXPECT_FALSE(basic::StartsWith(a, b));
  EXPECT_FALSE(basic::StartsWith(b, a));
  EXPECT_FALSE(basic::StartsWith(e, a));
}

TEST(MatchTest, EndsWith) {
  const std::string s1("123\0abc", 7);
  const basic::string_view a("foobar");
  const basic::string_view b(s1);
  const basic::string_view e;
  EXPECT_TRUE(basic::EndsWith(a, a));
  EXPECT_TRUE(basic::EndsWith(a, "bar"));
  EXPECT_TRUE(basic::EndsWith(a, e));
  EXPECT_TRUE(basic::EndsWith(b, s1));
  EXPECT_TRUE(basic::EndsWith(b, b));
  EXPECT_TRUE(basic::EndsWith(b, e));
  EXPECT_TRUE(basic::EndsWith(e, ""));
  EXPECT_FALSE(basic::EndsWith(a, b));
  EXPECT_FALSE(basic::EndsWith(b, a));
  EXPECT_FALSE(basic::EndsWith(e, a));
}

TEST(MatchTest, Contains) {
  basic::string_view a("abcdefg");
  basic::string_view b("abcd");
  basic::string_view c("efg");
  basic::string_view d("gh");
  EXPECT_TRUE(basic::StrContains(a, a));
  EXPECT_TRUE(basic::StrContains(a, b));
  EXPECT_TRUE(basic::StrContains(a, c));
  EXPECT_FALSE(basic::StrContains(a, d));
  EXPECT_TRUE(basic::StrContains("", ""));
  EXPECT_TRUE(basic::StrContains("abc", ""));
  EXPECT_FALSE(basic::StrContains("", "a"));
}

TEST(MatchTest, ContainsNull) {
  const std::string s = "foo";
  const char* cs = "foo";
  const basic::string_view sv("foo");
  const basic::string_view sv2("foo\0bar", 4);
  EXPECT_EQ(s, "foo");
  EXPECT_EQ(sv, "foo");
  EXPECT_NE(sv2, "foo");
  EXPECT_TRUE(basic::EndsWith(s, sv));
  EXPECT_TRUE(basic::StartsWith(cs, sv));
  EXPECT_TRUE(basic::StrContains(cs, sv));
  EXPECT_FALSE(basic::StrContains(cs, sv2));
}

TEST(MatchTest, EqualsIgnoreCase) {
  std::string text = "the";
  basic::string_view data(text);

  EXPECT_TRUE(basic::EqualsIgnoreCase(data, "The"));
  EXPECT_TRUE(basic::EqualsIgnoreCase(data, "THE"));
  EXPECT_TRUE(basic::EqualsIgnoreCase(data, "the"));
  EXPECT_FALSE(basic::EqualsIgnoreCase(data, "Quick"));
  EXPECT_FALSE(basic::EqualsIgnoreCase(data, "then"));
}

TEST(MatchTest, StartsWithIgnoreCase) {
  EXPECT_TRUE(basic::StartsWithIgnoreCase("foo", "foo"));
  EXPECT_TRUE(basic::StartsWithIgnoreCase("foo", "Fo"));
  EXPECT_TRUE(basic::StartsWithIgnoreCase("foo", ""));
  EXPECT_FALSE(basic::StartsWithIgnoreCase("foo", "fooo"));
  EXPECT_FALSE(basic::StartsWithIgnoreCase("", "fo"));
}

TEST(MatchTest, EndsWithIgnoreCase) {
  EXPECT_TRUE(basic::EndsWithIgnoreCase("foo", "foo"));
  EXPECT_TRUE(basic::EndsWithIgnoreCase("foo", "Oo"));
  EXPECT_TRUE(basic::EndsWithIgnoreCase("foo", ""));
  EXPECT_FALSE(basic::EndsWithIgnoreCase("foo", "fooo"));
  EXPECT_FALSE(basic::EndsWithIgnoreCase("", "fo"));
}

}  // namespace
