//===-- My66000PredicatePass.cpp - Transform to Predicated Code -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "My66000.h"
#include "My66000MachineFunctionInfo.h"
#include "My66000Subtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineInstrBundle.h"
#include "llvm/CodeGen/MachineOperand.h"

using namespace llvm;

#define DEBUG_TYPE "my66000-predicate"
#define PASS_NAME "My66000 predicate transform pass"

static cl::opt<bool> EnablePred("enable-predication", cl::Hidden,
  cl::desc("Enable predication instructions"));
static cl::opt<bool> EnablePred2("enable-predication2", cl::Hidden,
  cl::desc("Enable double predication instructions"));

STATISTIC(NumPREDs,        "Number of single predicated blocks inserted");
STATISTIC(NumPRED2s,       "Number of double predicated blocks inserted");


namespace {
  class My66000PredBlock : public MachineFunctionPass {
  public:
    static char ID;
    const My66000InstrInfo *TII;

    My66000PredBlock() : MachineFunctionPass(ID) {}

    bool runOnMachineFunction(MachineFunction &MF) override;

    StringRef getPassName() const override {
      return PASS_NAME;
    }
  private:
    bool InsertPredInstructions(MachineBasicBlock *MBB);
    bool onePass(MachineFunction &MF);
    bool ExamineBranch(MachineBasicBlock *Head,
		MachineBasicBlock *&TBB, MachineBasicBlock *&FBB,
		SmallVector<MachineOperand, 4> &Cond);
    void getConditionInfo(SmallVector<MachineOperand, 4> &Cond,
			bool invert, unsigned &op, unsigned &cc, unsigned &reg);
    int checkBlock(MachineBasicBlock *MBB);
    bool Convert(MachineBasicBlock *Head,
		 MachineBasicBlock *Succ0, MachineBasicBlock *Succ1,
		 MachineBasicBlock *Tail);
    bool ConvertT2(MachineBasicBlock *Head0, MachineBasicBlock *Head1,
		 MachineBasicBlock *Succ0, MachineBasicBlock *Succ1,
		 MachineBasicBlock *Tail);
    bool ConvertD2(MachineBasicBlock *Head0, MachineBasicBlock *Head1,
		 MachineBasicBlock *Succ0, MachineBasicBlock *Succ1,
		 MachineBasicBlock *Tail);
  };

} // end anonymous namespace

char My66000PredBlock::ID = 0;
char &llvm::My66000PredBlockID = My66000PredBlock::ID;

INITIALIZE_PASS(My66000PredBlock, DEBUG_TYPE, PASS_NAME, false, false)

// Returns true if branch can be changed to a predicate
bool My66000PredBlock::ExamineBranch(MachineBasicBlock *Head,
		MachineBasicBlock *&TBB, MachineBasicBlock *&FBB,
		SmallVector<MachineOperand, 4> &Cond) {

  Cond.clear();
  TBB = nullptr;
  FBB = nullptr;
  if (TII->analyzeBranch(*Head, TBB, FBB, Cond, false)) {
LLVM_DEBUG(dbgs() << "Branch not analyzable.\n");
    return false;
  }
  if (!TBB) { // This is weird, probably some sort of degenerate CFG.
LLVM_DEBUG(dbgs() << "AnalyzeBranch didn't find conditional branch.\n");
    return false;
  }
  // Make sure the analyzed branch is conditional; one of the successors
  // could be a landing pad. (Empty landing pads can be generated on Windows.)
  if (Cond.empty()) {
LLVM_DEBUG(dbgs() << "AnalyzeBranch found an unconditional branch.\n");
    return false;
  }
  if (FBB) {
    if (FBB == Head->getFallThrough()) {
LLVM_DEBUG(dbgs() << "\tcond/uncond branch pair, uncond branch to fallthru\n");
    MachineBasicBlock::iterator I = Head->getLastNonDebugInstr();
    I->eraseFromParent();		// Remove the branch.
    }
  } else {
    FBB = Head->getFallThrough();
  }
  return true;
}

