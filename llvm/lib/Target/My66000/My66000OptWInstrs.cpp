//===- My66000OptWInstrs.cpp - MI W instruction optimizations ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//
//
// This pass does some optimizations for sign/zero extension at the MI level.
//
//===---------------------------------------------------------------------===//

#include "My66000.h"
#include "My66000Subtarget.h"
#include "My66000MachineFunctionInfo.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

using namespace llvm;

#define DEBUG_TYPE "my66000-opt-ext-instrs"
#define MY66000_OPT_W_INSTRS_NAME "My66000 Optimize W Instructions"

STATISTIC(NumRemovedSExtW, "Number of removed sign-extensions");
STATISTIC(NumRemovedZExtW, "Number of removed zero-extensions");

static cl::opt<bool> EnableSmashS("enable-smash-sign", cl::Hidden,
  cl::desc("Enable elimination of redundant sign extensions"));
static cl::opt<bool> EnableSmashU("enable-smash-zero", cl::Hidden,
  cl::desc("Enable elimination of redundant zero extensions"));

namespace {

class My66000OptWInstrs : public MachineFunctionPass {
public:
  static char ID;

  My66000OptWInstrs() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  bool removeSExtWInstrs(MachineFunction &MF, const My66000InstrInfo &TII,
                         const My66000Subtarget &ST, MachineRegisterInfo &MRI);
};

} // end anonymous namespace

char My66000OptWInstrs::ID = 0;
INITIALIZE_PASS(My66000OptWInstrs, DEBUG_TYPE,  MY66000_OPT_W_INSTRS_NAME, false,
                false)

FunctionPass *llvm::createMy66000OptWInstrsPass() {
  return new My66000OptWInstrs();
}

static bool hasSingleUse(const MachineInstr *MI, const My66000Subtarget &ST,
			 const MachineRegisterInfo &MRI) {


  Register DstReg = MI->getOperand(0).getReg();
#ifndef NDEBUG
  for (auto &UserOp : MRI.use_nodbg_operands(DstReg)) {
    const MachineInstr *UserMI = UserOp.getParent();
LLVM_DEBUG(dbgs() << "User: " << *UserMI);
  }
#endif
  return MRI.hasOneUse(DstReg);
}

