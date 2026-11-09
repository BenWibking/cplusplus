// clang-format off
// RUN: %cxx --target=x86_64-unknown-linux-gnu -verify -emit-llvm %s -o %t.ll
// RUN: %filecheck %s < %t.ll

// CHECK-LABEL: define {{.*}} @_ZZ21template_copy_captureI1AEiT_ENKUlvE_clEv(
// CHECK: call ptr @_ZNK1AixEi(
// CHECK-LABEL: define internal i32 @"_ZZ17reference_capture1AENK3$_0clEv"(
// CHECK: call ptr @_ZN1AixEi(
// CHECK-LABEL: define internal i32 @"_ZZ20mutable_copy_capture1AEN3$_0clEv"(
// CHECK: call ptr @_ZN1AixEi(
// CHECK-LABEL: define internal i32 @"_ZZ12copy_capture1AENK3$_0clEv"(
// CHECK: call ptr @_ZNK1AixEi(
// clang-format on

// A copy captured by a non-mutable lambda is const, so subscripting it calls
// the const operator[]; a mutable lambda and a capture by reference call the
// non-const one.

struct A {
  int v[2];
  auto operator[](int i) -> int&;
  auto operator[](int i) const -> const int&;
};

auto copy_capture(A a) -> int {
  return [=] { return a[0]; }();
}

auto mutable_copy_capture(A a) -> int {
  return [=]() mutable { return a[0]; }();
}

auto reference_capture(A a) -> int {
  return [&] { return a[0]; }();
}

template <typename T>
auto template_copy_capture(T t) -> int {
  return [=] { return t[1]; }();
}

auto instantiate(A a) -> int { return template_copy_capture(a); }
