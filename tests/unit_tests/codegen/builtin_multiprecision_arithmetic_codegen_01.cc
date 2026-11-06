// RUN: %cxx -emit-ir %s -o - | %filecheck %s

unsigned add(unsigned lhs, unsigned rhs, unsigned carry_in,
             unsigned* carry_out) {
  return __builtin_addc(lhs, rhs, carry_in, carry_out);
}

unsigned subtract(unsigned lhs, unsigned rhs, unsigned borrow_in,
                  unsigned* borrow_out) {
  return __builtin_subc(lhs, rhs, borrow_in, borrow_out);
}

// CHECK-LABEL: cxx.func @_Z8subtractjjjPj
// CHECK: %[[FIRST:.+]] = arith.subi %{{.+}}, %{{.+}} : i32
// CHECK: %[[FIRST_BORROW:.+]] = arith.cmpi ult, %{{.+}}, %{{.+}} : i32
// CHECK: %[[RESULT:.+]] = arith.subi %[[FIRST]], %{{.+}} : i32
// CHECK: %[[SECOND_BORROW:.+]] = arith.cmpi ult, %[[FIRST]], %{{.+}} : i32
// CHECK: %[[BORROW:.+]] = arith.ori %[[FIRST_BORROW]], %[[SECOND_BORROW]] : i1
// CHECK: %[[STORED_BORROW:.+]] = arith.extui %[[BORROW]] : i1 to i32
// CHECK: cxx.store %[[STORED_BORROW]], %{{.+}} : i32, !{{.+}} align 4

// CHECK-LABEL: cxx.func @_Z3addjjjPj
// CHECK: %[[FIRST:.+]] = arith.addi %{{.+}}, %{{.+}} : i32
// CHECK: %[[FIRST_CARRY:.+]] = arith.cmpi ult, %[[FIRST]], %{{.+}} : i32
// CHECK: %[[RESULT:.+]] = arith.addi %[[FIRST]], %{{.+}} : i32
// CHECK: %[[SECOND_CARRY:.+]] = arith.cmpi ult, %[[RESULT]], %[[FIRST]] : i32
// CHECK: %[[CARRY:.+]] = arith.ori %[[FIRST_CARRY]], %[[SECOND_CARRY]] : i1
// CHECK: %[[STORED_CARRY:.+]] = arith.extui %[[CARRY]] : i1 to i32
// CHECK: cxx.store %[[STORED_CARRY]], %{{.+}} : i32, !{{.+}} align 4
