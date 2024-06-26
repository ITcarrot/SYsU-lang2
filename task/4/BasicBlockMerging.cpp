#include "BasicBlockMerging.hpp"
#include "llvm/IR/Operator.h"
using namespace llvm;

PreservedAnalyses
BasicBlockMerging::run(Module& mod, ModuleAnalysisManager& mam)
{
  // 遍历所有函数
  for (auto& func : mod) {
    // 遍历每个函数的基本块
    bool mergedBlock = true;
    while(mergedBlock){
      mergedBlock = false;
      for (auto& bb : func) {
        if(auto predBB = bb.getSinglePredecessor())
          if(predBB->getSingleSuccessor() == &bb && !predBB->empty()){
            auto it = predBB->rbegin();
            for(it++; it != predBB->rend();){
              auto &predInst = *it++;
              predInst.removeFromParent();
              predInst.insertInto(&bb, bb.begin());
            }
            predBB->replaceAllUsesWith(&bb);
            mergedBlock = true;
          }
      }
    }
    for(auto it = func.begin(); it != func.end();){
      auto &bb = *it++;
      if(bb.hasNPredecessors(0) && !bb.isEntryBlock())
        bb.eraseFromParent();
    }
  }

  mOut << "BasicBlockMerging running...\n";
  return PreservedAnalyses::all();
}
