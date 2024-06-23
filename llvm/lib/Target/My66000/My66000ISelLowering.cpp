//===-- My66000ISelLowering.cpp - My66000 DAG Lowering Implementation ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the My66000TargetLowering class.
//
//===----------------------------------------------------------------------===//

#include "My66000ISelLowering.h"
#include "My66000.h"
#include "My66000MachineFunctionInfo.h"
#include "My66000Subtarget.h"
#include "My66000TargetMachine.h"
#include "My66000TargetObjectFile.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

using namespace llvm;

#define DEBUG_TYPE "my66000-lower"

static cl::opt<bool> EnableCarry("enable-carry-generation", cl::Hidden,
    cl::desc("enable the use of the CARRY prefix"), cl::init(false));

static cl::opt<bool> OptimCarry("early-carry-coalesce", cl::Hidden,
    cl::desc("try early carry coalescing"), cl::init(false));

const char *My66000TargetLowering::getTargetNodeName(unsigned Opcode) const {

  switch (Opcode) {
  case My66000ISD::RET: return "My66000ISD::RET";
  case My66000ISD::CALL: return "My66000ISD::CALL";
  case My66000ISD::CALLI: return "My66000ISD::CALLI";
  case My66000ISD::TAIL: return "My66000ISD::TAIL";
  case My66000ISD::TAILI: return "My66000ISD::TAILI";
  case My66000ISD::CMP: return "My66000ISD::CMP";
  case My66000ISD::FCMP: return "My66000ISD::FCMP";
  case My66000ISD::EXT: return "My66000ISD::EXT";
  case My66000ISD::EXTS: return "My66000ISD::EXTS";
  case My66000ISD::CMOV: return "My66000ISD::CMOV";
  case My66000ISD::MUX: return "My66000ISD::MUX";
  case My66000ISD::BRcc: return "My66000ISD::BRcc";
  case My66000ISD::BRfcc: return "My66000ISD::BRfcc";
  case My66000ISD::BRbit: return "My66000ISD::BRbit";
  case My66000ISD::BRcond: return "My66000ISD::BRcond";
  case My66000ISD::JT8: return "My66000ISD::JT8";
  case My66000ISD::JT16: return "My66000ISD::JT16";
  case My66000ISD::JT32: return "My66000ISD::JT32";
  case My66000ISD::MEMCPY: return "My66000ISD::MEMCPY";
  case My66000ISD::MEMSET: return "My66000ISD: MEMSET";
  case My66000ISD::WRAPPER: return "My66000ISD::WRAPPER";
  case My66000ISD::FDIVREM: return "My66000ISD::FDIVREM";
  case My66000ISD::SHRUNK: return "My66000ISD::SHRUNK";
  case My66000ISD::F64I5: return "My66000ISD::F64I5";
  case My66000ISD::F32I5: return "My66000ISD::F32I5";
  }
  return nullptr;
}

static SDValue lowerCallResult(SDValue Chain, SDValue InFlag,
                               const SmallVectorImpl<CCValAssign> &RVLocs,
                               SDLoc dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals);

MVT My66000TargetLowering::getRegisterTypeForCallingConv(LLVMContext &Context,
				CallingConv::ID CC, EVT VT) const {
    if (VT == MVT::f32)
	return MVT::f32;
    return My66000TargetLowering::getRegisterType(Context, VT);
}

My66000TargetLowering::My66000TargetLowering(const TargetMachine &TM,
                                     const My66000Subtarget &Subtarget)
    : TargetLowering(TM), Subtarget(Subtarget) {

  setMinStackArgumentAlignment(Align(8));
  // Set up the register classes.
  addRegisterClass(MVT::i64, &My66000::GRegsRegClass);
  // Floating values use the same registers as integer.
  addRegisterClass(MVT::f64, &My66000::GRegsRegClass);

  addRegisterClass(MVT::f32, &My66000::FSRegsRegClass);

  // Compute derived properties from the register classes
  computeRegisterProperties(Subtarget.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(My66000::SP);

  setSchedulingPreference(Sched::Source);

  // Use i64 for setcc operations results (slt, sgt, ...).
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);

  // Expand all 32-bit operations
  for (unsigned Opc = 0; Opc < ISD::BUILTIN_OP_END; ++Opc)
    setOperationAction(Opc, MVT::i32, Promote);

  // Operations to get us off of the ground.
  // Basic.
  setOperationAction(ISD::ADD, MVT::i64, Legal);
  setOperationAction(ISD::SUB, MVT::i64, Legal);
  setOperationAction(ISD::MUL, MVT::i64, Legal);
  setOperationAction(ISD::AND, MVT::i64, Legal);
  setOperationAction(ISD::SMAX, MVT::i64, Legal);
  setOperationAction(ISD::SMIN, MVT::i64, Legal);
  setOperationAction(ISD::UMAX, MVT::i64, Legal);
  setOperationAction(ISD::UMIN, MVT::i64, Legal);
  setOperationAction(ISD::ABS, MVT::i64, Legal);
  setOperationAction(ISD::SHL, MVT::i64, Legal);
  setOperationAction(ISD::SRA, MVT::i64, Legal);
  setOperationAction(ISD::SRL, MVT::i64, Legal);
  setOperationAction(ISD::ROTR, MVT::i64, Legal);
  setOperationAction(ISD::ROTL, MVT::i64, Legal);
  setOperationAction(ISD::BSWAP, MVT::i64, Legal);
  // We don't have a modulo instruction use div+carry
  setOperationAction(ISD::UREM, MVT::i64, Expand);
  setOperationAction(ISD::SREM, MVT::i64, Expand);
  // We don't have a double length multiply
  setOperationAction(ISD::MULHU, MVT::i64, Expand);
  setOperationAction(ISD::MULHS, MVT::i64, Expand);
  if (!EnableCarry) {
    setOperationAction(ISD::UDIV, MVT::i64, Legal);
    setOperationAction(ISD::SDIV, MVT::i64, Legal);
    setOperationAction(ISD::UDIVREM, MVT::i64, Expand);
    setOperationAction(ISD::SDIVREM, MVT::i64, Expand);
    setOperationAction(ISD::UMUL_LOHI, MVT::i64, Expand);
    setOperationAction(ISD::SMUL_LOHI, MVT::i64, Expand);
  } else {  // Operations that require the CARRY instruction
    setOperationAction(ISD::UDIV, MVT::i64, Expand);
    setOperationAction(ISD::SDIV, MVT::i64, Expand);
    setOperationAction(ISD::UDIVREM, MVT::i64, Legal);
    setOperationAction(ISD::SDIVREM, MVT::i64, Legal);
    setOperationAction(ISD::UMUL_LOHI, MVT::i64, Legal);
    setOperationAction(ISD::SMUL_LOHI, MVT::i64, Legal);
    setOperationAction(ISD::ADDCARRY, MVT::i64, Legal);
    setOperationAction(ISD::SUBCARRY, MVT::i64, Legal);
    setOperationAction(ISD::UADDO, MVT::i64, Legal);
    setOperationAction(ISD::USUBO, MVT::i64, Legal);
  }
  setOperationAction(ISD::BITREVERSE, MVT::i64, Legal);
  // Sign extend inreg
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i32, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Legal);
//  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Custom);
  for (MVT VT : MVT::integer_valuetypes()) {
    setLoadExtAction(ISD::EXTLOAD, VT, MVT::i1, Promote);
    setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i1, Promote);
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i1, Promote);
  }
  setOperationAction(ISD::Constant, MVT::i64, Legal);
  setOperationAction(ISD::UNDEF, MVT::i64, Legal);

  setOperationAction(ISD::LOAD, MVT::i64, Legal);
  setOperationAction(ISD::STORE, MVT::i64, Legal);

  setOperationAction(ISD::SELECT_CC, MVT::i64, Custom);
  setOperationAction(ISD::SETCC, MVT::i64, Custom);
  setOperationAction(ISD::BR_CC, MVT::i64, Custom);
  setOperationAction(ISD::BRCOND, MVT::Other, Expand);
  setOperationAction(ISD::SELECT, MVT::i64, Expand);
  setOperationAction(ISD::BR_JT, MVT::Other, Custom);

  // Have psuedo instruction for frame addresses.
  setOperationAction(ISD::FRAMEADDR, MVT::i64, Legal);
  // Handle the various types of symbolic address.
  setOperationAction(ISD::GlobalAddress, MVT::i64, Custom);
  setOperationAction(ISD::BlockAddress, MVT::i64, Custom);

  // Expand var-args ops.
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);

  // Other expansions
  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i64, Expand);

  // Indexed loads and stores are supported.
  for (unsigned im = (unsigned)ISD::PRE_INC;
       im != (unsigned)ISD::LAST_INDEXED_MODE; ++im) {
    setIndexedLoadAction(im, MVT::i8, Legal);
    setIndexedLoadAction(im, MVT::i16, Legal);
    setIndexedLoadAction(im, MVT::i32, Legal);
    setIndexedLoadAction(im, MVT::i64, Legal);
    setIndexedLoadAction(im, MVT::f32, Legal);
    setIndexedLoadAction(im, MVT::f64, Legal);
    setIndexedStoreAction(im, MVT::i8, Legal);
    setIndexedStoreAction(im, MVT::i16, Legal);
    setIndexedStoreAction(im, MVT::i32, Legal);
    setIndexedStoreAction(im, MVT::i64, Legal);
    setIndexedStoreAction(im, MVT::f32, Legal);
    setIndexedStoreAction(im, MVT::f64, Legal);
  }

  // 64-bit floating point
  setOperationAction(ISD::ConstantFP, MVT::f64, Legal);
  setOperationAction(ISD::FADD, MVT::f64, Legal);
  setOperationAction(ISD::FMUL, MVT::f64, Legal);
  setOperationAction(ISD::FDIV, MVT::f64, Legal);
  setOperationAction(ISD::FMA,  MVT::f64, Legal);	// this or FMAD?
  setOperationAction(ISD::FMAD, MVT::f64, Legal);	// this or FMA
  setOperationAction(ISD::FMINNUM, MVT::f64, Legal);
  setOperationAction(ISD::FMAXNUM, MVT::f64, Legal);
  setOperationAction(ISD::FMINIMUM, MVT::f64, Legal);
  setOperationAction(ISD::FMAXIMUM, MVT::f64, Legal);
  setOperationAction(ISD::FSIN, MVT::f64, Legal);
  setOperationAction(ISD::FCOS, MVT::f64, Legal);
  setOperationAction(ISD::FLOG, MVT::f64, Legal);
  setOperationAction(ISD::FLOG2, MVT::f64, Legal);
  setOperationAction(ISD::FLOG10, MVT::f64, Legal);
  setOperationAction(ISD::FEXP, MVT::f64, Legal);
  setOperationAction(ISD::FEXP2, MVT::f64, Legal);
  setOperationAction(ISD::FFLOOR, MVT::f64, Legal);
  setOperationAction(ISD::FCEIL, MVT::f64, Legal);
  setOperationAction(ISD::FTRUNC, MVT::f64, Legal);
  setOperationAction(ISD::FROUND, MVT::f64, Legal);
  setOperationAction(ISD::FNEARBYINT, MVT::f64, Legal);
  setOperationAction(ISD::FCOPYSIGN, MVT::f64, Legal);
  setOperationAction(ISD::SELECT_CC, MVT::f64, Custom);
  setOperationAction(ISD::SETCC, MVT::f64, Custom);
  setOperationAction(ISD::BR_CC, MVT::f64, Custom);
  setOperationAction(ISD::SELECT, MVT::f64, Expand);
  if (!EnableCarry)
    setOperationAction(ISD::FREM, MVT::f64, Expand);
  else
    setOperationAction(ISD::FREM, MVT::f64, Custom);

  // 32-bit floating point
  setLoadExtAction(ISD::EXTLOAD, MVT::f64, MVT::f32, Expand);
  setTruncStoreAction(MVT::f64, MVT::f32, Expand);
  setOperationAction(ISD::BITCAST, MVT::f32, Legal);
  setOperationAction(ISD::FP_EXTEND, MVT::f64, Legal);
  setOperationAction(ISD::ConstantFP, MVT::f32, Legal);
  setOperationAction(ISD::FADD, MVT::f32, Legal);
  setOperationAction(ISD::FMUL, MVT::f32, Legal);
  setOperationAction(ISD::FDIV, MVT::f32, Legal);
  setOperationAction(ISD::FMA,  MVT::f32, Legal);	// this or FMAD?
  setOperationAction(ISD::FMAD, MVT::f32, Legal);	// this or FMA
  setOperationAction(ISD::FMINNUM, MVT::f32, Legal);
  setOperationAction(ISD::FMAXNUM, MVT::f32, Legal);
  setOperationAction(ISD::FMINIMUM, MVT::f32, Legal);
  setOperationAction(ISD::FMAXIMUM, MVT::f32, Legal);
  setOperationAction(ISD::FSIN, MVT::f32, Legal);
  setOperationAction(ISD::FCOS, MVT::f32, Legal);
  setOperationAction(ISD::FLOG, MVT::f32, Legal);
  setOperationAction(ISD::FLOG2, MVT::f32, Legal);
  setOperationAction(ISD::FLOG10, MVT::f32, Legal);
  setOperationAction(ISD::FEXP, MVT::f32, Legal);
  setOperationAction(ISD::FEXP2, MVT::f32, Legal);
  setOperationAction(ISD::FFLOOR, MVT::f32, Legal);
  setOperationAction(ISD::FCEIL, MVT::f32, Legal);
  setOperationAction(ISD::FTRUNC, MVT::f32, Legal);
  setOperationAction(ISD::FROUND, MVT::f32, Legal);
  setOperationAction(ISD::FNEARBYINT, MVT::f32, Legal);
  setOperationAction(ISD::FCOPYSIGN, MVT::f32, Legal);
  setOperationAction(ISD::SELECT_CC, MVT::f32, Custom);
  setOperationAction(ISD::SETCC, MVT::f32, Custom);
  setOperationAction(ISD::BR_CC, MVT::f32, Custom);
  setOperationAction(ISD::SELECT, MVT::f32, Expand);
  if (!EnableCarry)
    setOperationAction(ISD::FREM, MVT::f32, Expand);
  else
    setOperationAction(ISD::FREM, MVT::f32, Custom);

  setOperationAction(ISD::ConstantFP, MVT::f64, Custom);
  setOperationAction(ISD::ConstantFP, MVT::f32, Custom);

  MaxStoresPerMemcpy = 1;
  MaxStoresPerMemcpyOptSize = 1;
  MaxStoresPerMemmove = 1;
  MaxStoresPerMemmoveOptSize = 1;
  MaxStoresPerMemset = 1;
  MaxStoresPerMemsetOptSize = 1;
}

