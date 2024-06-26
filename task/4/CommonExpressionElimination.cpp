#include "CommonExpressionElimination.hpp"
#include "llvm/IR/Operator.h"
#include <map>
using namespace llvm;

PreservedAnalyses
CommonExpressionElimination::run(Module& mod, ModuleAnalysisManager& mam)
{
  // 遍历所有函数
  for (auto& func : mod) {
    if(func.isDeclaration()) continue;
    mDomTree.recalculate(func);
    bool opMerged = true;
    while(opMerged){
      opMerged = false;
      std::multimap<std::pair<std::pair<Value*, Value*>, Instruction::BinaryOps>, Instruction*> preBinOp;
      std::multimap<std::vector<Value*>, Instruction*> preGEP;
      // 遍历每个函数的基本块
      for (auto& bb : func) {
        // 遍历每个基本块的指令
        for(auto &inst: bb){
          if(inst.user_empty())
            continue;
          if(auto binOp = dyn_cast<BinaryOperator>(&inst)){
            auto operands = std::make_pair(std::make_pair(binOp->getOperand(0), binOp->getOperand(1)), binOp->getOpcode());
            bool replaced = false;
            for(auto it = preBinOp.find(operands); it != preBinOp.end(); it++){
              if(it->first != operands)
                break;
              if(mDomTree.dominates(it->second->getParent(), binOp->getParent())){
                binOp->replaceAllUsesWith(it->second);
                replaced = true;
                break;
              }
              if(mDomTree.properlyDominates(binOp->getParent(), it->second->getParent())){
                it->second->replaceAllUsesWith(binOp);
                it->second = binOp;
                replaced = true;
                break;
              }
            }
            if(!replaced)
              preBinOp.insert({operands, binOp});
            else
              opMerged = true;
          }
          if(auto gepOp = dyn_cast<GetElementPtrInst>(&inst)){
            std::vector<Value*> operands;
            bool replaced = false;
            for(int i = 0; i < gepOp->getNumOperands(); i++)
              operands.push_back(gepOp->getOperand(i));
            for(auto it = preGEP.find(operands); it != preGEP.end(); it++){
              if(it->first != operands)
                break;
              if(mDomTree.dominates(it->second->getParent(), gepOp->getParent())){
                gepOp->replaceAllUsesWith(it->second);
                replaced = true;
                break;
              }
              if(mDomTree.properlyDominates(gepOp->getParent(), it->second->getParent())){
                it->second->replaceAllUsesWith(gepOp);
                it->second = gepOp;
                replaced = true;
                break;
              }
            }
            if(!replaced)
              preGEP.insert({operands, gepOp});
            else
              opMerged = true;
          }
        }
      }
    }
  }

  mOut << "CommonExpressionElimination running...\n";
  return PreservedAnalyses::all();
}
