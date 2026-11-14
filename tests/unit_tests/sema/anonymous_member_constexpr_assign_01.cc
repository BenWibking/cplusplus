// RUN: %cxx -verify -fsyntax-only %s

// Members of an anonymous struct or union are members of the enclosing class.
// Constant evaluation assigns to them in the enclosing object, as libc++'s
// std::string does for its compressed-pair representation.

struct S {
  struct {
    int value;
    int other;
  };
  union {
    int i;
    float f;
  };
  constexpr S() : value(0), other(0), i(0) {}
  constexpr void set(bool c) {
    if (c) value = 1;
    other = 2;
    i = 3;
  }
};

constexpr int f() {
  S s;
  s.set(true);
  return s.value * 100 + s.other * 10 + s.i;
}

static_assert(f() == 123);
