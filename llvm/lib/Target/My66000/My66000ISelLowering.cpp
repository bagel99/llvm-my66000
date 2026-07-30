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
#include "llvm/ADT/APSInt.h"
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
  case My66000ISD::RORW: return "My66000ISD::RORW";
  case My66000ISD::ROLW: return "My66000ISD::ROLW";
  case My66000ISD::CMOV: return "My66000ISD::CMOV";
  case My66000ISD::BRcc: return "My66000ISD::BRcc";
  case My66000ISD::BRfcc: return "My66000ISD::BRfcc";
  case My66000ISD::BRbit1: return "My66000ISD::BRbit1";
  case My66000ISD::BRbit0: return "My66000ISD::BRbit0";
  case My66000ISD::BRcond: return "My66000ISD::BRcond";
  case My66000ISD::JT8: return "My66000ISD::JT8";
  case My66000ISD::JT16: return "My66000ISD::JT16";
  case My66000ISD::JT32: return "My66000ISD::JT32";
  case My66000ISD::MEMCPY: return "My66000ISD::MEMCPY";
  case My66000ISD::MEMSET: return "My66000ISD: MEMSET";
  case My66000ISD::WRAPPER: return "My66000ISD::WRAPPER";
  case My66000ISD::FDIVREM: return "My66000ISD::FDIVREM";
  case My66000ISD::COPYFMFS: return "My66000ISD::COPYFMFS";
  case My66000ISD::COPYTOFS: return "My66000ISD::COPYTOFS";
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
    : TargetLowering(TM, Subtarget), TM(TM), Subtarget(Subtarget) {

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
  setOperationAction(ISD::UDIV, MVT::i64, Legal);
  setOperationAction(ISD::SDIV, MVT::i64, Legal);
  // We don't have a modulo instruction use div+carry
  setOperationAction(ISD::UREM, MVT::i64, Expand);
  setOperationAction(ISD::SREM, MVT::i64, Expand);
  // We don't have a double length multiply
  setOperationAction(ISD::MULHU, MVT::i64, Expand);
  setOperationAction(ISD::MULHS, MVT::i64, Expand);
  if (!EnableCarry) {
    setOperationAction(ISD::UDIVREM, MVT::i64, Expand);
    setOperationAction(ISD::SDIVREM, MVT::i64, Expand);
    setOperationAction(ISD::UMUL_LOHI, MVT::i64, Expand);
    setOperationAction(ISD::SMUL_LOHI, MVT::i64, Expand);
  } else {  // Operations that require the CARRY instruction
    setOperationAction(ISD::UDIVREM, MVT::i64, Legal);
    setOperationAction(ISD::SDIVREM, MVT::i64, Legal);
    setOperationAction(ISD::UMUL_LOHI, MVT::i64, Legal);
    setOperationAction(ISD::SMUL_LOHI, MVT::i64, Legal);
    setOperationAction(ISD::UADDO_CARRY, MVT::i64, Legal);
    setOperationAction(ISD::USUBO_CARRY, MVT::i64, Legal);
    setOperationAction(ISD::UADDO, MVT::i64, Legal);
    setOperationAction(ISD::USUBO, MVT::i64, Legal);
    setOperationAction(ISD::SHL, MVT::i128, Custom);
    setOperationAction(ISD::SRL, MVT::i128, Custom);
    setOperationAction(ISD::SRA, MVT::i128, Custom);
  }
  setOperationAction(ISD::BITREVERSE, MVT::i64, Legal);
  // Sign extend inreg
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i32, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Legal);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Legal);
  // Zero extend
  setOperationAction(ISD::ZERO_EXTEND, MVT::i32, Legal);
  setOperationAction(ISD::ZERO_EXTEND, MVT::i16, Legal);
  setOperationAction(ISD::ZERO_EXTEND, MVT::i8, Legal);
  // Subword operations
  setOperationAction(ISD::ROTL, MVT::i32, Custom);
  setOperationAction(ISD::ROTR, MVT::i32, Custom);

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
  setOperationAction(ISD::FABS, MVT::f64, Legal);
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
  setOperationAction(ISD::FTAN, MVT::f64, Legal);
  setOperationAction(ISD::FLOG, MVT::f64, Legal);
  setOperationAction(ISD::FLOG2, MVT::f64, Legal);
  setOperationAction(ISD::FLOG10, MVT::f64, Legal);
  setOperationAction(ISD::FEXP, MVT::f64, Legal);
  setOperationAction(ISD::FEXP2, MVT::f64, Legal);
  setOperationAction(ISD::FFLOOR, MVT::f64, Legal);
  setOperationAction(ISD::FCEIL, MVT::f64, Legal);
  setOperationAction(ISD::FTRUNC, MVT::f64, Legal);
  setOperationAction(ISD::FROUND, MVT::f64, Legal);
  setOperationAction(ISD::FROUNDEVEN, MVT::f64, Legal);
  setOperationAction(ISD::FNEARBYINT, MVT::f64, Legal);
  setOperationAction(ISD::FCOPYSIGN, MVT::f64, Legal);
  setOperationAction(ISD::FASIN, MVT::f64, Legal);
  setOperationAction(ISD::FACOS, MVT::f64, Legal);
  setOperationAction(ISD::FATAN, MVT::f64, Legal);
  setOperationAction(ISD::FATAN2, MVT::f64, Legal);
  setOperationAction(ISD::SELECT_CC, MVT::f64, Custom);
  setOperationAction(ISD::SETCC, MVT::f64, Custom);
  setOperationAction(ISD::BR_CC, MVT::f64, Custom);
  setOperationAction(ISD::SELECT, MVT::f64, Expand);
  if (!EnableCarry)
    setOperationAction(ISD::FREM, {MVT::f32, MVT::f64}, Expand);
  else
    setOperationAction(ISD::FREM, {MVT::f32, MVT::f64}, Custom);
  setOperationAction(ISD::FLDEXP, {MVT::f32, MVT::f64}, Legal);
  setOperationAction(ISD::STRICT_FLDEXP, {MVT::f32, MVT::f64}, Legal);
  setOperationAction(ISD::FFREXP, {MVT::f32, MVT::f64}, Legal);

  // 32-bit floating point
  setLoadExtAction(ISD::EXTLOAD, MVT::f64, MVT::f32, Expand);
  setTruncStoreAction(MVT::f64, MVT::f32, Expand);
  setOperationAction(ISD::FP_EXTEND, MVT::f64, Legal);
  setOperationAction(ISD::ConstantFP, MVT::f32, Legal);
  setOperationAction(ISD::FABS, MVT::f32, Legal);
  setOperationAction(ISD::FADD, MVT::f32, Legal);
  setOperationAction(ISD::FMUL, MVT::f32, Legal);
  setOperationAction(ISD::FDIV, MVT::f32, Legal);
  setOperationAction(ISD::FMA,  MVT::f32, Legal);
  setOperationAction(ISD::FMAD, MVT::f32, Legal);
  setOperationAction(ISD::FMINNUM, MVT::f32, Legal);
  setOperationAction(ISD::FMAXNUM, MVT::f32, Legal);
  setOperationAction(ISD::FMINIMUM, MVT::f32, Legal);
  setOperationAction(ISD::FMAXIMUM, MVT::f32, Legal);
  setOperationAction(ISD::FSIN, MVT::f32, Legal);
  setOperationAction(ISD::FCOS, MVT::f32, Legal);
  setOperationAction(ISD::FTAN, MVT::f32, Legal);
  setOperationAction(ISD::FLOG, MVT::f32, Legal);
  setOperationAction(ISD::FLOG2, MVT::f32, Legal);
  setOperationAction(ISD::FLOG10, MVT::f32, Legal);
  setOperationAction(ISD::FEXP, MVT::f32, Legal);
  setOperationAction(ISD::FEXP2, MVT::f32, Legal);
  setOperationAction(ISD::FFLOOR, MVT::f32, Legal);
  setOperationAction(ISD::FCEIL, MVT::f32, Legal);
  setOperationAction(ISD::FTRUNC, MVT::f32, Legal);
  setOperationAction(ISD::FROUND, MVT::f32, Legal);
  setOperationAction(ISD::FROUNDEVEN, MVT::f32, Legal);
  setOperationAction(ISD::FNEARBYINT, MVT::f32, Legal);
  setOperationAction(ISD::FCOPYSIGN, MVT::f32, Legal);
  setOperationAction(ISD::FASIN, MVT::f32, Legal);
  setOperationAction(ISD::FACOS, MVT::f32, Legal);
  setOperationAction(ISD::FATAN, MVT::f32, Legal);
  setOperationAction(ISD::FATAN2, MVT::f32, Legal);
  setOperationAction(ISD::SELECT_CC, MVT::f32, Custom);
  setOperationAction(ISD::SETCC, MVT::f32, Custom);
  setOperationAction(ISD::BR_CC, MVT::f32, Custom);
  setOperationAction(ISD::SELECT, MVT::f32, Expand);
  setOperationAction(ISD::BITCAST, MVT::f32, Custom);
  setOperationAction(ISD::BITCAST, MVT::i32, Custom);

  setMaxAtomicSizeInBitsSupported(64);
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

