//===-- My66000MCAsmInfo.h - My66000 asm properties ----------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the My66000MCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MCTARGETDESC_MY66000MCASMINFO_H
#define LLVM_LIB_TARGET_MY66000_MCTARGETDESC_MY66000MCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class My66000MCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit My66000MCAsmInfo(const Triple &TT);
};

} // namespace llvm

#endif
