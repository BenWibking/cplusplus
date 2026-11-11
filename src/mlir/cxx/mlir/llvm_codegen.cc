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

#include <cxx/codegen/codegen.h>
#include <cxx/mlir/cxx_dialect.h>
#include <cxx/mlir/cxx_dialect_conversions.h>
#include <cxx/mlir/llvm_codegen.h>
#include <cxx/mlir/mlir_emitter.h>
#include <cxx/translation_unit.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <mlir/IR/MLIRContext.h>

#include <format>

namespace cxx {

auto llvmOptimizationLevel(int level) -> llvm::OptimizationLevel {
  switch (level) {
    case 1:
      return llvm::OptimizationLevel::O1;
    case 2:
      return llvm::OptimizationLevel::O2;
    case 3:
      return llvm::OptimizationLevel::O3;
    default:
      return llvm::OptimizationLevel::O0;
  }
}

auto createTargetMachine(const std::string& tripleName, int optimizationLevel,
                         std::string& error)
    -> std::unique_ptr<llvm::TargetMachine> {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  // The integrated assembler parses inline and module-level assembly.
  llvm::InitializeAllAsmParsers();

  auto triple = llvm::Triple{tripleName};

  std::string lookupError;
  auto target = llvm::TargetRegistry::lookupTarget(triple, lookupError);

  if (!target) {
    error = std::format("cannot find target for triple '{}': {}",
                        triple.getTriple(), lookupError);
    return nullptr;
  }

  const auto codeGenOptLevel = [&] {
    switch (optimizationLevel) {
      case 1:
        return llvm::CodeGenOptLevel::Less;
      case 2:
        return llvm::CodeGenOptLevel::Default;
      case 3:
        return llvm::CodeGenOptLevel::Aggressive;
      default:
        return llvm::CodeGenOptLevel::None;
    }
  }();

  llvm::TargetOptions opt;
  // Static constructors go in .init_array, as the driver emits them.
  opt.UseInitArray = true;

  auto targetMachine =
      std::unique_ptr<llvm::TargetMachine>(target->createTargetMachine(
          llvm::Triple{triple}, "generic", "", opt,
          std::optional<llvm::Reloc::Model>(), std::nullopt, codeGenOptLevel));

  if (!targetMachine) {
    error = std::format("cannot create target machine for '{}'",
                        triple.getTriple());
  }

  return targetMachine;
}

auto generateLLVMModule(TranslationUnit* unit, llvm::LLVMContext& context,
                        llvm::TargetMachine* targetMachine,
                        const LLVMCodegenOptions& options, std::string& error)
    -> std::unique_ptr<llvm::Module> {
  mlir::MLIRContext mlirContext;
  mlirContext.loadDialect<mlir::cxx::CxxDialect>();

  auto emitter = ir::MlirEmitter{mlirContext, unit};
  auto codegen = Codegen{
      emitter,
      unit,
      {.debugInfo = options.debugInfo,
       .debugCompilationDirectory = options.debugCompilationDirectory,
       .runsAlwaysInliner = llvmPipelineRunsAlwaysInliner()}};

  auto ir = codegen(unit->ast());
  mlir::ModuleOp module = emitter.module(ir.module);

  if (!module) {
    error = "failed to generate IR for the C++ AST";
    return nullptr;
  }

  if (failed(lowerToMLIR(module))) {
    error = "failed to lower C++ AST to MLIR";
    return nullptr;
  }

  auto llvmModule = exportToLLVMIR(module, context);

  if (!llvmModule) {
    error = "failed to lower MLIR module to LLVM IR";
    return nullptr;
  }

  // As the driver does, the pipeline runs at every level; at -O0 it runs only
  // the passes that code generation requires.
  optimizeLLVMIR(*llvmModule, targetMachine,
                 llvmOptimizationLevel(options.optimizationLevel));

  return llvmModule;
}

auto emitMachineCode(llvm::Module& module, llvm::TargetMachine* targetMachine,
                     llvm::raw_pwrite_stream& out, bool assembly,
                     std::string& error) -> bool {
  llvm::legacy::PassManager pm;

  const auto fileType = assembly ? llvm::CodeGenFileType::AssemblyFile
                                 : llvm::CodeGenFileType::ObjectFile;

  if (targetMachine->addPassesToEmitFile(pm, out, nullptr, fileType)) {
    error = "target machine cannot emit assembly";
    return false;
  }

  pm.run(module);
  out.flush();
  return true;
}

}  // namespace cxx
