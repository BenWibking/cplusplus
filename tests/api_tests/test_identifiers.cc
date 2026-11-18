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

#include <cxx/control.h>
#include <cxx/diagnostics_client.h>
#include <cxx/memory_layout.h>
#include <cxx/names.h>
#include <cxx/token.h>
#include <cxx/translation_unit.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace cxx {
namespace {

struct ExpectedIdentifier {
  std::string_view spelling;
  BuiltinTypeTraitKind trait{};
  UnaryBuiltinTypeKind unaryType{};
  BuiltinFunctionKind function{};
  BuiltinTemplateKind builtinTemplate{};
  WellKnownName wellKnownName{};
};

void expectIdentifier(Control& control, const ExpectedIdentifier& expected) {
  auto id = control.getIdentifier(expected.spelling);
  EXPECT_EQ(id->name(), expected.spelling);
  EXPECT_EQ(id->builtinTypeTrait(), expected.trait);
  EXPECT_EQ(id->unaryBuiltinType(), expected.unaryType);
  EXPECT_EQ(id->builtinFunction(), expected.function);
  EXPECT_EQ(id->builtinTemplate(), expected.builtinTemplate);
  EXPECT_EQ(id->wellKnownName(), expected.wellKnownName);
  EXPECT_EQ(Token::builtinFunctionKind(expected.spelling), expected.function);
  EXPECT_EQ(control.getIdentifier(expected.spelling), id);
}

TEST(Identifiers, RecognizesExactlyTheRegisteredSpellings) {
#define TRAIT(id, s) {.spelling = s, .trait = BuiltinTypeTraitKind::T_##id},
#define UNARY(id, s) {.spelling = s, .unaryType = UnaryBuiltinTypeKind::T_##id},
#define FUNCTION(id, s) \
  {.spelling = s, .function = BuiltinFunctionKind::T_##id},
#define TEMPLATE(id, s) \
  {.spelling = s, .builtinTemplate = BuiltinTemplateKind::T_##id},
#define KNOWN(id, s) {.spelling = s, .wellKnownName = WellKnownName::T_##id},
  static constexpr ExpectedIdentifier cases[] = {
      FOR_EACH_BUILTIN_TYPE_TRAIT(TRAIT) FOR_EACH_UNARY_BUILTIN_TYPE_TRAIT(
          UNARY) FOR_EACH_BUILTIN_FUNCTION(FUNCTION)
          FOR_EACH_BUILTIN_TEMPLATE(TEMPLATE) FOR_EACH_WELL_KNOWN_NAME(KNOWN)};
#undef TRAIT
#undef UNARY
#undef FUNCTION
#undef TEMPLATE
#undef KNOWN

  Control spellingFirst, enumFirst;
  EXPECT_EQ(spellingFirst.getIdentifier(WellKnownName::T_NONE), nullptr);
  for (const auto& expected : cases) {
    SCOPED_TRACE(expected.spelling);
    expectIdentifier(spellingFirst, expected);
    if (expected.wellKnownName != WellKnownName::T_NONE) {
      auto id = enumFirst.getIdentifier(expected.wellKnownName);
      EXPECT_EQ(enumFirst.getIdentifier(expected.spelling), id);
      EXPECT_EQ(id->wellKnownName(), expected.wellKnownName);
      auto other = spellingFirst.getIdentifier(expected.wellKnownName);
      EXPECT_EQ(other, spellingFirst.getIdentifier(expected.spelling));
      EXPECT_NE(id, other);
    }

    std::string miss(expected.spelling);
    miss[miss.size() / 2] = '$';
    expectIdentifier(spellingFirst, {miss});
  }
  expectIdentifier(spellingFirst, {""});
  expectIdentifier(spellingFirst, {std::string(256, 'a')});
  expectIdentifier(spellingFirst, {std::string_view("__is_\0lass", 10)});
  expectIdentifier(spellingFirst, {"__is_\xff"
                                   "lass"});
}

TEST(Identifiers, DeclaringATraitNameOnlyDemotesItInItsTranslationUnit) {
  DiagnosticsClient diagnostics;
  MemoryLayout layout{64};
  TranslationUnit local{&diagnostics}, other{&diagnostics};
  local.control()->setMemoryLayout(&layout);
  other.control()->setMemoryLayout(&layout);
  auto localId = local.control()->getIdentifier("__is_class");
  auto otherId = other.control()->getIdentifier("__is_class");

  local.setSource(
      "int __is_class = 1; static_assert(sizeof(__is_class) == sizeof(int));",
      "local.cxx");
  local.parse({.analysisMode = ParserAnalysisMode::kFull});
  other.setSource("struct C {}; static_assert(__is_class(C));", "other.cxx");
  other.parse({.analysisMode = ParserAnalysisMode::kFull});

  EXPECT_EQ(diagnostics.errorCount(), 0);
  EXPECT_EQ(localId, local.control()->getIdentifier("__is_class"));
  EXPECT_EQ(localId->builtinTypeTrait(), BuiltinTypeTraitKind::T_NONE);
  EXPECT_EQ(otherId->builtinTypeTrait(), BuiltinTypeTraitKind::T___IS_CLASS);
}

}  // namespace
}  // namespace cxx