bool My66000TargetLowering::isTruncateFree(EVT SrcVT, EVT DstVT) const {
  // The W instructions make promoting back to i64 free in many cases.
  if (SrcVT.isVector() || DstVT.isVector() || !SrcVT.isInteger() ||
      !DstVT.isInteger())
    return false;
  unsigned SrcBits = SrcVT.getSizeInBits();
  unsigned DestBits = DstVT.getSizeInBits();
LLVM_DEBUG(dbgs() << "isTruncateFree2 src=" << SrcBits << " dst=" << DestBits << '\n');
  return (SrcBits == 64 && DestBits == 32);
}

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

// For debug statements
#ifndef NDEBUG
static const char *getCCName(ISD:: CondCode CC) {
    switch (CC) {
    default: return "???";
    case ISD::SETEQ:                    return "eq";
    case ISD::SETGT:                    return "gt";
    case ISD::SETGE:                    return "ge";
    case ISD::SETLT:                    return "lt";
    case ISD::SETLE:                    return "le";
    case ISD::SETNE:                    return "ne";

    case ISD::SETOEQ:                   return "oeq";
    case ISD::SETOGT:                   return "ogt";
    case ISD::SETOGE:                   return "oge";
    case ISD::SETOLT:                   return "olt";
    case ISD::SETOLE:                   return "ole";
    case ISD::SETONE:                   return "one";

    case ISD::SETO:                     return "o";
    case ISD::SETUO:                    return "uo";
    case ISD::SETUEQ:                   return "ueq";
    case ISD::SETUGT:                   return "ugt";
    case ISD::SETUGE:                   return "uge";
    case ISD::SETULT:                   return "ult";
    case ISD::SETULE:                   return "ule";
    case ISD::SETUNE:                   return "une";

    case ISD::SETTRUE:                  return "true";
    case ISD::SETTRUE2:                 return "true2";
    case ISD::SETFALSE:                 return "false";
    case ISD::SETFALSE2:                return "false2";
    }
}
#endif

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
  default: {
LLVM_DEBUG(dbgs() << "ISDCCtoMy66000CBF CC=" << getCCName(CC) << '\n');
      llvm_unreachable("Unknown floating condition code!");
  }
  case ISD::SETEQ:  return MYCB::EQ;
  case ISD::SETNE:  return MYCB::NE;
  case ISD::SETLT:  return MYCB::LT;	// is this valid for floats?
  case ISD::SETGT:  return MYCB::GT;	// ""
  case ISD::SETLE:  return MYCB::LE;	// ""
  case ISD::SETGE:  return MYCB::GE;	// ""
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

