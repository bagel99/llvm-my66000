//===- My66000InstrInfoFP.td - FP Instr Description for My66000 ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//
// Floating point stuff
//
// Nodes that are not pre-defined
def fexp :  SDNode<"ISD::FEXP", SDTFPUnaryOp>;
def flog :  SDNode<"ISD::FLOG", SDTFPUnaryOp>;
def flog10: SDNode<"ISD::FLOG10", SDTFPUnaryOp>;
def ffrexp :  SDNode<"ISD::FFREXP", SDTFPUnaryOp>;
def AssertSext : SDNode<"ISD::AssertSext", SDTUnaryOp>;

def fpimm0  : FPImmLeaf<fAny, [{ return Imm.isExactlyValue(+0.0); }]>;
def fpimm1  : PatLeaf<(fpimm), [{ return N->isExactlyValue(+1.0); }]>;
def fpimm64 : Operand<f64>, FPImmLeaf<f64, [{ return true; }]>
		{ let PrintMethod = "printFP64Operand"; }
def fpimms32 : Operand<f64>, FPImmLeaf<f64, [{ return isFPshort(Imm); }]>
		{ let PrintMethod = "printFP32Operand"; }
def fpimmpi5 : Operand<f64>, FPImmLeaf<f64, [{ return isFPpimm5(Imm); }]>
		{ let PrintMethod = "printFPkOperand"; }
def fpimmi5 : Operand<f64>, FPImmLeaf<f64, [{ return isFPimm5(Imm); }]>
		{ let PrintMethod = "printFPkOperand"; }

//
// My66000 specific FP DAG Nodes.
//
def SDTFPBinHiLoOp : SDTypeProfile<2, 2,
    [ SDTCisSameAs<0, 1>, SDTCisSameAs<0, 2>, SDTCisSameAs<0, 3>,SDTCisFP<0>]>;

def myFCMP : SDNode<"My66000ISD::FCMP",
			SDTypeProfile<1, 2, [SDTCisVT<0, i64>,
			    SDTCisSameAs<1, 2>]>,
			[]>;
def myBRfcc :	    SDNode<"My66000ISD::BRfcc", SDT_BRcc,
			[SDNPHasChain]>;
def myFDIVREM :	    SDNode<"My66000ISD::FDIVREM", SDTFPBinHiLoOp>;
def myCOPYFMFS:     SDNode<"My66000ISD::COPYFMFS",
			SDTypeProfile<1, 1,
			  [SDTCisVT<0, i64>, SDTCisVT<1, f32>]>,
			[]>;
def myCOPYTOFS:     SDNode<"My66000ISD::COPYTOFS",
			SDTypeProfile<1, 1,
			  [SDTCisVT<0, f32>, SDTCisVT<1, i64>]>,
			[]>;

/*
   64-bit floating point shares the GRegs with integers
*/

multiclass FPrr<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def rr : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[(set (f64 GRegs:$rd), (OpNode (f64 GRegs:$rs1),
				       (f64 GRegs:$rs2)))]>;
  def rn : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,-$rs2"),
	[(set (f64 GRegs:$rd), (OpNode (f64 GRegs:$rs1),
				       (fneg (f64 GRegs: $rs2))))]>;
  def nr : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,$rs2"),
	[(set (f64 GRegs:$rd), (OpNode (fneg (f64 GRegs: $rs1)),
				       (f64 GRegs:$rs2)))]>;
  def nn : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,-$rs2"),
	[(set (f64 GRegs:$rd), (OpNode (fneg (f64 GRegs: $rs1)),
				       (fneg (f64 GRegs: $rs2))))]>;
  def rk : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimmi5:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimmi5:$imm))]>;
  def rf : ALU_RW<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimms32:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimms32:$imm))]>;
  def rd : ALU_RD<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimm64:$imm))]>;
  def jr : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins fpimmpi5:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd,  (OpNode fpimmpi5:$imm, (f64 GRegs:$rs2)))]>;
  def fr : ALU_RW<OpC, Flags,
	(outs GRegs:$rd), (ins fpimms32:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd,  (OpNode fpimms32:$imm, (f64 GRegs:$rs2)))]>;
  def dr : ALU_RW<OpC, Flags,
	(outs GRegs:$rd), (ins fpimm64:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd,  (OpNode fpimm64:$imm, (f64 GRegs:$rs2)))]>;
}

