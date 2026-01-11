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
def fpimm16  : Operand<f64>, FPImmLeaf<f64, [{ return isFPhalf(Imm); }]>
		{ let PrintMethod = "printFP16Operand"; }
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

multiclass FPrr<string OpName, bits<7>OpC, SDPatternOperator OpNode> {
  def rr : XOP3_R<OpC, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[(set (f64 GRegs:$rd), (OpNode (f64 GRegs:$rs1),
				       (f64 GRegs:$rs2)))]>;
  def rn : XOP3_R<OpC, 0b0100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,-$rs2"),
	[(set (f64 GRegs:$rd), (OpNode (f64 GRegs:$rs1),
				       (fneg (f64 GRegs: $rs2))))]>;
  def nr : XOP3_R<OpC, 0b1000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,$rs2"),
	[(set (f64 GRegs:$rd), (OpNode (fneg (f64 GRegs: $rs1)),
				       (f64 GRegs:$rs2)))]>;
  def nn : XOP3_R<OpC, 0b1100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,-$rs2"),
	[(set (f64 GRegs:$rd), (OpNode (fneg (f64 GRegs: $rs1)),
				       (fneg (f64 GRegs: $rs2))))]>;
  def rk : XOP3_R<OpC, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimmi5:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimmi5:$imm))]>;
  def rf : XOP3_W<OpC, 0b0010,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimms32:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimms32:$imm))]>;
  def rd : XOP3_D<OpC, 0b0011,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimm64:$imm))]>;
  def jr : XOP3_R<OpC, 0b0101,
	(outs GRegs:$rd), (ins fpimmpi5:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd,  (OpNode fpimmpi5:$imm, (f64 GRegs:$rs2)))]>;
  def fr : XOP3_W<OpC, 0b0110,
	(outs GRegs:$rd), (ins fpimms32:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd,  (OpNode fpimms32:$imm, (f64 GRegs:$rs2)))]>;
  def dr : XOP3_D<OpC, 0b0111,
	(outs GRegs:$rd), (ins fpimm64:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd,  (OpNode fpimm64:$imm, (f64 GRegs:$rs2)))]>;
}

defm FADD :  FPrr<"fadd",  0b1000010, fadd>;
defm FMUL :  FPrr<"fmul",  0b1000100, fmul>;
defm FDIV :  FPrr<"fdiv",  0b1000110, fdiv>;
// Caution: the following semantics change with changes to IEEE-754
defm FMAXN:  FPrr<"fmax",  0b1001100, fmaxnum>;
defm FMINN:  FPrr<"fmin",  0b1001110, fminnum>;
defm FMAX :  FPrr<"fmax",  0b1001100, fmaximum>;
defm FMIN :  FPrr<"fmin",  0b1001110, fminimum>;
defm CSGN :  FPrr<"csgn",  0b1010010, fcopysign>;
defm FPOW :  FPrr<"fpow",  0b1011010, fpow>;

// Can't use the standard pattern because the output type doesn't match inputs.
let isCompare = 1 in {
def FCMPrr : XOP3_R<0b1101000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"fcmp\t$rd,$rs1,$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), (f64 GRegs:$rs2)))]>;
def FCMPrn : XOP3_R<0b1101000, 0b0100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"fcmp\t$rd,$rs1,-$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1),
				       (fneg (f64 GRegs: $rs2))))]>;
def FCMPnr : XOP3_R<0b1101000, 0b1000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"fcmp\t$rd,-$rs1,$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (fneg (f64 GRegs: $rs1)),
				       (f64 GRegs:$rs2)))]>;
def FCMPnn : XOP3_R<0b1101000, 0b1100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"fcmp\t$rd,-$rs1,-$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (fneg (f64 GRegs: $rs1)),
				       (f64 GRegs:$rs2)))]>;
def FCMPrk : XOP3_R<0b1101000, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimmi5:$imm),
	"fcmp\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), fpimmi5:$imm))]>;
def FCMPrf : XOP3_W<0b1101000, 0b0010,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimms32:$imm),
	"fcmp\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), fpimms32:$imm))]>;
