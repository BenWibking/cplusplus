// RUN: %cxx -fsyntax-only -verify -ast-dump %s | %filecheck %s --match-full-lines

[[maybe_unused]] static __attribute__((used)) int value;
[[vendor::forward]] enum class Forward : int;

// clang-format off
//      CHECK:translation-unit
// CHECK-NEXT:  declaration-list
// CHECK-NEXT:    simple-declaration
// CHECK-NEXT:      attribute-list
// CHECK-NEXT:        cxx-attribute
// CHECK-NEXT:          attribute-list
// CHECK-NEXT:            attribute
// CHECK-NEXT:              attribute-token: simple-attribute-token
// CHECK-NEXT:                identifier: maybe_unused
// CHECK-NEXT:        gcc-attribute
// CHECK-NEXT:          attribute-list
// CHECK-NEXT:            attribute
// CHECK-NEXT:              attribute-token: simple-attribute-token
// CHECK-NEXT:                identifier: used
// CHECK-NEXT:      decl-specifier-list
// CHECK-NEXT:        static-specifier
// CHECK-NEXT:        integral-type-specifier
// CHECK-NEXT:          specifier: int
// CHECK-NEXT:      init-declarator-list
// CHECK-NEXT:        init-declarator
// CHECK-NEXT:          declarator: declarator
// CHECK-NEXT:            core-declarator: id-declarator
// CHECK-NEXT:              unqualified-id: name-id
// CHECK-NEXT:                identifier: value
// CHECK-NEXT:    opaque-enum-declaration
// CHECK-NEXT:      attribute-list
// CHECK-NEXT:        cxx-attribute
// CHECK-NEXT:          attribute-list
// CHECK-NEXT:            attribute
// CHECK-NEXT:              attribute-token: scoped-attribute-token
// CHECK-NEXT:                attribute-namespace: vendor
// CHECK-NEXT:                identifier: forward
// CHECK-NEXT:      unqualified-id: name-id
// CHECK-NEXT:        identifier: Forward
// CHECK-NEXT:      type-specifier-list
// CHECK-NEXT:        integral-type-specifier
// CHECK-NEXT:          specifier: int