// Count how many valid instructions can be predicated.
// If theres a call that is not at the end, return 0.
// Any branch should be at the end.
int My66000PredBlock::checkBlock(MachineBasicBlock *MBB) {
  MachineBasicBlock::iterator I = MBB->begin();
  MachineBasicBlock::iterator E = MBB->getFirstTerminator();
  unsigned NumInstrs = 0;

  while (I != E) {
    MachineInstr *MI = &*I;
    // FIXME - why are CFI_INSTRUCTIONs in the code?
    // answer: because of tail merged RETs
    if (!MI->isCFIInstruction()) {
      if (MI->isCall() && I != E) return -1;
      NumInstrs += 1;
    }
    ++I;
  }
  return NumInstrs;
}

// Compute the predicate info from the branch info
void My66000PredBlock::getConditionInfo(SmallVector<MachineOperand, 4> &Cond,
			bool invert, unsigned &op, unsigned &cc, unsigned &reg) {

  unsigned brop = Cond[0].getImm();
  reg = Cond[1].getReg();
  cc = Cond[2].getImm();
  switch (brop) {
    case My66000::BRC:
      op = My66000::PRC;
      if (invert)
        cc = TII->reverseBRC(static_cast<MYCC::CondCodes>(cc));
      break;
    case My66000::BRIB:
      op = My66000::PRIB;
      if (invert)
        cc = TII->reverseBRIB(static_cast<MYCB::CondBits>(cc));
      break;
    case My66000::BRFB:
      op = My66000::PRFB;
      if (invert)
        cc = TII->reverseBRFB(static_cast<MYCB::CondBits>(cc));
      break;
    case My66000::BBIT:
      op = My66000::PBIT;
      break;
    default:
      llvm_unreachable("Predicate conversion: unknown conditional branch");
  }
}