def FCMPrd : XOP3_D<0b1101000, 0b0011,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm),
	"fcmp\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), fpimm64:$imm))]>;
}

def FSUB  : Pat<(fsub (f64 GRegs:$rs1), (f64 GRegs:$rs2)),
		(FADDrn GRegs:$rs1, GRegs:$rs2)>;
def NFADD : Pat<(fsub (fneg (f64 GRegs:$rs1)), (f64 GRegs:$rs2)),
		(FADDnn GRegs:$rs1, GRegs:$rs2)>;

def FABS : XOP6_R<0b0111000, 0b00010, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"fabs\t$rd,$rs1",
	[(set GRegs:$rd, (fabs (f64 GRegs:$rs1)))]>;
def FNEG : XOP3_R<0b1100010, 0b1101,
	(outs GRegs:$rd), (ins GRegs:$rs2),
	"fadd\t$rd,#0.0,-$rs2",
	[(set GRegs:$rd, (fneg (f64 GRegs:$rs2)))]>;

// Non-commutative ops with constants on the left
def FSUBjr : XOP3_R<0b1100010, 0b0101,
	(outs GRegs:$rd), (ins fpimmpi5:$imm, GRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (fsub fpimmpi5:$imm, (f64 GRegs:$rs2)))]>;
def FSUBfr : XOP3_W<0b1100010, 0b0110,
	(outs GRegs:$rd), (ins fpimms32:$imm, GRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (fsub fpimms32:$imm, (f64 GRegs:$rs2)))]>;
def FSUBdr : XOP3_D<0b1100010, 0b0111,
	(outs GRegs:$rd), (ins fpimm64:$imm, GRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (fsub fpimm64:$imm, (f64 GRegs:$rs2)))]>;

multiclass FMArrr<SDPatternOperator OpNode> {
  def rr : FMA_R<0b00, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs1,$rs2,$rs3",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (f64 GRegs:$rs2),
				 (f64 GRegs:$rs3)))]>;
  def rn : FMA_R<0b00, 0b0100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs1,$rs2,-$rs3",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (f64 GRegs:$rs2),
				 (fneg (f64 GRegs:$rs3))))]>;
  def nr : FMA_R<0b00, 0b1000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs2,-$rs1,$rs3",	// must swap rs1 and rs2
	[(set GRegs:$rd, (OpNode (fneg GRegs:$rs1), (f64 GRegs:$rs2),
				 (f64 GRegs:$rs3)))]>;
  def nn : FMA_R<0b00, 0b1100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs2,-$rs1,-$rs3",	// must swap rs1 and rs2
	[(set GRegs:$rd, (OpNode (fneg GRegs:$rs1), (f64 GRegs:$rs2),
				 (fneg (f64 GRegs:$rs3))))]>;
  def rk : FMA_R<0b00, 0b1001,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, fpimms32:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (f64 GRegs:$rs2),
				 fpimmi5:$imm))]>;
  def rf : FMA_R<0b00, 0b0010,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, fpimms32:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (f64 GRegs:$rs2),
				 fpimms32:$imm))]>;
  def rd : FMA_R<0b00, 0b0011,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, fpimm64:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (f64 GRegs:$rs2),
				 fpimm64:$imm))]>;
  def jr : FMA_R<0b00, 0b0101,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimms32:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimmpi5:$imm,
				 (f64 GRegs:$rs3)))]>;
  def fr : FMA_R<0b00, 0b0110,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimms32:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimms32:$imm,
				 (f64 GRegs:$rs3)))]>;
  def dr : FMA_R<0b00, 0b0111,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimm64:$imm,
				 (f64 GRegs:$rs3)))]>;
}
// FMA 64-bit FMA - sometimes we get fma and sometime fmad
defm FMA  : FMArrr<fma>;
defm FMAD : FMArrr<fmad>;

