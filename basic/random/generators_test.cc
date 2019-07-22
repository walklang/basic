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

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "basic/random/distributions.h"
#include "basic/random/random.h"

namespace {

template <typename URBG>
void TestUniform(URBG* gen) {
  // [a, b) default-semantics, inferred types.
  basic::Uniform(*gen, 0, 100);     // int
  basic::Uniform(*gen, 0, 1.0);     // Promoted to double
  basic::Uniform(*gen, 0.0f, 1.0);  // Promoted to double
  basic::Uniform(*gen, 0.0, 1.0);   // double
  basic::Uniform(*gen, -1, 1L);     // Promoted to long

  // Roll a die.
  basic::Uniform(basic::IntervalClosedClosed, *gen, 1, 6);

  // Get a fraction.
  basic::Uniform(basic::IntervalOpenOpen, *gen, 0.0, 1.0);

  // Assign a value to a random element.
  std::vector<int> elems = {10, 20, 30, 40, 50};
  elems[basic::Uniform(*gen, 0u, elems.size())] = 5;
  elems[basic::Uniform<size_t>(*gen, 0, elems.size())] = 3;

  // Choose some epsilon around zero.
  basic::Uniform(basic::IntervalOpenOpen, *gen, -1.0, 1.0);

  // (a, b) semantics, inferred types.
  basic::Uniform(basic::IntervalOpenOpen, *gen, 0, 1.0);  // Promoted to double

  // Explict overriding of types.
  basic::Uniform<int>(*gen, 0, 100);
  basic::Uniform<int8_t>(*gen, 0, 100);
  basic::Uniform<int16_t>(*gen, 0, 100);
  basic::Uniform<uint16_t>(*gen, 0, 100);
  basic::Uniform<int32_t>(*gen, 0, 1 << 10);
  basic::Uniform<uint32_t>(*gen, 0, 1 << 10);
  basic::Uniform<int64_t>(*gen, 0, 1 << 10);
  basic::Uniform<uint64_t>(*gen, 0, 1 << 10);

  basic::Uniform<float>(*gen, 0.0, 1.0);
  basic::Uniform<float>(*gen, 0, 1);
  basic::Uniform<float>(*gen, -1, 1);
  basic::Uniform<double>(*gen, 0.0, 1.0);

  basic::Uniform<float>(*gen, -1.0, 0);
  basic::Uniform<double>(*gen, -1.0, 0);

  // Tagged
  basic::Uniform<double>(basic::IntervalClosedClosed, *gen, 0, 1);
  basic::Uniform<double>(basic::IntervalClosedOpen, *gen, 0, 1);
  basic::Uniform<double>(basic::IntervalOpenOpen, *gen, 0, 1);
  basic::Uniform<double>(basic::IntervalOpenClosed, *gen, 0, 1);
  basic::Uniform<double>(basic::IntervalClosedClosed, *gen, 0, 1);
  basic::Uniform<double>(basic::IntervalOpenOpen, *gen, 0, 1);

  basic::Uniform<int>(basic::IntervalClosedClosed, *gen, 0, 100);
  basic::Uniform<int>(basic::IntervalClosedOpen, *gen, 0, 100);
  basic::Uniform<int>(basic::IntervalOpenOpen, *gen, 0, 100);
  basic::Uniform<int>(basic::IntervalOpenClosed, *gen, 0, 100);
  basic::Uniform<int>(basic::IntervalClosedClosed, *gen, 0, 100);
  basic::Uniform<int>(basic::IntervalOpenOpen, *gen, 0, 100);

  // With *generator as an R-value reference.
  basic::Uniform<int>(URBG(), 0, 100);
  basic::Uniform<double>(URBG(), 0.0, 1.0);
}

template <typename URBG>
void TestExponential(URBG* gen) {
  basic::Exponential<float>(*gen);
  basic::Exponential<double>(*gen);
  basic::Exponential<double>(URBG());
}

template <typename URBG>
void TestPoisson(URBG* gen) {
  // [rand.dist.pois] Indicates that the std::poisson_distribution
  // is parameterized by IntType, however MSVC does not allow 8-bit
  // types.
  basic::Poisson<int>(*gen);
  basic::Poisson<int16_t>(*gen);
  basic::Poisson<uint16_t>(*gen);
  basic::Poisson<int32_t>(*gen);
  basic::Poisson<uint32_t>(*gen);
  basic::Poisson<int64_t>(*gen);
  basic::Poisson<uint64_t>(*gen);
  basic::Poisson<uint64_t>(URBG());
}

template <typename URBG>
void TestBernoulli(URBG* gen) {
  basic::Bernoulli(*gen, 0.5);
  basic::Bernoulli(*gen, 0.5);
}

template <typename URBG>
void TestZipf(URBG* gen) {
  basic::Zipf<int>(*gen, 100);
  basic::Zipf<int8_t>(*gen, 100);
  basic::Zipf<int16_t>(*gen, 100);
  basic::Zipf<uint16_t>(*gen, 100);
  basic::Zipf<int32_t>(*gen, 1 << 10);
  basic::Zipf<uint32_t>(*gen, 1 << 10);
  basic::Zipf<int64_t>(*gen, 1 << 10);
  basic::Zipf<uint64_t>(*gen, 1 << 10);
  basic::Zipf<uint64_t>(URBG(), 1 << 10);
}

template <typename URBG>
void TestGaussian(URBG* gen) {
  basic::Gaussian<float>(*gen, 1.0, 1.0);
  basic::Gaussian<double>(*gen, 1.0, 1.0);
  basic::Gaussian<double>(URBG(), 1.0, 1.0);
}

template <typename URBG>
void TestLogNormal(URBG* gen) {
  basic::LogUniform<int>(*gen, 0, 100);
  basic::LogUniform<int8_t>(*gen, 0, 100);
  basic::LogUniform<int16_t>(*gen, 0, 100);
  basic::LogUniform<uint16_t>(*gen, 0, 100);
  basic::LogUniform<int32_t>(*gen, 0, 1 << 10);
  basic::LogUniform<uint32_t>(*gen, 0, 1 << 10);
  basic::LogUniform<int64_t>(*gen, 0, 1 << 10);
  basic::LogUniform<uint64_t>(*gen, 0, 1 << 10);
  basic::LogUniform<uint64_t>(URBG(), 0, 1 << 10);
}

template <typename URBG>
void CompatibilityTest() {
  URBG gen;

  TestUniform(&gen);
  TestExponential(&gen);
  TestPoisson(&gen);
  TestBernoulli(&gen);
  TestZipf(&gen);
  TestGaussian(&gen);
  TestLogNormal(&gen);
}

TEST(std_mt19937_64, Compatibility) {
  // Validate with std::mt19937_64
  CompatibilityTest<std::mt19937_64>();
}

TEST(BitGen, Compatibility) {
  // Validate with basic::BitGen
  CompatibilityTest<basic::BitGen>();
}

TEST(InsecureBitGen, Compatibility) {
  // Validate with basic::InsecureBitGen
  CompatibilityTest<basic::InsecureBitGen>();
}

}  // namespace
