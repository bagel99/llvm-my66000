//===-- My66000MachineFunctionInfo.cpp - My66000 machine function info ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "My66000MachineFunctionInfo.h"

using namespace llvm;

void My66000FunctionInfo::anchor() { }

MachineFunctionInfo *My66000FunctionInfo::clone(
    BumpPtrAllocator &Allocator, MachineFunction &DestMF,
    const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
    const {
  return DestMF.cloneInfo< My66000FunctionInfo>(*this);
}
