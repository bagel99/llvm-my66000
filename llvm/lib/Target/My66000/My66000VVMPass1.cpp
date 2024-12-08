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
  MachineInstr *BrcMI,		// the conditional branch instruction
	       *BruMI = nullptr,// the ending uncoditional branch (if any)
	       *CmpMI = nullptr,// the compare instruction
	       *AddMI = nullptr,// the add to loop counter instruction
	       *IncMI = nullptr,// an increment by 1 instruction
	       *CpyMI = nullptr;// an intervening copy instruction (if any)
//  MachineOperand &CmpOp = nullptr;	// the compare operand of interest
  MachineInstr *MI;
  Register BReg, LReg;
  unsigned BCnd;
  unsigned Type;
  unsigned CmpOpNo;
  MachineBasicBlock *EB = nullptr;	// the exit block if not fall-thru
  bool CondIsExit = false;
  bool HasBRC;
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
    LLVM_DEBUG(dbgs() << " fail - no conditional branch\n");
    return false;	// weird, not a conditional branch
  }
  // Make sure this conditional branch goes to top of the loop
  // or else its the exit from the loop followed by an
  // unconditional branch to the top.
  BReg = BrcMI->getOperand(1).getReg();
  BCnd = BrcMI->getOperand(2).getImm();
  CB = BrcMI->getOperand(0).getMBB();
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
  unsigned NInstr = MaxVVMInstr;
  for (;;) {
    MachineInstr *MI = &*E;
    if (MI->isCall()) {
      LLVM_DEBUG(dbgs() << " fail - loop contains call\n");
      return false;	// calls not allowed in vector mode
    }
    if (MI->isCopy()) {
      LLVM_DEBUG(dbgs() << " warn - loop contains copy\n");
      if (CpyMI != nullptr)
      { LLVM_DEBUG(dbgs() << " fail - loop contains more than one copy\n");
        return false;	// we don't handle ths
      }
      CpyMI = MI;
    }
    if (NInstr == 0) {
      LLVM_DEBUG(dbgs() << " fail - too many instructions in loop\n");
      return false;
    }
    if (MI->getNumDefs() == 1 && MI->getOperand(0).isReg()) {
      if (MI->getOperand(0).getReg() == BReg) {
	if (MI->isCompare()) {
	  LLVM_DEBUG(dbgs() << " def of branch variable is compare: " << *MI);
	  CmpMI = MI;
	} else {
	  LLVM_DEBUG(dbgs() << " def of branch variable is not compare: " << *MI);
	  AddMI = MI;
	}
      } else if (CmpMI != nullptr) {	// we have seen the compare
	if (MI->getOperand(0).getReg() == CmpMI->getOperand(1).getReg()) {
	  LLVM_DEBUG(dbgs() << " def of compare variable op1: " << *MI);
	  CmpOpNo = 2;
	  AddMI = MI;
	} else if (CmpMI->getOperand(2).isReg() &&
		   MI->getOperand(0).getReg() == CmpMI->getOperand(2).getReg()) {
	  LLVM_DEBUG(dbgs() << " def of compare variable op2: " << *MI);
	  CmpOpNo = 1;
	  AddMI = MI;
        }
      }
      if (MI->getOpcode() == My66000::ADDri) {
	LLVM_DEBUG(dbgs() << " found ADDri: " << *MI);
	if (MI->getOperand(0).getReg() == MI->getOperand(1).getReg() &&
	    MI->getOperand(2).isImm() && MI->getOperand(2).getImm() == 1) {
	  IncMI = MI;
	}
      }
    }
    --NInstr;
    if (E == I) break;
    --E;
  }
  if (AddMI != nullptr) {
    if (AddMI->getOpcode() != My66000::ADDrr &&
        AddMI->getOpcode() != My66000::ADDri) {
      AddMI = nullptr; // AddMI must be an ADD instruction if incorporated into LOOP
    } else {	// We don't handle other than increment version of ADD
      if (!((AddMI->getOperand(0).getReg() == AddMI->getOperand(1).getReg()) ||
	    (AddMI->getOperand(2).isReg() &&
	     AddMI->getOperand(0).getReg() == AddMI->getOperand(2).getReg()))) {
	LLVM_DEBUG(dbgs() << " fail - ADD is not a simple increment\n");
	return false;
      }
    }
  }
  else {
    // We did not find an increment, so assume we are testing the leftmost
    // operand of the compare.
    // Can this be wrong? If the rightmost operand is a constant, then
    // we correct, but...
    CmpOpNo = 1;
  }
  if (AddMI == nullptr) LLVM_DEBUG(dbgs() << " AddMI= nullptr\n");
  else LLVM_DEBUG(dbgs() << " AddMI= " << *AddMI);
  if (CmpMI == nullptr) LLVM_DEBUG(dbgs() << " CmpMI= nullptr\n");
  else LLVM_DEBUG(dbgs() << " CmpMI= " << *CmpMI);
  if (IncMI != nullptr) LLVM_DEBUG(dbgs() << " IncMI= " << *IncMI);
  if (CpyMI != nullptr) LLVM_DEBUG(dbgs() << " CpyMI= " << *CpyMI);

  if (HasBRC) {
    if (!MapLoopCond(BCnd)) {
      LLVM_DEBUG(dbgs() << " fail - unsupported condition\n");
      return false;
    }
    Type = 2;
  }
  else {
    if (CmpMI == nullptr) {
      LLVM_DEBUG(dbgs() << " fail - BRIB has no compare\n");
      return false;
    }
    if (AddMI != nullptr) Type = 1;
    else if (IncMI != nullptr) Type = 4;
    else Type = 3;
  }
  LLVM_DEBUG(dbgs() << " will vectorize this block:\n");
  // Check for compare register destruction
  MachineFunction &MF = *TB->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
