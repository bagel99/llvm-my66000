//===- My66000InstrInfoFS.td - FP Instr Description for My66000 ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


def fpimm32 : Operand<f32>, FPImmLeaf<f32, [{ return true; }]>
		{ let PrintMethod = "printFP32Operand"; }
def fsimmpi5 : Operand<f32>, FPImmLeaf<f32, [{ return isFPpimm5(Imm); }]>
		{ let PrintMethod = "printFPkOperand"; }
def fsimmi5 : Operand<f32>, FPImmLeaf<f32, [{ return isFPimm5(Imm); }]>
		{ let PrintMethod = "printFPkOperand"; }

/*
   32-bit floating point uses different virtual registers, FSRegs
   (The same physical registers are used.)
*/

multiclass FSrr<string OpName, bits<7>OpC, SDPatternOperator OpNode> {
  def rr : XOP3_R<OpC, 0b0000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1),
				        (f32 FSRegs:$rs2)))]>;
  def rn : XOP3_R<OpC, 0b0001,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,-$rs2"),
	[(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1),
					(fneg (f32 FSRegs:$rs2))))]>;
  def nr : XOP3_R<OpC, 0b0010,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,$rs2"),
	[(set (f32 FSRegs:$rd), (OpNode (fneg (f32 FSRegs:$rs1)),
				        (f32 FSRegs:$rs2)))]>;
  def nn : XOP3_R<OpC, 0b0011,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,-$rs2"),
	[(set (f32 FSRegs:$rd), (OpNode (fneg (f32 FSRegs:$rs1)),
					(fneg (f32 FSRegs:$rs2))))]>;
  def rk : XOP3_W<OpC, 0b0100,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, fsimmi5:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), fsimmi5:$imm))]>;
  def rf : XOP3_W<OpC, 0b1000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), fpimm32:$imm))]>;
  def jr : XOP3_W<OpC, 0b0101,
	(outs FSRegs:$rd), (ins fsimmpi5:$imm, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set FSRegs:$rd, (OpNode fsimmpi5:$imm, (f32 FSRegs:$rs2)))]>;
  def fr : XOP3_W<OpC, 0b1001,
	(outs FSRegs:$rd), (ins fpimm32:$imm, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set FSRegs:$rd, (OpNode fpimm32:$imm, (f32 FSRegs:$rs2)))]>;
}
defm FADDF : FSrr<"faddf", 0b1100010, fadd>;
defm FMULF : FSrr<"fmulf", 0b1100100, fmul>;
defm FDIVF : FSrr<"fdivf", 0b1100110, fdiv>;
defm FMAXNF: FSrr<"fmaxf", 0b1101100, fmaxnum>;
defm FMINNF: FSrr<"fminf", 0b1101110, fminnum>;
defm FMAXF : FSrr<"fmaxf", 0b1101100, fmaximum>;
defm FMINF : FSrr<"fminf", 0b1101110, fminimum>;
defm CSGNF : FSrr<"csgnf", 0b1110010, fcopysign>;
defm FPOWF : FSrr<"fpowf", 0b1111010, fpow>;

let isCompare = 1 in {
def FCMPFrr : XOP3_R<0b1101000, 0b0000,
       (outs GRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
       "fcmpf\t$rd,$rs1,$rs2",
       [(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1), (f32 FSRegs:$rs2)))]>;
def FCMPFrn : XOP3_R<0b1101000, 0b0001,
       (outs GRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
       "fcmpf\t$rd,$rs1,-$rs2",
       [(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1),
                                      (fneg (f32 FSRegs:$rs2))))]>;
def FCMPFnr : XOP3_R<0b1101000, 0b0010,
       (outs GRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
       "fcmpf\t$rd,-$rs1,$rs2",
       [(set (i64 GRegs:$rd), (myFCMP (fneg (f32 FSRegs:$rs1)),
                                      (f32 FSRegs:$rs2)))]>;
def FCMPFnn : XOP3_R<0b1101000, 0b0011,
       (outs GRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
       "fcmpf\t$rd,-$rs1,-$rs2",
       [(set (i64 GRegs:$rd), (myFCMP (fneg (f32 FSRegs:$rs1)),
                                      (fneg (f32 FSRegs:$rs2))))]>;
def FCMPFrk : XOP3_R<0b1101000, 0b0100,
       (outs GRegs:$rd), (ins FSRegs:$rs1, fsimmi5:$imm),
       "fcmpf\t$rd,$rs1,#$imm",
       [(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1), fsimmi5:$imm))]>;
