//=- My66000VVMPass2.cpp - Fixup VEC instructions physical reg list -------===//
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
#include "llvm/CodeGen/MachineLoopInfo.h"

using namespace llvm;

#define DEBUG_TYPE "VVM fixup pass"
#define PASS_NAME "My66000 VVM Fixup VECs"
#include "llvm/InitializePasses.h"

namespace {

class My66000VVMFixup: public MachineFunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid

  My66000VVMFixup() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override {
    return PASS_NAME;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    MachineFunctionPass::getAnalysisUsage(AU);
    AU.addRequired<MachineLoopInfo>();
  }
private:
  bool fixLoop(MachineLoop *Loop);
  void findModified(MachineBasicBlock *BB, std::bitset<32> &Modified);
};

} // end anonymous namespace

char My66000VVMFixup::ID = 0;

INITIALIZE_PASS_BEGIN(My66000VVMFixup, DEBUG_TYPE, PASS_NAME, false, false)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
INITIALIZE_PASS_END(My66000VVMFixup, DEBUG_TYPE, PASS_NAME, false, false)

void My66000VVMFixup::findModified(MachineBasicBlock *MBB, std::bitset<32> &Modified) {
  MachineBasicBlock::iterator I = MBB->begin();
  MachineBasicBlock::iterator E = MBB->end();
  std::bitset<32> Def, Kill;
  unsigned reg;

  Modified.reset();
  while (I != E) {
    MachineInstr *MI = &*I;
    Def.reset(); Kill.reset();
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
    Modified &= ~Kill;
    Modified |= Def;
    ++I;
  }
}

bool My66000VVMFixup::fixLoop(MachineLoop *Loop) {
  MachineBasicBlock *TB = Loop->getTopBlock();
  MachineBasicBlock::iterator I = TB->begin();
  unsigned bits = 0;
  std::bitset<32> Livein, Modified;

  // We are only interested in loops that start with VEC
  if (I->getOpcode() == My66000::VEC) {
LLVM_DEBUG(dbgs() << "  fix loop\n");
    findModified(TB, Modified);
    Livein.reset();
    for (MachineBasicBlock::succ_iterator SI = TB->succ_begin(),
         SE = TB->succ_end(); SI != SE; ++SI) {
      if (*SI != TB) {
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
LLVM_DEBUG(dbgs() << "  modified=" << Modified.to_string() << '\n');
LLVM_DEBUG(dbgs() << "  livein=  " << Livein.to_string() << '\n');
    Modified &= Livein;
LLVM_DEBUG(dbgs() << "  bits=    " << Modified.to_string() << '\n');
    bits = Modified.to_ulong();
    I->removeOperand(1);
    I->addOperand(MachineOperand::CreateImm(bits>>1));
  }

  return false;
}

bool My66000VVMFixup::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;

//  if (!EnableVVM) return false;
LLVM_DEBUG(dbgs() << "VVMFixupPass: " << MF.getName() << '\n');

  MachineLoopInfo &MLI = getAnalysis<MachineLoopInfo>();
  SmallVector<MachineLoop *, 4> Loops(MLI.begin(), MLI.end());
  for (int i = 0; i < (int)Loops.size(); ++i)
    for (MachineLoop *Child : Loops[i]->getSubLoops())
      Loops.push_back(Child);
  for (MachineLoop *CurrLoop : Loops) {
    if (!CurrLoop->getSubLoops().empty())
      continue;
    Changed = fixLoop(CurrLoop);
  }

  return Changed;
}

FunctionPass *llvm::createMy66000VVMFixupPass() {
  return new My66000VVMFixup();
}

