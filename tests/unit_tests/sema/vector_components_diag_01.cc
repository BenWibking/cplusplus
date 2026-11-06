// RUN: %cxx -verify -fsyntax-only %s

using Int3 = int __attribute__((ext_vector_type(3)));
using Int4 = int __attribute__((ext_vector_type(4)));

void mixedFamilies(Int4 value) {
  // expected-error@+1 {{invalid vector component 'xg'}}
  (void)value.xg;
}

void pointOutOfRange(Int3 value) {
  // expected-error@+1 {{vector component 'w' exceeds 3 source elements}}
  (void)value.w;
}

void hexadecimalOutOfRange(Int4 value) {
  // expected-error@+1 {{vector component 's4' exceeds 4 source elements}}
  (void)value.s4;
}

void missingHexadecimalPrefix(Int4 value) {
  // expected-error@+1 {{invalid vector component 'A1'}}
  (void)value.A1;
}

void missingHexadecimalDigits(Int4 value) {
  // expected-error@+1 {{invalid vector component 's'}}
  (void)value.s;
}

void arrowRequiresPointer(Int4 value) {
  // expected-error@+1 {{invalid member access into expression of type 'int __attribute__((ext_vector_type(4)))'}}
  (void)value->x;
}
