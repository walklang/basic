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

#include "basic/debugging/failure_signal_handler.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "gtest/gtest.h"
#include "basic/base/internal/raw_logging.h"
#include "basic/debugging/stacktrace.h"
#include "basic/debugging/symbolize.h"
#include "basic/strings/match.h"
#include "basic/strings/str_cat.h"

namespace {

#if GTEST_HAS_DEATH_TEST

// For the parameterized death tests. GetParam() returns the signal number.
using FailureSignalHandlerDeathTest = ::testing::TestWithParam<int>;

// This function runs in a fork()ed process on most systems.
void InstallHandlerAndRaise(int signo) {
  basic::InstallFailureSignalHandler(basic::FailureSignalHandlerOptions());
  raise(signo);
}

TEST_P(FailureSignalHandlerDeathTest, AbslFailureSignal) {
  const int signo = GetParam();
  std::string exit_regex = basic::StrCat(
      "\\*\\*\\* ", basic::debugging_internal::FailureSignalToString(signo),
      " received at time=");
#ifndef _WIN32
  EXPECT_EXIT(InstallHandlerAndRaise(signo), testing::KilledBySignal(signo),
              exit_regex);
#else
  // Windows doesn't have testing::KilledBySignal().
  EXPECT_DEATH(InstallHandlerAndRaise(signo), exit_regex);
#endif
}

ABSL_CONST_INIT FILE* error_file = nullptr;

void WriteToErrorFile(const char* msg) {
  if (msg != nullptr) {
    ABSL_RAW_CHECK(fwrite(msg, strlen(msg), 1, error_file) == 1,
                   "fwrite() failed");
  }
  ABSL_RAW_CHECK(fflush(error_file) == 0, "fflush() failed");
}

std::string GetTmpDir() {
  // TEST_TMPDIR is set by Bazel. Try the others when not running under Bazel.
  static const char* const kTmpEnvVars[] = {"TEST_TMPDIR", "TMPDIR", "TEMP",
                                            "TEMPDIR", "TMP"};
  for (const char* const var : kTmpEnvVars) {
    const char* tmp_dir = std::getenv(var);
    if (tmp_dir != nullptr) {
      return tmp_dir;
    }
  }

  // Try something reasonable.
  return "/tmp";
}

// This function runs in a fork()ed process on most systems.
void InstallHandlerWithWriteToFileAndRaise(const char* file, int signo) {
  error_file = fopen(file, "w");
  ABSL_RAW_CHECK(error_file != nullptr, "Failed create error_file");
  basic::FailureSignalHandlerOptions options;
  options.writerfn = WriteToErrorFile;
  basic::InstallFailureSignalHandler(options);
  raise(signo);
}

TEST_P(FailureSignalHandlerDeathTest, AbslFatalSignalsWithWriterFn) {
  const int signo = GetParam();
  std::string tmp_dir = GetTmpDir();
  std::string file = basic::StrCat(tmp_dir, "/signo_", signo);

  std::string exit_regex = basic::StrCat(
      "\\*\\*\\* ", basic::debugging_internal::FailureSignalToString(signo),
      " received at time=");
#ifndef _WIN32
  EXPECT_EXIT(InstallHandlerWithWriteToFileAndRaise(file.c_str(), signo),
              testing::KilledBySignal(signo), exit_regex);
#else
  // Windows doesn't have testing::KilledBySignal().
  EXPECT_DEATH(InstallHandlerWithWriteToFileAndRaise(file.c_str(), signo),
               exit_regex);
#endif

  // Open the file in this process and check its contents.
  std::fstream error_output(file);
  ASSERT_TRUE(error_output.is_open()) << file;
  std::string error_line;
  std::getline(error_output, error_line);
  EXPECT_TRUE(basic::StartsWith(
      error_line,
      basic::StrCat("*** ",
                   basic::debugging_internal::FailureSignalToString(signo),
                   " received at ")));

  if (basic::debugging_internal::StackTraceWorksForTest()) {
    std::getline(error_output, error_line);
    EXPECT_TRUE(basic::StartsWith(error_line, "PC: "));
  }
}

constexpr int kFailureSignals[] = {
    SIGSEGV, SIGILL,  SIGFPE, SIGABRT, SIGTERM,
#ifndef _WIN32
    SIGBUS,  SIGTRAP,
#endif
};

std::string SignalParamToString(const ::testing::TestParamInfo<int>& info) {
  std::string result =
      basic::debugging_internal::FailureSignalToString(info.param);
  if (result.empty()) {
    result = basic::StrCat(info.param);
  }
  return result;
}

INSTANTIATE_TEST_SUITE_P(AbslDeathTest, FailureSignalHandlerDeathTest,
                         ::testing::ValuesIn(kFailureSignals),
                         SignalParamToString);

#endif  // GTEST_HAS_DEATH_TEST

}  // namespace

int main(int argc, char** argv) {
  basic::InitializeSymbolizer(argv[0]);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
