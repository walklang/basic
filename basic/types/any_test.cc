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

#include "basic/types/any.h"

#include <initializer_list>
#include <type_traits>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "basic/base/config.h"
#include "basic/base/internal/exception_testing.h"
#include "basic/base/internal/raw_logging.h"
#include "basic/container/internal/test_instance_tracker.h"

namespace {
using basic::test_internal::CopyableOnlyInstance;
using basic::test_internal::InstanceTracker;

template <typename T>
const T& AsConst(const T& t) {
  return t;
}

struct MoveOnly {
  MoveOnly() = default;
  explicit MoveOnly(int value) : value(value) {}
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;

  int value = 0;
};

struct CopyOnly {
  CopyOnly() = default;
  explicit CopyOnly(int value) : value(value) {}
  CopyOnly(CopyOnly&&) = delete;
  CopyOnly& operator=(CopyOnly&&) = delete;
  CopyOnly(const CopyOnly&) = default;
  CopyOnly& operator=(const CopyOnly&) = default;

  int value = 0;
};

struct MoveOnlyWithListConstructor {
  MoveOnlyWithListConstructor() = default;
  explicit MoveOnlyWithListConstructor(std::initializer_list<int> /*ilist*/,
                                       int value)
      : value(value) {}
  MoveOnlyWithListConstructor(MoveOnlyWithListConstructor&&) = default;
  MoveOnlyWithListConstructor& operator=(MoveOnlyWithListConstructor&&) =
      default;

  int value = 0;
};

struct IntMoveOnlyCopyOnly {
  IntMoveOnlyCopyOnly(int value, MoveOnly /*move_only*/, CopyOnly /*copy_only*/)
      : value(value) {}

  int value;
};

struct ListMoveOnlyCopyOnly {
  ListMoveOnlyCopyOnly(std::initializer_list<int> ilist, MoveOnly /*move_only*/,
                       CopyOnly /*copy_only*/)
      : values(ilist) {}