// isIntImmediate - This method tests to see if the node is a constant
// operand. If so Imm will receive the value.
static bool isIntImmediate(const SDNode *N, uint64_t &Imm) {
  if (const ConstantSDNode *C = dyn_cast<const ConstantSDNode>(N)) {
    Imm = C->getZExtValue();
    return true;
  }
  return false;
}

// Optimize a compare when the RHS is an immediate, particularly when
// the LHS is not full width load and the compare is EQ or NE.
// Return updated LHS, RHS and condition code
static ISD::CondCode optimizeIntCmp(SDValue &LHS, SDValue &RHS,
				    ISD::CondCode oldCC,
				    SelectionDAG &DAG, const SDLoc &dl) {
LLVM_DEBUG(dbgs() << "optimizeIntCmp\n");
  ISD::CondCode CC = oldCC;
  uint64_t imm;
  if (isIntImmediate(RHS.getNode(), imm)) {
LLVM_DEBUG(dbgs() << "\timm=" << (int64_t)imm << '\n');
    if (isa<LoadSDNode>(LHS) &&
        cast<LoadSDNode>(LHS)->getExtensionType() == ISD::ZEXTLOAD) {
      EVT VT = cast<LoadSDNode>(LHS)->getMemoryVT();
      if (VT == MVT::i32) {
	int32_t ValueofRHS = cast<ConstantSDNode>(RHS)->getZExtValue();
	if (ValueofRHS < 0) {
LLVM_DEBUG(dbgs() << "\tsign extend LHS load 32\n");
	  LHS = DAG.getNode(ISD::SIGN_EXTEND_INREG, dl, MVT::i64, LHS,
			    DAG.getValueType(MVT::i32));
	  RHS = DAG.getConstant(ValueofRHS, dl, RHS.getValueType());
	}
      } else if (VT == MVT::i16) {
	int16_t ValueofRHS = cast<ConstantSDNode>(RHS)->getZExtValue();
	if (ValueofRHS < 0) {
LLVM_DEBUG(dbgs() << "\tsign extend LHS load 16\n");
	  LHS = DAG.getNode(ISD::SIGN_EXTEND_INREG, dl, MVT::i64, LHS,
			    DAG.getValueType(MVT::i16));
	  RHS = DAG.getConstant(ValueofRHS, dl, RHS.getValueType());
	}
      } else if (VT == MVT::i8) {
	int8_t ValueofRHS = cast<ConstantSDNode>(RHS)->getZExtValue();
	if (ValueofRHS < 0) {
LLVM_DEBUG(dbgs() << "\tsign extend LHS load 8\n");
	  LHS = DAG.getNode(ISD::SIGN_EXTEND_INREG, dl, MVT::i64, LHS,
			    DAG.getValueType(MVT::i8));
	  RHS = DAG.getConstant(ValueofRHS, dl, RHS.getValueType());
	}
      }
    }
/*
    // Fix unhelpful optimization that converted negated operand
    if (LHS.getOpcode() == ISD::ADD && isAllOnesConstant(LHS.getOperand(1))) {
LLVM_DEBUG(dbgs() << "\tconvert LHS is ADD of -1\n");
	imm = -imm - 1;	// might be used in subsequent fixuup
	LHS = DAG.getNode(ISD::SUB, dl, MVT::i64,
			  DAG.getConstant(0, dl, MVT::i64),
			  LHS.getOperand(0));
	RHS = DAG.getConstant(imm, dl, MVT::i64);
	CC = ISD::getSetCCSwappedOperands(CC);
    }
*/
    // Fix unhelpful optimization that preferred LT and GT and shrink constant
    if (CC == ISD::SETLT && (int64_t)imm > 0) {
LLVM_DEBUG(dbgs() << "\tconvert LT x into LE x-1 (x > 0)\n");
      RHS = DAG.getConstant(imm-1, dl, MVT::i64);
      CC = ISD::SETLE;
    } else if (CC == ISD::SETGT && (int64_t)imm < 0) {
LLVM_DEBUG(dbgs() << "\tconvert GT x into GE x+1 (x < 0)\n");
      RHS = DAG.getConstant(imm+1, dl, MVT::i64);
      CC = ISD::SETGE;
    }
  }
  return CC;
}

