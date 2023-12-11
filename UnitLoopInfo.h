#ifndef INCLUDE_UNIT_LOOP_INFO_H
#define INCLUDE_UNIT_LOOP_INFO_H
#include "llvm/IR/PassManager.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>

using namespace llvm;

namespace cs426 {
/// An object holding information about the (natural) loops in an LLVM
/// function. At minimum this will need to identify the loops, may hold
/// additional information you find useful for your LICM pass
struct LoopMeta;
class UnitLoopInfo {

  // Define this class to provide the information you need in LICM
public:
  // Mapping from loop headers to their relevant loop information
  std::unordered_map<BasicBlock*, LoopMeta*> m_HeaderLoopMeta;

  // Innermost loop header (if there is one) for basic blocks
  std::unordered_map<BasicBlock*, BasicBlock*> m_InnerMostLoopHeader;
};

// An object holding the metadata of a natural loop, only attached to loop headers
struct LoopMeta {
  LoopMeta(BasicBlock* loop_header) : m_LoopHeader(loop_header) {}

  // Current Loop Header where this LoopMeta is attached to
  BasicBlock* m_LoopHeader;

  // The outer layer loop header, identified by sources of back edges
  std::unordered_map<BasicBlock*, BasicBlock*> m_ParentLoopHeader;

  // Loop members identified by different back edge source blocks
  std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> m_LoopMemberBlocks;
  
};

/// Loop Identification Analysis Pass. Produces a UnitLoopInfo object which
/// should contain any information about the loops in the function which is
/// needed for your implementation of LICM
class UnitLoopAnalysis : public AnalysisInfoMixin<UnitLoopAnalysis> {
  friend AnalysisInfoMixin<UnitLoopAnalysis>;
  static AnalysisKey Key;

public:
  typedef UnitLoopInfo Result;

  UnitLoopInfo run(Function &F, FunctionAnalysisManager &AM);
};
} // namespace
#endif // INCLUDE_UNIT_LOOP_INFO_H
