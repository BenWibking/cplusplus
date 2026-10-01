// RUN: %cxx -emit-ir %s -o - | %filecheck %s

using Int4 = int __attribute__((ext_vector_type(4)));

constexpr Int4 full{1, 2, 3, 4};

struct Holder {
  static constexpr Int4 reversed{4, 3, 2, 1};
};

const Int4* getFull() { return &full; }

const Int4* getReversed() { return &Holder::reversed; }

// CHECK: cxx.global @_ZN6Holder8reversedE linkonce_odr constant
// CHECK-SAME: : vector<4xi32> = dense<[4, 3, 2, 1]> : vector<4xi32>
// CHECK-SAME: alignment(16)
// CHECK: cxx.address_of @_ZN6Holder8reversedE : !ptr_
// CHECK: cxx.global @_ZL4full internal constant
// CHECK-SAME: : vector<4xi32> = dense<[1, 2, 3, 4]> : vector<4xi32>
// CHECK-SAME: alignment(16)
// CHECK: cxx.address_of @_ZL4full : !ptr_
