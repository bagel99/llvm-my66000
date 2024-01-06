//===-- My66000.h - Top-level interface for My66000 representation --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// My66000 back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MY66000_H
#define LLVM_LIB_TARGET_MY66000_MY66000_H

#include "MCTargetDesc/My66000MCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class FunctionPass;
  class ModulePass;
  class TargetMachine;
  class My66000TargetMachine;
  class PassRegistry;
  class formatted_raw_ostream;

  void initializeMy66000LowerThreadLocalPass(PassRegistry &);

  FunctionPass *createMy66000FrameToArgsOffsetEliminationPass();
  FunctionPass *createMy66000ISelDag(My66000TargetMachine &TM,
                                   CodeGenOpt::Level OptLevel);
  ModulePass *createMy66000LowerThreadLocalPass();
  FunctionPass *createMy66000PredBlockPass();
  void initializeMy66000PredBlockPass(PassRegistry &);
  FunctionPass *createMy66000FixJumpTablePass();
  void initializeMy66000FixJumpTablePass(PassRegistry &);
  FunctionPass *createMy66000VVMLoopPass();
  void initializeMy66000VVMLoopPass(PassRegistry &);
  FunctionPass *createMy66000VVMFixupPass();
  void initializeMy66000VVMFixupPass(PassRegistry &);

  extern char &My66000VVMLoopID;
  extern char &My66000PredBlockID;

  // Condition Codes used with BRcond
  namespace MYCC {
    enum CondCodes {
	EQ0=0, NE0, GE0, LT0, GT0, LE0,			// integer
	A=6, N,						// always, never
	DEQ=8, DNE, DGE, DLT, DGT, DLE, DOR, DUN,	// float double
	FEQ=16,FNE, FGE, FLT, FGT, FLE, FOR, FUN,	// float single
	IN=24,
	SVR=29, SVC, RET
    };
  }
  // Condition Bits resulting from CMP and FCMP
  // There are some duplicate values between integer and float
  namespace MYCB {
    enum CondBits {
	EQ=0, NEQ, NE, NNE,
	GE=4, NGE, LT, NLT,	// signed, float
	GT=8, NGT, LE, NLE,	// signed, float
	HS=12, LO, HI, LS,	// unsigned, float abs
	OR=16, NOR, TO, NTO,	// float ordered
	SIN=24, FIN, CIN, RIN,	// range
	SNaN=32, QNaN,		// float
	MINF=34, MNOR,		// float
	MDE=36, MZE, PZE, PDE,	// float
	PNOR=40, NINF		// float
    };
  }

} // end namespace llvm;

#endif
