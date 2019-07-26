#
# Copyright 2018 The Basic Authors.
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
    hash
  HDRS
    "hash.h"
  SRCS
    "internal/hash.cc"
    "internal/hash.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    basic::core_headers
    basic::endian
    basic::fixed_array
    basic::meta
    basic::int128
    basic::strings
    basic::optional
    basic::variant
    basic::utility
    basic::city
  PUBLIC
)

basic_cc_library(
  NAME
    hash_testing
  HDRS
    "hash_testing.h"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::spy_hash_state
    basic::meta
    basic::strings
    basic::variant
    gmock
  TESTONLY
)

basic_cc_test(
  NAME
    hash_test
  SRCS
    "hash_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::hash
    basic::hash_testing
    basic::core_headers
    basic::flat_hash_set
    basic::spy_hash_state
    basic::meta
    basic::int128
    gmock_main
)

basic_cc_library(
  NAME
    spy_hash_state
  HDRS
    "internal/spy_hash_state.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    basic::hash
    basic::strings
    basic::str_format
  TESTONLY
)

basic_cc_library(
  NAME
    city
  HDRS
    "internal/city.h"
  SRCS
    "internal/city.cc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    basic::config
    basic::core_headers
    basic::endian
)

basic_cc_test(
  NAME
    city_test
  SRCS
    "internal/city_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::city
    gmock_main
)

