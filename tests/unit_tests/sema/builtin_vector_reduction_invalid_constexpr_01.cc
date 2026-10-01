// RUN: %cxx -verify -fsyntax-only %s

using Int4 = int __attribute__((ext_vector_type(4)));

// expected-error@1 {{constexpr variable must be initialized}}
constexpr int directAddOverflow =
    __builtin_reduce_add(Int4{__INT_MAX__, 1, 0, 0});

// Reductions follow lane order, so a signed intermediate overflow invalidates
// the expression even if a later lane would bring the result into range.
// expected-error@1 {{constexpr variable must be initialized}}
constexpr int intermediateAddOverflow =
    __builtin_reduce_add(Int4{__INT_MAX__, 1, -1, 0});

// expected-error@1 {{constexpr variable must be initialized}}
constexpr int directMulOverflow =
    __builtin_reduce_mul(Int4{__INT_MAX__, 2, 1, 1});

// expected-error@1 {{constexpr variable must be initialized}}
constexpr int intermediateMulOverflow =
    __builtin_reduce_mul(Int4{__INT_MAX__, 2, 0, 1});
