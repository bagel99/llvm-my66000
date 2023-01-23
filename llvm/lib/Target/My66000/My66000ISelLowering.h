//===- My66000ISelLowering.h - My66000 DAG Lowering Interface ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that My66000 uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MY66000ISELLOWERING_H
#define LLVM_LIB_TARGET_MY66000_MY66000ISELLOWERING_H

#include "My66000.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

// Forward delcarations
class My66000Subtarget;
class My66000TargetMachine;

namespace My66000ISD {

enum NodeType : unsigned {
  // Start the numbering where the builtin ops and target ops leave off.
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  RET,		// Return with a flag operand. Operand 0 is the chain operand.
  CALL,		// Branch and link (direct call)
  CALLI,	// Branch and link thru register (indirect call)
  TAIL,		// Tail call
  TAILI,	// Tail call (indirect)
  CMP,		// CMP
  FCMP,		// Floating CMP
  EXT,		// Extract zero extended
  EXTS,		// Extract sign extended
  CMOV,		// Conditional move
  MUX,		// Multiplex instruction
  BRcc,		// Branch on bit set by CMP
  BRfcc,	// Branch on bit set by FCMP
  BRbit,	// Branch on bit not set by a compare
  BRcond,	// Branch compare with zero
  JT8,		// Jump through table, 8 bit
  JT16,		// Jump through table, 16 bit
  JT32,		// Jump through table, 32 bit
  MEMCPY,	// Memory copy
  WRAPPER,	// prefix for global address
  SHRUNK,	// wrapper for F64 constants shrunk to F32
  F64I5,	// wrapper for F64 constants that fit in I5
  F32I5		// wrapper for F32 constants that fit in I5
};

} // end namespace My66000ISD

//===--------------------------------------------------------------------===//
// TargetLowering Implementation
//===--------------------------------------------------------------------===//
class My66000TargetLowering : public TargetLowering {
 public:
  explicit My66000TargetLowering(const TargetMachine &TM,
			const My66000Subtarget &Subtarget);

  unsigned getJumpTableEncoding() const override;

  /// Provide custom lowering hooks for some operations.
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  // getTargetNodeName - This method returns the name of a target specific DAG node.
  const char *getTargetNodeName(unsigned Opcode) const override;
  bool isLegalAddressingMode(const DataLayout &DL, const AddrMode &AM,
			Type *Ty, unsigned AS,
			Instruction *I = nullptr) const override;
  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override;

  /// Returns true if the target allows unaligned memory accesses of the
  /// specified type.
  bool allowsMisalignedMemoryAccesses(
      EVT VT, unsigned AddrSpace = 0, Align Alignment = Align(1),
      MachineMemOperand::Flags Flags = MachineMemOperand::MONone,
      bool *Fast = nullptr) const override;

  ConstraintType getConstraintType(StringRef Constraint) const override;

//  unsigned getInlineAsmMemConstraint(StringRef ConstraintCode) const override;

  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               StringRef Constraint, MVT VT) const override;

  void LowerAsmOperandForConstraint(SDValue Op, std::string &Constraint,
                                    std::vector<SDValue> &Ops,
                                    SelectionDAG &DAG) const override;

  MachineBasicBlock *EmitInstrWithCustomInserter(MachineInstr &MI,
			MachineBasicBlock *BB) const override;

  // Override default assumptions
  MVT getRegisterTypeForCallingConv(LLVMContext &Context, CallingConv::ID CC,
                                      EVT VT) const override;

  Register
  getExceptionPointerRegister(const Constant *PersonalityFn) const override {
    return My66000::R1;
  }
  Register
  getExceptionSelectorRegister(const Constant *PersonalityFn) const override {
    return My66000::R2;
  }

 private:
  const My66000Subtarget &Subtarget;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_JT(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSIGN_EXTEND_INREG(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerConstantFP(SDValue Op, SelectionDAG &DAG) const;

  SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

  // Lower Operand helpers
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
			bool isVarArg,
			const SmallVectorImpl<ISD::InputArg> &Ins,
			const SDLoc &dl, SelectionDAG &DAG,
			SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
			SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
			const SmallVectorImpl<ISD::OutputArg> &Outs,
			const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl,
			SelectionDAG &DAG) const override;
  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
			bool isVarArg,
			const SmallVectorImpl<ISD::OutputArg> &ArgsFlags,
			LLVMContext &Context) const override;

  // Tuning knobs
  bool isIntDivCheap(EVT VT, AttributeList Attr) const override;
  bool isFMAFasterThanFMulAndFAdd(const MachineFunction &MF, EVT VT)
					    const override;
  bool isFPImmLegal(const APFloat &Imm, EVT VT, bool ForCodeSize)
					    const override;
  bool shouldConvertConstantLoadToIntImm(const APInt &Imm,Type *Ty)
					    const override;
  bool reduceSelectOfFPConstantLoads(EVT CmpOpVT) const override;

};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_MY66000_MY66000ISELLOWERING_H
