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
  class formatted_raw_ostream;

  void initializeMy66000LowerThreadLocalPass(PassRegistry &p);

  FunctionPass *createMy66000FrameToArgsOffsetEliminationPass();
  FunctionPass *createMy66000ISelDag(My66000TargetMachine &TM,
                                   CodeGenOpt::Level OptLevel);
  ModulePass *createMy66000LowerThreadLocalPass();
  FunctionPass *createMy66000PredBlockPass();
  void initializeMy66000PredBlockPass(PassRegistry &p);
  FunctionPass *createMy66000FixJumpTablePass();
  void initializeMy66000FixJumpTablePass(PassRegistry &p);
  FunctionPass *createMy66000VVMLoopPass();
  void initializeMy66000VVMLoopPass(PassRegistry &p);
  FunctionPass *createMy66000VVMFixupPass();
  void initializeMy66000VVMFixupPass(PassRegistry &p);

  extern char &My66000VVMLoopID;
  extern char &My66000PredBlockID;

  // Condition Codes used with BRcond
  namespace MYCC {
    enum CondCodes {
	NM=0,   NN,  EQ0, NE0, GE0, GT0, LE0, LT0,	// integer
	FOR=8,  FUN, FEQ, FNE, FGE, FLT, FGT, FLE,	// float double
	FORF=16,FUNF,FEQF,FNEF,FGEF,FLTF,FGTF,FLEF,	// float single
	IN=24, EXIT, RFE=30, AL=31
    };
  }
  // Condition Bits resulting from CMP
  // There are some duplicate values between integer and float
  namespace MYCB {
    enum CondBits {
	NE=0, EQ,
	GT=2, GE, LT, LE,	// signed
	OR=6, UN,		// float ordered, unordered
	HI=10, HS, LO, LS,	// unsigned
	NNE=8, NEQ,		// float
	NGE=10, NGT, NLT, NLE,	// float
	SIN=16, FIN, CIN, RIN,	// range
	NM=34, N, Z, P, SM, UM,	// integer number line
//	SNAN=32, QNAN,
	FPI=34, FPN, FPD, FPZ,	// float
	FNZ=38, FND, FNN, FNI	// float
    };
  }

} // end namespace llvm;

#endif
