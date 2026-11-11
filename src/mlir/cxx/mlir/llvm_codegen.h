// Copyright (c) 2026 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <llvm/Passes/OptimizationLevel.h>

#include <memory>
#include <string>

namespace llvm {
class LLVMContext;
class Module;
class TargetMachine;
class raw_pwrite_stream;
}  // namespace llvm

namespace cxx {

class TranslationUnit;

// The code generation pipeline the cxx driver runs for -c and -S, as a library
// for tools that parse a translation unit themselves: C++ AST, cxx dialect,
// LLVM dialect, LLVM IR, optimized LLVM IR, machine code.

struct LLVMCodegenOptions {
  // -O level, 0 through 3.
  int optimizationLevel = 0;
  // Emits debug information (-g).
  bool debugInfo = false;
  // Compilation directory recorded in debug information.
  std::string debugCompilationDirectory;
};

// Maps an -O level to the LLVM optimization pipeline level.
[[nodiscard]] auto llvmOptimizationLevel(int level) -> llvm::OptimizationLevel;

// Creates the target machine that optimizes and emits code for |triple| at an
// -O level. Returns null and sets |error| when no target supports the triple.
[[nodiscard]] auto createTargetMachine(const std::string& triple,
                                       int optimizationLevel,
                                       std::string& error)
    -> std::unique_ptr<llvm::TargetMachine>;

// Generates optimized LLVM IR for a parsed and type-checked translation unit
// that has no errors. The module belongs to |context| and no longer refers to
// the translation unit. Returns null and sets |error| on failure.
[[nodiscard]] auto generateLLVMModule(TranslationUnit* unit,
                                      llvm::LLVMContext& context,
                                      llvm::TargetMachine* targetMachine,
                                      const LLVMCodegenOptions& options,
                                      std::string& error)
    -> std::unique_ptr<llvm::Module>;

// Emits |module| to |out| as an object file, or as assembly when |assembly| is
// set. Module-level and inline assembly are assembled by the integrated
// assembler. Returns false and sets |error| on failure.
[[nodiscard]] auto emitMachineCode(llvm::Module& module,
                                   llvm::TargetMachine* targetMachine,
                                   llvm::raw_pwrite_stream& out, bool assembly,
                                   std::string& error) -> bool;

}  // namespace cxx
