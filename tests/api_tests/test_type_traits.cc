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

#include <cxx/ast.h>
#include <cxx/ast_visitor.h>
#include <cxx/symbols.h>
#include <cxx/type_traits.h>
#include <cxx/views/symbols.h>
#include <gtest/gtest.h>

#include "test_utils.h"

using namespace cxx;

namespace {

class LocalVariableCollector final : public ASTVisitor {
 public:
  void visit(InitDeclaratorAST* ast) override {
    if (auto* candidate = symbol_cast<VariableSymbol>(ast->symbol);
        candidate && candidate->enclosingFunction()) {
      variable = candidate;
    }
    ASTVisitor::visit(ast);
  }

  VariableSymbol* variable = nullptr;
};

}  // namespace

TEST(TypeTraits, PreservesRestrictQualifiedParameters) {
  auto source = R"(
    using Restricted = int* __restrict__;
    void restrict_copy(int* __restrict direct, Restricted alias,
                       int* __restrict__ alternate,
                       int& __restrict__ reference,
                       int* ordinary) {
      int* __restrict__ local = direct;
      direct[0] = alias[0] + alternate[0] + reference + ordinary[0];
      local = ordinary;
      ordinary = direct;
      local[1] = ordinary[1];
    }
  )"_cxx;

  ASSERT_EQ(source.diagnosticsClient.errorCount(), 0);
  auto functions = views::each_function(source.get("restrict_copy"));
  ASSERT_EQ(std::ranges::distance(functions), 1);
  auto* function = *functions.begin();
  const auto parameters = function->parameters();
  ASSERT_EQ(parameters.size(), 5u);
  auto traits = source.unit.typeTraits();
  EXPECT_TRUE(traits.is_restrict(parameters[0]->type()));
  EXPECT_TRUE(traits.is_restrict(parameters[1]->type()));
  EXPECT_TRUE(traits.is_restrict(parameters[2]->type()));
  EXPECT_TRUE(traits.is_restrict(parameters[3]->type()));
  EXPECT_TRUE(traits.is_lvalue_reference(parameters[3]->type()));
  EXPECT_TRUE(
      type_cast<IntType>(traits.remove_reference(parameters[3]->type())));
  EXPECT_FALSE(traits.is_restrict(parameters[4]->type()));

  auto unrestricted = traits.remove_restrict(parameters[0]->type());
  EXPECT_FALSE(traits.is_restrict(unrestricted));
  EXPECT_TRUE(traits.is_pointer(unrestricted));

  auto* functionType = type_cast<FunctionType>(function->type());
  ASSERT_NE(functionType, nullptr);
  ASSERT_EQ(functionType->parameterTypes().size(), 5u);
  for (auto* parameterType : functionType->parameterTypes()) {
    EXPECT_FALSE(traits.is_restrict(parameterType));
  }

  LocalVariableCollector collector;
  collector.accept(function->declaration());
  ASSERT_NE(collector.variable, nullptr);
  EXPECT_TRUE(traits.is_restrict(collector.variable->type()));
}

