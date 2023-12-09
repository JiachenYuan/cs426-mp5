#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"


#include "UnitLoopInfo.h"

using namespace llvm;
using namespace cs426;

/// Main function for running the Loop Identification analysis. This function
/// returns information about the loops in the function via the UnitLoopInfo
/// object
UnitLoopInfo UnitLoopAnalysis::run(Function &F, FunctionAnalysisManager &FAM) {
  dbgs() << "UnitLoopAnalysis running on " << F.getName() << "\n";
  // Acquires the Dominator Tree constructed by LLVM for this function. You may
  // find this useful in identifying the natural loops
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  
  UnitLoopInfo Loops;
  // Fill in appropriate information

  // Traverse the CFG to find edges (src, dst) such that the source BB src is dominated by the destination BB dst  
  for (DomTreeNode* Node: post_order(DT.getRootNode())) {
    BasicBlock *BB = Node->getBlock();
    std::vector<BasicBlock*> BackEdges;
    for (BasicBlock* Pred: predecessors(BB)) if (DT.dominates(BB, Pred)) BackEdges.push_back(Pred);
    if (BackEdges.size() > 0) {
      // TODO: Add the loop (contains dst and all nodes that can reach src from dst) to the list

    }
  }
  return Loops;
}
