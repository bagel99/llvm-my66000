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

ArrayRef<Builtin::Info> My66000TargetInfo::getTargetBuiltins() const {
  // FIXME: someday we might need target specific builtins
  return std::nullopt;
}

void My66000TargetInfo::getTargetDefines(const LangOptions &Opts,
                                      MacroBuilder &Builder) const {
  DefineStd(Builder, "unix", Opts);
  defineCPUMacros(Builder, "my66000", /*Tuning=*/false);
  Builder.defineMacro("__ELF__");
}
