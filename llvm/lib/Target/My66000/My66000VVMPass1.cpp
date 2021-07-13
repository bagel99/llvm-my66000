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

static cl::opt<bool> EnableVVM("enable-vvm", cl::Hidden,
  cl::desc("Enable VVM Loop Mode"));

namespace {

class My66000VVMLoop: public MachineFunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid

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


static unsigned MapLoopCond(unsigned cc) {
  switch (cc) {
  default:
    llvm_unreachable("Unrecognized VVM loop condition code");
  case MYCC::EQ0: return MYCB::EQ;   case MYCC::NE0: return MYCB::NE;
  case MYCC::GE0: return MYCB::GE;   case MYCC::LT0: return MYCB::LT;
  case MYCC::GT0: return MYCB::GT;   case MYCC::LE0: return MYCB::LE;
  }
}

bool My66000VVMLoop::checkLoop(MachineLoop *Loop) {
MachineBasicBlock *TB, *BB, *CB;
    TB = Loop->getTopBlock();
    BB = Loop->getBottomBlock();
    if (TB != BB)
      return false;	// For now, only single block loops
    CB = Loop->findLoopControlBlock();
    if (!CB || CB != TB)
      return false;

dbgs() << " found candidate inner loop " <<printMBBReference(*TB) << '\n';
  MachineBasicBlock::iterator I = TB->begin();
  MachineBasicBlock::iterator E = TB->getFirstTerminator();
  MachineInstr *BMI, *CMI = nullptr, *AMI = nullptr;
  Register BReg, LReg, CReg;
  unsigned BCnd;
  unsigned Type;
  // Skip any optional terminating unconditional branch
  if (E->isUnconditionalBranch())
  {
    --E;
dbgs() << " skip unconditional branch\n";
  }
  // Then we must have a conditional branch
  BMI = &*E;
  if (E->getOpcode() == My66000::BRIB) {
dbgs() << " found BRIB\n";
    Type = 1;
  } else if (E->getOpcode() == My66000::BRC) {
dbgs() << " found BRC\n";
    Type = 2;
  } else
    return false;	// weird, not a conditional branch
  // Make sure this conditional branch goes to top of this BB
  if (BMI->getOperand(0).getMBB() != TB) {
dbgs() << " bad branch target\n";
    return false;
  }
dbgs() << *BMI;
  BReg = BMI->getOperand(1).getReg();
  BCnd = BMI->getOperand(2).getImm();
  --E;
  // Now scan to top of loop looking for interesting stuff
  // FIXME - should count instructions, VVM has a limitation
  while (E != I) {
    MachineInstr *MI = &*E;
//dbgs() << *MI;
    if (MI->isCall())
      return false;	// calls not allowed in vector mode
    if (MI->getNumDefs() == 1 && MI->getOperand(0).isReg()) {
      if (MI->getOperand(0).getReg() == BReg) {
dbgs() << *MI;
	if (MI->isCompare()) {
dbgs() << " found def of branch variable is compare\n";
	  CReg = MI->getOperand(1).getReg();
	  CMI = MI;
	} else {
dbgs() << " found def of branch variable is not compare\n";
	  AMI = MI;
	}
      }
      else if (CMI != nullptr && MI->getOperand(0).getReg() == CReg) {
dbgs() << *MI;
dbgs() << " found def of compare variable\n";
	// FIXME - should be and ADD
	AMI = MI;
      }
    }
    --E;
  }
  // AMI must be an ADD instruction if incorporated into LOOP
  if (AMI != nullptr &&
      AMI->getOpcode() != My66000::ADDrr &&
      AMI->getOpcode() != My66000::ADDri)
    AMI = nullptr;
  // Type 1 requires both CMI and AMI
  if (Type == 1 && (CMI == nullptr || AMI == nullptr))
    return false;

dbgs() << " will vectorize this block:\n" << *TB;

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
  if (Type == 1) {
    LReg = AMI->getOperand(1).getReg();		// loop counter
    if (CMI->getOperand(2).isReg()) {
      if (AMI->getOperand(2).isReg()) {
	Opc = My66000::LOOP1rr;
      } else {
	Opc = My66000::LOOP1ri;
      }
    } else {
      if (AMI->getOperand(2).isReg()) {
	Opc = My66000::LOOP1ir;
      } else {
	Opc = My66000::LOOP1ii;
      }
    }
    LIB = BuildMI(*TB, E, DL, TII.get(Opc))
	    .addImm(BCnd)
	    .addReg(LReg)
	    .add(CMI->getOperand(2))
	    .add(AMI->getOperand(2));

  } else {	// Type 2
    BCnd = MapLoopCond(BCnd);
    if (AMI == nullptr) {	// no increment
      LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP1ii))
	      .addImm(BCnd)
	      .addReg(BReg)
	      .addImm(0)
	      .addImm(0);
    } else {
      if (AMI->getOperand(2).isReg()) {
	Opc = My66000::LOOP1ri;
      } else {
	Opc = My66000::LOOP1ii;
      }
      LIB = BuildMI(*TB, E, DL, TII.get(Opc))
	      .addImm(BCnd)
	      .addReg(BReg)
	      .addImm(0)
	      .add(AMI->getOperand(2));
    }
  }
  LIB.addReg(RA);
  LIB.addMBB(TB);
  BMI->eraseFromParent();
  if (CMI != nullptr)
    CMI->eraseFromParent();
  if (AMI != nullptr)
    AMI->eraseFromParent();
  return true;
}

bool My66000VVMLoop::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;

  if (!EnableVVM) return false;
dbgs() << "VVMLoopPass: " << MF.getName() << '\n';
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

