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

#include "basic/random/seed_sequences.h"

#include <iterator>
#include <random>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "basic/random/internal/nonsecure_base.h"
#include "basic/random/random.h"
namespace {

TEST(SeedSequences, Examples) {
  {
    basic::SeedSeq seed_seq({1, 2, 3});
    basic::BitGen bitgen(seed_seq);

    EXPECT_NE(0, bitgen());
  }
  {
    basic::BitGen engine;
    auto seed_seq = basic::CreateSeedSeqFrom(&engine);
    basic::BitGen bitgen(seed_seq);

    EXPECT_NE(engine(), bitgen());
  }
  {
    auto seed_seq = basic::MakeSeedSeq();
    std::mt19937 random(seed_seq);

    EXPECT_NE(0, random());
  }
}

TEST(CreateSeedSeqFrom, CompatibleWithStdTypes) {
  using ExampleNonsecureURBG =
      basic::random_internal::NonsecureURBGBase<std::minstd_rand0>;

  // Construct a URBG instance.
  ExampleNonsecureURBG rng;

  // Construct a Seed Sequence from its variates.
  auto seq_from_rng = basic::CreateSeedSeqFrom(&rng);

  // Ensure that another URBG can be validly constructed from the Seed Sequence.
  std::mt19937_64{seq_from_rng};
}

TEST(CreateSeedSeqFrom, CompatibleWithBitGenerator) {
  // Construct a URBG instance.
  basic::BitGen rng;

  // Construct a Seed Sequence from its variates.
  auto seq_from_rng = basic::CreateSeedSeqFrom(&rng);

  // Ensure that another URBG can be validly constructed from the Seed Sequence.
  std::mt19937_64{seq_from_rng};
}

TEST(CreateSeedSeqFrom, CompatibleWithInsecureBitGen) {
  // Construct a URBG instance.
  basic::InsecureBitGen rng;

  // Construct a Seed Sequence from its variates.
  auto seq_from_rng = basic::CreateSeedSeqFrom(&rng);

  // Ensure that another URBG can be validly constructed from the Seed Sequence.
  std::mt19937_64{seq_from_rng};
}

TEST(CreateSeedSeqFrom, CompatibleWithRawURBG) {
  // Construct a URBG instance.
  std::random_device urandom;

  // Construct a Seed Sequence from its variates, using 64b of seed-material.
  auto seq_from_rng = basic::CreateSeedSeqFrom(&urandom);

  // Ensure that another URBG can be validly constructed from the Seed Sequence.
  std::mt19937_64{seq_from_rng};
}

template <typename URBG>
void TestReproducibleVariateSequencesForNonsecureURBG() {
  const size_t kNumVariates = 1000;

  // Master RNG instance.
  URBG rng;
  // Reused for both RNG instances.
  auto reusable_seed = basic::CreateSeedSeqFrom(&rng);

  typename URBG::result_type variates[kNumVariates];
  {
    URBG child(reusable_seed);
    for (auto& variate : variates) {
      variate = child();
    }
  }
  // Ensure that variate-sequence can be "replayed" by identical RNG.
  {
    URBG child(reusable_seed);
    for (auto& variate : variates) {
      ASSERT_EQ(variate, child());
    }
  }
}

TEST(CreateSeedSeqFrom, ReproducesVariateSequencesForInsecureBitGen) {
  TestReproducibleVariateSequencesForNonsecureURBG<basic::InsecureBitGen>();
}

TEST(CreateSeedSeqFrom, ReproducesVariateSequencesForBitGenerator) {
  TestReproducibleVariateSequencesForNonsecureURBG<basic::BitGen>();
}
}  // namespace