  std::vector<int> values;
};

using FunctionType = void();
void FunctionToEmplace() {}

using ArrayType = int[2];
using DecayedArray = basic::decay_t<ArrayType>;

TEST(AnyTest, Noexcept) {
  static_assert(std::is_nothrow_default_constructible<basic::any>(), "");
  static_assert(std::is_nothrow_move_constructible<basic::any>(), "");
  static_assert(std::is_nothrow_move_assignable<basic::any>(), "");
  static_assert(noexcept(std::declval<basic::any&>().has_value()), "");
  static_assert(noexcept(std::declval<basic::any&>().type()), "");
  static_assert(noexcept(basic::any_cast<int>(std::declval<basic::any*>())), "");
  static_assert(
      noexcept(std::declval<basic::any&>().swap(std::declval<basic::any&>())),
      "");

  using std::swap;
  static_assert(
      noexcept(swap(std::declval<basic::any&>(), std::declval<basic::any&>())),
      "");
}

TEST(AnyTest, HasValue) {
  basic::any o;
  EXPECT_FALSE(o.has_value());
  o.emplace<int>();
  EXPECT_TRUE(o.has_value());
  o.reset();
  EXPECT_FALSE(o.has_value());
}

TEST(AnyTest, Type) {
  basic::any o;
  EXPECT_EQ(typeid(void), o.type());
  o.emplace<int>(5);
  EXPECT_EQ(typeid(int), o.type());
  o.emplace<float>(5.f);
  EXPECT_EQ(typeid(float), o.type());
  o.reset();
  EXPECT_EQ(typeid(void), o.type());
}

TEST(AnyTest, EmptyPointerCast) {
  // pointer-to-unqualified overload
  {
    basic::any o;
    EXPECT_EQ(nullptr, basic::any_cast<int>(&o));
    o.emplace<int>();
    EXPECT_NE(nullptr, basic::any_cast<int>(&o));
    o.reset();
    EXPECT_EQ(nullptr, basic::any_cast<int>(&o));
  }

  // pointer-to-const overload
  {
    basic::any o;
    EXPECT_EQ(nullptr, basic::any_cast<int>(&AsConst(o)));
    o.emplace<int>();
    EXPECT_NE(nullptr, basic::any_cast<int>(&AsConst(o)));
    o.reset();
    EXPECT_EQ(nullptr, basic::any_cast<int>(&AsConst(o)));
  }
}

TEST(AnyTest, InPlaceConstruction) {
  const CopyOnly copy_only{};
  basic::any o(basic::in_place_type_t<IntMoveOnlyCopyOnly>(), 5, MoveOnly(),
              copy_only);
  IntMoveOnlyCopyOnly& v = basic::any_cast<IntMoveOnlyCopyOnly&>(o);
  EXPECT_EQ(5, v.value);
}

TEST(AnyTest, InPlaceConstructionVariableTemplate) {
  const CopyOnly copy_only{};
  basic::any o(basic::in_place_type<IntMoveOnlyCopyOnly>, 5, MoveOnly(),
              copy_only);
  auto& v = basic::any_cast<IntMoveOnlyCopyOnly&>(o);
  EXPECT_EQ(5, v.value);
}

TEST(AnyTest, InPlaceConstructionWithCV) {
  const CopyOnly copy_only{};
  basic::any o(basic::in_place_type_t<const volatile IntMoveOnlyCopyOnly>(), 5,
              MoveOnly(), copy_only);
  IntMoveOnlyCopyOnly& v = basic::any_cast<IntMoveOnlyCopyOnly&>(o);
  EXPECT_EQ(5, v.value);
}

TEST(AnyTest, InPlaceConstructionWithCVVariableTemplate) {
  const CopyOnly copy_only{};
  basic::any o(basic::in_place_type<const volatile IntMoveOnlyCopyOnly>, 5,
              MoveOnly(), copy_only);
  auto& v = basic::any_cast<IntMoveOnlyCopyOnly&>(o);
  EXPECT_EQ(5, v.value);
}

TEST(AnyTest, InPlaceConstructionWithFunction) {
  basic::any o(basic::in_place_type_t<FunctionType>(), FunctionToEmplace);
  FunctionType*& construction_result = basic::any_cast<FunctionType*&>(o);
  EXPECT_EQ(&FunctionToEmplace, construction_result);
}

TEST(AnyTest, InPlaceConstructionWithFunctionVariableTemplate) {
  basic::any o(basic::in_place_type<FunctionType>, FunctionToEmplace);
  auto& construction_result = basic::any_cast<FunctionType*&>(o);
  EXPECT_EQ(&FunctionToEmplace, construction_result);
}

TEST(AnyTest, InPlaceConstructionWithArray) {
  ArrayType ar = {5, 42};
  basic::any o(basic::in_place_type_t<ArrayType>(), ar);
  DecayedArray& construction_result = basic::any_cast<DecayedArray&>(o);
  EXPECT_EQ(&ar[0], construction_result);
}

TEST(AnyTest, InPlaceConstructionWithArrayVariableTemplate) {
  ArrayType ar = {5, 42};
  basic::any o(basic::in_place_type<ArrayType>, ar);
  auto& construction_result = basic::any_cast<DecayedArray&>(o);
  EXPECT_EQ(&ar[0], construction_result);
}

TEST(AnyTest, InPlaceConstructionIlist) {
  const CopyOnly copy_only{};
  basic::any o(basic::in_place_type_t<ListMoveOnlyCopyOnly>(), {1, 2, 3, 4},
              MoveOnly(), copy_only);
  ListMoveOnlyCopyOnly& v = basic::any_cast<ListMoveOnlyCopyOnly&>(o);
  std::vector<int> expected_values = {1, 2, 3, 4};
  EXPECT_EQ(expected_values, v.values);
}

TEST(AnyTest, InPlaceConstructionIlistVariableTemplate) {
  const CopyOnly copy_only{};
  basic::any o(basic::in_place_type<ListMoveOnlyCopyOnly>, {1, 2, 3, 4},
              MoveOnly(), copy_only);
  auto& v = basic::any_cast<ListMoveOnlyCopyOnly&>(o);
  std::vector<int> expected_values = {1, 2, 3, 4};
  EXPECT_EQ(expected_values, v.values);
}

TEST(AnyTest, InPlaceConstructionIlistWithCV) {
  const CopyOnly copy_only{};
  basic::any o(basic::in_place_type_t<const volatile ListMoveOnlyCopyOnly>(),
              {1, 2, 3, 4}, MoveOnly(), copy_only);
  ListMoveOnlyCopyOnly& v = basic::any_cast<ListMoveOnlyCopyOnly&>(o);
  std::vector<int> expected_values = {1, 2, 3, 4};
  EXPECT_EQ(expected_values, v.values);
}

TEST(AnyTest, InPlaceConstructionIlistWithCVVariableTemplate) {
  const CopyOnly copy_only{};
  basic::any o(basic::in_place_type<const volatile ListMoveOnlyCopyOnly>,
              {1, 2, 3, 4}, MoveOnly(), copy_only);
  auto& v = basic::any_cast<ListMoveOnlyCopyOnly&>(o);
  std::vector<int> expected_values = {1, 2, 3, 4};
  EXPECT_EQ(expected_values, v.values);
}

TEST(AnyTest, InPlaceNoArgs) {
  basic::any o(basic::in_place_type_t<int>{});
  EXPECT_EQ(0, basic::any_cast<int&>(o));
}

TEST(AnyTest, InPlaceNoArgsVariableTemplate) {
  basic::any o(basic::in_place_type<int>);
  EXPECT_EQ(0, basic::any_cast<int&>(o));
}

template <typename Enabler, typename T, typename... Args>
struct CanEmplaceAnyImpl : std::false_type {};

template <typename T, typename... Args>
struct CanEmplaceAnyImpl<
    basic::void_t<decltype(
        std::declval<basic::any&>().emplace<T>(std::declval<Args>()...))>,
    T, Args...> : std::true_type {};

template <typename T, typename... Args>
using CanEmplaceAny = CanEmplaceAnyImpl<void, T, Args...>;

TEST(AnyTest, Emplace) {
  const CopyOnly copy_only{};
  basic::any o;
  EXPECT_TRUE((std::is_same<decltype(o.emplace<IntMoveOnlyCopyOnly>(
                                5, MoveOnly(), copy_only)),
                            IntMoveOnlyCopyOnly&>::value));
  IntMoveOnlyCopyOnly& emplace_result =
      o.emplace<IntMoveOnlyCopyOnly>(5, MoveOnly(), copy_only);
  EXPECT_EQ(5, emplace_result.value);
  IntMoveOnlyCopyOnly& v = basic::any_cast<IntMoveOnlyCopyOnly&>(o);
  EXPECT_EQ(5, v.value);
  EXPECT_EQ(&emplace_result, &v);

  static_assert(!CanEmplaceAny<int, int, int>::value, "");
  static_assert(!CanEmplaceAny<MoveOnly, MoveOnly>::value, "");
}

TEST(AnyTest, EmplaceWithCV) {
  const CopyOnly copy_only{};
  basic::any o;
  EXPECT_TRUE(
      (std::is_same<decltype(o.emplace<const volatile IntMoveOnlyCopyOnly>(
                        5, MoveOnly(), copy_only)),
                    IntMoveOnlyCopyOnly&>::value));
  IntMoveOnlyCopyOnly& emplace_result =
      o.emplace<const volatile IntMoveOnlyCopyOnly>(5, MoveOnly(), copy_only);
  EXPECT_EQ(5, emplace_result.value);
  IntMoveOnlyCopyOnly& v = basic::any_cast<IntMoveOnlyCopyOnly&>(o);
  EXPECT_EQ(5, v.value);
  EXPECT_EQ(&emplace_result, &v);
}

TEST(AnyTest, EmplaceWithFunction) {
  basic::any o;
  EXPECT_TRUE(
      (std::is_same<decltype(o.emplace<FunctionType>(FunctionToEmplace)),
                    FunctionType*&>::value));
  FunctionType*& emplace_result = o.emplace<FunctionType>(FunctionToEmplace);
  EXPECT_EQ(&FunctionToEmplace, emplace_result);
}

TEST(AnyTest, EmplaceWithArray) {
  basic::any o;
  ArrayType ar = {5, 42};
  EXPECT_TRUE(
      (std::is_same<decltype(o.emplace<ArrayType>(ar)), DecayedArray&>::value));
  DecayedArray& emplace_result = o.emplace<ArrayType>(ar);
  EXPECT_EQ(&ar[0], emplace_result);
}

TEST(AnyTest, EmplaceIlist) {
  const CopyOnly copy_only{};
  basic::any o;
  EXPECT_TRUE((std::is_same<decltype(o.emplace<ListMoveOnlyCopyOnly>(
                                {1, 2, 3, 4}, MoveOnly(), copy_only)),
                            ListMoveOnlyCopyOnly&>::value));
  ListMoveOnlyCopyOnly& emplace_result =
      o.emplace<ListMoveOnlyCopyOnly>({1, 2, 3, 4}, MoveOnly(), copy_only);
  ListMoveOnlyCopyOnly& v = basic::any_cast<ListMoveOnlyCopyOnly&>(o);
  EXPECT_EQ(&v, &emplace_result);
  std::vector<int> expected_values = {1, 2, 3, 4};
  EXPECT_EQ(expected_values, v.values);

  static_assert(!CanEmplaceAny<int, std::initializer_list<int>>::value, "");
  static_assert(!CanEmplaceAny<MoveOnlyWithListConstructor,
                               std::initializer_list<int>, int>::value,
                "");
}

TEST(AnyTest, EmplaceIlistWithCV) {
  const CopyOnly copy_only{};
  basic::any o;
  EXPECT_TRUE(
      (std::is_same<decltype(o.emplace<const volatile ListMoveOnlyCopyOnly>(
                        {1, 2, 3, 4}, MoveOnly(), copy_only)),
                    ListMoveOnlyCopyOnly&>::value));
  ListMoveOnlyCopyOnly& emplace_result =
      o.emplace<const volatile ListMoveOnlyCopyOnly>({1, 2, 3, 4}, MoveOnly(),
                                                     copy_only);
  ListMoveOnlyCopyOnly& v = basic::any_cast<ListMoveOnlyCopyOnly&>(o);
  EXPECT_EQ(&v, &emplace_result);
  std::vector<int> expected_values = {1, 2, 3, 4};
  EXPECT_EQ(expected_values, v.values);
}

TEST(AnyTest, EmplaceNoArgs) {
  basic::any o;
  o.emplace<int>();
  EXPECT_EQ(0, basic::any_cast<int>(o));
}

TEST(AnyTest, ConversionConstruction) {
  {
    basic::any o = 5;
    EXPECT_EQ(5, basic::any_cast<int>(o));
  }

  {
    const CopyOnly copy_only(5);
    basic::any o = copy_only;
    EXPECT_EQ(5, basic::any_cast<CopyOnly&>(o).value);
  }

  static_assert(!std::is_convertible<MoveOnly, basic::any>::value, "");
}

TEST(AnyTest, ConversionAssignment) {
  {
    basic::any o;
    o = 5;
    EXPECT_EQ(5, basic::any_cast<int>(o));
  }

  {
    const CopyOnly copy_only(5);
    basic::any o;
    o = copy_only;
    EXPECT_EQ(5, basic::any_cast<CopyOnly&>(o).value);
  }

  static_assert(!std::is_assignable<MoveOnly, basic::any>::value, "");
}

// Suppress MSVC warnings.
// 4521: multiple copy constructors specified
// We wrote multiple of them to test that the correct overloads are selected.
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4521)
#endif

