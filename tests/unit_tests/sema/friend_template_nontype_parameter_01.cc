// RUN: %cxx -verify -fsyntax-only %s
// expected-no-diagnostics

// A friend function template declared inside a class template (depth 1) is
// the same template as its namespace-scope declaration (depth 0), including
// when a non-type parameter's type is another template parameter.

template <class U, U a>
struct E;

template <class C, class U1, U1 a1>
C& operator<<(C&, const E<U1, a1>&);

template <class U, U a>
struct E {
  template <class C, class U1, U1 a1>
  friend C& operator<<(C&, const E<U1, a1>&);

 private:
  int x = 0;
};

template <class C, class U1, U1 a1>
C& operator<<(C& os, const E<U1, a1>& e) {
  (void)e.x;
  return os;
}

struct OS {};
void t(OS& os, E<unsigned, 5u>& e) { os << e; }