//===----------------------------------------------------------------------===//
//  Tuning knobs
//===----------------------------------------------------------------------===//
bool My66000TargetLowering::isIntDivCheap(EVT VT, AttributeList Attr) const {
  return true;
}

bool My66000TargetLowering::isFMAFasterThanFMulAndFAdd(const MachineFunction &MF,
						       EVT VT) const {
  VT = VT.getScalarType();

  if (!VT.isSimple())
    return false;
  switch (VT.getSimpleVT().SimpleTy) {
  case MVT::f32:
  case MVT::f64:
    return true;
  default:
    break;
  }
  return false;
}

bool My66000TargetLowering::allowsMisalignedMemoryAccesses(
    EVT VT, unsigned AddrSpace, Align Alignment,
    MachineMemOperand::Flags Flags, unsigned *Fast) const {

    if (Fast != nullptr)
      *Fast = 0;
    return true;
}

// FIXME - do we neet the LLT version of allowsMisalignedMemoryAccesses?

bool My66000TargetLowering::isFPImmLegal(const APFloat &Imm, EVT VT,
                                     bool ForCodeSize) const {
   return true;	// FIXME - just f32 and f64?
}

bool My66000TargetLowering::shouldConvertConstantLoadToIntImm(const APInt &Imm,
                                                          Type *Ty) const {
  assert(Ty->isIntegerTy());

  unsigned BitSize = Ty->getPrimitiveSizeInBits();
  if (BitSize == 0 || BitSize > 64)
    return false;
  return true;
}

bool My66000TargetLowering::reduceSelectOfFPConstantLoads(EVT CmpOpVT) const {
  return false;
}

bool My66000TargetLowering::decomposeMulByConstant(LLVMContext &Context, EVT VT,
                                               SDValue C) const {
  // Check integral scalar types.
  if (!VT.isScalarInteger())
    return false;
// FIXME - how can we do the equivalent of below
//  if (CurDAG->shouldOptForSize())
//    return false;
  if (auto *ConstNode = dyn_cast<ConstantSDNode>(C.getNode())) {
    if (!ConstNode->getAPIntValue().isSignedIntN(64))
      return false;
    uint64_t UImm = static_cast<uint64_t>(ConstNode->getSExtValue());
    if (isPowerOf2_64(UImm + 1) || isPowerOf2_64(UImm - 1) ||
        isPowerOf2_64(1 - UImm) || isPowerOf2_64(-1 - UImm))
      return true;
  }
  return false;
}

//===----------------------------------------------------------------------===//
//  Misc Lower Operation implementation
//===----------------------------------------------------------------------===//

// Map to my condition bits, integer
static MYCB::CondBits ISDCCtoMy66000CBI(ISD:: CondCode CC) {
  switch (CC) {
  default: llvm_unreachable("Unknown integer condition code!");
  case ISD::SETEQ:  return MYCB::EQ;
  case ISD::SETNE:  return MYCB::NE;
  case ISD::SETLT:  return MYCB::LT;
  case ISD::SETGT:  return MYCB::GT;
  case ISD::SETLE:  return MYCB::LE;
  case ISD::SETGE:  return MYCB::GE;
  case ISD::SETULT: return MYCB::LO;
  case ISD::SETULE: return MYCB::LS;
  case ISD::SETUGT: return MYCB::HI;
  case ISD::SETUGE: return MYCB::HS;
  }
}

// Map to my condition bits, float
static MYCB::CondBits ISDCCtoMy66000CBF(ISD:: CondCode CC) {
  switch (CC) {
  default: llvm_unreachable("Unknown condition code!");
  case ISD::SETEQ:  return MYCB::EQ;
  case ISD::SETNE:  return MYCB::NE;
  // float ordered
  case ISD::SETOEQ: return MYCB::EQ;
  case ISD::SETONE: return MYCB::NE;
  case ISD::SETOLT: return MYCB::LT;
  case ISD::SETOLE: return MYCB::LE;
  case ISD::SETOGT: return MYCB::GT;
  case ISD::SETOGE: return MYCB::GE;
  // float unordered
  case ISD::SETUEQ: return MYCB::NNE;
  case ISD::SETUNE: return MYCB::NEQ;
  case ISD::SETULT: return MYCB::NGE;
  case ISD::SETUGT: return MYCB::NLE;
  case ISD::SETULE: return MYCB::NGT;
  case ISD::SETUGE: return MYCB::NLT;
  // float check order
  case ISD::SETO:   return MYCB::OR;
  case ISD::SETUO:  return MYCB::NOR;
  }
}

