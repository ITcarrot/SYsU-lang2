#include "EmitIR.hpp"
#include <llvm/Transforms/Utils/ModuleUtils.h>

#define self (*this)

using namespace asg;

EmitIR::EmitIR(Obj::Mgr& mgr, llvm::LLVMContext& ctx, llvm::StringRef mid)
  : mMgr(mgr)
  , mMod(mid, ctx)
  , mCtx(ctx)
  , mIntTy(llvm::Type::getInt32Ty(ctx))
  , mCurIrb(std::make_unique<llvm::IRBuilder<>>(ctx))
  , mCtorTy(llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), false))
{
}

llvm::Module&
EmitIR::operator()(asg::TranslationUnit* tu)
{
  for (auto&& i : tu->decls)
    self(i);
  return mMod;
}

//==============================================================================
// 类型
//==============================================================================

llvm::Type*
EmitIR::operator()(const Type* type)
{
  if (type->texp == nullptr) {
    switch (type->spec) {
      case Type::Spec::kInt:
        return llvm::Type::getInt32Ty(mCtx);
      // TODO: 在此添加对更多基础类型的处理
      case Type::Spec::kVoid:
        return llvm::Type::getVoidTy(mCtx);
      default:
        ABORT();
    }
  }

  Type subt;
  subt.spec = type->spec;
  subt.qual = type->qual;
  subt.texp = type->texp->sub;

  // TODO: 在此添加对指针类型、数组类型和函数类型的处理
  if (auto p = type->texp->dcst<ArrayType>())
    return llvm::ArrayType::get(self(&subt), p->len);

  if (auto p = type->texp->dcst<PointerType>())
    return llvm::PointerType::get(self(&subt), 0);

  if (auto p = type->texp->dcst<FunctionType>()) {
    std::vector<llvm::Type*> pty;
    // TODO: 在此添加对函数参数类型的处理
    for(auto param: p->params)
      pty.push_back(self(param));
    return llvm::FunctionType::get(self(&subt), std::move(pty), false);
  }

  ABORT();
}

llvm::Value*
EmitIR::bool_cast(llvm::Value* var)
{
  auto ty = var->getType();
  if(ty->isIntegerTy())
    return mCurIrb->CreateICmpNE(var, llvm::Constant::getNullValue(ty));

  ABORT();
}

//==============================================================================
// 表达式
//==============================================================================

llvm::Value*
EmitIR::operator()(Expr* obj)
{
  // TODO: 在此添加对更多表达式处理的跳转
  if (auto p = obj->dcst<IntegerLiteral>())
    return self(p);

  if (auto p = obj->dcst<DeclRefExpr>())
    return self(p);

  if (auto p = obj->dcst<ImplicitCastExpr>())
    return self(p);

  if (auto p = obj->dcst<BinaryExpr>())
    return self(p);

  if (auto p = obj->dcst<UnaryExpr>())
    return self(p);

  if (auto p = obj->dcst<ParenExpr>())
    return self(p);

  if (auto p = obj->dcst<CallExpr>())
    return self(p);

  ABORT();
}

llvm::Constant*
EmitIR::operator()(IntegerLiteral* obj)
{
  return llvm::ConstantInt::get(self(obj->type), obj->val);
}

// TODO: 在此添加对更多表达式类型的处理
llvm::Value*
EmitIR::operator()(DeclRefExpr* obj)
{
  return reinterpret_cast<llvm::Value*>(obj->decl->any);
}

llvm::Value*
EmitIR::operator()(ImplicitCastExpr* obj)
{
  auto sub = self(obj->sub);

  auto& irb = *mCurIrb;
  switch (obj->kind) {
    case ImplicitCastExpr::kLValueToRValue:
      return irb.CreateLoad(self(obj->sub->type), sub);

    case ImplicitCastExpr::kArrayToPointerDecay:
      obj->type = obj->sub->type;
      return sub;

    case ImplicitCastExpr::kFunctionToPointerDecay:
      return sub;

    default:
      ABORT();
  }
}

