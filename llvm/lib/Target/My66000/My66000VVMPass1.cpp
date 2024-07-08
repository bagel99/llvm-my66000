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
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "VVM loop pass"
#define PASS_NAME "My66000 VVM Loop Analysis"

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

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    MachineFunctionPass::getAnalysisUsage(AU);
    AU.addRequired<MachineLoopInfo>();
  }
private:
  bool checkLoop(MachineLoop *Loop);
};

} // end anonymous namespace

char &llvm::My66000VVMLoopID = My66000VVMLoop::ID;

char My66000VVMLoop::ID = 0;

INITIALIZE_PASS_BEGIN(My66000VVMLoop, DEBUG_TYPE, PASS_NAME, false, false)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
INITIALIZE_PASS_END(My66000VVMLoop, DEBUG_TYPE, PASS_NAME, false, false)


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

bool My66000VVMLoop::checkLoop(MachineLoop *Loop) {
  LLVM_DEBUG(dbgs() << "checkLoop\n");
//  Loop->dump();
  MachineBasicBlock *TB = Loop->getTopBlock();	// the loop block
  MachineBasicBlock *CB = Loop->findLoopControlBlock();
  if (!CB || CB != TB)
    return false;
  MachineBasicBlock *BB = Loop->getBottomBlock();
  if (TB != BB)
    return false;	// For now, only single block loops
  LLVM_DEBUG(dbgs() << " found candidate inner loop " << printMBBReference(*TB) << '\n');
  MachineBasicBlock::iterator I = TB->begin();
  MachineBasicBlock::iterator E = TB->getLastNonDebugInstr();
  MachineInstr *BMI, *CMI = nullptr, *AMI = nullptr, *UMI = nullptr;
  MachineInstr *MI;
  Register BReg, LReg;
  unsigned BCnd;
  unsigned Type;
  unsigned CmpOpNo;
  MachineBasicBlock *EB = nullptr;	// the exit block if not fall-thru
  bool CondIsExit = false;
  // Skip any optional terminating unconditional branch
  MI = &*E;
  if (MI->isUnconditionalBranch()) {
    EB = MI->getOperand(0).getMBB();
    if (EB == TB) CondIsExit = true;	// it is the loop branch
    LLVM_DEBUG(dbgs() << " skip unconditional branch to " << printMBBReference(*EB) << '\n');
    LLVM_DEBUG(dbgs() << " CondIsExit=" << CondIsExit << '\n');
    UMI = MI;	// remember we need to delete this
    --E;
  }
  // Then we must have a conditional branch
  BMI = &*E;
  if (E->getOpcode() == My66000::BRIB) {
    LLVM_DEBUG(dbgs() << " found BRIB\n");
    Type = 1;
  } else if (E->getOpcode() == My66000::BRC) {
    LLVM_DEBUG(dbgs() << " found BRC\n");
    Type = 2;
  } else {
    LLVM_DEBUG(dbgs() << " fail - no conditional branch\n");
    return false;	// weird, not a conditional branch
  }
  // Make sure this conditional branch goes to top of the loop
  // or else its the exit from the loop followed by an
  // unconditional branch to the top.
  BReg = BMI->getOperand(1).getReg();
  BCnd = BMI->getOperand(2).getImm();
  CB = BMI->getOperand(0).getMBB();
  if (CB != TB) {
    if (!CondIsExit) {
      LLVM_DEBUG(dbgs() << " fail - bad branch target\n");
      return false;
    } else {
      LLVM_DEBUG(dbgs() << " exit was conditional to " << printMBBReference(*CB) << '\n');
      if (Type == 1)
	BCnd = TII->reverseBRIB(static_cast<MYCB::CondBits>(BCnd));
      else
	BCnd = TII->reverseBRC(static_cast<MYCC::CondCodes>(BCnd));
      EB = CB;
    }
  }
  --E;
  // Now scan to top of loop looking for interesting stuff
  // FIXME - should count instructions, VVM has a limitation
  unsigned NInstr = MaxVVMInstr;
  while (E != I) {
    MachineInstr *MI = &*E;
    if (MI->isCall()) {
      LLVM_DEBUG(dbgs() << " fail - loop contains call\n");
      return false;	// calls not allowed in vector mode
    }
    if (NInstr == 0) {
      LLVM_DEBUG(dbgs() << " fail - too many instructions in loop\n");
      return false;
    }
    if (MI->getNumDefs() == 1 && MI->getOperand(0).isReg()) {
      if (MI->getOperand(0).getReg() == BReg) {
	if (MI->isCompare()) {
	  LLVM_DEBUG(dbgs() << " def of branch variable is compare: " << *MI);
	  CMI = MI;
	} else {
	  LLVM_DEBUG(dbgs() << " def of branch variable is not compare: " << *MI);
	  AMI = MI;
	}
      }
      else if (CMI != nullptr) {	// we have seen the compare
	if (MI->getOperand(0).getReg() == CMI->getOperand(1).getReg()) {
	  LLVM_DEBUG(dbgs() << " def of compare variable op1: " << *MI);
	  CmpOpNo = 2;
	  AMI = MI;
	} else if (CMI->getOperand(2).isReg() &&
		   MI->getOperand(0).getReg() == CMI->getOperand(2).getReg()) {
	  LLVM_DEBUG(dbgs() << " def of compare variable op2: " << *MI);
	  CmpOpNo = 1;
	  AMI = MI;
        }
      }
    }
    --NInstr;
    --E;
  }
  if (AMI != nullptr) {
    if (AMI->getOpcode() != My66000::ADDrr &&
        AMI->getOpcode() != My66000::ADDri) {
      AMI = nullptr; // AMI must be an ADD instruction if incorporated into LOOP
    } else {	// We don't handle other than increment version of ADD
      if (!((AMI->getOperand(0).getReg() == AMI->getOperand(1).getReg()) ||
	    (AMI->getOperand(2).isReg() &&
	     AMI->getOperand(0).getReg() == AMI->getOperand(2).getReg()))) {
	LLVM_DEBUG(dbgs() << " fail - ADD is not a simple increment\n");
	return false;
      }
    }
  }
  // Type 1 requires both CMI and AMI
  if (Type == 1) {
    if (CMI == nullptr) {
      LLVM_DEBUG(dbgs() << " fail - type 1 has CMI == null\n");
      return false;
    }
    if (AMI == nullptr) {
      Type = 3;
    }
  }
  else if (Type == 2) {
    if (!MapLoopCond(BCnd)) {
      LLVM_DEBUG(dbgs() << " unsupported condition\n");
      return false;
    }
  }
  LLVM_DEBUG(dbgs() << " will vectorize this block:\n");
  MachineFunction &MF = *TB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  // Create the VEC instruction
  unsigned RA = MRI.createVirtualRegister(&My66000::GRegsRegClass);
  BuildMI(*TB, I, I->getDebugLoc(), TII.get(My66000::VEC), RA)
	.addImm(0);	// pass2 will fill this in after reg allocation

  MachineInstrBuilder LIB;
  DebugLoc DL = BMI->getDebugLoc();
  E = TB->getFirstTerminator();
  unsigned Opc;
  switch (Type) {
  case 1: {
    LReg = AMI->getOperand(1).getReg();		// loop counter
    if (CMI->getOperand(CmpOpNo).isReg()) {
      if (AMI->getOperand(2).isReg()) {
	Opc = My66000::LOOP1rr;
	LLVM_DEBUG(dbgs() << " type1rr\n");
      } else {
	Opc = My66000::LOOP1ri;
	LLVM_DEBUG(dbgs() << " type1ri\n");
      }
    } else {
      if (AMI->getOperand(2).isReg()) {
	Opc = My66000::LOOP1ir;
	LLVM_DEBUG(dbgs() << " type1ir\n");
      } else {
	Opc = My66000::LOOP1ii;
	LLVM_DEBUG(dbgs() << " type1ii\n");
      }
    }
    LIB = BuildMI(*TB, E, DL, TII.get(Opc))
	    .addImm(BCnd)
	    .addReg(LReg)
	    .add(CMI->getOperand(CmpOpNo))
	    .add(AMI->getOperand(2));
    break;
  }
  case 2: {
    if (AMI == nullptr) {	// no increment use LOOP1
      LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP1ii))
	      .addImm(BCnd)
	      .addReg(BReg)
	      .addImm(0)
	      .addImm(0);
      LLVM_DEBUG(dbgs() << " type100\n");
    } else {
      if (AMI->getOperand(2).isReg()) {
	Opc = My66000::LOOP2ri;
	LLVM_DEBUG(dbgs() << " type2ri\n");
      } else {
	Opc = My66000::LOOP2ii;
	LLVM_DEBUG(dbgs() << " type2ii\n");
      }
      LIB = BuildMI(*TB, E, DL, TII.get(Opc))
	      .addImm(BCnd)
	      .addReg(BReg)
	      .addImm(0)
	      .add(AMI->getOperand(2));
    }
    break;
   }
   case 3: {
      if (CMI->getOperand(2).isReg()) {
	Opc = My66000::LOOP3ri;
	LLVM_DEBUG(dbgs() << " type3ri\n");
      } else {
	Opc = My66000::LOOP3ii;
	LLVM_DEBUG(dbgs() << " type3ii\n");
      }
      LIB = BuildMI(*TB, E, DL, TII.get(Opc))
	      .addImm(BCnd)
	      .add(CMI->getOperand(1))
	      .add(CMI->getOperand(2))
	      .addImm(0);
    }
  }
  LIB.addReg(RA);
  LIB.addMBB(TB);
  if (EB != nullptr && !TB->isLayoutSuccessor(EB)) {	// not a fall-thru
    BuildMI(*TB, E, DL, TII.get(My66000::BRU)).addMBB(EB);
    LLVM_DEBUG(dbgs() << " need terminating BRU\n");
  }
  if (UMI != nullptr)
    UMI->eraseFromParent();
  BMI->eraseFromParent();
  if (CMI != nullptr)
    CMI->eraseFromParent();
  if (AMI != nullptr)
    AMI->eraseFromParent();
  return true;
}

bool My66000VVMLoop::runOnMachineFunction(MachineFunction &MF) {
  TII = MF.getSubtarget<My66000Subtarget>().getInstrInfo();
  bool Changed = false;

  if (!MF.getSubtarget<My66000Subtarget>().useVVM()) return false;
LLVM_DEBUG(dbgs() << "VVMLoopPass: " << MF.getName() << '\n');
  MachineLoopInfo &MLI = getAnalysis<MachineLoopInfo>();
  SmallVector<MachineLoop *, 4> Loops(MLI.begin(), MLI.end());
  for (int i = 0; i < (int)Loops.size(); ++i)
    for (MachineLoop *Child : Loops[i]->getSubLoops())
      Loops.push_back(Child);
  for (MachineLoop *CurrLoop : Loops) {
    if (!CurrLoop->getSubLoops().empty())
      continue;
    Changed = checkLoop(CurrLoop);
  }

  return Changed;
}

FunctionPass *llvm::createMy66000VVMLoopPass() {
  return new My66000VVMLoop();
}

