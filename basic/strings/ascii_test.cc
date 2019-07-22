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

#include "basic/strings/ascii.h"

#include <cctype>
#include <clocale>
#include <cstring>
#include <string>

#include "gtest/gtest.h"
#include "basic/base/macros.h"
#include "basic/base/port.h"

namespace {

TEST(AsciiIsFoo, All) {
  for (int i = 0; i < 256; i++) {
    if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z'))
      EXPECT_TRUE(basic::ascii_isalpha(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_isalpha(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if ((i >= '0' && i <= '9'))
      EXPECT_TRUE(basic::ascii_isdigit(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_isdigit(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (basic::ascii_isalpha(i) || basic::ascii_isdigit(i))
      EXPECT_TRUE(basic::ascii_isalnum(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_isalnum(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i != '\0' && strchr(" \r\n\t\v\f", i))
      EXPECT_TRUE(basic::ascii_isspace(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_isspace(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i >= 32 && i < 127)
      EXPECT_TRUE(basic::ascii_isprint(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_isprint(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (basic::ascii_isprint(i) && !basic::ascii_isspace(i) &&
        !basic::ascii_isalnum(i))
      EXPECT_TRUE(basic::ascii_ispunct(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_ispunct(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i == ' ' || i == '\t')
      EXPECT_TRUE(basic::ascii_isblank(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_isblank(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i < 32 || i == 127)
      EXPECT_TRUE(basic::ascii_iscntrl(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_iscntrl(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (basic::ascii_isdigit(i) || (i >= 'A' && i <= 'F') ||
        (i >= 'a' && i <= 'f'))
      EXPECT_TRUE(basic::ascii_isxdigit(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_isxdigit(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i > 32 && i < 127)
      EXPECT_TRUE(basic::ascii_isgraph(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_isgraph(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i >= 'A' && i <= 'Z')
      EXPECT_TRUE(basic::ascii_isupper(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_isupper(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 256; i++) {
    if (i >= 'a' && i <= 'z')
      EXPECT_TRUE(basic::ascii_islower(i)) << ": failed on " << i;
    else
      EXPECT_TRUE(!basic::ascii_islower(i)) << ": failed on " << i;
  }
  for (int i = 0; i < 128; i++) {
    EXPECT_TRUE(basic::ascii_isascii(i)) << ": failed on " << i;
  }
  for (int i = 128; i < 256; i++) {
    EXPECT_TRUE(!basic::ascii_isascii(i)) << ": failed on " << i;
  }

  // The official is* functions don't accept negative signed chars, but
  // our basic::ascii_is* functions do.
  for (int i = 0; i < 256; i++) {
    signed char sc = static_cast<signed char>(static_cast<unsigned char>(i));
    EXPECT_EQ(basic::ascii_isalpha(i), basic::ascii_isalpha(sc)) << i;
    EXPECT_EQ(basic::ascii_isdigit(i), basic::ascii_isdigit(sc)) << i;
    EXPECT_EQ(basic::ascii_isalnum(i), basic::ascii_isalnum(sc)) << i;
    EXPECT_EQ(basic::ascii_isspace(i), basic::ascii_isspace(sc)) << i;
    EXPECT_EQ(basic::ascii_ispunct(i), basic::ascii_ispunct(sc)) << i;
    EXPECT_EQ(basic::ascii_isblank(i), basic::ascii_isblank(sc)) << i;
    EXPECT_EQ(basic::ascii_iscntrl(i), basic::ascii_iscntrl(sc)) << i;
    EXPECT_EQ(basic::ascii_isxdigit(i), basic::ascii_isxdigit(sc)) << i;
    EXPECT_EQ(basic::ascii_isprint(i), basic::ascii_isprint(sc)) << i;
    EXPECT_EQ(basic::ascii_isgraph(i), basic::ascii_isgraph(sc)) << i;
    EXPECT_EQ(basic::ascii_isupper(i), basic::ascii_isupper(sc)) << i;
    EXPECT_EQ(basic::ascii_islower(i), basic::ascii_islower(sc)) << i;
    EXPECT_EQ(basic::ascii_isascii(i), basic::ascii_isascii(sc)) << i;
  }
}

// Checks that basic::ascii_isfoo returns the same value as isfoo in the C
// locale.
TEST(AsciiIsFoo, SameAsIsFoo) {
#ifndef __ANDROID__
  // temporarily change locale to C. It should already be C, but just for safety
  const char* old_locale = setlocale(LC_CTYPE, "C");
  ASSERT_TRUE(old_locale != nullptr);
#endif

  for (int i = 0; i < 256; i++) {
    EXPECT_EQ(isalpha(i) != 0, basic::ascii_isalpha(i)) << i;
    EXPECT_EQ(isdigit(i) != 0, basic::ascii_isdigit(i)) << i;
    EXPECT_EQ(isalnum(i) != 0, basic::ascii_isalnum(i)) << i;
    EXPECT_EQ(isspace(i) != 0, basic::ascii_isspace(i)) << i;
    EXPECT_EQ(ispunct(i) != 0, basic::ascii_ispunct(i)) << i;
    EXPECT_EQ(isblank(i) != 0, basic::ascii_isblank(i)) << i;
    EXPECT_EQ(iscntrl(i) != 0, basic::ascii_iscntrl(i)) << i;
    EXPECT_EQ(isxdigit(i) != 0, basic::ascii_isxdigit(i)) << i;
    EXPECT_EQ(isprint(i) != 0, basic::ascii_isprint(i)) << i;
    EXPECT_EQ(isgraph(i) != 0, basic::ascii_isgraph(i)) << i;
    EXPECT_EQ(isupper(i) != 0, basic::ascii_isupper(i)) << i;
    EXPECT_EQ(islower(i) != 0, basic::ascii_islower(i)) << i;
    EXPECT_EQ(isascii(i) != 0, basic::ascii_isascii(i)) << i;
  }

#ifndef __ANDROID__
  // restore the old locale.
  ASSERT_TRUE(setlocale(LC_CTYPE, old_locale));
#endif
}

TEST(AsciiToFoo, All) {
#ifndef __ANDROID__
  // temporarily change locale to C. It should already be C, but just for safety
  const char* old_locale = setlocale(LC_CTYPE, "C");
  ASSERT_TRUE(old_locale != nullptr);
#endif

  for (int i = 0; i < 256; i++) {
    if (basic::ascii_islower(i))
      EXPECT_EQ(basic::ascii_toupper(i), 'A' + (i - 'a')) << i;
    else
      EXPECT_EQ(basic::ascii_toupper(i), static_cast<char>(i)) << i;

    if (basic::ascii_isupper(i))
      EXPECT_EQ(basic::ascii_tolower(i), 'a' + (i - 'A')) << i;
    else
      EXPECT_EQ(basic::ascii_tolower(i), static_cast<char>(i)) << i;

    // These CHECKs only hold in a C locale.
    EXPECT_EQ(static_cast<char>(tolower(i)), basic::ascii_tolower(i)) << i;
    EXPECT_EQ(static_cast<char>(toupper(i)), basic::ascii_toupper(i)) << i;

    // The official to* functions don't accept negative signed chars, but
    // our basic::ascii_to* functions do.
    signed char sc = static_cast<signed char>(static_cast<unsigned char>(i));
    EXPECT_EQ(basic::ascii_tolower(i), basic::ascii_tolower(sc)) << i;
    EXPECT_EQ(basic::ascii_toupper(i), basic::ascii_toupper(sc)) << i;
  }
#ifndef __ANDROID__
  // restore the old locale.
  ASSERT_TRUE(setlocale(LC_CTYPE, old_locale));
#endif
}

TEST(AsciiStrTo, Lower) {
  const char buf[] = "ABCDEF";
  const std::string str("GHIJKL");
  const std::string str2("MNOPQR");
  const basic::string_view sp(str2);

  EXPECT_EQ("abcdef", basic::AsciiStrToLower(buf));
  EXPECT_EQ("ghijkl", basic::AsciiStrToLower(str));
  EXPECT_EQ("mnopqr", basic::AsciiStrToLower(sp));

  char mutable_buf[] = "Mutable";
  std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                 mutable_buf, basic::ascii_tolower);
  EXPECT_STREQ("mutable", mutable_buf);
}

TEST(AsciiStrTo, Upper) {
  const char buf[] = "abcdef";
  const std::string str("ghijkl");
  const std::string str2("mnopqr");
  const basic::string_view sp(str2);

  EXPECT_EQ("ABCDEF", basic::AsciiStrToUpper(buf));
  EXPECT_EQ("GHIJKL", basic::AsciiStrToUpper(str));
  EXPECT_EQ("MNOPQR", basic::AsciiStrToUpper(sp));

  char mutable_buf[] = "Mutable";
  std::transform(mutable_buf, mutable_buf + strlen(mutable_buf),
                 mutable_buf, basic::ascii_toupper);
  EXPECT_STREQ("MUTABLE", mutable_buf);
}

TEST(StripLeadingAsciiWhitespace, FromStringView) {
  EXPECT_EQ(basic::string_view{},
            basic::StripLeadingAsciiWhitespace(basic::string_view{}));
  EXPECT_EQ("foo", basic::StripLeadingAsciiWhitespace({"foo"}));
  EXPECT_EQ("foo", basic::StripLeadingAsciiWhitespace({"\t  \n\f\r\n\vfoo"}));
  EXPECT_EQ("foo foo\n ",
            basic::StripLeadingAsciiWhitespace({"\t  \n\f\r\n\vfoo foo\n "}));
  EXPECT_EQ(basic::string_view{}, basic::StripLeadingAsciiWhitespace(
                                     {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(StripLeadingAsciiWhitespace, InPlace) {
  std::string str;

  basic::StripLeadingAsciiWhitespace(&str);
  EXPECT_EQ("", str);

  str = "foo";
  basic::StripLeadingAsciiWhitespace(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo";
  basic::StripLeadingAsciiWhitespace(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo foo\n ";
  basic::StripLeadingAsciiWhitespace(&str);
  EXPECT_EQ("foo foo\n ", str);

  str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  basic::StripLeadingAsciiWhitespace(&str);
  EXPECT_EQ(basic::string_view{}, str);
}

TEST(StripTrailingAsciiWhitespace, FromStringView) {
  EXPECT_EQ(basic::string_view{},
            basic::StripTrailingAsciiWhitespace(basic::string_view{}));
  EXPECT_EQ("foo", basic::StripTrailingAsciiWhitespace({"foo"}));
  EXPECT_EQ("foo", basic::StripTrailingAsciiWhitespace({"foo\t  \n\f\r\n\v"}));
  EXPECT_EQ(" \nfoo foo",
            basic::StripTrailingAsciiWhitespace({" \nfoo foo\t  \n\f\r\n\v"}));
  EXPECT_EQ(basic::string_view{}, basic::StripTrailingAsciiWhitespace(
                                     {"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(StripTrailingAsciiWhitespace, InPlace) {
  std::string str;

  basic::StripTrailingAsciiWhitespace(&str);
  EXPECT_EQ("", str);

  str = "foo";
  basic::StripTrailingAsciiWhitespace(&str);
  EXPECT_EQ("foo", str);

  str = "foo\t  \n\f\r\n\v";
  basic::StripTrailingAsciiWhitespace(&str);
  EXPECT_EQ("foo", str);

  str = " \nfoo foo\t  \n\f\r\n\v";
  basic::StripTrailingAsciiWhitespace(&str);
  EXPECT_EQ(" \nfoo foo", str);

  str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  basic::StripTrailingAsciiWhitespace(&str);
  EXPECT_EQ(basic::string_view{}, str);
}

TEST(StripAsciiWhitespace, FromStringView) {
  EXPECT_EQ(basic::string_view{},
            basic::StripAsciiWhitespace(basic::string_view{}));
  EXPECT_EQ("foo", basic::StripAsciiWhitespace({"foo"}));
  EXPECT_EQ("foo",
            basic::StripAsciiWhitespace({"\t  \n\f\r\n\vfoo\t  \n\f\r\n\v"}));
  EXPECT_EQ("foo foo", basic::StripAsciiWhitespace(
                           {"\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v"}));
  EXPECT_EQ(basic::string_view{},
            basic::StripAsciiWhitespace({"\t  \n\f\r\v\n\t  \n\f\r\v\n"}));
}

TEST(StripAsciiWhitespace, InPlace) {
  std::string str;

  basic::StripAsciiWhitespace(&str);
  EXPECT_EQ("", str);

  str = "foo";
  basic::StripAsciiWhitespace(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo\t  \n\f\r\n\v";
  basic::StripAsciiWhitespace(&str);
  EXPECT_EQ("foo", str);

  str = "\t  \n\f\r\n\vfoo foo\t  \n\f\r\n\v";
  basic::StripAsciiWhitespace(&str);
  EXPECT_EQ("foo foo", str);

  str = "\t  \n\f\r\v\n\t  \n\f\r\v\n";
  basic::StripAsciiWhitespace(&str);
  EXPECT_EQ(basic::string_view{}, str);
}

TEST(RemoveExtraAsciiWhitespace, InPlace) {
  const char* inputs[] = {"No extra space",
                          "  Leading whitespace",
                          "Trailing whitespace  ",
                          "  Leading and trailing  ",
                          " Whitespace \t  in\v   middle  ",
                          "'Eeeeep!  \n Newlines!\n",
                          "nospaces",
                          "",
                          "\n\t a\t\n\nb \t\n"};

  const char* outputs[] = {
      "No extra space",
      "Leading whitespace",
      "Trailing whitespace",
      "Leading and trailing",
      "Whitespace in middle",
      "'Eeeeep! Newlines!",
      "nospaces",
      "",
      "a\nb",
  };
  const int NUM_TESTS = ABSL_ARRAYSIZE(inputs);

  for (int i = 0; i < NUM_TESTS; i++) {
    std::string s(inputs[i]);
    basic::RemoveExtraAsciiWhitespace(&s);
    EXPECT_EQ(outputs[i], s);
  }
}

}  // namespace
