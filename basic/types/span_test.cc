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

#include "basic/types/span.h"

#include <array>
#include <initializer_list>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "basic/base/attributes.h"
#include "basic/base/config.h"
#include "basic/base/internal/exception_testing.h"
#include "basic/container/fixed_array.h"
#include "basic/container/inlined_vector.h"
#include "basic/hash/hash_testing.h"
#include "basic/strings/str_cat.h"

namespace {

MATCHER_P(DataIs, data,
          basic::StrCat("data() is ", negation ? "is " : "isn't ",
                       testing::PrintToString(data))) {
  return arg.data() == data;
}

template <typename T>
auto SpanIs(T data, size_t size)
    -> decltype(testing::AllOf(DataIs(data), testing::SizeIs(size))) {
  return testing::AllOf(DataIs(data), testing::SizeIs(size));
}

template <typename Container>
auto SpanIs(const Container& c) -> decltype(SpanIs(c.data(), c.size())) {
  return SpanIs(c.data(), c.size());
}

std::vector<int> MakeRamp(int len, int offset = 0) {
  std::vector<int> v(len);
  std::iota(v.begin(), v.end(), offset);
  return v;
}

TEST(IntSpan, EmptyCtors) {
  basic::Span<int> s;
  EXPECT_THAT(s, SpanIs(nullptr, 0));
}

TEST(IntSpan, PtrLenCtor) {
  int a[] = {1, 2, 3};
  basic::Span<int> s(&a[0], 2);
  EXPECT_THAT(s, SpanIs(a, 2));
}

TEST(IntSpan, ArrayCtor) {
  int a[] = {1, 2, 3};
  basic::Span<int> s(a);
  EXPECT_THAT(s, SpanIs(a, 3));

  EXPECT_TRUE((std::is_constructible<basic::Span<const int>, int[3]>::value));
  EXPECT_TRUE(
      (std::is_constructible<basic::Span<const int>, const int[3]>::value));
  EXPECT_FALSE((std::is_constructible<basic::Span<int>, const int[3]>::value));
  EXPECT_TRUE((std::is_convertible<int[3], basic::Span<const int>>::value));
  EXPECT_TRUE(
      (std::is_convertible<const int[3], basic::Span<const int>>::value));
}

template <typename T>
void TakesGenericSpan(basic::Span<T>) {}

TEST(IntSpan, ContainerCtor) {
  std::vector<int> empty;
  basic::Span<int> s_empty(empty);
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::vector<int> filled{1, 2, 3};
  basic::Span<int> s_filled(filled);
  EXPECT_THAT(s_filled, SpanIs(filled));

  basic::Span<int> s_from_span(filled);
  EXPECT_THAT(s_from_span, SpanIs(s_filled));

  basic::Span<const int> const_filled = filled;
  EXPECT_THAT(const_filled, SpanIs(filled));

  basic::Span<const int> const_from_span = s_filled;
  EXPECT_THAT(const_from_span, SpanIs(s_filled));

  EXPECT_TRUE(
      (std::is_convertible<std::vector<int>&, basic::Span<const int>>::value));
  EXPECT_TRUE(
      (std::is_convertible<basic::Span<int>&, basic::Span<const int>>::value));

  TakesGenericSpan(basic::Span<int>(filled));
}

// A struct supplying shallow data() const.
struct ContainerWithShallowConstData {
  std::vector<int> storage;
  int* data() const { return const_cast<int*>(storage.data()); }
  int size() const { return storage.size(); }
};

TEST(IntSpan, ShallowConstness) {
  const ContainerWithShallowConstData c{MakeRamp(20)};
  basic::Span<int> s(
      c);  // We should be able to do this even though data() is const.
  s[0] = -1;
  EXPECT_EQ(c.storage[0], -1);
}

TEST(CharSpan, StringCtor) {
  std::string empty = "";
  basic::Span<char> s_empty(empty);
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::string abc = "abc";
  basic::Span<char> s_abc(abc);
  EXPECT_THAT(s_abc, SpanIs(abc));

  basic::Span<const char> s_const_abc = abc;
  EXPECT_THAT(s_const_abc, SpanIs(abc));

  EXPECT_FALSE((std::is_constructible<basic::Span<int>, std::string>::value));
  EXPECT_FALSE(
      (std::is_constructible<basic::Span<const int>, std::string>::value));
  EXPECT_TRUE(
      (std::is_convertible<std::string, basic::Span<const char>>::value));
}

TEST(IntSpan, FromConstPointer) {
  EXPECT_TRUE((std::is_constructible<basic::Span<const int* const>,
                                     std::vector<int*>>::value));
  EXPECT_TRUE((std::is_constructible<basic::Span<const int* const>,
                                     std::vector<const int*>>::value));
  EXPECT_FALSE((
      std::is_constructible<basic::Span<const int*>, std::vector<int*>>::value));
  EXPECT_FALSE((
      std::is_constructible<basic::Span<int*>, std::vector<const int*>>::value));
}

struct TypeWithMisleadingData {
  int& data() { return i; }
  int size() { return 1; }
  int i;
};

struct TypeWithMisleadingSize {
  int* data() { return &i; }
  const char* size() { return "1"; }
  int i;
};

TEST(IntSpan, EvilTypes) {
  EXPECT_FALSE(
      (std::is_constructible<basic::Span<int>, TypeWithMisleadingData&>::value));
  EXPECT_FALSE(
      (std::is_constructible<basic::Span<int>, TypeWithMisleadingSize&>::value));
}

struct Base {
  int* data() { return &i; }
  int size() { return 1; }
  int i;
};
struct Derived : Base {};

TEST(IntSpan, SpanOfDerived) {
  EXPECT_TRUE((std::is_constructible<basic::Span<int>, Base&>::value));
  EXPECT_TRUE((std::is_constructible<basic::Span<int>, Derived&>::value));
  EXPECT_FALSE(
      (std::is_constructible<basic::Span<Base>, std::vector<Derived>>::value));
}

void TestInitializerList(basic::Span<const int> s, const std::vector<int>& v) {
  EXPECT_TRUE(basic::equal(s.begin(), s.end(), v.begin(), v.end()));
}

TEST(ConstIntSpan, InitializerListConversion) {
  TestInitializerList({}, {});
  TestInitializerList({1}, {1});
  TestInitializerList({1, 2, 3}, {1, 2, 3});

  EXPECT_FALSE((std::is_constructible<basic::Span<int>,
                                      std::initializer_list<int>>::value));
  EXPECT_FALSE((
      std::is_convertible<basic::Span<int>, std::initializer_list<int>>::value));
}

TEST(IntSpan, Data) {
  int i;
  basic::Span<int> s(&i, 1);
  EXPECT_EQ(&i, s.data());
}

TEST(IntSpan, SizeLengthEmpty) {
  basic::Span<int> empty;
  EXPECT_EQ(empty.size(), 0);
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(empty.size(), empty.length());

  auto v = MakeRamp(10);
  basic::Span<int> s(v);
  EXPECT_EQ(s.size(), 10);
  EXPECT_FALSE(s.empty());
  EXPECT_EQ(s.size(), s.length());
}

TEST(IntSpan, ElementAccess) {
  auto v = MakeRamp(10);
  basic::Span<int> s(v);
  for (int i = 0; i < s.size(); ++i) {
    EXPECT_EQ(s[i], s.at(i));
  }

  EXPECT_EQ(s.front(), s[0]);
  EXPECT_EQ(s.back(), s[9]);
}

TEST(IntSpan, AtThrows) {
  auto v = MakeRamp(10);
  basic::Span<int> s(v);

  EXPECT_EQ(s.at(9), 9);
  ABSL_BASE_INTERNAL_EXPECT_FAIL(s.at(10), std::out_of_range,
                                 "failed bounds check");
}

TEST(IntSpan, RemovePrefixAndSuffix) {
  auto v = MakeRamp(20, 1);
  basic::Span<int> s(v);
  EXPECT_EQ(s.size(), 20);

  s.remove_suffix(0);
  s.remove_prefix(0);
  EXPECT_EQ(s.size(), 20);

  s.remove_prefix(1);
  EXPECT_EQ(s.size(), 19);
  EXPECT_EQ(s[0], 2);

  s.remove_suffix(1);
  EXPECT_EQ(s.size(), 18);
  EXPECT_EQ(s.back(), 19);

  s.remove_prefix(7);
  EXPECT_EQ(s.size(), 11);
  EXPECT_EQ(s[0], 9);

  s.remove_suffix(11);
  EXPECT_EQ(s.size(), 0);

  EXPECT_EQ(v, MakeRamp(20, 1));
}

TEST(IntSpan, Subspan) {
  std::vector<int> empty;
  EXPECT_EQ(basic::MakeSpan(empty).subspan(), empty);
  EXPECT_THAT(basic::MakeSpan(empty).subspan(0, 0), SpanIs(empty));
  EXPECT_THAT(basic::MakeSpan(empty).subspan(0, basic::Span<const int>::npos),
              SpanIs(empty));

  auto ramp = MakeRamp(10);
  EXPECT_THAT(basic::MakeSpan(ramp).subspan(), SpanIs(ramp));
  EXPECT_THAT(basic::MakeSpan(ramp).subspan(0, 10), SpanIs(ramp));
  EXPECT_THAT(basic::MakeSpan(ramp).subspan(0, basic::Span<const int>::npos),
              SpanIs(ramp));
  EXPECT_THAT(basic::MakeSpan(ramp).subspan(0, 3), SpanIs(ramp.data(), 3));
  EXPECT_THAT(basic::MakeSpan(ramp).subspan(5, basic::Span<const int>::npos),
              SpanIs(ramp.data() + 5, 5));
  EXPECT_THAT(basic::MakeSpan(ramp).subspan(3, 3), SpanIs(ramp.data() + 3, 3));
  EXPECT_THAT(basic::MakeSpan(ramp).subspan(10, 5), SpanIs(ramp.data() + 10, 0));

#ifdef ABSL_HAVE_EXCEPTIONS
  EXPECT_THROW(basic::MakeSpan(ramp).subspan(11, 5), std::out_of_range);
#else
  EXPECT_DEATH_IF_SUPPORTED(basic::MakeSpan(ramp).subspan(11, 5), "");
#endif
}

TEST(IntSpan, First) {
  std::vector<int> empty;
  EXPECT_THAT(basic::MakeSpan(empty).first(0), SpanIs(empty));

  auto ramp = MakeRamp(10);
  EXPECT_THAT(basic::MakeSpan(ramp).first(0), SpanIs(ramp.data(), 0));
  EXPECT_THAT(basic::MakeSpan(ramp).first(10), SpanIs(ramp));
  EXPECT_THAT(basic::MakeSpan(ramp).first(3), SpanIs(ramp.data(), 3));

#ifdef ABSL_HAVE_EXCEPTIONS
  EXPECT_THROW(basic::MakeSpan(ramp).first(11), std::out_of_range);
#else
  EXPECT_DEATH_IF_SUPPORTED(basic::MakeSpan(ramp).first(11), "");
#endif
}

TEST(IntSpan, Last) {
  std::vector<int> empty;
  EXPECT_THAT(basic::MakeSpan(empty).last(0), SpanIs(empty));

  auto ramp = MakeRamp(10);
  EXPECT_THAT(basic::MakeSpan(ramp).last(0), SpanIs(ramp.data() + 10, 0));
  EXPECT_THAT(basic::MakeSpan(ramp).last(10), SpanIs(ramp));
  EXPECT_THAT(basic::MakeSpan(ramp).last(3), SpanIs(ramp.data() + 7, 3));

#ifdef ABSL_HAVE_EXCEPTIONS
  EXPECT_THROW(basic::MakeSpan(ramp).last(11), std::out_of_range);
#else
  EXPECT_DEATH_IF_SUPPORTED(basic::MakeSpan(ramp).last(11), "");
#endif
}

TEST(IntSpan, MakeSpanPtrLength) {
  std::vector<int> empty;
  auto s_empty = basic::MakeSpan(empty.data(), empty.size());
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::array<int, 3> a{{1, 2, 3}};
  auto s = basic::MakeSpan(a.data(), a.size());
  EXPECT_THAT(s, SpanIs(a));

  EXPECT_THAT(basic::MakeConstSpan(empty.data(), empty.size()), SpanIs(s_empty));
  EXPECT_THAT(basic::MakeConstSpan(a.data(), a.size()), SpanIs(s));
}

TEST(IntSpan, MakeSpanTwoPtrs) {
  std::vector<int> empty;
  auto s_empty = basic::MakeSpan(empty.data(), empty.data());
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::vector<int> v{1, 2, 3};
  auto s = basic::MakeSpan(v.data(), v.data() + 1);
  EXPECT_THAT(s, SpanIs(v.data(), 1));

  EXPECT_THAT(basic::MakeConstSpan(empty.data(), empty.data()), SpanIs(s_empty));
  EXPECT_THAT(basic::MakeConstSpan(v.data(), v.data() + 1), SpanIs(s));
}

TEST(IntSpan, MakeSpanContainer) {
  std::vector<int> empty;
  auto s_empty = basic::MakeSpan(empty);
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::vector<int> v{1, 2, 3};
  auto s = basic::MakeSpan(v);
  EXPECT_THAT(s, SpanIs(v));

  EXPECT_THAT(basic::MakeConstSpan(empty), SpanIs(s_empty));
  EXPECT_THAT(basic::MakeConstSpan(v), SpanIs(s));

  EXPECT_THAT(basic::MakeSpan(s), SpanIs(s));
  EXPECT_THAT(basic::MakeConstSpan(s), SpanIs(s));
}

TEST(CharSpan, MakeSpanString) {
  std::string empty = "";
  auto s_empty = basic::MakeSpan(empty);
  EXPECT_THAT(s_empty, SpanIs(empty));

  std::string str = "abc";
  auto s_str = basic::MakeSpan(str);
  EXPECT_THAT(s_str, SpanIs(str));

  EXPECT_THAT(basic::MakeConstSpan(empty), SpanIs(s_empty));
  EXPECT_THAT(basic::MakeConstSpan(str), SpanIs(s_str));
}

TEST(IntSpan, MakeSpanArray) {
  int a[] = {1, 2, 3};
  auto s = basic::MakeSpan(a);
  EXPECT_THAT(s, SpanIs(a, 3));

  const int ca[] = {1, 2, 3};
  auto s_ca = basic::MakeSpan(ca);
  EXPECT_THAT(s_ca, SpanIs(ca, 3));

  EXPECT_THAT(basic::MakeConstSpan(a), SpanIs(s));
  EXPECT_THAT(basic::MakeConstSpan(ca), SpanIs(s_ca));
}

// Compile-asserts that the argument has the expected decayed type.
template <typename Expected, typename T>
void CheckType(const T& /* value */) {
  testing::StaticAssertTypeEq<Expected, T>();
}

TEST(IntSpan, MakeSpanTypes) {
  std::vector<int> vec;
  const std::vector<int> cvec;
  int a[1];
  const int ca[] = {1};
  int* ip = a;
  const int* cip = ca;
  std::string s = "";
  const std::string cs = "";
  CheckType<basic::Span<int>>(basic::MakeSpan(vec));
  CheckType<basic::Span<const int>>(basic::MakeSpan(cvec));
  CheckType<basic::Span<int>>(basic::MakeSpan(ip, ip + 1));
  CheckType<basic::Span<int>>(basic::MakeSpan(ip, 1));
  CheckType<basic::Span<const int>>(basic::MakeSpan(cip, cip + 1));
  CheckType<basic::Span<const int>>(basic::MakeSpan(cip, 1));
  CheckType<basic::Span<int>>(basic::MakeSpan(a));
  CheckType<basic::Span<int>>(basic::MakeSpan(a, a + 1));
  CheckType<basic::Span<int>>(basic::MakeSpan(a, 1));
  CheckType<basic::Span<const int>>(basic::MakeSpan(ca));
  CheckType<basic::Span<const int>>(basic::MakeSpan(ca, ca + 1));
  CheckType<basic::Span<const int>>(basic::MakeSpan(ca, 1));
  CheckType<basic::Span<char>>(basic::MakeSpan(s));
  CheckType<basic::Span<const char>>(basic::MakeSpan(cs));
}

TEST(ConstIntSpan, MakeConstSpanTypes) {
  std::vector<int> vec;
  const std::vector<int> cvec;
  int array[1];
  const int carray[] = {0};
  int* ptr = array;
  const int* cptr = carray;
  std::string s = "";
  std::string cs = "";
  CheckType<basic::Span<const int>>(basic::MakeConstSpan(vec));
  CheckType<basic::Span<const int>>(basic::MakeConstSpan(cvec));
  CheckType<basic::Span<const int>>(basic::MakeConstSpan(ptr, ptr + 1));
  CheckType<basic::Span<const int>>(basic::MakeConstSpan(ptr, 1));
  CheckType<basic::Span<const int>>(basic::MakeConstSpan(cptr, cptr + 1));
  CheckType<basic::Span<const int>>(basic::MakeConstSpan(cptr, 1));
  CheckType<basic::Span<const int>>(basic::MakeConstSpan(array));
  CheckType<basic::Span<const int>>(basic::MakeConstSpan(carray));
  CheckType<basic::Span<const char>>(basic::MakeConstSpan(s));
  CheckType<basic::Span<const char>>(basic::MakeConstSpan(cs));
}

TEST(IntSpan, Equality) {
  const int arr1[] = {1, 2, 3, 4, 5};
  int arr2[] = {1, 2, 3, 4, 5};
  std::vector<int> vec1(std::begin(arr1), std::end(arr1));
  std::vector<int> vec2 = vec1;
  std::vector<int> other_vec = {2, 4, 6, 8, 10};
  // These two slices are from different vectors, but have the same size and
  // have the same elements (right now).  They should compare equal. Test both
  // == and !=.
  const basic::Span<const int> from1 = vec1;
  const basic::Span<const int> from2 = vec2;
  EXPECT_EQ(from1, from1);
  EXPECT_FALSE(from1 != from1);
  EXPECT_EQ(from1, from2);
  EXPECT_FALSE(from1 != from2);

  // These two slices have different underlying vector values. They should be
  // considered not equal. Test both == and !=.
  const basic::Span<const int> from_other = other_vec;
  EXPECT_NE(from1, from_other);
  EXPECT_FALSE(from1 == from_other);

  // Comparison between a vector and its slice should be equal. And vice-versa.
  // This ensures implicit conversion to Span works on both sides of ==.
  EXPECT_EQ(vec1, from1);
  EXPECT_FALSE(vec1 != from1);
  EXPECT_EQ(from1, vec1);
  EXPECT_FALSE(from1 != vec1);

  // This verifies that basic::Span<T> can be compared freely with
  // basic::Span<const T>.
  const basic::Span<int> mutable_from1(vec1);
  const basic::Span<int> mutable_from2(vec2);
  EXPECT_EQ(from1, mutable_from1);
  EXPECT_EQ(mutable_from1, from1);
  EXPECT_EQ(mutable_from1, mutable_from2);
  EXPECT_EQ(mutable_from2, mutable_from1);

  // Comparison between a vector and its slice should be equal for mutable
  // Spans as well.
  EXPECT_EQ(vec1, mutable_from1);
  EXPECT_FALSE(vec1 != mutable_from1);
  EXPECT_EQ(mutable_from1, vec1);
  EXPECT_FALSE(mutable_from1 != vec1);

  // Comparison between convertible-to-Span-of-const and Span-of-mutable. Arrays
  // are used because they're the only value type which converts to a
  // Span-of-mutable. EXPECT_TRUE is used instead of EXPECT_EQ to avoid
  // array-to-pointer decay.
  EXPECT_TRUE(arr1 == mutable_from1);
  EXPECT_FALSE(arr1 != mutable_from1);
  EXPECT_TRUE(mutable_from1 == arr1);
  EXPECT_FALSE(mutable_from1 != arr1);

  // Comparison between convertible-to-Span-of-mutable and Span-of-const
  EXPECT_TRUE(arr2 == from1);
  EXPECT_FALSE(arr2 != from1);
  EXPECT_TRUE(from1 == arr2);
  EXPECT_FALSE(from1 != arr2);

  // With a different size, the array slices should not be equal.
  EXPECT_NE(from1, basic::Span<const int>(from1).subspan(0, from1.size() - 1));

  // With different contents, the array slices should not be equal.
  ++vec2.back();
  EXPECT_NE(from1, from2);
}

class IntSpanOrderComparisonTest : public testing::Test {
 public:
  IntSpanOrderComparisonTest()
      : arr_before_{1, 2, 3},
        arr_after_{1, 2, 4},
        carr_after_{1, 2, 4},
        vec_before_(std::begin(arr_before_), std::end(arr_before_)),
        vec_after_(std::begin(arr_after_), std::end(arr_after_)),
        before_(vec_before_),
        after_(vec_after_),
        cbefore_(vec_before_),
        cafter_(vec_after_) {}

