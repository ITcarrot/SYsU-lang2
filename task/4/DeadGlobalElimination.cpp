
#include "DeadGlobalElimination.hpp"
#include "llvm/IR/Instructions.h"
#include <unordered_set>

using namespace llvm;

PreservedAnalyses
DeadGlobalElimination::run(Module& mod, ModuleAnalysisManager& mam)
{
  std::unordered_set<Instruction*> instToErase;
  for (auto& gv : mod.globals()) {
    bool notStore = false;

    for (auto* u : gv.users())
      if (auto* si = dyn_cast<StoreInst>(u)) {
        if (si->getPointerOperand() != &gv) {
          notStore = true;
          break;
        }
      } else {
        notStore = true;
        break;
      }

    if (!notStore)
      for (auto* u : gv.users())
        instToErase.insert(dyn_cast<StoreInst>(u));
  }
  for(auto i: instToErase)
    i->eraseFromParent();

  mOut << "DeadGlobalElimination running...\nTo eliminate " << instToErase.size()
         << " instructions\n";
  return PreservedAnalyses::all();
}
