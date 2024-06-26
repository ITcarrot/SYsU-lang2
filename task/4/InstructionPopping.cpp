#include "InstructionPopping.hpp"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Passes/PassBuilder.h"
#include <type_traits>
using namespace llvm;

PreservedAnalyses
InstructionPopping::run(Module& mod, ModuleAnalysisManager& mam)
{
  DominatorTree domTree;
  FunctionAnalysisManager fam;
  PassBuilder pb;
  fam.registerPass( [&]{return LoopAnalysis();} );
  pb.registerFunctionAnalyses(fam);
  // 遍历所有函数
  for (auto& func : mod) {
    if(func.isDeclaration()) continue;
    domTree.recalculate(func);
    LoopInfo& loopInfo = fam.getResult<LoopAnalysis>(func);
    bool insturctionPopped = true;
    while(insturctionPopped){
      insturctionPopped = false;
      // 遍历每个函数的基本块
      for (auto& bb : func) {
        // 遍历每个基本块的指令
        for(auto it = bb.begin(); it != bb.end();){
          auto &inst = *(it++);
          if(auto binOp = dyn_cast<BinaryOperator>(&inst)){
            auto highOp = dyn_cast<Instruction>(binOp->getOperand(0));
            auto lowOp = dyn_cast<Instruction>(binOp->getOperand(1));
            if(!highOp && !lowOp){
              if(binOp->getParent() != &func.getEntryBlock()){
                binOp->removeFromParent();
                binOp->insertInto(&func.getEntryBlock(), func.getEntryBlock().begin());
                insturctionPopped = true;
              }
              continue;
            }
            if(!highOp) highOp = lowOp;
            if(!lowOp) lowOp = highOp;

            auto highBB = highOp->getParent();
            auto lowBB = lowOp->getParent();
            if(domTree.properlyDominates(lowBB, highBB)){
              std::swap(highOp, lowOp);
              std::swap(highBB, lowBB);
            }

            BasicBlock *currentBB = binOp->getParent();
            BasicBlock *targetBB = currentBB;
            int targetDepth = loopInfo.getLoopDepth(currentBB);
            while(currentBB != lowBB){
              currentBB = domTree.getNode(currentBB)->getIDom()->getBlock();
              if(loopInfo.getLoopDepth(currentBB) < targetDepth){
                targetDepth = loopInfo.getLoopDepth(currentBB);
                targetBB = currentBB;
              }
            }
            if(targetBB == binOp->getParent())
              continue;

            if(targetBB == lowBB){
              binOp->moveAfter(lowOp);
              insturctionPopped = true;
            }else{
              binOp->removeFromParent();
              binOp->insertInto(targetBB, targetBB->begin());
              insturctionPopped = true;
            }
          }
        }
      }
    }
  }

  mOut << "InstructionPopping running...\n";
  return PreservedAnalyses::all();
}
