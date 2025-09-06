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

multiclass FSrr<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def rr : ALU_RR<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1),
				        (f32 FSRegs:$rs2)))]>;
  def rn : ALU_RR<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,-$rs2"),
	[(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1),
					(fneg (f32 FSRegs:$rs2))))]>;
  def nr : ALU_RR<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,$rs2"),
	[(set (f32 FSRegs:$rd), (OpNode (fneg (f32 FSRegs:$rs1)),
				        (f32 FSRegs:$rs2)))]>;
  def nn : ALU_RR<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,-$rs2"),
	[(set (f32 FSRegs:$rd), (OpNode (fneg (f32 FSRegs:$rs1)),
					(fneg (f32 FSRegs:$rs2))))]>;
  def rk : ALU_RW<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, fsimmi5:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), fsimmi5:$imm))]>;
  def rf : ALU_RW<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), fpimm32:$imm))]>;
  def jr : ALU_RW<OpC, Flags,
	(outs FSRegs:$rd), (ins fsimmpi5:$imm, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set FSRegs:$rd, (OpNode fsimmpi5:$imm, (f32 FSRegs:$rs2)))]>;
  def fr : ALU_RW<OpC, Flags,
	(outs FSRegs:$rd), (ins fpimm32:$imm, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set FSRegs:$rd, (OpNode fpimm32:$imm, (f32 FSRegs:$rs2)))]>;
}
defm FADDF : FSrr<"faddf", 0b110001, 0b00010, fadd>;
defm FMULF : FSrr<"fmulf", 0b110010, 0b00010, fmul>;
defm FDIVF : FSrr<"fdivf", 0b110011, 0b00010, fdiv>;
defm FMAXNF: FSrr<"fmaxf", 0b110110, 0b00010, fmaxnum>;
defm FMINNF: FSrr<"fminf", 0b110111, 0b00010, fminnum>;
defm FMAXF : FSrr<"fmaxf", 0b110110, 0b00010, fmaximum>;
defm FMINF : FSrr<"fminf", 0b110111, 0b00010, fminimum>;
defm CSGNF : FSrr<"csgnf", 0b111001, 0b00010, fcopysign>;
defm FPOWF : FSrr<"fpowf", 0b111101, 0b00000, fpow>;

let isCompare = 1 in {
def FCMPFrr : ALU_RR<0b110100, 0b00010,
	(outs GRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	"fcmpf\t$rd,$rs1,$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1), (f32 FSRegs:$rs2)))]>;
def FCMPFrn : ALU_RR<0b110100, 0b00010,
	(outs GRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	"fcmpf\t$rd,$rs1,-$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1),
				       (fneg (f32 FSRegs:$rs2))))]>;
def FCMPFnr : ALU_RR<0b110100, 0b00010,
	(outs GRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	"fcmpf\t$rd,-$rs1,$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (fneg (f32 FSRegs:$rs1)),
				       (f32 FSRegs:$rs2)))]>;
def FCMPFnn : ALU_RR<0b110100, 0b00010,
	(outs GRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	"fcmpf\t$rd,-$rs1,-$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (fneg (f32 FSRegs:$rs1)),
				       (fneg (f32 FSRegs:$rs2))))]>;
def FCMPFrk : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins FSRegs:$rs1, fsimmi5:$imm),
	"fcmpf\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1), fsimmi5:$imm))]>;
def FCMPFrs : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm),
	"fcmpf\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1), fpimm32:$imm))]>;
}

def FSUBF : Pat<(fsub (f32 FSRegs:$rs1), (f32 FSRegs:$rs2)),
		(FADDFrn FSRegs:$rs1, FSRegs:$rs2)>;
def NFADDF: Pat<(fsub (fneg (f32 FSRegs:$rs1)), (f32 FSRegs:$rs2)),
		(FADDFnn FSRegs:$rs1, FSRegs:$rs2)>;

def FABSF : ABS<0b000101, 0b00001, 0b00100,
	(outs FSRegs:$rd), (ins FSRegs:$rs1),
	"fabsf\t$rd,$rs1",
	[(set FSRegs:$rd, (fabs (f32 FSRegs:$rs1)))]>;
