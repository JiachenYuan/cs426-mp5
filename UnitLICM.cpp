// Usage: opt -load-pass-plugin=libUnitProject.so -passes="unit-licm"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/ValueTracking.h"

#include "UnitLICM.h"
#include "UnitLoopInfo.h"


using namespace llvm;
using namespace cs426;
#define DEBUG_TYPE "UnitLICM"
// Define any statistics here
STATISTIC(NumStoreHoisted, "Number of store instructions hoisted");
STATISTIC(NumLoadHoisted, "Number of load instructions hoisted");
STATISTIC(NumComputeHoisted, "Number of computes hoisted");

STATISTIC(Total, "Total number of computes hoisted");

void InnerToOuterLoops(UnitLoopInfo& Loops, BasicBlock* loop_header, std::vector<LoopRange>& result) {
  LoopMeta* loop_info = Loops.m_HeaderLoopMeta[loop_header];
  
  for (auto& [back_src, members] : loop_info->m_LoopMemberBlocks) {
    for (auto& child_ranges : loop_info->m_ChildrenLoopHeader[back_src]) {
      InnerToOuterLoops(Loops, child_ranges.first, result);
    }
    result.push_back({loop_header, back_src});
  }
}

bool ShouldConsiderInstr(Instruction& I) {
    // Check for binary, unary, bitwise logic, and cast instructions
    if (I.isBinaryOp() || I.isUnaryOp() || I.isBitwiseLogicOp() || I.isCast() || isa<ICmpInst>(I) || isa<FCmpInst>(I) || isa<SelectInst>(I) \
    || isa<GetElementPtrInst>(I) || isa<StoreInst>(I) || isa<LoadInst>(I)) {
      return true;
    }
    return false;
}

