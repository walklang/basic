//
// Copyright 2018 The Basic Authors.
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
//
#ifndef ABSL_RANDOM_DISTRIBUTION_FORMAT_TRAITS_H_
#define ABSL_RANDOM_DISTRIBUTION_FORMAT_TRAITS_H_

#include <string>
#include <tuple>
#include <typeinfo>

#include "basic/meta/type_traits.h"
#include "basic/random/bernoulli_distribution.h"
#include "basic/random/beta_distribution.h"
#include "basic/random/exponential_distribution.h"
#include "basic/random/gaussian_distribution.h"
#include "basic/random/log_uniform_int_distribution.h"
#include "basic/random/poisson_distribution.h"
#include "basic/random/uniform_int_distribution.h"
#include "basic/random/uniform_real_distribution.h"
#include "basic/random/zipf_distribution.h"
#include "basic/strings/str_cat.h"
#include "basic/strings/str_join.h"
#include "basic/strings/string_view.h"
#include "basic/types/span.h"

namespace basic {
namespace random_internal {

// ScalarTypeName defines a preferred hierarchy of preferred type names for
// scalars, and is evaluated at compile time for the specific type
// specialization.
template <typename T>
constexpr const char* ScalarTypeName() {
  static_assert(std::is_integral<T>() || std::is_floating_point<T>(), "");
  // clang-format off
    return
        std::is_same<T, float>::value ? "float" :
        std::is_same<T, double>::value ? "double" :
        std::is_same<T, long double>::value ? "long double" :
        std::is_same<T, bool>::value ? "bool" :
        std::is_signed<T>::value && sizeof(T) == 1 ? "int8_t" :
        std::is_signed<T>::value && sizeof(T) == 2 ? "int16_t" :
        std::is_signed<T>::value && sizeof(T) == 4 ? "int32_t" :
        std::is_signed<T>::value && sizeof(T) == 8 ? "int64_t" :
        std::is_unsigned<T>::value && sizeof(T) == 1 ? "uint8_t" :
        std::is_unsigned<T>::value && sizeof(T) == 2 ? "uint16_t" :
        std::is_unsigned<T>::value && sizeof(T) == 4 ? "uint32_t" :
        std::is_unsigned<T>::value && sizeof(T) == 8 ? "uint64_t" :
            "undefined";
  // clang-format on

  // NOTE: It would be nice to use typeid(T).name(), but that's an
  // implementation-defined attribute which does not necessarily
  // correspond to a name. We could potentially demangle it
  // using, e.g. abi::__cxa_demangle.
}

// Distribution traits used by DistributionCaller and internal implementation
// details of the mocking framework.
/*
struct DistributionFormatTraits {
   // Returns the parameterized name of the distribution function.
   static constexpr const char* FunctionName()
   // Format DistrT parameters.
   static std::string FormatArgs(DistrT& dist);
   // Format DistrT::result_type results.
   static std::string FormatResults(DistrT& dist);
};
*/
template <typename DistrT>
struct DistributionFormatTraits;

template <typename R>
struct DistributionFormatTraits<basic::uniform_int_distribution<R>> {
  using distribution_t = basic::uniform_int_distribution<R>;
  using result_t = typename distribution_t::result_type;

  static constexpr const char* Name() { return "Uniform"; }

  static std::string FunctionName() {
    return basic::StrCat(Name(), "<", ScalarTypeName<R>(), ">");
  }
  static std::string FormatArgs(const distribution_t& d) {
    return basic::StrCat("basic::IntervalClosedClosed, ", (d.min)(), ", ",
                        (d.max)());
  }
  static std::string FormatResults(basic::Span<const result_t> results) {
    return basic::StrJoin(results, ", ");
  }
};

template <typename R>
struct DistributionFormatTraits<basic::uniform_real_distribution<R>> {
  using distribution_t = basic::uniform_real_distribution<R>;
  using result_t = typename distribution_t::result_type;

  static constexpr const char* Name() { return "Uniform"; }

  static std::string FunctionName() {
    return basic::StrCat(Name(), "<", ScalarTypeName<R>(), ">");
  }
  static std::string FormatArgs(const distribution_t& d) {
    return basic::StrCat((d.min)(), ", ", (d.max)());
  }
  static std::string FormatResults(basic::Span<const result_t> results) {
    return basic::StrJoin(results, ", ");
  }
};

template <typename R>
struct DistributionFormatTraits<basic::exponential_distribution<R>> {
  using distribution_t = basic::exponential_distribution<R>;
  using result_t = typename distribution_t::result_type;