bool My66000PredBlock::Convert(MachineBasicBlock *Head,
			MachineBasicBlock *Succ0, MachineBasicBlock *Succ1,
			MachineBasicBlock *Tail) {
  MachineBasicBlock *TBB, *FBB;
  SmallVector<MachineOperand, 4> Cond;
LLVM_DEBUG(dbgs() << "My66000PredBlock::Convert\n");
  if (!ExamineBranch(Head, TBB, FBB, Cond))
    return false;
  // AnalyzeBranch doesn't set FBB on a fall-through branch.
  FBB = TBB == Succ0 ? Succ1 : Succ0;

LLVM_DEBUG(dbgs() << "\tTBB=" << printMBBReference(*TBB) << '\n');
LLVM_DEBUG(dbgs() << "\tFBB=" << printMBBReference(*FBB) << '\n');
  // See how many instructions we can shadow
  int ninstrsT, ninstrsF;
  if (TBB == Tail)
    ninstrsT = 0;
  else
    ninstrsT = checkBlock(TBB);
  if (FBB == Tail)
    ninstrsF = 0;
  else
    ninstrsF = checkBlock(FBB);
LLVM_DEBUG(dbgs() << "\tninstr=" << ninstrsT << ',' << ninstrsF << '\n');
  if (ninstrsT < 0 || ninstrsF < 0 ||	// unpredicatable instructions
     (ninstrsT == 0 && ninstrsF == 0) || ninstrsT+ninstrsF > 8) {
LLVM_DEBUG(dbgs() << "\tCannot convert\n");
    return false;
  }

  MachineBasicBlock::iterator IP = Head->getFirstTerminator();
  DebugLoc dl = IP->getDebugLoc();

  unsigned cc;
  unsigned prop;
  unsigned reg;
  bool invert = false;	// FIXME
  getConditionInfo(Cond, invert, prop, cc, reg);

  // Create the predicate instruction
  MachineInstrBuilder MIB = BuildMI(*Head, IP, dl, TII->get(prop));
  MIB.addImm(cc);
  MIB.addReg(reg);
  MIB.addImm(((ninstrsT+ninstrsF-1)<<8) | ((1 << ninstrsT)-1));

  // Move all instructions into Head, except for the terminators.
  if (TBB != Tail)
    Head->splice(IP, TBB, TBB->begin(), TBB->getFirstTerminator());
  if (FBB != Tail)
    Head->splice(IP, FBB, FBB->begin(), FBB->getFirstTerminator());

  // Are there extra Tail predecessors?
  bool ExtraPreds = Tail->pred_size() != 2;

  // Fix up the CFG, temporarily leave Head without any successors.
  Head->removeSuccessor(TBB);
  Head->removeSuccessor(FBB, true);
  if (TBB != Tail)
    TBB->removeSuccessor(Tail, true);
  if (FBB != Tail)
    FBB->removeSuccessor(Tail, true);

  // Fix up Head's terminators.
  // It should become a single branch or a fallthrough.
  DebugLoc HeadDL = Head->getFirstTerminator()->getDebugLoc();
LLVM_DEBUG(dbgs() << "\tremove branch from Head\n");
  TII->removeBranch(*Head);
  if (Head->getFirstTerminator() != nullptr) {
LLVM_DEBUG(dbgs() << "\tperhaps 2 branches were in head?\n");
  }

  // Erase the now empty conditional blocks. It is likely that Head can fall
  // through to Tail, and we can join the two blocks.
  if (TBB != Tail) {
    TBB->eraseFromParent();
  }
  if (FBB != Tail) {
    FBB->eraseFromParent();
  }

  assert(Head->succ_empty() && "Additional head successors?");
  if (!ExtraPreds && Head->isLayoutSuccessor(Tail)) {
    // Splice Tail onto the end of Head.
LLVM_DEBUG(dbgs() << "\tjoining tail " << printMBBReference(*Tail)
           << " into head " << printMBBReference(*Head) << '\n');
    Head->splice(Head->end(), Tail, Tail->begin(), Tail->end());
    Head->transferSuccessors(Tail);
    Tail->eraseFromParent();
  } else {
    // We need a branch to Tail, let code placement work it out later.
LLVM_DEBUG(dbgs() << "\tconverting to unconditional branch.\n");
    SmallVector<MachineOperand, 0> EmptyCond;
    TII->insertBranch(*Head, Tail, nullptr, EmptyCond, HeadDL);
    Head->addSuccessor(Tail);
  }
  return true;  // FIXME - temp
}

