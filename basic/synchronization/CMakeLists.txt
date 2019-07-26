#
# Copyright 2017 The Basic Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

basic_cc_library(
  NAME
    graphcycles_internal
  HDRS
    "internal/graphcycles.h"
  SRCS
    "internal/graphcycles.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    basic::base
    basic::base_internal
    basic::core_headers
    basic::malloc_internal
)

basic_cc_library(
  NAME
    synchronization
  HDRS
    "barrier.h"
    "blocking_counter.h"
    "internal/create_thread_identity.h"
    "internal/kernel_timeout.h"
    "internal/mutex_nonprod.inc"
    "internal/per_thread_sem.h"
    "internal/waiter.h"
    "mutex.h"
    "notification.h"
  SRCS
    "barrier.cc"
    "blocking_counter.cc"
    "internal/create_thread_identity.cc"
    "internal/per_thread_sem.cc"
    "internal/waiter.cc"
    "notification.cc"
    "mutex.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    basic::graphcycles_internal
    basic::atomic_hook
    basic::base
    basic::base_internal
    basic::config
    basic::core_headers
    basic::dynamic_annotations
    basic::malloc_internal
    basic::stacktrace
    basic::symbolize
    basic::time
    Threads::Threads
  PUBLIC
)

basic_cc_test(
  NAME
    barrier_test
  SRCS
    "barrier_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::synchronization
    basic::time
    gmock_main
)

basic_cc_test(
  NAME
    blocking_counter_test
  SRCS
    "blocking_counter_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::synchronization
    basic::time
    gmock_main
)

basic_cc_test(
  NAME
    graphcycles_test
  SRCS
    "internal/graphcycles_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::graphcycles_internal
    basic::base
    basic::core_headers
    gmock_main
)

basic_cc_library(
  NAME
    thread_pool
  HDRS
    "internal/thread_pool.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    basic::synchronization
    basic::core_headers
  TESTONLY
)

basic_cc_test(
  NAME
    mutex_test
  SRCS
    "mutex_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::synchronization
    basic::thread_pool
    basic::base
    basic::core_headers
    basic::memory
    basic::time
    gmock_main
)

basic_cc_test(
  NAME
    notification_test
  SRCS
    "notification_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::synchronization
    basic::time
    gmock_main
)

basic_cc_library(
  NAME
    per_thread_sem_test_common
  SRCS
    "internal/per_thread_sem_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::synchronization
    basic::base
    basic::strings
    basic::time
    gmock
  TESTONLY
)

basic_cc_test(
  NAME
    per_thread_sem_test
  SRCS
    "internal/per_thread_sem_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::per_thread_sem_test_common
    basic::synchronization
    basic::base
    basic::strings
    basic::time
    gmock_main
)

basic_cc_test(
  NAME
    lifetime_test
  SRCS
    "lifetime_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::synchronization
    basic::base
    basic::core_headers
)