//  MachineOperand &CmpOp = CmpMI->getOperand(CmpOpNo);
  bool HasCopy = false;
  Register RC;
  MachineInstr *SavMI = nullptr;
  if (CpyMI != nullptr && CmpMI != nullptr) {
    LLVM_DEBUG(dbgs() << " CmpOpNo= " << CmpOpNo << '\n');
    if (CmpMI->getOperand(CmpOpNo).isReg()) {
      Register CmpReg = CmpMI->getOperand(CmpOpNo).getReg();
      Register CpyReg = CpyMI->getOperand(0).getReg();;
      if (CmpReg == CpyReg) {
	LLVM_DEBUG(dbgs() << " warn - compare input register overwritten\n");
	RC = MRI.createVirtualRegister(&My66000::GRegsRegClass);
	SavMI = BuildMI(*TB, CmpMI, CmpMI->getDebugLoc(),
		TII.get(TargetOpcode::COPY), RC)
	    .addReg(CpyReg);
	HasCopy = true;
//	CmpOp = SIB.getOperand(0);
//	return false;
      }
    }
  }
  // Create the VEC instruction
  Register RA = MRI.createVirtualRegister(&My66000::GRegsRegClass);
  BuildMI(*TB, I, I->getDebugLoc(), TII.get(My66000::VEC), RA)
	.addImm(0);	// pass2 will fill this in after reg allocation

  MachineInstrBuilder LIB;
  DebugLoc DL = BrcMI->getDebugLoc();
  E = TB->getFirstTerminator();
  unsigned Opc;
  switch (Type) {
  case 1: {	// Have CmpMI and AddMI
    LReg = AddMI->getOperand(1).getReg();		// loop counter
    MachineOperand &CmpOp = (HasCopy) ?
	SavMI->getOperand(0) :
	CmpMI->getOperand(CmpOpNo);
    if (CmpOp.isReg()) {
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
    LIB = BuildMI(*TB, E, DL, TII.get(Opc))
	    .addImm(BCnd)
	    .addReg(LReg)
	    .add(AddMI->getOperand(2))
	    .add(CmpOp);
   break;
  }
  case 2: {	// No CmpMI and possibly AddMI
    if (AddMI == nullptr) {
      LLVM_DEBUG(dbgs() << " type100\n");
      LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP1ii))
	    .addImm(BCnd)
	    .addReg(BReg)
	    .addImm(0)
	    .addImm(0);

    } else if (AddMI->getOperand(2).isReg()) {
      LLVM_DEBUG(dbgs() << " type10r\n");
      LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP1ri))
	    .addImm(BCnd)
	    .addReg(BReg)
	    .addReg(AddMI->getOperand(2).getReg())
	    .addImm(0);
    } else {
      LLVM_DEBUG(dbgs() << " type10i\n");
      LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP1ii))
	    .addImm(BCnd)
	    .addReg(BReg)
	    .add(AddMI->getOperand(2))
	    .addImm(0);
    }
    break;
  }
  case 3: {	// Have CmpMI and NO AddMI
    if (CmpMI->getOperand(2).isReg()) {
    LLVM_DEBUG(dbgs() << " type1r0\n");
    LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP1ir))
	  .addImm(BCnd)
	  .add(CmpMI->getOperand(1))
	  .addImm(0)
	  .add(CmpMI->getOperand(2));
    } else {
    LLVM_DEBUG(dbgs() << " type1i0\n");
    LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP1ii))
	  .addImm(BCnd)
	  .add(CmpMI->getOperand(1))
	  .addImm(0)
	  .add(CmpMI->getOperand(2));
    }
    break;
  }
  case 4: {	// Have CmpMI and IncMI but NO AddMI
      if (CmpMI->getOperand(2).isReg()) {
	LLVM_DEBUG(dbgs() << " type3rr\n");
	LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP3rr))
	      .addImm(BCnd)
	      .addReg(IncMI->getOperand(0).getReg())
	      .add(CmpMI->getOperand(1))
	      .add(CmpMI->getOperand(2));
      } else {
	LLVM_DEBUG(dbgs() << " type3ri\n");
	LIB = BuildMI(*TB, E, DL, TII.get(My66000::LOOP3ri))
	      .addImm(BCnd)
	      .addReg(IncMI->getOperand(0).getReg())
	      .add(CmpMI->getOperand(1))
	      .add(CmpMI->getOperand(2));
      }
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
  // CmpMI may also be dead
  // It will be removed by a subsequent DeadMachineInstructionElim pass
  LLVM_DEBUG(dbgs() << "*** Modified basic block ***\n");
  LLVM_DEBUG(dbgs() << *TB);
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