// Map to my condition codes (used with BRcond)
static MYCC::CondCodes ISDCCtoMy66000CC(ISD:: CondCode CC, EVT VT) {
  if (VT == MVT::f64) {
    switch (CC) {
    default: llvm_unreachable("Unknown f64 condition code!");
    // float unordered
    case ISD::SETUEQ: return MYCC::DEQ;
    case ISD::SETNE:		// is this correct?
    case ISD::SETUNE: return MYCC::DNE;
    case ISD::SETUGE: return MYCC::DGE;
    case ISD::SETLT:
    case ISD::SETULT: return MYCC::DLT;
    case ISD::SETUGT: return MYCC::DGT;
    case ISD::SETLE:
    case ISD::SETULE: return MYCC::DLE;
    // float ordered
    case ISD::SETEQ:		// is this correct?
    case ISD::SETOEQ: return MYCC::DEQ;
    case ISD::SETONE: return MYCC::DNE;
    case ISD::SETGE:
    case ISD::SETOGE: return MYCC::DGE;
    case ISD::SETOLT: return MYCC::DLT;
    case ISD::SETGT:
    case ISD::SETOGT: return MYCC::DGT;
    case ISD::SETOLE: return MYCC::DLE;
    // float check order
    case ISD::SETO:   return MYCC::DOR;
    case ISD::SETUO:  return MYCC::DUN;
    }
  } else if (VT == MVT::f32) {
    switch (CC) {
    default: llvm_unreachable("Unknown f32 condition code!");
    // float unordered
    case ISD::SETUEQ: return MYCC::FEQ;
    case ISD::SETNE:		// is this correct?
    case ISD::SETUNE: return MYCC::FNE;
    case ISD::SETUGE: return MYCC::FGE;
    case ISD::SETLT:
    case ISD::SETULT: return MYCC::FLT;
    case ISD::SETUGT: return MYCC::FGT;
    case ISD::SETLE:
    case ISD::SETULE: return MYCC::FLE;
    // float ordered
    case ISD::SETEQ:		// is this correct?
    case ISD::SETOEQ: return MYCC::FEQ;
    case ISD::SETONE: return MYCC::FNE;
    case ISD::SETGE:
    case ISD::SETOGE: return MYCC::FGE;
    case ISD::SETOLT: return MYCC::FLT;
    case ISD::SETGT:
    case ISD::SETOGT: return MYCC::FGT;
    case ISD::SETOLE: return MYCC::FLE;
    // float check order
    case ISD::SETO:   return MYCC::FOR;
    case ISD::SETUO:  return MYCC::FUN;
    }
  } else {	// assume integer
    switch (CC) {
    default: llvm_unreachable("Unknown integer condition code!");
    case ISD::SETEQ:  return MYCC::EQ0;
    case ISD::SETNE:  return MYCC::NE0;
    case ISD::SETLT:  return MYCC::LT0;
    case ISD::SETGT:  return MYCC::GT0;
    case ISD::SETLE:  return MYCC::LE0;
    case ISD::SETGE:  return MYCC::GE0;
    }
  }
}

/*
 * See if a floating compare involves absolute values or an absolute value
 * and a positive constant.  The My66000 compare bits can deal with this.
 * If found, convert the RHS and LHS(if needed) to remove the fabs().]
 */
static void fabsConversion(SDValue &LHS, SDValue &RHS, MYCB::CondBits &CB,
			   ISD::CondCode CC) {
LLVM_DEBUG(dbgs() << "fabsConversion\n");
    bool inrange = false;
    bool rhsconst = false;
    if (LHS.getNode()->getOpcode() == ISD::FABS) {
      if (RHS.getNode()->getOpcode() == ISD::FABS) {
	inrange = true;
      } else if (const auto *CFP = dyn_cast<ConstantFPSDNode>(RHS)) {
	if (!CFP->isNegative()) {
	  rhsconst = true;
	  inrange = true;
	}
      }
      if (inrange) {
LLVM_DEBUG(dbgs() << "Convert fabs\n");
        switch (CC) {
          case ISD::SETOGE: { CB = MYCB::HS; break; }
	  case ISD::SETOLT: { CB = MYCB::LO; break; }
	  case ISD::SETOGT: { CB = MYCB::HI; break; }
	  case ISD::SETOLE: { CB = MYCB::LS; break; }
          case ISD::SETUGE: { CB = MYCB::HS; break; }
	  case ISD::SETULT: { CB = MYCB::LO; break; }
	  case ISD::SETUGT: { CB = MYCB::HI; break; }
	  case ISD::SETULE: { CB = MYCB::LS; break; }
	  default: inrange = false;
        }
        if (inrange) {
	  LHS = LHS.getOperand(0);
	  if (!rhsconst)
	    RHS = RHS.getOperand(0);
        }
      }
    }
}


// Compare and make result into a boolean
SDValue My66000TargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
  SDLoc dl(Op);
  unsigned inst;
  MYCB::CondBits CB;
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerSETCC\n");
  if (LHS.getValueType().isInteger()) {
    inst = My66000ISD::CMP;
    // Check for cmpne (and ry,(shl 1,rx),0) which is a bit test
    // Turn it into (and (srl ry,rx),1)
    if ((CC == ISD::SETNE) && isNullConstant(RHS) &&
	(LHS.getNode()->getOpcode() == ISD::AND)) {
        SDValue LLHS = LHS.getOperand(0);
        SDValue LRHS = LHS.getOperand(1);
      if ((LRHS.getNode()->getOpcode() == ISD::SHL) &&
	  isOneConstant(LRHS->getOperand(0))) {
	SDValue Shf = DAG.getNode(ISD::SRL, dl, MVT::i64,
		LLHS, LRHS->getOperand(1));
	return DAG.getNode(ISD::AND, dl, MVT::i64,
		Shf, DAG.getConstant(1, dl, MVT::i64));
      }
    }
    if (CC == ISD::SETLT && isOneConstant(RHS)) {
LLVM_DEBUG(dbgs() << "Convert LT 1 into LE 0\n");
	RHS = DAG.getConstant(0, dl, MVT::i64);
	CC = ISD::SETLE;
    } else if (CC == ISD::SETGT && isAllOnesConstant(RHS)) {
LLVM_DEBUG(dbgs() << "Convert GT -1 into GE 0\n");
	RHS = DAG.getConstant(0, dl, MVT::i64);
	CC = ISD::SETGE;
    }
    CB = ISDCCtoMy66000CBI(CC);
  } else {
    inst = My66000ISD::FCMP;
    CB = ISDCCtoMy66000CBF(CC);
    fabsConversion(LHS, RHS, CB, CC);
  }
  SDValue Cmp = DAG.getNode(inst, dl, MVT::i64, LHS, RHS);
  return DAG.getNode(My66000ISD::EXT, dl, MVT::i64, Cmp,
		     DAG.getConstant(1, dl, MVT::i64),
		     DAG.getConstant(CB, dl, MVT::i64));
}

SDValue My66000TargetLowering::LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue TVal = Op.getOperand(2);
  SDValue FVal = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
  SDLoc dl(Op);
  unsigned inst;
  MYCB::CondBits CB;
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerSELECT_CC\n");
  if (LHS.getValueType().isInteger()) {
    if (isNullConstant(RHS) && (CC == ISD::SETEQ || CC == ISD::SETNE)) {
      if (CC == ISD::SETEQ)
        std::swap(TVal, FVal);
      return DAG.getNode(My66000ISD::CMOV, dl, TVal.getValueType(), TVal, FVal, LHS);
    }
    inst = My66000ISD::CMP;
    if ((CC == ISD::SETLT) && isOneConstant(RHS)) {
LLVM_DEBUG(dbgs() << "Convert LT 1 into LE 0\n");
	RHS = DAG.getConstant(0, dl, MVT::i64);
	CC = ISD::SETLE;
    }
    /* If items selected are constants that differ by 1 */
    else if (CC == ISD::SETLT && isNullConstant(RHS) &&
             isa<ConstantSDNode>(TVal) && isa<ConstantSDNode>(FVal)) {
      const APInt &TrueVal = cast<ConstantSDNode>(TVal)->getAPIntValue();
      const APInt &FalseVal = cast<ConstantSDNode>(FVal)->getAPIntValue();
      if (TrueVal - 1 == FalseVal) {
	SDValue Sra = DAG.getNode(ISD::SRL, dl, MVT::i64, LHS,
				  DAG.getConstant(63, dl, MVT::i64));
	return DAG.getNode(ISD::ADD, dl, MVT::i64, Sra, FVal);
      }
      if (TrueVal + 1 == FalseVal) {
	SDValue Sra = DAG.getNode(ISD::SRA, dl, MVT::i64, LHS,
				  DAG.getConstant(63, dl, MVT::i64));
	return DAG.getNode(ISD::ADD, dl, MVT::i64, Sra, FVal);
      }
    }
    CB = ISDCCtoMy66000CBI(CC);
  } else {
    inst = My66000ISD::FCMP;
    CB = ISDCCtoMy66000CBF(CC);
  }
  SDValue Cmp = DAG.getNode(inst, dl, MVT::i64, LHS, RHS);
  SDValue Ext = DAG.getNode(My66000ISD::EXTS, dl, MVT::i64, Cmp,
		     DAG.getConstant(1, dl, MVT::i64),
		     DAG.getConstant(CB, dl, MVT::i64));
  return DAG.getNode(My66000ISD::MUX, dl, TVal.getValueType(), TVal, FVal, Ext);
}

