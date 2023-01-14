//===-- My66000MCTargetDesc.cpp - My66000 Target Descriptions -----------------===//
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

#include "MCTargetDesc/My66000MCTargetDesc.h"
#include "MCTargetDesc/My66000InstPrinter.h"
#include "MCTargetDesc/My66000MCAsmInfo.h"
#include "TargetInfo/My66000TargetInfo.h"
#include "My66000TargetStreamer.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "My66000GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "My66000GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "My66000GenRegisterInfo.inc"

static MCInstrInfo *createMy66000MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitMy66000MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createMy66000MCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitMy66000MCRegisterInfo(X, My66000::R0);	// FIXME
  return X;
}

static MCSubtargetInfo *
createMy66000MCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  return createMy66000MCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/ CPU, FS);
}

static MCAsmInfo *createMy66000MCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
				       const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new My66000MCAsmInfo(TT);

  // Initial state of the frame pointer is SP.
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(nullptr, My66000::SP, 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCInstPrinter *createMy66000MCInstPrinter(const Triple &T,
						 unsigned SyntaxVariant,
						 const MCAsmInfo &MAI,
						 const MCInstrInfo &MII,
						 const MCRegisterInfo &MRI) {
  return new My66000InstPrinter(MAI, MII, MRI);
}

My66000TargetStreamer::My66000TargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

My66000TargetStreamer::~My66000TargetStreamer() = default;

namespace {

class My66000TargetAsmStreamer : public My66000TargetStreamer {
  formatted_raw_ostream &OS;

public:
  My66000TargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);

  void emitCCTopData(StringRef Name) override;
  void emitCCTopFunction(StringRef Name) override;
  void emitCCBottomData(StringRef Name) override;
  void emitCCBottomFunction(StringRef Name) override;
};

} // end anonymous namespace

My66000TargetAsmStreamer::My66000TargetAsmStreamer(MCStreamer &S,
                                               formatted_raw_ostream &OS)
    : My66000TargetStreamer(S), OS(OS) {}

void My66000TargetAsmStreamer::emitCCTopData(StringRef Name) {
//  OS << "\t.cc_top " << Name << ".data," << Name << '\n';
}

void My66000TargetAsmStreamer::emitCCTopFunction(StringRef Name) {
//  OS << "\t.cc_top " << Name << ".function," << Name << '\n';
}

void My66000TargetAsmStreamer::emitCCBottomData(StringRef Name) {
//  OS << "\t.cc_bottom " << Name << ".data\n";
}

void My66000TargetAsmStreamer::emitCCBottomFunction(StringRef Name) {
//  OS << "\t.cc_bottom " << Name << ".function\n";
}

static MCTargetStreamer *createTargetAsmStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS,
                                                 MCInstPrinter *InstPrint,
                                                 bool isVerboseAsm) {
  return new My66000TargetAsmStreamer(S, OS);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMy66000TargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfoFn X(getTheMy66000Target(), createMy66000MCAsmInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(getTheMy66000Target(),
                                      createMy66000MCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(getTheMy66000Target(),
                                    createMy66000MCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(getTheMy66000Target(),
                                          createMy66000MCSubtargetInfo);

  // Register the MCInstPrinter
  TargetRegistry::RegisterMCInstPrinter(getTheMy66000Target(),
                                        createMy66000MCInstPrinter);

  TargetRegistry::RegisterAsmTargetStreamer(getTheMy66000Target(),
                                            createTargetAsmStreamer);
}
