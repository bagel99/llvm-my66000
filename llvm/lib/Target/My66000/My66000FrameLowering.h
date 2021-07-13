//===-- My66000FrameLowering.h - Frame info for My66000 Target ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains My66000 frame information that doesn't fit anywhere else
// cleanly...
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MY66000FRAMELOWERING_H
#define LLVM_LIB_TARGET_MY66000_MY66000FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
  class My66000Subtarget;

  class My66000FrameLowering: public TargetFrameLowering {
  public:
    explicit My66000FrameLowering(const My66000Subtarget &STI)
      : TargetFrameLowering(StackGrowsDown,
			    /*StackAlignment=*/Align(8),
			    /*LocalAreaOffset=*/0),
        STI(STI) {}


    /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
    /// the function.
    void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
    void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

    StackOffset getFrameIndexReference(const MachineFunction &MF, int FI,
                             Register &FrameReg) const override;

    bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
				 ArrayRef<CalleeSavedInfo> CSI,
                                 const TargetRegisterInfo *TRI) const override;

    bool restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MI,
			      MutableArrayRef<CalleeSavedInfo> CSI,
                              const TargetRegisterInfo *TRI) const override;

    MachineBasicBlock::iterator
    eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I) const override;
    bool hasFP(const MachineFunction &MF) const override;

    void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs,
                              RegScavenger *RS = nullptr) const override;

    void processFunctionBeforeFrameFinalized(MachineFunction &MF,
                                     RegScavenger *RS = nullptr) const override;

    static int stackSlotSize() {
      return 8;
    }

  protected:
    const My66000Subtarget &STI;

  private:
    void determineFrameLayout(MachineFunction &MF) const;
    void adjustReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
		   const DebugLoc &DL, Register DstReg, Register SrcReg,
		   int64_t Val, MachineInstr::MIFlag Flag) const;

  };
}

#endif