// Matched in DAGToDAG
def EADDrr : XOP3_R<0b1001000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"eadd\t$rd,$rs1,$rs2",
	[(set GRegs:$rd, (fldexp (f64 GRegs:$rs1), (i64 GRegs:$rs2)))]>;
def EADDrj : XOP3_R<0b1001000, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, simmp5:$imm),
	"eadd\t$rd,$rs1,#$imm",
	[]>;
def EADDrk : XOP3_R<0b1001000, 0b1001,
	(outs GRegs:$rd), (ins GRegs:$rs1, simmn5:$imm),
	"eadd\t$rd,$rs1,#$imm",
	[]>;
def EADDrd : XOP3_W<0b1001000, 0b0011,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"eadd\t$rd,$rs1,#$imm",
	[]>;

def FREXPr : Pseudo<
	(outs GRegs:$rf, GRegs:$re), (ins GRegs:$rs),
	"# FREXPr",
	[(set GRegs:$rf, GRegs:$re, (ffrexp GRegs:$rs))]>;

def EXPONr : XOP6_R<0b0111000, 0b00010, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs),
	"expon\t$rd,$rs",
	[]>;
def FRACTr : XOP6_R<0b0111000, 0b00011, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs),
	"fract\t$rd,$rs",
	[]>;

// Moves
let isAsCheapAsAMove = 1 in {
def FMOVrz : MOV_RI<0b111011, 0b00000,
	(outs GRegs:$rd), (ins fpimm64:$imm),
	"mov\t$rd,#0",
	[(set (f64 GRegs:$rd), fpimm0:$imm)]>;
def FMOVrh : MOV_RI<0b111011, 0b00001,
	(outs GRegs:$rd), (ins fpimm16:$imm),
	"movd\t$rd,#$imm",
	[(set (f64 GRegs:$rd), fpimm16:$imm)]>;
def FMOVrf : CVT_R<0b0110000, 0b0010,
	(outs GRegs:$rd), (ins fpimms32:$imm),
	"cvtfd\t$rd,#$imm",
	[(set (f64 GRegs:$rd), fpimms32:$imm)]>;
def FMOVrd : XOP6_W<0b0111100, 0b00000, 0b0011,
	(outs GRegs:$rd), (ins fpimm64:$imm),
	"mov\t$rd,#$imm",
	[(set (f64 GRegs:$rd), fpimm64:$imm)]>;
}

def : Pat<(cmov (f64 GRegs:$rs1), (f64 GRegs:$rs2), GRegs:$rs3),
          (CMOVrrr GRegs:$rs1, GRegs:$rs2, GRegs:$rs3)>;

// Converts
def CVTud : CVT_R<0b0100000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtud\t$rd,$rs1",
	[(set (f64 GRegs:$rd), (uint_to_fp (i64 GRegs:$rs1)))]>;
def CVTsd : CVT_R<0b0101000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtsd\t$rd,$rs1",
	[(set (f64 GRegs:$rd), (sint_to_fp (i64 GRegs:$rs1)))]>;
def CVTdu : CVT_R<0b0110000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtdu\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_uint (f64 GRegs:$rs1)))]>;
let IsSignExtendingOp = 1 in {
def CVTds : CVT_R<0b0111000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (f64 GRegs:$rs1)))]>;
def CVTdsni : CVT_R<0b0111000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#7",
	[(set (i64 GRegs:$rd), (fp_to_sint (ffloor (f64 GRegs:$rs1))))]>;
def CVTdspi : CVT_R<0b0111000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#6",
	[(set (i64 GRegs:$rd), (fp_to_sint (fceil (f64 GRegs:$rs1))))]>;
def CVTdstz : CVT_R<0b0111000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#5",
	[(set (i64 GRegs:$rd), (fp_to_sint (ftrunc (f64 GRegs:$rs1))))]>;
def CVTdsaz : CVT_R<0b0111000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#4",
	[(set (i64 GRegs:$rd), (fp_to_sint (fround (f64 GRegs:$rs1))))]>;