SDValue My66000TargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerBR_CC\n");
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  SDLoc dl(Op);
  EVT VT = LHS.getValueType();
  if (VT.isInteger()) {
    if ((CC == ISD::SETLT) && isOneConstant(RHS)) {
LLVM_DEBUG(dbgs() << "Convert LT 1 into LE 0\n");
	RHS = DAG.getConstant(0, dl, MVT::i64);
	CC = ISD::SETLE;
    }
    if (isNullConstant(RHS)) {
      MYCC::CondCodes cc = ISDCCtoMy66000CC(CC, VT);
      if ((CC == ISD::SETNE || CC == ISD::SETEQ) &&
	  LHS.getOpcode() == ISD::AND &&
          isa<ConstantSDNode>(LHS.getOperand(1)) &&
          isPowerOf2_64(LHS.getConstantOperandVal(1))) { // testing a single bit
	uint64_t Mask = LHS.getConstantOperandVal(1);
	SDValue Test = LHS.getOperand(0);
	unsigned Bit = Log2_64(Mask);
	if (CC == ISD::SETNE) {
	  // Can change BNE(AND x,#<single bit> into BBIT
	  return DAG.getNode(My66000ISD::BRbit, dl, MVT::Other, Chain, Dest,
			     Test,
			     DAG.getConstant(Bit, dl, MVT::i64));
	}
	// Can change BEQ(AND x,#<single bit> into BEQ after extraction
	LHS = DAG.getNode(My66000ISD::EXT, dl, MVT::i64, Test,
			   DAG.getConstant(1, dl, MVT::i64),
			   DAG.getConstant(Bit, dl, MVT::i64));
	cc = MYCC::EQ0;
      }
      return DAG.getNode(My66000ISD::BRcond, dl, MVT::Other, Chain, Dest,
		         LHS, DAG.getConstant(cc, dl, MVT::i64));
    }
    MYCB::CondBits cb = ISDCCtoMy66000CBI(CC);
    SDValue Cmp = DAG.getNode(My66000ISD::CMP, dl, MVT::i64, LHS, RHS);
    return DAG.getNode(My66000ISD::BRcc, dl, MVT::Other, Chain, Dest, Cmp,
                       DAG.getConstant(cb, dl, MVT::i64));
  } else {	// floating point
    if (isNullFPConstant(RHS)) {
      MYCC::CondCodes cc = ISDCCtoMy66000CC(CC, VT);
      return DAG.getNode(My66000ISD::BRcond, dl, MVT::Other, Chain, Dest,
		         LHS, DAG.getConstant(cc, dl, MVT::i64));
    }
    MYCB::CondBits cb = ISDCCtoMy66000CBF(CC);
    fabsConversion(RHS, LHS, cb, CC);
    SDValue Cmp = DAG.getNode(My66000ISD::FCMP, dl, MVT::i64, LHS, RHS);
    return DAG.getNode(My66000ISD::BRfcc, dl, MVT::Other, Chain, Dest, Cmp,
                       DAG.getConstant(cb, dl, MVT::i64));
  }
}

SDValue My66000TargetLowering::LowerSIGN_EXTEND_INREG(SDValue Op,
                                                  SelectionDAG &DAG) const {
  SDValue Op0 = Op.getOperand(0);
  SDLoc dl(Op);
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerSIGN_EXTEND_INREG\n");
  assert(Op.getValueType() == MVT::i64 && "Unhandled target sign_extend_inreg.");
  unsigned Width = cast<VTSDNode>(Op.getOperand(1))->getVT().getSizeInBits();
  return DAG.getNode(My66000::SRAri, dl, MVT::i64, Op0,
		     DAG.getConstant(64 - Width, dl, MVT::i64),
		     DAG.getConstant(64 - Width, dl, MVT::i64));
}

#include "My66000GenCallingConv.inc"

//===----------------------------------------------------------------------===//
//                  Call Calling Convention Implementation
//===----------------------------------------------------------------------===//

static bool canUseTailCall(SmallVectorImpl<CCValAssign> &ArgLocs) {
  // Punt if there are any indirect or stack arguments.
  for (unsigned I = 0, E = ArgLocs.size(); I != E; ++I) {
    CCValAssign &VA = ArgLocs[I];
    if (VA.getLocInfo() == CCValAssign::Indirect)
      return false;
    if (!VA.isRegLoc())
      return false;
  }
  return true;
}

/// My66000 call implementation
SDValue My66000TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                     SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &dl = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  CallingConv::ID CallConv = CLI.CallConv;
  bool IsVarArg = CLI.IsVarArg;
  bool &IsTailCall = CLI.IsTailCall;;
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerCall"
		  << " TailCall=" << CLI.IsTailCall
		  << " VarArg=" << CLI.IsVarArg << '\n');

  MachineFunction &MF = DAG.getMachineFunction();

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());

  CCInfo.AnalyzeCallOperands(Outs, CC_My66000);
  if (IsTailCall && !canUseTailCall(ArgLocs))
    IsTailCall = false;

  SmallVector<CCValAssign, 16> RVLocs;
  // Analyze return values to determine the number of bytes of stack required.
  CCState RetCCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                    *DAG.getContext());
  RetCCInfo.AllocateStack(CCInfo.getNextStackOffset(), Align(8));
  RetCCInfo.AnalyzeCallResult(Ins, RetCC_My66000);

  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NumBytes = RetCCInfo.getNextStackOffset();
  auto PtrVT = getPointerTy(DAG.getDataLayout());

  // Mark the start of the call.
  if (!IsTailCall)
    Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, dl);

  SmallVector<std::pair<unsigned, SDValue>, 4> RegsToPass;
  SmallVector<SDValue, 12> MemOpChains;

  SDValue StackPtr;
  // Walk the register/memloc assignments, inserting copies/loads.
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    SDValue Arg = OutVals[i];

    // Promote the value if needed.
    switch (VA.getLocInfo()) {
    default:
      llvm_unreachable("Unknown loc info!");
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::SIGN_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::ZERO_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::ANY_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    }

    // Arguments that can be passed on register must be kept at
    // RegsToPass vector
    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
    } else {
      assert(VA.isMemLoc() && "Must be register or memory argument.");
      if (!StackPtr.getNode())
        StackPtr = DAG.getCopyFromReg(Chain, dl, My66000::SP,
                                      getPointerTy(DAG.getDataLayout()));
      // Calculate the stack position.
      SDValue SOffset = DAG.getIntPtrConstant(VA.getLocMemOffset(), dl);
      SDValue PtrOff = DAG.getNode(
          ISD::ADD, dl, getPointerTy(DAG.getDataLayout()), StackPtr, SOffset);

      SDValue Store =
          DAG.getStore(Chain, dl, Arg, PtrOff, MachinePointerInfo());
      MemOpChains.push_back(Store);
      IsTailCall = false;
    }
  }

  // Transform all store nodes into one single node because
  // all store nodes are independent of each other.
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, MemOpChains);

  // Build a sequence of copy-to-reg nodes chained together with token
  // chain and flag operands which copy the outgoing args into registers.
  // The InFlag in necessary since all emitted instructions must be
  // stuck together.
  SDValue Glue;
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
    Chain = DAG.getCopyToReg(Chain, dl, RegsToPass[i].first,
                             RegsToPass[i].second, Glue);
    Glue = Chain.getValue(1);
  }

  // If the callee is a GlobalAddress node (quite common, every direct call is)
  // turn it into a TargetGlobalAddress node so that legalize doesn't hack it.
  // Likewise ExternalSymbol -> TargetExternalSymbol.
  bool IsDirect = true;
  if (auto *G = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), dl, MVT::i64);
  else if (auto *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), MVT::i64);
  else
    IsDirect = false;
  // Branch + Link = #chain, #target_address, #opt_in_flags...
  //             = Chain, Callee, Reg#1, Reg#2, ...
  //
  // Returns a chain & a flag for retval copy to use.
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i)
    Ops.push_back(DAG.getRegister(RegsToPass[i].first,
                                  RegsToPass[i].second.getValueType()));

  // Add a register mask operand representing the call-preserved registers.
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
  const uint32_t *Mask = TRI->getCallPreservedMask(MF, CallConv);
  assert(Mask && "Missing call preserved mask for calling convention");
  Ops.push_back(DAG.getRegisterMask(Mask));

  if (Glue.getNode())
    Ops.push_back(Glue);

  if (IsTailCall) {
    MF.getFrameInfo().setHasTailCall();
    return DAG.getNode(IsDirect ? My66000ISD::TAIL : My66000ISD::TAILI, dl,
		       NodeTys, Ops);
  }

  Chain = DAG.getNode(IsDirect ? My66000ISD::CALL : My66000ISD::CALLI, dl,
		      NodeTys, Ops);
  Glue = Chain.getValue(1);

  // Create the CALLSEQ_END node.
  Chain = DAG.getCALLSEQ_END(Chain, DAG.getConstant(NumBytes, dl, PtrVT, true),
                             DAG.getConstant(0, dl, PtrVT, true), Glue, dl);
  Glue = Chain.getValue(1);

  // Handle result values, copying them out of physregs into vregs that we
  // return.
  return lowerCallResult(Chain, Glue, RVLocs, dl, DAG, InVals);
}

