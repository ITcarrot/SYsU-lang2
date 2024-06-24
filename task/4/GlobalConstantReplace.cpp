
#include "GlobalConstantReplace.hpp"
#include "llvm/IR/Instructions.h"

using namespace llvm;

PreservedAnalyses
GlobalConstantReplace::run(Module& mod, ModuleAnalysisManager& mam)
{
    std::vector<Instruction*> instToErase;
    std::vector<GlobalVariable*> varToErase;

    for (auto &gv : mod.globals()) {
        if (!gv.hasInitializer())
            continue;
        auto val = gv.getInitializer();
        bool notLoad = false;

        for (auto *u : gv.users()) 
            if (auto *li = dyn_cast<LoadInst>(u));
            else{
                notLoad = true;
                break;
            }

        if (!notLoad){
            for (auto *u : gv.users()){
                u->replaceAllUsesWith(val);
                instToErase.push_back(dyn_cast<Instruction>(u));
            }
            varToErase.push_back(&gv);
        }
    }
    for(auto i: instToErase)
        i->removeFromParent();
    for(auto v: varToErase)
        v->removeFromParent();

    mOut << "GlobalConstantReplace running...\nTo eliminate " << instToErase.size()
         << " instructions\n";
    return PreservedAnalyses::all();
}
