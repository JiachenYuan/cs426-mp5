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

// Helper method on deciding whether set B is a subset of set A
bool SetAContainsSetB(std::unordered_set<BasicBlock*> A, std::unordered_set<BasicBlock*> B) {
  if (A.size() < B.size()) 
    return false;
  for (BasicBlock* b : B) {
    if (!A.count(b)) {
      return false;
    }
  }
  return true;
}

// Helper 
// Assumes that the natural loop formed by end->outer_loop_header is already properly set in Loops
void SetupInnerLoops(BasicBlock* outer_loop_header, BasicBlock* end, UnitLoopInfo& Loops) {
  assert(Loops.m_HeaderLoopMeta.count(outer_loop_header));
  std::vector<BasicBlock*>& loop_member = Loops.m_HeaderLoopMeta[outer_loop_header]->m_LoopMemberBlocks[end];
  std::unordered_set<BasicBlock*> member_set(loop_member.begin(), loop_member.end());

  for (auto it = loop_member.rbegin(); it!=loop_member.rend(); it++) {
    BasicBlock* member = *it;
    // Only consider whether or not inner loop if this member is a loop header
    if (Loops.m_HeaderLoopMeta.count(member)) {
      LoopMeta* member_loop_header_meta = Loops.m_HeaderLoopMeta[member];
      for (auto& [member_back_src, member_loop_members] : member_loop_header_meta->m_LoopMemberBlocks) {
        // Only consider top-level loops when deciding whether or not inner
        if (member_loop_header_meta->m_ParentLoopHeader.count(member_back_src)) {
          continue;
        }
        std::unordered_set<BasicBlock*> member_loop_members_set(member_loop_members.begin(), member_loop_members.end());
        // If all members of the member loop are also members of the outer loop, then it is a innerloop
        if (SetAContainsSetB(member_set, member_loop_members_set)) {
          member_loop_header_meta->m_ParentLoopHeader[member_back_src] = outer_loop_header;
        }
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
      LoopMeta* loop_metadata = new LoopMeta(BB);
      Loops.m_HeaderLoopMeta[BB] = loop_metadata;
      // Set metadata information about the loop for every loop header
      for (BasicBlock* back_src : back_edges_srcs) {
        // Get all loop members corresponding to this back edge
        std::vector<BasicBlock*> natural_loop_members;
        GetNaturalLoop(BB, back_src, DT, natural_loop_members);
        loop_metadata->m_LoopMemberBlocks[back_src] = natural_loop_members;

        // Identify inner loops and setup nested relationships
        SetupInnerLoops(BB, back_src, Loops);
      }
    }
  }




  return Loops;
}
