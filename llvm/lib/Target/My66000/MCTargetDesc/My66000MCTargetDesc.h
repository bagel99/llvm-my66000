//===-- My66000MCTargetDesc.h - My66000 Target Descriptions ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides My66000 specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MCTARGETDESC_MY66000MCTARGETDESC_H
#define LLVM_LIB_TARGET_MY66000_MCTARGETDESC_MY66000MCTARGETDESC_H

namespace llvm {

class Target;

} // end namespace llvm

// Defines symbolic names for My66000 registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "My66000GenRegisterInfo.inc"

// Defines symbolic names for the My66000 instructions.
//
#define GET_INSTRINFO_ENUM
#include "My66000GenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "My66000GenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_MY66000_MCTARGETDESC_MY66000MCTARGETDESC_H
