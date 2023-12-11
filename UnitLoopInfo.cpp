#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"


#include "UnitLoopInfo.h"

using namespace llvm;
using namespace cs426;

// Helper function to get all natural loop members corresponding to the back edge end->loop_header
void GetNaturalLoop(BasicBlock* loop_header, BasicBlock* end, DominatorTree& DT, std::vector<BasicBlock*>& natural_loop) {

  SmallVector<BasicBlock*> temp;
  DT.getDescendants(loop_header, temp);
  std::unordered_set<BasicBlock*> header_reacheable_blocks(temp.begin(), temp.end());
  header_reacheable_blocks.erase(loop_header);

  // BFS to identify blocks that can reach end without going thru loop_header
  std::unordered_set<BasicBlock*> can_reach_end;  
  std::queue<BasicBlock*> work_list;
  work_list.push(end);
  can_reach_end.insert(end);
  std::unordered_set<BasicBlock*> visited;

  while (work_list.size()) {
    BasicBlock* curr = work_list.front();
    work_list.pop();
    visited.insert(curr);

    for (BasicBlock* pred : predecessors(curr)) {
      if (!visited.count(curr) && header_reacheable_blocks.count(pred) && pred != loop_header) {
        can_reach_end.insert(pred);
        work_list.push(pred);
      }
    }
  }

  // BFS to adjust the order to the natural loop members
  visited.clear();
  work_list = std::queue<BasicBlock*>(); // Clear the work_list
  work_list.push(loop_header);

  while (work_list.size()) {
    BasicBlock* curr = work_list.front();
    work_list.pop();
    visited.insert(curr);
    natural_loop.push_back(curr);

    for (BasicBlock* child : successors(curr)) {
      if (!visited.count(curr) && can_reach_end.count(curr)) {
        work_list.push(child);
      }
    }
  }
}

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

  // Post-order traversal to identify loop headers
  for (DomTreeNode* node : post_order(DT.getRootNode())) {
    BasicBlock* BB = node->getBlock();
    std::vector<BasicBlock*> back_edges_srcs;
    for (BasicBlock* incoming_block : predecessors(BB)) {
      if (DT.dominates(BB, incoming_block)) {
        back_edges_srcs.push_back(incoming_block);
      }
    }
    // If current block dominates some blocks that point to itself, then it is a loop header
    if (back_edges_srcs.size()) {
      // Set metadata information about the loop for every loop header
      for (BasicBlock* back_src : back_edges_srcs) {
        if (!Loops.m_HeaderLoopMeta.count(BB)) {
          LoopMeta* loop_metadata = new LoopMeta();
        }
        LoopMeta* loop_metadata = Loops.m_HeaderLoopMeta[BB];
        // Get all loop members corresponding to this back edge
        std::vector<BasicBlock*> natural_loop_members;
        GetNaturalLoop(BB, back_src, DT, natural_loop_members);
        loop_metadata->m_LoopMemberBlocks[back_src] = natural_loop_members;

        if (!Loops.m_InnerMostLoopHeader.count(back_src)) {
          Loops.m_InnerMostLoopHeader[back_src] = BB;
        } else {
          // If this src already binds to a loop header, then modify the loopMeta of that loopheader to have this one as the parent
          BasicBlock* inner_loop_header = Loops.m_InnerMostLoopHeader[back_src];
          LoopMeta* inner_loop_meta = Loops.m_HeaderLoopMeta[inner_loop_header];
          inner_loop_meta->m_ParentLoopHeader = BB;
        }
      }
    }
  }




  return Loops;
}