llvm::Value*
EmitIR::operator()(BinaryExpr* obj)
{
  llvm::Value *lftVal, *rhtVal;
  auto& irb = *mCurIrb;

  lftVal = self(obj->lft);
  if(obj->op == BinaryExpr::kAnd){
    lftVal = bool_cast(lftVal);
    auto lftBlock = mCurIrb->GetInsertBlock();

    auto trueBlock = llvm::BasicBlock::Create(mCtx, "and.true", mCurFunc);
    auto endBlock = llvm::BasicBlock::Create(mCtx, "and.end", mCurFunc);
    
    mCurIrb->CreateCondBr(lftVal, trueBlock, endBlock);
    
    mCurIrb->SetInsertPoint(trueBlock);
    rhtVal = bool_cast(self(obj->rht));
    auto rhtBlock = mCurIrb->GetInsertBlock();
    mCurIrb->CreateBr(endBlock);

    mCurIrb->SetInsertPoint(endBlock);
    auto phi = mCurIrb->CreatePHI(mCurIrb->getInt1Ty(), 2);
    phi->addIncoming(lftVal, lftBlock);
    phi->addIncoming(rhtVal, rhtBlock);
    return phi;
  }
  if(obj->op == BinaryExpr::kOr){
    lftVal = bool_cast(lftVal);
    auto lftBlock = mCurIrb->GetInsertBlock();

    auto falseBlock = llvm::BasicBlock::Create(mCtx, "or.false", mCurFunc);
    auto endBlock = llvm::BasicBlock::Create(mCtx, "or.end", mCurFunc);
    
    mCurIrb->CreateCondBr(lftVal, endBlock, falseBlock);
    
    mCurIrb->SetInsertPoint(falseBlock);
    rhtVal = bool_cast(self(obj->rht));
    auto rhtBlock = mCurIrb->GetInsertBlock();
    mCurIrb->CreateBr(endBlock);

    mCurIrb->SetInsertPoint(endBlock);
    auto phi = mCurIrb->CreatePHI(mCurIrb->getInt1Ty(), 2);
    phi->addIncoming(lftVal, lftBlock);
    phi->addIncoming(rhtVal, rhtBlock);
    return phi;
  }

  rhtVal = self(obj->rht);
  switch (obj->op) {
    case BinaryExpr::kAdd:
      return irb.CreateAdd(lftVal, rhtVal);

    case BinaryExpr::kMul:
      return irb.CreateMul(lftVal, rhtVal);

    case BinaryExpr::kDiv:
      return irb.CreateSDiv(lftVal, rhtVal);

    case BinaryExpr::kMod:
      return irb.CreateSRem(lftVal, rhtVal);

    case BinaryExpr::kSub:
      return irb.CreateSub(lftVal, rhtVal);

    case BinaryExpr::kGt:
      return irb.CreateICmpSGT(lftVal, rhtVal);

    case BinaryExpr::kLt:
      return irb.CreateICmpSLT(lftVal, rhtVal);

    case BinaryExpr::kGe:
      return irb.CreateICmpSGE(lftVal, rhtVal);

    case BinaryExpr::kLe:
      return irb.CreateICmpSLE(lftVal, rhtVal);

    case BinaryExpr::kEq:
      return irb.CreateICmpEQ(lftVal, rhtVal);

    case BinaryExpr::kNe:
      return irb.CreateICmpNE(lftVal, rhtVal);

    case BinaryExpr::kAssign:
      return irb.CreateStore(rhtVal, lftVal);

    case BinaryExpr::kComma:
      return rhtVal;

    case BinaryExpr::kIndex:{
      Type elementType;
      elementType.spec = obj->lft->type->spec;
      elementType.qual = obj->lft->type->qual;
      elementType.texp = obj->lft->type->texp->sub;
      return irb.CreateGEP(self(&elementType), lftVal, {rhtVal});
    }

    default:
      ABORT();
  }
}

llvm::Value*
EmitIR::operator()(UnaryExpr* obj)
{
  llvm::Value *val;

  val = self(obj->sub);

  auto& irb = *mCurIrb;
  switch (obj->op) {
    case UnaryExpr::kPos:
      return val;
    
    case UnaryExpr::kNeg:
      return irb.CreateNeg(val);

    case UnaryExpr::kNot:
      return irb.CreateICmpEQ(val, llvm::Constant::getNullValue(val->getType()));

    default:
      ABORT();
  }
}

llvm::Value*
EmitIR::operator()(ParenExpr* obj)
{
  return self(obj->sub);
}

llvm::Value*
EmitIR::operator()(CallExpr* obj)
{
  auto callee = llvm::dyn_cast<llvm::Function>(self(obj->head));
  std::vector<llvm::Value*> args;
  for(auto i: obj->args)
    args.push_back(self(i));
  return mCurIrb->CreateCall(callee, args);
}

