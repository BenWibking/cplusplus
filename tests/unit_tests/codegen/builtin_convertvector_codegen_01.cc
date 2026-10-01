// RUN: %cxx -emit-ir %s -o - | %filecheck %s --check-prefixes=COMMON,MLIR
// RUN: %cxx -fexperimental-cxxir -emit-ir %s -o - | %filecheck %s --check-prefixes=COMMON,CXXIR

using Int4 = int __attribute__((ext_vector_type(4)));
using UInt4 = unsigned __attribute__((ext_vector_type(4)));
using Short4 = short __attribute__((ext_vector_type(4)));
using SByte4 = signed char __attribute__((ext_vector_type(4)));
using UByte4 = unsigned char __attribute__((ext_vector_type(4)));
using Float4 = float __attribute__((ext_vector_type(4)));
using Bool4 = bool __attribute__((ext_vector_type(4)));
using Half4 = _Float16 __attribute__((ext_vector_type(4)));
using BFloat4 = __bf16 __attribute__((ext_vector_type(4)));
using E4M3FN4 = __float8_e4m3fn __attribute__((ext_vector_type(4)));
using E5M2_4 = __float8_e5m2 __attribute__((ext_vector_type(4)));

Float4 unsignedToFloat(UByte4 value) {
  return __builtin_convertvector(value, Float4);
}

Int4 floatToSigned(Float4 value) {
  return __builtin_convertvector(value, Int4);
}

UInt4 floatToUnsigned(Float4 value) {
  return __builtin_convertvector(value, UInt4);
}

Int4 signExtend(SByte4 value) {
  return __builtin_convertvector(value, Int4);
}

UInt4 zeroExtend(UByte4 value) {
  return __builtin_convertvector(value, UInt4);
}

Short4 truncate(Int4 value) {
  return __builtin_convertvector(value, Short4);
}

Bool4 floatToBool(Float4 value) {
  return __builtin_convertvector(value, Bool4);
}

Bool4 integerToBool(UInt4 value) {
  return __builtin_convertvector(value, Bool4);
}

Int4 fromBool(Bool4 value) {
  return __builtin_convertvector(value, Int4);
}

Half4 toHalf(BFloat4 value) {
  return __builtin_convertvector(value, Half4);
}

BFloat4 toBFloat(Half4 value) {
  return __builtin_convertvector(value, BFloat4);
}

E4M3FN4 toFloat8(Int4 value) {
  return __builtin_convertvector(value, E4M3FN4);
}

extern "C" E5M2_4 toFloat8E5(Int4 value) {
  return __builtin_convertvector(value, E5M2_4);
}

extern "C" E4M3FN4 e5ToE4(E5M2_4 value) {
  return __builtin_convertvector(value, E4M3FN4);
}

extern "C" E5M2_4 e4ToE5(E4M3FN4 value) {
  return __builtin_convertvector(value, E5M2_4);
}

// CXXIR-DAG: ![[I1:cxxir[0-9]+]] = vector<4xi1>
// CXXIR-DAG: ![[I8:cxxir[0-9]+]] = vector<4xi8>
// CXXIR-DAG: ![[I16:cxxir[0-9]+]] = vector<4xi16>
// CXXIR-DAG: ![[I32:cxxir[0-9]+]] = vector<4xi32>
// CXXIR-DAG: ![[BF16:cxxir[0-9]+]] = vector<4xbf16>
// CXXIR-DAG: ![[F16:cxxir[0-9]+]] = vector<4xf16>
// CXXIR-DAG: ![[F32:cxxir[0-9]+]] = vector<4xf32>
// CXXIR-DAG: ![[E4:cxxir[0-9]+]] = vector<4xf8E4M3FN>
// CXXIR-DAG: ![[E5:cxxir[0-9]+]] = vector<4xf8E5M2>

// COMMON-LABEL: cxx.func @e4ToE5
// MLIR: arith.extf {{.*}} : vector<4xf8E4M3FN> to vector<4xf32>
// MLIR-NEXT: arith.truncf {{.*}} : vector<4xf32> to vector<4xf8E5M2>
// CXXIR: arith.extf {{.*}} : ![[E4]] to ![[F32]]
// CXXIR-NEXT: arith.truncf {{.*}} : ![[F32]] to ![[E5]]

// COMMON-LABEL: cxx.func @e5ToE4
// MLIR: arith.extf {{.*}} : vector<4xf8E5M2> to vector<4xf32>
// MLIR: arith.cmpf ogt, {{.*}} : vector<4xf32>
// MLIR: arith.cmpf olt, {{.*}} : vector<4xf32>
// MLIR: arith.truncf {{.*}} : vector<4xf32> to vector<4xf8E4M3FN>
// CXXIR: arith.extf {{.*}} : ![[E5]] to ![[F32]]
// CXXIR: arith.cmpf ogt, {{.*}} : ![[F32]]
// CXXIR: arith.cmpf olt, {{.*}} : ![[F32]]
// CXXIR: arith.truncf {{.*}} : ![[F32]] to ![[E4]]