// Compare and make result into a boolean
SDValue My66000TargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
  SDLoc dl(Op);
  unsigned inst;
  MYCB::CondBits CB;
LLVM_DEBUG(dbgs() << "LowerSETCC\n");
  if (LHS.getValueType().isInteger()) {
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
    CC = optimizeIntCmp(LHS, RHS, CC, DAG, dl);
    CB = ISDCCtoMy66000CBI(CC);
    inst = My66000ISD::CMP;
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
LLVM_DEBUG(dbgs() << "LowerSELECT_CC\n");
  if (LHS.getValueType().isInteger()) {
    if (isNullConstant(RHS) && (CC == ISD::SETEQ || CC == ISD::SETNE)) {
      if (CC == ISD::SETEQ)
        std::swap(TVal, FVal);
      return DAG.getNode(My66000ISD::CMOV, dl, TVal.getValueType(), TVal, FVal, LHS);
    }
    inst = My66000ISD::CMP;
    CC = optimizeIntCmp(LHS, RHS, CC, DAG, dl);
    /* If items selected are constants that differ by 1 */
    if (CC == ISD::SETLT && isNullConstant(RHS) &&
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
  SDValue Ext = DAG.getNode(My66000ISD::EXT, dl, MVT::i64, Cmp,
		     DAG.getConstant(1, dl, MVT::i64),
		     DAG.getConstant(CB, dl, MVT::i64));
  return DAG.getNode(My66000ISD::CMOV, dl, TVal.getValueType(), TVal, FVal, Ext);
}

SDValue My66000TargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
LLVM_DEBUG(dbgs() << "LowerBR_CC CC=" << getCCName(CC) << '\n');
  SDLoc dl(Op);
  EVT VT = LHS.getValueType();
  if (VT.isInteger()) {
    CC = optimizeIntCmp(LHS, RHS, CC, DAG, dl);
    if (isNullConstant(RHS)) {
      MYCC::CondCodes cc = ISDCCtoMy66000CC(CC, VT);
      if ((CC == ISD::SETNE || CC == ISD::SETEQ) &&
	  LHS.getOpcode() == ISD::AND &&
          isa<ConstantSDNode>(LHS.getOperand(1)) &&
          isPowerOf2_64(LHS.getConstantOperandVal(1))) { // testing a single bit
	uint64_t Mask = LHS.getConstantOperandVal(1);
	SDValue Test = LHS.getOperand(0);
	unsigned Bit = Log2_64(Mask);
	unsigned Opcode = (CC == ISD::SETNE) ? My66000ISD::BRbit1:
					       My66000ISD::BRbit0;
	return DAG.getNode(Opcode, dl, MVT::Other, Chain, Dest,
			   Test, DAG.getConstant(Bit, dl, MVT::i64));
      }
      return DAG.getNode(My66000ISD::BRcond, dl, MVT::Other, Chain, Dest,
		         LHS, DAG.getConstant(cc, dl, MVT::i64));
    }
    MYCB::CondBits CB = ISDCCtoMy66000CBI(CC);
    SDValue Cmp = DAG.getNode(My66000ISD::CMP, dl, MVT::i64, LHS, RHS);
    return DAG.getNode(My66000ISD::BRcc, dl, MVT::Other, Chain, Dest, Cmp,
                       DAG.getConstant(CB, dl, MVT::i64));
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
LLVM_DEBUG(dbgs() << "LowerSIGN_EXTEND_INREG\n");
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
LLVM_DEBUG(dbgs() << "LowerCall" << " TailCall=" << CLI.IsTailCall
		  << " VarArg=" << CLI.IsVarArg << '\n');

  MachineFunction &MF = DAG.getMachineFunction();

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());

  CCInfo.AnalyzeCallOperands(Outs, CC_My66000);
  if (IsTailCall && !canUseTailCall(ArgLocs))
    IsTailCall = false;

  SmallVector<CCValAssign, 16> RVLocs;
  // Analyze return values to determine the number of bytes of stack required.
  CCState RetCCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                    *DAG.getContext());
  RetCCInfo.AllocateStack(CCInfo.getStackSize(), Align(8));
  RetCCInfo.AnalyzeCallResult(Ins, RetCC_My66000);

  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NumBytes = RetCCInfo.getStackSize();
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
LLVM_DEBUG(dbgs() << "lowerCallResult"
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
  MachineFunction &MF = DAG.getMachineFunction();
LLVM_DEBUG(dbgs() << "LowerFormalArguments: " << MF.getName() << '\n');

  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineRegisterInfo &RegInfo = MF.getRegInfo();
  My66000MachineFunctionInfo *FI = MF.getInfo<My66000MachineFunctionInfo>();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_My66000);

  if (!IsVarArg)
    FI->setReturnStackOffset(CCInfo.getStackSize());

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
        if (Ins[i].Flags.isSExt())
	    FI->addSExtRegister(VReg);
        if (Ins[i].Flags.isZExt())
	    FI->addZExtRegister(VReg);
      }
    } else {      		// Arguments passed in memory
      assert(VA.isMemLoc());      // sanity check
      // Load the argument to a virtual register
      unsigned ObjSize = VA.getLocVT().getStoreSize();
      unsigned StackSlotSize = 8;
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
//    ArrayRef<MCPhysReg> ArgRegs = ArrayRef(ArgGPRs);
    auto *XFI = MF.getInfo<My66000MachineFunctionInfo>();
    unsigned FirstVAReg = CCInfo.getFirstUnallocated(ArgGPRs);
    LLVM_DEBUG(dbgs() << "\tFirstVAReg=" << FirstVAReg << '\n');
    LLVM_DEBUG(dbgs() << "\tNVarregs=" << std::size(ArgGPRs) << '\n');
    int Offset = -(8 * 8);
    int VaFI = MFI.CreateFixedObject(8, Offset, true);
    // All registers R1-R8 pushed by ENTER
    for (unsigned i = 0; i < 8; i++) {
      VaFI = MFI.CreateFixedObject(8, Offset, true);
      if (i == FirstVAReg)
	XFI->setVarArgsFrameIndex(VaFI);
      Offset += 8;
    }
    int VaSaveSize = 8 * 8;
    LLVM_DEBUG(dbgs() << "\tVaSaveSize=" << VaSaveSize << '\n');
    // But the AP has to point at the first saved VA reg
    XFI->setVarArgsSaveSize(VaSaveSize);
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
          Chain, dl, FIN, ArgDI.SDV, DAG.getConstant(Size, dl, MVT::i64),
	  Alignment, false, false, /*CI=*/nullptr, std::nullopt,
	  MachinePointerInfo(), MachinePointerInfo()));
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

