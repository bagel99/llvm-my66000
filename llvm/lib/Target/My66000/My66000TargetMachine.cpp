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
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

static Reloc::Model getEffectiveRelocModel(std::optional<Reloc::Model> RM) {
  if (!RM.has_value())
    return Reloc::Static;
  return *RM;
}

static CodeModel::Model
getEffectiveMy66000CodeModel(std::optional<CodeModel::Model> CM) {
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
				       std::optional<Reloc::Model> RM,
				       std::optional<CodeModel::Model> CM,
				       CodeGenOptLevel OL, bool JIT)
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
  void addMachineSSAOptimization() override;
  void addMachineLateOptimization() override;
  void addPreSched2() override;
  void addPreEmitPass() override;
};

} // end anonymous namespace

TargetPassConfig *My66000TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new My66000PassConfig(*this, PM);
}

MachineFunctionInfo *My66000TargetMachine::createMachineFunctionInfo(
    BumpPtrAllocator &Allocator, const Function &F,
    const TargetSubtargetInfo *STI) const {
  return My66000MachineFunctionInfo::create<My66000MachineFunctionInfo>(Allocator, F, STI);
}

void My66000PassConfig::addIRPasses() {
  addPass(createAtomicExpandPass());
  addPass(createCFGSimplificationPass(SimplifyCFGOptions()
					.speculateBlocks(false)));

  TargetPassConfig::addIRPasses();

  addPass(createSelectOptimizePass());

}

bool My66000PassConfig::addInstSelector() {
  addPass(createMy66000ISelDag(getMy66000TargetMachine(), getOptLevel()));
  addPass(createMy66000FixJumpTablePass());
  return false;
}

void My66000PassConfig::addMachineSSAOptimization() {
  TargetPassConfig::addMachineSSAOptimization();
  addPass(createMy66000OptWInstrsPass());
}

void My66000PassConfig::addMachineLateOptimization() {
  // Cleanup of redundant immediate/address loads.
  addPass(&MachineLateInstrsCleanupID);

  // Branch folding must be run after regalloc and prolog/epilog insertion.
  addPass(&BranchFolderPassID);

  // Tail duplication.
  // Note that duplicating tail just increases code size and degrades
  // performance for targets that require Structured Control Flow.
  // In addition it can also make CFG irreducible. Thus we disable it.
//  if (!TM->requiresStructuredCFG())
    addPass(&TailDuplicateID);

  // Copy propagation.
  // FIXME - this breaks things, deletes returned value
//  addPass(&MachineCopyPropagationID);
}

// Predication pass must be done after COPY pseudos lowered
void My66000PassConfig::addPreSched2() {
  addPass(createMy66000ExpandPseudoPass());
  addPass(createMy66000PredBlockPass());
}

void My66000PassConfig::addPreEmitPass() {
  // Machine Block Placement might have created new VVM opportunities.
  addPass(createMy66000VVMLoopPass());
}

// Force static initialization.
extern "C" void LLVMInitializeMy66000Target() {
  RegisterTargetMachine<My66000TargetMachine> X(getTheMy66000Target());
}

TargetTransformInfo
My66000TargetMachine::getTargetTransformInfo(const Function &F) const {
  return TargetTransformInfo(My66000TTIImpl(this, F));
}
