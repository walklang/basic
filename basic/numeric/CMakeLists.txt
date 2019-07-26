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
    int128
  HDRS
    "int128.h"
  SRCS
    "int128.cc"
    "int128_have_intrinsic.inc"
    "int128_no_intrinsic.inc"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    basic::config
    basic::core_headers
  PUBLIC
)

basic_cc_test(
  NAME
    int128_test
  SRCS
    "int128_stream_test.cc"
    "int128_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::int128
    basic::base
    basic::core_headers
    basic::hash_testing
    basic::type_traits
    gmock_main
)

# component target
basic_cc_library(
  NAME
    numeric
  COPTS
    ${ABSL_DEFAULT_COPTS}
  DEPS
    basic::int128
  PUBLIC
)