/*
 * The varargs stuff assumes the offset to the saved varargs
 * is at the top (offset 0). But we save all the arg registers,
 * so we have to figure out how far down the varargs starts.
 */
SDValue My66000TargetLowering::LowerVASTART(SDValue Op,
					    SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "LowerVASTART\n");
  MachineFunction &MF = DAG.getMachineFunction();
  auto *XFI = MF.getInfo<My66000MachineFunctionInfo>();
  SDLoc DL(Op);
  SDValue FI = DAG.getFrameIndex(XFI->getVarArgsFrameIndex(),
                                 getPointerTy(MF.getDataLayout()));
LLVM_DEBUG(Op.getOperand(0).getNode()->dump(););
LLVM_DEBUG(Op.getOperand(1).getNode()->dump(););
LLVM_DEBUG(Op.getOperand(2).getNode()->dump(););
LLVM_DEBUG(FI.getNode()->dump(););
  // vastart just stores the address of the VarArgsFrameIndex slot into the
  // memory location argument.
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0) /*chain*/, DL,
		      FI/*Val*/, Op.getOperand(1)/*Ptr*/,
                      MachinePointerInfo(SV)/*PtrInfo*/);
}


//===----------------------------------------------------------------------===//
//               Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

bool My66000TargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
  if (!CCInfo.CheckReturn(Outs, RetCC_My66000))
    return false;
  if (CCInfo.getStackSize() != 0 && IsVarArg)
    return false;
  return true;
}

SDValue
My66000TargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                               bool IsVarArg,
                               const SmallVectorImpl<ISD::OutputArg> &Outs,
                               const SmallVectorImpl<SDValue> &OutVals,
                               const SDLoc &dl, SelectionDAG &DAG) const {
  auto *AFI = DAG.getMachineFunction().getInfo<My66000MachineFunctionInfo>();
  MachineFrameInfo &MFI = DAG.getMachineFunction().getFrameInfo();
LLVM_DEBUG(dbgs() << "LowerReturn\n");

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
LLVM_DEBUG(dbgs() << "LowerGlobalAddress\n");

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
LLVM_DEBUG(dbgs() << "LowerBlockAddress\n");
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
LLVM_DEBUG(dbgs() << "LowerBR_JT\n");
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

SDValue My66000TargetLowering::lowerBITCAST(SDValue Op,
					    SelectionDAG &DAG) const {
  SDLoc DL(Op);
  EVT VT = Op.getValueType();
  SDValue Op0 = Op.getOperand(0);
  EVT Op0VT = Op0.getValueType();
  if (VT == MVT::f32 && Op0VT == MVT::i32) {
    SDValue Ext = DAG.getNode(ISD::ANY_EXTEND, DL, MVT::i64, Op0);
    return DAG.getNode(My66000ISD::COPYTOFS, DL, MVT::f32, Ext);
  }
  return Op;
}

SDValue My66000TargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "LowerOperation: ");
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
  case ISD::FREM:			return lowerFREM(Op, DAG);
  case ISD::BITCAST:			return lowerBITCAST(Op, DAG);
  default:
    llvm_unreachable("unimplemented operand");
  }
}

