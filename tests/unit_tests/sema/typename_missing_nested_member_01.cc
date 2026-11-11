// RUN: %cxx -verify -fsyntax-only %s
// expected-no-diagnostics

// A missing member in the middle of a nested-name-specifier of a concrete
// class is a substitution failure.

template <class...>
using void_t = void;

template <class D, class = void>
struct Is {
  static constexpr bool value = false;
};

template <class D>
struct Is<D, void_t<typename D::A::B>> {
  static constexpr bool value = true;
};

struct NoA {};
struct HasAB {
  struct A {
    using B = int;
  };
};

static_assert(Is<HasAB>::value);
static_assert(!Is<NoA>::value);

template <class D>
concept Deep = requires { typename D::A::B; };
static_assert(Deep<HasAB>);
static_assert(!Deep<NoA>);

template <class D>
  requires requires { typename D::A::B; }
constexpr int pick(D) {
  return 1;
}
constexpr int pick(...) { return 2; }
static_assert(pick(HasAB{}) == 1);
static_assert(pick(NoA{}) == 2);