def FNEGF : ALU_DR<0b110001, 0b10111,
	(outs FSRegs:$rd), (ins FSRegs:$rs2),
	"faddf\t$rd,#0,-$rs2",
	[(set FSRegs:$rd, (fneg (f32 FSRegs:$rs2)))]>;

// Non-commutative ops with constants on the left
def FSUBFkr : ALU_DR<0b110001, 0b10101,
	(outs FSRegs:$rd), (ins fsimmpi5:$imm, FSRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set FSRegs:$rd, (fsub fsimmpi5:$imm, (f32 FSRegs:$rs2)))]>;
def FSUBFfr : ALU_DR<0b110001, 0b10101,
	(outs FSRegs:$rd), (ins fpimm32:$imm, FSRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set FSRegs:$rd, (fsub fpimm32:$imm, (f32 FSRegs:$rs2)))]>;

multiclass FMAFrrr<bits<3> OpC, bits<3>Flags, SDPatternOperator OpNode> {
  def rr : ALU_RRR<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,$rs2,$rs3",
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), (f32 FSRegs:$rs2),
				  (f32 FSRegs:$rs3)))]>;
  def rn : ALU_RRR<0b000, 0b001,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,$rs2,-$rs3",
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), (f32 FSRegs:$rs2),
				   (fneg (f32 FSRegs:$rs3))))]>;
  def nr : ALU_RRR<0b000, 0b010,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs2,-$rs1,$rs3",	// must swap rs1 and rs2
	[(set FSRegs:$rd, (OpNode (fneg FSRegs:$rs1), (f32 FSRegs:$rs2),
				  (f32 FSRegs:$rs3)))]>;
  def nn : ALU_RRR<0b000, 0b010,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs2,-$rs1,-$rs3",	// must swap rs1 and rs2
	[(set FSRegs:$rd, (OpNode (fneg FSRegs:$rs1), (f32 FSRegs:$rs2),
				  (fneg (f32 FSRegs:$rs3))))]>;
  def rf : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, fpimm32:$imm),
	"fmacf\t$rd,$rs1,$rs2,#$imm",
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), (f32 FSRegs:$rs2),
				   fpimm32:$imm))]>;
  def fr : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,#$imm,$rs3",
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), fpimm32:$imm,
				  (f32 FSRegs:$rs3)))]>;
}
// FMA 32-bit FMA - sometimes we get fma and sometime fmad
defm FMAF  : FMAFrrr<0b100, 0b000, fma>;
defm FMADF : FMAFrrr<0b100, 0b000, fmad>;

// Matched in DAGToDAG
def EADDFrr : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, GRegs:$rs2),
	"eaddf\t$rd,$rs1,$rs2",
	[(set FSRegs:$rd, (fldexp (f32 FSRegs:$rs1), GRegs:$rs2))]>;
def EADDFrk : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, simm5:$imm),
	"eaddf\t$rd,$rs1,#$imm",
	[]>;
def EADDFrd : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, i64imm:$imm),
	"eaddf\t$rd,$rs1,#$imm",
	[]>;

def FREXPFr : Pseudo<
	(outs FSRegs:$rf, GRegs:$re), (ins FSRegs:$rs),
	"# FREXPFr",
	[(set FSRegs:$rf, GRegs:$re, (ffrexp (f32 FSRegs:$rs)))]>;

def EXPONFr : R1<0b000110, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs),
	"exponf\t$rd,$rs",
	[]>;
def FRACTFr : R1<0b000111, 0b00000,
	(outs FSRegs:$rd), (ins FSRegs:$rs),
	"fractf\t$rd,$rs",
	[]>;

