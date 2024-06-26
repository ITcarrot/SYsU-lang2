#include "ConstantBranchReplace.hpp"
#include "llvm/IR/Operator.h"
using namespace llvm;

PreservedAnalyses
ConstantBranchReplace::run(Module& mod, ModuleAnalysisManager& mam)
{
  int instEraseCount = 0;
  // 遍历所有函数
  for (auto& func : mod) {
    // 遍历每个函数的基本块
    for (auto& bb : func) {
      // 遍历每个基本块的指令
      for(auto it = bb.begin(); it != bb.end();){
        auto &inst = *(it++);
        if(auto branchInst = dyn_cast<BranchInst>(&inst))
          if(branchInst->isConditional())
            if(auto cond = dyn_cast<ConstantInt>(branchInst->getCondition())){
              BasicBlock *dest = cond->isOne() ? branchInst->getSuccessor(0) : branchInst->getSuccessor(1);
              BranchInst::Create(dest, branchInst);
              branchInst->eraseFromParent();
            }
      }
    }
  }

  mOut << "ConstantBranchReplace running...\n";
  return PreservedAnalyses::all();
}
