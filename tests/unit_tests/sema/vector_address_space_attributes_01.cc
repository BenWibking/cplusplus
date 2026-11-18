// RUN: %cxx -verify -fsyntax-only -fvalidate-ast %s

template <class T> using Device = T __attribute__((address_space(1)));
template <class T> using Bytes = T __attribute__((vector_size(16)));
template <class T> using Lanes = T __attribute__((ext_vector_type(4)));

using DeviceElements = int __attribute__((address_space(1), vector_size(16)));
using DeviceVector = int __attribute__((vector_size(16), address_space(1)));
static_assert(__is_same(DeviceElements, Bytes<Device<int>>));
static_assert(__is_same(DeviceVector, Device<Bytes<int>>));
static_assert(!__is_same(DeviceElements, DeviceVector));

constexpr int select(Device<Bytes<int>>*) { return 1; }
constexpr int select(...) { return 0; }
static_assert(select(static_cast<DeviceVector*>(nullptr)) == 1);

template <class T>
using DeviceLanes = T [[clang::ext_vector_type(4)]] [[clang::address_space(1)]];
static_assert(__is_same(DeviceLanes<int>, Device<Lanes<int>>));

using NestedSpaces =
    int __attribute__((address_space(1), ext_vector_type(4), address_space(2)));
using SharedLanes = Lanes<Device<int>> __attribute__((address_space(2)));
static_assert(__is_same(NestedSpaces, SharedLanes));

// expected-error@+1 {{invalid vector element type 'int __attribute__((vector_size(4 * sizeof(int))))'}}
using Twice = int __attribute__((vector_size(16), vector_size(16)));
// expected-error@+1 {{invalid vector element type 'int __attribute__((ext_vector_type(4)))'}}
using Mixed = int __attribute__((ext_vector_type(4), vector_size(16)));
