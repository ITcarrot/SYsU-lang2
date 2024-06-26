#include "InstructionPopping.hpp"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include <type_traits>
using namespace llvm;

PreservedAnalyses
InstructionPopping::run(Module& mod, ModuleAnalysisManager& mam)
{
  // 遍历所有函数
  for (auto& func : mod) {
    if(func.isDeclaration()) continue;
    mDomTree.recalculate(func);
    LoopInfo& loopInfo = mFam.getResult<LoopAnalysis>(func);
    bool insturctionPopped = true;

    while(insturctionPopped){
      insturctionPopped = false;
      // 遍历每个函数的基本块
      for (auto& bb : func) {
        // 遍历每个基本块的指令
        for(auto it = bb.begin(); it != bb.end();){
          auto &inst = *(it++);
          if(isa<BinaryOperator>(inst) || isa<CmpInst>(inst) || isa<GetElementPtrInst>(inst)
            || isa<ZExtInst>(inst) || isa<SExtInst>(inst)){
            Instruction *lowOp = nullptr;
            for(int i = 0; i < inst.getNumOperands(); i++)
              if(auto opI = dyn_cast<Instruction>(inst.getOperand(i))){
                if(!lowOp)
                  lowOp = opI;
                else if(mDomTree.properlyDominates(lowOp->getParent(), opI->getParent()))
                  lowOp = opI;
                else if(lowOp->getParent() == opI->getParent() && lowOp->comesBefore(opI))
                  lowOp = opI;
              }

            if(!lowOp){
              if(inst.getParent() != &func.getEntryBlock()){
                inst.removeFromParent();
                inst.insertInto(&func.getEntryBlock(), func.getEntryBlock().begin());
                insturctionPopped = true;
              }
              continue;
            }

            auto lowBB = lowOp->getParent();
            auto currentBB = inst.getParent();
            BasicBlock *targetBB = currentBB;
            int targetDepth = loopInfo.getLoopDepth(currentBB);
            while(currentBB != lowBB){
              currentBB = mDomTree.getNode(currentBB)->getIDom()->getBlock();
              if(loopInfo.getLoopDepth(currentBB) < targetDepth){
                targetDepth = loopInfo.getLoopDepth(currentBB);
                targetBB = currentBB;
              }
            }
            if(targetBB == inst.getParent())
              continue;

            if(targetBB == lowBB)
              inst.moveAfter(lowOp);
            else{
              inst.removeFromParent();
              inst.insertInto(targetBB, targetBB->begin());
            }
            insturctionPopped = true;
          }
        }
      }
    }
  }

  mOut << "InstructionPopping running...\n";
  return PreservedAnalyses::all();
}
