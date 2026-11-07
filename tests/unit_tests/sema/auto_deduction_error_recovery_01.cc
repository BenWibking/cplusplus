// RUN: %cxx -verify -fsyntax-only -fvalidate-ast %s

template <class T>
struct box {
  T value;
  constexpr T get() const { return value; }
};

template <class T>
constexpr box<T> make_box(T value) {
  return {value};
}

constexpr auto unresolved = missing;  // expected-error {{use of undeclared identifier 'missing'}}
static_assert(make_box(unresolved).get() == 42);
