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
//
// -----------------------------------------------------------------------------
// File: marshalling.h
// -----------------------------------------------------------------------------
//
// This header file defines the API for extending Basic flag support to
// custom types, and defines the set of overloads for fundamental types.
//
// Out of the box, the Basic flags library supports the following types:
//
// * `bool`
// * `int16_t`
// * `uint16_t`
// * `int32_t`
// * `uint32_t`
// * `int64_t`
// * `uint64_t`
// * `float`
// * `double`
// * `std::string`
// * `std::vector<std::string>`
//
// Note that support for integral types is implemented using overloads for
// variable-width fundamental types (`short`, `int`, `long`, etc.). However,
// you should prefer the fixed-width integral types (`int32_t`, `uint64_t`,
// etc.) we've noted above within flag definitions.

//
// In addition, several Basic libraries provide their own custom support for
// Basic flags.
//
// The Basic time library provides the following support for civil time values:
//
// * `basic::CivilSecond`
// * `basic::CivilMinute`
// * `basic::CivilHour`
// * `basic::CivilDay`
// * `basic::CivilMonth`
// * `basic::CivilYear`
//
// and also provides support for the following absolute time values:
//
// * `basic::Duration`
// * `basic::Time`
//
// Additional support for Basic types will be noted here as it is added.
//
// You can also provide your own custom flags by adding overloads for
// `AbslParseFlag()` and `AbslUnparseFlag()` to your type definitions. (See
// below.)
//
// -----------------------------------------------------------------------------
// Adding Type Support for Basic Flags
// -----------------------------------------------------------------------------
//
// To add support for your user-defined type, add overloads of `AbslParseFlag()`
// and `AbslUnparseFlag()` as free (non-member) functions to your type. If `T`
// is a class type, these functions can be friend function definitions. These
// overloads must be added to the same namespace where the type is defined, so
// that they can be discovered by Argument-Dependent Lookup (ADL).
//
// Example:
//
//   namespace foo {
//
//   enum OutputMode { kPlainText, kHtml };
//
//   // AbslParseFlag converts from a string to OutputMode.
//   // Must be in same namespace as OutputMode.
//
//   // Parses an OutputMode from the command line flag value `text. Returns
//   // `true` and sets `*mode` on success; returns `false` and sets `*error`
//   // on failure.
//   bool AbslParseFlag(basic::string_view text,
//                      OutputMode* mode,
//                      std::string* error) {
//     if (text == "plaintext") {
//       *mode = kPlainText;
//       return true;
//     }
//     if (text == "html") {
//       *mode = kHtml;
//      return true;
//     }
//     *error = "unknown value for enumeration";
//     return false;
//  }
//
//  // AbslUnparseFlag converts from an OutputMode to a string.
//  // Must be in same namespace as OutputMode.
//
//  // Returns a textual flag value corresponding to the OutputMode `mode`.
//  std::string AbslUnparseFlag(OutputMode mode) {
//    switch (mode) {
//      case kPlainText: return "plaintext";
//      case kHtml: return "html";
//    }
//    return basic::StrCat(mode);
//  }
//
// Notice that neither `AbslParseFlag()` nor `AbslUnparseFlag()` are class
// members, but free functions. `AbslParseFlag/AbslUnparseFlag()` overloads
// for a type should only be declared in the same file and namespace as said
// type. The proper `AbslParseFlag/AbslUnparseFlag()` implementations for a
// given type will be discovered via Argument-Dependent Lookup (ADL).
//
// `AbslParseFlag()` may need, in turn, to parse simpler constituent types
// using `basic::ParseFlag()`. For example, a custom struct `MyFlagType`
// consisting of a `std::pair<int, std::string>` would add an `AbslParseFlag()`
// overload for its `MyFlagType` like so:
//
// Example:
//
//   namespace my_flag_type {
//
//   struct MyFlagType {
//     std::pair<int, std::string> my_flag_data;
//   };
//
//   bool AbslParseFlag(basic::string_view text, MyFlagType* flag,
//                      std::string* err);
//
//   std::string AbslUnparseFlag(const MyFlagType&);
//
//   // Within the implementation, `AbslParseFlag()` will, in turn invoke
//   // `basic::ParseFlag()` on its constituent `int` and `std::string` types
//   // (which have built-in Basic flag support.
//
//   bool AbslParseFlag(basic::string_view text, MyFlagType* flag,
//                      std::string* err) {
//     std::pair<basic::string_view, basic::string_view> tokens =
//         basic::StrSplit(text, ',');
//     if (!basic::ParseFlag(tokens.first, &flag->my_flag_data.first, err))
//         return false;
//     if (!basic::ParseFlag(tokens.second, &flag->my_flag_data.second, err))
//         return false;
//     return true;
//   }
//
//   // Similarly, for unparsing, we can simply invoke `basic::UnparseFlag()` on
//   // the constituent types.
//   std::string AbslUnparseFlag(const MyFlagType& flag) {
//     return basic::StrCat(basic::UnparseFlag(flag.my_flag_data.first),
//                         ",",
//                         basic::UnparseFlag(flag.my_flag_data.second));
//   }
#ifndef ABSL_FLAGS_MARSHALLING_H_
#define ABSL_FLAGS_MARSHALLING_H_

