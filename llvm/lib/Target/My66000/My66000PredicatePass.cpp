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
#include "My66000TargetMachine.h"
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

static cl::opt<bool> EnablePred2("enable-predication2", cl::Hidden,
  cl::desc("Enable double predication instructions"));

STATISTIC(NumPREDs,        "Number of single predicated blocks inserted");
STATISTIC(NumPRED2s,       "Number of double predicated blocks inserted");
STATISTIC(NumRanges,       "Number of range checks converted");

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
    void MakeBundle(MachineBasicBlock *MBB, MachineInstr *Head, unsigned N);
    bool Convert(MachineBasicBlock *Head,
		 MachineBasicBlock *Succ0, MachineBasicBlock *Succ1,
		 MachineBasicBlock *Tail);
    bool ConvertT2(MachineBasicBlock *Head0, MachineBasicBlock *Head1,
		 MachineBasicBlock *Succ0, MachineBasicBlock *Succ1,
		 MachineBasicBlock *Tail);
    bool ConvertD2(MachineBasicBlock *Head0, MachineBasicBlock *Head1,
		 MachineBasicBlock *Succ0, MachineBasicBlock *Succ1,
		 MachineBasicBlock *Tail);
    void MakeBundles(MachineBasicBlock *MBB);
    bool findCompare(MachineBasicBlock *MBB, Register reg, MachineInstr *&Cmp);
    bool RangeCheck2(MachineBasicBlock *MBB);
    bool RangeCheck1(MachineFunction &MF);
    void RangeCheck(MachineFunction &MF);
    void ExpandBBIT0(MachineBasicBlock *MBB);
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
  unsigned NumInstrs = 0;
LLVM_DEBUG(dbgs() << "My66000PredBlock::checkBlock\n");
  MachineBasicBlock::instr_iterator MII = MBB->instr_begin();
  MachineBasicBlock::instr_iterator MIE = MBB->instr_end();
  if (MII == MIE)
    return 0;
  for (; MII != MIE; ++MII) {
    MachineInstr *MI = &*MII;
    if (MI->isTerminator()) return NumInstrs;
    // VVM doesn't allow calls
    if (MI->isCall()) return -1;
    // Bad things happen if IMPLICIT_DEF is inside a bundle
    if (MI->getOpcode() == TargetOpcode::IMPLICIT_DEF) return -1;
    // FIXME - why are CFI_INSTRUCTIONs in the code?
    // answer: because of tail merged RETs
//LLVM_DEBUG(dbgs() << "check " << *MI);
    if (!MI->isCFIInstruction() && !MI->isBundle()) {
      NumInstrs += 1;
    }
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
    case My66000::BBIT1:
      op = My66000::PBIT;
      break;
    default:
      llvm_unreachable("Predicate conversion: unknown conditional branch");
  }
}

// This code was copied from the inner loop of
// MachineInstrBundle::unpackBundles. That code does an entire basic block,
// but we just need to unpack a single bundle.
// Return iterator to next instruction after the bundle.
static MachineBasicBlock::instr_iterator unBundle(
		     MachineBasicBlock::instr_iterator MII,
		     MachineBasicBlock::instr_iterator MIE) {
  MachineInstr *MI = &*MII;
LLVM_DEBUG(dbgs() << "unBundle " << *MI);
  // Remove BUNDLE instruction and the InsideBundle flags from bundled
  // instructions.
  if (MI->isBundle()) {
    while (++MII != MIE && MII->isBundledWithPred()) {
LLVM_DEBUG(dbgs() << "\tunbundleFromPred " << *MII);
      MII->unbundleFromPred();
      for (MachineOperand &MO  : MII->operands()) {
        if (MO.isReg() && MO.isInternalRead())
          MO.setIsInternalRead(false);
      }
    }
LLVM_DEBUG(dbgs() << "\terasing " << *MI);
    MI->eraseFromParent();
  }
  return MII;
}