TEST(TypeTraits, PreservesSourceTypeAttributes) {
  auto source = R"(
    typedef int MayAliasInt __attribute__((may_alias));
    using CxxMayAlias [[gnu::may_alias]] = int;
    typedef MayAliasInt InheritedMayAlias;
    typedef const MayAliasInt ConstMayAlias;
    typedef unsigned AlignedMayAlias
        __attribute__((aligned(16), may_alias));
    typedef AlignedMayAlias ReducedAlignment
        __attribute__((aligned(2)));
    typedef int* MayAliasPointer __attribute__((may_alias));
    using Plain = int;

    struct __attribute__((may_alias)) TaggedRecord { int value; };
    enum __attribute__((may_alias)) TaggedEnum { tagged_value };
    enum class __attribute__((may_alias)) TaggedScopedEnum { value };
  )"_cxx;

  ASSERT_EQ(source.diagnosticsClient.errorCount(), 0);
  auto traits = source.unit.typeTraits();
  const auto hasMayAlias = [&](const Type* type) {
    return (traits.source_type_attribute_flags(type) &
            TypeAttributeFlags::kMayAlias) != TypeAttributeFlags::kNone;
  };
  const auto aliasType = [&](std::string_view name) {
    auto* alias = source.getAs<TypeAliasSymbol>(name);
    EXPECT_NE(alias, nullptr);
    return alias ? alias->type() : nullptr;
  };

  auto* mayAliasInt = aliasType("MayAliasInt");
  EXPECT_TRUE(hasMayAlias(mayAliasInt));
  EXPECT_TRUE(traits.is_same(mayAliasInt, source.unit.control()->getIntType()));
  EXPECT_TRUE(hasMayAlias(aliasType("CxxMayAlias")));
  EXPECT_TRUE(hasMayAlias(aliasType("InheritedMayAlias")));
  EXPECT_TRUE(hasMayAlias(aliasType("ConstMayAlias")));
  EXPECT_FALSE(hasMayAlias(aliasType("Plain")));

  auto* aligned = type_cast<AttributedType>(aliasType("AlignedMayAlias"));
  ASSERT_NE(aligned, nullptr);
  EXPECT_EQ(aligned->alignment(), 16u);
  EXPECT_TRUE(hasMayAlias(aligned));

  auto* reduced = type_cast<AttributedType>(aliasType("ReducedAlignment"));
  ASSERT_NE(reduced, nullptr);
  EXPECT_EQ(reduced->alignment(), 2u);
  EXPECT_TRUE(hasMayAlias(reduced));

  auto* pointer = type_cast<AttributedType>(aliasType("MayAliasPointer"));
  ASSERT_NE(pointer, nullptr);
  EXPECT_TRUE(type_cast<PointerType>(pointer->elementType()));
  EXPECT_TRUE(hasMayAlias(pointer));

  auto* record = source.getAs<ClassSymbol>("TaggedRecord");
  ASSERT_NE(record, nullptr);
  EXPECT_TRUE(hasMayAlias(record->type()));
  auto* enumeration = source.getAs<EnumSymbol>("TaggedEnum");
  ASSERT_NE(enumeration, nullptr);
  EXPECT_TRUE(hasMayAlias(enumeration->type()));
  auto* scopedEnumeration =
      source.getAs<ScopedEnumSymbol>("TaggedScopedEnum");
  ASSERT_NE(scopedEnumeration, nullptr);
  EXPECT_TRUE(hasMayAlias(scopedEnumeration->type()));
}

TEST(TypeTraits, SeparatesValueOperationAndDestructorTriviality) {
  auto source = R"(
    struct Guard {
      unsigned value;
      Guard() = default;
      Guard(const Guard&) = default;
      Guard(Guard&&) = default;
      Guard& operator=(const Guard&) = default;
      Guard& operator=(Guard&&) = default;
      ~Guard() {}
    };
    struct Owner { Guard guard; };
    struct Mixed {
      Mixed() = default;
      Mixed(const Mixed&) {}
      Mixed(Mixed&&) = default;
      Mixed& operator=(const Mixed&) = default;
      Mixed& operator=(Mixed&&) { return *this; }
      ~Mixed() {}
    };
  )"_cxx;
  auto traits = source.unit.typeTraits();

  const auto expectTrivial = [&](FunctionSymbol* function, bool expected) {
    ASSERT_NE(function, nullptr);
    EXPECT_EQ(traits.is_trivial_special_member(function), expected);
  };

  auto* guard = source.getAs<ClassSymbol>("Guard");
  ASSERT_TRUE(guard);
  expectTrivial(guard->copyConstructor(), true);
  expectTrivial(guard->moveConstructor(), true);
  expectTrivial(guard->copyAssignmentOperator(), true);
  expectTrivial(guard->moveAssignmentOperator(), true);
  expectTrivial(guard->destructor(), false);

  auto* owner = source.getAs<ClassSymbol>("Owner");
  ASSERT_TRUE(owner);
  expectTrivial(owner->copyConstructor(), true);
  expectTrivial(owner->moveConstructor(), true);
  expectTrivial(owner->copyAssignmentOperator(), true);
  expectTrivial(owner->moveAssignmentOperator(), true);
  expectTrivial(owner->destructor(), false);

  auto* mixed = source.getAs<ClassSymbol>("Mixed");
  ASSERT_TRUE(mixed);
  expectTrivial(mixed->copyConstructor(), false);
  expectTrivial(mixed->moveConstructor(), true);
  expectTrivial(mixed->copyAssignmentOperator(), true);
  expectTrivial(mixed->moveAssignmentOperator(), false);
  expectTrivial(mixed->destructor(), false);
}