// Weird type for testing, only used to make sure we "properly" perfect-forward
// when being placed into an basic::any (use the l-value constructor if given an
// l-value rather than use the copy constructor).
struct WeirdConstructor42 {
  explicit WeirdConstructor42(int value) : value(value) {}

  // Copy-constructor
  WeirdConstructor42(const WeirdConstructor42& other) : value(other.value) {}

  // L-value "weird" constructor (used when given an l-value)
  WeirdConstructor42(
      WeirdConstructor42& /*other*/)  // NOLINT(runtime/references)
      : value(42) {}

  int value;
};
#ifdef _MSC_VER
#pragma warning( pop )
#endif

TEST(AnyTest, WeirdConversionConstruction) {
  {
    const WeirdConstructor42 source(5);
    basic::any o = source;  // Actual copy
    EXPECT_EQ(5, basic::any_cast<WeirdConstructor42&>(o).value);
  }

  {
    WeirdConstructor42 source(5);
    basic::any o = source;  // Weird "conversion"
    EXPECT_EQ(42, basic::any_cast<WeirdConstructor42&>(o).value);
  }
}

TEST(AnyTest, WeirdConversionAssignment) {
  {
    const WeirdConstructor42 source(5);
    basic::any o;
    o = source;  // Actual copy
    EXPECT_EQ(5, basic::any_cast<WeirdConstructor42&>(o).value);
  }

  {
    WeirdConstructor42 source(5);
    basic::any o;
    o = source;  // Weird "conversion"
    EXPECT_EQ(42, basic::any_cast<WeirdConstructor42&>(o).value);
  }
}