def FCMPFrw : XOP3_W<0b1101000, 0b1000,
       (outs GRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm),
       "fcmpf\t$rd,$rs1,#$imm",
       [(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1), fpimm32:$imm))]>;
 }


def FSUBF : Pat<(fsub (f32 FSRegs:$rs1), (f32 FSRegs:$rs2)),
		(FADDFrn FSRegs:$rs1, FSRegs:$rs2)>;
def NFADDF: Pat<(fsub (fneg (f32 FSRegs:$rs1)), (f32 FSRegs:$rs2)),
		(FADDFnn FSRegs:$rs1, FSRegs:$rs2)>;

def FABSF : XOP6_R<0b0111001, 0b00010, 0b0000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1),
	"fabsf\t$rd,$rs1",
	[(set FSRegs:$rd, (fabs (f32 FSRegs:$rs1)))]>;
def FNEGF : XOP3_R<0b1100010, 0b0001,
	(outs FSRegs:$rd), (ins FSRegs:$rs2),
	"faddf\t$rd,#0,-$rs2",
	[(set FSRegs:$rd, (fneg (f32 FSRegs:$rs2)))]>;

// Non-commutative ops with constants on the left
def FSUBFkr : XOP3_R<0b1100010, 0b0101,
	(outs FSRegs:$rd), (ins fsimmpi5:$imm, FSRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set FSRegs:$rd, (fsub fsimmpi5:$imm, (f32 FSRegs:$rs2)))]>;
def FSUBFfr : XOP3_W<0b1100010, 0b1001,
	(outs FSRegs:$rd), (ins fpimm32:$imm, FSRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set FSRegs:$rd, (fsub fpimm32:$imm, (f32 FSRegs:$rs2)))]>;

multiclass FMAFrrr<SDPatternOperator OpNode> {
  def rr : FMA_R<0b01, 0b0000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,$rs2,$rs3",
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), (f32 FSRegs:$rs2),
				  (f32 FSRegs:$rs3)))]>;
  def rn : FMA_R<0b01, 0b0001,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,$rs2,-$rs3",
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), (f32 FSRegs:$rs2),
				   (fneg (f32 FSRegs:$rs3))))]>;
  def nr : FMA_R<0b01, 0b0010,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs2,-$rs1,$rs3",	// must swap rs1 and rs2
	[(set FSRegs:$rd, (OpNode (fneg FSRegs:$rs1), (f32 FSRegs:$rs2),
				  (f32 FSRegs:$rs3)))]>;
  def nn : FMA_R<0b01, 0b0011,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs2,-$rs1,-$rs3",	// must swap rs1 and rs2
	[(set FSRegs:$rd, (OpNode (fneg FSRegs:$rs1), (f32 FSRegs:$rs2),
				  (fneg (f32 FSRegs:$rs3))))]>;
  def rf : FMA_R<0b01, 0b0100,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, fpimm32:$imm),
	"fmacf\t$rd,$rs1,$rs2,#$imm",
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), (f32 FSRegs:$rs2),
				   fpimm32:$imm))]>;
  def fr : FMA_R<0b01, 0b0110,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,#$imm,$rs3",
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), fpimm32:$imm,
				  (f32 FSRegs:$rs3)))]>;
}
// FMA 32-bit FMA - sometimes we get fma and sometime fmad
defm FMAF  : FMAFrrr<fma>;
defm FMADF : FMAFrrr<fmad>;

// Matched in DAGToDAG
def EADDFrr : XOP3_R<0b1001001, 0b0000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, GRegs:$rs2),
	"eaddf\t$rd,$rs1,$rs2",
	[(set FSRegs:$rd, (fldexp (f32 FSRegs:$rs1), (i64 GRegs:$rs2)))]>;
def EADDFrk : XOP3_R<0b1001001, 0b0001,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, simm5:$imm),
	"eaddf\t$rd,$rs1,#$imm",
	[]>;