/// Lower the result values of a call into the appropriate copies out of
/// physical registers / memory locations.
static SDValue lowerCallResult(SDValue Chain, SDValue Glue,
                               const SmallVectorImpl<CCValAssign> &RVLocs,
                               SDLoc dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) {
  SmallVector<std::pair<int, unsigned>, 4> ResultMemLocs;
LLVM_DEBUG(dbgs() << "My66000TargetLowering::lowerCallResult"
		  << " reg=" << RVLocs.size() << '\n');
  // Copy results out of physical registers.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    const CCValAssign &VA = RVLocs[i];
    if (VA.isRegLoc()) {
      SDValue RetValue;
      RetValue =
          DAG.getCopyFromReg(Chain, dl, VA.getLocReg(), VA.getValVT(), Glue);
      Chain = RetValue.getValue(1);
      Glue = RetValue.getValue(2);
      InVals.push_back(RetValue);
    } else {
      assert(VA.isMemLoc() && "Must be memory location.");
      ResultMemLocs.push_back(
          std::make_pair(VA.getLocMemOffset(), InVals.size()));

      // Reserve space for this result.
      InVals.push_back(SDValue());
    }
  }

  // Copy results out of memory.
  SmallVector<SDValue, 4> MemOpChains;
  for (unsigned i = 0, e = ResultMemLocs.size(); i != e; ++i) {
    int Offset = ResultMemLocs[i].first;
    unsigned Index = ResultMemLocs[i].second;
    SDValue StackPtr = DAG.getRegister(My66000::SP, MVT::i32);
    SDValue SpLoc = DAG.getNode(ISD::ADD, dl, MVT::i32, StackPtr,
                                DAG.getConstant(Offset, dl, MVT::i32));
    SDValue Load =
        DAG.getLoad(MVT::i32, dl, Chain, SpLoc, MachinePointerInfo());
    InVals[Index] = Load;
    MemOpChains.push_back(Load.getValue(1));
  }

  // Transform all loads nodes into one single node because
  // all load nodes are independent of each other.
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, MemOpChains);
LLVM_DEBUG(dbgs() << "\tInVals=" << InVals.size() << '\n');
  return Chain;
}

//===----------------------------------------------------------------------===//
//             Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//

namespace {

struct ArgDataPair {
  SDValue SDV;
  ISD::ArgFlagsTy Flags;
};

} // end anonymous namespace

static const MCPhysReg ArgGPRs[] = {
  My66000::R1, My66000::R2, My66000::R3, My66000::R4,
  My66000::R5, My66000::R6, My66000::R7, My66000::R8
};

/// Transform physical registers into virtual registers, and generate load
/// operations for argument places on the stack.
SDValue My66000TargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &dl,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerFormalArguments\n");

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();
  My66000FunctionInfo *FI = MF.getInfo<My66000FunctionInfo>();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_My66000);

  unsigned StackSlotSize = 8;

  if (!IsVarArg)
    FI->setReturnStackOffset(CCInfo.getNextStackOffset());

  // All getCopyFromReg ops must precede any getMemcpys to prevent the
  // scheduler clobbering a register before it has been copied.
  // The stages are:
  // 1. CopyFromReg (and load) arg & vararg registers.
  // 2. Chain CopyFromReg nodes into a TokenFactor.
  // 3. Memcpy 'byVal' args & push final InVals.
  // 4. Chain mem ops nodes into a TokenFactor.
  SmallVector<SDValue, 4> CFRegNode;
  SmallVector<ArgDataPair, 4> ArgData;
  SmallVector<SDValue, 4> MemOps;

  LLVM_DEBUG(dbgs() << "\tArgLocs.size=" << ArgLocs.size() << '\n');
  // 1a. CopyFromReg (and load) arg registers.
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    SDValue ArgIn;

    if (VA.isRegLoc()) {      // Arguments passed in registers
      EVT RegVT = VA.getLocVT();
      switch (RegVT.getSimpleVT().SimpleTy) {
      default: {
        LLVM_DEBUG(errs() << "LowerFormalArguments Unhandled argument type: "
                          << (unsigned)RegVT.getSimpleVT().SimpleTy << "\n");
        llvm_unreachable("Unhandled LowerFormalArguments type.");
      }
      case MVT::i64:
      case MVT::f64:
      case MVT::f32:
        unsigned VReg = RegInfo.createVirtualRegister(&My66000::GRegsRegClass);
        RegInfo.addLiveIn(VA.getLocReg(), VReg);
        ArgIn = DAG.getCopyFromReg(Chain, dl, VReg, RegVT);
        CFRegNode.push_back(ArgIn.getValue(ArgIn->getNumValues() - 1));
      }
    } else {      		// Arguments passed in memory
      assert(VA.isMemLoc());      // sanity check
      // Load the argument to a virtual register
      unsigned ObjSize = VA.getLocVT().getStoreSize();
      assert((ObjSize <= StackSlotSize) && "Unhandled argument");

      // Create the frame index object for this incoming parameter...
      int FI = MFI.CreateFixedObject(ObjSize, VA.getLocMemOffset(), true);

      // Create the SelectionDAG nodes corresponding to a load
      // from this parameter
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i64);
      ArgIn = DAG.getLoad(VA.getLocVT(), dl, Chain, FIN,
                          MachinePointerInfo::getFixedStack(MF, FI));
    }
    const ArgDataPair ADP = {ArgIn, Ins[i].Flags};
    ArgData.push_back(ADP);
  }

  // CopyFromReg vararg registers.
  if (IsVarArg) {
    // Argument registers
    ArrayRef<MCPhysReg> ArgRegs = ArrayRef(ArgGPRs);
    auto *XFI = MF.getInfo<My66000FunctionInfo>();
    unsigned FirstVAReg = CCInfo.getFirstUnallocated(ArgGPRs);
    LLVM_DEBUG(dbgs() << "\tIsVarArg FirstVAReg=" << FirstVAReg << '\n');
    LLVM_DEBUG(dbgs() << "\tFirstVAReg=" << FirstVAReg << '\n');
    LLVM_DEBUG(dbgs() << "\tlengthof(ArgRegs)=" << ArgRegs.size() << '\n');
    // Save remaining registers possibly containing varags.
    int Offset;
    int VaSaveSize = (ArgRegs.size() - FirstVAReg) * 8;
    if (VaSaveSize == 0)
      Offset = CCInfo.getNextStackOffset();
    else
      Offset = -VaSaveSize;
    // Record the frame index of the first variable argument
    // which is a value necessary to VASTART.
    int VaFI = MFI.CreateFixedObject(8, Offset, true);
    XFI->setVarArgsFrameIndex(VaFI);
    if (VaSaveSize > 0) {
      // FIXME - use STM if more than one
      for (unsigned i = FirstVAReg; i < ArgRegs.size(); i++) {
        // Move argument from phys reg -> virt reg
        unsigned VReg = RegInfo.createVirtualRegister(&My66000::GRegsRegClass);
        RegInfo.addLiveIn(ArgRegs[i], VReg);
        SDValue Val = DAG.getCopyFromReg(Chain, dl, VReg, MVT::i64);
	VaFI = MFI.CreateFixedObject(8, Offset, true);
	SDValue PtrOff = DAG.getFrameIndex(VaFI, MVT::i64);
        // Move argument from virt reg -> stack
        SDValue Store =
            DAG.getStore(Chain, dl, Val, PtrOff, MachinePointerInfo());
        cast<StoreSDNode>(Store.getNode())->getMemOperand()
	    ->setValue((Value *)nullptr);
        MemOps.push_back(Store);
        Offset += 8;
      }
      XFI->setVarArgsSaveSize(VaSaveSize);
    }
  }

  // 2. Chain CopyFromReg nodes into a TokenFactor.
//  if (!CFRegNode.empty())
//    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, CFRegNode);

  // 3. Memcpy 'byVal' args & push final InVals.
  // Aggregates passed "byVal" need to be copied by the callee.
  // The callee will use a pointer to this copy, rather than the original
  // pointer.
  for (const auto &ArgDI : ArgData) {
    if (ArgDI.Flags.isByVal() && ArgDI.Flags.getByValSize()) {
      unsigned Size = ArgDI.Flags.getByValSize();
      Align Alignment = ArgDI.Flags.getNonZeroByValAlign();
      // Create a new object on the stack and copy the pointee into it.
      int FI = MFI.CreateStackObject(Size, Alignment, false);
      SDValue FIN = DAG.getFrameIndex(FI, MVT::i64);
      InVals.push_back(FIN);
      MemOps.push_back(DAG.getMemcpy(
          Chain, dl, FIN, ArgDI.SDV, DAG.getConstant(Size, dl, MVT::i64), Alignment,
          false, false, false, MachinePointerInfo(), MachinePointerInfo()));
    } else {
      InVals.push_back(ArgDI.SDV);
    }
  }

  // 4. Chain mem ops nodes into a TokenFactor.
  if (!MemOps.empty()) {
    MemOps.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, MemOps);
  }

LLVM_DEBUG(dbgs() << "End LowerFormalArguments\n");
  return Chain;
}

//===----------------------------------------------------------------------===//
//               Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

