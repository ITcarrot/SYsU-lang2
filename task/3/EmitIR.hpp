#include "asg.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

class EmitIR
{
public:
  Obj::Mgr& mMgr;
  llvm::Module mMod;

  EmitIR(Obj::Mgr& mgr, llvm::LLVMContext& ctx, llvm::StringRef mid = "-");

  llvm::Module& operator()(asg::TranslationUnit* tu);

private:
  llvm::LLVMContext& mCtx;

  llvm::Type* mIntTy;
  llvm::FunctionType* mCtorTy;

  llvm::Function* mCurFunc;
  std::vector<llvm::CallInst*> mCurFuncSp;
  std::unique_ptr<llvm::IRBuilder<>> mCurIrb;

  std::vector<llvm::BasicBlock*> mCurLoopCond;
  std::vector<llvm::BasicBlock*> mCurLoopEnd;

  //============================================================================
  // 类型
  //============================================================================

  llvm::Type* operator()(const asg::Type* type);

  llvm::Value* bool_cast(llvm::Value* var);

  //============================================================================
  // 表达式
  //============================================================================

  llvm::Value* operator()(asg::Expr* obj);

  llvm::Constant* operator()(asg::IntegerLiteral* obj);

  // TODO: 添加表达式处理相关声明
  llvm::Value* operator()(asg::BinaryExpr* obj);
  
  llvm::Value* operator()(asg::ImplicitCastExpr* obj);

  llvm::Value* operator()(asg::DeclRefExpr* obj);

  llvm::Value* operator()(asg::UnaryExpr* obj);

  llvm::Value* operator()(asg::ParenExpr* obj);

  llvm::Value* operator()(asg::CallExpr* obj);

  //============================================================================
  // 语句
  //============================================================================

  void operator()(asg::Stmt* obj);

  void operator()(asg::CompoundStmt* obj);

  void operator()(asg::ReturnStmt* obj);

  // TODO: 添加语句处理相关声明
  void operator()(asg::DeclStmt* obj);

  void operator()(asg::ExprStmt* obj);

  void operator()(asg::IfStmt* obj);

  void operator()(asg::WhileStmt* obj);

  void operator()(asg::BreakStmt* obj);

  void operator()(asg::ContinueStmt* obj);

  //============================================================================
  // 声明
  //============================================================================

  void operator()(asg::Decl* obj);

  void operator()(asg::FunctionDecl* obj);

  // TODO: 添加声明处理相关声明
  void var_init(llvm::Value* var, asg::Expr* expr, const asg::Type* type, int &index);

  void operator()(asg::VarDecl* obj);
};