def CVTdsrm : CVT_R<0b0111000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtds\t$rd,$rs1,#2",
	[(set (i64 GRegs:$rd), (fp_to_sint (fnearbyint (f64 GRegs:$rs1))))]>;
}

// Rounds
multiclass RNDrr<string mode, SDPatternOperator OpNode> {
    def rr : CVT_R<0b1100000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	!strconcat("rnd\t$rd,$rs1,#", mode),
	[(set (f64 GRegs:$rd), (OpNode (f64 GRegs:$rs1)))]>;
}
defm RNDRM : RNDrr<"2", fnearbyint>;
defm RNDAZ : RNDrr<"4", fround>;
defm RNDTZ : RNDrr<"5", ftrunc>;
defm RNDPI : RNDrr<"6", fceil>;
defm RNDNI : RNDrr<"7", ffloor>;
def  RNDNE : CVT_R<0b0110000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"rnd\t$rd,$rs1,#0",
	[]>;

// Patterns for rounding and conversion
def : Pat<(sint_to_fp (i64 (fp_to_sint (f64 GRegs:$Rn)))),
	  (RNDTZrr (f64 GRegs:$Rn))>;
def : Pat<(sint_to_fp (i64 (AssertSext (i64 (fp_to_sint (f64 GRegs:$Rn)))))),
	  (RNDTZrr (f64 GRegs:$Rn))>;
def : Pat<(fpextend (f32 (sint_to_fp (i64 GRegs:$Rn)))),
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
multiclass FTRAN<string OpName,  bits<5>func, SDPatternOperator OpNode> {
  def r : XOP6_R<0b1111000, func, 0b0000,
	    (outs GRegs:$rd), (ins GRegs:$rs1),
	    !strconcat(OpName, "\t$rd,$rs1"),
	    [(set (f64 GRegs:$rd), (OpNode (f64 GRegs:$rs1)))]>;
}
// transcendentals not defined in LLVM
multiclass UTRAN<string OpName, bits<5>func> {
  def u : XOP6_R<0b1111000, func, 0b0000,
	    (outs GRegs:$rd), (ins GRegs:$rs1),
	    !strconcat(OpName, "\t$rd,$rs1"),
	    []>;
}
defm SQRT :   FTRAN<"fsqrt",  0b01101, fsqrt>;
defm FLOG2 :  FTRAN<"fln2",   0b00000, flog2>;
defm FLOG :   FTRAN<"fln",    0b00001, flog>;
defm FLOG10 : FTRAN<"flog",   0b00010, flog10>;
defm FEXP2 :  FTRAN<"fexp2",  0b00100, fexp2>;
defm FEXP :   FTRAN<"fexp",   0b00101, fexp>;
defm FSIN :   FTRAN<"fsin",   0b11000, fsin>;
defm FCOS :   FTRAN<"fcos",   0b11001, fcos>;
defm FTAN :   UTRAN<"ftan",   0b11010>;
defm RSQRT :  UTRAN<"frsqrt", 0b01100>;
defm FEXP10:  UTRAN<"fexp10", 0b00110>;
defm FASIN :  UTRAN<"fasin",  0b11100>;
defm FACOS :  UTRAN<"facos",  0b11101>;
defm FATAN :  UTRAN<"fatan",  0b11110>;

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
  defm LDG : Load<"ldd",  0b100011, 0b1100000, f64, load>;
}

let mayStore = 1 in {
  defm STG : Store<"std", 0b101011, 0b1100100, f64, store>;
}
// Because ADDRrr rejects anything ADDRri can handle, we need a pattern for both.
// But they both map to LDST_RX
def STGZri : LDST_RX<0b1111000, 0b00,
	(outs), (ins fpimm64:$imm, MEMri:$addr),
	"std\t#0,[$addr]",
	[(store (f64 fpimm0:$imm), ADDRri:$addr)]>;
def STGZrr : LDST_RX<0b1111000, 0b00,
	(outs), (ins fpimm64:$imm, MEMrr:$addr),
	"std\t#0,[$addr]",
	[(store (f64 fpimm0:$imm), ADDRrr:$addr)]>;
