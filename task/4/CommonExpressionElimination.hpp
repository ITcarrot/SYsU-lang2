#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/IR/Dominators.h"
#include <map>

class CommonExpressionElimination : public llvm::PassInfoMixin<CommonExpressionElimination>
{
public:
  explicit CommonExpressionElimination(llvm::raw_ostream& out)
    : mOut(out)
  {
  }

  llvm::PreservedAnalyses run(llvm::Module& mod,
                              llvm::ModuleAnalysisManager& mam);

private:
  llvm::DominatorTree mDomTree;
  llvm::raw_ostream& mOut;
  template<typename T>
  bool eliminate_common(std::multimap<T, llvm::Instruction*> &preInst, llvm::Instruction *inst, const T &operands);
};
