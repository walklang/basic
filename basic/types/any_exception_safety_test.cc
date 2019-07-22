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

#include <typeinfo>
#include <vector>

#include "gtest/gtest.h"
#include "basic/base/internal/exception_safety_testing.h"

using Thrower = testing::ThrowingValue<>;
using NoThrowMoveThrower =
    testing::ThrowingValue<testing::TypeSpec::kNoThrowMove>;
using ThrowerList = std::initializer_list<Thrower>;
using ThrowerVec = std::vector<Thrower>;
using ThrowingAlloc = testing::ThrowingAllocator<Thrower>;
using ThrowingThrowerVec = std::vector<Thrower, ThrowingAlloc>;

namespace {

testing::AssertionResult AnyInvariants(basic::any* a) {
  using testing::AssertionFailure;
  using testing::AssertionSuccess;

  if (a->has_value()) {
    if (a->type() == typeid(void)) {
      return AssertionFailure()
             << "A non-empty any should not have type `void`";
    }
  } else {
    if (a->type() != typeid(void)) {
      return AssertionFailure()
             << "An empty any should have type void, but has type "
             << a->type().name();
    }
  }

  //  Make sure that reset() changes any to a valid state.
  a->reset();
  if (a->has_value()) {
    return AssertionFailure() << "A reset `any` should be valueless";
  }
  if (a->type() != typeid(void)) {
    return AssertionFailure() << "A reset `any` should have type() of `void`, "
                                 "but instead has type "
                              << a->type().name();
  }
  try {
    auto unused = basic::any_cast<Thrower>(*a);
    static_cast<void>(unused);
    return AssertionFailure()
           << "A reset `any` should not be able to be any_cast";
  } catch (const basic::bad_any_cast&) {
  } catch (...) {
    return AssertionFailure()
           << "Unexpected exception thrown from basic::any_cast";
  }
  return AssertionSuccess();
}

testing::AssertionResult AnyIsEmpty(basic::any* a) {
  if (!a->has_value()) {
    return testing::AssertionSuccess();
  }
  return testing::AssertionFailure()
         << "a should be empty, but instead has value "
         << basic::any_cast<Thrower>(*a).Get();
}

TEST(AnyExceptionSafety, Ctors) {
  Thrower val(1);
  testing::TestThrowingCtor<basic::any>(val);

  Thrower copy(val);
  testing::TestThrowingCtor<basic::any>(copy);

  testing::TestThrowingCtor<basic::any>(basic::in_place_type_t<Thrower>(), 1);

  testing::TestThrowingCtor<basic::any>(basic::in_place_type_t<ThrowerVec>(),
                                       ThrowerList{val});

  testing::TestThrowingCtor<basic::any,
                            basic::in_place_type_t<ThrowingThrowerVec>,
                            ThrowerList, ThrowingAlloc>(
      basic::in_place_type_t<ThrowingThrowerVec>(), {val}, ThrowingAlloc());
}

TEST(AnyExceptionSafety, Assignment) {
  auto original =
      basic::any(basic::in_place_type_t<Thrower>(), 1, testing::nothrow_ctor);
  auto any_is_strong = [original](basic::any* ap) {
    return testing::AssertionResult(ap->has_value() &&
                                    basic::any_cast<Thrower>(original) ==
                                        basic::any_cast<Thrower>(*ap));
  };
  auto any_strong_tester = testing::MakeExceptionSafetyTester()
                               .WithInitialValue(original)
                               .WithContracts(AnyInvariants, any_is_strong);

  Thrower val(2);
  basic::any any_val(val);
  NoThrowMoveThrower mv_val(2);

  auto assign_any = [&any_val](basic::any* ap) { *ap = any_val; };
  auto assign_val = [&val](basic::any* ap) { *ap = val; };
  auto move = [&val](basic::any* ap) { *ap = std::move(val); };
  auto move_movable = [&mv_val](basic::any* ap) { *ap = std::move(mv_val); };

  EXPECT_TRUE(any_strong_tester.Test(assign_any));
  EXPECT_TRUE(any_strong_tester.Test(assign_val));
  EXPECT_TRUE(any_strong_tester.Test(move));
  EXPECT_TRUE(any_strong_tester.Test(move_movable));

  auto empty_any_is_strong = [](basic::any* ap) {
    return testing::AssertionResult{!ap->has_value()};
  };
  auto strong_empty_any_tester =
      testing::MakeExceptionSafetyTester()
          .WithInitialValue(basic::any{})
          .WithContracts(AnyInvariants, empty_any_is_strong);

  EXPECT_TRUE(strong_empty_any_tester.Test(assign_any));
  EXPECT_TRUE(strong_empty_any_tester.Test(assign_val));
  EXPECT_TRUE(strong_empty_any_tester.Test(move));
}

// libstdc++ std::any fails this test
#if !defined(ABSL_HAVE_STD_ANY)
TEST(AnyExceptionSafety, Emplace) {
  auto initial_val =
      basic::any{basic::in_place_type_t<Thrower>(), 1, testing::nothrow_ctor};
  auto one_tester = testing::MakeExceptionSafetyTester()
                        .WithInitialValue(initial_val)
                        .WithContracts(AnyInvariants, AnyIsEmpty);

  auto emp_thrower = [](basic::any* ap) { ap->emplace<Thrower>(2); };
  auto emp_throwervec = [](basic::any* ap) {
    std::initializer_list<Thrower> il{Thrower(2, testing::nothrow_ctor)};
    ap->emplace<ThrowerVec>(il);
  };
  auto emp_movethrower = [](basic::any* ap) {
    ap->emplace<NoThrowMoveThrower>(2);
  };

  EXPECT_TRUE(one_tester.Test(emp_thrower));
  EXPECT_TRUE(one_tester.Test(emp_throwervec));
  EXPECT_TRUE(one_tester.Test(emp_movethrower));

  auto empty_tester = one_tester.WithInitialValue(basic::any{});

  EXPECT_TRUE(empty_tester.Test(emp_thrower));
  EXPECT_TRUE(empty_tester.Test(emp_throwervec));
}
#endif  // ABSL_HAVE_STD_ANY

}  // namespace
