//===-- My66000TargetInfo.cpp - My66000 Target Implementation -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/My66000TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheMy66000Target() {
  static Target TheMy66000Target;
  return TheMy66000Target;
}

extern "C" void LLVMInitializeMy66000TargetInfo() {
  RegisterTarget<Triple::my66000> X(getTheMy66000Target(), "my66000", "My66000",
				    "My66000");
}
