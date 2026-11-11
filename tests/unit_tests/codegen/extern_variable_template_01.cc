// clang-format off
// RUN: %cxx --target=x86_64-unknown-linux-gnu -emit-llvm %s -o %t.ll
// RUN: %filecheck %s < %t.ll

// Specializations of a variable template that is only declared extern name
// storage defined in another translation unit: they are external
// declarations, not zero-initialized definitions, including specializations
// whose arguments have internal linkage.

struct descriptor {
  unsigned version;
  unsigned size;
};

template <auto K>
extern const descriptor descriptor_of;

void kernel(int);

namespace {
void local_kernel(int);
}

// CHECK-DAG: @_Z13descriptor_ofIXadL_Z6kerneliEEE = external {{.*}}constant
// CHECK-DAG: @_Z13descriptor_ofIXadL_ZN12_GLOBAL__N_1{{[^ ]*}}local_kernel{{[^ ]*}} = external {{.*}}constant
// CHECK-NOT: descriptor_of{{.*}} = {{.*}}zeroinitializer

const descriptor* external_one() { return &descriptor_of<&kernel>; }

unsigned internal_one() { return descriptor_of<&local_kernel>.size; }