//==============================================================================
// 语句
//==============================================================================

void
EmitIR::operator()(Stmt* obj)
{
  // TODO: 在此添加对更多Stmt类型的处理的跳转

  if (auto p = obj->dcst<CompoundStmt>())
    return self(p);

  if (auto p = obj->dcst<ReturnStmt>())
    return self(p);

  if (auto p = obj->dcst<DeclStmt>())
    return self(p);

  if (auto p = obj->dcst<ExprStmt>())
    return self(p);

  if (auto p = obj->dcst<IfStmt>())
    return self(p);

  if (auto p = obj->dcst<WhileStmt>())
    return self(p);

  if (auto p = obj->dcst<BreakStmt>())
    return self(p);

  if (auto p = obj->dcst<ContinueStmt>())
    return self(p);

  if (auto p = obj->dcst<NullStmt>())
    return;

  ABORT();
}

// TODO: 在此添加对更多Stmt类型的处理
void
EmitIR::operator()(CompoundStmt* obj)
{
  // TODO: 可以在此添加对符号重名的处理
  auto sp = mCurIrb->CreateIntrinsic(llvm::Intrinsic::stacksave,
    {}, {}, nullptr, "sp");
  for (auto&& stmt : obj->subs)
    self(stmt);
  mCurIrb->CreateIntrinsic(llvm::Intrinsic::stackrestore, {}, {sp});
}

void
EmitIR::operator()(ReturnStmt* obj)
{
  auto& irb = *mCurIrb;

  llvm::Value* retVal;
  if (!obj->expr)
    retVal = nullptr;
  else
    retVal = self(obj->expr);

  mCurIrb->CreateIntrinsic(llvm::Intrinsic::stackrestore, {}, {mCurFuncSp.back()});
  mCurIrb->CreateRet(retVal);

  auto exitBb = llvm::BasicBlock::Create(mCtx, "return_exit", mCurFunc);
  mCurIrb->SetInsertPoint(exitBb);
}

void
EmitIR::operator()(DeclStmt* obj)
{
  for (auto&& decls : obj->decls)
    self(decls);
}

void
EmitIR::operator()(ExprStmt* obj)
{
  self(obj->expr);
}

void
EmitIR::operator()(IfStmt* obj)
{
  auto thenBlock = llvm::BasicBlock::Create(mCtx, "if.then", mCurFunc);
  auto elseBlock = llvm::BasicBlock::Create(mCtx, "if.else", mCurFunc);
  auto endBlock = llvm::BasicBlock::Create(mCtx, "if.end", mCurFunc);
  
  mCurIrb->CreateCondBr(bool_cast(self(obj->cond)), thenBlock, elseBlock);
  
  mCurIrb->SetInsertPoint(thenBlock);
  self(obj->then);
  mCurIrb->CreateBr(endBlock);

  mCurIrb->SetInsertPoint(elseBlock);
  if(obj->else_)
    self(obj->else_);
  mCurIrb->CreateBr(endBlock);

  mCurIrb->SetInsertPoint(endBlock);
}

void
EmitIR::operator()(WhileStmt* obj)
{
  auto condBlock = llvm::BasicBlock::Create(mCtx, "while.cond", mCurFunc);
  auto bodyBlock = llvm::BasicBlock::Create(mCtx, "while.body", mCurFunc);
  auto endBlock = llvm::BasicBlock::Create(mCtx, "while.end", mCurFunc);

  mCurIrb->CreateBr(condBlock);
  mCurIrb->SetInsertPoint(condBlock);
  mCurIrb->CreateCondBr(bool_cast(self(obj->cond)), bodyBlock, endBlock);

  mCurIrb->SetInsertPoint(bodyBlock);
  mCurLoopCond.push_back(condBlock);
  mCurLoopEnd.push_back(endBlock);
  self(obj->body);
  mCurIrb->CreateBr(condBlock);
  mCurLoopCond.pop_back();
  mCurLoopEnd.pop_back();

  mCurIrb->SetInsertPoint(endBlock);
}

void
EmitIR::operator()(BreakStmt* obj)
{
  mCurIrb->CreateBr(mCurLoopEnd.back());
  auto exitBb = llvm::BasicBlock::Create(mCtx, "break_exit", mCurFunc);
  mCurIrb->SetInsertPoint(exitBb);
}