bool My66000PredBlock::ConvertT2(MachineBasicBlock *Head0,
			MachineBasicBlock *Head1,
			MachineBasicBlock *Succ0, MachineBasicBlock *Succ1,
			MachineBasicBlock *Tail) {
  SmallVector<MachineOperand, 4> Cond0, Cond1;
  MachineBasicBlock *TBB0, *FBB0, *TBB1, *FBB1;

LLVM_DEBUG(dbgs() << "ConvertT2\n");
LLVM_DEBUG(dbgs() << "\tHead0:  " << printMBBReference(*Head0) << '\n');
LLVM_DEBUG(dbgs() << "\tHead1:  " << printMBBReference(*Head1) << '\n');
LLVM_DEBUG(dbgs() << "\tSucc0:  " << printMBBReference(*Succ0) << '\n');
LLVM_DEBUG(dbgs() << "\tSucc1:  " << printMBBReference(*Succ1) << '\n');
LLVM_DEBUG(dbgs() << "\tTail:   " << printMBBReference(*Tail) << '\n');
  if (!ExamineBranch(Head0, TBB0, FBB0, Cond0)) {
   return false;
  }
  if (!ExamineBranch(Head1, TBB1, FBB1, Cond1)) {
   return false;
  }
  // See how many instructions we can shadow
  int ninstrsT0 = checkBlock(Head1);
  int ninstrsT1 = checkBlock(Succ0);
  int ninstrsF1 = 0;	// FIXME - not true for Diamond2
  if (ninstrsT0 < 0 || ninstrsT1 < 0 || ninstrsF1 < 0)	// unpredicatable
    return false;
  ninstrsT0 += 1;		// add back the predicate instruction
LLVM_DEBUG(dbgs() << "\tninstrsT0=" << ninstrsT0 << '\n');
LLVM_DEBUG(dbgs() << "\tninstrsT1=" << ninstrsT1 << '\n');
LLVM_DEBUG(dbgs() << "\tninstrsF1=" << ninstrsF1 << '\n');
  if (ninstrsT0 + ninstrsT1 + ninstrsF1 > 8)		// too many
    return false;
  unsigned M1 = (1 << ninstrsT1)-1;		// inner predicate mask
  unsigned M0 = (1 << ninstrsT0)-1;		// outer predicate mask
LLVM_DEBUG(dbgs() << "\tM1=" << M1 << '\n');
LLVM_DEBUG(dbgs() << "\tM0=" << M0 << '\n');
LLVM_DEBUG(dbgs() << "\tTBB0:   " << printMBBReference(*TBB0) << '\n');
LLVM_DEBUG(dbgs() << "\tFBB0:   " << printMBBReference(*FBB0) << '\n');
LLVM_DEBUG(dbgs() << "\tTBB1:   " << printMBBReference(*TBB1) << '\n');
LLVM_DEBUG(dbgs() << "\tFBB1:   " << printMBBReference(*FBB1) << '\n');

  unsigned cc;
  unsigned prop;
  unsigned reg;
  bool invert = TBB1 != Succ0;
  getConditionInfo(Cond1, invert, prop, cc, reg);
  // Create predicate instruction for 2nd condition
  MachineBasicBlock::iterator IP = Head1->getFirstTerminator();
  DebugLoc dl = IP->getDebugLoc();
  MachineInstrBuilder MIB = BuildMI(*Head1, IP, dl, TII->get(prop));
  MIB.addImm(cc);
  MIB.addReg(reg);
  MIB.addImm(((ninstrsT1+ninstrsF1-1)<<8) | M1);
  // Move all instructions into Head1, except for the terminators
  if (Succ0 != Tail) {
    Head1->splice(IP, Succ0, Succ0->begin(), Succ0->getFirstTerminator());
    Head1->removeSuccessor(Succ0);
    Head0->removeSuccessor(Succ0);
    Succ0->removeSuccessor(Tail);
    Succ0->eraseFromParent();
  }
  if (Succ1 != Tail) {
    Head1->splice(IP, Succ1, Succ1->begin(), Succ1->getFirstTerminator());
    Head1->removeSuccessor(Succ1);
    Head0->removeSuccessor(Succ1);
    Succ0->removeSuccessor(Tail);
    Succ1->eraseFromParent();
  }
  TII->removeBranch(*Head1);

  invert = TBB0 != Head1;
  getConditionInfo(Cond0, invert, prop, cc, reg);
  IP = Head0->getFirstTerminator();
  dl = IP->getDebugLoc();
  MIB = BuildMI(*Head0, IP, dl, TII->get(prop));
  MIB.addImm(cc);
  MIB.addReg(reg);
  MIB.addImm(((ninstrsT0+ninstrsT1+ninstrsF1-1)<<8) | M0);
  // Move all instructions into Head0, except for the terminators
  Head0->splice(IP, Head1, Head1->begin(), Head1->getFirstTerminator());
  Head0->removeSuccessor(Head1, true);
  if (Head1->isSuccessor(Tail))
    Head1->removeSuccessor(Tail);
  Head1->eraseFromParent();
  TII->removeBranch(*Head0);
  Head0->addSuccessor(Tail);
  if (!Head0->isLayoutSuccessor(Tail)) {
    // We need a branch to Tail, let code placement work it out later.
LLVM_DEBUG(dbgs() << "\tconverting to unconditional branch.\n");
    SmallVector<MachineOperand, 0> EmptyCond;
    TII->insertBranch(*Head0, Tail, nullptr, EmptyCond, dl);
  }
  unsigned N = ninstrsT0+ninstrsT1+ninstrsF1;
  MachineBasicBlock::iterator IB = MIB;	// save location of 1st predicate
  MachineBasicBlock::iterator IE = std::next(IB, N+1);
  MIBundleBuilder(*Head0, IB, IE);
  return true;
}

