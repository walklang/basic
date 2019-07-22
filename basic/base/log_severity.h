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

#ifndef ABSL_BASE_INTERNAL_LOG_SEVERITY_H_
#define ABSL_BASE_INTERNAL_LOG_SEVERITY_H_

#include <array>
#include <ostream>

#include "basic/base/attributes.h"

namespace basic {

// Four severity levels are defined.  Logging APIs should terminate the program
// when a message is logged at severity `kFatal`; the other levels have no
// special semantics.
enum class LogSeverity : int {
  kInfo = 0,
  kWarning = 1,
  kError = 2,
  kFatal = 3,
};

// Returns an iterable of all standard `basic::LogSeverity` values, ordered from
// least to most severe.
constexpr std::array<basic::LogSeverity, 4> LogSeverities() {
  return {{basic::LogSeverity::kInfo, basic::LogSeverity::kWarning,
           basic::LogSeverity::kError, basic::LogSeverity::kFatal}};
}

// Returns the all-caps string representation (e.g. "INFO") of the specified
// severity level if it is one of the normal levels and "UNKNOWN" otherwise.
constexpr const char* LogSeverityName(basic::LogSeverity s) {
  return s == basic::LogSeverity::kInfo
             ? "INFO"
             : s == basic::LogSeverity::kWarning
                   ? "WARNING"
                   : s == basic::LogSeverity::kError
                         ? "ERROR"
                         : s == basic::LogSeverity::kFatal ? "FATAL" : "UNKNOWN";
}

// Values less than `kInfo` normalize to `kInfo`; values greater than `kFatal`
// normalize to `kError` (**NOT** `kFatal`).
constexpr basic::LogSeverity NormalizeLogSeverity(basic::LogSeverity s) {
  return s < basic::LogSeverity::kInfo
             ? basic::LogSeverity::kInfo
             : s > basic::LogSeverity::kFatal ? basic::LogSeverity::kError : s;
}
constexpr basic::LogSeverity NormalizeLogSeverity(int s) {
  return NormalizeLogSeverity(static_cast<basic::LogSeverity>(s));
}

// The exact representation of a streamed `basic::LogSeverity` is deliberately
// unspecified; do not rely on it.
std::ostream& operator<<(std::ostream& os, basic::LogSeverity s);

}  // namespace basic

#endif  // ABSL_BASE_INTERNAL_LOG_SEVERITY_H_
