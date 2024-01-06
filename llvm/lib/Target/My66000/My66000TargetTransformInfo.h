//===-- My66000TargetTransformInfo.h - My66000 specific TTI ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// This file a TargetTransformInfo::Concept conforming object specific to the
/// My66000 target machine. It uses the target's detailed information to
/// provide more precise answers to certain TTI queries, while letting the
/// target independent and default TTI implementations handle the rest.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MY66000TARGETTRANSFORMINFO_H
#define LLVM_LIB_TARGET_MY66000_MY66000TARGETTRANSFORMINFO_H

#include "My66000.h"
#include "My66000TargetMachine.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/BasicTTIImpl.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class My66000TTIImpl : public BasicTTIImplBase<My66000TTIImpl> {
  typedef BasicTTIImplBase<My66000TTIImpl> BaseT;
  typedef TargetTransformInfo TTI;
  friend BaseT;

  const My66000Subtarget *ST;
  const My66000TargetLowering *TLI;

  const My66000Subtarget *getST() const { return ST; }
  const My66000TargetLowering *getTLI() const { return TLI; }

public:
  explicit My66000TTIImpl(const My66000TargetMachine *TM, const Function &F)
      : BaseT(TM, F.getParent()->getDataLayout()), ST(TM->getSubtargetImpl()),
        TLI(ST->getTargetLowering()) {}

  // FIXME - should the following be put into a My66000TargetTransformInfo.cpp.
  bool hasDivRemOp(Type *DataType, bool IsSigned)
  { return true; }	// FIXME

  bool isNumRegsMajorCostOfLSR()
  { return false; }

  bool isLSRCostLess(const TargetTransformInfo::LSRCost &C1,
                     const TargetTransformInfo::LSRCost &C2) {
    // My66000 specific here are "instruction number 1st priority".
    return std::tie(C1.Insns, C1.NumRegs, C1.AddRecCost,
                    C1.NumIVMuls, C1.NumBaseAdds,
                    C1.ScaleCost, C1.ImmCost, C1.SetupCost) <
           std::tie(C2.Insns, C2.NumRegs, C2.AddRecCost,
                    C2.NumIVMuls, C2.NumBaseAdds,
                    C2.ScaleCost, C2.ImmCost, C2.SetupCost);
  }
};

} // end namespace llvm

#endif
