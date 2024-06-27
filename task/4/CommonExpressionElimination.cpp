#include "CommonExpressionElimination.hpp"
#include "llvm/IR/Operator.h"
#include <unordered_set>
using namespace llvm;

PreservedAnalyses
CommonExpressionElimination::run(Module& mod, ModuleAnalysisManager& mam)
{
  std::unordered_set<Function*> cleanFunction;
  bool newCleanFunction = true;
  while(newCleanFunction){
    newCleanFunction = false;
    for (auto& func : mod){
      if(func.isDeclaration() || cleanFunction.count(&func))
        continue;
      bool isClean = true;
      for(auto &bb: func)
        for(auto &inst: bb){
          if(isa<LoadInst>(inst) || isa<StoreInst>(inst)){
            isClean = false;
            break;
          }
          if(auto callInst = dyn_cast<CallInst>(&inst))
            if(callInst->getCalledFunction() != &func && !cleanFunction.count(callInst->getCalledFunction())){
              isClean = false;
              break;
            }
        }
      if(isClean){
        cleanFunction.insert(&func);
        newCleanFunction = true;
      }
    }
  }
  // 遍历所有函数
  for (auto& func : mod) {
    if(func.isDeclaration()) continue;
    mDomTree.recalculate(func);
    bool opMerged = true;
    while(opMerged){
      opMerged = false;
      std::multimap<std::pair<std::pair<Value*, Value*>, Instruction::BinaryOps>, Instruction*> preBinOp;
      std::multimap<std::vector<Value*>, Instruction*> preGEP;
      std::multimap<std::pair<Function*, std::vector<Value*>>, Instruction*> preCallInst;
      // 遍历每个函数的基本块
      for (auto& bb : func) {
        // 遍历每个基本块的指令
        for(auto it = bb.begin(); it != bb.end();){
          auto &inst = *it++;
          if(inst.user_empty())
            continue;
          if(auto binOp = dyn_cast<BinaryOperator>(&inst)){
            auto operands = std::make_pair(std::make_pair(binOp->getOperand(0), binOp->getOperand(1)), binOp->getOpcode());
            opMerged |= eliminate_common(preBinOp, binOp, operands);
          }
          if(auto gepOp = dyn_cast<GetElementPtrInst>(&inst)){
            std::vector<Value*> operands;
            for(int i = 0; i < gepOp->getNumOperands(); i++)
              operands.push_back(gepOp->getOperand(i));
            opMerged |= eliminate_common(preGEP, gepOp, operands);
          }
          if(auto callInst = dyn_cast<CallInst>(&inst)){
            if(!cleanFunction.count(callInst->getCalledFunction()))
              continue;
            std::vector<Value*> args;
            for(int i = 0; i < callInst->arg_size(); i++)
              args.push_back(callInst->getArgOperand(i));
            auto operands = std::make_pair(callInst->getCalledFunction(), args);
            opMerged |= eliminate_common(preCallInst, callInst, operands);
          }
        }
      }
    }
  }

  mOut << "CommonExpressionElimination running...\n";
  return PreservedAnalyses::all();
}

template<typename T>
bool CommonExpressionElimination::eliminate_common
  (std::multimap<T, llvm::Instruction*> &preInst, llvm::Instruction *inst, const T &operands)
{
  for(auto it = preInst.find(operands); it != preInst.end(); it++){
    if(it->first != operands)
      break;
    if(mDomTree.dominates(it->second->getParent(), inst->getParent())){
      inst->replaceAllUsesWith(it->second);
      inst->eraseFromParent();
      return true;
    }
    if(mDomTree.properlyDominates(inst->getParent(), it->second->getParent())){
      it->second->replaceAllUsesWith(inst);
      it->second->eraseFromParent();
      it->second = inst;
      return true;
    }
  }
  preInst.insert({operands, inst});
  return false;
}