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

// See also //basic/synchronization:mutex_benchmark for a comparison of SpinLock
// and Mutex performance under varying levels of contention.

#include "basic/base/internal/raw_logging.h"
#include "basic/base/internal/scheduling_mode.h"
#include "basic/base/internal/spinlock.h"
#include "basic/synchronization/internal/create_thread_identity.h"
#include "benchmark/benchmark.h"

namespace {

template <basic::base_internal::SchedulingMode scheduling_mode>
static void BM_SpinLock(benchmark::State& state) {
  // Ensure a ThreadIdentity is installed.
  ABSL_INTERNAL_CHECK(
      basic::synchronization_internal::GetOrCreateCurrentThreadIdentity() !=
          nullptr,
      "GetOrCreateCurrentThreadIdentity() failed");

  static auto* spinlock = new basic::base_internal::SpinLock(scheduling_mode);
  for (auto _ : state) {
    basic::base_internal::SpinLockHolder holder(spinlock);
  }
}

BENCHMARK_TEMPLATE(BM_SpinLock,
                   basic::base_internal::SCHEDULE_KERNEL_ONLY)
    ->UseRealTime()
    ->Threads(1)
    ->ThreadPerCpu();

BENCHMARK_TEMPLATE(BM_SpinLock,
                   basic::base_internal::SCHEDULE_COOPERATIVE_AND_KERNEL)
    ->UseRealTime()
    ->Threads(1)
    ->ThreadPerCpu();

}  // namespace
