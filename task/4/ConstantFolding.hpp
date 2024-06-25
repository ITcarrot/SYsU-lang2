#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/raw_ostream.h>

class ConstantFolding : public llvm::PassInfoMixin<ConstantFolding>
{
public:
  explicit ConstantFolding(llvm::raw_ostream& out)
    : mOut(out)
  {
  }

  llvm::PreservedAnalyses run(llvm::Module& mod,
                              llvm::ModuleAnalysisManager& mam);

private:
  llvm::raw_ostream& mOut;
  static void push_up_add(llvm::Value* target, llvm::Value* var, bool neg, int64_t val);
};
