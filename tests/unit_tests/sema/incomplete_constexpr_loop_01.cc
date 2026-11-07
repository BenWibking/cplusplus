// RUN: %cxx -verify -fsyntax-only %s

constexpr int f() {
  // expected-error@1 {{expected an expression}}
  for (int i = 0; i < 1; ++ case 0:
    return 0;
// expected-warning@1 {{non-void function does not return a value of type 'int' on every path}}
}

// expected-error@1 {{static assertion expression is not an integral constant expression}}
static_assert(f() == 0);
