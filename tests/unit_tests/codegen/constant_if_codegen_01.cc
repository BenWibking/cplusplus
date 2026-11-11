// RUN: %cxx -O0 -emit-llvm %s -o %t.ll
// RUN: %filecheck %s < %t.ll
// RUN: %filecheck %s --check-prefix=DISCARDED < %t.ll

// An if statement whose condition is a constant expression emits only the
// branch it takes, even at -O0, so the discarded branch references nothing.

// DISCARDED-NOT: @guarded

// Functions are emitted in reverse source order. Functions with run-time
// branches may place their return block before the calls.
// CHECK-LABEL: define {{.*}}@label_in_discarded
// CHECK: call void @labeled_call()
// CHECK-LABEL: define {{.*}}@runtime_condition
// CHECK: call void @runtime_call()
// CHECK: call void @kept()
// CHECK-LABEL: define {{.*}}@if_constexpr
// CHECK: call void @kept()
// CHECK: ret void
// CHECK-LABEL: define {{.*}}@not_constant_evaluated
// CHECK: call void @kept()
// CHECK: ret void
// CHECK-LABEL: define {{.*}}@with_initializer
// CHECK: call i32 @side_effect()
// CHECK: ret void
// CHECK-LABEL: define {{.*}}@nested
// CHECK: call void @kept()
// CHECK: ret void
// CHECK-LABEL: define {{.*}}@negated_else
// CHECK: call void @kept()
// CHECK: ret void
// CHECK-LABEL: define {{.*}}@constexpr_call
// CHECK-NOT: call
// CHECK: ret void
// CHECK-LABEL: define {{.*}}@literal_false
// CHECK: call void @kept()
// CHECK: ret void

extern "C" void guarded();
extern "C" void kept();
extern "C" void runtime_call();
extern "C" void labeled_call();
extern "C" int side_effect();

constexpr bool is_device() noexcept { return false; }

constexpr bool in_constant_evaluation() noexcept {
  return __builtin_is_constant_evaluated();
}

int global_flag = 0;

extern "C" void literal_false() {
  if (false) guarded();
  else kept();
}

extern "C" void constexpr_call() {
  if (is_device()) {
    guarded();
  }
}

extern "C" void negated_else() {
  if (!is_device()) {
    kept();
  } else {
    guarded();
  }
}

extern "C" void nested() {
  if (!is_device()) {
    if (is_device() && true) guarded();
    else if (sizeof(int) == 4) kept();
    else guarded();
  }
}

// The initializer still runs when the condition is constant.
extern "C" void with_initializer() {
  if (int value = side_effect(); is_device()) guarded();
}

// Code generation folds the condition as a run-time expression.
extern "C" void not_constant_evaluated() {
  if (in_constant_evaluation()) guarded();
  else kept();
}

extern "C" void if_constexpr() {
  if constexpr (is_device()) guarded();
  else kept();
}

// Run-time conditions keep both branches.
extern "C" void runtime_condition(int x) {
  if (x) runtime_call();
  if (global_flag) kept();
}

// A discarded branch holding a label stays: a goto may enter it.
extern "C" void label_in_discarded(int x) {
  if (x) goto inside;
  if (false) {
  inside:
    labeled_call();
  }
}
