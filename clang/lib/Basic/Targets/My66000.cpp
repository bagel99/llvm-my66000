//===--- My66000.cpp - Implement My66000 target feature support -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements My66000 TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "My66000.h"
#include "Targets.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/MacroBuilder.h"
#include "clang/Basic/TargetBuiltins.h"

using namespace clang;
using namespace clang::targets;

ArrayRef<const char *> My66000TargetInfo::getGCCRegNames() const {
  static const char *const GCCRegNames[] = {
      "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
      "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
      "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
      "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"};
  return llvm::ArrayRef(GCCRegNames);
}

ArrayRef<Builtin::Info> My66000TargetInfo::getTargetBuiltins() const {
  // FIXME: someday we might need target specific builtins
  return std::nullopt;
}

void My66000TargetInfo::getTargetDefines(const LangOptions &Opts,
                                      MacroBuilder &Builder) const {
  // Target identification.
  Builder.defineMacro("__my66000__");
  Builder.defineMacro("__LP64__");
  DefineStd(Builder, "unix", Opts);
  defineCPUMacros(Builder, "my66000", /*Tuning=*/false);
  Builder.defineMacro("__ELF__");
}
