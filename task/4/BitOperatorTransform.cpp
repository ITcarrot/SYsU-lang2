#include "BitOperatorTransform.hpp"
#include "llvm/IR/Operator.h"

using namespace llvm;

PreservedAnalyses
BitOperatorTransform::run(Module& mod, ModuleAnalysisManager& mam)
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
          if(constLhs && binOp->getOpcode() == Instruction::Mul){
            // 若左操作数为整数常量，检查乘法
            auto val = constLhs->getSExtValue();
            if(val > 0 && (val & -val) == val){
              int shiftBits = 0;
              for(; val > 1; val >>= 1, shiftBits++);
              auto inst = BinaryOperator::Create(Instruction::Shl, rhs,
                ConstantInt::getSigned(binOp->getType(), shiftBits));
              inst->insertAfter(binOp);
              binOp->replaceAllUsesWith(inst);
            }
          }
          if(constRhs){
            // 若右操作数为整数常量，检查乘、除、模
            auto val = constRhs->getSExtValue();
            if(val > 0 && (val & -val) == val){
              BinaryOperator *inst;
              int shiftBits = 0;
              switch (binOp->getOpcode()) {
                case Instruction::Mul:
                  for(; val > 1; val >>= 1, shiftBits++);
                  inst = BinaryOperator::Create(Instruction::Shl, lhs,
                    ConstantInt::getSigned(binOp->getType(), shiftBits));
                  inst->insertAfter(binOp);
                  binOp->replaceAllUsesWith(inst);
                  break;
                case Instruction::UDiv:
                  for(; val > 1; val >>= 1, shiftBits++);
                  inst = BinaryOperator::Create(Instruction::LShr, lhs,
                    ConstantInt::getSigned(binOp->getType(), shiftBits));
                  inst->insertAfter(binOp);
                  binOp->replaceAllUsesWith(inst);
                  break;
                case Instruction::SDiv:{
                  for(; val > 1; val >>= 1, shiftBits++);
                  auto sign = BinaryOperator::Create(Instruction::LShr, lhs,
                    ConstantInt::getSigned(lhs->getType(), lhs->getType()->getPrimitiveSizeInBits() - 1));
                  auto x1 = BinaryOperator::Create(Instruction::Sub, lhs, sign);
                  auto x1y = BinaryOperator::Create(Instruction::AShr, x1,
                    ConstantInt::getSigned(binOp->getType(), shiftBits));
                  auto x1y1 = BinaryOperator::Create(Instruction::Add, x1y, sign);
                  sign->insertBefore(binOp);
                  x1->insertBefore(binOp);
                  x1y->insertBefore(binOp);
                  x1y1->insertBefore(binOp);
                  binOp->replaceAllUsesWith(x1y1);
                  break;
                }
                case Instruction::URem:
                  inst = BinaryOperator::Create(Instruction::And, lhs,
                    ConstantInt::getSigned(binOp->getType(), val - 1));
                  inst->insertAfter(binOp);
                  binOp->replaceAllUsesWith(inst);
                  break;
                case Instruction::SRem:{
                  auto sign = BinaryOperator::Create(Instruction::AShr, lhs,
                    ConstantInt::getSigned(lhs->getType(), lhs->getType()->getPrimitiveSizeInBits() - 1));
                  auto _x = BinaryOperator::Create(Instruction::Xor, lhs, sign);
                  auto _x1 = BinaryOperator::Create(Instruction::Sub, _x,sign);
                  auto _x1y = BinaryOperator::Create(Instruction::And, _x1,
                    ConstantInt::getSigned(binOp->getType(), val - 1));
                  auto __x1y = BinaryOperator::Create(Instruction::Xor, _x1y, sign);
                  auto __x1y1 = BinaryOperator::Create(Instruction::Sub, __x1y,sign);
                  sign->insertBefore(binOp);
                  _x->insertBefore(binOp);
                  _x1->insertBefore(binOp);
                  _x1y->insertBefore(binOp);
                  __x1y->insertBefore(binOp);
                  __x1y1->insertBefore(binOp);
                  binOp->replaceAllUsesWith(__x1y1);
                  break;
                }
                default:
                  break;
              }
            }
          }
        }
      }
    }
  }

  mOut << "BitOperatorTransform running...\n";
  return PreservedAnalyses::all();
}
