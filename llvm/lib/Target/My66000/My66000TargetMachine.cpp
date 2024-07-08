//===-- My66000TargetMachine.cpp - Define TargetMachine for My66000 -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "My66000TargetMachine.h"
#include "MCTargetDesc/My66000MCTargetDesc.h"
#include "TargetInfo/My66000TargetInfo.h"
#include "My66000.h"
#include "My66000MachineFunctionInfo.h"
#include "My66000TargetObjectFile.h"
#include "My66000TargetTransformInfo.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  if (!RM.has_value())
    return Reloc::Static;
  return *RM;
}

static CodeModel::Model
getEffectiveMy66000CodeModel(Optional<CodeModel::Model> CM) {
  if (CM) {
    if (*CM != CodeModel::Large)
      report_fatal_error("Target only supports CodeModel Small or Large");
    return *CM;
  }
  return CodeModel::Small;
}

/// Create an ILP64 architecture model
///
My66000TargetMachine::My66000TargetMachine(const Target &T, const Triple &TT,
				       StringRef CPU, StringRef FS,
				       const TargetOptions &Options,
				       Optional<Reloc::Model> RM,
				       Optional<CodeModel::Model> CM,
				       CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(
	  T,"e-m:e-p:64:64-i1:8-i8:8-i16:16-i32:32-i64:64-f64:64-a:0:64-n64",
	  TT, CPU, FS, Options, getEffectiveRelocModel(RM),
          getEffectiveMy66000CodeModel(CM), OL),
      TLOF(std::make_unique<My66000TargetObjectFile>()),
      Subtarget(TT, std::string(CPU), std::string(FS), *this) {
  initAsmInfo();
}

My66000TargetMachine::~My66000TargetMachine() = default;

namespace {

/// My66000 Code Generator Pass Configuration Options.
class My66000PassConfig : public TargetPassConfig {
public:
  My66000PassConfig(My66000TargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  My66000TargetMachine &getMy66000TargetMachine() const {
    return getTM<My66000TargetMachine>();
  }

  void addIRPasses() override;
  bool addInstSelector() override;
  void addPreRegAlloc() override;
  void addMachineLateOptimization() override;
  void addPreSched2() override;

};

} // end anonymous namespace

TargetPassConfig *My66000TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new My66000PassConfig(*this, PM);
}

MachineFunctionInfo *My66000TargetMachine::createMachineFunctionInfo(
    BumpPtrAllocator &Allocator, const Function &F,
    const TargetSubtargetInfo *STI) const {
  return My66000FunctionInfo::create<My66000FunctionInfo>(Allocator, F, STI);
}

void My66000PassConfig::addIRPasses() {
  addPass(createAtomicExpandPass());

  TargetPassConfig::addIRPasses();
}

bool My66000PassConfig::addInstSelector() {
  addPass(createMy66000ISelDag(getMy66000TargetMachine(), getOptLevel()));
  addPass(createMy66000FixJumpTablePass());
  return false;
}

// Really would like to run Predicate before VVM
void My66000PassConfig::addPreRegAlloc() {
  initializeMy66000VVMLoopPass(*PassRegistry::getPassRegistry());
  insertPass(&RegisterCoalescerID, &My66000VVMLoopID);
}

void My66000PassConfig::addMachineLateOptimization() {
}

// Predication pass must be done after COPY pseudos lowered
void My66000PassConfig::addPreSched2() {
  addPass(createMy66000PredBlockPass());
  addPass(createMy66000VVMFixupPass());
}

// Force static initialization.
extern "C" void LLVMInitializeMy66000Target() {
  RegisterTargetMachine<My66000TargetMachine> X(getTheMy66000Target());
}

TargetTransformInfo
My66000TargetMachine::getTargetTransformInfo(const Function &F) const {
  return TargetTransformInfo(My66000TTIImpl(this, F));
}
