
#include "FunctionInline.hpp"
#include "llvm/IR/Instructions.h"
#include <unordered_set>

using namespace llvm;

PreservedAnalyses
FunctionInline::run(Module& mod, ModuleAnalysisManager& mam)
{
  std::unordered_set<Function*> canInline;
  bool newInline = true;
  while(newInline){
    newInline = false;
    for(auto &func: mod)
      if(!canInline.count(&func) && !func.isDeclaration()
        && succ_size(&func.getEntryBlock()) == 0){
        bool hasCall = false;
        for(auto &inst: func.getEntryBlock())
          if(auto callInst = dyn_cast<CallInst>(&inst)){
            hasCall = true;
            break;
          }
        if(!hasCall)
          canInline.insert(&func);
      }
    
    for(auto &func: mod)
      if(!canInline.count(&func))
        for(auto &bb: func)
          for(auto it = bb.begin(); it != bb.end();){
            auto &inst = *it++;
            if(auto callInst = dyn_cast<CallInst>(&inst))
              if(canInline.count(callInst->getCalledFunction())){
                std::unordered_map<Value*, Value*> varMap;
                auto calledFunc = callInst->getCalledFunction();

                for(int i = 0; i < callInst->arg_size(); i++)
                  varMap[calledFunc->getArg(i)] = callInst->getArgOperand(i);
                for(auto &inst: calledFunc->getEntryBlock()){
                  if(auto returnInst = dyn_cast<ReturnInst>(&inst))
                    break;
                  auto clonedInst = inst.clone();
                  varMap[&inst] = clonedInst;
                  clonedInst->insertBefore(callInst);
                }

                for(auto &inst: calledFunc->getEntryBlock()){
                  if(auto returnInst = dyn_cast<ReturnInst>(&inst)){
                    if(varMap.count(returnInst->getReturnValue()))
                      callInst->replaceAllUsesWith(varMap[returnInst->getReturnValue()]);
                    else if(returnInst->getReturnValue())
                      callInst->replaceAllUsesWith(returnInst->getReturnValue());
                    break;
                  }
                  auto clonedInst = dyn_cast<Instruction>(varMap[&inst]);
                  for(int i = 0; i < clonedInst->getNumOperands(); i++){
                    if(varMap.count(clonedInst->getOperand(i)))
                      clonedInst->setOperand(i, varMap[clonedInst->getOperand(i)]);
                  }
                }
                callInst->eraseFromParent();
                newInline = true;
              }
          }
  }

  mOut << "FunctionInline running...\n";
  return PreservedAnalyses::all();
}
