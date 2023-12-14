// Usage: opt -load-pass-plugin=libUnitProject.so -passes="unit-licm"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/ADT/Statistic.h"

#include "UnitLICM.h"
#include "UnitLoopInfo.h"


using namespace llvm;
using namespace cs426;
#define DEBUG_TYPE "UnitLICM"
// Define any statistics here
STATISTIC(NumStoreHoisted, "Number of store instructions hoisted");
STATISTIC(NumLoadHoisted, "Number of load instructions hoisted");
STATISTIC(NumComputeHoisted, "Number of computes hoisted");



/// Main function for running the LICM optimization
PreservedAnalyses UnitLICM::run(Function& F, FunctionAnalysisManager& FAM) {
  dbgs() << "UnitLICM running on " << F.getName() << "\n";
  // Acquires the UnitLoopInfo object constructed by your Loop Identification
  // (LoopAnalysis) pass
  UnitLoopInfo &Loops = FAM.getResult<UnitLoopAnalysis>(F);
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  AAResults &AA = FAM.getResult<AAManager>(F);


  // Perform the optimization
  

  // Set proper preserved analyses
  return PreservedAnalyses::all();
}
