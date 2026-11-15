// RUN: %cxx -verify -fsyntax-only -fvalidate-ast %s

// expected-no-diagnostics

typedef int MayAliasInt __attribute__((may_alias));
using CxxMayAlias [[gnu::may_alias]] = int;
typedef MayAliasInt InheritedMayAlias;
typedef unsigned AlignedMayAlias __attribute__((aligned(16), may_alias));
typedef AlignedMayAlias ReducedAlignment __attribute__((aligned(2)));

static_assert(__is_same(MayAliasInt, int));
static_assert(__is_same(CxxMayAlias, int));
static_assert(__is_same(InheritedMayAlias, int));
static_assert(alignof(AlignedMayAlias) == 16);
static_assert(alignof(ReducedAlignment) == 2);

template <class T>
struct Identity {};

static_assert(__is_same(Identity<MayAliasInt>, Identity<int>));

struct __attribute__((may_alias)) Record {
  int value;
};

enum __attribute__((may_alias)) Enumeration { value };
enum class [[gnu::may_alias]] ScopedEnumeration { value };

static_assert(__has_attribute(may_alias));
static_assert(__has_cpp_attribute(gnu::may_alias));
