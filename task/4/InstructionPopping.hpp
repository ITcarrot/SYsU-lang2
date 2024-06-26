#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Passes/PassBuilder.h"

class InstructionPopping : public llvm::PassInfoMixin<InstructionPopping>
{
public:
  explicit InstructionPopping(llvm::raw_ostream& out)
    : mOut(out)
  {
    mFam.registerPass( [&]{return llvm::LoopAnalysis();} );
    mPb.registerFunctionAnalyses(mFam);
  }

  llvm::PreservedAnalyses run(llvm::Module& mod,
                              llvm::ModuleAnalysisManager& mam);

private:
  llvm::raw_ostream& mOut;
  llvm::DominatorTree mDomTree;
  llvm::FunctionAnalysisManager mFam;
  llvm::PassBuilder mPb;
};