bool My66000TargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
  if (!CCInfo.CheckReturn(Outs, RetCC_My66000))
    return false;
  if (CCInfo.getNextStackOffset() != 0 && IsVarArg)
    return false;
  return true;
}

SDValue
My66000TargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::OutputArg> &Outs,
                               const SmallVectorImpl<SDValue> &OutVals,
                               const SDLoc &dl, SelectionDAG &DAG) const {
  auto *AFI = DAG.getMachineFunction().getInfo<My66000FunctionInfo>();
  MachineFrameInfo &MFI = DAG.getMachineFunction().getFrameInfo();
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerReturn\n");

  // CCValAssign - represent the assignment of
  // the return value to a location
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());

  // Analyze return values.
  if (!IsVarArg)
    CCInfo.AllocateStack(AFI->getReturnStackOffset(), Align(8));

  CCInfo.AnalyzeReturn(Outs, RetCC_My66000);

  SDValue Flag;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  SmallVector<SDValue, 4> MemOpChains;
  // Handle return values that must be copied to memory.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    if (VA.isRegLoc())
      continue;
    assert(VA.isMemLoc());
    if (IsVarArg) {
      report_fatal_error("Can't return value from vararg function in memory");
    }

    int Offset = VA.getLocMemOffset();
    unsigned ObjSize = VA.getLocVT().getStoreSize();
    // Create the frame index object for the memory location.
    int FI = MFI.CreateFixedObject(ObjSize, Offset, false);

    // Create a SelectionDAG node corresponding to a store
    // to this memory location.
    SDValue FIN = DAG.getFrameIndex(FI, MVT::i32);
    MemOpChains.push_back(DAG.getStore(
        Chain, dl, OutVals[i], FIN,
        MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI)));
  }

  // Transform all store nodes into one single node because
  // all stores are independent of each other.
  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, MemOpChains);

  // Now handle return values copied to registers.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    if (!VA.isRegLoc())
      continue;
    // Copy the result values into the output registers.
    Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), OutVals[i], Flag);

    // guarantee that all emitted copies are
    // stuck together, avoiding something bad
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain; // Update chain.

  // Add the flag if we have it.
  if (Flag.getNode())
    RetOps.push_back(Flag);

  // What to do with the RetOps?
  return DAG.getNode(My66000ISD::RET, dl, MVT::Other, RetOps);
}

//===----------------------------------------------------------------------===//
// Target Optimization Hooks
//===----------------------------------------------------------------------===//

SDValue My66000TargetLowering::PerformDAGCombine(SDNode *N,
                                             DAGCombinerInfo &DCI) const {
  return {};
}

//===----------------------------------------------------------------------===//
//  Addressing mode description hooks
//===----------------------------------------------------------------------===//

/// Return true if the addressing mode represented by AM is legal for this
/// target, for a load/store of the specified type.
bool My66000TargetLowering::isLegalAddressingMode(const DataLayout &DL,
                                              const AddrMode &AM, Type *Ty,
                                              unsigned AS,
                                              Instruction *I) const {
  // No global is ever allowed as a base.
  if (AM.BaseGV)
    return false;

  // FIXME - for now, anything else goes
  return true;
}

bool My66000TargetLowering::isOffsetFoldingLegal(
				    const GlobalAddressSDNode *GA) const {
  // For now
  return true;
}


/*
// Don't emit tail calls for the time being.
bool My66000TargetLowering::mayBeEmittedAsTailCall(const CallInst *CI) const {
  return false;
}
*/
SDValue My66000TargetLowering::LowerFRAMEADDR(SDValue Op,
                                            SelectionDAG &DAG) const {
  // This nodes represent llvm.frameaddress on the DAG.
  // It takes one operand, the index of the frame address to return.
  // An index of zero corresponds to the current function's frame address.
  // An index of one to the parent's frame address, and so on.
  // Depths > 0 not supported yet!
  if (cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue() > 0)
    return SDValue();

  MachineFunction &MF = DAG.getMachineFunction();
  const TargetRegisterInfo *RegInfo = Subtarget.getRegisterInfo();
  return DAG.getCopyFromReg(DAG.getEntryNode(), SDLoc(Op),
                            RegInfo->getFrameRegister(MF), MVT::i64);
}

SDValue My66000TargetLowering::LowerGlobalAddress(SDValue Op,
                                              SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerGlobalAddress\n");

  const GlobalAddressSDNode *GN = cast<GlobalAddressSDNode>(Op);
  const GlobalValue *GV = GN->getGlobal();
  int64_t Offset = GN->getOffset();
  SDLoc dl(GN);
  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result;

  // Create the TargetGlobalAddress node, folding in the constant offset.
  Result = DAG.getTargetGlobalAddress(GV, dl, PtrVT, Offset);
  // Wrap it
  return DAG.getNode(My66000ISD::WRAPPER, dl, PtrVT, Result);
}

SDValue My66000TargetLowering::LowerBlockAddress(SDValue Op,
                                                 SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerBlockAddress\n");
  const BlockAddressSDNode *Node = cast<BlockAddressSDNode>(Op);
  SDLoc DL(Node);
  const BlockAddress *BA = Node->getBlockAddress();
  int64_t Offset = Node->getOffset();
  auto PtrVT = getPointerTy(DAG.getDataLayout());

  SDValue Result = DAG.getTargetBlockAddress(BA, PtrVT, Offset);
  Result = DAG.getNode(My66000ISD::WRAPPER, DL, PtrVT, Result);
  return Result;
}

unsigned My66000TargetLowering::getJumpTableEncoding() const {
  return MachineJumpTableInfo::EK_Inline;
}

SDValue My66000TargetLowering::LowerBR_JT(SDValue Op,
                                              SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerBR_JT\n");
  SDValue Chain = Op.getOperand(0);
  SDValue Table = Op.getOperand(1);
  SDValue Index = Op.getOperand(2);
  SDLoc DL(Op);
  JumpTableSDNode *JT = cast<JumpTableSDNode>(Table);
  unsigned JTI = JT->getIndex();
  MachineFunction &MF = DAG.getMachineFunction();
  const MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
  SDValue TargetJT = DAG.getTargetJumpTable(JT->getIndex(), MVT::i64);

  unsigned NumEntries = MJTI->getJumpTables()[JTI].MBBs.size();
//dbgs() << "NumEntries=" << NumEntries << '\n';
  SDValue Size = DAG.getConstant(NumEntries, DL, MVT::i64);
  // The width of the table entries really doesn't depend on the number
  // of entries.  It depends more on the total size of the basic blocks
  // to which the entries refer.  The basic blocks could be reordered,
  // say sorted by size, to minimize the width of the entries.
  // All of this is punted until later.
  // For now, 8-bit entries aren't very useful.
  unsigned OpCode = (NumEntries <= 1024) ? My66000ISD::JT16 :
		    My66000ISD::JT32;
  // The default target will be replaced in the FixJumpTable pass
  const auto &MBBs = MJTI->getJumpTables()[JTI].MBBs;
  SDValue DefMBB = DAG.getBasicBlock(*MBBs.begin());
  return DAG.getNode(OpCode, DL, MVT::Other, Chain, TargetJT, Index, Size, DefMBB);
}

SDValue My66000TargetLowering::LowerVASTART(SDValue Op,
                                              SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerVASTART\n");
  MachineFunction &MF = DAG.getMachineFunction();
  auto *XFI = MF.getInfo<My66000FunctionInfo>();
  SDLoc DL(Op);
  SDValue FI = DAG.getFrameIndex(XFI->getVarArgsFrameIndex(),
                                 getPointerTy(MF.getDataLayout()));

  // vastart just stores the address of the VarArgsFrameIndex slot into the
  // memory location argument.
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0), DL, FI, Op.getOperand(1),
                      MachinePointerInfo(SV));
}

SDValue My66000TargetLowering::lowerFRAMEADDR(SDValue Op,
                                            SelectionDAG &DAG) const {
  const My66000RegisterInfo &RI = *Subtarget.getRegisterInfo();
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setFrameAddressIsTaken(true);
  Register FrameReg = RI.getFrameRegister(MF);
  int XLenInBytes = 8;

  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  SDValue FrameAddr = DAG.getCopyFromReg(DAG.getEntryNode(), DL, FrameReg, VT);
  unsigned Depth = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
  while (Depth--) {
    int Offset = -(XLenInBytes * 2);
    SDValue Ptr = DAG.getNode(ISD::ADD, DL, VT, FrameAddr,
                              DAG.getIntPtrConstant(Offset, DL));
    FrameAddr =
        DAG.getLoad(VT, DL, DAG.getEntryNode(), Ptr, MachinePointerInfo());
  }
  return FrameAddr;
}

SDValue My66000TargetLowering::lowerRETURNADDR(SDValue Op,
                                             SelectionDAG &DAG) const {
  const My66000RegisterInfo &RI = *Subtarget.getRegisterInfo();
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MFI.setReturnAddressIsTaken(true);
  int XLenInBytes = 8;

  if (verifyReturnAddressArgumentIsConstant(Op, DAG))
    return SDValue();

  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  unsigned Depth = cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
  if (Depth) {
    int Off = -XLenInBytes;
    SDValue FrameAddr = lowerFRAMEADDR(Op, DAG);
    SDValue Offset = DAG.getConstant(Off, DL, VT);
    return DAG.getLoad(VT, DL, DAG.getEntryNode(),
                       DAG.getNode(ISD::ADD, DL, VT, FrameAddr, Offset),
                       MachinePointerInfo());
  }

  // Return the value of the return address register, marking it an implicit
  // live-in.
  Register Reg = MF.addLiveIn(RI.getRARegister(), getRegClassFor(MVT::i64));
  return DAG.getCopyFromReg(DAG.getEntryNode(), DL, Reg, MVT::i64);
}

