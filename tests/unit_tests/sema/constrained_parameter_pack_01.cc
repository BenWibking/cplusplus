// RUN: %cxx -verify -fsyntax-only %s
// expected-no-diagnostics

// The type-constraint of a parameter pack applies to each element:
// template <C<A>... Ts> constrains with (C<Ts, A> && ...).

template <class T, class U>
concept Same = __is_same(T, U);

template <Same<float>... Ts>
constexpr int f(Ts...) {
  return 1;
}

template <Same<double>... Ts>
constexpr int f(Ts...) {
  return 2;
}

static_assert(f(1.0f) == 1);
static_assert(f(1.0f, 2.0f) == 1);
static_assert(f(1.0, 2.0) == 2);
