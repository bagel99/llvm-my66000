//=- My66000FixJumpTable.cpp - Remove range checks preceeding jump tables -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Adopted from WebAssembly code.
//===----------------------------------------------------------------------===//
///
/// \file This file implements a pass that eliminates redundant range checks
/// guarding jump table instructions. Since jump tables on most targets cannot
/// handle out of range indices, LLVM emits these checks before most jump
/// tables. We do not need the range checks.
///
//===----------------------------------------------------------------------===//

#include "My66000.h"
#include "My66000MachineFunctionInfo.h"
#include "My66000Subtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

#define DEBUG_TYPE "fix-jump-table"
#define PASS_NAME "My66000 Remove range checks for jump tables"

static cl::opt<bool> EnableRangeOpt("enable-remove-range-check", cl::Hidden,
  cl::desc("Enable the removal of range checks before jump via table"));

namespace {

class My66000FixJumpTable: public MachineFunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid

  My66000FixJumpTable() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override {
    return PASS_NAME;
  }

};

} // end anonymous namespace


char My66000FixJumpTable::ID = 0;

INITIALIZE_PASS(My66000FixJumpTable, DEBUG_TYPE, PASS_NAME, false, false)


static bool isJumpTable(MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case My66000::JT8:
  case My66000::JT16:
  case My66000::JT32:
    return true;
  default:
    return false;
  }
}

// `MI` is a JTT instruction with a dummy default target argument. This
// function finds and changes the default target argument and removes any redundant
// range check preceding the JTT.
MachineBasicBlock *FixJumpTable(MachineInstr &MI, MachineBasicBlock *MBB,
                              MachineFunction &MF) {
  LLVM_DEBUG(dbgs() << "My66000FixJumpTable in " << MF.getName() << '\n');
  // Get the header block, which contains the redundant range check.
  assert(MBB->pred_size() == 1 && "Expected a single guard predecessor");
  auto *HeaderMBB = *MBB->pred_begin();

  // Find the conditional jump to the default target. If it doesn't exist, the
  // default target is unreachable anyway, so we can choose anything.
  MachineBasicBlock *TBB = nullptr, *FBB = nullptr;
  SmallVector<MachineOperand, 2> Cond;
  const auto &TII = *MF.getSubtarget<My66000Subtarget>().getInstrInfo();
  TII.analyzeBranch(*HeaderMBB, TBB, FBB, Cond, false);

  // Here are the possible outcomes. '_' is nullptr, `J` is the jump table block
  // aka MBB, 'D' is the default block.
  //
  // TBB | FBB | Meaning
  //  _  |  _  | No default block, header falls through to jump table
  //  J  |  _  | No default block, header jumps to the jump table
  //  D  |  _  | Header jumps to the default and falls through to the jump table
  //  D  |  J  | Header jumps to the default and also to the jump table
  if (TBB && TBB != MBB) {
    // Install branch to default target following the jump table
    assert((FBB == nullptr || FBB == MBB) &&
           "Expected jump or fallthrough to br_table block");

    // Remove the dummy default target and install the real one.
    MI.removeOperand(MI.getNumExplicitOperands() - 1);
    MI.addOperand(MF, MachineOperand::CreateMBB(TBB));
  }
  // Remove any branches from the header and splice in the jump table instead
  TII.removeBranch(*HeaderMBB, nullptr);
  HeaderMBB->splice(HeaderMBB->end(), MBB, MBB->begin(), MBB->end());

  // Update CFG to skip the old jump table block. Remove shared successors
  // before transferring to avoid duplicated successors.
  HeaderMBB->removeSuccessor(MBB);
  for (auto &Succ : MBB->successors())
    if (HeaderMBB->isSuccessor(Succ))
      HeaderMBB->removeSuccessor(Succ);
  HeaderMBB->transferSuccessorsAndUpdatePHIs(MBB);

  // Remove the old jump table block from the function
  MF.erase(MBB);
  return HeaderMBB;
}

bool My66000FixJumpTable::runOnMachineFunction(MachineFunction &MF) {

  if (!EnableRangeOpt) return false;
  bool Changed = false;
  SmallPtrSet<MachineBasicBlock *, 16> MBBSet;
  for (auto &MBB : MF)
    MBBSet.insert(&MBB);

  while (!MBBSet.empty()) {
    MachineBasicBlock *MBB = *MBBSet.begin();
    MBBSet.erase(MBB);
    for (auto &MI : *MBB) {
      if (isJumpTable(MI)) {
        auto *Fixed = FixJumpTable(MI, MBB, MF);
        MBBSet.erase(Fixed);
        Changed = true;
        break;
      }
    }
  }

  if (Changed) {
    // We rewrote part of the function; recompute relevant things.
    MF.RenumberBlocks();
    return true;
  }
  return false;
}

FunctionPass *llvm::createMy66000FixJumpTablePass() {
  return new My66000FixJumpTable();
}

