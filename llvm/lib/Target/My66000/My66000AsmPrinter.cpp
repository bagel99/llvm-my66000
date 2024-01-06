//===-- My66000AsmPrinter.cpp - My66000 LLVM assembly writer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to the XAS-format My66000 assembly language.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/My66000InstPrinter.h"
#include "TargetInfo/My66000TargetInfo.h"
#include "My66000.h"
#include "My66000InstrInfo.h"
#include "My66000MCInstLower.h"
#include "My66000Subtarget.h"
#include "My66000TargetMachine.h"
#include "My66000TargetStreamer.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include <algorithm>
#include <cctype>
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {
  class My66000AsmPrinter : public AsmPrinter {
    My66000MCInstLower MCInstLowering;
    My66000TargetStreamer &getTargetStreamer();

  public:
    explicit My66000AsmPrinter(TargetMachine &TM,
                             std::unique_ptr<MCStreamer> Streamer)
        : AsmPrinter(TM, std::move(Streamer)), MCInstLowering(*this) {}

    StringRef getPassName() const override { return "My66000 Assembly Printer"; }

    void printInlineJT(const MachineInstr *MI, raw_ostream &O,
		const std::string inst, const std::string &table);
    void printOperand(const MachineInstr *MI, int opNum, raw_ostream &O);
    bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                         const char *ExtraCode, raw_ostream &O) override;
    bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNum,
                               const char *ExtraCode, raw_ostream &O) override;

    void emitGlobalVariable(const GlobalVariable *GV) override;

    void emitFunctionEntryLabel() override;
    void emitInstruction(const MachineInstr *MI) override;
    void emitFunctionBodyStart() override;
    void emitFunctionBodyEnd() override;
  };
} // end of anonymous namespace

My66000TargetStreamer &My66000AsmPrinter::getTargetStreamer() {
  return static_cast<My66000TargetStreamer&>(*OutStreamer->getTargetStreamer());
}

void My66000AsmPrinter::emitGlobalVariable(const GlobalVariable *GV) {
  // Check to see if this is a special global used by LLVM, if so, emit it.
  if (!GV->hasInitializer() || emitSpecialLLVMGlobal(GV))
    return;

  const DataLayout &DL = getDataLayout();
  OutStreamer->switchSection(getObjFileLowering().SectionForGlobal(GV, TM));

  MCSymbol *GVSym = getSymbol(GV);
  const Constant *C = GV->getInitializer();
  const Align Alignment(DL.getPrefTypeAlignment(C->getType()));

  // Mark the start of the global
  getTargetStreamer().emitCCTopData(GVSym->getName());

  switch (GV->getLinkage()) {
  case GlobalValue::AppendingLinkage:
    report_fatal_error("AppendingLinkage is not supported by this target!");
  case GlobalValue::LinkOnceAnyLinkage:
  case GlobalValue::LinkOnceODRLinkage:
  case GlobalValue::WeakAnyLinkage:
  case GlobalValue::WeakODRLinkage:
  case GlobalValue::ExternalLinkage:
  case GlobalValue::CommonLinkage:
    OutStreamer->emitSymbolAttribute(GVSym, MCSA_Global);

    if (GV->hasWeakLinkage() || GV->hasLinkOnceLinkage() ||
        GV->hasCommonLinkage())
      OutStreamer->emitSymbolAttribute(GVSym, MCSA_Weak);
    LLVM_FALLTHROUGH;
  case GlobalValue::InternalLinkage:
  case GlobalValue::PrivateLinkage:
    break;
  default:
    llvm_unreachable("Unknown linkage type!");
  }

  emitAlignment(std::max(Alignment, Align(4)), GV);

  unsigned Size = DL.getTypeAllocSize(C->getType());
  if (MAI->hasDotTypeDotSizeDirective()) {
    OutStreamer->emitSymbolAttribute(GVSym, MCSA_ELF_TypeObject);
    OutStreamer->emitELFSize(GVSym, MCConstantExpr::create(Size, OutContext));
  }
  OutStreamer->emitLabel(GVSym);

  emitGlobalConstant(DL, C);
  // The ABI requires that unsigned scalar types smaller than 32 bits
  // are padded to 32 bits.
  if (Size < 4)
    OutStreamer->emitZeros(4 - Size);

  // Mark the end of the global
  getTargetStreamer().emitCCBottomData(GVSym->getName());
}

void My66000AsmPrinter::emitFunctionBodyStart() {
  MCInstLowering.Initialize(&MF->getContext());
}

