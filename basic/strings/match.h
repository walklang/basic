//
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
//
// -----------------------------------------------------------------------------
// File: match.h
// -----------------------------------------------------------------------------
//
// This file contains simple utilities for performing string matching checks.
// All of these function parameters are specified as `basic::string_view`,
// meaning that these functions can accept `std::string`, `basic::string_view` or
// nul-terminated C-style strings.
//
// Examples:
//   std::string s = "foo";
//   basic::string_view sv = "f";
//   assert(basic::StrContains(s, sv));
//
// Note: The order of parameters in these functions is designed to mimic the
// order an equivalent member function would exhibit;
// e.g. `s.Contains(x)` ==> `basic::StrContains(s, x).
#ifndef ABSL_STRINGS_MATCH_H_
#define ABSL_STRINGS_MATCH_H_

#include <cstring>

#include "basic/strings/string_view.h"

namespace basic {

// StrContains()
//
// Returns whether a given string `haystack` contains the substring `needle`.
inline bool StrContains(basic::string_view haystack, basic::string_view needle) {
  return haystack.find(needle, 0) != haystack.npos;
}

// StartsWith()
//
// Returns whether a given string `text` begins with `prefix`.
inline bool StartsWith(basic::string_view text, basic::string_view prefix) {
  return prefix.empty() ||
         (text.size() >= prefix.size() &&
          memcmp(text.data(), prefix.data(), prefix.size()) == 0);
}

// EndsWith()
//
// Returns whether a given string `text` ends with `suffix`.
inline bool EndsWith(basic::string_view text, basic::string_view suffix) {
  return suffix.empty() ||
         (text.size() >= suffix.size() &&
          memcmp(text.data() + (text.size() - suffix.size()), suffix.data(),
                 suffix.size()) == 0);
}

// EqualsIgnoreCase()
//
// Returns whether given ASCII strings `piece1` and `piece2` are equal, ignoring
// case in the comparison.
bool EqualsIgnoreCase(basic::string_view piece1, basic::string_view piece2);

// StartsWithIgnoreCase()
//
// Returns whether a given ASCII string `text` starts with `prefix`,
// ignoring case in the comparison.
bool StartsWithIgnoreCase(basic::string_view text, basic::string_view prefix);

// EndsWithIgnoreCase()
//
// Returns whether a given ASCII string `text` ends with `suffix`, ignoring
// case in the comparison.
bool EndsWithIgnoreCase(basic::string_view text, basic::string_view suffix);

}  // namespace basic

#endif  // ABSL_STRINGS_MATCH_H_
