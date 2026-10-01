// RUN: %cxx --target=x86_64-unknown-linux-gnu -fexperimental-cxxir -emit-ir %s -o - | %filecheck %s

extern "C" _Float16 identity_half(_Float16 value) { return value; }
extern "C" __bf16 identity_bfloat(__bf16 value) { return value; }
extern "C" __float8_e4m3fn identity_e4m3fn(__float8_e4m3fn value) {
  return value;
}
extern "C" __float8_e5m2 identity_e5m2(__float8_e5m2 value) {
  return value;
}
extern "C" float identity_float(float value) { return value; }
extern "C" double identity_double(double value) { return value; }
extern "C" long double identity_extended(long double value) { return value; }
extern "C" __float128 identity_quad(__float128 value) { return value; }

// CHECK-LABEL: cxx.func @identity_quad(
// CHECK-SAME: : f128) -> f128
// CHECK-LABEL: cxx.func @identity_extended(
// CHECK-SAME: : f80) -> f80
// CHECK-LABEL: cxx.func @identity_double(
// CHECK-SAME: : f64) -> f64
// CHECK-LABEL: cxx.func @identity_float(
// CHECK-SAME: : f32) -> f32
// CHECK-LABEL: cxx.func @identity_e5m2(
// CHECK-SAME: : f8E5M2) -> f8E5M2
// CHECK-LABEL: cxx.func @identity_e4m3fn(
// CHECK-SAME: : f8E4M3FN) -> f8E4M3FN
// CHECK-LABEL: cxx.func @identity_bfloat(
// CHECK-SAME: : bf16) -> bf16
// CHECK-LABEL: cxx.func @identity_half(
// CHECK-SAME: : f16) -> f16
