// clang-format off
// RUN: %cxx -verify -fsyntax-only %s

// A name that denotes an entity captured by copy has the type of the closure
// member, which is const unless the lambda is mutable ([expr.prim.id.unqual]).
// Overload resolution on a captured copy therefore picks the const member
// functions. A capture by reference names the entity itself.

struct A {
  int value;
  auto operator[](int) -> int&;
  auto operator[](int) const -> const int&;
  auto get() -> int*;
  auto get() const -> const int*;
};

template <typename T, typename U>
constexpr bool same = __is_same(T, U);

void implicit_copy(A a, A& ref, const A& cref) {
  [=] {
    static_assert(same<decltype(a[0]), const int&>);
    static_assert(same<decltype(a.get()), const int*>);
    static_assert(same<decltype((a)), const A&>);
    static_assert(same<decltype(a), A>);
    static_assert(same<decltype(ref[0]), const int&>);
    static_assert(same<decltype(cref[0]), const int&>);
    return a[0] + ref[0] + cref[0];
  }();
}

void explicit_copy(A a) {
  [a] {
    static_assert(same<decltype(a[0]), const int&>);
    static_assert(same<decltype(a.get()), const int*>);
    return a[0];
  }();
  [b = a] {
    static_assert(same<decltype(b[0]), const int&>);
    return b[0];
  }();
}

void by_reference(A a) {
  [&] {
    static_assert(same<decltype(a[0]), int&>);
    return a[0];
  }();
  [&a] {
    static_assert(same<decltype(a[0]), int&>);
    return a[0];
  }();
  [&b = a] {
    static_assert(same<decltype(b[0]), int&>);
    return b[0];
  }();
}

void mutable_copy(A a) {
  [=]() mutable {
    static_assert(same<decltype(a[0]), int&>);
    static_assert(same<decltype(a.get()), int*>);
    a.value = 1;
    return a[0];
  }();
  [a]() mutable {
    static_assert(same<decltype(a[0]), int&>);
    return a[0];
  }();
  [b = a]() mutable {
    static_assert(same<decltype(b[0]), int&>);
    return b[0];
  }();
}

void locals_and_parameters(A a) {
  [=](A param) {
    A local{};
    static_assert(same<decltype(param[0]), int&>);
    static_assert(same<decltype(local[0]), int&>);
    return param[0] + local[0] + a[0];
  }(a);
}

void nested(A a) {
  [=] {
    [&] {
      static_assert(same<decltype(a[0]), const int&>);
      return a[0];
    }();
  }();
  [a]() mutable {
    [=] {
      static_assert(same<decltype(a[0]), const int&>);
      return a[0];
    }();
    [&] {
      static_assert(same<decltype(a[0]), int&>);
      return a[0];
    }();
  }();
  [&] {
    [=] {
      static_assert(same<decltype(a[0]), const int&>);
      return a[0];
    }();
  }();
}

void generic(A a) {
  [=](auto i) {
    static_assert(same<decltype(a[i]), const int&>);
    return a[i];
  }(0);
  [a](auto i) {
    static_assert(same<decltype(a[i]), const int&>);
    return a[i];
  }(0);
  [=](auto i) mutable {
    static_assert(same<decltype(a[i]), int&>);
    return a[i];
  }(0);
}

template <typename T>
void in_template(T t) {
  [=] {
    static_assert(same<decltype(t[0]), const int&>);
    return t[0];
  }();
  [t] {
    static_assert(same<decltype(t[0]), const int&>);
    return t[0];
  }();
  [=]() mutable {
    static_assert(same<decltype(t[0]), int&>);
    t.value = 1;
    return t[0];
  }();
}

void instantiate(A a) { in_template(a); }

struct S {
  A a;

  void const_member() const {
    static_assert(same<decltype(a[0]), const int&>);
    [this] {
      static_assert(same<decltype(a[0]), const int&>);
      return a[0];
    }();
  }

  void member() {
    static_assert(same<decltype(a[0]), int&>);
    [this] {
      static_assert(same<decltype(a[0]), int&>);
      return a[0];
    }();
  }
};
