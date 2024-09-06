//===-- My66000ExpandPseudoInsts.cpp - Expand pseudo instructions ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions that must be done after register allocation.
//
//===----------------------------------------------------------------------===//

#include "My66000.h"
#include "My66000MachineFunctionInfo.h"
#include "My66000TargetMachine.h"
#include "My66000Subtarget.h"


using namespace llvm;

#define DEBUG_TYPE "my66000-pseudo"

#define PASS_NAME "My66000 pseudo instruction expansion"

namespace {

  class My66000ExpandPseudo : public MachineFunctionPass {
   public:
    static char ID;
    My66000ExpandPseudo() : MachineFunctionPass(ID) {}

    const My66000InstrInfo *TII;

    bool runOnMachineFunction(MachineFunction &Fn) override;

    StringRef getPassName() const override {
      return PASS_NAME;
    }

  private:
    bool CarryO(MachineBasicBlock &MBB,
		        MachineBasicBlock::iterator MBBI, unsigned inst);
    bool CarryIO(MachineBasicBlock &MBB,
		        MachineBasicBlock::iterator MBBI, unsigned inst);
    bool ShfIO(MachineBasicBlock &MBB,
		        MachineBasicBlock::iterator MBBI, unsigned inst);
    bool ExpandMI(MachineBasicBlock &MBB,
                  MachineBasicBlock::iterator MBBI,
                  MachineBasicBlock::iterator &NextMBBI);
    bool ExpandMBB(MachineBasicBlock &MBB);

  };
  char My66000ExpandPseudo::ID = 0;
}


INITIALIZE_PASS(My66000ExpandPseudo, DEBUG_TYPE, PASS_NAME, false, false)

bool My66000ExpandPseudo::ShfIO(MachineBasicBlock &MBB,
				  MachineBasicBlock::iterator MBBI,
				  unsigned inst) {
  MachineInstr &MI = *MBBI;
  MachineInstr *Carry, *Inst;
  Carry = BuildMI(MBB, MBBI, MI.getDebugLoc(), TII->get(My66000::CARRYio))
	  .add(MI.getOperand(1))
	  .add(MI.getOperand(3))
	  .addImm(3);	// {IO}
  Inst  = BuildMI(MBB, MBBI, MI.getDebugLoc(), TII->get(inst))
	  .add(MI.getOperand(0))
	  .add(MI.getOperand(2))
	  .addImm(0)			// full width
	  .add(MI.getOperand(4));	// offset
  finalizeBundle(MBB, Carry->getIterator(), ++Inst->getIterator());
  MI.eraseFromParent();
  return true;
}

bool My66000ExpandPseudo::CarryIO(MachineBasicBlock &MBB,
				  MachineBasicBlock::iterator MBBI,
				  unsigned inst) {
  MachineInstr &MI = *MBBI;
  MachineInstr *Carry, *Inst;
  unsigned CarryFlag = MI.getOperand(1).isDead() ? 1: 3;
  Carry = BuildMI(MBB, MBBI, MI.getDebugLoc(), TII->get(My66000::CARRYio))
	  .add(MI.getOperand(1))
	  .add(MI.getOperand(4))
	  .addImm(CarryFlag);	// {IO} or {I}
  Inst  = BuildMI(MBB, MBBI, MI.getDebugLoc(), TII->get(inst))
	  .add(MI.getOperand(0))
	  .add(MI.getOperand(2))
	  .add(MI.getOperand(3));
  finalizeBundle(MBB, Carry->getIterator(), ++Inst->getIterator());
  MI.eraseFromParent();
  return true;
}

bool My66000ExpandPseudo::CarryO(MachineBasicBlock &MBB,
				 MachineBasicBlock::iterator MBBI,
				 unsigned inst) {
  MachineInstr &MI = *MBBI;
  MachineInstr *Carry, *Inst;
  Carry = BuildMI(MBB, MBBI, MI.getDebugLoc(), TII->get(My66000::CARRYo))
	  .add(MI.getOperand(1))
	  .addImm(2);	// {O}
  Inst  = BuildMI(MBB, MBBI, MI.getDebugLoc(), TII->get(inst))
	  .add(MI.getOperand(0))
	  .add(MI.getOperand(2))
	  .add(MI.getOperand(3));
  finalizeBundle(MBB, Carry->getIterator(), ++Inst->getIterator());
  MI.eraseFromParent();
  return true;
}