defm FADD :  FPrr<"fadd",  0b110001, 0b00000, fadd>;
defm FMUL :  FPrr<"fmul",  0b110010, 0b00000, fmul>;
defm FDIV :  FPrr<"fdiv",  0b110011, 0b00000, fdiv>;
// Caution: the following semantics change with changes to IEEE-754
defm FMAXN:  FPrr<"fmax",  0b110110, 0b00000, fmaxnum>;
defm FMINN:  FPrr<"fmin",  0b110111, 0b00000, fminnum>;
defm FMAX :  FPrr<"fmax",  0b110110, 0b00000, fmaximum>;
defm FMIN :  FPrr<"fmin",  0b110111, 0b00000, fminimum>;
defm CSGN :  FPrr<"csgn",  0b111001, 0b00000, fcopysign>;
defm FPOW :  FPrr<"fpow",  0b111101, 0b00000, fpow>;

// Can't use the standard pattern because the output type doesn't match inputs.
let isCompare = 1 in {
def FCMPrr : ALU_RR<0b110100, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"fcmp\t$rd,$rs1,$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), (f64 GRegs:$rs2)))]>;
def FCMPrn : ALU_RR<0b110100, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"fcmp\t$rd,$rs1,-$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1),
				       (fneg (f64 GRegs: $rs2))))]>;
def FCMPnr : ALU_RR<0b110100, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"fcmp\t$rd,-$rs1,$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (fneg (f64 GRegs: $rs1)),
				       (f64 GRegs:$rs2)))]>;
def FCMPnn : ALU_RR<0b110100, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"fcmp\t$rd,-$rs1,-$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (fneg (f64 GRegs: $rs1)),
				       (f64 GRegs:$rs2)))]>;
def FCMPrk : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimmi5:$imm),
	"fcmp\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), fpimmi5:$imm))]>;
def FCMPrf : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimms32:$imm),
	"fcmp\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), fpimms32:$imm))]>;
def FCMPrd : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm),
	"fcmp\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), fpimm64:$imm))]>;
}

def FSUB  : Pat<(fsub (f64 GRegs:$rs1), (f64 GRegs:$rs2)),
		(FADDrn GRegs:$rs1, GRegs:$rs2)>;
def NFADD : Pat<(fsub (fneg (f64 GRegs:$rs1)), (f64 GRegs:$rs2)),
		(FADDnn GRegs:$rs1, GRegs:$rs2)>;

def FABS : ABS<0b000101, 0b00000, 0b00100,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"fabs\t$rd,$rs1",
	[(set GRegs:$rd, (fabs (f64 GRegs:$rs1)))]>;
def FNEG : ALU_DR<0b110001, 0b10101,
	(outs GRegs:$rd), (ins GRegs:$rs2),
	"fadd\t$rd,#0,-$rs2",
	[(set GRegs:$rd, (fneg (f64 GRegs:$rs2)))]>;

// Non-commutative ops with constants on the left
def FSUBkr : ALU_DR<0b110001, 0b10101,
	(outs GRegs:$rd), (ins fpimmpi5:$imm, GRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (fsub fpimmpi5:$imm, (f64 GRegs:$rs2)))]>;
def FSUBfr : ALU_DR<0b110001, 0b10101,
	(outs GRegs:$rd), (ins fpimms32:$imm, GRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (fsub fpimms32:$imm, (f64 GRegs:$rs2)))]>;
def FSUBdr : ALU_DR<0b110001, 0b10101,
	(outs GRegs:$rd), (ins fpimm64:$imm, GRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (fsub fpimm64:$imm, (f64 GRegs:$rs2)))]>;

