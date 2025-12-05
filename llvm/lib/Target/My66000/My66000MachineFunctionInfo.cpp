//===-- My66000MachineFunctionInfo.cpp - My66000 machine function info ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "My66000MachineFunctionInfo.h"

using namespace llvm;

#define DEBUG_TYPE "my66000-lower"

void My66000MachineFunctionInfo::anchor() { }

MachineFunctionInfo *My66000MachineFunctionInfo::clone(
    BumpPtrAllocator &Allocator, MachineFunction &DestMF,
    const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
    const {
  return DestMF.cloneInfo< My66000MachineFunctionInfo>(*this);
}

void My66000MachineFunctionInfo::addSExtRegister(Register Reg) {
LLVM_DEBUG(dbgs() << "addSExtRegister: " << Reg <<'\n');
  SExtRegisters.push_back(Reg);
}

bool My66000MachineFunctionInfo::isSExtRegister(Register Reg) const {
LLVM_DEBUG(dbgs() << "isSExtRegister: " << Reg <<'\n');
  return is_contained(SExtRegisters, Reg);
}

void My66000MachineFunctionInfo::addZExtRegister(Register Reg) {
  ZExtRegisters.push_back(Reg);
}

bool My66000MachineFunctionInfo::isZExtRegister(Register Reg) const {
  return is_contained(ZExtRegisters, Reg);
}