bool My66000ExpandPseudo::ExpandMI(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MBBI,
                               MachineBasicBlock::iterator &NextMBBI) {
  MachineInstr &MI = *MBBI;
  unsigned Opcode = MI.getOpcode();
  switch (Opcode) {
    default:
      return false;
    case My66000::UADDOrrc:	return CarryO(MBB, MBBI, My66000::ADDrr);
    case My66000::UADDOric:	return CarryO(MBB, MBBI, My66000::ADDri);
    case My66000::UADDOrwc:	return CarryO(MBB, MBBI, My66000::ADDrw);
    case My66000::UADDOrdc:	return CarryO(MBB, MBBI, My66000::ADDrd);
    case My66000::USUBOrrc:	return CarryO(MBB, MBBI, My66000::ADDrn);
    case My66000::USUBOric:	return CarryO(MBB, MBBI, My66000::ADDri);
    case My66000::ADDrrbc:	return CarryIO(MBB, MBBI, My66000::ADDrr);
    case My66000::ADDribc:	return CarryIO(MBB, MBBI, My66000::ADDri);
    case My66000::ADDrwbc:	return CarryIO(MBB, MBBI, My66000::ADDrw);
    case My66000::ADDrdbc:	return CarryIO(MBB, MBBI, My66000::ADDrd);
    case My66000::SUBrrbc:	return CarryIO(MBB, MBBI, My66000::ADDrn);
    case My66000::SUBribc:	return CarryIO(MBB, MBBI, My66000::ADDri);
    case My66000::UMULHILOrrc:	return CarryO(MBB, MBBI, My66000::MULrr);
    case My66000::UMULHILOric:	return CarryO(MBB, MBBI, My66000::MULri);
    case My66000::UMULHILOrwc:	return CarryO(MBB, MBBI, My66000::MULrw);
    case My66000::UMULHILOrdc:	return CarryO(MBB, MBBI, My66000::MULrd);
    case My66000::SMULHILOrrc:	return CarryO(MBB, MBBI, My66000::MULrr);
    case My66000::SMULHILOric:	return CarryO(MBB, MBBI, My66000::MULri);
    case My66000::SMULHILOrwc:	return CarryO(MBB, MBBI, My66000::MULrw);
    case My66000::SMULHILOrdc:	return CarryO(MBB, MBBI, My66000::MULrd);
    case My66000::UDIVREMrrc:	return CarryO(MBB, MBBI, My66000::UDIVrr);
    case My66000::UDIVREMric:	return CarryO(MBB, MBBI, My66000::UDIVri);
    case My66000::UDIVREMrwc:	return CarryO(MBB, MBBI, My66000::UDIVrw);
    case My66000::UDIVREMwrc:	return CarryO(MBB, MBBI, My66000::UDIVwr);
    case My66000::UDIVREMrdc:	return CarryO(MBB, MBBI, My66000::UDIVrd);
    case My66000::UDIVREMdrc:	return CarryO(MBB, MBBI, My66000::UDIVdr);
    case My66000::SDIVREMrrc:	return CarryO(MBB, MBBI, My66000::SDIVrr);
    case My66000::SDIVREMrnc:	return CarryO(MBB, MBBI, My66000::SDIVrn);
    case My66000::SDIVREMnrc:	return CarryO(MBB, MBBI, My66000::SDIVnr);
    case My66000::SDIVREMnnc:	return CarryO(MBB, MBBI, My66000::SDIVnn);
    case My66000::SDIVREMrxc:	return CarryO(MBB, MBBI, My66000::SDIVrx);
    case My66000::SDIVREMwrc:	return CarryO(MBB, MBBI, My66000::SDIVwr);
    case My66000::SDIVREMrdc:	return CarryO(MBB, MBBI, My66000::SDIVrd);
    case My66000::SDIVREMdrc:	return CarryO(MBB, MBBI, My66000::SDIVdr);
    case My66000::FREMrrc:	return CarryO(MBB, MBBI, My66000::FDIVrr);
    case My66000::FREMrdc:	return CarryO(MBB, MBBI, My66000::FDIVrd);
    case My66000::FREMrfc:	return CarryO(MBB, MBBI, My66000::FDIVrf);
    case My66000::FREMrkc:	return CarryO(MBB, MBBI, My66000::FDIVrk);
    case My66000::FREMdrc:	return CarryO(MBB, MBBI, My66000::FDIVdr);
    case My66000::FREMfrc:	return CarryO(MBB, MBBI, My66000::FDIVfr);
    case My66000::FREMkrc:	return CarryO(MBB, MBBI, My66000::FDIVkr);
    case My66000::FREMFrrc:	return CarryO(MBB, MBBI, My66000::FDIVFrr);
    case My66000::FREMFrfc:	return CarryO(MBB, MBBI, My66000::FDIVFrf);
    case My66000::FREMFrkc:	return CarryO(MBB, MBBI, My66000::FDIVFrk);
    case My66000::FREMFfrc:	return CarryO(MBB, MBBI, My66000::FDIVFfr);
    case My66000::FREMFkrc:	return CarryO(MBB, MBBI, My66000::FDIVFkr);
    case My66000::SRL2rrbc:	return ShfIO(MBB, MBBI, My66000::SRLrr);
    case My66000::SLL2rrbc:	return ShfIO(MBB, MBBI, My66000::SLLrr);
    case My66000::SRA2rrbc:	return ShfIO(MBB, MBBI, My66000::SRArr);
    case My66000::SRL2ribc:	return ShfIO(MBB, MBBI, My66000::SRLri);
    case My66000::SLL2ribc:	return ShfIO(MBB, MBBI, My66000::SLLri);
    case My66000::SRA2ribc:	return ShfIO(MBB, MBBI, My66000::SRAri);
  }
}

bool My66000ExpandPseudo::ExpandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;
  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= ExpandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }
  return Modified;
}

bool My66000ExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
LLVM_DEBUG(dbgs() << "M66000ExpandPseudo\n");
  const My66000Subtarget *STI = &MF.getSubtarget<My66000Subtarget>();
  TII = STI->getInstrInfo();

  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= ExpandMBB(MBB);
  if (Modified) {
LLVM_DEBUG(dbgs() << "After ExpandPseudo\n");
    for (auto &MBB : MF ) {
      LLVM_DEBUG(dbgs() << MBB);
    }
  }
  return Modified;
}


FunctionPass *llvm::createMy66000ExpandPseudoPass() {
  return new My66000ExpandPseudo();
}