#include <string>
#include <vector>

#include "basic/strings/string_view.h"

namespace basic {
namespace flags_internal {

// Overloads of `AbslParseFlag()` and `AbslUnparseFlag()` for fundamental types.
bool AbslParseFlag(basic::string_view, bool*, std::string*);
bool AbslParseFlag(basic::string_view, short*, std::string*);           // NOLINT
bool AbslParseFlag(basic::string_view, unsigned short*, std::string*);  // NOLINT
bool AbslParseFlag(basic::string_view, int*, std::string*);             // NOLINT
bool AbslParseFlag(basic::string_view, unsigned int*, std::string*);    // NOLINT
bool AbslParseFlag(basic::string_view, long*, std::string*);            // NOLINT
bool AbslParseFlag(basic::string_view, unsigned long*, std::string*);   // NOLINT
bool AbslParseFlag(basic::string_view, long long*, std::string*);       // NOLINT
bool AbslParseFlag(basic::string_view, unsigned long long*,
                   std::string*);  // NOLINT
bool AbslParseFlag(basic::string_view, float*, std::string*);
bool AbslParseFlag(basic::string_view, double*, std::string*);
bool AbslParseFlag(basic::string_view, std::string*, std::string*);
bool AbslParseFlag(basic::string_view, std::vector<std::string>*, std::string*);

template <typename T>
bool InvokeParseFlag(basic::string_view input, T* dst, std::string* err) {
  // Comment on next line provides a good compiler error message if T
  // does not have AbslParseFlag(basic::string_view, T*, std::string*).
  return AbslParseFlag(input, dst, err);  // Is T missing AbslParseFlag?
}

// Strings and std:: containers do not have the same overload resolution
// considerations as fundamental types. Naming these 'AbslUnparseFlag' means we
// can avoid the need for additional specializations of Unparse (below).
std::string AbslUnparseFlag(basic::string_view v);
std::string AbslUnparseFlag(const std::vector<std::string>&);

template <typename T>
std::string Unparse(const T& v) {
  // Comment on next line provides a good compiler error message if T does not
  // have UnparseFlag.
  return AbslUnparseFlag(v);  // Is T missing AbslUnparseFlag?
}

// Overloads for builtin types.
std::string Unparse(bool v);
std::string Unparse(short v);               // NOLINT
std::string Unparse(unsigned short v);      // NOLINT
std::string Unparse(int v);                 // NOLINT
std::string Unparse(unsigned int v);        // NOLINT
std::string Unparse(long v);                // NOLINT
std::string Unparse(unsigned long v);       // NOLINT
std::string Unparse(long long v);           // NOLINT
std::string Unparse(unsigned long long v);  // NOLINT
std::string Unparse(float v);
std::string Unparse(double v);

}  // namespace flags_internal

// ParseFlag()
//
// Parses a string value into a flag value of type `T`. Do not add overloads of
// this function for your type directly; instead, add an `AbslParseFlag()`
// free function as documented above.
//
// Some implementations of `AbslParseFlag()` for types which consist of other,
// constituent types which already have Basic flag support, may need to call
// `basic::ParseFlag()` on those consituent string values. (See above.)
template <typename T>
inline bool ParseFlag(basic::string_view input, T* dst, std::string* error) {
  return flags_internal::InvokeParseFlag(input, dst, error);
}

// UnparseFlag()
//
// Unparses a flag value of type `T` into a string value. Do not add overloads
// of this function for your type directly; instead, add an `AbslUnparseFlag()`
// free function as documented above.
//
// Some implementations of `AbslUnparseFlag()` for types which consist of other,
// constituent types which already have Basic flag support, may want to call
// `basic::UnparseFlag()` on those constituent types. (See above.)
template <typename T>
inline std::string UnparseFlag(const T& v) {
  return flags_internal::Unparse(v);
}

}  // namespace basic

#endif  // ABSL_FLAGS_MARSHALLING_H_
