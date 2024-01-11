//===-- My66000RegisterInfo.cpp - My66000 Register Information ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the My66000 implementation of the RegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "My66000RegisterInfo.h"
#include "My66000.h"
#include "My66000InstrInfo.h"
#include "My66000MachineFunctionInfo.h"
#include "My66000Subtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

#define DEBUG_TYPE "my66000-reg-info"

#define GET_REGINFO_TARGET_DESC
#include "My66000GenRegisterInfo.inc"

static const Register FPReg = My66000::R30;
static const Register SPReg = My66000::SP;

My66000RegisterInfo::My66000RegisterInfo()
  : My66000GenRegisterInfo(My66000::R0) {
}



bool My66000RegisterInfo::needsFrameMoves(const MachineFunction &MF) {
  return MF.getMMI().hasDebugInfo() || MF.getFunction().needsUnwindTableEntry();
}

const MCPhysReg *
My66000RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
LLVM_DEBUG(dbgs() << "My66000RegisterInfo::getCalleeSavedRegs\n");
  return CSR_ABI_SaveList;
}

BitVector My66000RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  const My66000FrameLowering *TFI = getFrameLowering(MF);

  Reserved.set(SPReg);
  Reserved.set(My66000::R0);
  if (TFI->hasFP(MF)) {
    Reserved.set(FPReg);
  }
  return Reserved;
}

const TargetRegisterClass*
My66000RegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                        unsigned Kind) const {
  return &My66000::GRegsRegClass;
}

bool
My66000RegisterInfo::requiresRegisterScavenging(const MachineFunction &MF) const {
  return true;
}

bool
My66000RegisterInfo::trackLivenessAfterRegAlloc(const MachineFunction &MF) const {
  return true;
}

bool
My66000RegisterInfo::useFPForScavengingIndex(const MachineFunction &MF) const {
  return false;
}

bool
My66000RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                       int SPAdj, unsigned FIOperandNum,
                                       RegScavenger *RS) const {
LLVM_DEBUG(dbgs() << "My66000RegisterInfo::eliminateFrameIndex\n");
  assert(SPAdj == 0 && "Unexpected non-zero SPAdj value");

  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  DebugLoc DL = MI.getDebugLoc();

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  Register FrameReg;
  int Offset = getFrameLowering(MF)
	->getFrameIndexReference(MF, FrameIndex, FrameReg).getFixed() +
	MI.getOperand(FIOperandNum + 1).getImm();

  if (!isInt<32>(Offset)) {
    report_fatal_error(
        "Frame offsets outside of the signed 32-bit range not supported");
  }

  bool FrameRegIsKill = false;

  MI.getOperand(FIOperandNum)
      .ChangeToRegister(FrameReg, false, false, FrameRegIsKill);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);

  return false;		// FIXME - what is this?
}


Register My66000RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const My66000FrameLowering *TFI = getFrameLowering(MF);

  return TFI->hasFP(MF) ? FPReg : SPReg;
}

const uint32_t *
My66000RegisterInfo::getCallPreservedMask(const MachineFunction & MF,
                                        CallingConv::ID /*CC*/) const {
LLVM_DEBUG(dbgs() << "My66000RegisterInfo::getCallPreservedMask\n");
  return CSR_ABI_RegMask;
}
