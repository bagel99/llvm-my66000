//===-- My66000Subtarget.cpp - My66000 Subtarget Information ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the My66000 specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "My66000Subtarget.h"
#include "My66000.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "my66000-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "My66000GenSubtargetInfo.inc"

static cl::opt<bool> EnablePred("enable-predication", cl::Hidden,
  cl::desc("Enable predication instructions"));
static cl::opt<bool> EnableVVM("enable-vvm", cl::Hidden,
  cl::desc("Enable VVM Loop Mode"));

void My66000Subtarget::anchor() { }

My66000Subtarget::My66000Subtarget(const Triple &TT, const std::string &CPU,
                           const std::string &FS, const TargetMachine &TM)
    : My66000GenSubtargetInfo(TT, CPU, /*TuneCPU*/ CPU, FS), InstrInfo(),
      FrameLowering(*this), TLInfo(TM, *this), TSInfo() {}

bool My66000Subtarget::usePredication() const
{
  return EnablePred;
}

bool My66000Subtarget::useVVM() const
{
  return EnableVVM;
}

