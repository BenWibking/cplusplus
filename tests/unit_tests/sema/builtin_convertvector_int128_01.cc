// RUN: %cxx -verify -fsyntax-only %s
// REQUIRES: int128
// expected-no-diagnostics

using UInt128x2 = unsigned __int128 __attribute__((ext_vector_type(2)));
using Double2 = double __attribute__((ext_vector_type(2)));

constexpr UInt128x2 veryLarge{
    (static_cast<unsigned __int128>(1) << 100) + 1,
    static_cast<unsigned __int128>(1) << 127};
constexpr auto veryLargeDoubles =
    __builtin_convertvector(veryLarge, Double2);
static_assert(veryLargeDoubles[0] == 0x1p100);
static_assert(veryLargeDoubles[1] == 0x1p127);
