//=- My66000VVMPass1.cpp - Try to make inner loops into VVM loops ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file This file implements a pass that examines inner most loops
/// and inserts VEC and LOOP instructions.
///
//===----------------------------------------------------------------------===//

#include "My66000.h"
#include "My66000MachineFunctionInfo.h"
#include "My66000TargetMachine.h"
#include "My66000Subtarget.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/InitializePasses.h"
#include <bitset>

using namespace llvm;

#define DEBUG_TYPE "VVM loop pass"
#define PASS_NAME "My66000 VVM Loop Conversion"

static cl::opt<unsigned> MaxVVMInstr("max-inst-vvm", cl::Hidden,
  cl::desc("Maximum number of instructions in VVM loop"), cl::init(16));

namespace {

class My66000VVMLoop: public MachineFunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid
  const My66000InstrInfo *TII;

  My66000VVMLoop() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override {
    return PASS_NAME;
  }

private:
  bool checkLoop(MachineBasicBlock *MBB);
  void calcLiveOuts(MachineBasicBlock *MBB, std::bitset<32> &Liveout);
  void findModified(MachineBasicBlock *MBB, std::bitset<32> &Modified);
};

} // end anonymous namespace

char &llvm::My66000VVMLoopID = My66000VVMLoop::ID;

char My66000VVMLoop::ID = 0;

INITIALIZE_PASS(My66000VVMLoop, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createMy66000VVMLoopPass() {
  return new My66000VVMLoop();
}

void My66000VVMLoop::findModified(MachineBasicBlock *MBB,
				  std::bitset<32> &Modified) {
  MachineBasicBlock::iterator I = MBB->begin();
  MachineBasicBlock::iterator E = MBB->end();
  std::bitset<32> Def, Kill;
  unsigned reg;

  Modified.reset();
  while (I != E) {
    MachineInstr *MI = &*I;
    Def.reset(); Kill.reset();
//LLVM_DEBUG(dbgs() << "  findModified= " << *MI);
    for (ConstMIBundleOperands O(*MI); O.isValid(); ++O) {
      if (O->isReg() && !O->isDebug()) {
	reg = O->getReg()-1;
	if (reg == 0) reg = 31;		// SP fixup
	else --reg;
	if (O->isDef()) {
	  Def[reg] = 1;
	} else if (O->isKill()) {
	  Kill[reg] = 1;
	}
      }
    }
//LLVM_DEBUG(dbgs() << "  kill=    " << Kill.to_string() << '\n');
//LLVM_DEBUG(dbgs() << "  def=     " << Def.to_string() << '\n');
    Modified &= ~Kill;
    Modified |= Def;
    ++I;
  }
}

// Find live outs by looking at live ins of successor blocks
void My66000VVMLoop::calcLiveOuts(MachineBasicBlock *MBB,
				  std::bitset<32> &Liveout) {
  std::bitset<32> Livein, Modified;

  findModified(MBB, Modified);
  Livein.reset();
  for (MachineBasicBlock::succ_iterator SI = MBB->succ_begin(),
       SE = MBB->succ_end(); SI != SE; ++SI) {
    if (*SI != MBB) {
      const MachineBasicBlock *SB = *SI;
      for (MachineBasicBlock::livein_iterator LI = SB->livein_begin(),
	   LE = SB->livein_end(); LI != LE; ++LI) {
	unsigned reg = LI->PhysReg-1;		// 0 is illegal
	if (reg == 0) reg = 31;		// SP fixup
	else --reg;
	Livein[reg] = 1;
      }
    }
  }
LLVM_DEBUG(dbgs() << "  modified= " << Modified.to_string() << '\n');
LLVM_DEBUG(dbgs() << "  live ins= " << Livein.to_string() << '\n');
  Liveout = Livein & Modified;
LLVM_DEBUG(dbgs() << "  live out= " << Liveout.to_string() << '\n');
}

static unsigned MapLoopCond(unsigned &cc) {
  switch (cc) {
  default:
    return false;  // Unsupported VVM loop condition code
  case MYCC::EQ0: cc = MYCB::EQ; break;
  case MYCC::NE0: cc = MYCB::NE; break;
  case MYCC::GE0: cc = MYCB::GE; break;
  case MYCC::LT0: cc = MYCB::LT; break;
  case MYCC::GT0: cc = MYCB::GT; break;
  case MYCC::LE0: cc = MYCB::LE; break;
  }
  return true;
}

// Can't deal with compares that negate an input
static bool isSimpleCompare(MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case My66000::CMPrr:
  case My66000::CMPri:
  case My66000::CMPrw:
  case My66000::CMPrd: return true;
  }
  return false;
}

