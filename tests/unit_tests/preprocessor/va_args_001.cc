// RUN: %cxx -E -P %s -o - | %filecheck %s

#define declare(type, ...) __VA_OPT__(type) __VA_ARGS__ __VA_OPT__(;)

// clang-format off

declare(int, a, b, c)

// CHECK: int a, b, c ;

#define spell_(...) #__VA_ARGS__
#define spell(...) spell_(__VA_ARGS__)
spell(declare(int, a, b, c))

// CHECK-NEXT: "int a, b, c ;"

declare()
