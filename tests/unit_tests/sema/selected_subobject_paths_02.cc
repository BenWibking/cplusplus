// RUN: %cxx -std=c++23 -fsyntax-only -verify %s

struct VirtualBase {
  unsigned value;
};
struct VirtualLeft : virtual VirtualBase {};
struct VirtualRight : virtual VirtualBase {};
struct Diamond : VirtualLeft, VirtualRight {};
extern Diamond diamond;

template <const unsigned& Value>
struct Ref {};

// ConstObject has no single storage node for a shared virtual base. Reject the
// selected path instead of assigning it to either inheritance branch.
// expected-error@+1 {{template argument is not a converted constant expression of type 'const unsigned int&'}}
Ref<diamond.VirtualLeft::value> unsupportedVirtualBase;

struct Base {
  unsigned value;
  unsigned get() const { return value; }
  unsigned overloaded() const { return value; }
  static unsigned overloaded(unsigned value) { return value; }
};
struct Left : Base {};
struct Right : Base {};
struct Both : Left, Right {
  using Left::value;
  using Left::get;
  using Left::overloaded;

  unsigned read() const { return value; } // expected-error {{member 'value' found in multiple base classes of '::Both'}}
  unsigned call() const { return get(); } // expected-error {{member 'get' found in multiple base classes of '::Both'}}
  unsigned callOverloaded() const { return overloaded(); } // expected-error {{member 'overloaded' found in multiple base classes of '::Both'}}
  unsigned callStatic() const { return overloaded(11); }
};

unsigned read(Both& object) {
  return object.value; // expected-error {{member 'value' found in multiple base classes of '::Both'}}
}
unsigned call(Both& object) {
  return object.get(); // expected-error {{member 'get' found in multiple base classes of '::Both'}}
}
unsigned callOverloaded(Both& object) {
  return object.overloaded(); // expected-error {{member 'overloaded' found in multiple base classes of '::Both'}}
}
unsigned callStatic(Both& object) { return object.overloaded(13); }

unsigned Base::* memberPointer = &Both::value;
