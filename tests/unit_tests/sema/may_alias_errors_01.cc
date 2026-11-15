// RUN: %cxx -verify -fsyntax-only %s

typedef int InvalidAlias
    __attribute__((may_alias(1)));  // expected-error {{'may_alias' attribute takes no arguments}}

struct __attribute__((may_alias(2))) InvalidRecord {  // expected-error {{'may_alias' attribute takes no arguments}}
  int value;
};
