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

#include <cxx/identifier_info.h>
#include <cxx/private/identifier_index-priv.h>
#include <cxx/token.h>

#include <iterator>

namespace cxx {
namespace index = identifier_index;
namespace {

template <typename Kind>
auto kindInRange(unsigned ordinal, unsigned first, unsigned end) -> Kind {
  if (ordinal < first || ordinal >= end) return Kind::T_NONE;
  return static_cast<Kind>(ordinal - first + 1);
}

}  // namespace

IdentifierInfo::IdentifierInfo(WellKnownName name)
    : ordinal_(name == WellKnownName::T_NONE
                   ? 0
                   : static_cast<std::uint16_t>(index::firstWellKnown +
                                                static_cast<unsigned>(name) -
                                                1)) {}

auto IdentifierInfo::classify(std::string_view spelling) -> IdentifierInfo {
  if (spelling.size() < index::minLength ||
      spelling.size() >= index::minLength + std::size(index::positions))
    return {};
  auto lengthIndex = spelling.size() - index::minLength;

  auto byte =
      static_cast<unsigned char>(spelling[index::positions[lengthIndex]]);
  auto bucket = lengthIndex * index::bucketCount + byte % index::bucketCount;
  for (auto i = index::starts[bucket]; i < index::starts[bucket + 1]; ++i) {
    auto ordinal = index::candidates[i];
    auto text = index::spellings + index::offsets[ordinal];
    if (std::char_traits<char>::compare(text, spelling.data(),
                                        spelling.size()) == 0)
      return IdentifierInfo(ordinal);
  }
  return {};
}

auto IdentifierInfo::spelling() const -> std::string_view {
  auto start = index::offsets[ordinal_];
  return {index::spellings + start,
          static_cast<std::size_t>(index::offsets[ordinal_ + 1] - start)};
}

auto IdentifierInfo::builtinTypeTrait() const -> BuiltinTypeTraitKind {
  return kindInRange<BuiltinTypeTraitKind>(ordinal_, index::firstTypeTrait,
                                           index::firstUnaryType);
}

auto IdentifierInfo::unaryBuiltinType() const -> UnaryBuiltinTypeKind {
  return kindInRange<UnaryBuiltinTypeKind>(ordinal_, index::firstUnaryType,
                                           index::firstFunction);
}

auto IdentifierInfo::builtinFunction() const -> BuiltinFunctionKind {
  return kindInRange<BuiltinFunctionKind>(ordinal_, index::firstFunction,
                                          index::firstTemplate);
}

auto IdentifierInfo::builtinTemplate() const -> BuiltinTemplateKind {
  return kindInRange<BuiltinTemplateKind>(ordinal_, index::firstTemplate,
                                          index::firstWellKnown);
}

auto IdentifierInfo::wellKnownName() const -> WellKnownName {
  return kindInRange<WellKnownName>(ordinal_, index::firstWellKnown,
                                    index::end);
}

auto Token::builtinFunctionKind(std::string_view spelling)
    -> BuiltinFunctionKind {
  return IdentifierInfo::classify(spelling).builtinFunction();
}

}  // namespace cxx