multiclass FMArrr<bits<3> OpC, bits<3>Flags, SDPatternOperator OpNode> {
  def rr : ALU_RRR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs1,$rs2,$rs3",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (f64 GRegs:$rs2),
				 (f64 GRegs:$rs3)))]>;
  def rn : ALU_RRR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs1,$rs2,-$rs3",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (f64 GRegs:$rs2),
				 (fneg (f64 GRegs:$rs3))))]>;
  def nr : ALU_RRR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs2,-$rs1,$rs3",	// must swap rs1 and rs2
	[(set GRegs:$rd, (OpNode (fneg GRegs:$rs1), (f64 GRegs:$rs2),
				 (f64 GRegs:$rs3)))]>;
  def nn : ALU_RRR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs2,-$rs1,-$rs3",	// must swap rs1 and rs2
	[(set GRegs:$rd, (OpNode (fneg GRegs:$rs1), (f64 GRegs:$rs2),
				 (fneg (f64 GRegs:$rs3))))]>;
  def rf : ALU_RRR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, fpimms32:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (f64 GRegs:$rs2),
				 fpimms32:$imm))]>;
  def rd : ALU_RRR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, fpimm64:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (f64 GRegs:$rs2),
				 fpimm64:$imm))]>;
  def fr : ALU_RRR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimms32:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimms32:$imm,
				 (f64 GRegs:$rs3)))]>;
  def dr : ALU_RRR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimm64:$imm,
				 (f64 GRegs:$rs3)))]>;
}
// FMA 64-bit FMA - sometimes we get fma and sometime fmad
defm FMA  : FMArrr<0b000, 0b000, fma>;
defm FMAD : FMArrr<0b000, 0b000, fmad>;

// Matched in DAGToDAG
def EADDrr : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"eadd\t$rd,$rs1,$rs2",
	[(set GRegs:$rd, (fldexp (f64 GRegs:$rs1), GRegs:$rs2))]>;
def EADDrk : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, simm5:$imm),
	"eadd\t$rd,$rs1,#$imm",
	[]>;
def EADDrd : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"eadd\t$rd,$rs1,#$imm",
	[]>;

def FREXPr : Pseudo<
	(outs GRegs:$rf, GRegs:$re), (ins GRegs:$rs),
	"# FREXPr",
	[(set GRegs:$rf, GRegs:$re, (ffrexp GRegs:$rs))]>;

def EXPONr : R1<0b000110, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs),
	"expon\t$rd,$rs",
	[]>;
def FRACTr : R1<0b000111, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs),
	"fract\t$rd,$rs",
	[]>;

