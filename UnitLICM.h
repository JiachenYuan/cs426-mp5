#ifndef INCLUDE_UNIT_LICM_H
#define INCLUDE_UNIT_LICM_H
#include "llvm/IR/PassManager.h"

using namespace llvm;

namespace cs426 {

struct LoopRange {
  BasicBlock* loop_header;
  BasicBlock* back_edge_src;
  LoopRange(BasicBlock* _loop_header, BasicBlock* _back_edge_src) : loop_header(_loop_header), back_edge_src(_back_edge_src) {}
};

/// Loop Invariant Code Motion Optimization Pass
struct UnitLICM : PassInfoMixin<UnitLICM> {
  PreservedAnalyses run(Function& F, FunctionAnalysisManager& FAM);
};
} // namespace

#endif // INCLUDE_UNIT_LICM_H
