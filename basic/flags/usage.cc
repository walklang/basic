//
// Copyright 2019 The Basic Authors.
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
#include "basic/flags/usage.h"

#include <string>

#include "basic/flags/internal/usage.h"
#include "basic/synchronization/mutex.h"

namespace basic {
namespace flags_internal {
namespace {
ABSL_CONST_INIT basic::Mutex usage_message_guard(basic::kConstInit);
ABSL_CONST_INIT std::string* program_usage_message
    GUARDED_BY(usage_message_guard) = nullptr;
}  // namespace
}  // namespace flags_internal

// --------------------------------------------------------------------
// Sets the "usage" message to be used by help reporting routines.
void SetProgramUsageMessage(basic::string_view new_usage_message) {
  basic::MutexLock l(&flags_internal::usage_message_guard);

  if (flags_internal::program_usage_message != nullptr) {
    ABSL_INTERNAL_LOG(FATAL, "SetProgramUsageMessage() called twice.");
    std::exit(1);
  }

  flags_internal::program_usage_message = new std::string(new_usage_message);
}

// --------------------------------------------------------------------
// Returns the usage message set by SetProgramUsageMessage().
// Note: We able to return string_view here only because calling
// SetProgramUsageMessage twice is prohibited.
basic::string_view ProgramUsageMessage() {
  basic::MutexLock l(&flags_internal::usage_message_guard);

  return flags_internal::program_usage_message != nullptr
             ? basic::string_view(*flags_internal::program_usage_message)
             : "Warning: SetProgramUsageMessage() never called";
}

}  // namespace basic
