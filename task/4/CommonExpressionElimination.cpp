#include "CommonExpressionElimination.hpp"
#include "llvm/IR/Operator.h"
#include <map>
using namespace llvm;

PreservedAnalyses
CommonExpressionElimination::run(Module& mod, ModuleAnalysisManager& mam)
{
  // 遍历所有函数
  for (auto& func : mod) {
    // 遍历每个函数的基本块
    for (auto& bb : func) {
      // 遍历每个基本块的指令
      std::map<std::pair<std::pair<Value*, Value*>, Instruction::BinaryOps>, Instruction*> preBinOp;
      for(auto &inst: bb)
        if(auto binOp = dyn_cast<BinaryOperator>(&inst)){
          auto operands = std::make_pair(std::make_pair(binOp->getOperand(0), binOp->getOperand(1)), binOp->getOpcode());
          if(preBinOp.count(operands))
            binOp->replaceAllUsesWith(preBinOp[operands]);
          else
            preBinOp[operands] = binOp;
        }
    }
  }

  mOut << "CommonExpressionElimination running...\n";
  return PreservedAnalyses::all();
}
