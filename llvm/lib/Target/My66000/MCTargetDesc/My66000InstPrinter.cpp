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
#include "llvm/MC/MCRegister.h"
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
  case MYCC::EQ0: return "eq0";
  case MYCC::NE0: return "ne0";
  case MYCC::GE0: return "ge0";
  case MYCC::LT0: return "lt0";
  case MYCC::GT0: return "gt0";
  case MYCC::LE0: return "le0";
  case MYCC::DEQ: return "feq";
  case MYCC::DNE: return "fne";
  case MYCC::DGE: return "fge";
  case MYCC::DLT: return "flt";
  case MYCC::DGT: return "fgt";
  case MYCC::DLE: return "fle";
  case MYCC::DOR: return "for";
  case MYCC::DUN: return "fun";
  case MYCC::FEQ: return "feqf";
  case MYCC::FNE: return "fnef";
  case MYCC::FGE: return "fgef";
  case MYCC::FLT: return "fltf";
  case MYCC::FGT: return "fgtf";
  case MYCC::FLE: return "flef";
  case MYCC::FOR: return "forf";
  case MYCC::FUN: return "funf";
  default: return "???";
  }
}

inline static const char *CondBitString(unsigned CC) {
  switch (CC) {
  case MYCB::EQ: return "eq";
  case MYCB::NE: return "ne";
  case MYCB::GE: return "ge";
  case MYCB::LT: return "lt";
  case MYCB::GT: return "gt";
  case MYCB::LE: return "le";
  case MYCB::HS: return "hs";
  case MYCB::LO: return "lo";
  case MYCB::HI: return "hi";
  case MYCB::LS: return "ls";
  case MYCB::SIN: return "sin";
  case MYCB::FIN: return "fin";
  case MYCB::CIN: return "cin";
  case MYCB::RIN: return "rin";
  default: return "???";
  }
}

inline static const char *FCondBitString(unsigned CC) {
  switch (CC) {
  case MYCB::EQ: return "eq";
  case MYCB::NE: return "ne";
  case MYCB::GE: return "ge";
  case MYCB::LT: return "lt";
  case MYCB::GT: return "gt";
  case MYCB::LE: return "le";
  case MYCB::HS: return "hs";		// fabs range compares
  case MYCB::LO: return "lo";		// "
  case MYCB::HI: return "hi";		// "
  case MYCB::LS: return "ls";		// "
  case MYCB::NEQ: return "neq";
  case MYCB::NNE: return "nne";
  case MYCB::NGE: return "nge";
  case MYCB::NLT: return "nlt";
  case MYCB::NGT: return "ngt";
  case MYCB::NLE: return "nle";
  case MYCB::OR: return "or";
  case MYCB::NOR: return "nor";
  case MYCB::TO: return "to";
  case MYCB::NTO: return "nto";
  case MYCB::SNaN: return "snan";
  case MYCB::QNaN: return "qnan";
  case MYCB::MINF: return "minf";
  case MYCB::MNOR: return "mnor";
  case MYCB::MDE: return "mde";
  case MYCB::MZE: return "mze";
  case MYCB::PZE: return "pze";
  case MYCB::PDE: return "pde";
  case MYCB::PNOR: return "pnor";
  case MYCB::NINF: return "ninf";
  default: return "???";
  }
}

void My66000InstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) const {
  OS << StringRef(getRegisterName(Reg)).lower();
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

static void printShadow(raw_ostream &OS, unsigned imm32) {
  unsigned cnt = imm32 >> 16;

  for (unsigned i=0; i <= cnt; i++) {
    OS << (((imm32&1) == 0) ? 'F' : 'T');
    imm32 >>= 1;
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
					raw_ostream &O) {
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