struct Value {};

TEST(AnyTest, AnyCastValue) {
  {
    basic::any o;
    o.emplace<int>(5);
    EXPECT_EQ(5, basic::any_cast<int>(o));
    EXPECT_EQ(5, basic::any_cast<int>(AsConst(o)));
    static_assert(
        std::is_same<decltype(basic::any_cast<Value>(o)), Value>::value, "");
  }

  {
    basic::any o;
    o.emplace<int>(5);
    EXPECT_EQ(5, basic::any_cast<const int>(o));
    EXPECT_EQ(5, basic::any_cast<const int>(AsConst(o)));
    static_assert(std::is_same<decltype(basic::any_cast<const Value>(o)),
                               const Value>::value,
                  "");
  }
}

TEST(AnyTest, AnyCastReference) {
  {
    basic::any o;
    o.emplace<int>(5);
    EXPECT_EQ(5, basic::any_cast<int&>(o));
    EXPECT_EQ(5, basic::any_cast<const int&>(AsConst(o)));
    static_assert(
        std::is_same<decltype(basic::any_cast<Value&>(o)), Value&>::value, "");
  }

  {
    basic::any o;
    o.emplace<int>(5);
    EXPECT_EQ(5, basic::any_cast<const int>(o));
    EXPECT_EQ(5, basic::any_cast<const int>(AsConst(o)));
    static_assert(std::is_same<decltype(basic::any_cast<const Value&>(o)),
                               const Value&>::value,
                  "");
  }

  {
    basic::any o;
    o.emplace<int>(5);
    EXPECT_EQ(5, basic::any_cast<int&&>(std::move(o)));
    static_assert(std::is_same<decltype(basic::any_cast<Value&&>(std::move(o))),
                               Value&&>::value,
                  "");
  }

  {
    basic::any o;
    o.emplace<int>(5);
    EXPECT_EQ(5, basic::any_cast<const int>(std::move(o)));
    static_assert(
        std::is_same<decltype(basic::any_cast<const Value&&>(std::move(o))),
                     const Value&&>::value,
        "");
  }
}