bool My66000PredBlock::ConvertD2(MachineBasicBlock *Head0,
			MachineBasicBlock *Head1,
			MachineBasicBlock *Succ0, MachineBasicBlock *Succ1,
			MachineBasicBlock *Tail) {
  SmallVector<MachineOperand, 4> Cond0, Cond1;
  MachineBasicBlock *TBB0, *FBB0, *TBB1, *FBB1;

LLVM_DEBUG(dbgs() << "ConvertD2\n");
LLVM_DEBUG(dbgs() << "\tHead0:  " << printMBBReference(*Head0) << '\n');
LLVM_DEBUG(dbgs() << "\tHead1:  " << printMBBReference(*Head1) << '\n');
LLVM_DEBUG(dbgs() << "\tSucc0:  " << printMBBReference(*Succ0) << '\n');
LLVM_DEBUG(dbgs() << "\tSucc1:  " << printMBBReference(*Succ1) << '\n');
LLVM_DEBUG(dbgs() << "\tTail:   " << printMBBReference(*Tail) << '\n');
  if (!ExamineBranch(Head0, TBB0, FBB0, Cond0)) {
   return false;
  }
  if (!ExamineBranch(Head1, TBB1, FBB1, Cond1)) {
   return false;
  }
  // See how many instructions we can shadow
  int ninstrsT0 = checkBlock(Head1);
  int ninstrsT1 = checkBlock(Succ0);
  int ninstrsF1 = checkBlock(Succ1);
  if (ninstrsT0 < 0 || ninstrsT1 < 0 || ninstrsF1 < 0)	// unpredicatable
    return false;
  ninstrsT0 += 1;		// add back the nested predicate instruction
LLVM_DEBUG(dbgs() << "\tninstrsT0=" << ninstrsT0 << '\n');
LLVM_DEBUG(dbgs() << "\tninstrsT1=" << ninstrsT1 << '\n');
LLVM_DEBUG(dbgs() << "\tninstrsF1=" << ninstrsF1 << '\n');
  if (ninstrsT0 + ninstrsT1 + ninstrsF1 > 8)		// too many
    return false;
  unsigned M1 = (1 << ninstrsT1)-1;		// inner predicate mask
//  unsigned M0 = (1 << ninstrsT0)-1;		// outer predicate mask
  unsigned M0 = (1 << (ninstrsT0+ninstrsT1))-1;	// outer predicate mask
LLVM_DEBUG(dbgs() << "\tM1=" << M1 << '\n');
LLVM_DEBUG(dbgs() << "\tM0=" << M0 << '\n');
LLVM_DEBUG(dbgs() << "\tTBB0:   " << printMBBReference(*TBB0) << '\n');
LLVM_DEBUG(dbgs() << "\tFBB0:   " << printMBBReference(*FBB0) << '\n');
LLVM_DEBUG(dbgs() << "\tTBB1:   " << printMBBReference(*TBB1) << '\n');
LLVM_DEBUG(dbgs() << "\tFBB1:   " << printMBBReference(*FBB1) << '\n');

  unsigned cc;
  unsigned prop;
  unsigned reg;
  bool invert = TBB1 != Succ0;
  getConditionInfo(Cond1, invert, prop, cc, reg);
  // Create predicate instruction for 2nd condition
  MachineBasicBlock::iterator IP = Head1->getFirstTerminator();
  DebugLoc dl = IP->getDebugLoc();
  MachineInstrBuilder MIB = BuildMI(*Head1, IP, dl, TII->get(prop));
  MIB.addImm(cc);
  MIB.addReg(reg);
  MIB.addImm(((ninstrsT1+ninstrsF1-1)<<8) | M1);
  // Move all instructions into Head1, except for the terminators
  Head1->splice(IP, Succ0, Succ0->begin(), Succ0->getFirstTerminator());
  Head1->removeSuccessor(Succ0);
  Succ0->removeSuccessor(Tail);
  Succ0->eraseFromParent();
  Head1->splice(IP, Succ1, Succ1->begin(), Succ1->getFirstTerminator());
  Head1->removeSuccessor(Succ1);
  Head0->removeSuccessor(Succ1);
  Succ1->removeSuccessor(Tail);
  Succ1->eraseFromParent();
  TII->removeBranch(*Head1);
  invert = TBB0 != Head1;
  getConditionInfo(Cond0, invert, prop, cc, reg);
  IP = Head0->getFirstTerminator();
  dl = IP->getDebugLoc();
  MIB = BuildMI(*Head0, IP, dl, TII->get(prop));
  MIB.addImm(cc);
  MIB.addReg(reg);
  MIB.addImm(((ninstrsT0+ninstrsT1+ninstrsF1-1)<<8) | M0);
  // Move all instructions into Head0, except for the terminators
  Head0->splice(IP, Head1, Head1->begin(), Head1->getFirstTerminator());
  Head0->removeSuccessor(Head1, true);
  if (Head1->isSuccessor(Tail))
    Head1->removeSuccessor(Tail);
  Head1->eraseFromParent();
  TII->removeBranch(*Head0);
  Head0->addSuccessor(Tail);
  if (!Head0->isLayoutSuccessor(Tail)) {
    // We need a branch to Tail, let code placement work it out later.
LLVM_DEBUG(dbgs() << "\tconverting to unconditional branch.\n");
    SmallVector<MachineOperand, 0> EmptyCond;
    TII->insertBranch(*Head0, Tail, nullptr, EmptyCond, dl);
  }
  unsigned N = ninstrsT0+ninstrsT1+ninstrsF1;
  MachineBasicBlock::iterator IB = MIB;	// save location of 1st predicate
  MachineBasicBlock::iterator IE = std::next(IB, N+1);
  MIBundleBuilder(*Head0, IB, IE);
  return true;
}

