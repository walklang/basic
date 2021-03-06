//
//  Copyright 2019 The Basic Authors.
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

#include "basic/flags/internal/program_name.h"

#include "gtest/gtest.h"
#include "basic/strings/match.h"

namespace {

namespace flags = basic::flags_internal;

TEST(FlagsPathUtilTest, TestInitialProgamName) {
  flags::SetProgramInvocationName("basic/flags/program_name_test");
  std::string program_name = flags::ProgramInvocationName();
  for (char& c : program_name)
    if (c == '\\') c = '/';

#if !defined(__wasm__) && !defined(__asmjs__)
  const std::string expect_name = "basic/flags/program_name_test";
  const std::string expect_basename = "program_name_test";
#else
  // For targets that generate javascript or webassembly the invocation name
  // has the special value below.
  const std::string expect_name = "this.program";
  const std::string expect_basename = "this.program";
#endif

  EXPECT_TRUE(basic::EndsWith(program_name, expect_name)) << program_name;
  EXPECT_EQ(flags::ShortProgramInvocationName(), expect_basename);
}

TEST(FlagsPathUtilTest, TestProgamNameInterfaces) {
  flags::SetProgramInvocationName("a/my_test");

  EXPECT_EQ(flags::ProgramInvocationName(), "a/my_test");
  EXPECT_EQ(flags::ShortProgramInvocationName(), "my_test");

  basic::string_view not_null_terminated("basic/aaa/bbb");
  not_null_terminated = not_null_terminated.substr(1, 10);

  flags::SetProgramInvocationName(not_null_terminated);

  EXPECT_EQ(flags::ProgramInvocationName(), "bsl/aaa/bb");
  EXPECT_EQ(flags::ShortProgramInvocationName(), "bb");
}

}  // namespace
