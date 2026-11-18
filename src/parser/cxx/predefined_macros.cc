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

#include <cxx/predefined_macros.h>

#include <algorithm>

namespace cxx {

auto PredefinedMacros::find(std::string_view name) const
    -> const PredefinedMacro* {
  if (!(lengths_ & (std::uint64_t{1} << (name.size() & 63))) ||
      !name.starts_with(prefix_)) {
    return nullptr;
  }
  if (name.size() > prefix_.size()) {
    const auto ch = static_cast<unsigned char>(name[prefix_.size()]);
    if (!(nextCharacters_[ch / 64] & (std::uint64_t{1} << (ch & 63)))) {
      return nullptr;
    }
  }
  auto it = std::ranges::lower_bound(definitions_, name, {},
                                     &PredefinedMacro::identifier);
  return it != definitions_.end() && it->identifier() == name ? &*it : nullptr;
}

}  // namespace cxx