def EADDFrd : XOP3_R<0b1001001, 0b0011,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, i64imm:$imm),
	"eaddf\t$rd,$rs1,#$imm",
	[]>;

def FREXPFr : Pseudo<
	(outs FSRegs:$rf, GRegs:$re), (ins FSRegs:$rs),
	"# FREXPFr",
	[(set FSRegs:$rf, GRegs:$re, (ffrexp (f32 FSRegs:$rs)))]>;

def EXPONFr : XOP6_R<0b0111001, 0b00010, 0b0000,
	(outs GRegs:$rd), (ins FSRegs:$rs),
	"exponf\t$rd,$rs",
	[]>;
def FRACTFr : XOP6_R<0b0111001, 0b00011, 0b0000,
	(outs FSRegs:$rd), (ins FSRegs:$rs),
	"fractf\t$rd,$rs",
	[]>;

// Moves
let isAsCheapAsAMove = 1 in {
def FMOVFrz : ALU_RI<0b111011,
	(outs FSRegs:$rd), (ins fpimm32:$imm),
	"mov\t$rd,#0",
	[(set (f32 FSRegs:$rd), fpimm0:$imm)]>;
def FMOVFrk : CVT_R<0b0110001, 0b1000,
	(outs FSRegs:$rd), (ins fsimmi5:$imm),
	"cvtsf\t$rd,#$imm",
	[(set (f32 FSRegs:$rd), fsimmi5:$imm)]>;
def FMOVFrf : ALU_RI<0b111011,
	(outs FSRegs:$rd), (ins fpimm32:$imm),
	"mov\t$rd,#$imm",
	[(set (f32 FSRegs:$rd), fpimm32:$imm)]>;
}

/* tried a Pat<> for this, but no joy */
def CMOVFSrrr: MUX_R<0b00, 0b0000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, GRegs:$rs3),
	"cmov\t$rd,$rs3,$rs1,$rs2",
	[(set (f32 FSRegs:$rd), (cmov (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), GRegs:$rs3))]>;

// Converts
def CVTuf : CVT_R<0b0100001, 0b0000,
	(outs FSRegs:$rd), (ins GRegs:$rs1),
	"cvtuf\t$rd,$rs1",
	[(set (f32 FSRegs:$rd), (uint_to_fp (i64 GRegs:$rs1)))]>;
def CVTsf : CVT_R<0b0101001, 0b0000,
	(outs FSRegs:$rd), (ins GRegs:$rs1),
	"cvtsf\t$rd,$rs1",
	[(set (f32 FSRegs:$rd), (sint_to_fp (i64 GRegs:$rs1)))]>;
def CVTfu : CVT_R<0b0110001, 0b0000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfu\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_uint (f32 FSRegs:$rs1)))]>;
let IsSignExtendingOp = 1 in {
def CVTfs : CVT_R<0b0111001, 0b0000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (f32 FSRegs:$rs1)))]>;
def CVTfsni : CVT_R<0b0111001, 0b0000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#7",
	[(set (i64 GRegs:$rd), (fp_to_sint (ffloor (f32 FSRegs:$rs1))))]>;
def CVTfspi : CVT_R<0b0111001, 0b0000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#6",
	[(set (i64 GRegs:$rd), (fp_to_sint (fceil (f32 FSRegs:$rs1))))]>;
def CVTfstz : CVT_R<0b0111001, 0b0000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#5",
	[(set (i64 GRegs:$rd), (fp_to_sint (ftrunc (f32 FSRegs:$rs1))))]>;
def CVTfsaz : CVT_R<0b0111001, 0b0000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#4",
	[(set (i64 GRegs:$rd), (fp_to_sint (fround (f32 FSRegs:$rs1))))]>;
def CVTfsrm : CVT_R<0b0111001, 0b0000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#2",
	[(set (i64 GRegs:$rd), (fp_to_sint (fnearbyint (f32 FSRegs:$rs1))))]>;
}
def CVTfdr : CVT_R<0b1101000, 0b0000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfd\t$rd,$rs1",
	[(set (f64 GRegs:$rd), (fpextend (f32 FSRegs:$rs1)))]>;
