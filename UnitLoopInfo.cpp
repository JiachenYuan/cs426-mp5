#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"


#include "UnitLoopInfo.h"

using namespace llvm;
using namespace cs426;
AnalysisKey UnitLoopAnalysis::Key;


/// Main function for running the Loop Identification analysis. This function
/// returns information about the loops in the function via the UnitLoopInfo
/// object

// check whether exists a path from curr->one latch inside latches without getting through header
// simple BFS
bool is_inside_loop(BasicBlock* header, DenseSet<BasicBlock*>& latches, BasicBlock* curr) {
  // trivial case that the curr node itself is header or latch
  if (curr == header) return true;
  if (latches.find(curr) != latches.end()) return true; // itself is the latch

  // precolor for the basic block we are checking for
  DenseSet<BasicBlock*> color;
  color.insert(curr);
  std::queue<BasicBlock*> q;
  q.push(curr);

  while (!q.empty()) {
    auto current_block = q.front();
    q.pop();

    for (auto iter = succ_begin(current_block); iter != succ_end(current_block); ++ iter) {
      if (*iter != header) {
        if (color.find(*iter) == color.end()) {
          color.insert(*iter);
          q.push(*iter);
          if (latches.find(*iter) != latches.end()) { // fins inside the latches
            return true;
          }
        }
      }
    }
  }

  return false;
}

UnitLoopInfo UnitLoopAnalysis::run(Function &F, FunctionAnalysisManager &FAM) {
  dbgs() << "UnitLoopAnalysis running on " << F.getName() << "\n";
  // Acquires the Dominator Tree constructed by LLVM for this function. You may
  // find this useful in identifying the natural loops
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  LoopInfo& LLVM_LOOPINFO = FAM.getResult<LoopAnalysis>(F);
  
  
  UnitLoopInfo Loops;
  // Fill in appropriate information

  // Traverse the CFG to find edges (src, dst) such that the source BB src is dominated by the destination BB dst  
  for (DomTreeNode* Node: post_order(DT.getRootNode())) {
    BasicBlock *BB = Node->getBlock();
    std::vector<BasicBlock*> BackEdges;
    for (BasicBlock* Pred: predecessors(BB)) if (DT.dominates(BB, Pred)) BackEdges.push_back(Pred);
    if (BackEdges.size() > 0) {
      // TODO: Add the loop (contains dst and all nodes that can reach src from dst) to the list
      // building the loop latch
      for (BasicBlock* Pred: predecessors(BB)) {
        Loops.header_to_latch[BB].insert(Pred);
      }
    }
  }

  // building the loop body
  for (auto header_iter: Loops.header_to_latch) {
    SmallVector<BasicBlock*, 5> all_dominate_children;
    DT.getDescendants(header_iter.first, all_dominate_children);
    for (llvm::BasicBlock* dominate_children_block: all_dominate_children) {
      if (is_inside_loop(header_iter.first, header_iter.second, dominate_children_block)) {
        Loops.header_to_body[header_iter.first].insert(dominate_children_block);
      }
    }
  }

  // setting up the exiting 
  for (auto header_iter: Loops.header_to_body) {
    for (auto body_block: header_iter.second) {
      for (auto body_block_succ_iter = succ_begin(body_block); body_block_succ_iter != succ_end(body_block); ++ body_block_succ_iter) {
        // if one of the loop body block contains a successor which does not belong to the loop body, can not find
        // existing edge: existing block(inside) -> exit block(outside)
        if (header_iter.second.find(*body_block_succ_iter) == header_iter.second.end()) { 
          Loops.header_to_exiting[header_iter.first].insert(body_block);
        }
      }
    }
  }

  // setting up the forest
  DenseSet<BasicBlock*> All_Header;
  for (auto header_iter: Loops.header_to_body) {
    All_Header.insert(header_iter.first);
  }
  for (auto header_iter: Loops.header_to_body) {
    for (auto body_block: header_iter.second) {
      if (All_Header.find(body_block) != All_Header.end()) {
        Loops.forest[header_iter.first].insert(body_block);
      }
    }
  }


  dbgs() << "-------------------------------------------\n"; 
  for (auto it: LLVM_LOOPINFO) {
  //  printLoop(*it, dbgs(), "");
  }
  dbgs() << "\n-------------------------------------------\n"; 

  return Loops;
}
