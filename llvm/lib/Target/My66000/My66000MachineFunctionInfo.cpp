//===-- My66000MachineFunctionInfo.cpp - My66000 machine function info ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "My66000MachineFunctionInfo.h"
#include "My66000InstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Function.h"

using namespace llvm;

void My66000FunctionInfo::anchor() { }
