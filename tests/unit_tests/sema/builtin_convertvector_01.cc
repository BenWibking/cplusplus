// RUN: %cxx -verify -fsyntax-only %s
// expected-no-diagnostics

using Int4 = int __attribute__((ext_vector_type(4)));
using UInt4 = unsigned __attribute__((ext_vector_type(4)));
using SByte4 = signed char __attribute__((ext_vector_type(4)));
using UByte4 = unsigned char __attribute__((ext_vector_type(4)));
using Float4 = float __attribute__((ext_vector_type(4)));
using Bool4 = bool __attribute__((ext_vector_type(4)));
using Half4 = _Float16 __attribute__((ext_vector_type(4)));
using BFloat4 = __bf16 __attribute__((ext_vector_type(4)));
using E4M3FN4 = __float8_e4m3fn __attribute__((ext_vector_type(4)));
using E5M2_4 = __float8_e5m2 __attribute__((ext_vector_type(4)));

static_assert(__has_builtin(__builtin_convertvector));

template <typename To, typename From>
constexpr To convert(From value) {
  return __builtin_convertvector(value, To);
}

constexpr Float4 floating{1.75f, -2.75f, 257.5f, 0.0f};
constexpr auto integers = convert<Int4>(floating);
static_assert(integers[0] == 1 && integers[1] == -2);
static_assert(integers[2] == 257 && integers[3] == 0);

constexpr auto bytes = convert<UByte4>(integers);
static_assert(bytes[0] == 1 && bytes[1] == 254);
static_assert(bytes[2] == 1 && bytes[3] == 0);

constexpr SByte4 signedBytes{-1, -2, 126, 127};
constexpr auto signedIntegers = convert<Int4>(signedBytes);
static_assert(signedIntegers[0] == -1 && signedIntegers[1] == -2);
static_assert(signedIntegers[2] == 126 && signedIntegers[3] == 127);

constexpr auto booleans = convert<Bool4>(floating);
static_assert(booleans[0] && booleans[1] && booleans[2] && !booleans[3]);
constexpr auto booleanIntegers = convert<UInt4>(booleans);
static_assert(booleanIntegers[0] == 1 && booleanIntegers[3] == 0);

constexpr Float4 exactFloating{1.0f, -2.0f, 448.0f, 0.125f};
constexpr auto halves = convert<Half4>(exactFloating);
constexpr auto bfloats = convert<BFloat4>(halves);
static_assert(bfloats[0] == 1.0 && bfloats[1] == -2.0);
static_assert(bfloats[2] == 448.0 && bfloats[3] == 0.125);

constexpr Int4 float8Inputs{1, -2, 1000, 0};
constexpr auto e4m3fn = convert<E4M3FN4>(float8Inputs);
static_assert(e4m3fn[0] == 1.0 && e4m3fn[1] == -2.0);
static_assert(e4m3fn[2] == 448.0 && e4m3fn[3] == 0.0);
constexpr auto e5m2 = convert<E5M2_4>(e4m3fn);
static_assert(e5m2[0] == 1.0 && e5m2[1] == -2.0);
static_assert(e5m2[2] == 448.0 && e5m2[3] == 0.0);

using UInt64x2 = unsigned long long __attribute__((ext_vector_type(2)));
using Double2 = double __attribute__((ext_vector_type(2)));
constexpr UInt64x2 large{0xffffffffffffffffULL, 0x8000000000000000ULL};
constexpr auto doubles = convert<Double2>(large);
static_assert(doubles[0] == 0x1p64 && doubles[1] == 0x1p63);
