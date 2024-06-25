#include "ConstantFolding.hpp"
#include "llvm/IR/Operator.h"

using namespace llvm;

PreservedAnalyses
ConstantFolding::run(Module& mod, ModuleAnalysisManager& mam)
{
  // 遍历所有函数
  for (auto& func : mod) {
    // 遍历每个函数的基本块
    for (auto& bb : func) {
      // 遍历每个基本块的指令
      for (auto& inst : bb) {
        // 判断当前指令是否是二元运算指令
        if (auto binOp = dyn_cast<BinaryOperator>(&inst)) {
          // 获取二元运算指令的左右操作数，并尝试转换为常整数
          Value* lhs = binOp->getOperand(0);
          Value* rhs = binOp->getOperand(1);
          auto constLhs = dyn_cast<ConstantInt>(lhs);
          auto constRhs = dyn_cast<ConstantInt>(rhs);
          // 若左右操作数均为整数常量，则进行常量折叠与use替换
          if (constLhs && constRhs) {
            switch (binOp->getOpcode()) {
              case Instruction::Add: 
                binOp->replaceAllUsesWith(ConstantInt::getSigned(
                  binOp->getType(),
                  constLhs->getSExtValue() + constRhs->getSExtValue()));
                break;
              case Instruction::Sub: 
                binOp->replaceAllUsesWith(ConstantInt::getSigned(
                  binOp->getType(),
                  constLhs->getSExtValue() - constRhs->getSExtValue()));
                break;
              case Instruction::Mul: 
                binOp->replaceAllUsesWith(ConstantInt::getSigned(
                  binOp->getType(),
                  constLhs->getSExtValue() * constRhs->getSExtValue()));
                break;
              case Instruction::UDiv:
              case Instruction::SDiv: 
                binOp->replaceAllUsesWith(ConstantInt::getSigned(
                  binOp->getType(),
                  constLhs->getSExtValue() / constRhs->getSExtValue()));
                break;
              default:
                break;
            }
          }else if(constLhs){
            // 若左操作数为整数常量，检查算术恒等式和常量表达式合并
            switch (binOp->getOpcode()) {
              case Instruction::Add:
                if(constLhs->getSExtValue() == 0)
                  binOp->replaceAllUsesWith(rhs);
                else
                  push_up_add(binOp, rhs, false, constLhs->getSExtValue());
                break;
              case Instruction::Sub:
                push_up_add(binOp, rhs, true, constLhs->getSExtValue());
                break;
              case Instruction::Mul:
                if(constLhs->getSExtValue() == 0)
                  binOp->replaceAllUsesWith(ConstantInt::getSigned(
                    binOp->getType(), 0));
                if(constLhs->getSExtValue() == 1)
                  binOp->replaceAllUsesWith(rhs);
                break;
              case Instruction::UDiv:
              case Instruction::SDiv:
                if(constLhs->getSExtValue() == 0)
                  binOp->replaceAllUsesWith(ConstantInt::getSigned(
                    binOp->getType(), 0));
                break;
              default:
                break;
            }
          }else if(constRhs){
            // 若右操作数为整数常量，检查算术恒等式和常量表达式合并
            switch (binOp->getOpcode()) {
              case Instruction::Add:
                if(constRhs->getSExtValue() == 0)
                  binOp->replaceAllUsesWith(lhs);
                else
                  push_up_add(binOp, lhs, false, constRhs->getSExtValue());
                break;
              case Instruction::Sub: 
                if(constRhs->getSExtValue() == 0)
                  binOp->replaceAllUsesWith(lhs);
                else
                  push_up_add(binOp, lhs, false, -constRhs->getSExtValue());
                break;
              case Instruction::Mul:
                if(constRhs->getSExtValue() == 0)
                  binOp->replaceAllUsesWith(ConstantInt::getSigned(
                    binOp->getType(), 0));
                if(constRhs->getSExtValue() == 1)
                  binOp->replaceAllUsesWith(lhs);
                break;
              case Instruction::UDiv:
              case Instruction::SDiv:
                if(constRhs->getSExtValue() == 1)
                  binOp->replaceAllUsesWith(lhs);
                break;
              case Instruction::URem:
              case Instruction::SRem:
                if(constRhs->getSExtValue() == 1)
                  binOp->replaceAllUsesWith(ConstantInt::getSigned(
                    binOp->getType(), 0));
                break;
              default:
                break;
            }
          }
        }
      }
    }
  }

  mOut << "ConstantFolding running...\n";
  return PreservedAnalyses::all();
}

void ConstantFolding::push_up_add(llvm::Value* target, llvm::Value* var, bool neg, int64_t val)
{
  for(auto u: target->users())
    if(auto binOp = dyn_cast<BinaryOperator>(u)){
      Value* lhs = binOp->getOperand(0);
      Value* rhs = binOp->getOperand(1);
      auto constLhs = dyn_cast<ConstantInt>(lhs);
      auto constRhs = dyn_cast<ConstantInt>(rhs);
      if(constLhs && rhs == target){
        switch (binOp->getOpcode()) {
          BinaryOperator* mergedInst;
          case Instruction::Add:
            if(neg)
              mergedInst = BinaryOperator::Create(Instruction::Sub,
                ConstantInt::getSigned(binOp->getType(),
                  constLhs->getSExtValue() + val),
                var);
            else
              mergedInst = BinaryOperator::Create(Instruction::Add,
                ConstantInt::getSigned(binOp->getType(),
                  constLhs->getSExtValue() + val),
                var);
            mergedInst->insertAfter(binOp);
            binOp->replaceAllUsesWith(mergedInst);
            break;
          case Instruction::Sub:
            if(neg)
              mergedInst = BinaryOperator::Create(Instruction::Add,
                ConstantInt::getSigned(binOp->getType(),
                  constLhs->getSExtValue() - val),
                var);
            else
              mergedInst = BinaryOperator::Create(Instruction::Sub,
                ConstantInt::getSigned(binOp->getType(),
                  constLhs->getSExtValue() - val),
                var);
            mergedInst->insertAfter(binOp);
            binOp->replaceAllUsesWith(mergedInst);
            break;
          default:
            break;
        }
      }
      if(constRhs && lhs == target){
        switch (binOp->getOpcode()) {
          BinaryOperator* mergedInst;
          case Instruction::Add:
            if(neg)
              mergedInst = BinaryOperator::Create(Instruction::Sub,
                ConstantInt::getSigned(binOp->getType(),
                  val + constRhs->getSExtValue()),
                var);
            else
              mergedInst = BinaryOperator::Create(Instruction::Add,
                ConstantInt::getSigned(binOp->getType(),
                  val + constRhs->getSExtValue()),
                var);
            mergedInst->insertAfter(binOp);
            binOp->replaceAllUsesWith(mergedInst);
            break;
          case Instruction::Sub:
            if(neg)
              mergedInst = BinaryOperator::Create(Instruction::Sub,
                ConstantInt::getSigned(binOp->getType(),
                  val - constRhs->getSExtValue()),
                var);
            else
              mergedInst = BinaryOperator::Create(Instruction::Add,
                ConstantInt::getSigned(binOp->getType(),
                  val - constRhs->getSExtValue()),
                var);
            mergedInst->insertAfter(binOp);
            binOp->replaceAllUsesWith(mergedInst);
            break;
          default:
            break;
        }
      }
    }
}