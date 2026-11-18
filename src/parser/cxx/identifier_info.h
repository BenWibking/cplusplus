// Copyright (c) 2026 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cxx/token_fwd.h>

#include <cstdint>
#include <string_view>

namespace cxx {

// One ordinal in the generated spelling table. Each family is a contiguous
// range in its token enum's order. Zero denotes an ordinary identifier.
class IdentifierInfo {
 public:
  IdentifierInfo() = default;
  explicit IdentifierInfo(WellKnownName name);
  [[nodiscard]] static auto classify(std::string_view spelling)
      -> IdentifierInfo;

  [[nodiscard]] auto spelling() const -> std::string_view;
  [[nodiscard]] auto builtinTypeTrait() const -> BuiltinTypeTraitKind;
  [[nodiscard]] auto unaryBuiltinType() const -> UnaryBuiltinTypeKind;
  [[nodiscard]] auto builtinFunction() const -> BuiltinFunctionKind;
  [[nodiscard]] auto builtinTemplate() const -> BuiltinTemplateKind;
  [[nodiscard]] auto wellKnownName() const -> WellKnownName;

 private:
  explicit IdentifierInfo(std::uint16_t ordinal) : ordinal_(ordinal) {}

  // Index into the immutable spelling table, independent of source lifetime.
  std::uint16_t ordinal_ = 0;
};

}  // namespace cxx