bool My66000PredBlock::InsertPredInstructions(MachineBasicBlock *Head) {
LLVM_DEBUG(dbgs() << "My66000PredBlock::InsertPredInstructions "
		  << printMBBReference(*Head) << '\n');
  bool Modified = false;
  if (Head->succ_size() != 2)
    return false;
  MachineBasicBlock *Tail = nullptr;
  MachineBasicBlock *Succ0 = Head->succ_begin()[0];
  MachineBasicBlock *Succ1 = Head->succ_begin()[1];

  // Canonicalize so Succ0 has Head as its single predecessor.
  if (Succ0->pred_size() != 1) {
LLVM_DEBUG(dbgs() << "\tswapped arms\n");
    std::swap(Succ0, Succ1);
  }
LLVM_DEBUG(dbgs() << "\tSucc0: " << printMBBReference(*Succ0) <<
" #P=" << Succ0->pred_size() << " #S=" << Succ0->succ_size() << '\n');
LLVM_DEBUG(dbgs() << "\tSucc1: " << printMBBReference(*Succ1) <<
" #P=" << Succ1->pred_size() << " #S=" << Succ1->succ_size() << '\n');
  if (Succ0->pred_size() != 1)
    return false;

  if (Succ0->succ_size() == 1) { // Could be simple triangle or diamond
    Tail = Succ0->succ_begin()[0];
LLVM_DEBUG(dbgs() << "\tTail:  " << printMBBReference(*Tail) << '\n');
    if (Tail == Succ1) {
LLVM_DEBUG(dbgs() << "\tTriangle\n");
    } else {
      // Check for a diamond. We won't deal with any critical edges.
      if (Succ1->pred_size() == 1 && Succ1->succ_size() == 1 &&
          Succ1->succ_begin()[0] == Tail) {
LLVM_DEBUG(dbgs() << "\tDiamond\n");
      } else {
        return false;
      }
    }
    // We have a simple triangle or diamond
    Modified = Convert(Head, Succ0, Succ1, Tail);
    if (Modified) NumPREDs += 1;

  } else {	// not a simple triangle or diamond

LLVM_DEBUG(dbgs() << "\tpossible && or ||\n");
    if (!EnablePred2)
      return false;
    MachineBasicBlock *Head1 = Succ0;
    if (Head1->succ_size() != 2)
      return false;
    if (Head1->succ_begin()[0] == Succ1) {
      Succ0 = Head1->succ_begin()[1];
    } else if (Head1->succ_begin()[1] == Succ1) {
      Succ0 = Head1->succ_begin()[0];
    } else	// not a 2 level triangle or diamond
      return false;
    // Canonicalize so Succ0 has Head1 as its single predecessor.
    if (Succ0->pred_size() != 1) {
LLVM_DEBUG(dbgs() << "\tswapped arms\n");
      std::swap(Succ0, Succ1);
    }
LLVM_DEBUG(dbgs() << "\tHead1:  " << printMBBReference(*Head1) << '\n');
LLVM_DEBUG(dbgs() << "\tSucc0: " << printMBBReference(*Succ0) <<
" #P=" << Succ0->pred_size() << " #S=" << Succ0->succ_size() << '\n');
LLVM_DEBUG(dbgs() << "\tSucc1: " << printMBBReference(*Succ1) <<
" #P=" << Succ1->pred_size() << " #S=" << Succ1->succ_size() << '\n');
    if (Succ1->pred_size() != 2)
      return false;
    if (Succ0->pred_size() != 1)
      return false;
    if (Succ0->succ_size() != 1)
      return false;
    Tail = Succ0->succ_begin()[0];
LLVM_DEBUG(dbgs() << "\tTail:  " << printMBBReference(*Tail) <<
" #P=" << Tail->pred_size() << " #S=" << Tail->succ_size() << '\n');
    if (Tail == Succ1) {
LLVM_DEBUG(dbgs() << "\tTriangle2\n");
      Modified = ConvertT2(Head, Head1, Succ0, Succ1, Tail);
    } else {
      if (Succ1->succ_size() != 1)
	return false;
      // Both Succ? must have Tail as only succ.
      if (Succ1->succ_begin()[0] != Tail) {
	return false;
      }
LLVM_DEBUG(dbgs() << "\tDiamond2\n");
      Modified = ConvertD2(Head, Head1, Succ0, Succ1, Tail);
    }
    if (Modified) NumPRED2s += 1;
  }

  return Modified;
}