/// EmitFunctionBodyEnd - Targets can override this to emit stuff after
/// the last basic block in the function.
void My66000AsmPrinter::emitFunctionBodyEnd() {
  // Emit function end directives
  getTargetStreamer().emitCCBottomFunction(CurrentFnSym->getName());
}

void My66000AsmPrinter::emitFunctionEntryLabel() {
  // Mark the start of the function
  getTargetStreamer().emitCCTopFunction(CurrentFnSym->getName());
  OutStreamer->emitLabel(CurrentFnSym);
}

void My66000AsmPrinter::printOperand(const MachineInstr *MI, int opNum,
                                   raw_ostream &O) {
  const DataLayout &DL = getDataLayout();
  const MachineOperand &MO = MI->getOperand(opNum);
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    O << My66000InstPrinter::getRegisterName(MO.getReg());
    break;
  case MachineOperand::MO_Immediate:
    O << MO.getImm();
    break;
  case MachineOperand::MO_MachineBasicBlock:
    MO.getMBB()->getSymbol()->print(O, MAI);
    break;
  case MachineOperand::MO_GlobalAddress:
    PrintSymbolOperand(MO, O);
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    O << DL.getPrivateGlobalPrefix() << "CPI" << getFunctionNumber() << '_'
      << MO.getIndex();
    break;
  case MachineOperand::MO_BlockAddress:
    GetBlockAddressSymbol(MO.getBlockAddress())->print(O, MAI);
    break;
  default:
    llvm_unreachable("not implemented");
  }
}

/// PrintAsmOperand - Print out an operand for an inline asm expression.
///
bool My66000AsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                      const char *ExtraCode, raw_ostream &O) {
  // Print the operand if there is no operand modifier.
  if (!ExtraCode || !ExtraCode[0]) {
    printOperand(MI, OpNo, O);
    return false;
  }

  // Otherwise fallback on the default implementation.
  return AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, O);
}

bool My66000AsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                            unsigned OpNum,
                                            const char *ExtraCode,
                                            raw_ostream &O) {
  if (ExtraCode && ExtraCode[0]) {
    return true; // Unknown modifier.
  }
  printOperand(MI, OpNum, O);
  O << '[';
  printOperand(MI, OpNum + 1, O);
  O << ']';
  return false;
}

void My66000AsmPrinter::printInlineJT(const MachineInstr *MI,
		raw_ostream &O, const std::string inst, const std::string &table) {
  unsigned JTI = MI->getOperand(0).getIndex();
  const MachineFunction *MF = MI->getParent()->getParent();
  const MachineJumpTableInfo *MJTI = MF->getJumpTableInfo();
  const std::vector<MachineJumpTableEntry> &JT = MJTI->getJumpTables();
  const std::vector<MachineBasicBlock*> &JTBBs = JT[JTI].MBBs;
  O << "\t" << inst << "\t";
  printOperand(MI, 1, O);
  O << ",#";
  printOperand(MI, 2, O);
  O << "\n\t" << table << "\t";
  MachineBasicBlock *MBB;
  for (unsigned i = 0, e = JTBBs.size(); i != e; ++i) {
    MBB = JTBBs[i];
    MBB->getSymbol()->print(O, MAI);
    O << ",";
  }
  printOperand(MI, 3, O);	// default branch target
  O << "\n\t.p2align 2";
}

void My66000AsmPrinter::emitInstruction(const MachineInstr *MI) {
  SmallString<128> Str;
  raw_svector_ostream O(Str);

  switch (MI->getOpcode()) {
  case My66000::JT8:
    printInlineJT(MI, O, "jttb", ".jt8");
    OutStreamer->emitRawText(O.str());
    return;
  case My66000::JT16:
    printInlineJT(MI, O, "jtth", ".jt16");
    OutStreamer->emitRawText(O.str());
    return;
  case My66000::JT32:
    printInlineJT(MI, O, "jttw", ".jt32");
    OutStreamer->emitRawText(O.str());
    return;
  }

  MachineBasicBlock::const_instr_iterator I = MI->getIterator();
  MachineBasicBlock::const_instr_iterator E = MI->getParent()->instr_end();
  do {
    MCInst TmpInst;
    MCInstLowering.Lower(&*I, TmpInst);
    EmitToStreamer(*OutStreamer, TmpInst);
  } while ((++I != E) && I->isInsideBundle());
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMy66000AsmPrinter() {
  RegisterAsmPrinter<My66000AsmPrinter> X(getTheMy66000Target());
}
