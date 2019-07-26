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

#include <string>

#include "basic/flags/internal/path_util.h"
#include "basic/synchronization/mutex.h"

namespace basic {
namespace flags_internal {

ABSL_CONST_INIT static basic::Mutex program_name_guard(basic::kConstInit);
ABSL_CONST_INIT static std::string* program_name
    GUARDED_BY(program_name_guard) = nullptr;

std::string ProgramInvocationName() {
  basic::MutexLock l(&program_name_guard);

  return program_name ? *program_name : "UNKNOWN";
}

std::string ShortProgramInvocationName() {
  basic::MutexLock l(&program_name_guard);

  return program_name ? std::string(flags_internal::Basename(*program_name))
                      : "UNKNOWN";
}

void SetProgramInvocationName(basic::string_view prog_name_str) {
  basic::MutexLock l(&program_name_guard);

  if (!program_name)
    program_name = new std::string(prog_name_str);
  else
    program_name->assign(prog_name_str.data(), prog_name_str.size());
}

}  // namespace flags_internal
}  // namespace basic