// Converts the given i8/i16/i32 operation to a target-specific SelectionDAG
// node. Because i8/i16/i32 isn't a legal type for us, these operations would
// otherwise be promoted to i64, making it difficult to select.
static SDValue customLegalizeToWOp(SDNode *N, SelectionDAG &DAG,
				   const My66000ISD::NodeType OpCode,
                                   unsigned ExtOpc = ISD::ANY_EXTEND) {
  SDLoc DL(N);
  SDValue NewOp0 = DAG.getNode(ExtOpc, DL, MVT::i64, N->getOperand(0));
  SDValue NewOp1 = DAG.getNode(ExtOpc, DL, MVT::i64, N->getOperand(1));
  SDValue NewRes = DAG.getNode(OpCode, DL, MVT::i64, NewOp0, NewOp1);
  // ReplaceNodeResults requires we maintain the same type for the return value.
  return DAG.getNode(ISD::TRUNCATE, DL, N->getValueType(0), NewRes);
}


void My66000TargetLowering::ReplaceNodeResults(SDNode *N,
					       SmallVectorImpl<SDValue> &Results,
		                               SelectionDAG &DAG) const {
LLVM_DEBUG(dbgs() << "ReplaceNodeResults\n");
  SDLoc DL(N);
  EVT VT = N->getValueType(0);
  switch (N->getOpcode()) {
  case ISD::BITCAST: {
    SDValue Op0 = N->getOperand(0);
    EVT Op0VT = Op0.getValueType();
    if (VT == MVT::i32 && Op0VT == MVT::f32) {
      SDValue Copy = DAG.getNode(My66000ISD::COPYFMFS, DL, MVT::i64, Op0);
      Results.push_back(DAG.getNode(ISD::TRUNCATE, DL, MVT::i32, Copy));
    }
  }
  break;
  case ISD::ROTL:
    Results.push_back(customLegalizeToWOp(N, DAG, My66000ISD::ROLW));
  break;
  case ISD::ROTR:
    Results.push_back(customLegalizeToWOp(N, DAG, My66000ISD::RORW));
  break;
  } // end switch
}


static MachineBasicBlock *emitDIVREM(MachineInstr &MI,
				     MachineBasicBlock *BB,
				     unsigned inst1, unsigned inst2) {
  MachineFunction &MF = *BB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  unsigned DIV = MI.getOperand(0).getReg();
  unsigned REM = MI.getOperand(1).getReg();
  MachineRegisterInfo &MRI = BB->getParent()->getRegInfo();
  bool REMunused = MRI.use_empty(REM);
LLVM_DEBUG(dbgs() << "emitUDIVREM\n" << MI << '\n');
  if (REMunused) {
    BuildMI(*BB, MI, DL, TII.get(inst1), DIV)
	    .add(MI.getOperand(2)).add(MI.getOperand(3));
  } else {
    BuildMI(*BB, MI, DL, TII.get(inst2), DIV)
	    .addDef(REM)
	    .add(MI.getOperand(2)).add(MI.getOperand(3));
  }
  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return BB;
}

// Copies between FSRegs and GRegs should be NOPs
static MachineBasicBlock *emitCPFS(MachineInstr &MI, MachineBasicBlock *BB) {
  MachineFunction &MF = *BB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();
  unsigned TO = MI.getOperand(0).getReg();
  unsigned FM = MI.getOperand(1).getReg();
  BuildMI(*BB, MI, DL, TII.get(TargetOpcode::COPY), TO)
	.addReg(FM);
  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return BB;
}