void My66000PredBlock::MakeBundle(MachineBasicBlock *MBB, MachineInstr *MI,
				  unsigned N) {
//  MI->setFlag(MachineInstr::NoMerge);
  // FIXME - it is not clear if IE is the last instruction in the bundle
  // or the next instruction folling the bundle.
  MachineBasicBlock::instr_iterator IB = MI->getIterator();
  MachineBasicBlock::instr_iterator IE = std::next(IB, N);
LLVM_DEBUG(dbgs() << "make bundle BB=" << printMBBReference(*MBB) <<
		     " N=" << N << '\n');
LLVM_DEBUG(dbgs() << "\tIB= " << *IB);
LLVM_DEBUG(dbgs() << "\tIE= " << *IE);
  MachineBasicBlock::instr_iterator I = IB;
  while (I != IE) {
    MachineInstr *MI = &*I;
    if (MI->isBundle())		// remove interior bundles
      I = unBundle(I, IE);
    else
      ++I;
  }
LLVM_DEBUG(dbgs() << "\tBefore finalizeBundle\n" << *MBB);
  finalizeBundle(*MBB, IB, IE);
}

void My66000PredBlock::MakeBundles(MachineBasicBlock *MBB) {
  MachineBasicBlock::iterator I = MBB->begin();
  MachineBasicBlock::iterator E = MBB->end();
LLVM_DEBUG(dbgs() << "My66000PredBlock::MakeBundles\n");
  while (I != E) {
    if (I->isPredicable()) {
      // number of predicated instructions plus 1 for the predicate instruction
      unsigned N = I->getOperand(2).getImm() + I->getOperand(3).getImm() + 1;
      MakeBundle(MBB, &*I, N);
    }
    ++I;	// This will increment over an entire (just made) bundle
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
     (ninstrsT == 0 && ninstrsF == 0) ||
      ninstrsT > 8 || ninstrsF > 8) {
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
  MIB.addImm(ninstrsT);
  MIB.addImm(ninstrsF);

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
LLVM_DEBUG(dbgs() << "\tconverting to unconditional branch\n");
    SmallVector<MachineOperand, 0> EmptyCond;
    TII->insertBranch(*Head, Tail, nullptr, EmptyCond, HeadDL);
    Head->addSuccessor(Tail);
  }
//  MakeBundle(Head, MIB, ninstrsT+ninstrsF);
  return true;
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
LLVM_DEBUG(dbgs() << "\tinner: invert=" << invert <<
	" nT=" << ninstrsT1 << " nF=" << ninstrsF1 << '\n');
  MachineInstrBuilder MIB = BuildMI(*Head1, IP, dl, TII->get(prop));
  MIB.addImm(cc);
  MIB.addReg(reg);
  MIB.addImm(ninstrsT1);
  MIB.addImm(ninstrsF1);
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
LLVM_DEBUG(dbgs() << "\touter: invert=" << invert <<
	" nT=" << ninstrsT0+ninstrsF1 << " nF=" << ninstrsT1 << '\n');
  MIB = BuildMI(*Head0, IP, dl, TII->get(prop));
  MIB.addImm(cc);
  MIB.addReg(reg);
  MIB.addImm(ninstrsT0);
  MIB.addImm(ninstrsT1+ninstrsF1);
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
//  MakeBundle(Head0, MIB, ninstrsT0+ninstrsT1+ninstrsF1);
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
LLVM_DEBUG(dbgs() << "\tinner: invert=" << invert <<
	" nT=" << ninstrsT1 << " nF=" << ninstrsF1 << '\n');
  MachineInstrBuilder MIB = BuildMI(*Head1, IP, dl, TII->get(prop));
  MIB.addImm(cc);
  MIB.addReg(reg);
  MIB.addImm(ninstrsT1);
  MIB.addImm(ninstrsF1);
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
LLVM_DEBUG(dbgs() << "\touter: invert=" << invert <<
	" nT=" << ninstrsT0+ninstrsF1 << " nF=" << ninstrsT1 << '\n');
  MIB = BuildMI(*Head0, IP, dl, TII->get(prop));
  MIB.addImm(cc);
  MIB.addReg(reg);
  MIB.addImm(ninstrsT0+ninstrsT1);
  MIB.addImm(ninstrsF1);
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
//  MakeBundle(Head0, MIB, ninstrsT0+ninstrsT1+ninstrsF1);
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
    if (!EnablePred2)
      return false;
LLVM_DEBUG(dbgs() << "\tcheck for && or ||\n");
    // Succ0 has Head as sole predecessor
    if (Succ0->succ_size() != 2)	// is Succ0 conditional?
      return false;			// no
    MachineBasicBlock *Head1 = Succ0;
    // Find new Succ0
    if (Head1->succ_begin()[0] == Succ1) {
      Succ0 = Head1->succ_begin()[1];
    } else if (Head1->succ_begin()[1] == Succ1) {
      Succ0 = Head1->succ_begin()[0];
    } else	// not a 2 level triangle or diamond
      return false;
    // Canonicalize so Succ0 has Head1 as its single predecessor.
    if (Succ0->pred_size() == 2 && Succ1->pred_size() == 1) {
LLVM_DEBUG(dbgs() << "\tswapped arms\n");
      std::swap(Succ0, Succ1);
    } else if (Succ0->pred_size() != 1 || Succ1->pred_size() != 2) {
      return false;
    }
LLVM_DEBUG(dbgs() << "\tHead1:  " << printMBBReference(*Head1) << '\n');
LLVM_DEBUG(dbgs() << "\tSucc0: " << printMBBReference(*Succ0) <<
" #P=" << Succ0->pred_size() << " #S=" << Succ0->succ_size() << '\n');
LLVM_DEBUG(dbgs() << "\tSucc1: " << printMBBReference(*Succ1) <<
" #P=" << Succ1->pred_size() << " #S=" << Succ1->succ_size() << '\n');
    if (Succ0->succ_size() != 1)
      return false;
    Tail = Succ0->succ_begin()[0];
LLVM_DEBUG(dbgs() << "\tTail:  " << printMBBReference(*Tail) <<
" #P=" << Tail->pred_size() << " #S=" << Tail->succ_size() << '\n');
    if (Tail->pred_size() != 2)
      return false;
    if (Succ1->pred_size() != 2)
      return false;
    if (Tail == Succ1) {	// possible 2 level triangle
      if (Succ0->pred_size() > 2)
        return false;
LLVM_DEBUG(dbgs() << "\tTriangle2\n");
      Modified = ConvertT2(Head, Head1, Succ0, Succ1, Tail);
    } else {			// possible 2 level diamond
      if (Succ1->succ_size() != 1 || Succ1->succ_begin()[0] != Tail)
	return false;
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

/*
 * The My66000 compare instruction checks for integer ranges.
 * Given two BB, MBB1 and MBB2,
 * if MBB1 ends with BRC rx
 * and MBB2 ends with CMP rx,ry; BRIB
 * then if the conditions indicate a range check
 * the two blocks can be merged to one block with a BRIB with range bit.
 */
static MYCB::CondBits mapRangeBit(MYCC::CondCodes cc,
				  MYCB::CondBits cb) {
    if      (cc == MYCC::LE0 && cb == MYCB::GE)
      return MYCB::SIN;
    else if (cc == MYCC::LE0 && cb == MYCB::GT)
      return MYCB::FIN;
    else if (cc == MYCC::LT0 && cb == MYCB::GE)
      return MYCB::CIN;
    else if (cc == MYCC::LT0 && cb == MYCB::GT)
      return MYCB::RIN;
    else
      return MYCB::EQ;	// This is bogus
}

// Find the compare instruction that defines reg
// This would be simple if the define information was still around
bool My66000PredBlock::findCompare(MachineBasicBlock *MBB,
				   Register reg, MachineInstr *&Cmp) {

LLVM_DEBUG(dbgs() << "\tfindCompare: " << printMBBReference(*MBB) << '\n');
  MachineBasicBlock::iterator I = MBB->getFirstTerminator();
  while (I != MBB->begin()) {
    --I;
LLVM_DEBUG(dbgs() << "\ttest: " << *I);
    if (I->isCompare()) {
LLVM_DEBUG(dbgs() << "\tfound a CMP\n");
      if (I->getOperand(0).getReg() == reg) {
LLVM_DEBUG(dbgs() << "\tfound the CMP\n");
	Cmp = &*I;
	return true;
      }
    }
  }
  for (MachineBasicBlock::pred_iterator PI = MBB->pred_begin(),
                                        PE = MBB->pred_end();
					PI != PE; ++PI) {
    if (findCompare(*PI, reg, Cmp))
      return true;
  }
  return false;
}

bool My66000PredBlock::RangeCheck2(MachineBasicBlock *MBB1) {
LLVM_DEBUG(dbgs() << "My66000PredBlock::RangeCheck2\n");
  if (MBB1->succ_size() != 2)
    return false;
  MachineBasicBlock *MBB2 = MBB1->succ_begin()[0];
  MachineBasicBlock *MBB3 = MBB1->succ_begin()[1];
  // Canonicalize so MBB2 has MBB1 as its single predecessor.
  if (MBB2->pred_size() != 1) {
LLVM_DEBUG(dbgs() << "\tswapped arms\n");
    std::swap(MBB2, MBB3);
  }
  if (MBB2->pred_begin()[0] != MBB1)
    return false;
LLVM_DEBUG(dbgs() << "\tMBB1: " << printMBBReference(*MBB1) << '\n');
LLVM_DEBUG(dbgs() << "\tMBB2: " << printMBBReference(*MBB2) << '\n');
  SmallVector<MachineOperand, 4> Cond1, Cond2, Cond3;
  MachineBasicBlock *TBB1, *FBB1, *TBB2, *FBB2;
  if (!ExamineBranch(MBB1, TBB1, FBB1, Cond1))
    return false;
  if (!ExamineBranch(MBB2, TBB2, FBB2, Cond2))
    return false;
  MYCC::CondCodes cc;
  MYCB::CondBits cb;
  Register CondReg;	// the register that CMP sets and BRIB uses
  Register TestReg;	// the register that BRC uses and CMP uses
  // First BB must end with BRC and cc={LE0,LT0}
  if (       Cond1[0].getImm() == My66000::BRC &&
             Cond2[0].getImm() == My66000::BRIB) {
    // "normal" order
LLVM_DEBUG(dbgs() << "\tnormal order\n");
    cc = static_cast<MYCC::CondCodes>(Cond1[2].getImm());
    cb = static_cast<MYCB::CondBits> (Cond2[2].getImm());
    TestReg = Cond1[1].getReg();
    CondReg = Cond2[1].getReg();
    Cond3 = Cond2;
  } else if (Cond1[0].getImm() == My66000::BRIB &&
             Cond2[0].getImm() == My66000::BRC) {
    // "inverse" order
LLVM_DEBUG(dbgs() << "\tinverse order\n");
    cc = static_cast<MYCC::CondCodes>(Cond2[2].getImm());
    cb = static_cast<MYCB::CondBits> (Cond1[2].getImm());
    TestReg = Cond2[1].getReg();
    CondReg = Cond1[1].getReg();
    Cond3 = Cond1;
  } else
    return false;
  if (cc != MYCC::LT0 && cc != MYCC::LE0)
    return false;
  if (cb != MYCB::GT && cb != MYCB::GE)
    return false;
LLVM_DEBUG(dbgs() << "\tTBB1: " << printMBBReference(*TBB1) << '\n');
LLVM_DEBUG(dbgs() << "\tFBB1: " << printMBBReference(*FBB1) << '\n');
  if (FBB1 != MBB2)
    return false;
LLVM_DEBUG(dbgs() << "\tTBB2: " << printMBBReference(*TBB2) << '\n');
LLVM_DEBUG(dbgs() << "\tFBB2: " << printMBBReference(*FBB2) << '\n');
  // Both BRC and BBIT have to branch to the same place
  if (TBB1 != TBB2)
    return false;
  MachineBasicBlock::iterator IP2 = MBB2->getFirstTerminator();
  MachineInstr *BMI = &*IP2;   // the BRIB instruction
LLVM_DEBUG(dbgs() << "\tBRIB: " << *BMI);
  // Try and find the CMP instruction that produces CondReg
  MachineInstr *Cmp;
  if (!findCompare(MBB2, CondReg, Cmp))
    return false;
LLVM_DEBUG(dbgs() << "\tCMP: " << *Cmp);
  // The register in the BRC must be the same in the CMP
//  if (TestReg != IP2->getOperand(1).getReg())
//    return false;
LLVM_DEBUG(dbgs() << "\tpotential merge candidate\n");
LLVM_DEBUG(dbgs() << "\tcc= " << cc << "  cb= " << cb << '\n');
  cb = mapRangeBit(cc, cb);
LLVM_DEBUG(dbgs() << "\tnewcb= " << cb << '\n');
  // Merge MBB2 into MBB1 replacing the BRC (and any following BRU)
  MachineBasicBlock::iterator IP1 = MBB1->getFirstTerminator();
  MBB1->splice(IP1, MBB2, MBB2->begin(), MBB2->getFirstTerminator());
  // Remove old BRC
  TII->removeBranch(*MBB1);
  // Create a new BRIB with new condition bit
  Cond3[2] = MachineOperand::CreateImm(cb);
  TII->insertBranch(*MBB1, FBB2, TBB2, Cond3, BMI->getDebugLoc());
  // Update successors (this will update predecessors)
  MBB1->replaceSuccessor(MBB2, FBB2);
  MBB2->removeSuccessor(FBB2);
  MBB2->removeSuccessor(TBB2);
  // MBB2 is now unused
  MBB2->eraseFromParent();
  NumRanges++;
  return true;
}

bool My66000PredBlock::RangeCheck1(MachineFunction &MF) {
  for (auto &MBB : MF ) {
    if (RangeCheck2(&MBB))
      return true;
  }
  return false;
}

void My66000PredBlock::RangeCheck(MachineFunction &MF) {
LLVM_DEBUG(dbgs() << "My66000PredBlock::RangeCheck\n");
  bool Mod;
  do {
LLVM_DEBUG(dbgs() << "***Before RangeCheck ***\n");
#ifndef NDEBUG
    for (auto &MBB : MF ) {
      LLVM_DEBUG(dbgs() << MBB);
    }
#endif
    Mod = RangeCheck1(MF);
  } while (Mod);
}

void My66000PredBlock::ExpandBBIT0(MachineBasicBlock *MBB) {
  MachineBasicBlock *TBB, *FBB;
  SmallVector<MachineOperand, 4> Cond;

  Cond.clear();
  TBB = nullptr;
  FBB = nullptr;
  if (!ExamineBranch(MBB, TBB, FBB, Cond))
    return;	// not the right kind of branch
  if (Cond[0].getImm() == My66000::BBIT0) {
LLVM_DEBUG(dbgs() << "My66000PredBlock::ExpandBBIT0\n");
    // reverse true and false
    Cond[0].setImm(My66000::BBIT1);
    MachineBasicBlock::iterator MBI = MBB->getFirstTerminator();
    TII->removeBranch(*MBB);
    TII->insertBranch(*MBB, FBB, TBB, Cond, MBI->getDebugLoc());
  }
}

bool My66000PredBlock::runOnMachineFunction(MachineFunction &MF) {
  TII = MF.getSubtarget<My66000Subtarget>().getInstrInfo();

  // First expand any pseudo BBIT0
  for (auto &MBB : MF ) {
      ExpandBBIT0(&MBB);
  }

  if (!MF.getSubtarget<My66000Subtarget>().usePredication()) return false;
LLVM_DEBUG(dbgs() << "My66000PredBlock::runOnMachineFunction\n");
  RangeCheck(MF);
// begin debug
LLVM_DEBUG(dbgs() << "*** Original basic blocks ***\n");
#ifndef NDEBUG
    for (auto &MBB : MF ) {
      LLVM_DEBUG(dbgs() << MBB);
    }
#endif
// end debug
  bool Modified = false;
  bool Mod;
  do {
    Mod = onePass(MF);
    Modified |= Mod;
  } while (Mod);
  // if we predicated anything, bundle them
  if (Modified) {
    for (auto &MBB : MF ) {
      LLVM_DEBUG(dbgs() << MBB);
      MakeBundles(&MBB);
    }
// begin debug
LLVM_DEBUG(dbgs() << "*** Modified basic blocks ***\n");
#ifndef NDEBUG
    for (auto &MBB : MF ) {
      LLVM_DEBUG(dbgs() << MBB);
    }
#endif
// end debug
  }
  return Modified;
}

/// createMy66000PredBlock - Returns an instance of the My66000PredBlock
/// insertion pass.
FunctionPass *llvm::createMy66000PredBlockPass() { return new My66000PredBlock(); }
