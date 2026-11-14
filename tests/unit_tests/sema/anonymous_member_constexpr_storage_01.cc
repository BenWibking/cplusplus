// RUN: %cxx -verify -fsyntax-only %s

struct AnonymousMembers {
  int prefix;
  struct {
    int value;
    int other;
  };
  union {
    int integer;
    float floating;
  };

  constexpr AnonymousMembers() : prefix(91), value(0), other(0), integer(0) {}
  constexpr void reset(bool condition) {
    if (condition) value = 1;
    other = 2;
    integer = 3;
  }
  constexpr void update() {
    value += 4;
    ++other;
    integer++;
  }
  constexpr int& reference() { return value; }
};

constexpr int implicit_members() {
  AnonymousMembers object;
  object.reset(true);
  object.update();
  return object.value * 100 + object.other * 10 + object.integer;
}
static_assert(implicit_members() == 534);

constexpr bool retained_member_identity() {
  AnonymousMembers object;
  int& reference = object.reference();
  int* address = &object.value;
  object.reset(true);
  reference += 6;
  return &reference == address && *address == 7 && object.value == 7 &&
         object.prefix == 91;
}
static_assert(retained_member_identity());

struct NestedMembers {
  struct {
    struct {
      int value;
    };
    union {
      int integer;
      float floating;
    };
  };

  constexpr NestedMembers() : value(1), integer(2) {}
  constexpr int update() {
    value += 4;
    integer *= 3;
    return value * 10 + integer;
  }
  constexpr int explicit_update() {
    this->value += 7;
    this->integer += 2;
    return this->value * 10 + this->integer;
  }
};

constexpr int nested_members() {
  NestedMembers object;
  const int before = object.update();
  return before * 1000 + object.explicit_update();
}
static_assert(nested_members() == 56128);

struct DerivedMembers : AnonymousMembers {
  int own = 9;
  constexpr void modify_base() {
    value = 4;
    integer = 6;
  }
};

constexpr int inherited_members() {
  DerivedMembers object;
  object.modify_base();
  return object.value * 100 + object.integer * 10 + object.own;
}
static_assert(inherited_members() == 469);

struct Representation {
  int first = 0;
  int second = 0;
};

struct AnonymousRepresentation {
  int prefix = 37;
  struct {
    Representation representation;
  };

  constexpr AnonymousRepresentation() : representation{} {}
  constexpr void reset(int value) {
    representation = Representation{value, value + 1};
  }
};

constexpr int owned_record_assignment() {
  AnonymousRepresentation original;
  original.reset(7);
  AnonymousRepresentation copy = original;
  copy.reset(9);
  return original.representation.first * 1000 +
         original.representation.second * 100 +
         copy.representation.first * 10 + copy.representation.second;
}
static_assert(owned_record_assignment() == 7900);

struct UnionMembers {
  union {
    int integer;
    unsigned other;
  };
  constexpr UnionMembers() : integer(1) {}
  constexpr unsigned change() {
    other = 9;
    return other;
  }
};

constexpr unsigned active_union_member() {
  UnionMembers object;
  return object.change();
}
static_assert(active_union_member() == 9);

struct NamedMembers {
  Representation representation;
  constexpr void reset() { representation.first = 11; }
};

constexpr int named_member_control() {
  NamedMembers object;
  object.reset();
  return object.representation.first;
}
static_assert(named_member_control() == 11);

constexpr int inactive_union_member() {
  UnionMembers object;
  object.change();
  return object.integer;
}
// expected-error@+1 {{static assertion expression is not an integral constant expression}}
static_assert(inactive_union_member() == 1);

constexpr const int* expired_member_address() {
  AnonymousMembers object;
  object.reset(true);
  return &object.reference();
}
// expected-error@+1 {{static assertion expression is not an integral constant expression}}
static_assert(*expired_member_address() == 1);
