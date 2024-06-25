#include "DeadCodeElimination.hpp"
#include "llvm/IR/Operator.h"
using namespace llvm;

PreservedAnalyses
DeadCodeElimination::run(Module& mod, ModuleAnalysisManager& mam)
{
  int instEraseCount = 0;
  // 遍历所有函数
  for (auto& func : mod) {
    // 遍历每个函数的基本块
    for (auto& bb : func) {
      // 遍历每个基本块的指令
      for(auto it = bb.rbegin(); it != bb.rend();){
        auto &inst = *(it++);
        if (inst.use_empty() && !inst.isTerminator() && !inst.mayHaveSideEffects()){
          inst.eraseFromParent();
          instEraseCount++;
        }
      }
    }
  }

  mOut << "DeadCodeElimination running...\nTo eliminate " << instEraseCount
         << " instructions\n";
  return PreservedAnalyses::all();
}