// COMMON-LABEL: cxx.func @toFloat8E5
// MLIR: arith.sitofp {{.*}} : vector<4xi32> to vector<4xf8E5M2>
// CXXIR: arith.sitofp {{.*}} : ![[I32]] to ![[E5]]

// COMMON-LABEL: cxx.func @_Z8toFloat8Dv4_i
// MLIR: arith.sitofp {{.*}} : vector<4xi32> to vector<4xf32>
// MLIR: arith.cmpf ogt, {{.*}} : vector<4xf32>
// MLIR: arith.cmpf olt, {{.*}} : vector<4xf32>
// MLIR: arith.truncf {{.*}} : vector<4xf32> to vector<4xf8E4M3FN>
// CXXIR: arith.sitofp {{.*}} : ![[I32]] to ![[F32]]
// CXXIR: arith.cmpf ogt, {{.*}} : ![[F32]]
// CXXIR: arith.cmpf olt, {{.*}} : ![[F32]]
// CXXIR: arith.truncf {{.*}} : ![[F32]] to ![[E4]]

// COMMON-LABEL: cxx.func @_Z8toBFloatDv4_DF16_
// MLIR: arith.extf {{.*}} : vector<4xf16> to vector<4xf32>
// MLIR-NEXT: arith.truncf {{.*}} : vector<4xf32> to vector<4xbf16>
// CXXIR: arith.extf {{.*}} : ![[F16]] to ![[F32]]
// CXXIR-NEXT: arith.truncf {{.*}} : ![[F32]] to ![[BF16]]

// COMMON-LABEL: cxx.func @_Z6toHalfDv4_DF16b
// MLIR: arith.extf {{.*}} : vector<4xbf16> to vector<4xf32>
// MLIR-NEXT: arith.truncf {{.*}} : vector<4xf32> to vector<4xf16>
// CXXIR: arith.extf {{.*}} : ![[BF16]] to ![[F32]]
// CXXIR-NEXT: arith.truncf {{.*}} : ![[F32]] to ![[F16]]

// COMMON-LABEL: cxx.func @_Z8fromBoolDv4_b
// MLIR: arith.extui {{.*}} : vector<4xi1> to vector<4xi32>
// CXXIR: arith.extui {{.*}} : ![[I1]] to ![[I32]]

// COMMON-LABEL: cxx.func @_Z13integerToBoolDv4_j
// MLIR: arith.cmpi ne, {{.*}} : vector<4xi32>
// CXXIR: arith.cmpi ne, {{.*}} : ![[I32]]

// COMMON-LABEL: cxx.func @_Z11floatToBoolDv4_f
// MLIR: arith.cmpf une, {{.*}} : vector<4xf32>
// CXXIR: arith.cmpf une, {{.*}} : ![[F32]]

// COMMON-LABEL: cxx.func @_Z8truncateDv4_i
// MLIR: arith.trunci {{.*}} : vector<4xi32> to vector<4xi16>
// CXXIR: arith.trunci {{.*}} : ![[I32]] to ![[I16]]

// COMMON-LABEL: cxx.func @_Z10zeroExtendDv4_h
// MLIR: arith.extui {{.*}} : vector<4xi8> to vector<4xi32>
// CXXIR: arith.extui {{.*}} : ![[I8]] to ![[I32]]

// COMMON-LABEL: cxx.func @_Z10signExtendDv4_a
// MLIR: arith.extsi {{.*}} : vector<4xi8> to vector<4xi32>
// CXXIR: arith.extsi {{.*}} : ![[I8]] to ![[I32]]

// COMMON-LABEL: cxx.func @_Z15floatToUnsignedDv4_f
// MLIR: arith.fptoui {{.*}} : vector<4xf32> to vector<4xi32>
// CXXIR: arith.fptoui {{.*}} : ![[F32]] to ![[I32]]

// COMMON-LABEL: cxx.func @_Z13floatToSignedDv4_f
// MLIR: arith.fptosi {{.*}} : vector<4xf32> to vector<4xi32>
// CXXIR: arith.fptosi {{.*}} : ![[F32]] to ![[I32]]

// COMMON-LABEL: cxx.func @_Z15unsignedToFloatDv4_h
// MLIR: arith.uitofp {{.*}} : vector<4xi8> to vector<4xf32>
// CXXIR: arith.uitofp {{.*}} : ![[I8]] to ![[F32]]