TEST(AnyTest, AnyCastPointer) {
  {
    basic::any o;
    EXPECT_EQ(nullptr, basic::any_cast<char>(&o));
    o.emplace<int>(5);
    EXPECT_EQ(nullptr, basic::any_cast<char>(&o));
    o.emplace<char>('a');
    EXPECT_EQ('a', *basic::any_cast<char>(&o));
    static_assert(
        std::is_same<decltype(basic::any_cast<Value>(&o)), Value*>::value, "");
  }

  {
    basic::any o;
    EXPECT_EQ(nullptr, basic::any_cast<const char>(&o));
    o.emplace<int>(5);
    EXPECT_EQ(nullptr, basic::any_cast<const char>(&o));
    o.emplace<char>('a');
    EXPECT_EQ('a', *basic::any_cast<const char>(&o));
    static_assert(std::is_same<decltype(basic::any_cast<const Value>(&o)),
                               const Value*>::value,
                  "");
  }
}

TEST(AnyTest, MakeAny) {
  const CopyOnly copy_only{};
  auto o = basic::make_any<IntMoveOnlyCopyOnly>(5, MoveOnly(), copy_only);
  static_assert(std::is_same<decltype(o), basic::any>::value, "");
  EXPECT_EQ(5, basic::any_cast<IntMoveOnlyCopyOnly&>(o).value);
}

