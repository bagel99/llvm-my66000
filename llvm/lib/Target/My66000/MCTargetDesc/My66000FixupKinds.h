//===-- My66000FixupKinds.h - My66000 Specific Fixup Entries ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MY66000FIXUPKINDS_H
#define LLVM_LIB_TARGET_MY66000_MY66000FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace My66000 {

  // This table *must* be in the same order of
  // MCFixupKindInfo Infos[My66000::NumTargetFixupKinds]
  // in My66000AsmBackend.cpp.
  enum Fixups {
    fixup_My66000_NONE = FirstTargetFixupKind,

    // Pure 32 but data fixup
    fixup_My66000_32,

    // Marker
    LastTargetFixupKind,
    NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
  };
} // namespace My66000
} // namespace llvm


#endif
