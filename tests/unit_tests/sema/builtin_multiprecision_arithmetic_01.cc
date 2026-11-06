// RUN: %cxx -verify -fsyntax-only %s
// expected-no-diagnostics

constexpr bool add_family() {
  unsigned char byte_carry = 0;
  if (__builtin_addcb(255, 1, 0, &byte_carry) != 0 || byte_carry != 1)
    return false;

  unsigned short short_carry = 0;
  if (__builtin_addcs(65535, 1, 0, &short_carry) != 0 || short_carry != 1)
    return false;

  unsigned carry = 0;
  if (__builtin_addc(1, 0, ~0u, &carry) != 0 || carry != 1) return false;
  if (__builtin_addc(~0u, ~0u, ~0u, &carry) != ~2u || carry != 1)
    return false;

  unsigned long long_carry = 0;
  if (__builtin_addcl(~0ul, 1, 0, &long_carry) != 0 || long_carry != 1)
    return false;

  unsigned long long long_long_carry = 0;
  if (__builtin_addcll(~0ull, 1, 0, &long_long_carry) != 0 ||
      long_long_carry != 1)
    return false;
  return true;
}

constexpr bool subtract_family() {
  unsigned char byte_borrow = 0;
  if (__builtin_subcb(0, 1, 0, &byte_borrow) != 255 || byte_borrow != 1)
    return false;

  unsigned short short_borrow = 0;
  if (__builtin_subcs(0, 1, 0, &short_borrow) != 65535 || short_borrow != 1)
    return false;

  unsigned borrow = 0;
  if (__builtin_subc(0, ~0u, ~0u, &borrow) != 2 || borrow != 1)
    return false;
  if (__builtin_subc(~0u, 0, ~0u, &borrow) != 0 || borrow != 0)
    return false;

  unsigned long long_borrow = 0;
  if (__builtin_subcl(0, 1, 0, &long_borrow) != ~0ul || long_borrow != 1)
    return false;

  unsigned long long long_long_borrow = 0;
  if (__builtin_subcll(0, 1, 0, &long_long_borrow) != ~0ull ||
      long_long_borrow != 1)
    return false;
  return true;
}

static_assert(add_family());
static_assert(subtract_family());
