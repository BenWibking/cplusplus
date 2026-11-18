// RUN: %cxx -verify -fsyntax-only -fvalidate-ast %s

// expected-no-diagnostics

static_assert(!__has_cpp_attribute(packed));
static_assert(__has_cpp_attribute(gnu::packed));
static_assert(__has_attribute(__packed__));

template <class T> struct [[packed]] Ordinary { char tag; T value; };
template <class T> struct [[using gnu: packed]] Wire { char tag; T value; };
struct Fields {
  char tag;
  int natural [[packed]];
  char separator;
  int wire __attribute__((__packed__));
};

static_assert(__builtin_offsetof(Ordinary<int>, value) == 4);
static_assert(__builtin_offsetof(Wire<int>, value) == 1);
static_assert(__builtin_offsetof(Fields, natural) == 4);
static_assert(__builtin_offsetof(Fields, wire) == 9);

[[nothrow]] void ordinary();
[[gnu::nothrow]] void noThrow();
static_assert(!noexcept(ordinary()));
static_assert(noexcept(noThrow()));
