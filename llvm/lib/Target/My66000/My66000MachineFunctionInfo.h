//=== My66000MachineFunctionInfo.h - My66000 machine function info -------*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares My66000-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_MY66000MACHINEFUNCTIONINFO_H

#include "My66000FrameLowering.h"
#include "My66000ISelLowering.h"
#include "My66000InstrInfo.h"
#include "My66000SelectionDAGInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include <cassert>
#include <utility>
#include <vector>

namespace llvm {

/// My66000MachineFunctionInfo - This class is derived from MachineFunction private
/// My66000 target-specific information for each MachineFunction.
class My66000MachineFunctionInfo : public MachineFunctionInfo {
  virtual void anchor();

  unsigned ReturnStackOffset;
  bool ReturnStackOffsetSet = false;
  int VarArgsFrameIndex = 0;
  int VarArgsSaveSize = 0;
  mutable int CachedEStackSize = -1;
  std::vector<std::pair<MachineBasicBlock::iterator, CalleeSavedInfo>>
  SpillLabels;
  unsigned LoSavedReg;
  unsigned HiSavedReg;
  /// Registers that have been sign extended.
  SmallVector<Register, 8> SExtRegisters;
  /// Registers that have been zero extended..
  SmallVector<Register, 8> ZExtRegisters;


public:
  My66000MachineFunctionInfo(const Function &F, const TargetSubtargetInfo *STI)
      : ReturnStackOffset(false), VarArgsFrameIndex(0), VarArgsSaveSize(0) {}

  ~My66000MachineFunctionInfo() {}

  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override;

  void setVarArgsFrameIndex(int off) { VarArgsFrameIndex = off; }
  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }

  unsigned getVarArgsSaveSize() const { return VarArgsSaveSize; }
  void setVarArgsSaveSize(int Size) { VarArgsSaveSize = Size; }

  void setReturnStackOffset(unsigned value) {
    assert(!ReturnStackOffsetSet && "Return stack offset set twice");
    ReturnStackOffset = value;
    ReturnStackOffsetSet = true;
  }

  unsigned getReturnStackOffset() const {
    assert(ReturnStackOffsetSet && "Return stack offset not set");
    return ReturnStackOffset;
  }

  std::vector<std::pair<MachineBasicBlock::iterator, CalleeSavedInfo>> &
  getSpillLabels() {
    return SpillLabels;
  }

  // Get and set the first call-saved GPR that should be saved and restored
  // by this function.  This is 0 if no GPRs need to be saved or restored.
  unsigned getLoSavedReg() const { return LoSavedReg; }
  void setLoSavedReg(unsigned Reg) { LoSavedReg = Reg; }

  // Get and set the last call-saved GPR that should be saved and restored
  // by this function.
  unsigned getHiSavedReg() const { return HiSavedReg; }
  void setHiSavedReg(unsigned Reg) { HiSavedReg = Reg; }

  void addSExtRegister(Register Reg);
  bool isSExtRegister(Register Reg) const;

  void addZExtRegister(Register Reg);
  bool isZExtRegister(Register Reg) const;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_MY66000MACHINEFUNCTIONINFO_H
