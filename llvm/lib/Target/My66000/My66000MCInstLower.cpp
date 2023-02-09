//===-- My66000MCInstLower.cpp - Convert My66000 MachineInstr to MCInst -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains code to lower My66000 MachineInstrs to their
/// corresponding MCInst records.
///
//===----------------------------------------------------------------------===//
#include "My66000MCInstLower.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

My66000MCInstLower::My66000MCInstLower(class AsmPrinter &asmprinter)
    : Printer(asmprinter) {}

void My66000MCInstLower::Initialize(MCContext *C) { Ctx = C; }

MCOperand My66000MCInstLower::LowerSymbolOperand(const MachineOperand &MO,
                                               MachineOperandType MOTy,
                                               unsigned Offset) const {
  MCSymbolRefExpr::VariantKind Kind = MCSymbolRefExpr::VK_None;
  const MCSymbol *Symbol;

  switch (MOTy) {
    case MachineOperand::MO_MachineBasicBlock:
      Symbol = MO.getMBB()->getSymbol();
      break;
    case MachineOperand::MO_GlobalAddress:
      Symbol = Printer.getSymbol(MO.getGlobal());
      Offset += MO.getOffset();
      break;
    case MachineOperand::MO_BlockAddress:
      Symbol = Printer.GetBlockAddressSymbol(MO.getBlockAddress());
      Offset += MO.getOffset();
      break;
    case MachineOperand::MO_ExternalSymbol:
      Symbol = Printer.GetExternalSymbolSymbol(MO.getSymbolName());
      Offset += MO.getOffset();
      break;
    case MachineOperand::MO_JumpTableIndex:
      Symbol = Printer.GetJTISymbol(MO.getIndex());
      break;
    case MachineOperand::MO_ConstantPoolIndex:
      Symbol = Printer.GetCPISymbol(MO.getIndex());
      Offset += MO.getOffset();
      break;
    default:
      llvm_unreachable("<unknown operand type>");
  }

  const MCSymbolRefExpr *MCSym = MCSymbolRefExpr::create(Symbol, Kind, *Ctx);

  if (!Offset)
    return MCOperand::createExpr(MCSym);

  // Assume offset is never negative.
  assert(Offset > 0);

  const MCConstantExpr *OffsetExpr =  MCConstantExpr::create(Offset, *Ctx);
  const MCBinaryExpr *Add = MCBinaryExpr::createAdd(MCSym, OffsetExpr, *Ctx);
  return MCOperand::createExpr(Add);
}

MCOperand My66000MCInstLower::LowerOperand(const MachineOperand &MO,
                                         unsigned offset) const {
  MachineOperandType MOTy = MO.getType();

  switch (MOTy) {
    default: llvm_unreachable("unknown operand type");
    case MachineOperand::MO_Register:
      // Ignore all implicit register operands.
      if (MO.isImplicit()) break;
      return MCOperand::createReg(MO.getReg());
    case MachineOperand::MO_Immediate:
      return MCOperand::createImm(MO.getImm() + offset);
    case MachineOperand::MO_FPImmediate: {
      const ConstantFP *Imm = MO.getFPImm();
      const uint64_t BitPattern =
          Imm->getValueAPF().bitcastToAPInt().getZExtValue();
      if (Imm->getType()->isFloatTy())
        return MCOperand::createSFPImm(static_cast<uint32_t>(BitPattern));
      else if (Imm->getType()->isDoubleTy())
        return MCOperand::createDFPImm(BitPattern);
      else
        llvm_unreachable("unknown floating point immediate type");
      break;
    }
    case MachineOperand::MO_MachineBasicBlock:
    case MachineOperand::MO_GlobalAddress:
    case MachineOperand::MO_ExternalSymbol:
    case MachineOperand::MO_JumpTableIndex:
    case MachineOperand::MO_ConstantPoolIndex:
    case MachineOperand::MO_BlockAddress:
      return LowerSymbolOperand(MO, MOTy, offset);
    case MachineOperand::MO_RegisterMask:
      break;
  }

  return MCOperand();
}

void My66000MCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    MCOperand MCOp = LowerOperand(MO);

    if (MCOp.isValid())
      OutMI.addOperand(MCOp);
  }
}