static MachineBasicBlock *emitAtomicOp(MachineInstr &MI, MachineBasicBlock *BB,
			unsigned Size, unsigned OpCode) {
LLVM_DEBUG(dbgs() << "emitAtomicOp\n" << MI << '\n');
  MachineFunction &MF = *BB->getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();

  auto LdOp = My66000::LDDXrr;
  auto StOp = My66000::STDXrr;
  switch (Size) {
  default:
    llvm_unreachable("Unexpected size of atomic entity");
  case 1:
    LdOp = My66000::LDUBXrr;
    StOp = My66000::STBXrr;
    break;
  case 2:
    LdOp = My66000::LDSHXrr;
    StOp = My66000::STHXrr;
    break;
  case 4:
    LdOp = My66000::LDSWXrr;
    StOp = My66000::STWXrr;
    break;
  case 8:
    LdOp = My66000::LDDXrr;
    StOp = My66000::STDXrr;
    break;
  }
  Register dest = MI.getOperand(0).getReg();
  MachineOperand &base = MI.getOperand(1);
  Register indx = MI.getOperand(2).getReg();
  unsigned shft = MI.getOperand(3).getImm();
  DebugLoc dl = MI.getDebugLoc();

  // Build the load
  BuildMI(*BB, MI, dl, TII.get(LdOp), dest)
	.add(base).addReg(indx).addImm(shft)
	.add(MI.getOperand(4));
  // Build the operation
  MachineRegisterInfo &MRI = MF.getRegInfo();
  Register temp;
  if (OpCode == 0) {				// its swap
    temp = MI.getOperand(5).getReg();
  } else if (OpCode == My66000::CMPrr) {	// its compare and swap
    temp = MRI.createVirtualRegister(&My66000::GRegsRegClass);
    BuildMI(*BB, MI, dl, TII.get(OpCode), temp)
	    .addReg(dest) .addReg(MI.getOperand(5).getReg());
    BuildMI(*BB, MI, dl, TII.get(My66000::PRIB))
	    .addImm(MYCB::NE) .addReg(temp) .addImm(1) .addImm(0);
    temp = MI.getOperand(6).getReg();
  } else {					// its an operation, e.g. ADD
    temp = MRI.createVirtualRegister(&My66000::GRegsRegClass);
    BuildMI(*BB, MI, dl, TII.get(OpCode), temp)
	    .addReg(dest) .add(MI.getOperand(5));
  }
  // Build the store
  BuildMI(*BB, MI, dl, TII.get(StOp))
	.addReg(temp).add(base).addReg(indx).addImm(shft)
	.add(MI.getOperand(4));
  // Do we need this
  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return BB;
}

// Since My66000 does not have a SUB immediate, we negate the increment
// and use ADD immediate.
static MachineBasicBlock *emitAtomicSub(MachineInstr &MI, MachineBasicBlock *BB,
			unsigned Size, unsigned OpCode) {
  int64_t Imm = MI.getOperand(5).getImm();
  MI.getOperand(5).setImm(-Imm);
  return emitAtomicOp(MI, BB, Size, OpCode);
}


