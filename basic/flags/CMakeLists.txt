#
# Copyright 2019 The Basic Authors.
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

# Internal-only target, do not depend on directly.
basic_cc_library(
  NAME
    flags_internal
  SRCS
    "internal/program_name.cc"
  HDRS
    "internal/path_util.h"
    "internal/program_name.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    basic::strings
    basic::synchronization
  PUBLIC
)

basic_cc_library(
  NAME
    flags_config
  SRCS
    "usage_config.cc"
  HDRS
    "config.h"
    "usage_config.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    basic::flags_internal
    basic::core_headers
    basic::strings
    basic::synchronization
)

basic_cc_library(
  NAME
    flags_marshalling
  SRCS
    "marshalling.cc"
  HDRS
    "marshalling.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    basic::core_headers
    basic::strings
    basic::str_format
)

# Internal-only target, do not depend on directly.
basic_cc_library(
  NAME
    flags_handle
  SRCS
    "internal/commandlineflag.cc"
  HDRS
    "internal/commandlineflag.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    basic::flags_config
    basic::flags_marshalling
    basic::base
    basic::core_headers
    basic::strings
    basic::synchronization
    basic::optional
)

# Internal-only target, do not depend on directly.
basic_cc_library(
  NAME
    flags_registry
  SRCS
    "internal/registry.cc"
    "internal/type_erased.cc"
  HDRS
    "internal/registry.h"
    "internal/type_erased.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    basic::flags_config
    basic::flags_handle
    basic::base
    basic::core_headers
    basic::dynamic_annotations
    basic::strings
    basic::synchronization
)

basic_cc_library(
  NAME
    flags
  SRCS
    "flag.cc"
  HDRS
    "declare.h"
    "flag.h"
    "internal/flag.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    basic::flags_config
    basic::flags_handle
    basic::flags_marshalling
    basic::flags_registry
    basic::base
    basic::core_headers
    basic::strings
)

# Internal-only target, do not depend on directly.
basic_cc_library(
  NAME
    flags_usage_internal
  SRCS
    "internal/usage.cc"
  HDRS
    "internal/usage.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    basic::flags_config
    basic::flags
    basic::flags_handle
    basic::flags_internal
    basic::strings
    basic::synchronization
)

basic_cc_library(
  NAME
    flags_usage
  SRCS
    "usage.cc"
  HDRS
    "usage.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    basic::flags_usage_internal
    basic::strings
    basic::synchronization
)

basic_cc_library(
  NAME
    flags_parse
  SRCS
    "parse.cc"
  HDRS
    "internal/parse.h"
    "parse.h"
  COPTS
    ${ABSL_DEFAULT_COPTS}
  LINKOPTS
    ${ABSL_DEFAULT_LINKOPTS}
  DEPS
    basic::flags_config
    basic::flags
    basic::flags_handle
    basic::flags_internal
    basic::flags_registry
    basic::flags_usage
    basic::strings
    basic::synchronization
)

############################################################################
# Unit tests in alpahabetical order.

basic_cc_test(
  NAME
    flags_commandlineflag_test
  SRCS
    "internal/commandlineflag_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags
    basic::flags_handle
    basic::flags_registry
    basic::memory
    basic::strings
    gtest_main
)

basic_cc_test(
  NAME
    flags_config_test
  SRCS
    "config_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags_config
    gtest_main
)

basic_cc_test(
  NAME
    flags_flag_test
  SRCS
    "flag_test.cc"
    "flag_test_defs.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags
    basic::strings
    gtest_main
)

basic_cc_test(
  NAME
    flags_marshalling_test
  SRCS
    "marshalling_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags_marshalling
    gtest_main
)

basic_cc_test(
  NAME
    flags_parse_test
  SRCS
    "parse_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags
    basic::base
    basic::flags_parse
    basic::scoped_set_env
    basic::span
    basic::strings
    gmock_main
)

basic_cc_test(
  NAME
    flags_path_util_test
  SRCS
    "internal/path_util_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags_internal
    gtest_main
)

basic_cc_test(
  NAME
    flags_program_name_test
  SRCS
    "internal/program_name_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags_internal
    basic::strings
    gtest_main
)

basic_cc_test(
  NAME
    flags_type_erased_test
  SRCS
    "internal/type_erased_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags
    basic::flags_registry
    basic::memory
    basic::strings
    gtest_main
)

basic_cc_test(
  NAME
    flags_usage_config_test
  SRCS
    "usage_config_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags_config
    basic::flags_internal
    basic::strings
    gtest_main
)

basic_cc_test(
  NAME
    flags_usage_test
  SRCS
    "internal/usage_test.cc"
  COPTS
    ${ABSL_TEST_COPTS}
  DEPS
    basic::flags_config
    basic::flags
    basic::flags_internal
    basic::flags_parse
    basic::flags_usage
    basic::memory
    basic::strings
    gtest
)
