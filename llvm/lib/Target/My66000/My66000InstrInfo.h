//===- My66000InstrInfo.h - My66000 Instruction Information -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the My66000 implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MY66000INSTRINFO_H
#define LLVM_LIB_TARGET_MY66000_MY66000INSTRINFO_H

#include "My66000RegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "My66000GenInstrInfo.inc"

namespace llvm {
class My66000Subtarget;

class My66000InstrInfo : public My66000GenInstrInfo {
  const My66000RegisterInfo RI;
  virtual void anchor();

public:
  explicit My66000InstrInfo(const My66000Subtarget &ST);

//  const TargetRegisterInfo &getRegisterInfo() const { return RI; }

  void copyPhysReg(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator I, const DebugLoc &DL,
      Register DstReg, Register SrcReg, bool KillSrc,
      bool RenamableDest = false, bool RenamableSrc = false) const override;

  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
      bool isKill, int FrameIndex, const TargetRegisterClass *RC,
      Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;

  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
      int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      unsigned SubReg = 0,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;

  bool analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify) const override;

  unsigned insertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL,
                        int *BytesAdded = nullptr) const override;

  unsigned removeBranch(MachineBasicBlock &MBB,
                        int *BytesRemoved = nullptr) const override;

  unsigned reverseBRC(MYCC::CondCodes cc) const;

  unsigned reverseBRIB(MYCB::CondBits cb) const;

  unsigned reverseBRFB(MYCB::CondBits cb) const;	// FIXME - use float CB

  bool
  reverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;

  bool isAsCheapAsAMove(const MachineInstr &MI) const override;

  const My66000RegisterInfo &getRegisterInfo() const { return RI; }

  unsigned getInstSizeInBytes(const MachineInstr &MI) const override;

  bool isSEXTW(const MachineInstr &MI) const;

  bool isZEXTW(const MachineInstr &MI) const;
};

}  // end namespace llvm

#endif