// Moves
let isAsCheapAsAMove = 1 in {
def FMOVrz : ALU_RRR<0b000, 0b111,
	(outs GRegs:$rd), (ins fpimm64:$imm),
	"mov\t$rd,#0",
	[(set (f64 GRegs:$rd), fpimm0:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
def FMOVrk : ALU_RRR<0b000, 0b111,
	(outs GRegs:$rd), (ins fpimmi5:$imm),
	"cvtsd\t$rd,#$imm",
	[(set (f64 GRegs:$rd), fpimmi5:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
def FMOVrf : ALU_RRR<0b000, 0b111,
	(outs GRegs:$rd), (ins fpimms32:$imm),
	"cvtfd\t$rd,#$imm",
	[(set (f64 GRegs:$rd), fpimms32:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
def FMOVrd : ALU_RRR<0b000, 0b111,
	(outs GRegs:$rd), (ins fpimm64:$imm),
	"mov\t$rd,#$imm",
	[(set (f64 GRegs:$rd), fpimm64:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
}
def : Pat<(myCMOV (f64 GRegs:$rs1), (f64 GRegs:$rs2), GRegs:$rs3),
          (CMOVrrr GRegs:$rs1, GRegs:$rs2, GRegs:$rs3)>;
def : Pat<(myMUX (f64 GRegs:$rs1), (f64 GRegs:$rs2), GRegs:$rs3),
          (MUXrrr GRegs:$rs1, GRegs:$rs2, GRegs:$rs3)>;
def MUXFPrrf :  ALU_RRR<0b001, 0b110,
	(outs GRegs:$rd), (ins fpimms32:$imm, GRegs:$rs1, GRegs:$rs3),
	"mux\t$rd,$rs3,$rs1,#$imm",
	[(set (f64 GRegs:$rd), (myMUX fpimms32:$imm, GRegs:$rs1, GRegs:$rs3))]>;
def MUXFPrrd :  ALU_RRR<0b001, 0b110,
	(outs GRegs:$rd), (ins fpimm64:$imm, GRegs:$rs1, GRegs:$rs3),
	"mux\t$rd,$rs3,$rs1,#$imm",
	[(set (f64 GRegs:$rd), (myMUX fpimm64:$imm, GRegs:$rs1, GRegs:$rs3))]>;

// Converts
def CVTud : CVT<0b010000, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtud\t$rd,$rs1",
	[(set (f64 GRegs:$rd), (uint_to_fp GRegs:$rs1))]>;
def CVTsd : CVT<0b011000, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtsd\t$rd,$rs1",
	[(set (f64 GRegs:$rd), (sint_to_fp GRegs:$rs1))]>;
def CVTdu : CVT<0b000000, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtdu\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_uint (f64 GRegs:$rs1)))]>;
def CVTds : CVT<0b100000, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (f64 GRegs:$rs1)))]>;
def CVTdsni : CVT<0b100111, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#7",
	[(set (i64 GRegs:$rd), (fp_to_sint (ffloor (f64 GRegs:$rs1))))]>;
def CVTdspi : CVT<0b100110, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#6",
	[(set (i64 GRegs:$rd), (fp_to_sint (fceil (f64 GRegs:$rs1))))]>;
def CVTdstz : CVT<0b100101, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#5",
	[(set (i64 GRegs:$rd), (fp_to_sint (ftrunc (f64 GRegs:$rs1))))]>;
def CVTdsaz : CVT<0b100100, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#4",
	[(set (i64 GRegs:$rd), (fp_to_sint (fround (f64 GRegs:$rs1))))]>;
def CVTdsrm : CVT<0b100100, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#2",
	[(set (i64 GRegs:$rd), (fp_to_sint (fnearbyint (f64 GRegs:$rs1))))]>;

// Rounds
multiclass RNDrr<string mode, SDPatternOperator OpNode> {
    def rr : RND<0b10010, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	!strconcat("rnd\t$rd,$rs1,#", mode),
	[(set (f64 GRegs:$rd), (OpNode (f64 GRegs:$rs1)))]>;
}
defm RNDRM : RNDrr<"2", fnearbyint>;
defm RNDAZ : RNDrr<"4", fround>;
defm RNDTZ : RNDrr<"5", ftrunc>;
defm RNDPI : RNDrr<"6", fceil>;
defm RNDNI : RNDrr<"7", ffloor>;
def  RNDNE : RND<0b10010, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"rnd\t$rd,$rs1,#0",
	[]>;

// Patterns for rounding and conversion
def : Pat<(sint_to_fp (i64 (fp_to_sint (f64 GRegs:$Rn)))),
	  (RNDTZrr (f64 GRegs:$Rn))>;
def : Pat<(sint_to_fp (i64 (AssertSext (fp_to_sint (f64 GRegs:$Rn))))),
	  (RNDTZrr (f64 GRegs:$Rn))>;
def : Pat<(fpextend (f32 (sint_to_fp GRegs:$Rn))),
	  (CVTsd GRegs:$Rn)>;

// FP64 remainder - requires CARRY instruction
// The following are expanded by the ExpandPseudoInsts pass
def FREMrrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# FREMrrc",
	[(set GRegs:$div, GRegs:$rem, (myFDIVREM GRegs:$lhs, GRegs:$rhs))]>;
def FREMrkc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, fpimmi5:$imm),
	"# FREMrkc",
	[(set GRegs:$div, GRegs:$rem, (myFDIVREM GRegs:$lhs, fpimmi5:$imm))]>;
def FREMrfc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, fpimms32:$imm),
	"# FREMrfc",
	[(set GRegs:$div, GRegs:$rem, (myFDIVREM GRegs:$lhs, fpimms32:$imm))]>;
def FREMrdc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, fpimm64:$rhs),
	"# FREMrdc",
	[(set GRegs:$div, GRegs:$rem, (myFDIVREM GRegs:$lhs, fpimm64:$rhs))]>;
def FREMjrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins fpimmpi5:$imm, GRegs:$rhs),
	"# FREMjrc",
	[(set GRegs:$div, GRegs:$rem, (myFDIVREM fpimmpi5:$imm, (f64 GRegs:$rhs)))]>;
def FREMfrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins fpimms32:$imm, GRegs:$rhs),
	"# FREMdrc",
	[(set GRegs:$div, GRegs:$rem, (myFDIVREM fpimms32:$imm, GRegs:$rhs))]>;
def FREMdrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins fpimm64:$lhs, GRegs:$rhs),
	"# FREMdrc",
	[(set GRegs:$div, GRegs:$rem, (myFDIVREM fpimm64:$lhs, GRegs:$rhs))]>;

// Transcendentals

// transcendentals defined in LLVM
multiclass FTRAN<string OpName,  bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def x : TRAN<OpC, Flags,
	    (outs GRegs:$rd), (ins GRegs:$rs1),
	    !strconcat(OpName, "\t$rd,$rs1"),
	    [(set (f64 GRegs:$rd), (OpNode (f64 GRegs:$rs1)))]>;
}
// transcendentals not defined in LLVM
multiclass UTRAN<string OpName,  bits<6>OpC, bits<5>Flags> {
  def u : TRAN<OpC, Flags,
	    (outs GRegs:$rd), (ins GRegs:$rs1),
	    !strconcat(OpName, "\t$rd,$rs1"),
	    []>;
}
defm SQRT :   FTRAN<"fsqrt",  0b001101, 0b00000, fsqrt>;
defm FLOG2 :  FTRAN<"fln2",   0b000000, 0b00000, flog2>;
defm FLOG :   FTRAN<"fln",    0b000001, 0b00000, flog>;
defm FLOG10 : FTRAN<"flog",   0b000010, 0b00000, flog10>;
defm FEXP2 :  FTRAN<"fexp2",  0b000100, 0b00000, fexp2>;
defm FEXP :   FTRAN<"fexp",   0b000101, 0b00000, fexp>;
defm FSIN :   FTRAN<"fsin",   0b011000, 0b00000, fsin>;
defm FCOS :   FTRAN<"fcos",   0b011001, 0b00000, fcos>;
defm FTAN :   UTRAN<"ftan",   0b011010, 0b00000>;
defm RSQRT :  UTRAN<"frsqrt", 0b001100, 0b00000>;
defm FEXP10:  UTRAN<"fexp10", 0b000110, 0b00000>;
defm FASIN :  UTRAN<"fasin",  0b011100, 0b00000>;
defm FACOS :  UTRAN<"facos",  0b011101, 0b00000>;
defm FATAN :  UTRAN<"fatan",  0b011110, 0b00000>;

// Patterns to use reciprocal square root
def : Pat<(fdiv fpimm1, (fsqrt (f64 GRegs:$bot))),
	(RSQRTu (f64 GRegs:$bot))>;
def : Pat<(fdiv fpimmi5:$top, (fsqrt (f64 GRegs:$bot))),
	(FMULrk (RSQRTu (f64 GRegs:$bot)), fpimmi5:$top)>;
def : Pat<(fdiv fpimms32:$top, (fsqrt (f64 GRegs:$bot))),
	(FMULrf (RSQRTu (f64 GRegs:$bot)), fpimms32:$top)>;
def : Pat<(fdiv fpimm64:$top, (fsqrt (f64 GRegs:$bot))),
	(FMULrd (RSQRTu (f64 GRegs:$bot)), fpimm64:$top)>;
def : Pat<(fdiv (f64 GRegs:$top), (fsqrt (f64 GRegs:$bot))),
	(FMULrr (f64 GRegs:$top), (RSQRTu (f64 GRegs:$bot)))>;

// These bitcasts are NOPs
def       : Pat<(bitconvert (f64 GRegs:$rs)),
		(i64 GRegs:$rs)>;
def       : Pat<(bitconvert (i64 GRegs:$rs)),
		(f64 GRegs:$rs)>;

let mayLoad = 1 in {
  defm LDG : Load<"ldd",  0b100011, 0b000011, f64, load>;
}

let mayStore = 1 in {
  defm STG : Store<"std", 0b101011, 0b001011, f64, store>;
}