TEST(AnyTest, MakeAnyIList) {
  const CopyOnly copy_only{};
  auto o =
      basic::make_any<ListMoveOnlyCopyOnly>({1, 2, 3}, MoveOnly(), copy_only);
  static_assert(std::is_same<decltype(o), basic::any>::value, "");
  ListMoveOnlyCopyOnly& v = basic::any_cast<ListMoveOnlyCopyOnly&>(o);
  std::vector<int> expected_values = {1, 2, 3};
  EXPECT_EQ(expected_values, v.values);
}

// Test the use of copy constructor and operator=
TEST(AnyTest, Copy) {
  InstanceTracker tracker_raii;

  {
    basic::any o(basic::in_place_type<CopyableOnlyInstance>, 123);
    CopyableOnlyInstance* f1 = basic::any_cast<CopyableOnlyInstance>(&o);

    basic::any o2(o);
    const CopyableOnlyInstance* f2 = basic::any_cast<CopyableOnlyInstance>(&o2);
    EXPECT_EQ(123, f2->value());
    EXPECT_NE(f1, f2);

    basic::any o3;
    o3 = o2;
    const CopyableOnlyInstance* f3 = basic::any_cast<CopyableOnlyInstance>(&o3);
    EXPECT_EQ(123, f3->value());
    EXPECT_NE(f2, f3);

    const basic::any o4(4);
    // copy construct from const lvalue ref.
    basic::any o5 = o4;
    EXPECT_EQ(4, basic::any_cast<int>(o4));
    EXPECT_EQ(4, basic::any_cast<int>(o5));

    // Copy construct from const rvalue ref.
    basic::any o6 = std::move(o4);  // NOLINT
    EXPECT_EQ(4, basic::any_cast<int>(o4));
    EXPECT_EQ(4, basic::any_cast<int>(o6));
  }
}

TEST(AnyTest, Move) {
  InstanceTracker tracker_raii;

  basic::any any1;
  any1.emplace<CopyableOnlyInstance>(5);

  // This is a copy, so copy count increases to 1.
  basic::any any2 = any1;
  EXPECT_EQ(5, basic::any_cast<CopyableOnlyInstance&>(any1).value());
  EXPECT_EQ(5, basic::any_cast<CopyableOnlyInstance&>(any2).value());
  EXPECT_EQ(1, tracker_raii.copies());

  // This isn't a copy, so copy count doesn't increase.
  basic::any any3 = std::move(any2);
  EXPECT_EQ(5, basic::any_cast<CopyableOnlyInstance&>(any3).value());
  EXPECT_EQ(1, tracker_raii.copies());

  basic::any any4;
  any4 = std::move(any3);
  EXPECT_EQ(5, basic::any_cast<CopyableOnlyInstance&>(any4).value());
  EXPECT_EQ(1, tracker_raii.copies());

  basic::any tmp4(4);
  basic::any o4(std::move(tmp4));  // move construct
  EXPECT_EQ(4, basic::any_cast<int>(o4));
  o4 = *&o4;  // self assign
  EXPECT_EQ(4, basic::any_cast<int>(o4));
  EXPECT_TRUE(o4.has_value());

  basic::any o5;
  basic::any tmp5(5);
  o5 = std::move(tmp5);  // move assign
  EXPECT_EQ(5, basic::any_cast<int>(o5));
}

// Reset the ObjectOwner with an object of a different type
TEST(AnyTest, Reset) {
  basic::any o;
  o.emplace<int>();

  o.reset();
  EXPECT_FALSE(o.has_value());

  o.emplace<char>();
  EXPECT_TRUE(o.has_value());
}

TEST(AnyTest, ConversionConstructionCausesOneCopy) {
  InstanceTracker tracker_raii;
  CopyableOnlyInstance counter(5);
  basic::any o(counter);
  EXPECT_EQ(5, basic::any_cast<CopyableOnlyInstance&>(o).value());
  EXPECT_EQ(1, tracker_raii.copies());
}

//////////////////////////////////
// Tests for Exception Behavior //
//////////////////////////////////

#if defined(ABSL_HAVE_STD_ANY)

// If using a std `any` implementation, we can't check for a specific message.
#define ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(...)                      \
  ABSL_BASE_INTERNAL_EXPECT_FAIL((__VA_ARGS__), basic::bad_any_cast, \
                                 "")

#else

