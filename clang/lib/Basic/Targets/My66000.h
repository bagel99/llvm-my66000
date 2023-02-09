//===--- My66000.h - Declare My66000 target feature support -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares My66000 TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_MY66000_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_MY66000_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetBuiltins.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY My66000TargetInfo : public TargetInfo {
  static const Builtin::Info BuiltinInfo[];

public:
  My66000TargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    NoAsmVariants = true;
    LongWidth = LongAlign = PointerWidth = PointerAlign = 64;
    MaxAtomicPromoteWidth = MaxAtomicInlineWidth = 64;
    resetDataLayout("e-m:e-p:64:64-i1:8-i8:8-i16:16-i32:32-i64:64-f64:64-a:0:64-n64");
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  ArrayRef<Builtin::Info> getTargetBuiltins() const override;

  const char *getClobbers() const override { return ""; }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }
  ArrayRef<const char *> getGCCRegNames() const override { return None; }
  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override {
    return None;
  }
  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    return false;
  }

  bool hasProtectedVisibility() const override { return false; }
};

} // namespace targets
} // namespace clang
#endif // LLVM_CLANG_LIB_BASIC_TARGETS_MY66000_H