bool My66000PredBlock::onePass(MachineFunction &MF) {
  // If we did any inserts, blocks may have been deleted so
  // we must start at the beginning again.
  for (auto &MBB : MF ) {
    if (InsertPredInstructions(&MBB))
      return true;
  }
  return false;
}

bool My66000PredBlock::runOnMachineFunction(MachineFunction &MF) {
  TII = MF.getSubtarget<My66000Subtarget>().getInstrInfo();

  if (!EnablePred) return false;
LLVM_DEBUG(dbgs() << "My66000PredBlock::runOnMachineFunction\n");
// begin debug
LLVM_DEBUG(dbgs() << "*** Original basic blocks ***\n");
    for (auto &MBB : MF ) {
      LLVM_DEBUG(dbgs() << MBB);
    }
// end debug
  bool Modified = false;
  bool Mod;
  do {
    Mod = onePass(MF);
    Modified |= Mod;
// begin debug
  if (Mod) {
LLVM_DEBUG(dbgs() << "*** Modified basic blocks ***\n");
    for (auto &MBB : MF ) {
      LLVM_DEBUG(dbgs() << MBB);
    }
  }
// end debug
  } while (Mod);
  return Modified;
}

/// createMy66000PredBlock - Returns an instance of the My66000PredBlock
/// insertion pass.
FunctionPass *llvm::createMy66000PredBlockPass() { return new My66000PredBlock(); }
