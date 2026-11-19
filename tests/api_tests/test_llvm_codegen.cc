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

#include <cxx/control.h>
#include <cxx/diagnostics_client.h>
#include <cxx/memory_layout.h>
#include <cxx/mlir/llvm_codegen.h>
#include <cxx/translation_unit.h>
#include <cxx/triple.h>
#include <gtest/gtest.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include <memory>
#include <string>

using namespace cxx;

namespace {

constexpr auto kTriple = "x86_64-unknown-linux-gnu";

// A parsed and type-checked unit for kTriple.
class Unit {
 public:
  explicit Unit(std::string source) : memoryLayout_{Triple{kTriple}} {
    unit_ = std::make_unique<TranslationUnit>(&diagnostics_);
    unit_->control()->setMemoryLayout(&memoryLayout_);
    unit_->setSource(std::move(source), "unit.cc");
    unit_->parse({.checkTypes = true});
  }

  [[nodiscard]] auto get() -> TranslationUnit* { return unit_.get(); }

 private:
  DiagnosticsClient diagnostics_;
  MemoryLayout memoryLayout_;
  std::unique_ptr<TranslationUnit> unit_;
};

// The target machine for kTriple, or null when the LLVM build has no x86
// target.
auto targetMachine(int optimizationLevel)
    -> std::unique_ptr<llvm::TargetMachine> {
  std::string error;
  return createTargetMachine(kTriple, optimizationLevel, error,
                             llvm::Reloc::PIC_);
}

constexpr auto kSource = "int add(int a, int b) { return a + b; }\n";

TEST(LLVMCodegen, GeneratesAndEmitsATranslationUnit) {
  auto machine = targetMachine(2);
  if (!machine) GTEST_SKIP() << "LLVM has no x86 target";
  EXPECT_EQ(machine->getRelocationModel(), llvm::Reloc::PIC_);

  Unit unit{kSource};
  llvm::LLVMContext context;
  std::string error;
  auto module = generateLLVMModule(unit.get(), context, machine.get(),
                                   {.optimizationLevel = 2}, error);
  ASSERT_NE(module, nullptr) << error;
  EXPECT_NE(module->getFunction("_Z3addii"), nullptr);

  llvm::SmallString<0> object;
  llvm::raw_svector_ostream objectStream{object};
  ASSERT_TRUE(emitMachineCode(*module, machine.get(), objectStream,
                              /*assembly=*/false, error))
      << error;
  ASSERT_GE(object.size(), 4u);
  EXPECT_EQ(object.substr(0, 4),
            "\x7f"
            "ELF");

  llvm::SmallString<0> assembly;
  llvm::raw_svector_ostream assemblyStream{assembly};
  ASSERT_TRUE(emitMachineCode(*module, machine.get(), assemblyStream,
                              /*assembly=*/true, error))
      << error;
  EXPECT_NE(assembly.find("_Z3addii:"), llvm::StringRef::npos);
}

TEST(LLVMCodegen, AssemblesModuleLevelAssembly) {
  auto machine = targetMachine(0);
  if (!machine) GTEST_SKIP() << "LLVM has no x86 target";

  Unit unit{std::string(kSource)};
  llvm::LLVMContext context;
  std::string error;
  auto module =
      generateLLVMModule(unit.get(), context, machine.get(), {}, error);
  ASSERT_NE(module, nullptr) << error;
  module->appendModuleInlineAsm(".globl marker\nmarker:\n  nop\n");

  llvm::SmallString<0> object;
  llvm::raw_svector_ostream stream{object};
  EXPECT_TRUE(emitMachineCode(*module, machine.get(), stream,
                              /*assembly=*/false, error))
      << error;
}

TEST(LLVMCodegen, RejectsAnUnknownTriple) {
  std::string error;
  auto machine = createTargetMachine("nonexistent-unknown-unknown", 0, error);
  EXPECT_EQ(machine, nullptr);
  EXPECT_NE(error.find("cannot find target"), std::string::npos) << error;
}

TEST(LLVMCodegen, MapsOptimizationLevels) {
  EXPECT_EQ(llvmOptimizationLevel(0), llvm::OptimizationLevel::O0);
  EXPECT_EQ(llvmOptimizationLevel(1), llvm::OptimizationLevel::O1);
  EXPECT_EQ(llvmOptimizationLevel(2), llvm::OptimizationLevel::O2);
  EXPECT_EQ(llvmOptimizationLevel(3), llvm::OptimizationLevel::O3);
}

}  // namespace
