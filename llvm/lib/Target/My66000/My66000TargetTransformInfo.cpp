//===-- My66000TargetTransformInfo.cpp - mY66000-specific TTI -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements a TargetTransformInfo analysis pass specific to the
// target machine. It uses the target's detailed information to provide
// more precise answers to certain TTI queries, while letting the target
// independent and default TTI implementations handle the rest.
//
//===----------------------------------------------------------------------===//

#include "My66000TargetTransformInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/CostTable.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Debug.h"
using namespace llvm;

#define DEBUG_TYPE "my66000tti"

InstructionCost My66000TTIImpl::getIntImmCost(const APInt &Imm, Type *Ty,
                                              TTI::TargetCostKind CostKind) const {
  assert(Ty->isIntegerTy());
  return TTI::TCC_Free;
}

InstructionCost My66000TTIImpl::getIntImmCostInst(unsigned Opcode, unsigned Idx,
                                                  const APInt &Imm, Type *Ty,
                                                  TTI::TargetCostKind CostKind,
                                                  Instruction *Inst) const {
  assert(Ty->isIntegerTy());
  return TTI::TCC_Free;
}

bool My66000TTIImpl::isLSRCostLess(const TargetTransformInfo::LSRCost &C1,
                                   const TargetTransformInfo::LSRCost &C2) const {
    // My66000 specific here are "instruction number 1st priority".
LLVM_DEBUG(dbgs() << "isLSRCostLess\n");
LLVM_DEBUG(dbgs() << "Insts       " << C1.Insns << " : " << C2.Insns << '\n');
LLVM_DEBUG(dbgs() << "NumRegs     " << C1.NumRegs << " : " << C2.NumRegs << '\n');
LLVM_DEBUG(dbgs() << "AddRecCost  " << C1.AddRecCost << " : " << C2.AddRecCost << '\n');
LLVM_DEBUG(dbgs() << "NumIVMuls   " << C1.NumIVMuls << " : " << C2.NumIVMuls << '\n');
LLVM_DEBUG(dbgs() << "NumBaseAdds " << C1.NumBaseAdds << " : " << C2.NumBaseAdds << '\n');
LLVM_DEBUG(dbgs() << "ScaleCost   " << C1.ScaleCost << " : " << C2.ScaleCost << '\n');
LLVM_DEBUG(dbgs() << "ImmCost     " << C1.ImmCost << " : " << C2.ImmCost << '\n');
LLVM_DEBUG(dbgs() << "SetupCost   " << C1.SetupCost << " : " << C2.SetupCost << '\n');
    return std::tie(C1.Insns, C1.NumRegs, C1.AddRecCost,
                    C1.NumIVMuls, C1.NumBaseAdds,
                    C1.ScaleCost, C1.ImmCost, C1.SetupCost) <
           std::tie(C2.Insns, C2.NumRegs, C2.AddRecCost,
                    C2.NumIVMuls, C2.NumBaseAdds,
                    C2.ScaleCost, C2.ImmCost, C2.SetupCost);
}

void My66000TTIImpl::getUnrollingPreferences(Loop *L, ScalarEvolution &SE,
                               TTI::UnrollingPreferences &UP,
                               OptimizationRemarkEmitter *ORE) const {
  UP.OptSizeThreshold = 0;
  UP.PartialOptSizeThreshold = 0;
  UP.Threshold = 0;	// FIXME - does this really turnoff unrolling?

}