void
EmitIR::operator()(ContinueStmt* obj)
{
  mCurIrb->CreateBr(mCurLoopCond.back());
  auto exitBb = llvm::BasicBlock::Create(mCtx, "continue_exit", mCurFunc);
  mCurIrb->SetInsertPoint(exitBb);
}

//==============================================================================
// 声明
//==============================================================================

void
EmitIR::operator()(Decl* obj)
{
  // TODO: 添加变量声明处理的跳转
  if (auto p = obj->dcst<VarDecl>())
    return self(p);

  if (auto p = obj->dcst<FunctionDecl>())
    return self(p);

  ABORT();
}

// TODO: 添加变量声明的处理
void
EmitIR::var_init(llvm::Value* var, Expr* expr, const Type* type, int &index)
{
  auto& irb = *mCurIrb;

  if(auto p = expr->dcst<ImplicitInitExpr>()){
    llvm::MaybeAlign align;
    if(auto v = llvm::dyn_cast<llvm::AllocaInst>(var))
      align = v->getAlign();
    if(auto v = llvm::dyn_cast<llvm::GlobalVariable>(var))
      align = v->getAlign();
    uint64_t arraySize = mMod.getDataLayout().getTypeAllocSize(var->getType());

    irb.CreateMemSet(var, irb.getInt8(0), irb.getInt64(32), align);
  }else if(auto p = expr->dcst<InitListExpr>())
    for(auto exprI: p->list)
      var_init(var, exprI, type, index);
  else{
    Type baseType;
    baseType.spec = type->spec;
    baseType.qual = type->qual;
    baseType.texp = nullptr;
    if(index)
      var = irb.CreateGEP(self(&baseType), var, {irb.getInt32(index)});
    irb.CreateStore(self(expr), var);
    index++;
  }
}

void
EmitIR::operator()(VarDecl* obj)
{
  auto ty = self(obj->type);

  if(mCurFuncSp.empty()){
    auto gvar = new llvm::GlobalVariable(
      mMod, ty, false, llvm::GlobalVariable::ExternalLinkage,
      llvm::ConstantAggregateZero::get(ty), obj->name);
    obj->any = gvar;

    if (obj->init == nullptr)
      return;

    // 创建构造函数用于初始化
    mCurFunc = llvm::Function::Create(
      mCtorTy, llvm::GlobalVariable::PrivateLinkage, "ctor_" + obj->name, mMod);
    llvm::appendToGlobalCtors(mMod, mCurFunc, 65535);

    auto entryBb = llvm::BasicBlock::Create(mCtx, "entry", mCurFunc);
    mCurIrb->SetInsertPoint(entryBb);
    int index = 0;
    var_init(gvar, obj->init, obj->type, index);
    mCurIrb->CreateRet(nullptr);
  }else{
    auto var = mCurIrb->CreateAlloca(ty, nullptr, obj->name);
    obj->any = var;

    if(obj->init){
      int index = 0;
      var_init(var, obj->init, obj->type, index);
    }
  }
}

void
EmitIR::operator()(FunctionDecl* obj)
{
  // 创建函数
  auto fty = llvm::dyn_cast<llvm::FunctionType>(self(obj->type));
  auto func = llvm::Function::Create(
    fty, llvm::GlobalVariable::ExternalLinkage, obj->name, mMod);
  obj->any = func;

  if (obj->body == nullptr)
    return;
  auto entryBb = llvm::BasicBlock::Create(mCtx, "entry", func);
  mCurIrb->SetInsertPoint(entryBb);
  auto& entryIrb = *mCurIrb;
  auto sp = mCurIrb->CreateIntrinsic(llvm::Intrinsic::stacksave,
    {}, {}, nullptr, "sp");
  mCurFuncSp.push_back(sp);

  // TODO: 添加对函数参数的处理
  auto argIter = func->arg_begin();
  for(auto p: obj->params){
    p->any = mCurIrb->CreateAlloca(argIter->getType());
    mCurIrb->CreateStore(argIter, (llvm::Value*)p->any);
    argIter++;
  }

  // 翻译函数体
  mCurFunc = func;
  self(obj->body);
  mCurFuncSp.pop_back();
  auto& exitIrb = *mCurIrb;

  if (fty->getReturnType()->isVoidTy())
    exitIrb.CreateRetVoid();
  else
    exitIrb.CreateUnreachable();
}