static bool isSimpleAdd(MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case My66000::ADDrr:
  case My66000::ADDri:
  case My66000::ADDrw:
  case My66000::ADDrd: break;
  default: return false;
  }
  if ((MI.getOperand(0).getReg() == MI.getOperand(1).getReg()) ||
      (MI.getOperand(2).isReg() &&
       MI.getOperand(0).getReg() == MI.getOperand(2).getReg()))
    return true;
  return false;
}

bool My66000VVMLoop::checkLoop(MachineBasicBlock *TB) {
  LLVM_DEBUG(dbgs() << "checkLoop " << printMBBReference(*TB) << '\n');
  if (!TB->isSuccessor(TB))
    return false;
  LLVM_DEBUG(dbgs() << " found candidate inner loop\n");
  MachineBasicBlock::iterator I = TB->begin();
  MachineBasicBlock::iterator E = TB->getLastNonDebugInstr();
  if (I == E) {
    LLVM_DEBUG(dbgs() << " loop is infinite\n");
    return false;
  }
  MachineInstr *BrcMI,		// the conditional branch instruction
	       *BruMI = nullptr;// the ending uncoditional branch (if any)
//  MachineOperand &CmpOp = nullptr;	// the compare operand of interest
  MachineInstr *MI;
  Register BReg, LReg;
  unsigned BCnd;
  bool CondIsExit = false;
  bool HasBRC;
  MachineBasicBlock *EB = nullptr;	// the exit block if not fall-thru
  // Skip any optional terminating unconditional branch
  MI = &*E;
  if (MI->isUnconditionalBranch()) {
    EB = MI->getOperand(0).getMBB();
    if (EB == TB) CondIsExit = true;	// it is the loop branch
    LLVM_DEBUG(dbgs() << " skip unconditional branch to " << printMBBReference(*EB) << '\n');
    LLVM_DEBUG(dbgs() << " CondIsExit=" << CondIsExit << '\n');
    BruMI= MI;	// remember we need to delete this
    --E;
  }
  // Then we must have a conditional branch
  BrcMI = &*E;
  if (E->getOpcode() == My66000::BRIB) {
    LLVM_DEBUG(dbgs() << " found BRIB\n");
    HasBRC = false;
  } else if (E->getOpcode() == My66000::BRC) {
    LLVM_DEBUG(dbgs() << " found BRC\n");
    HasBRC = true;
  } else {
    LLVM_DEBUG(dbgs() << " fail - no conditional branch or bb1\n");
    return false;	// weird, not a conditional branch
  }
  // Make sure this conditional branch goes to top of the loop
  // or else its the exit from the loop followed by an
  // unconditional branch to the top.
  BReg = BrcMI->getOperand(1).getReg();
  BCnd = BrcMI->getOperand(2).getImm();
  if (HasBRC && !MapLoopCond(BCnd)) {
    LLVM_DEBUG(dbgs() << " fail - unsupported condition code\n");
    return false;
  }
  MachineBasicBlock *CB = BrcMI->getOperand(0).getMBB();
  if (CB != TB) {
    if (!CondIsExit) {
      LLVM_DEBUG(dbgs() << " fail - bad branch target\n");
      return false;
    } else {
      LLVM_DEBUG(dbgs() << " exit was conditional to " <<
			   printMBBReference(*CB) << '\n');
      if (HasBRC)
	BCnd = TII->reverseBRC(static_cast<MYCC::CondCodes>(BCnd));
      else
	BCnd = TII->reverseBRIB(static_cast<MYCB::CondBits>(BCnd));
      EB = CB;
    }
  }
  --E;
  // Now scan to top of loop looking for interesting stuff
  // FIXME - should count instructions, VVM has a limitation
  MachineInstr *DefMI = nullptr,// the def of branch condition BReg
	       *CmpMI = nullptr,// the compare instruction
	       *MovMI = nullptr,// intervening MOV
	       *AddMI = nullptr,// the add to loop counter instruction
	       *IncMI = nullptr;// an increment by 1 instruction
  unsigned NInstr = MaxVVMInstr;
  unsigned CmpOpNo;
  for (;;) {
    MachineInstr *MI = &*E;
    if (MI->isCall()) {
      LLVM_DEBUG(dbgs() << " fail - loop contains call\n");
      return false;	// calls not allowed in VVM
    }
    if (NInstr == 0) {
      LLVM_DEBUG(dbgs() << " fail - too many instructions in loop\n");
      return false;
    }
LLVM_DEBUG(dbgs() << " examine " << *MI);
    if (MI->getNumDefs() == 1 && MI->getOperand(0).isReg()) {
      if (MI->getOperand(0).getReg() == BReg && DefMI == nullptr) {
	DefMI = MI;
	if (HasBRC) {
	    if (isSimpleAdd(*MI)) {
	      LLVM_DEBUG(dbgs() << " def of BRC variable is add: " << *MI);
	      AddMI = MI;
	    }
	} else {
	    if (isSimpleCompare(*MI)) {
	      LLVM_DEBUG(dbgs() << " def of BRIB variable is compare: " << *MI);
	      CmpMI = MI;
	    }
	}
      } else if (CmpMI != nullptr && AddMI == nullptr) {
	// we have seen the compare but not its operands
	if (MI->getOperand(0).getReg() == CmpMI->getOperand(1).getReg()) {
	  LLVM_DEBUG(dbgs() << " def of compare variable op1: " << *MI);
	  CmpOpNo = 2;
	  if (isSimpleAdd(*MI))
	    AddMI = MI;
	} else if (CmpMI->getOperand(2).isReg() &&
		   MI->getOperand(0).getReg() == CmpMI->getOperand(2).getReg()) {
	  LLVM_DEBUG(dbgs() << " def of compare variable op2: " << *MI);
	  CmpOpNo = 1;
	  if (isSimpleAdd(*MI))
	    AddMI = MI;
        }
      } else if (DefMI == nullptr && MI->getOpcode() == My66000::MOVrr) {
	LLVM_DEBUG(dbgs() << " MOV between branch and operand\n");
	if (MovMI != nullptr) {
	  LLVM_DEBUG(dbgs() << " fail - Too many MOVs\n");
	  return false;
	}
	MovMI = MI;
      }
      if (MI->getOpcode() == My66000::ADDri) {
	if (MI->getOperand(1).isReg() &&
	    MI->getOperand(0).getReg() == MI->getOperand(1).getReg() &&
	    MI->getOperand(2).isImm() && MI->getOperand(2).getImm() == 1) {
	  LLVM_DEBUG(dbgs() << " found IncMI: " << *MI);
	  IncMI = MI;
	}
      }
    }
    --NInstr;
    if (E == I) break;
    --E;
  }
  if (CmpMI != nullptr) LLVM_DEBUG(dbgs() << " CmpMI= " << *CmpMI);
  if (MovMI != nullptr) LLVM_DEBUG(dbgs() << " MovMI= " << *MovMI);
  if (AddMI != nullptr) LLVM_DEBUG(dbgs() << " AddMI= " << *AddMI);
  if (IncMI != nullptr) LLVM_DEBUG(dbgs() << " IncMI= " << *IncMI);

  unsigned Type;
  if (HasBRC) {
    LReg = BReg;
    if (AddMI != nullptr) Type = 2;
    else Type = 3;
  } else {
    if (CmpMI == nullptr) {
      LLVM_DEBUG(dbgs() << " fail - BRIB has no compare\n");
      return false;
    }
    if (MovMI != nullptr &&
        MovMI->getOperand(0).getReg() == CmpMI->getOperand(1).getReg()) {
      // FIXME - we should be able to work around this, e.g insert a MOV
      LLVM_DEBUG(dbgs() << " fail - compare operand overwritten\n");
      return false;
    }
    if (AddMI != nullptr) {
      LReg = AddMI->getOperand(1).getReg();		// loop counter
      Type = 1;
    } else {
      if (IncMI != nullptr) {
	LReg = IncMI->getOperand(0).getReg();
	Type = 5;
      } else {
	LReg = CmpMI->getOperand(1).getReg();
	Type = 4;
      }
    }
  }
  LLVM_DEBUG(dbgs() << " will vectorize BCND=" << HasBRC << " type=" << Type << '\n');
  MachineFunction &MF = *TB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  std::bitset<32> Liveout;
  calcLiveOuts(TB, Liveout);
  Register RA;
  // Check for compare register destruction
  LLVM_DEBUG(dbgs() << " BrcMI= " << *BrcMI);
//LLVM_DEBUG(dbgs() << " branch reg isDead=" << BrcMI->getOperand(1).isDead());
//  if (CmpMI != nullptr && BrcMI->getOperand(1).isDead()) {
//    LLVM_DEBUG(dbgs() << " compare result is dead\n");
//    RA = BrcMI->getOperand(1).getReg();
//  } else {
    RegScavenger RS;
    RS.enterBasicBlockEnd(*TB);
    RA = RS.scavengeRegisterBackwards(My66000::GRegsRegClass, I,
					     false, 0, false);
    if (!RA) {
      LLVM_DEBUG(dbgs() << " giving up - no free register to allocate\n");
      return false;
    }
//  }
  // Create the VEC instruction
  BuildMI(*TB, I, I->getDebugLoc(), TII.get(My66000::VEC), RA)
	.addImm(Liveout.to_ulong() >> 1);

  MachineInstrBuilder LIB;
  DebugLoc DL = BrcMI->getDebugLoc();
  E = TB->getFirstTerminator();
  LLVM_DEBUG(dbgs() << " Type=" << Type << '\n');
  unsigned Opc;
  switch (Type) {
  case 1: {	// Have CmpMI and AddMI
    MachineOperand *CmpOp = &CmpMI->getOperand(CmpOpNo);
    if (CmpOp->isReg()) {
      if (AddMI->getOperand(2).isReg()) {
	Opc = My66000::LOOP1rr;
	LLVM_DEBUG(dbgs() << " type1rr\n");
      } else {
	Opc = My66000::LOOP1ir;
	LLVM_DEBUG(dbgs() << " type1ir\n");
      }
    } else {
      if (AddMI->getOperand(2).isReg()) {
	Opc = My66000::LOOP1ri;
	LLVM_DEBUG(dbgs() << " type1ri\n");
      } else {
	Opc = My66000::LOOP1ii;
	LLVM_DEBUG(dbgs() << " type1ii\n");
      }
    }
    LIB = BuildMI(*TB, E, DL, TII.get(Opc), LReg)
	  .addImm(BCnd)
	  .addReg(LReg)
	  .add(AddMI->getOperand(2))
	  .add(*CmpOp);
   break;
  }
  case 2: {	// No CmpMI but have AddMI
    if (AddMI->getOperand(2).isReg()) {
      LLVM_DEBUG(dbgs() << " type1rz\n");
      Opc = My66000::LOOP1ri;
    } else {
      LLVM_DEBUG(dbgs() << " type1iz\n");
      Opc = My66000::LOOP1ii;
    }
    LIB = BuildMI(*TB, E, DL, TII.get(Opc), LReg)
	  .addImm(BCnd)
	  .addReg(LReg)
	  .add(AddMI->getOperand(2))
	  .addImm(0);
    break;
  }
  case 3: {	// No CmpMI and no AddMI
    LLVM_DEBUG(dbgs() << " type1zz\n");
    LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP1ii), LReg)
	  .addImm(BCnd)
	  .addReg(LReg)
	  .addImm(0)
	  .addImm(0);
    break;
  }
  case 4: {	// Have CmpMI and No AddMI
    if (CmpMI->getOperand(2).isReg()) {
      LLVM_DEBUG(dbgs() << " type1zr\n");
      Opc = My66000::LOOP1ir;
    } else {
      LLVM_DEBUG(dbgs() << " type1zi\n");
      Opc = My66000::LOOP1ii;
    }
    LIB = BuildMI(*TB, E, DL, TII.get(Opc), LReg)
	  .addImm(BCnd)
	  .addReg(LReg)
	  .addImm(0)
	  .add(CmpMI->getOperand(2));
    break;
  }
  case 5: {	// Have CmpMI and IncMI but NO AddMI
      if (CmpMI->getOperand(2).isReg()) {
	LLVM_DEBUG(dbgs() << " type3rr\n");
	Opc = My66000::LOOP3rr;
      } else {
	LLVM_DEBUG(dbgs() << " type3ri\n");
	Opc = My66000::LOOP3ri;
      }
      LIB = BuildMI(*TB, E, DL, TII.get(Opc), LReg)
	    .addImm(BCnd)
	    .addReg(LReg)
	    .add(CmpMI->getOperand(1))
	    .add(CmpMI->getOperand(2));
      IncMI->eraseFromParent();
      break;
    }
  }
  LIB.addReg(RA);
  LIB.addMBB(TB);
  if (EB != nullptr && !TB->isLayoutSuccessor(EB)) {	// not a fall-thru
    BuildMI(*TB, E, DL, TII.get(My66000::BRU)).addMBB(EB);
    LLVM_DEBUG(dbgs() << " need terminating BRU\n");
  }
  // If there was and unconditional branch get rid of it
  if (BruMI != nullptr)
    BruMI->eraseFromParent();
  // The conditional branch is not longer needed
  BrcMI->eraseFromParent();
  if (AddMI != nullptr)
    AddMI->eraseFromParent();	// Is this safe?
  if (CmpMI != nullptr) {
    if (Liveout.test(CmpMI->getOperand(0).getReg()-2))
      LLVM_DEBUG(dbgs() << " CmpMI result is live out:" << *CmpMI);
    else
      CmpMI->eraseFromParent();
  }
  LLVM_DEBUG(dbgs() << "*** Modified basic block ***\n");
  LLVM_DEBUG(dbgs() << *TB);
  return true;
}

bool My66000VVMLoop::runOnMachineFunction(MachineFunction &MF) {
  TII = MF.getSubtarget<My66000Subtarget>().getInstrInfo();
  if (!MF.getSubtarget<My66000Subtarget>().useVVM()) return false;
  bool Changed = false;
LLVM_DEBUG(dbgs() << "VVMLoopPass: " << MF.getName() << '\n');
  for (auto &MBB : MF ) {
    Changed |= checkLoop(&MBB);
  }
  return Changed;
}
