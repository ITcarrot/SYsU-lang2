#include "LoadStoreMerging.hpp"
#include "llvm/IR/Operator.h"
#include <map>
using namespace llvm;

PreservedAnalyses
LoadStoreMerging::run(Module& mod, ModuleAnalysisManager& mam)
{
  int instEraseCount = 0;
  // 遍历所有函数
  for (auto& func : mod) {
    // 遍历每个函数的基本块
    for (auto& bb : func) {
      // 遍历每个基本块的指令
      std::map<std::pair<Type*, Value*>, Value*> loadedVar;
      std::map<std::pair<Type*, Value*>, Instruction*> preStoreInst;
      for(auto it = bb.begin(); it != bb.end();){
        auto &inst = *(it++);
        if(auto loadInst = dyn_cast<LoadInst>(&inst)){
          auto operands = std::make_pair(loadInst->getType(), loadInst->getOperand(0));
          if(loadedVar.count(operands))
            loadInst->replaceAllUsesWith(loadedVar[operands]);
          else
            loadedVar[operands] = loadInst;
        }
        if(auto storeInst= dyn_cast<StoreInst>(&inst)){
          auto operands = std::make_pair(storeInst->getOperand(0)->getType(), storeInst->getOperand(1));
          loadedVar[operands] = storeInst->getOperand(0);
          if(preStoreInst.count(operands))
            preStoreInst[operands]->eraseFromParent();
          preStoreInst[operands] = storeInst;
        }
      }
    }
  }

  mOut << "LoadStoreMerging running...\nTo eliminate " << instEraseCount
         << " instructions\n";
  return PreservedAnalyses::all();
}