// If using the basic `any` implementation, we can rely on a specific message.
#define ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(...)                      \
  ABSL_BASE_INTERNAL_EXPECT_FAIL((__VA_ARGS__), basic::bad_any_cast, \
                                 "Bad any cast")

#endif  // defined(ABSL_HAVE_STD_ANY)

TEST(AnyTest, ThrowBadAlloc) {
  {
    basic::any a;
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<int&>(a));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const int&>(a));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<int&&>(basic::any{}));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const int&&>(basic::any{}));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<int>(a));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const int>(a));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<int>(basic::any{}));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const int>(basic::any{}));

    // const basic::any operand
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const int&>(AsConst(a)));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<int>(AsConst(a)));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const int>(AsConst(a)));
  }

  {
    basic::any a(basic::in_place_type<int>);
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<float&>(a));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const float&>(a));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<float&&>(basic::any{}));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(
        basic::any_cast<const float&&>(basic::any{}));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<float>(a));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const float>(a));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<float>(basic::any{}));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const float>(basic::any{}));

    // const basic::any operand
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const float&>(AsConst(a)));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<float>(AsConst(a)));
    ABSL_ANY_TEST_EXPECT_BAD_ANY_CAST(basic::any_cast<const float>(AsConst(a)));
  }
}

class BadCopy {};

struct BadCopyable {
  BadCopyable() = default;
  BadCopyable(BadCopyable&&) = default;
  BadCopyable(const BadCopyable&) {
#ifdef ABSL_HAVE_EXCEPTIONS
    throw BadCopy();
#else
    ABSL_RAW_LOG(FATAL, "Bad copy");
#endif
  }
};

#define ABSL_ANY_TEST_EXPECT_BAD_COPY(...) \
  ABSL_BASE_INTERNAL_EXPECT_FAIL((__VA_ARGS__), BadCopy, "Bad copy")

// Test the guarantees regarding exceptions in copy/assign.
TEST(AnyTest, FailedCopy) {
  {
    const BadCopyable bad{};
    ABSL_ANY_TEST_EXPECT_BAD_COPY(basic::any{bad});
  }

  {
    basic::any src(basic::in_place_type<BadCopyable>);
    ABSL_ANY_TEST_EXPECT_BAD_COPY(basic::any{src});
  }

  {
    BadCopyable bad;
    basic::any target;
    ABSL_ANY_TEST_EXPECT_BAD_COPY(target = bad);
  }

  {
    BadCopyable bad;
    basic::any target(basic::in_place_type<BadCopyable>);
    ABSL_ANY_TEST_EXPECT_BAD_COPY(target = bad);
    EXPECT_TRUE(target.has_value());
  }

  {
    basic::any src(basic::in_place_type<BadCopyable>);
    basic::any target;
    ABSL_ANY_TEST_EXPECT_BAD_COPY(target = src);
    EXPECT_FALSE(target.has_value());
  }

  {
    basic::any src(basic::in_place_type<BadCopyable>);
    basic::any target(basic::in_place_type<BadCopyable>);
    ABSL_ANY_TEST_EXPECT_BAD_COPY(target = src);
    EXPECT_TRUE(target.has_value());
  }
}

// Test the guarantees regarding exceptions in emplace.
TEST(AnyTest, FailedEmplace) {
  {
    BadCopyable bad;
    basic::any target;
    ABSL_ANY_TEST_EXPECT_BAD_COPY(target.emplace<BadCopyable>(bad));
  }

  {
    BadCopyable bad;
    basic::any target(basic::in_place_type<int>);
    ABSL_ANY_TEST_EXPECT_BAD_COPY(target.emplace<BadCopyable>(bad));
#if defined(ABSL_HAVE_STD_ANY) && defined(__GLIBCXX__)
    // libstdc++ std::any::emplace() implementation (as of 7.2) has a bug: if an
    // exception is thrown, *this contains a value.
#define ABSL_GLIBCXX_ANY_EMPLACE_EXCEPTION_BUG 1
#endif
#if defined(ABSL_HAVE_EXCEPTIONS) && \
    !defined(ABSL_GLIBCXX_ANY_EMPLACE_EXCEPTION_BUG)
    EXPECT_FALSE(target.has_value());
#endif
  }
}

}  // namespace
