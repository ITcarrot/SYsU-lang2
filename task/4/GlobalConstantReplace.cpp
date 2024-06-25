
#include "GlobalConstantReplace.hpp"
#include "llvm/IR/Instructions.h"

using namespace llvm;

PreservedAnalyses
GlobalConstantReplace::run(Module& mod, ModuleAnalysisManager& mam)
{
    int instEraseCount = 0;

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

        if (!notLoad)
            for (auto *u : gv.users()){
                u->replaceAllUsesWith(val);
                instEraseCount++;
            }
    }

    mOut << "GlobalConstantReplace running...\nWill eliminate " << instEraseCount
         << " instructions\n";
    return PreservedAnalyses::all();
}
