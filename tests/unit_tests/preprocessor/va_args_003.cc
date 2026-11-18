// RUN: %cxx -E -P %s -o - | %filecheck %s

#define X(a, ...) static_assert(a, ## __VA_ARGS__)

#define reason "it must be true"
X(true);
X(true, reason);
X(true,);

//      CHECK: static_assert(true);
// CHECK-NEXT: static_assert(true, "it must be true");
// CHECK-NEXT: static_assert(true,);

