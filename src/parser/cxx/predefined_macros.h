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

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <string_view>

namespace cxx {

// An immutable toolchain definition. Tables are sorted by identifier, contain
// one definition per identifier, and outlive every preprocessor that borrows
// them.
class PredefinedMacro {
 public:
  // The directive after '#define ', including any formals and replacement.
  template <std::size_t N>
  consteval PredefinedMacro(const char (&text)[N])
      : text_(text), length_(N - 1), identifierLength_(N - 1) {
    static_assert(N - 1 <= UINT32_MAX);
    const auto end = spelling().find_first_of("( \t\v\f\r\n");
    if (end != std::string_view::npos)
      identifierLength_ = static_cast<std::uint32_t>(end);
  }

  [[nodiscard]] constexpr auto spelling() const -> std::string_view {
    return {text_, length_};
  }

  [[nodiscard]] constexpr auto identifier() const -> std::string_view {
    return {text_, identifierLength_};
  }

 private:
  // Borrowed bytes shared by lookup and tokenization.
  const char* text_;
  // Byte length of the entire directive, excluding its terminating NUL.
  std::uint32_t length_;
  // Byte length of the identifier at the beginning of the directive.
  std::uint32_t identifierLength_;
};

// Compile-time index of a borrowed, sorted definition table. Negative lookup
// filters depend on spelling families, not the number of definitions in them.
class PredefinedMacros {
 public:
  consteval explicit PredefinedMacros(
      std::span<const PredefinedMacro> definitions)
      : definitions_(definitions) {
    if (definitions.empty()) return;
    const auto first = definitions.front().identifier();
    const auto last = definitions.back().identifier();
    std::size_t common = 0;
    while (common < first.size() && common < last.size() &&
           first[common] == last[common]) {
      ++common;
    }
    prefix_ = first.substr(0, common);
    std::string_view previous;
    for (const auto& definition : definitions) {
      const auto name = definition.identifier();
      if (name.empty() || (!previous.empty() && previous >= name)) {
        std::abort();
      }
      previous = name;
      initials_ |= initialMask(name);
      lengths_ |= std::uint64_t{1} << (name.size() & 63);
      if (name.size() > common) {
        const auto ch = static_cast<unsigned char>(name[common]);
        nextCharacters_[ch / 64] |= std::uint64_t{1} << (ch & 63);
      }
    }
  }

  [[nodiscard]] auto definitions() const -> std::span<const PredefinedMacro> {
    return definitions_;
  }

  [[nodiscard]] auto find(std::string_view name) const
      -> const PredefinedMacro*;

 private:
  friend class Preprocessor;

  [[nodiscard]] static constexpr auto initialMask(std::string_view name)
      -> std::uint64_t {
    if (name.empty()) return 0;
    return std::uint64_t{1} << (static_cast<unsigned char>(name.front()) & 63);
  }

  // Possible first identifier bytes modulo 64, combined at registration.
  std::uint64_t initials_ = 0;
  // Borrowed definitions, sorted by identifier, with unique names.
  std::span<const PredefinedMacro> definitions_;
  // Shared identifier prefix, borrowing spelling bytes from the first row.
  std::string_view prefix_;
  // Possible identifier lengths modulo 64; false positives use exact lookup.
  std::uint64_t lengths_ = 0;
  // Possible first bytes after the common prefix.
  std::array<std::uint64_t, 4> nextCharacters_ = {};
};

}  // namespace cxx