  static constexpr const char* Name() { return "Exponential"; }

  static std::string FunctionName() {
    return basic::StrCat(Name(), "<", ScalarTypeName<R>(), ">");
  }
  static std::string FormatArgs(const distribution_t& d) {
    return basic::StrCat(d.lambda());
  }
  static std::string FormatResults(basic::Span<const result_t> results) {
    return basic::StrJoin(results, ", ");
  }
};

template <typename R>
struct DistributionFormatTraits<basic::poisson_distribution<R>> {
  using distribution_t = basic::poisson_distribution<R>;
  using result_t = typename distribution_t::result_type;

  static constexpr const char* Name() { return "Poisson"; }

  static std::string FunctionName() {
    return basic::StrCat(Name(), "<", ScalarTypeName<R>(), ">");
  }
  static std::string FormatArgs(const distribution_t& d) {
    return basic::StrCat(d.mean());
  }
  static std::string FormatResults(basic::Span<const result_t> results) {
    return basic::StrJoin(results, ", ");
  }
};

template <>
struct DistributionFormatTraits<basic::bernoulli_distribution> {
  using distribution_t = basic::bernoulli_distribution;
  using result_t = typename distribution_t::result_type;

  static constexpr const char* Name() { return "Bernoulli"; }

  static constexpr const char* FunctionName() { return Name(); }
  static std::string FormatArgs(const distribution_t& d) {
    return basic::StrCat(d.p());
  }
  static std::string FormatResults(basic::Span<const result_t> results) {
    return basic::StrJoin(results, ", ");
  }
};

template <typename R>
struct DistributionFormatTraits<basic::beta_distribution<R>> {
  using distribution_t = basic::beta_distribution<R>;
  using result_t = typename distribution_t::result_type;

  static constexpr const char* Name() { return "Beta"; }

  static std::string FunctionName() {
    return basic::StrCat(Name(), "<", ScalarTypeName<R>(), ">");
  }
  static std::string FormatArgs(const distribution_t& d) {
    return basic::StrCat(d.alpha(), ", ", d.beta());
  }
  static std::string FormatResults(basic::Span<const result_t> results) {
    return basic::StrJoin(results, ", ");
  }
};

template <typename R>
struct DistributionFormatTraits<basic::zipf_distribution<R>> {
  using distribution_t = basic::zipf_distribution<R>;
  using result_t = typename distribution_t::result_type;

  static constexpr const char* Name() { return "Zipf"; }

  static std::string FunctionName() {
    return basic::StrCat(Name(), "<", ScalarTypeName<R>(), ">");
  }
  static std::string FormatArgs(const distribution_t& d) {
    return basic::StrCat(d.k(), ", ", d.v(), ", ", d.q());
  }
  static std::string FormatResults(basic::Span<const result_t> results) {
    return basic::StrJoin(results, ", ");
  }
};

template <typename R>
struct DistributionFormatTraits<basic::gaussian_distribution<R>> {
  using distribution_t = basic::gaussian_distribution<R>;
  using result_t = typename distribution_t::result_type;

  static constexpr const char* Name() { return "Gaussian"; }

  static std::string FunctionName() {
    return basic::StrCat(Name(), "<", ScalarTypeName<R>(), ">");
  }
  static std::string FormatArgs(const distribution_t& d) {
    return basic::StrJoin(std::make_tuple(d.mean(), d.stddev()), ", ");
  }
  static std::string FormatResults(basic::Span<const result_t> results) {
    return basic::StrJoin(results, ", ");
  }
};

template <typename R>
struct DistributionFormatTraits<basic::log_uniform_int_distribution<R>> {
  using distribution_t = basic::log_uniform_int_distribution<R>;
  using result_t = typename distribution_t::result_type;

  static constexpr const char* Name() { return "LogUniform"; }

  static std::string FunctionName() {
    return basic::StrCat(Name(), "<", ScalarTypeName<R>(), ">");
  }
  static std::string FormatArgs(const distribution_t& d) {
    return basic::StrJoin(std::make_tuple((d.min)(), (d.max)(), d.base()), ", ");
  }
  static std::string FormatResults(basic::Span<const result_t> results) {
    return basic::StrJoin(results, ", ");
  }
};

}  // namespace random_internal
}  // namespace basic

#endif  // ABSL_RANDOM_DISTRIBUTION_FORMAT_TRAITS_H_
