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
#include "llvm/Support/Debug.h"
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
  case 8: return "for";
  case 9: return "fun";
  case 10: return "feq";
  case 11: return "fne";
  case 12: return "fge";
  case 13: return "flt";
  case 14: return "fgt";
  case 15: return "fle";
  case 16: return "forf";
  case 17: return "funf";
  case 18: return "feqf";
  case 19: return "fnef";
  case 20: return "fgef";
  case 21: return "fltf";
  case 22: return "fgtf";
  case 23: return "flef";
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

static void printCarryBits(unsigned bits, raw_ostream &O) {

    O << '{';
    while (bits != 0) {
      switch (bits & 3) {
        case 0:
	  O << '-';
	  break;
	case 1:
	  O << 'I';
	  break;
	case 2:
	  O << 'O';
	  break;
	case 3:
	  O << "IO";
	  break;
	}
	bits >>= 2;
	if (bits != 0) O << ',';
    };
    O << '}';
}

static void printShadow(raw_ostream &OS, unsigned imm12) {
  unsigned cnt = (imm12 >> 8) & 7;
  OS << "0,";	// FIXME - perhaps unused field in predication instrs
  for (unsigned i=0; i <= cnt; i++) {
    OS << (((imm12&1) == 0) ? 'F' : 'T');
    imm12 >>= 1;
  }
}

static void printRegList(raw_ostream &O, unsigned imm21) {
  unsigned reg = 0;
  unsigned bit;

    O << '{';
    while (imm21 != 0) {
      ++reg;			// first reg in mask is R1
      bit = imm21 & 1;
      imm21 >>= 1;
      if (bit != 0) {
        O << 'r' << reg;
	if (imm21 != 0) O << ',';
      }
    }
    O << '}';
}

void My66000InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                 StringRef Annot, const MCSubtargetInfo &STI,
				 raw_ostream &O) {
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
  case My66000::CARRYo: {
    O << "\tcarry\t";
    printOperand(MI, 0, O);
    O << ",";
    const MCOperand &Opc = MI->getOperand(1);
    printCarryBits(Opc.getImm(), O);
    }
    break;
  case My66000::CARRYio: {
    O << "\tcarry\t";
    printOperand(MI, 0, O);
    O << ",";
    const MCOperand &Opc = MI->getOperand(2);
    printCarryBits(Opc.getImm(), O);
    }
    break;
  case My66000::PRC: {
    const MCOperand &Opcc = MI->getOperand(0);
    O << "\tp" << CondCodeString(Opcc.getImm()) << "\t";
    printOperand(MI, 1, O);
    O << ",";
    printShadow(O, MI->getOperand(2).getImm());
    }
    break;
  case My66000::PRIB: {
    const MCOperand &Opcc = MI->getOperand(0);
    O << "\tp" << CondBitString(Opcc.getImm()) << "\t";
    printOperand(MI, 1, O);
    O << ",";
    printShadow(O, MI->getOperand(2).getImm());
    }
    break;
  case My66000::PRFB: {
    const MCOperand &Opcc = MI->getOperand(0);
    O << "\tp" << FCondBitString(Opcc.getImm()) << "\t";
    printOperand(MI, 1, O);
    O << ",";
    printShadow(O, MI->getOperand(2).getImm());
    }
    break;
  case My66000::PBIT: {
    O << "\tpb1\t";
    printOperand(MI, 0, O);
    O << ",";
    printOperand(MI, 1, O);
    O << ",";
    printShadow(O, MI->getOperand(2).getImm());
    }
    break;
  case My66000::VEC: {
    O << "\tvec\t";
    printOperand(MI, 0, O);
    O << ",";
    printRegList(O, MI->getOperand(1).getImm());
    }
    break;
  default:
    printInstruction(MI, Address, O);
  }
  printAnnotation(O, Annot);
}

void My66000InstPrinter::
printInlineJT8(const MCInst *MI, unsigned opNum, raw_ostream &O) {
  report_fatal_error("can't handle InlineJT8");
}

void My66000InstPrinter::
printInlineJT16(const MCInst *MI, unsigned opNum, raw_ostream &O) {
  report_fatal_error("can't handle InlineJT16");
}

void My66000InstPrinter::
printInlineJT32(const MCInst *MI, unsigned opNum, raw_ostream &O) {
  report_fatal_error("can't handle InlineJT32");
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
  if (Op.isSFPImm()) {
    O << Op.getSFPImm();
    return;
  }
  if (Op.isDFPImm()) {
    O << Op.getDFPImm();
    return;
  }
  assert(Op.isExpr() && "unknown operand kind in printOperand");
  printExpr(Op.getExpr(), &MAI, O);
}

void My66000InstPrinter::printS16ImmOperand(const MCInst *MI, unsigned OpNo,
                                        raw_ostream &O) {
//dbgs() << "My66000InstPrinter::printS16ImmOperand\n";
  if (MI->getOperand(OpNo).isImm())
    O << (int16_t)MI->getOperand(OpNo).getImm();
  else
    printOperand(MI, OpNo, O);
}

void My66000InstPrinter::printS32ImmOperand(const MCInst *MI, unsigned OpNo,
                                        raw_ostream &O) {
//dbgs() << "My66000InstPrinter::printS16ImmOperand\n";
  if (MI->getOperand(OpNo).isImm())
    O << (int32_t)MI->getOperand(OpNo).getImm();
  else
    printOperand(MI, OpNo, O);
}

void My66000InstPrinter::printFP32Operand(const MCInst *MI, unsigned opNum,
					raw_ostream &O) {
  uint32_t i;
  const MCOperand &Op = MI->getOperand(opNum);
  if (Op.isSFPImm()) {
    i = Op.getSFPImm();
    O << format_hex(i, 9, true);
  }
}

void My66000InstPrinter::printFP64Operand(const MCInst *MI, unsigned opNum,
					raw_ostream &O) {    double   f;
  uint64_t i;
  const MCOperand &Op = MI->getOperand(opNum);
  if (Op.isDFPImm()) {
    i = Op.getDFPImm();
    O << format_hex(i, 18, true);
  }
}

void My66000InstPrinter::printMEMriOperand(const MCInst *MI, unsigned opNum,
                                         raw_ostream &O) {
  printOperand(MI, opNum, O);
  const MCOperand &MO = MI->getOperand(opNum+1);
  if (MO.isImm() && MO.getImm() == 0)
    return;   // don't print ",0"
  O << ",";
  printOperand(MI, opNum+1, O);
}

void My66000InstPrinter::printMEMrrOperand(const MCInst *MI, unsigned opNum,
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

void My66000InstPrinter::printCBOperand(const MCInst *MI, unsigned opNum,
                                         raw_ostream &O) {
  O << CondBitString(MI->getOperand(opNum).getImm());
}

void My66000InstPrinter::printCCOperand(const MCInst *MI, unsigned opNum,
                                         raw_ostream &O) {
  O << CondCodeString(MI->getOperand(opNum).getImm());
}