SDValue My66000TargetLowering::lowerFREM(SDValue Op,
				         SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerFREM\n");
  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDVTList VTs = DAG.getVTList(VT, VT);
  return DAG.getNode(My66000ISD::FDIVREM, DL, VTs, LHS, RHS).getValue(1);
}

static bool CanShrinkToI5(APFloat FPVal, int64_t &imm) {
APSInt IVal(64, false);
bool isExact;
FPVal.convertToInteger(IVal, APFloat::rmTowardZero, &isExact);
LLVM_DEBUG(dbgs() << "\tAttempt shrink to i5: " << isExact <<
    ", IVal=" << IVal << '\n');
  if (isExact) {
    imm = IVal.getExtValue();
    if (imm >= -31 && imm <= 31)
      return true;
  }
  return false;
}

/*
 * Convert all 64-bit floating point constants that have a 32-bit
 * exact representation to 32-bit constant extended.  We will have patterns to
 * match this.  In addition, attempt to shrink to IMM5.
 */
SDValue My66000TargetLowering::LowerConstantFP(SDValue Op,
						SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerConstantFP\n");
  EVT VT = Op.getValueType();
  SDLoc DL(Op);
  ConstantFPSDNode *CFP = cast<ConstantFPSDNode>(Op);
  APFloat FPVal = CFP->getValueAPF();
  int64_t imm;
  if (VT == MVT::f64) {
    if (CanShrinkToI5(FPVal, imm)) {
      return DAG.getNode(My66000ISD::F64I5, DL, MVT::f64,
			     DAG.getConstant(imm, DL, MVT::i64));
    }
    APFloat FPVal2 = FPVal;	// convert clobbers it
    bool losesInfo;
    // The following copied from ConstantFP::isValueValidForType()
    FPVal2.convert(APFloat::IEEEsingle(), APFloat::rmNearestTiesToEven, &losesInfo);
LLVM_DEBUG(dbgs() << "\tAttempt shrink to f32: " << losesInfo << '\n');
    if (!losesInfo) {
      // FPVal can be represented by a f32
      return DAG.getNode(My66000ISD::SHRUNK, DL, MVT::f64,
			 DAG.getConstantFP(FPVal2, DL, MVT::f32));
    }
  }
  else if (VT == MVT::f32) {
    if (CanShrinkToI5(FPVal, imm)) {
      return DAG.getNode(My66000ISD::F32I5, DL, MVT::f32,
			     DAG.getConstant(imm, DL, MVT::i64));
    }
  }
  return DAG.getConstantFP(FPVal, DL, VT);
}

SDValue My66000TargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::LowerOperation: ");
LLVM_DEBUG(Op.dump());
  switch (Op.getOpcode()) {
  case ISD::BR_CC:			return LowerBR_CC(Op, DAG);
  case ISD::SELECT_CC:			return LowerSELECT_CC(Op, DAG);
  case ISD::SETCC:			return LowerSETCC(Op, DAG);
  case ISD::SIGN_EXTEND_INREG:		return LowerSIGN_EXTEND_INREG(Op, DAG);
  case ISD::GlobalAddress:		return LowerGlobalAddress(Op, DAG);
  case ISD::BlockAddress:		return LowerBlockAddress(Op, DAG);
  case ISD::BR_JT:			return LowerBR_JT(Op, DAG);
  case ISD::VASTART:			return LowerVASTART(Op, DAG);
  case ISD::FRAMEADDR:			return lowerFRAMEADDR(Op, DAG);
  case ISD::RETURNADDR:			return lowerRETURNADDR(Op, DAG);
  case ISD::ConstantFP:			return LowerConstantFP(Op, DAG);
  case ISD::FREM:			return lowerFREM(Op, DAG);
  default:
    llvm_unreachable("unimplemented operand");
  }
}

//===----------------------------------------------------------------------===//
//  Custom instruction emit
//===----------------------------------------------------------------------===//
//
// An ADDCARRY is being emitted, so there must have been a
// UADD0 that was emitted previously.  Search back to find
// the CARRYo instruction that resulted.  Modify the immediate
// to include the ADD instruction now being emitted, if they
// are close enought together.
static bool adjustFirstCarry(MachineInstr &MI, MachineBasicBlock *BB,
				    unsigned imm, unsigned &reg) {
  if (!OptimCarry) return false;
  MachineBasicBlock::iterator I = MI;
  I--;		// backup to before the ADDCARRY
  unsigned n = 0;

  while (I->getOpcode() != My66000::CARRYo) {
    n += 1;
    if (I == BB->begin()) {
LLVM_DEBUG(dbgs() << "did not find FirstCarry n=" << n << '\n');
	return false;
    }
    I--;
  }
  unsigned old = I->getOperand(1).getImm();
  unsigned chg = (imm << n*2) | old;
LLVM_DEBUG(dbgs() << "found FirstCarry n=" << n << " old=" << old << '\n');
  if (n > 8) return false;
  reg = I->getOperand(0).getReg();
  // Replace the immediate operand(1). Is there a better way to do this?
  I->removeOperand(1);
  I->addOperand(MachineOperand::CreateImm(chg));
  return true;
}

static MachineBasicBlock *emitADDCARRY(MachineInstr &MI,
                                       MachineBasicBlock *BB, unsigned inst) {
  MachineFunction &MF = *BB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  unsigned Sum = MI.getOperand(0).getReg();
  unsigned CO = MI.getOperand(1).getReg();
  unsigned LHS = MI.getOperand(2).getReg();
  unsigned CI = MI.getOperand(4).getReg();
  unsigned Reg;
LLVM_DEBUG(dbgs() << "emitADDCARRY\n" << MI << '\n');
  MachineRegisterInfo &MRI = BB->getParent()->getRegInfo();
  unsigned CarryFlag = MRI.use_empty(CO) ? 1: 3;
  if (!adjustFirstCarry(MI, BB, CarryFlag, Reg)) {
    MachineRegisterInfo &MRI = BB->getParent()->getRegInfo();
    unsigned CA = MRI.createVirtualRegister(&My66000::GRegsRegClass);
    BuildMI(*BB, MI, DL, TII.get(My66000::CARRYio), CA)
	    .addReg(CI).addImm(CarryFlag);
  }
  MachineInstr *Add =
    BuildMI(*BB, MI, DL, TII.get(inst), Sum)
	    .addReg(LHS).add(MI.getOperand(3))
	    .addReg(CI, RegState::Implicit)
	    .addReg(CO, RegState::ImplicitDefine);
  Add->tieOperands(4, 3);
  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return BB;
}

static MachineBasicBlock *emitUADDO(MachineInstr &MI,
                                    MachineBasicBlock *BB, unsigned inst) {
  MachineFunction &MF = *BB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  unsigned Sum = MI.getOperand(0).getReg();
  unsigned CO = MI.getOperand(1).getReg();
  unsigned LHS = MI.getOperand(2).getReg();
LLVM_DEBUG(dbgs() << "emitUADD0\n" << MI << '\n');
  BuildMI(*BB, MI, DL, TII.get(My66000::CARRYo), CO)
      .addImm(2);	// Out
  BuildMI(*BB, MI, DL, TII.get(inst), Sum)
      .addReg(LHS).add(MI.getOperand(3))
      .addReg(CO, RegState::Implicit);
  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return BB;
}

static MachineBasicBlock *emitUMULHILO(MachineInstr &MI,
                                       MachineBasicBlock *BB, unsigned inst) {
  MachineFunction &MF = *BB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  // ISD::UMUL_LOHI is defined to return the low half first
  unsigned LO = MI.getOperand(0).getReg();
  unsigned HI = MI.getOperand(1).getReg();
  unsigned LHS = MI.getOperand(2).getReg();
  MachineRegisterInfo &MRI = BB->getParent()->getRegInfo();
  unsigned CI = MRI.createVirtualRegister(&My66000::GRegsRegClass);
LLVM_DEBUG(dbgs() << "emitUMULHILO\n" << MI << '\n');

  BuildMI(*BB, MI, DL, TII.get(My66000::CARRYo), CI)
      .addImm(2);	// Out
  MachineInstr *Mul;
  Mul = BuildMI(*BB, MI, DL, TII.get(inst), LO)
	.addReg(LHS).add(MI.getOperand(3))
	.addReg(CI, RegState::Implicit)
	.addReg(HI, RegState::ImplicitDefine);
  Mul->tieOperands(4, 3);
  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return BB;
}

static MachineBasicBlock *emitDIVREM(MachineInstr &MI,
                                       MachineBasicBlock *BB, unsigned inst) {
  MachineFunction &MF = *BB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  unsigned DIV = MI.getOperand(0).getReg();
  unsigned REM = MI.getOperand(1).getReg();
  unsigned CI;
  MachineRegisterInfo &MRI = BB->getParent()->getRegInfo();
  bool REMunused = MRI.use_empty(REM);
  MachineInstr *Div;
LLVM_DEBUG(dbgs() << "emitUDIVREM\n" << MI << '\n');
  if (!REMunused) {
    CI = MRI.createVirtualRegister(&My66000::GRegsRegClass);
    BuildMI(*BB, MI, DL, TII.get(My66000::CARRYo), CI)
	.addImm(2);	// Out
  }
  Div = BuildMI(*BB, MI, DL, TII.get(inst), DIV)
	.add(MI.getOperand(2)).add(MI.getOperand(3));
  if (!REMunused) {	// deal with the CARRY instruction
    Div->addOperand(MachineOperand::CreateReg(CI, false, true));
    Div->addOperand(MachineOperand::CreateReg(REM, true, true));
    Div->tieOperands(4, 3);
  }
  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return BB;
}

