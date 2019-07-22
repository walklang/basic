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

#include "basic/numeric/int128.h"

#include <algorithm>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "basic/base/internal/cycleclock.h"
#include "basic/hash/hash_testing.h"
#include "basic/meta/type_traits.h"

#if defined(_MSC_VER) && _MSC_VER == 1900
// Disable "unary minus operator applied to unsigned type" warnings in Microsoft
// Visual C++ 14 (2015).
#pragma warning(disable:4146)
#endif

namespace {

template <typename T>
class Uint128IntegerTraitsTest : public ::testing::Test {};
typedef ::testing::Types<bool, char, signed char, unsigned char, char16_t,
                         char32_t, wchar_t,
                         short,           // NOLINT(runtime/int)
                         unsigned short,  // NOLINT(runtime/int)
                         int, unsigned int,
                         long,                // NOLINT(runtime/int)
                         unsigned long,       // NOLINT(runtime/int)
                         long long,           // NOLINT(runtime/int)
                         unsigned long long>  // NOLINT(runtime/int)
    IntegerTypes;

template <typename T>
class Uint128FloatTraitsTest : public ::testing::Test {};
typedef ::testing::Types<float, double, long double> FloatingPointTypes;

TYPED_TEST_SUITE(Uint128IntegerTraitsTest, IntegerTypes);

TYPED_TEST(Uint128IntegerTraitsTest, ConstructAssignTest) {
  static_assert(std::is_constructible<basic::uint128, TypeParam>::value,
                "basic::uint128 must be constructible from TypeParam");
  static_assert(std::is_assignable<basic::uint128&, TypeParam>::value,
                "basic::uint128 must be assignable from TypeParam");
  static_assert(!std::is_assignable<TypeParam&, basic::uint128>::value,
                "TypeParam must not be assignable from basic::uint128");
}

TYPED_TEST_SUITE(Uint128FloatTraitsTest, FloatingPointTypes);

TYPED_TEST(Uint128FloatTraitsTest, ConstructAssignTest) {
  static_assert(std::is_constructible<basic::uint128, TypeParam>::value,
                "basic::uint128 must be constructible from TypeParam");
  static_assert(!std::is_assignable<basic::uint128&, TypeParam>::value,
                "basic::uint128 must not be assignable from TypeParam");
  static_assert(!std::is_assignable<TypeParam&, basic::uint128>::value,
                "TypeParam must not be assignable from basic::uint128");
}

#ifdef ABSL_HAVE_INTRINSIC_INT128
// These type traits done separately as TYPED_TEST requires typeinfo, and not
// all platforms have this for __int128 even though they define the type.
TEST(Uint128, IntrinsicTypeTraitsTest) {
  static_assert(std::is_constructible<basic::uint128, __int128>::value,
                "basic::uint128 must be constructible from __int128");
  static_assert(std::is_assignable<basic::uint128&, __int128>::value,
                "basic::uint128 must be assignable from __int128");
  static_assert(!std::is_assignable<__int128&, basic::uint128>::value,
                "__int128 must not be assignable from basic::uint128");

  static_assert(std::is_constructible<basic::uint128, unsigned __int128>::value,
                "basic::uint128 must be constructible from unsigned __int128");
  static_assert(std::is_assignable<basic::uint128&, unsigned __int128>::value,
                "basic::uint128 must be assignable from unsigned __int128");
  static_assert(!std::is_assignable<unsigned __int128&, basic::uint128>::value,
                "unsigned __int128 must not be assignable from basic::uint128");
}
#endif  // ABSL_HAVE_INTRINSIC_INT128

TEST(Uint128, TrivialTraitsTest) {
  static_assert(basic::is_trivially_default_constructible<basic::uint128>::value,
                "");
  static_assert(basic::is_trivially_copy_constructible<basic::uint128>::value,
                "");
  static_assert(basic::is_trivially_copy_assignable<basic::uint128>::value, "");
  static_assert(std::is_trivially_destructible<basic::uint128>::value, "");
}

TEST(Uint128, AllTests) {
  basic::uint128 zero = 0;
  basic::uint128 one = 1;
  basic::uint128 one_2arg = basic::MakeUint128(0, 1);
  basic::uint128 two = 2;
  basic::uint128 three = 3;
  basic::uint128 big = basic::MakeUint128(2000, 2);
  basic::uint128 big_minus_one = basic::MakeUint128(2000, 1);
  basic::uint128 bigger = basic::MakeUint128(2001, 1);
  basic::uint128 biggest = basic::Uint128Max();
  basic::uint128 high_low = basic::MakeUint128(1, 0);
  basic::uint128 low_high =
      basic::MakeUint128(0, std::numeric_limits<uint64_t>::max());
  EXPECT_LT(one, two);
  EXPECT_GT(two, one);
  EXPECT_LT(one, big);
  EXPECT_LT(one, big);
  EXPECT_EQ(one, one_2arg);
  EXPECT_NE(one, two);
  EXPECT_GT(big, one);
  EXPECT_GE(big, two);
  EXPECT_GE(big, big_minus_one);
  EXPECT_GT(big, big_minus_one);
  EXPECT_LT(big_minus_one, big);
  EXPECT_LE(big_minus_one, big);
  EXPECT_NE(big_minus_one, big);
  EXPECT_LT(big, biggest);
  EXPECT_LE(big, biggest);
  EXPECT_GT(biggest, big);
  EXPECT_GE(biggest, big);
  EXPECT_EQ(big, ~~big);
  EXPECT_EQ(one, one | one);
  EXPECT_EQ(big, big | big);
  EXPECT_EQ(one, one | zero);
  EXPECT_EQ(one, one & one);
  EXPECT_EQ(big, big & big);
  EXPECT_EQ(zero, one & zero);
  EXPECT_EQ(zero, big & ~big);
  EXPECT_EQ(zero, one ^ one);
  EXPECT_EQ(zero, big ^ big);
  EXPECT_EQ(one, one ^ zero);

  // Shift operators.
  EXPECT_EQ(big, big << 0);
  EXPECT_EQ(big, big >> 0);
  EXPECT_GT(big << 1, big);
  EXPECT_LT(big >> 1, big);
  EXPECT_EQ(big, (big << 10) >> 10);
  EXPECT_EQ(big, (big >> 1) << 1);
  EXPECT_EQ(one, (one << 80) >> 80);
  EXPECT_EQ(zero, (one >> 80) << 80);

  // Shift assignments.
  basic::uint128 big_copy = big;
  EXPECT_EQ(big << 0, big_copy <<= 0);
  big_copy = big;
  EXPECT_EQ(big >> 0, big_copy >>= 0);
  big_copy = big;
  EXPECT_EQ(big << 1, big_copy <<= 1);
  big_copy = big;
  EXPECT_EQ(big >> 1, big_copy >>= 1);
  big_copy = big;
  EXPECT_EQ(big << 10, big_copy <<= 10);
  big_copy = big;
  EXPECT_EQ(big >> 10, big_copy >>= 10);
  big_copy = big;
  EXPECT_EQ(big << 64, big_copy <<= 64);
  big_copy = big;
  EXPECT_EQ(big >> 64, big_copy >>= 64);
  big_copy = big;
  EXPECT_EQ(big << 73, big_copy <<= 73);
  big_copy = big;
  EXPECT_EQ(big >> 73, big_copy >>= 73);

  EXPECT_EQ(basic::Uint128High64(biggest), std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(basic::Uint128Low64(biggest), std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(zero + one, one);
  EXPECT_EQ(one + one, two);
  EXPECT_EQ(big_minus_one + one, big);
  EXPECT_EQ(one - one, zero);
  EXPECT_EQ(one - zero, one);
  EXPECT_EQ(zero - one, biggest);
  EXPECT_EQ(big - big, zero);
  EXPECT_EQ(big - one, big_minus_one);
  EXPECT_EQ(big + std::numeric_limits<uint64_t>::max(), bigger);
  EXPECT_EQ(biggest + 1, zero);
  EXPECT_EQ(zero - 1, biggest);
  EXPECT_EQ(high_low - one, low_high);
  EXPECT_EQ(low_high + one, high_low);
  EXPECT_EQ(basic::Uint128High64((basic::uint128(1) << 64) - 1), 0);
  EXPECT_EQ(basic::Uint128Low64((basic::uint128(1) << 64) - 1),
            std::numeric_limits<uint64_t>::max());
  EXPECT_TRUE(!!one);
  EXPECT_TRUE(!!high_low);
  EXPECT_FALSE(!!zero);
  EXPECT_FALSE(!one);
  EXPECT_FALSE(!high_low);
  EXPECT_TRUE(!zero);
  EXPECT_TRUE(zero == 0);       // NOLINT(readability/check)
  EXPECT_FALSE(zero != 0);      // NOLINT(readability/check)
  EXPECT_FALSE(one == 0);       // NOLINT(readability/check)
  EXPECT_TRUE(one != 0);        // NOLINT(readability/check)
  EXPECT_FALSE(high_low == 0);  // NOLINT(readability/check)
  EXPECT_TRUE(high_low != 0);   // NOLINT(readability/check)

  basic::uint128 test = zero;
  EXPECT_EQ(++test, one);
  EXPECT_EQ(test, one);
  EXPECT_EQ(test++, one);
  EXPECT_EQ(test, two);
  EXPECT_EQ(test -= 2, zero);
  EXPECT_EQ(test, zero);
  EXPECT_EQ(test += 2, two);
  EXPECT_EQ(test, two);
  EXPECT_EQ(--test, one);
  EXPECT_EQ(test, one);
  EXPECT_EQ(test--, one);
  EXPECT_EQ(test, zero);
  EXPECT_EQ(test |= three, three);
  EXPECT_EQ(test &= one, one);
  EXPECT_EQ(test ^= three, two);
  EXPECT_EQ(test >>= 1, one);
  EXPECT_EQ(test <<= 1, two);

  EXPECT_EQ(big, -(-big));
  EXPECT_EQ(two, -((-one) - 1));
  EXPECT_EQ(basic::Uint128Max(), -one);
  EXPECT_EQ(zero, -zero);

  EXPECT_EQ(basic::Uint128Max(), basic::kuint128max);
}

TEST(Uint128, ConversionTests) {
  EXPECT_TRUE(basic::MakeUint128(1, 0));

#ifdef ABSL_HAVE_INTRINSIC_INT128
  unsigned __int128 intrinsic =
      (static_cast<unsigned __int128>(0x3a5b76c209de76f6) << 64) +
      0x1f25e1d63a2b46c5;
  basic::uint128 custom =
      basic::MakeUint128(0x3a5b76c209de76f6, 0x1f25e1d63a2b46c5);

  EXPECT_EQ(custom, basic::uint128(intrinsic));
  EXPECT_EQ(custom, basic::uint128(static_cast<__int128>(intrinsic)));
  EXPECT_EQ(intrinsic, static_cast<unsigned __int128>(custom));
  EXPECT_EQ(intrinsic, static_cast<__int128>(custom));
#endif  // ABSL_HAVE_INTRINSIC_INT128

  // verify that an integer greater than 2**64 that can be stored precisely
  // inside a double is converted to a basic::uint128 without loss of
  // information.
  double precise_double = 0x530e * std::pow(2.0, 64.0) + 0xda74000000000000;
  basic::uint128 from_precise_double(precise_double);
  basic::uint128 from_precise_ints =
      basic::MakeUint128(0x530e, 0xda74000000000000);
  EXPECT_EQ(from_precise_double, from_precise_ints);
  EXPECT_DOUBLE_EQ(static_cast<double>(from_precise_ints), precise_double);

  double approx_double = 0xffffeeeeddddcccc * std::pow(2.0, 64.0) +
                         0xbbbbaaaa99998888;
  basic::uint128 from_approx_double(approx_double);
  EXPECT_DOUBLE_EQ(static_cast<double>(from_approx_double), approx_double);

  double round_to_zero = 0.7;
  double round_to_five = 5.8;
  double round_to_nine = 9.3;
  EXPECT_EQ(static_cast<basic::uint128>(round_to_zero), 0);
  EXPECT_EQ(static_cast<basic::uint128>(round_to_five), 5);
  EXPECT_EQ(static_cast<basic::uint128>(round_to_nine), 9);

  basic::uint128 highest_precision_in_long_double =
      ~basic::uint128{} >> (128 - std::numeric_limits<long double>::digits);
  EXPECT_EQ(highest_precision_in_long_double,
            static_cast<basic::uint128>(
                static_cast<long double>(highest_precision_in_long_double)));
  // Apply a mask just to make sure all the bits are the right place.
  const basic::uint128 arbitrary_mask =
      basic::MakeUint128(0xa29f622677ded751, 0xf8ca66add076f468);
  EXPECT_EQ(highest_precision_in_long_double & arbitrary_mask,
            static_cast<basic::uint128>(static_cast<long double>(
                highest_precision_in_long_double & arbitrary_mask)));

  EXPECT_EQ(static_cast<basic::uint128>(-0.1L), 0);
}

TEST(Uint128, OperatorAssignReturnRef) {
  basic::uint128 v(1);
  (v += 4) -= 3;
  EXPECT_EQ(2, v);
}

TEST(Uint128, Multiply) {
  basic::uint128 a, b, c;

  // Zero test.
  a = 0;
  b = 0;
  c = a * b;
  EXPECT_EQ(0, c);

  // Max carries.
  a = basic::uint128(0) - 1;
  b = basic::uint128(0) - 1;
  c = a * b;
  EXPECT_EQ(1, c);

  // Self-operation with max carries.
  c = basic::uint128(0) - 1;
  c *= c;
  EXPECT_EQ(1, c);

  // 1-bit x 1-bit.
  for (int i = 0; i < 64; ++i) {
    for (int j = 0; j < 64; ++j) {
      a = basic::uint128(1) << i;
      b = basic::uint128(1) << j;
      c = a * b;
      EXPECT_EQ(basic::uint128(1) << (i + j), c);
    }
  }

  // Verified with dc.
  a = basic::MakeUint128(0xffffeeeeddddcccc, 0xbbbbaaaa99998888);
  b = basic::MakeUint128(0x7777666655554444, 0x3333222211110000);
  c = a * b;
  EXPECT_EQ(basic::MakeUint128(0x530EDA741C71D4C3, 0xBF25975319080000), c);
  EXPECT_EQ(0, c - b * a);
  EXPECT_EQ(a*a - b*b, (a+b) * (a-b));

  // Verified with dc.
  a = basic::MakeUint128(0x0123456789abcdef, 0xfedcba9876543210);
  b = basic::MakeUint128(0x02468ace13579bdf, 0xfdb97531eca86420);
  c = a * b;
  EXPECT_EQ(basic::MakeUint128(0x97a87f4f261ba3f2, 0x342d0bbf48948200), c);
  EXPECT_EQ(0, c - b * a);
  EXPECT_EQ(a*a - b*b, (a+b) * (a-b));
}

TEST(Uint128, AliasTests) {
  basic::uint128 x1 = basic::MakeUint128(1, 2);
  basic::uint128 x2 = basic::MakeUint128(2, 4);
  x1 += x1;
  EXPECT_EQ(x2, x1);

  basic::uint128 x3 = basic::MakeUint128(1, static_cast<uint64_t>(1) << 63);
  basic::uint128 x4 = basic::MakeUint128(3, 0);
  x3 += x3;
  EXPECT_EQ(x4, x3);
}

TEST(Uint128, DivideAndMod) {
  using std::swap;

  // a := q * b + r
  basic::uint128 a, b, q, r;

  // Zero test.
  a = 0;
  b = 123;
  q = a / b;
  r = a % b;
  EXPECT_EQ(0, q);
  EXPECT_EQ(0, r);

  a = basic::MakeUint128(0x530eda741c71d4c3, 0xbf25975319080000);
  q = basic::MakeUint128(0x4de2cab081, 0x14c34ab4676e4bab);
  b = basic::uint128(0x1110001);
  r = basic::uint128(0x3eb455);
  ASSERT_EQ(a, q * b + r);  // Sanity-check.

  basic::uint128 result_q, result_r;
  result_q = a / b;
  result_r = a % b;
  EXPECT_EQ(q, result_q);
  EXPECT_EQ(r, result_r);

  // Try the other way around.
  swap(q, b);
  result_q = a / b;
  result_r = a % b;
  EXPECT_EQ(q, result_q);
  EXPECT_EQ(r, result_r);
  // Restore.
  swap(b, q);

  // Dividend < divisor; result should be q:0 r:<dividend>.
  swap(a, b);
  result_q = a / b;
  result_r = a % b;
  EXPECT_EQ(0, result_q);
  EXPECT_EQ(a, result_r);
  // Try the other way around.
  swap(a, q);
  result_q = a / b;
  result_r = a % b;
  EXPECT_EQ(0, result_q);
  EXPECT_EQ(a, result_r);
  // Restore.
  swap(q, a);
  swap(b, a);

  // Try a large remainder.
  b = a / 2 + 1;
  basic::uint128 expected_r =
      basic::MakeUint128(0x29876d3a0e38ea61, 0xdf92cba98c83ffff);
  // Sanity checks.
  ASSERT_EQ(a / 2 - 1, expected_r);
  ASSERT_EQ(a, b + expected_r);
  result_q = a / b;
  result_r = a % b;
  EXPECT_EQ(1, result_q);
  EXPECT_EQ(expected_r, result_r);
}

TEST(Uint128, DivideAndModRandomInputs) {
  const int kNumIters = 1 << 18;
  std::minstd_rand random(testing::UnitTest::GetInstance()->random_seed());
  std::uniform_int_distribution<uint64_t> uniform_uint64;
  for (int i = 0; i < kNumIters; ++i) {
    const basic::uint128 a =
        basic::MakeUint128(uniform_uint64(random), uniform_uint64(random));
    const basic::uint128 b =
        basic::MakeUint128(uniform_uint64(random), uniform_uint64(random));
    if (b == 0) {
      continue;  // Avoid a div-by-zero.
    }
    const basic::uint128 q = a / b;
    const basic::uint128 r = a % b;
    ASSERT_EQ(a, b * q + r);
  }
}

TEST(Uint128, ConstexprTest) {
  constexpr basic::uint128 zero = basic::uint128();
  constexpr basic::uint128 one = 1;
  constexpr basic::uint128 minus_two = -2;
  EXPECT_EQ(zero, basic::uint128(0));
  EXPECT_EQ(one, basic::uint128(1));
  EXPECT_EQ(minus_two, basic::MakeUint128(-1, -2));
}

TEST(Uint128, NumericLimitsTest) {
  static_assert(std::numeric_limits<basic::uint128>::is_specialized, "");
  static_assert(!std::numeric_limits<basic::uint128>::is_signed, "");
  static_assert(std::numeric_limits<basic::uint128>::is_integer, "");
  EXPECT_EQ(static_cast<int>(128 * std::log10(2)),
            std::numeric_limits<basic::uint128>::digits10);
  EXPECT_EQ(0, std::numeric_limits<basic::uint128>::min());
  EXPECT_EQ(0, std::numeric_limits<basic::uint128>::lowest());
  EXPECT_EQ(basic::Uint128Max(), std::numeric_limits<basic::uint128>::max());
}

TEST(Uint128, Hash) {
  EXPECT_TRUE(basic::VerifyTypeImplementsAbslHashCorrectly({
      // Some simple values
      basic::uint128{0},
      basic::uint128{1},
      ~basic::uint128{},
      // 64 bit limits
      basic::uint128{std::numeric_limits<int64_t>::max()},
      basic::uint128{std::numeric_limits<uint64_t>::max()} + 0,
      basic::uint128{std::numeric_limits<uint64_t>::max()} + 1,
      basic::uint128{std::numeric_limits<uint64_t>::max()} + 2,
      // Keeping high same
      basic::uint128{1} << 62,
      basic::uint128{1} << 63,
      // Keeping low same
      basic::uint128{1} << 64,
      basic::uint128{1} << 65,
      // 128 bit limits
      std::numeric_limits<basic::uint128>::max(),
      std::numeric_limits<basic::uint128>::max() - 1,
      std::numeric_limits<basic::uint128>::min() + 1,
      std::numeric_limits<basic::uint128>::min(),
  }));
}

}  // namespace