MachineBasicBlock *My66000TargetLowering::EmitInstrWithCustomInserter(
			MachineInstr &MI,
			MachineBasicBlock *BB) const {
LLVM_DEBUG(dbgs() << "EmitInstrWithCustomInserter\n");
  switch (MI.getOpcode()) {
  default:
    llvm_unreachable("Unexpected instr type to insert");
  case My66000::UDIVREMrr:
	return emitDIVREM(MI, BB, My66000::UDIVrr, My66000::UDIVREMrrc);
  case My66000::UDIVREMri:
	return emitDIVREM(MI, BB, My66000::UDIVri, My66000::UDIVREMric);
  case My66000::UDIVREMrw:
	return emitDIVREM(MI, BB, My66000::UDIVrw, My66000::UDIVREMrwc);
  case My66000::UDIVREMwr:
	return emitDIVREM(MI, BB, My66000::UDIVwr, My66000::UDIVREMwrc);
  case My66000::UDIVREMrd:
	return emitDIVREM(MI, BB, My66000::UDIVrd, My66000::UDIVREMrdc);
  case My66000::UDIVREMdr:
	return emitDIVREM(MI, BB, My66000::UDIVdr, My66000::UDIVREMdrc);
  case My66000::SDIVREMrr:
	return emitDIVREM(MI, BB, My66000::SDIVrr, My66000::SDIVREMrrc);
  case My66000::SDIVREMrn:
	return emitDIVREM(MI, BB, My66000::SDIVrn, My66000::SDIVREMrnc);
  case My66000::SDIVREMnr:
	return emitDIVREM(MI, BB, My66000::SDIVnr, My66000::SDIVREMnrc);
  case My66000::SDIVREMnn:
	return emitDIVREM(MI, BB, My66000::SDIVnn, My66000::SDIVREMnnc);
  case My66000::SDIVREMrx:
	return emitDIVREM(MI, BB, My66000::SDIVrw, My66000::SDIVREMrwc);
  case My66000::SDIVREMwr:
	return emitDIVREM(MI, BB, My66000::SDIVwr, My66000::SDIVREMwrc);
  case My66000::SDIVREMrd:
	return emitDIVREM(MI, BB, My66000::SDIVrd, My66000::SDIVREMrdc);
  case My66000::SDIVREMdr:
	return emitDIVREM(MI, BB, My66000::SDIVdr, My66000::SDIVREMdrc);
  case My66000::CPFMFS:		return emitCPFS(MI, BB);
  case My66000::CPTOFS:		return emitCPFS(MI, BB);
  case My66000::AADDDr:	return emitAtomicOp(MI, BB, 8, My66000::ADDrr);
  case My66000::AADDWr:	return emitAtomicOp(MI, BB, 4, My66000::ADDrr);
  case My66000::AADDHr:	return emitAtomicOp(MI, BB, 2, My66000::ADDrr);
  case My66000::AADDBr:	return emitAtomicOp(MI, BB, 1, My66000::ADDrr);
  case My66000::AADDDi:	return emitAtomicOp(MI, BB, 8, My66000::ADDri);
  case My66000::AADDWi:	return emitAtomicOp(MI, BB, 4, My66000::ADDri);
  case My66000::AADDHi:	return emitAtomicOp(MI, BB, 2, My66000::ADDri);
  case My66000::AADDBi:	return emitAtomicOp(MI, BB, 1, My66000::ADDri);
  case My66000::ASUBDr:	return emitAtomicOp(MI, BB, 8, My66000::ADDrn);
  case My66000::ASUBWr:	return emitAtomicOp(MI, BB, 4, My66000::ADDrn);
  case My66000::ASUBHr:	return emitAtomicOp(MI, BB, 2, My66000::ADDrn);
  case My66000::ASUBBr:	return emitAtomicOp(MI, BB, 1, My66000::ADDrn);
  case My66000::ASUBDi:	return emitAtomicSub(MI, BB, 8, My66000::ADDri);
  case My66000::ASUBWi:	return emitAtomicSub(MI, BB, 4, My66000::ADDri);
  case My66000::ASUBHi:	return emitAtomicSub(MI, BB, 2, My66000::ADDri);
  case My66000::ASUBBi:	return emitAtomicSub(MI, BB, 1, My66000::ADDri);
  case My66000::AANDDr:	return emitAtomicOp(MI, BB, 8, My66000::ANDrr);
  case My66000::AANDWr:	return emitAtomicOp(MI, BB, 4, My66000::ANDrr);
  case My66000::AANDHr:	return emitAtomicOp(MI, BB, 2, My66000::ANDrr);
  case My66000::AANDBr:	return emitAtomicOp(MI, BB, 1, My66000::ANDrr);
  case My66000::AANDDi:	return emitAtomicOp(MI, BB, 8, My66000::ANDri);
  case My66000::AANDWi:	return emitAtomicOp(MI, BB, 4, My66000::ANDri);
  case My66000::AANDHi:	return emitAtomicOp(MI, BB, 2, My66000::ANDri);
  case My66000::AANDBi:	return emitAtomicOp(MI, BB, 1, My66000::ANDri);
  case My66000::AORDr:	return emitAtomicOp(MI, BB, 8, My66000::ORrr);
  case My66000::AORWr:	return emitAtomicOp(MI, BB, 4, My66000::ORrr);
  case My66000::AORHr:	return emitAtomicOp(MI, BB, 2, My66000::ORrr);
  case My66000::AORBr:	return emitAtomicOp(MI, BB, 1, My66000::ORrr);
  case My66000::AORDi:	return emitAtomicOp(MI, BB, 8, My66000::ORri);
  case My66000::AORWi:	return emitAtomicOp(MI, BB, 4, My66000::ORri);
  case My66000::AORHi:	return emitAtomicOp(MI, BB, 2, My66000::ORri);
  case My66000::AORBi:	return emitAtomicOp(MI, BB, 1, My66000::ORri);
  case My66000::AXORDr:	return emitAtomicOp(MI, BB, 8, My66000::XORrr);
  case My66000::AXORWr:	return emitAtomicOp(MI, BB, 4, My66000::XORrr);
  case My66000::AXORHr:	return emitAtomicOp(MI, BB, 2, My66000::XORrr);
  case My66000::AXORBr:	return emitAtomicOp(MI, BB, 1, My66000::XORrr);
  case My66000::AXORDi:	return emitAtomicOp(MI, BB, 8, My66000::XORri);
  case My66000::AXORWi:	return emitAtomicOp(MI, BB, 4, My66000::XORri);
  case My66000::AXORHi:	return emitAtomicOp(MI, BB, 2, My66000::XORri);
  case My66000::AXORBi:	return emitAtomicOp(MI, BB, 1, My66000::XORri);
  case My66000::ASWAPDr:return emitAtomicOp(MI, BB, 8, 0);
  case My66000::ASWAPWr:return emitAtomicOp(MI, BB, 4, 0);
  case My66000::ASWAPHr:return emitAtomicOp(MI, BB, 2, 0);
  case My66000::ASWAPBr:return emitAtomicOp(MI, BB, 1, 0);
  case My66000::ACMPSWAPDr: return emitAtomicOp(MI, BB, 8, My66000::CMPrr);
  case My66000::ACMPSWAPWr: return emitAtomicOp(MI, BB, 4, My66000::CMPrr);
  case My66000::ACMPSWAPHr: return emitAtomicOp(MI, BB, 2, My66000::CMPrr);
  case My66000::ACMPSWAPBr: return emitAtomicOp(MI, BB, 1, My66000::CMPrr);
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
    SDValue Op, StringRef Constraint, std::vector<SDValue> &Ops,
    SelectionDAG &DAG) const {
  // Currently only support length 1 constraints.
  if (Constraint.size() == 1) {
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