static bool isExtendedW(bool tryUnsigned, Register SrcReg, const My66000Subtarget &ST,
                            const MachineRegisterInfo &MRI) {

  SmallPtrSet<const MachineInstr *, 4> Visited;
  SmallVector<MachineInstr *, 4> Worklist;
  uint64_t mask = tryUnsigned ? 0x04 : 0x02;

  auto AddRegDefToWorkList = [&](Register SrcReg) {
    if (!SrcReg.isVirtual())
      return false;
    MachineInstr *SrcMI = MRI.getVRegDef(SrcReg);
    if (!SrcMI)
      return false;
    // Code assumes the register is operand 0.
    // TODO: Maybe the worklist should store register?
    if (!SrcMI->getOperand(0).isReg() ||
        SrcMI->getOperand(0).getReg() != SrcReg)
      return false;
    // Add SrcMI to the worklist.
    Worklist.push_back(SrcMI);
    return true;
  };

  if (!AddRegDefToWorkList(SrcReg))
    return false;

  while (!Worklist.empty()) {
    MachineInstr *MI = Worklist.pop_back_val();
LLVM_DEBUG(dbgs() << "SrcMI: " << *MI);

    // If we already visited this instruction, we don't need to check it again.
    if (!Visited.insert(MI).second)
      continue;
    // If this is a extending operation we don't need to look any further.
    if (MI->getDesc().TSFlags & mask)
{
LLVM_DEBUG(dbgs() << "Removed " << (tryUnsigned ? "zero" : "sign") <<
	    "-extended opcode\n");
      continue;
}
    // Is this an instruction that propagates sign extend?
    switch (MI->getOpcode()) {
    default:
      // Unknown opcode, give up.
LLVM_DEBUG(dbgs() << "Failed on opcode\n\n");
      return false;
    case My66000::CMOVrrr:
    case My66000::CMOVrnr:
    case My66000::CMOVrrn:
    case My66000::CMOVrrw:
    case My66000::CMOVrwr: {
      // Operand(3) is control which selects either Operand(1) or Operand(2)
      if (MI->getOperand(1).isReg()) {
        if (!AddRegDefToWorkList(MI->getOperand(1).getReg()))
          return false;
      }
      if (MI->getOperand(2).isReg()) {
        if (!AddRegDefToWorkList(MI->getOperand(2).getReg()))
          return false;
      }
      break;
    }
    case My66000::COPY: {
      const MachineFunction *MF = MI->getMF();
      const My66000MachineFunctionInfo *MFI =
          MF->getInfo<My66000MachineFunctionInfo>();

      // If this is the entry block and the register is livein, see if we know
      // it is sign extended.
      if (MI->getParent() == &MF->front()) {
        Register VReg = MI->getOperand(0).getReg();
        if (MF->getRegInfo().isLiveIn(VReg)) {
	  if ((tryUnsigned && MFI->isZExtRegister(VReg)) ||
	                      MFI->isSExtRegister(VReg))
{
LLVM_DEBUG(dbgs() << "Removed COPY of sign/zero extended register\n");
          continue;
}
	}
      }
      Register CopySrcReg = MI->getOperand(1).getReg();
      if (!AddRegDefToWorkList(CopySrcReg))
        return false;
      break;
    }
    case My66000::PHI: {
      unsigned E = MI->getNumOperands();
      for (unsigned I = 1; I != E; I += 2) {
        if (!MI->getOperand(I).isReg())
          return false;
        if (!AddRegDefToWorkList(MI->getOperand(I).getReg()))
          return false;
      }
      break;
    }
    case My66000::LDUWri:
    case My66000::LDUWrr: {
      if (hasSingleUse(MI, ST, MRI)) {
LLVM_DEBUG(dbgs() << "hasSingleUse\n");
      }
      return false;	// FIXME
      break;
    }
    }
  }
  // If we get here, then every node we visited produces a sign extended value
  // or propagated sign extended values. So the result must be sign extended.
  return true;
}


bool My66000OptWInstrs::removeSExtWInstrs(MachineFunction &MF,
                                          const My66000InstrInfo &TII,
                                          const My66000Subtarget &ST,
					  MachineRegisterInfo &MRI) {
  bool MadeChange = false;
  bool isSign, isZero;
  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : llvm::make_early_inc_range(MBB)) {
      isSign = EnableSmashS ? TII.isSEXTW(MI) : false;
      isZero = EnableSmashU ? TII.isZEXTW(MI) : false;
      if (!isSign && !isZero)
        continue;
LLVM_DEBUG(dbgs() << "Found EXTW in " << printMBBReference(MBB) << ": " << MI);
      Register SrcReg = MI.getOperand(1).getReg();
      // If all definitions reaching MI extend their output,
      // then SEXTW or ZEXTW is redundant.
      if (!isExtendedW(isZero, SrcReg, ST, MRI))
	continue;
      Register DstReg = MI.getOperand(0).getReg();
      LLVM_DEBUG(dbgs() << "Removing redundant " << (isSign ? "sign" : "zero") <<
	    "-extension\n\n");
      MRI.replaceRegWith(DstReg, SrcReg);
      MRI.clearKillFlags(SrcReg);
      MI.eraseFromParent();
      if (isZero) ++NumRemovedZExtW; else ++NumRemovedSExtW;
      MadeChange = true;
    }
  }
  return MadeChange;
}


bool My66000OptWInstrs::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(MF.getFunction())) return false;
  if (!EnableSmashS && !EnableSmashU) return false;

  MachineRegisterInfo &MRI = MF.getRegInfo();
  const My66000Subtarget &ST = MF.getSubtarget<My66000Subtarget>();
  const My66000InstrInfo &TII = *ST.getInstrInfo();

LLVM_DEBUG(dbgs() << "RemoveSExtWInstrsPass: " << MF.getName() << '\n');
  bool MadeChange = false;
  MadeChange |= removeSExtWInstrs(MF, TII, ST, MRI);

  return MadeChange;
}
