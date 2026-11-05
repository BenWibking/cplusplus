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

#include <cxx/symbols.h>
#include <cxx/type_traits.h>
#include <gtest/gtest.h>

#include "test_utils.h"

using namespace cxx;

TEST(TypeTraits, SeparatesConstructorAndDestructorTriviality) {
  auto source = R"(
    struct Guard { unsigned value; ~Guard() {} };
    struct Owner { Guard guard; };
    struct Copied {
      Copied(const Copied&) {}
      ~Copied() {}
    };
    struct Moved {
      Moved(Moved&&) {}
      ~Moved() {}
    };
    struct CopyAssigned {
      CopyAssigned& operator=(const CopyAssigned&) { return *this; }
      ~CopyAssigned() {}
    };
    struct MoveAssigned {
      MoveAssigned& operator=(MoveAssigned&&) { return *this; }
      ~MoveAssigned() {}
    };
    struct ConstMember { const unsigned value; };
  )"_cxx;
  auto traits = source.unit.typeTraits();

  auto* guard = source.getAs<ClassSymbol>("Guard");
  ASSERT_TRUE(guard);
  EXPECT_TRUE(traits.has_trivial_copy_constructor(guard->type()));
  EXPECT_TRUE(traits.has_trivial_move_constructor(guard->type()));
  EXPECT_TRUE(traits.has_trivial_copy_assignment(guard->type()));
  EXPECT_TRUE(traits.has_trivial_move_assignment(guard->type()));
  EXPECT_TRUE(traits.has_trivial_value_operations(guard->type()));
  EXPECT_FALSE(traits.has_trivial_destructor(guard->type()));

  auto* owner = source.getAs<ClassSymbol>("Owner");
  ASSERT_TRUE(owner);
  EXPECT_TRUE(traits.has_trivial_copy_constructor(owner->type()));
  EXPECT_TRUE(traits.has_trivial_move_constructor(owner->type()));
  EXPECT_TRUE(traits.has_trivial_copy_assignment(owner->type()));
  EXPECT_TRUE(traits.has_trivial_move_assignment(owner->type()));
  EXPECT_TRUE(traits.has_trivial_value_operations(owner->type()));
  EXPECT_FALSE(traits.has_trivial_destructor(owner->type()));

  auto* copied = source.getAs<ClassSymbol>("Copied");
  ASSERT_TRUE(copied);
  EXPECT_FALSE(traits.has_trivial_copy_constructor(copied->type()));
  EXPECT_FALSE(traits.has_trivial_move_constructor(copied->type()));
  EXPECT_FALSE(traits.has_trivial_value_operations(copied->type()));

  auto* moved = source.getAs<ClassSymbol>("Moved");
  ASSERT_TRUE(moved);
  EXPECT_FALSE(traits.has_trivial_copy_constructor(moved->type()));
  EXPECT_FALSE(traits.has_trivial_move_constructor(moved->type()));
  EXPECT_FALSE(traits.has_trivial_value_operations(moved->type()));

  auto* copy_assigned = source.getAs<ClassSymbol>("CopyAssigned");
  ASSERT_TRUE(copy_assigned);
  EXPECT_FALSE(traits.has_trivial_copy_assignment(copy_assigned->type()));
  EXPECT_FALSE(traits.has_trivial_move_assignment(copy_assigned->type()));
  EXPECT_FALSE(traits.has_trivial_value_operations(copy_assigned->type()));

  auto* move_assigned = source.getAs<ClassSymbol>("MoveAssigned");
  ASSERT_TRUE(move_assigned);
  EXPECT_FALSE(traits.has_trivial_copy_assignment(move_assigned->type()));
  EXPECT_FALSE(traits.has_trivial_move_assignment(move_assigned->type()));
  EXPECT_FALSE(traits.has_trivial_value_operations(move_assigned->type()));

  auto* const_member = source.getAs<ClassSymbol>("ConstMember");
  ASSERT_TRUE(const_member);
  EXPECT_FALSE(traits.has_trivial_copy_assignment(const_member->type()));
  EXPECT_FALSE(traits.has_trivial_move_assignment(const_member->type()));
  EXPECT_TRUE(traits.has_trivial_value_operations(const_member->type()));
}