bool AliasExistsInLoop(AAResults& AA, Instruction& target, UnitLoopInfo& Loops, BasicBlock* start, BasicBlock* end) {
  for (BasicBlock* BB : Loops.m_HeaderLoopMeta[start]->m_LoopMemberBlocks[end]) {
    for (Instruction& I : *BB) {
      if ((isa<StoreInst>(I) || isa<LoadInst>(I)) && (&target != &I)) {
        if (StoreInst* target_casted = dyn_cast<StoreInst>(&target)) {
          if (StoreInst* I_casted = dyn_cast<StoreInst>(&I)) {
            if (AA.alias(target_casted->getPointerOperand(), I_casted->getPointerOperand()) != AliasResult::NoAlias) {
              return true;
            }
          } else if (LoadInst* I_casted = dyn_cast<LoadInst>(&I)) {
            if (AA.alias(target_casted->getPointerOperand(), I_casted->getPointerOperand()) != AliasResult::NoAlias) {
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}

void UpdateStatistics(Instruction* instr) {
  if (isa<LoadInst>(*instr)) {
    NumLoadHoisted++;
  } else if (isa<StoreInst>(*instr)) {
    NumStoreHoisted++;
  } else if (!isa<CastInst>(*instr) && !isa<GetElementPtrInst>(*instr)) {
    NumComputeHoisted++;
  }
  Total++;
}

/// Main function for running the LICM optimization
PreservedAnalyses UnitLICM::run(Function& F, FunctionAnalysisManager& FAM) {
  dbgs() << "UnitLICM running on " << F.getName() << "\n";
  // Acquires the UnitLoopInfo object constructed by your Loop Identification
  // (LoopAnalysis) pass
  UnitLoopInfo &Loops = FAM.getResult<UnitLoopAnalysis>(F);
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  AAResults &AA = FAM.getResult<AAManager>(F);


  // Perform the optimization
  // For simplicity, just add a preheader for each loop 
  std::unordered_map<BasicBlock*, BasicBlock*> LoopPreHeaders;
  for (auto& [loop_header, loop_info] : Loops.m_HeaderLoopMeta) {
    // BB creation alters the entry point of the predecessor list of the original loop header, need to precomputed
    std::vector<BasicBlock*> original_enter_nodes;
    for (auto pred : predecessors(loop_header)) {
      // dbgs() << "original predecessor: " << *pred->getTerminator() << '\n';
      if (!DT.dominates(loop_header, pred)) {
        original_enter_nodes.push_back(pred);
      }
    }
    if (original_enter_nodes.size() == 1) {
      LoopPreHeaders[loop_header] = original_enter_nodes[0];
    } else {
      BasicBlock* preheader = BasicBlock::Create(loop_header->getContext(), "", loop_header->getParent(), loop_header);
      for (BasicBlock* pred : original_enter_nodes) {
        Instruction* last_instr = pred->getTerminator();
        last_instr->replaceSuccessorWith(loop_header, preheader);
        loop_header->replacePhiUsesWith(pred, preheader);
      }
      BranchInst* branch_instr = BranchInst::Create(loop_header, preheader);
      LoopPreHeaders[loop_header] = preheader;
      dbgs() << "Loop preheader for " << loop_header->front() << " added\n";
    }
  }


  for (BasicBlock* outermost_loop_header : Loops.m_OuterMostLoopHeaders) {
    std::vector<LoopRange> opt_order;
    InnerToOuterLoops(Loops, outermost_loop_header, opt_order);
    for (LoopRange range : opt_order) {
      // Optimizing for a loop
      std::vector<Instruction*> instr_to_move;
      std::unordered_set<Instruction*> invariant_instrs;
      for (BasicBlock* BB : Loops.m_HeaderLoopMeta[range.loop_header]->m_LoopMemberBlocks[range.back_edge_src]) {
        for (Instruction& I : *BB) {
          // Skip instructions that are required to be considered
          if (!ShouldConsiderInstr(I)) {
            continue;
          }

          bool is_invariant = true;
          // A load/store instruction can be hoisted if no alias in the loop
          if (isa<StoreInst>(I) || isa<LoadInst>(I)) {
            if (AliasExistsInLoop(AA, I, Loops, range.loop_header, range.back_edge_src)) {
              is_invariant = false;
            }
          }
          // } else {
            // A hoist candidate must dominates all exit blocks
            for (BasicBlock* exit_block : Loops.m_HeaderLoopMeta[range.loop_header]->m_LoopExitBlocks[range.back_edge_src]) {
              if (!DT.dominates(BB, exit_block)) {
                is_invariant = false;
                break;
              }
            }
            // Even if not dominating all exits, speculative hoist non-excepting instructions
            if (!is_invariant && isSafeToSpeculativelyExecute(&I)) {
              is_invariant = true;
            }
            // If invariant candidate so far, check whether data dependency outside the loop
            if (is_invariant) {
              for (Use& use : I.operands()) {
                Value* value = use.get();
                // If value is an instruction that instruction has to be invariant too.
                // If value is not an instruction, then current instruction is safe to be invariant because the value is something like constant
                if (Instruction* instr = dyn_cast<Instruction>(value)) {
                  // Check if the def of the use is in the loop
                  BasicBlock* block = instr->getParent();
                  std::vector<BasicBlock*>& members = Loops.m_HeaderLoopMeta[range.loop_header]->m_LoopMemberBlocks[range.back_edge_src];
                  if (std::find(members.begin(), members.end(), block) != members.end()) {
                    if (!invariant_instrs.count(instr)) {
                      is_invariant = false;
                      break;
                    }
                  }
                }
              }
            }
          
          // Instruction reaching this point should be invariant
          if (is_invariant) {
            invariant_instrs.insert(&I);
            instr_to_move.push_back(&I);
          }
        }
      }
      // Hoist the instructions
      for (Instruction* instr : instr_to_move) {
        dbgs() << "Instruction to be moved: " << *instr << '\n';
        BasicBlock* BB =LoopPreHeaders[range.loop_header];
        Instruction* insert_point = LoopPreHeaders[range.loop_header]->getTerminator();
        instr->moveBefore(insert_point);
      }

      // Finally get the statistics...
      for (Instruction* instr : instr_to_move) {
        UpdateStatistics(instr);
      }

    }
  }

  dbgs() << "[UnitLICM]: NumStoreHoisted: " << NumStoreHoisted << '\n';
  dbgs() << "[UnitLICM]: NumLoadHoisted: " << NumLoadHoisted << '\n';
  dbgs() << "[UnitLICM]: NumComputeHoisted: " << NumComputeHoisted << '\n';
  dbgs() << "[UnitLICM]: Total: " << Total << '\n';


  // Set proper preserved analyses
  // Since the CFG is now changed, need to rerun the analysis passes again
  PreservedAnalyses PA;
  return PA;

  
}