static MachineBasicBlock *emitFREM(MachineInstr &MI,
				   MachineBasicBlock *BB, unsigned inst) {
  MachineFunction &MF = *BB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  unsigned DIV = MI.getOperand(0).getReg();
  unsigned REM = MI.getOperand(1).getReg();

  unsigned CO;
  MachineRegisterInfo &MRI = BB->getParent()->getRegInfo();
  MachineInstr *Div;
LLVM_DEBUG(dbgs() << "emitFREM\n" << MI << '\n');
  CO = MRI.createVirtualRegister(&My66000::GRegsRegClass);
  BuildMI(*BB, MI, DL, TII.get(My66000::CARRYo), CO).addImm(2);	// Out
  Div = BuildMI(*BB, MI, DL, TII.get(inst), DIV)
	.add(MI.getOperand(2)).add(MI.getOperand(3));
    Div->addOperand(MachineOperand::CreateReg(CO, false, true));
    Div->addOperand(MachineOperand::CreateReg(REM, true, true));
    Div->tieOperands(4, 3);
  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return BB;
}


// Double length shifts using CARRY
static MachineBasicBlock *emitSHF2(MachineInstr &MI,
                                       MachineBasicBlock *BB, unsigned inst) {
  MachineFunction &MF = *BB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  unsigned RESLO = MI.getOperand(0).getReg();
  unsigned RESHI = MI.getOperand(1).getReg();
  unsigned INLO  = MI.getOperand(2).getReg();
  unsigned INHI  = MI.getOperand(3).getReg();

  BuildMI(*BB, MI, DL, TII.get(My66000::CARRYio), RESHI)
	.addReg(INHI).addImm(3); //In/Out
  BuildMI(*BB, MI, DL, TII.get(inst), RESLO).addReg(INLO).add(MI.getOperand(4));
  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return BB;
}


MachineBasicBlock *My66000TargetLowering::EmitInstrWithCustomInserter(
			MachineInstr &MI,
			MachineBasicBlock *BB) const {
LLVM_DEBUG(dbgs() << "My66000TargetLowering::EmitInstrWithCustomInserter\n");
  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("Unexpected instr type to insert");
  case My66000::UADDOrr:	return emitUADDO(MI, BB, My66000::ADDrr);
  case My66000::USUBOrr:	return emitUADDO(MI, BB, My66000::ADDrn);
  case My66000::UADDOri:	return emitUADDO(MI, BB, My66000::ADDri);
  case My66000::USUBOri:	return emitUADDO(MI, BB, My66000::ADDri);
  case My66000::ADDCARRYrr:	return emitADDCARRY(MI, BB, My66000::ADDrr);
  case My66000::SUBCARRYrr:	return emitADDCARRY(MI, BB, My66000::ADDrn);
  case My66000::ADDCARRYri:	return emitADDCARRY(MI, BB, My66000::ADDri);
  case My66000::SUBCARRYri:	return emitADDCARRY(MI, BB, My66000::ADDri);
  case My66000::UMULHILOrr:	return emitUMULHILO(MI, BB, My66000::MULrr);
  case My66000::UMULHILOri:	return emitUMULHILO(MI, BB, My66000::MULri);
  case My66000::UMULHILOrw:	return emitUMULHILO(MI, BB, My66000::MULrw);
  case My66000::UMULHILOrd:	return emitUMULHILO(MI, BB, My66000::MULrd);
  case My66000::SMULHILOrr:	return emitUMULHILO(MI, BB, My66000::MULrr);
  case My66000::SMULHILOri:	return emitUMULHILO(MI, BB, My66000::MULri);
  case My66000::SMULHILOrw:	return emitUMULHILO(MI, BB, My66000::MULrw);
  case My66000::SMULHILOrd:	return emitUMULHILO(MI, BB, My66000::MULrd);
  case My66000::UDIVREMrr:	return emitDIVREM(MI, BB, My66000::UDIVrr);
  case My66000::UDIVREMri:	return emitDIVREM(MI, BB, My66000::UDIVri);
  case My66000::UDIVREMrw:	return emitDIVREM(MI, BB, My66000::UDIVrw);
  case My66000::UDIVREMwr:	return emitDIVREM(MI, BB, My66000::UDIVwr);
  case My66000::UDIVREMrd:	return emitDIVREM(MI, BB, My66000::UDIVrd);
  case My66000::UDIVREMdr:	return emitDIVREM(MI, BB, My66000::UDIVdr);
  case My66000::SDIVREMrr:	return emitDIVREM(MI, BB, My66000::SDIVrr);
  case My66000::SDIVREMrn:	return emitDIVREM(MI, BB, My66000::SDIVrn);
  case My66000::SDIVREMnr:	return emitDIVREM(MI, BB, My66000::SDIVnr);
  case My66000::SDIVREMnn:	return emitDIVREM(MI, BB, My66000::SDIVnn);
  case My66000::SDIVREMrx:	return emitDIVREM(MI, BB, My66000::SDIVrx);
  case My66000::SDIVREMwr:	return emitDIVREM(MI, BB, My66000::SDIVwr);
  case My66000::SDIVREMrd:	return emitDIVREM(MI, BB, My66000::SDIVrd);
  case My66000::SDIVREMdr:	return emitDIVREM(MI, BB, My66000::SDIVdr);
  case My66000::SRL2rr:		return emitSHF2(MI, BB, My66000::SRLrr);
  case My66000::SLL2rr:		return emitSHF2(MI, BB, My66000::SLLrr);
  case My66000::SRA2rr:		return emitSHF2(MI, BB, My66000::SRArr);
  case My66000::FREMrr:		return emitFREM(MI, BB, My66000::FDIVrr);
  case My66000::FREMrd:		return emitFREM(MI, BB, My66000::FDIVrd);
  case My66000::FREMrf:		return emitFREM(MI, BB, My66000::FDIVrf);
  case My66000::FREMrk:		return emitFREM(MI, BB, My66000::FDIVrk);
  case My66000::FREMdr:		return emitFREM(MI, BB, My66000::FDIVdr);
  case My66000::FREMfr:		return emitFREM(MI, BB, My66000::FDIVfr);
  case My66000::FREMkr:		return emitFREM(MI, BB, My66000::FDIVkr);
  case My66000::FREMFrr:	return emitFREM(MI, BB, My66000::FDIVFrr);
  case My66000::FREMFrf:	return emitFREM(MI, BB, My66000::FDIVFrf);
  case My66000::FREMFrk:	return emitFREM(MI, BB, My66000::FDIVFrk);
  case My66000::FREMFfr:	return emitFREM(MI, BB, My66000::FDIVFfr);
  case My66000::FREMFkr:	return emitFREM(MI, BB, My66000::FDIVFkr);
  }
}

//===----------------------------------------------------------------------===//
//  Inline ASM support
//===----------------------------------------------------------------------===//
My66000TargetLowering::ConstraintType
My66000TargetLowering::getConstraintType(StringRef Constraint) const {
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    default:
      break;
    case 'I':
    case 'J':
    case 'K':
      return C_Immediate;
    }
  }
  return TargetLowering::getConstraintType(Constraint);
}

std::pair<unsigned, const TargetRegisterClass *>
My66000TargetLowering::getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                                                  StringRef Constraint,
                                                  MVT VT) const {
  // First, see if this is a constraint that directly corresponds to a
  // RISCV register class.
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    case 'r':
      return std::make_pair(0U, &My66000::GRegsRegClass);
    default:
      break;
    }
  }
  return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);
}

void My66000TargetLowering::LowerAsmOperandForConstraint(
    SDValue Op, std::string &Constraint, std::vector<SDValue> &Ops,
    SelectionDAG &DAG) const {
  // Currently only support length 1 constraints.
  if (Constraint.length() == 1) {
    switch (Constraint[0]) {
    case 'I':
      // Validate & create a 16-bit signed immediate operand.
      if (auto *C = dyn_cast<ConstantSDNode>(Op)) {
        uint64_t CVal = C->getSExtValue();
        if (isInt<16>(CVal))
          Ops.push_back(
              DAG.getTargetConstant(CVal, SDLoc(Op), MVT::i64));
      }
      return;
    case 'J':
      // Validate & create a 5-bit signed integer zero operand.
      if (auto *C = dyn_cast<ConstantSDNode>(Op)) {
        uint64_t CVal = C->getSExtValue();
        if (isInt<5>(CVal))
          Ops.push_back(
              DAG.getTargetConstant(CVal, SDLoc(Op), MVT::i64));
      }
      return;
    case 'K':
      // Validate & create a 6-bit unsigned immediate operand.
      if (auto *C = dyn_cast<ConstantSDNode>(Op)) {
        uint64_t CVal = C->getZExtValue();
        if (isUInt<6>(CVal))
          Ops.push_back(
              DAG.getTargetConstant(CVal, SDLoc(Op), MVT::i64));
      }
      return;
    default:
      break;
    }
  }
  TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops, DAG);
}
