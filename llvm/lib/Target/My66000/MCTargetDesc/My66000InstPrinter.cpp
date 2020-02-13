//===-- My66000InstPrinter.cpp - Convert My66000 MCInst to assembly syntax ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class prints an My66000 MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "My66000InstPrinter.h"
#include "My66000.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "My66000GenAsmWriter.inc"

inline static const char *CondCodeString(unsigned CC) {
  switch (CC) {
  case 0: return "nm";
  case 1: return "nn";
  case 2: return "eq0";
  case 3: return "ne0";
  case 4: return "ge0";
  case 5: return "gt0";
  case 6: return "le0";
  case 7: return "lt0";
  default: return "???";
  }
}

inline static const char *CondBitString(unsigned CC) {
  switch (CC) {
  case 0: return "ne";
  case 1: return "eq";
  case 2: return "gt";
  case 3: return "ge";
  case 4: return "lt";
  case 5: return "le";
  case 8: return "ne";
  case 9: return "eq";
  case 10: return "hi";
  case 11: return "hs";
  case 12: return "lo";
  case 13: return "ls";
  case 20: return "sin";
  case 21: return "fin";
  case 22: return "cin";
  case 23: return "rin";
  case 34: return "s1nm";
  case 36: return "s1n";
  case 38: return "s1z";
  case 39: return "s1p";
  case 40: return "s1sm";
  case 41: return "s1um";
  default: return "???";
  }
}

inline static const char *FCondBitString(unsigned CC) {
  switch (CC) {
  case 0: return "ne";
  case 1: return "eq";
  case 2: return "gt";
  case 3: return "ge";
  case 4: return "lt";
  case 5: return "le";
  case 6: return "or";
  case 7: return "un";
  case 8: return "nne";
  case 9: return "neq";
  case 10: return "ngt";
  case 11: return "nge";
  case 12: return "nlt";
  case 13: return "nle";
  case 20: return "sin";
  case 21: return "fin";
  case 22: return "cin";
  case 23: return "rin";
  case 32: return "f1s";
  case 33: return "f1q";
  case 34: return "f1pi";
  case 35: return "f1pn";
  case 36: return "f1pd";
  case 37: return "f1pz";
  case 38: return "f1nz";
  case 39: return "f1nd";
  case 40: return "f1nn";
  case 41: return "f1ni";
  default: return "???";
  }
}

void My66000InstPrinter::printRegName(raw_ostream &OS, unsigned RegNo) const {
  OS << StringRef(getRegisterName(RegNo)).lower();
}

void My66000InstPrinter::printInst(const MCInst *MI, raw_ostream &O,
                                 StringRef Annot, const MCSubtargetInfo &STI) {
  switch (MI->getOpcode()) {
  case My66000::BRC: {
    const MCOperand &Opcc = MI->getOperand(2);
    O << "\tb" << CondCodeString(Opcc.getImm()) << "\t";
    printOperand(MI, 1, O);
    O << ",";
    printOperand(MI, 0, O);
    }
    break;
  case My66000::BRIB: {
    const MCOperand &Opcc = MI->getOperand(2);
    O << "\tb" << CondBitString(Opcc.getImm()) << "\t";
    printOperand(MI, 1, O);
    O << ",";
    printOperand(MI, 0, O);
    }
    break;
  case My66000::BRFB: {
    const MCOperand &Opcc = MI->getOperand(2);
    O << "\tb" << FCondBitString(Opcc.getImm()) << "\t";
    printOperand(MI, 1, O);
    O << ",";
    printOperand(MI, 0, O);
    }
    break;
  default:
    printInstruction(MI, O);
  }
  printAnnotation(O, Annot);
}


static void printExpr(const MCExpr *Expr, const MCAsmInfo *MAI,
                      raw_ostream &OS) {
  int Offset = 0;
  const MCSymbolRefExpr *SRE;

  if (const MCBinaryExpr *BE = dyn_cast<MCBinaryExpr>(Expr)) {
    SRE = dyn_cast<MCSymbolRefExpr>(BE->getLHS());
    const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(BE->getRHS());
    assert(SRE && CE && "Binary expression must be sym+const.");
    Offset = CE->getValue();
  } else {
    SRE = dyn_cast<MCSymbolRefExpr>(Expr);
    assert(SRE && "Unexpected MCExpr type.");
  }
  assert(SRE->getKind() == MCSymbolRefExpr::VK_None);

  SRE->getSymbol().print(OS, MAI);

  if (Offset) {
    if (Offset > 0)
      OS << '+';
    OS << Offset;
  }
}

void My66000InstPrinter::
printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    printRegName(O, Op.getReg());
    return;
  }
  if (Op.isImm()) {
    O << Op.getImm();
    return;
  }
  if (Op.isFPImm()) {
    O << Op.getFPImm();
    return;
  }
  assert(Op.isExpr() && "unknown operand kind in printOperand");
  printExpr(Op.getExpr(), &MAI, O);
}

void My66000InstPrinter::printFP64Operand(const MCInst *MI, int opNum,
					raw_ostream &O) {
  union {
    double   f;
    uint64_t i;
  } u;
  const MCOperand &Op = MI->getOperand(opNum);
  if (Op.isFPImm()) {
    u.f = Op.getFPImm();
    O << format_hex(u.i, 18, true);
  }
}

void My66000InstPrinter::printMEMriOperand(const MCInst *MI, int opNum,
                                         raw_ostream &O) {
  printOperand(MI, opNum, O);
  const MCOperand &MO = MI->getOperand(opNum+1);
  if (MO.isImm() && MO.getImm() == 0)
    return;   // don't print ",0"
  O << ",";
  printOperand(MI, opNum+1, O);
}

void My66000InstPrinter::printMEMrrOperand(const MCInst *MI, int opNum,
                                         raw_ostream &O) {
  const MCOperand &Op0 = MI->getOperand(opNum);
  if (Op0.isReg() && Op0.getReg() == My66000::R0)
    O << "ip";
  else
    printOperand(MI, opNum, O);	// base reg
  const MCOperand &Op1 = MI->getOperand(opNum+1);
  if (Op1.isReg() && Op1.getReg() != My66000::R0) {
    O << ",";
    printOperand(MI, opNum+1, O);	// index reg
    const MCOperand &Op2 = MI->getOperand(opNum+2);
    if (Op2.isImm() && Op2.getImm() != 0) {
      O << "<<";
      printOperand(MI, opNum+2, O);	// shift amt
    }
  }
  O << ",";
  printOperand(MI, opNum+3, O);	// offset
}

