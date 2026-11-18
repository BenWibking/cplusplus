// RUN: %cxx -verify -fsyntax-only -fvalidate-ast %s

// expected-warning@+1 {{'gnu::vector_size' ignored on a type specifier; place it on the declaration}}
typedef int [[gnu::vector_size(16)]] Word;
using Words [[using gnu: __vector_size__(16)]] = int;
using GnuWords = int __attribute__((vector_size(16)));
static_assert(__is_same(Word, int));
static_assert(__is_same(Words, GnuWords));

// expected-warning@+1 {{'gnu::vector_size' ignored on a type specifier; place it on the declaration}}
template<class T> using Scalar = T [[gnu::vector_size(16)]];
template<class T> using Lanes [[gnu::vector_size(16)]] = T;
static_assert(__is_same(Scalar<int>, Word));
static_assert(__is_same(Lanes<int>, Words));

// expected-warning@+1 {{'gnu::vector_size' ignored on a type specifier; place it on the declaration}}
int [[gnu::vector_size(16)]] first, second [[gnu::vector_size(16)]];
static_assert(__is_same(decltype(first), int));
static_assert(__is_same(decltype(second), Words));

// expected-warning@+1 {{'gnu::vector_size' ignored on a type specifier; place it on the declaration}}
void take(int [[gnu::vector_size(16)]], Words);
static_assert(__is_same(decltype(take), void(int, Words)));

struct Fields {
  char head;
  // expected-warning@+1 {{'gnu::vector_size' ignored on a type specifier; place it on the declaration}}
  int [[gnu::vector_size(16)]] word;
  Words lanes;
};
static_assert(__builtin_offsetof(Fields, word) == 4);
static_assert(__builtin_offsetof(Fields, lanes) == 16);

// expected-error@+1 {{an attribute list cannot appear between declaration specifiers}}
typedef [[gnu::vector_size(16)]] int Misplaced;
// expected-error@+1 {{an attribute list cannot appear between declaration specifiers}}
unsigned [[clang::ext_vector_type(4)]] long interrupted;

// expected-error@+1 {{'gnu::aligned' attribute cannot be applied to types}}
int [[gnu::aligned(16)]] object;
// expected-error@+1 {{'gnu::packed' attribute cannot be applied to types}}
struct Record { int [[gnu::packed]] value; };
// expected-error@+1 {{'gnu::may_alias' attribute cannot be applied to types}}
using InvalidAlias = int [[gnu::may_alias]];

// expected-warning@+1 {{'gnu::aligned' ignored on a declarator type; place it on the declaration}}
int array[2] [[gnu::aligned(16)]];
static_assert(__alignof__(array) == alignof(int));
// expected-error@+1 {{invalid vector element type 'int*'}}
using InvalidLanes = int * [[gnu::vector_size(16)]];