def CVTfdn : CVT_R<0b1101000, 0b1000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfd\t$rd,-$rs1",
	[(set (f64 GRegs:$rd), (fpextend (fneg (f32 FSRegs:$rs1))))]>;
def CVTdfr : CVT_R<0b1100001, 0b0000,
	(outs FSRegs:$rd), (ins GRegs:$rs1),
	"cvtdf\t$rd,$rs1",
	[(set (f32 FSRegs:$rd), (fpround (f64 GRegs:$rs1)))]>;
def CVTdfn : CVT_R<0b1100001, 0b1000,
	(outs FSRegs:$rd), (ins GRegs:$rs1),
	"cvtdf\t$rd,-$rs1",
	[(set (f32 FSRegs:$rd), (fneg (fpround (f64 GRegs:$rs1))))]>;

// Rounds
multiclass RNDFrr<string mode, SDPatternOperator OpNode> {
    def rr : CVT_R<0b1101001, 0b0000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1),
	!strconcat("rndf\t$rd,$rs1,#", mode),
	[(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1)))]>;
}
defm RNDRMF: RNDFrr<"2", fnearbyint>;
defm RNDAZF: RNDFrr<"4", fround>;
defm RNDTZF: RNDFrr<"5", ftrunc>;
defm RNDPIF: RNDFrr<"6", fceil>;
defm RNDNIF: RNDFrr<"7", ffloor>;
def RNDNEF: CVT_R<0b1101001, 0b0000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1),
	"rndf\t$rd,$rs1,#0",
	[]>;

//
// FP32 remainder - requires CARRY instruction
// The following are expanded by the ExpandPseudoInsts pass
def FREMFrrc : Pseudo<
	(outs FSRegs:$div, FSRegs:$rem), (ins FSRegs:$lhs, FSRegs:$rhs),
	"# FREMFrrc",
	[(set (f32 FSRegs:$div), (f32 FSRegs:$rem),
	    (myFDIVREM (f32 FSRegs:$lhs), (f32 FSRegs:$rhs)))]>;
def FREMFrkc : Pseudo<
	(outs FSRegs:$div, FSRegs:$rem), (ins FSRegs:$lhs, fsimmi5:$imm),
	"# FREMFrkc",
	[(set (f32 FSRegs:$div), (f32 FSRegs:$rem),
	    (myFDIVREM (f32 FSRegs:$lhs), fsimmi5:$imm))]>;
def FREMFrfc : Pseudo<
	(outs FSRegs:$div, FSRegs:$rem), (ins FSRegs:$lhs, fpimm32:$imm),
	"# FREMFrfc",
	[(set (f32 FSRegs:$div), (f32 FSRegs:$rem),
	    (myFDIVREM (f32 FSRegs:$lhs), fpimm32:$imm))]>;
def FREMFjrc : Pseudo<
	(outs FSRegs:$div, FSRegs:$rem), (ins fsimmpi5:$imm, FSRegs:$rhs),
	"# FREMFjrc",
	[(set (f32 FSRegs:$div), (f32 FSRegs:$rem),
	    (myFDIVREM fsimmpi5:$imm, (f32 FSRegs:$rhs)))]>;
def FREMFfrc : Pseudo<
	(outs FSRegs:$div, FSRegs:$rem), (ins fpimm32:$imm, FSRegs:$rhs),
	"# FREMFwcr",
	[(set (f32 FSRegs:$div), (f32 FSRegs:$rem),
	    (myFDIVREM fpimm32:$imm, (f32 FSRegs:$rhs)))]>;