// Moves
let isAsCheapAsAMove = 1 in {
def FMOVFrz : ALU_RRR<0b000, 0b111,
	(outs FSRegs:$rd), (ins fpimm32:$imm),
	"mov\t$rd,#0",
	[(set (f32 FSRegs:$rd), fpimm0:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
def FMOVFrk : ALU_RRR<0b000, 0b111,
	(outs FSRegs:$rd), (ins fsimmi5:$imm),
	"cvtsf\t$rd,#$imm",
	[(set (f32 FSRegs:$rd), fsimmi5:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
def FMOVFrf : ALU_RRR<0b000, 0b111,
	(outs FSRegs:$rd), (ins fpimm32:$imm),
	"mov\t$rd,#$imm",
	[(set (f32 FSRegs:$rd), fpimm32:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
}
/* tried a Pat<> for this, but no joy */
def CMOVSrrr:   ALU_RRR<0b001, 0b001,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, GRegs:$rs3),
	"cmov\t$rd,$rs3,$rs1,$rs2",
	[(set (f32 FSRegs:$rd), (myCMOV (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), GRegs:$rs3))]>;
def MUXFSrrr :  ALU_RRR<0b001, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, GRegs:$rs3),
	"mux\t$rd,$rs3,$rs1,$rs2",
	[(set (f32 FSRegs:$rd), (myMUX FSRegs:$rs1, FSRegs:$rs2, GRegs:$rs3))]>;
def MUXFSrrw :  ALU_RRR<0b001, 0b100,
	(outs FSRegs:$rd), (ins fpimm32:$imm, FSRegs:$rs1, GRegs:$rs3),
	"mux\t$rd,$rs3,$rs1,#$imm",
	[(set (f32 FSRegs:$rd), (myMUX fpimm32:$imm, FSRegs:$rs1, GRegs:$rs3))]>;

// Converts
def CVTfdr : CVT<0b010001, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfd\t$rd,$rs1",
	[(set (f64 GRegs:$rd), (fpextend (f32 FSRegs:$rs1)))]>;
def CVTfdn : CVT<0b010001, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfd\t$rd,-$rs1",
	[(set (f64 GRegs:$rd), (fpextend (fneg (f32 FSRegs:$rs1))))]>;
def CVTdfr : CVT<0b110001, 0b00000,
	(outs FSRegs:$rd), (ins GRegs:$rs1),
	"cvtdf\t$rd,$rs1",
	[(set (f32 FSRegs:$rd), (fpround (f64 GRegs:$rs1)))]>;
def CVTdfn : CVT<0b110001, 0b00000,
	(outs FSRegs:$rd), (ins GRegs:$rs1),
	"cvtdf\t$rd,-$rs1",
	[(set (f32 FSRegs:$rd), (fneg (fpround (f64 GRegs:$rs1))))]>;
def CVTuf : CVT<0b110000, 0b00000,
	(outs FSRegs:$rd), (ins GRegs:$rs1),
	"cvtuf\t$rd,$rs1",
	[(set (f32 FSRegs:$rd), (uint_to_fp GRegs:$rs1))]>;
def CVTsf : CVT<0b111000, 0b00000,
	(outs FSRegs:$rd), (ins GRegs:$rs1),
	"cvtsf\t$rd,$rs1",
	[(set (f32 FSRegs:$rd), (sint_to_fp GRegs:$rs1))]>;
def CVTfu : CVT<0b001000, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfu\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_uint (f32 FSRegs:$rs1)))]>;
def CVTfs : CVT<0b101000, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (f32 FSRegs:$rs1)))]>;
def CVTfsni : CVT<0b101111, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#7",
	[(set (i64 GRegs:$rd), (fp_to_sint (ffloor (f32 FSRegs:$rs1))))]>;
def CVTfspi : CVT<0b101110, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#6",
	[(set (i64 GRegs:$rd), (fp_to_sint (fceil (f32 FSRegs:$rs1))))]>;
def CVTfstz : CVT<0b101101, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#5",
	[(set (i64 GRegs:$rd), (fp_to_sint (ftrunc (f32 FSRegs:$rs1))))]>;
def CVTfsaz : CVT<0b101100, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#4",
	[(set (i64 GRegs:$rd), (fp_to_sint (fround (f32 FSRegs:$rs1))))]>;
def CVTfsrm : CVT<0b101100, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfs\t$rd,$rs1,#2",
	[(set (i64 GRegs:$rd), (fp_to_sint (fnearbyint (f32 FSRegs:$rs1))))]>;

