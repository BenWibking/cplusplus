// RUN: %cxx -verify -fsyntax-only -fvalidate-ast %s

struct Empty {};

template <class T> struct Prefix {
  [[no_unique_address]] T empty;
  char value;
};
template <class T> struct Suffix {
  T empty [[no_unique_address]];
  char value;
};
static_assert(sizeof(Prefix<Empty>) == 1);
static_assert(sizeof(Suffix<Empty>) == 1);
static_assert(__builtin_offsetof(Suffix<Empty>, value) == 0);

struct Members {
  char head;
  Empty overlap [[__no_unique_address__]], ordinary;
  char tail;
};
static_assert(__builtin_offsetof(Members, overlap) == 0);
static_assert(__builtin_offsetof(Members, ordinary) == 1);
static_assert(__builtin_offsetof(Members, tail) == 2);

struct Vendor {
  [[using other: no_unique_address]] Empty ordinary;
  char value;
};
static_assert(sizeof(Vendor) == 2);

struct Invalid {
  // expected-error@+1 {{'no_unique_address' attribute requires a non-static data member that is not a bit-field}}
  static Empty object [[no_unique_address]];
  // expected-error@+1 {{'no_unique_address' attribute requires a non-static data member that is not a bit-field}}
  unsigned bits [[no_unique_address]] : 1;
  // expected-error@+1 {{'no_unique_address' attribute takes no arguments}}
  Empty argument [[no_unique_address()]];
  // expected-error@+1 {{'no_unique_address' attribute cannot be applied to types}}
  Empty [[no_unique_address]] misplaced;
};