// transcendentals defined in LLVM
multiclass FSTRAN<string OpName,  bits<5>func, SDPatternOperator OpNode> {
  def r : XOP6_R<0b1111001, func, 0b0000,
	    (outs FSRegs:$rd), (ins FSRegs:$rs1),
	    !strconcat(OpName, "\t$rd,$rs1"),
	    [(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1)))]>;
}
// transcendentals not defined in LLVM
multiclass USTRAN<string OpName, bits<5>func> {
  def u : XOP6_R<0b1111001, func, 0b0000,
	    (outs FSRegs:$rd), (ins FSRegs:$rs1),
	    !strconcat(OpName, "\t$rd,$rs1"),
	    []>;
}
defm SQRTF :  FSTRAN<"fsqrtf", 0b01101, fsqrt>;
defm FLOG2F : FSTRAN<"fln2f",  0b00000, flog2>;
defm FLOGF :  FSTRAN<"flnf",   0b00001, flog>;
defm FLOG10F: FSTRAN<"flogf",  0b00010, flog10>;
defm FEXP2F : FSTRAN<"fexp2f", 0b00100, fexp2>;
defm FEXPF :  FSTRAN<"fexpf",  0b00101, fexp>;
defm FSINF :  FSTRAN<"fsinf",  0b11000, fsin>;
defm FCOSF :  FSTRAN<"fcosf",  0b11001, fcos>;
defm FTANF :  USTRAN<"ftanf",  0b11010>;
defm RSQRTF : USTRAN<"frsqrtf",0b01100>;
defm FEXP10F: USTRAN<"fexp10f",0b00110>;
defm FASINF : USTRAN<"fasinf", 0b11100>;
defm FACOSF : USTRAN<"facosf", 0b11101>;
defm FATANF : USTRAN<"fatan",  0b11110>;

// Patterns to use reciprocal square root
def : Pat<(fdiv fpimm1, (fsqrt (f32 FSRegs:$bot))),
	(RSQRTFu (f32 FSRegs:$bot))>;
def : Pat<(fdiv fsimmi5:$top, (fsqrt (f32 FSRegs:$bot))),
	(FMULFrk (RSQRTFu (f32 FSRegs:$bot)), fsimmi5:$top)>;
def : Pat<(fdiv fpimm32:$top, (fsqrt (f32 FSRegs:$bot))),
	(FMULFrf (RSQRTFu (f32 FSRegs:$bot)), fpimm32:$top)>;
def : Pat<(fdiv (f32 FSRegs:$top), (fsqrt (f32 FSRegs:$bot))),
	(FMULFrr (f32 FSRegs:$top), (RSQRTFu (f32 FSRegs:$bot)))>;

let usesCustomInserter = 1 in {
def CPFMFS : Pseudo<
	(outs GRegs:$rd), (ins FSRegs:$rs),
	"# CPFMFS",
	[(set (i64 GRegs:$rd), (myCOPYFMFS (f32 FSRegs:$rs)))]>;
def CPTOFS : Pseudo<
	(outs FSRegs:$rd), (ins GRegs:$rs),
	"# CPTOFS",
	[(set (f32 FSRegs:$rd), (myCOPYTOFS GRegs:$rs))]>;
}
/*
// These bitcasts are NOPs
def       : Pat<(myCOPYFMF32 FSRegs:$rs),
		(CPFMF32 GRegs:$rs)>;
def       : Pat<(myCOPYTOF32 GRegs:$rs),
		(CPTOF32 FSRegs:$rs)>;
*/

// Loads and Stores
multiclass LoadFS<string OpName, bits<6>OpCri, bits<7>OpCrr,
		SDPatternOperator OpNode> {
  def ri : LDST_BD<OpCri,
	(outs FSRegs:$rd), (ins MEMri:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(set (f32 FSRegs:$rd), (OpNode ADDRri:$addr))]>;
  def rr : LDST_RX<OpCrr, 0b00,
	(outs FSRegs:$rd), (ins MEMrr:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(set (f32 FSRegs:$rd), (OpNode ADDRrr:$addr))]>;
}
multiclass StoreFS<string OpName, bits<6>OpCri, bits<7>OpCrr,
		 SDPatternOperator OpNode> {
  def ri : LDST_BD<OpCri,
	(outs), (ins FSRegs:$rd, MEMri:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(OpNode (f32 FSRegs:$rd), ADDRri:$addr)]>;
  def rr : LDST_RX<OpCrr, 0b00,
	(outs), (ins FSRegs:$rd, MEMrr:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(OpNode (f32 FSRegs:$rd), ADDRrr:$addr)]>;
}
let mayLoad = 1 in {
  defm LDF : LoadFS<"lduw", 0b100010, 0b0000000, load>;
}

let mayStore = 1 in {
  defm STF : StoreFS<"stw", 0b101010, 0b0000010, store>;
}
