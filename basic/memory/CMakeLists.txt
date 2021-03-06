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
    memory
  HDRS
    "memory.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    basic::core_headers
    basic::meta
  PUBLIC
)

basic_cc_test(
  NAME
    memory_test
  SRCS
    "memory_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::memory
    basic::base
    basic::core_headers
    gmock_main
)

basic_cc_test(
  NAME
    memory_exception_safety_test
  SRCS
    "memory_exception_safety_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
    ${ABSL_EXCEPTIONS_FLAG}
  LINKOPTS
    ${ABSL_EXCEPTIONS_FLAG_LINKOPTS}
  DEPS
    basic::memory
    basic::exception_safety_testing
    gmock_main
)
