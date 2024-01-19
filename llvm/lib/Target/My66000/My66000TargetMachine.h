//===-- My66000TargetMachine.h - Define TargetMachine for My66000 ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the My66000 specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MY66000TARGETMACHINE_H
#define LLVM_LIB_TARGET_MY66000_MY66000TARGETMACHINE_H

#include "My66000Subtarget.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"
#include <memory>

namespace llvm {

class My66000TargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  My66000Subtarget Subtarget;

public:
  My66000TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                     StringRef FS, const TargetOptions &Options,
                     Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                     CodeGenOpt::Level OL, bool JIT);
  ~My66000TargetMachine() override;

  const My66000Subtarget *getSubtargetImpl() const { return &Subtarget; }
  const My66000Subtarget *getSubtargetImpl(const Function &) const override {
    return &Subtarget;
  }

  // Pass Pipeline Configuration
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetTransformInfo getTargetTransformInfo(const Function &F) const override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }

  MachineFunctionInfo *
  createMachineFunctionInfo(BumpPtrAllocator &Allocator, const Function &F,
                            const TargetSubtargetInfo *STI) const override;

};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_MY66000_MY66000TARGETMACHINE_H