 protected:
  int arr_before_[3], arr_after_[3];
  const int carr_after_[3];
  std::vector<int> vec_before_, vec_after_;
  basic::Span<int> before_, after_;
  basic::Span<const int> cbefore_, cafter_;
};

TEST_F(IntSpanOrderComparisonTest, CompareSpans) {
  EXPECT_TRUE(cbefore_ < cafter_);
  EXPECT_TRUE(cbefore_ <= cafter_);
  EXPECT_TRUE(cafter_ > cbefore_);
  EXPECT_TRUE(cafter_ >= cbefore_);

  EXPECT_FALSE(cbefore_ > cafter_);
  EXPECT_FALSE(cafter_ < cbefore_);

  EXPECT_TRUE(before_ < after_);
  EXPECT_TRUE(before_ <= after_);
  EXPECT_TRUE(after_ > before_);
  EXPECT_TRUE(after_ >= before_);

  EXPECT_FALSE(before_ > after_);
  EXPECT_FALSE(after_ < before_);

  EXPECT_TRUE(cbefore_ < after_);
  EXPECT_TRUE(cbefore_ <= after_);
  EXPECT_TRUE(after_ > cbefore_);
  EXPECT_TRUE(after_ >= cbefore_);

  EXPECT_FALSE(cbefore_ > after_);
  EXPECT_FALSE(after_ < cbefore_);
}

TEST_F(IntSpanOrderComparisonTest, SpanOfConstAndContainer) {
  EXPECT_TRUE(cbefore_ < vec_after_);
  EXPECT_TRUE(cbefore_ <= vec_after_);
  EXPECT_TRUE(vec_after_ > cbefore_);
  EXPECT_TRUE(vec_after_ >= cbefore_);

  EXPECT_FALSE(cbefore_ > vec_after_);
  EXPECT_FALSE(vec_after_ < cbefore_);

  EXPECT_TRUE(arr_before_ < cafter_);
  EXPECT_TRUE(arr_before_ <= cafter_);
  EXPECT_TRUE(cafter_ > arr_before_);
  EXPECT_TRUE(cafter_ >= arr_before_);

  EXPECT_FALSE(arr_before_ > cafter_);
  EXPECT_FALSE(cafter_ < arr_before_);
}

TEST_F(IntSpanOrderComparisonTest, SpanOfMutableAndContainer) {
  EXPECT_TRUE(vec_before_ < after_);
  EXPECT_TRUE(vec_before_ <= after_);
  EXPECT_TRUE(after_ > vec_before_);
  EXPECT_TRUE(after_ >= vec_before_);

  EXPECT_FALSE(vec_before_ > after_);
  EXPECT_FALSE(after_ < vec_before_);

  EXPECT_TRUE(before_ < carr_after_);
  EXPECT_TRUE(before_ <= carr_after_);
  EXPECT_TRUE(carr_after_ > before_);
  EXPECT_TRUE(carr_after_ >= before_);

  EXPECT_FALSE(before_ > carr_after_);
  EXPECT_FALSE(carr_after_ < before_);
}

TEST_F(IntSpanOrderComparisonTest, EqualSpans) {
  EXPECT_FALSE(before_ < before_);
  EXPECT_TRUE(before_ <= before_);
  EXPECT_FALSE(before_ > before_);
  EXPECT_TRUE(before_ >= before_);
}

TEST_F(IntSpanOrderComparisonTest, Subspans) {
  auto subspan = before_.subspan(0, 1);
  EXPECT_TRUE(subspan < before_);
  EXPECT_TRUE(subspan <= before_);
  EXPECT_TRUE(before_ > subspan);
  EXPECT_TRUE(before_ >= subspan);

  EXPECT_FALSE(subspan > before_);
  EXPECT_FALSE(before_ < subspan);
}

TEST_F(IntSpanOrderComparisonTest, EmptySpans) {
  basic::Span<int> empty;
  EXPECT_FALSE(empty < empty);
  EXPECT_TRUE(empty <= empty);
  EXPECT_FALSE(empty > empty);
  EXPECT_TRUE(empty >= empty);

  EXPECT_TRUE(empty < before_);
  EXPECT_TRUE(empty <= before_);
  EXPECT_TRUE(before_ > empty);
  EXPECT_TRUE(before_ >= empty);

  EXPECT_FALSE(empty > before_);
  EXPECT_FALSE(before_ < empty);
}

TEST(IntSpan, ExposesContainerTypesAndConsts) {
  basic::Span<int> slice;
  CheckType<basic::Span<int>::iterator>(slice.begin());
  EXPECT_TRUE((std::is_convertible<decltype(slice.begin()),
                                   basic::Span<int>::const_iterator>::value));
  CheckType<basic::Span<int>::const_iterator>(slice.cbegin());
  EXPECT_TRUE((std::is_convertible<decltype(slice.end()),
                                   basic::Span<int>::const_iterator>::value));
  CheckType<basic::Span<int>::const_iterator>(slice.cend());
  CheckType<basic::Span<int>::reverse_iterator>(slice.rend());
  EXPECT_TRUE(
      (std::is_convertible<decltype(slice.rend()),
                           basic::Span<int>::const_reverse_iterator>::value));
  CheckType<basic::Span<int>::const_reverse_iterator>(slice.crend());
  testing::StaticAssertTypeEq<int, basic::Span<int>::value_type>();
  testing::StaticAssertTypeEq<int, basic::Span<const int>::value_type>();
  testing::StaticAssertTypeEq<int*, basic::Span<int>::pointer>();
  testing::StaticAssertTypeEq<const int*, basic::Span<const int>::pointer>();
  testing::StaticAssertTypeEq<int&, basic::Span<int>::reference>();
  testing::StaticAssertTypeEq<const int&, basic::Span<const int>::reference>();
  testing::StaticAssertTypeEq<const int&, basic::Span<int>::const_reference>();
  testing::StaticAssertTypeEq<const int&,
                              basic::Span<const int>::const_reference>();
  EXPECT_EQ(static_cast<basic::Span<int>::size_type>(-1), basic::Span<int>::npos);
}

TEST(IntSpan, IteratorsAndReferences) {
  auto accept_pointer = [](int*) {};
  auto accept_reference = [](int&) {};
  auto accept_iterator = [](basic::Span<int>::iterator) {};
  auto accept_const_iterator = [](basic::Span<int>::const_iterator) {};
  auto accept_reverse_iterator = [](basic::Span<int>::reverse_iterator) {};
  auto accept_const_reverse_iterator =
      [](basic::Span<int>::const_reverse_iterator) {};

  int a[1];
  basic::Span<int> s = a;

  accept_pointer(s.data());
  accept_iterator(s.begin());
  accept_const_iterator(s.begin());
  accept_const_iterator(s.cbegin());
  accept_iterator(s.end());
  accept_const_iterator(s.end());
  accept_const_iterator(s.cend());
  accept_reverse_iterator(s.rbegin());
  accept_const_reverse_iterator(s.rbegin());
  accept_const_reverse_iterator(s.crbegin());
  accept_reverse_iterator(s.rend());
  accept_const_reverse_iterator(s.rend());
  accept_const_reverse_iterator(s.crend());

  accept_reference(s[0]);
  accept_reference(s.at(0));
  accept_reference(s.front());
  accept_reference(s.back());
}

TEST(IntSpan, IteratorsAndReferences_Const) {
  auto accept_pointer = [](int*) {};
  auto accept_reference = [](int&) {};
  auto accept_iterator = [](basic::Span<int>::iterator) {};
  auto accept_const_iterator = [](basic::Span<int>::const_iterator) {};
  auto accept_reverse_iterator = [](basic::Span<int>::reverse_iterator) {};
  auto accept_const_reverse_iterator =
      [](basic::Span<int>::const_reverse_iterator) {};

  int a[1];
  const basic::Span<int> s = a;

  accept_pointer(s.data());
  accept_iterator(s.begin());
  accept_const_iterator(s.begin());
  accept_const_iterator(s.cbegin());
  accept_iterator(s.end());
  accept_const_iterator(s.end());
  accept_const_iterator(s.cend());
  accept_reverse_iterator(s.rbegin());
  accept_const_reverse_iterator(s.rbegin());
  accept_const_reverse_iterator(s.crbegin());
  accept_reverse_iterator(s.rend());
  accept_const_reverse_iterator(s.rend());
  accept_const_reverse_iterator(s.crend());

  accept_reference(s[0]);
  accept_reference(s.at(0));
  accept_reference(s.front());
  accept_reference(s.back());
}

TEST(IntSpan, NoexceptTest) {
  int a[] = {1, 2, 3};
  std::vector<int> v;
  EXPECT_TRUE(noexcept(basic::Span<const int>()));
  EXPECT_TRUE(noexcept(basic::Span<const int>(a, 2)));
  EXPECT_TRUE(noexcept(basic::Span<const int>(a)));
  EXPECT_TRUE(noexcept(basic::Span<const int>(v)));
  EXPECT_TRUE(noexcept(basic::Span<int>(v)));
  EXPECT_TRUE(noexcept(basic::Span<const int>({1, 2, 3})));
  EXPECT_TRUE(noexcept(basic::MakeSpan(v)));
  EXPECT_TRUE(noexcept(basic::MakeSpan(a)));
  EXPECT_TRUE(noexcept(basic::MakeSpan(a, 2)));
  EXPECT_TRUE(noexcept(basic::MakeSpan(a, a + 1)));
  EXPECT_TRUE(noexcept(basic::MakeConstSpan(v)));
  EXPECT_TRUE(noexcept(basic::MakeConstSpan(a)));
  EXPECT_TRUE(noexcept(basic::MakeConstSpan(a, 2)));
  EXPECT_TRUE(noexcept(basic::MakeConstSpan(a, a + 1)));

  basic::Span<int> s(v);
  EXPECT_TRUE(noexcept(s.data()));
  EXPECT_TRUE(noexcept(s.size()));
  EXPECT_TRUE(noexcept(s.length()));
  EXPECT_TRUE(noexcept(s.empty()));
  EXPECT_TRUE(noexcept(s[0]));
  EXPECT_TRUE(noexcept(s.front()));
  EXPECT_TRUE(noexcept(s.back()));
  EXPECT_TRUE(noexcept(s.begin()));
  EXPECT_TRUE(noexcept(s.cbegin()));
  EXPECT_TRUE(noexcept(s.end()));
  EXPECT_TRUE(noexcept(s.cend()));
  EXPECT_TRUE(noexcept(s.rbegin()));
  EXPECT_TRUE(noexcept(s.crbegin()));
  EXPECT_TRUE(noexcept(s.rend()));
  EXPECT_TRUE(noexcept(s.crend()));
  EXPECT_TRUE(noexcept(s.remove_prefix(0)));
  EXPECT_TRUE(noexcept(s.remove_suffix(0)));
}

// ConstexprTester exercises expressions in a constexpr context. Simply placing
// the expression in a constexpr function is not enough, as some compilers will
// simply compile the constexpr function as runtime code. Using template
// parameters forces compile-time execution.
template <int i>
struct ConstexprTester {};

#define ABSL_TEST_CONSTEXPR(expr)                       \
  do {                                                  \
    ABSL_ATTRIBUTE_UNUSED ConstexprTester<(expr, 1)> t; \
  } while (0)

struct ContainerWithConstexprMethods {
  constexpr int size() const { return 1; }
  constexpr const int* data() const { return &i; }
  const int i;
};

TEST(ConstIntSpan, ConstexprTest) {
  static constexpr int a[] = {1, 2, 3};
  static constexpr int sized_arr[2] = {1, 2};
  static constexpr ContainerWithConstexprMethods c{1};
  ABSL_TEST_CONSTEXPR(basic::Span<const int>());
  ABSL_TEST_CONSTEXPR(basic::Span<const int>(a, 2));
  ABSL_TEST_CONSTEXPR(basic::Span<const int>(sized_arr));
  ABSL_TEST_CONSTEXPR(basic::Span<const int>(c));
  ABSL_TEST_CONSTEXPR(basic::MakeSpan(&a[0], 1));
  ABSL_TEST_CONSTEXPR(basic::MakeSpan(c));
  ABSL_TEST_CONSTEXPR(basic::MakeSpan(a));
  ABSL_TEST_CONSTEXPR(basic::MakeConstSpan(&a[0], 1));
  ABSL_TEST_CONSTEXPR(basic::MakeConstSpan(c));
  ABSL_TEST_CONSTEXPR(basic::MakeConstSpan(a));

  constexpr basic::Span<const int> span = c;
  ABSL_TEST_CONSTEXPR(span.data());
  ABSL_TEST_CONSTEXPR(span.size());
  ABSL_TEST_CONSTEXPR(span.length());
  ABSL_TEST_CONSTEXPR(span.empty());
  ABSL_TEST_CONSTEXPR(span.begin());
  ABSL_TEST_CONSTEXPR(span.cbegin());
  ABSL_TEST_CONSTEXPR(span.subspan(0, 0));
  ABSL_TEST_CONSTEXPR(span.first(1));
  ABSL_TEST_CONSTEXPR(span.last(1));
  ABSL_TEST_CONSTEXPR(span[0]);
}

struct BigStruct {
  char bytes[10000];
};

TEST(Span, SpanSize) {
  EXPECT_LE(sizeof(basic::Span<int>), 2 * sizeof(void*));
  EXPECT_LE(sizeof(basic::Span<BigStruct>), 2 * sizeof(void*));
}

TEST(Span, Hash) {
  int array[] = {1, 2, 3, 4};
  int array2[] = {1, 2, 3};
  using T = basic::Span<const int>;
  EXPECT_TRUE(basic::VerifyTypeImplementsAbslHashCorrectly(
      {// Empties
       T(), T(nullptr, 0), T(array, 0), T(array2, 0),
       // Different array with same value
       T(array, 3), T(array2), T({1, 2, 3}),
       // Same array, but different length
       T(array, 1), T(array, 2),
       // Same length, but different array
       T(array + 1, 2), T(array + 2, 2)}));
}

}  // namespace