// Rounds
multiclass RNDFrr<string mode, SDPatternOperator OpNode> {
    def rr : RND<0b10010, 0b00100,
	(outs FSRegs:$rd), (ins FSRegs:$rs1),
	!strconcat("rndf\t$rd,$rs1,#", mode),
	[(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1)))]>;
}
defm RNDRMF: RNDFrr<"2", fnearbyint>;
defm RNDAZF: RNDFrr<"4", fround>;
defm RNDTZF: RNDFrr<"5", ftrunc>;
defm RNDPIF: RNDFrr<"6", fceil>;
defm RNDNIF: RNDFrr<"7", ffloor>;
def RNDNEF: RND<0b10010, 0b00100,
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
multiclass FSTRAN<string OpName,  bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def x : TRAN<OpC, Flags,
	    (outs FSRegs:$rd), (ins FSRegs:$rs1),
	    !strconcat(OpName, "\t$rd,$rs1"),
	    [(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1)))]>;
}
// transcendentals not defined in LLVM
multiclass USTRAN<string OpName,  bits<6>OpC, bits<5>Flags> {
  def u : TRAN<OpC, Flags,
	    (outs FSRegs:$rd), (ins FSRegs:$rs1),
	    !strconcat(OpName, "\t$rd,$rs1"),
	    []>;
}
defm SQRTF :  FSTRAN<"fsqrtf", 0b001101, 0b00010, fsqrt>;
defm FLOG2F : FSTRAN<"fln2f",  0b000000, 0b00010, flog2>;
defm FLOGF :  FSTRAN<"flnf",   0b000001, 0b00010, flog>;
defm FLOG10F: FSTRAN<"flogf",  0b000010, 0b00010, flog10>;
defm FEXP2F : FSTRAN<"fexp2f", 0b000100, 0b00010, fexp2>;
defm FEXPF :  FSTRAN<"fexpf",  0b000101, 0b00010, fexp>;
defm FSINF :  FSTRAN<"fsinf",  0b011000, 0b00010, fsin>;
defm FCOSF :  FSTRAN<"fcosf",  0b011001, 0b00010, fcos>;
defm FTANF :  USTRAN<"ftanf",  0b011010, 0b00010>;
defm RSQRTF : USTRAN<"frsqrtf",0b001100, 0b00010>;
defm FEXP10F: USTRAN<"fexp10f",0b000110, 0b00010>;
defm FASINF : USTRAN<"fasinf", 0b011100, 0b00010>;
defm FACOSF : USTRAN<"facosf", 0b011101, 0b00010>;
defm FATANF : USTRAN<"fatan",  0b011110, 0b00010>;

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
multiclass LoadFS<string OpName, bits<6>OpCri, bits<6>OpCrr,
		SDPatternOperator OpNode> {
  def ri : LDST_BD<OpCri,
	(outs FSRegs:$rd), (ins MEMri:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(set (f32 FSRegs:$rd), (OpNode ADDRri:$addr))]>;
  def rr : LDST_RX<OpCrr, 0b00000,
	(outs FSRegs:$rd), (ins MEMrr:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(set (f32 FSRegs:$rd), (OpNode ADDRrr:$addr))]>;
}
multiclass StoreFS<string OpName, bits<6>OpCri, bits<6>OpCrr,
		 SDPatternOperator OpNode> {
  def ri : LDST_BD<OpCri,
	(outs), (ins FSRegs:$rd, MEMri:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(OpNode (f32 FSRegs:$rd), ADDRri:$addr)]>;
  def rr : LDST_RX<OpCrr, 0b00000,
	(outs), (ins FSRegs:$rd, MEMrr:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(OpNode (f32 FSRegs:$rd), ADDRrr:$addr)]>;
}
let mayLoad = 1 in {
  defm LDF : LoadFS<"lduw", 0b100010, 0b000010, load>;
}

let mayStore = 1 in {
  defm STF : StoreFS<"stw", 0b101010, 0b001010, store>;
}
