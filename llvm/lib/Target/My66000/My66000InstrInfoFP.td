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
def fpimm64 : Operand<f64>, FPImmLeaf<f64, [{ return true; }]>
		{ let PrintMethod = "printFP64Operand"; }
def fpimm32 : Operand<f32>, FPImmLeaf<f32, [{ return true; }]>
		{ let PrintMethod = "printFP32Operand"; }
def fpimm1 : PatLeaf<(fpimm), [{ return N->isExactlyValue(+1.0); }]>;

//
// My66000 specific FP DAG Nodes.
//
def myFCMP : SDNode<"My66000ISD::FCMP",
			SDTypeProfile<1, 2, [SDTCisVT<0, i64>,
			    SDTCisSameAs<1, 2>]>,
			[]>;
def myBRfcc :	    SDNode<"My66000ISD::BRfcc", SDT_BRcc,
			[SDNPHasChain]>;
def mySHRUNK:	    SDNode<"My66000ISD::SHRUNK",
			SDTypeProfile<1, 1,
			  [SDTCisVT<0, f64>, SDTCisVT<1, f32>]>,
			[]>;
def myF64I5:	    SDNode<"My66000ISD::F64I5",
			SDTypeProfile<1, 1,
			  [SDTCisVT<0, f64>, SDTCisVT<1, i64>]>,
			[]>;
def myF32I5:	    SDNode<"My66000ISD::F32I5",
			SDTypeProfile<1, 1,
			  [SDTCisVT<0, f32>, SDTCisVT<1, i64>]>,
			[]>;

//
// 64-bit floating point shares the GRegs with integers
// It can use many of the multiclass pattersn with integers
//

defm FADD :  ALUrr<"fadd",  0b110001, 0b00000, f64, fadd>;
defm FMUL :  ALUrr<"fmul",  0b110010, 0b00000, f64, fmul>;
defm FDIV :  ALUrr<"fdiv",  0b110011, 0b00000, f64, fdiv>;
// Caution: the following semantics change with changes to IEEE-754
defm FMAXN:  ALUrr<"fmax",  0b110110, 0b00000, f64, fmaxnum>;
defm FMINN:  ALUrr<"fmin",  0b110111, 0b00000, f64, fminnum>;
defm FMAX :  ALUrr<"fmax",  0b110110, 0b00000, f64, fmaximum>;
defm FMIN :  ALUrr<"fmin",  0b110111, 0b00000, f64, fminimum>;
defm CSGN :  ALUrr<"csgn",  0b111001, 0b00000, f64, fcopysign>;
defm FPOW :  ALUrr<"fpow",  0b111101, 0b00000, f64, fpow>;

// Can't use the standard pattern because the output type doesn't match inputs.
let isCompare = 1 in {
def FCMPrr : ALU_RR<0b110100, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	"fcmp\t$rd,$rs1,$rs2",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), (f64 GRegs:$rs2)))]>;
}

def FSUB  : Pat<(fsub (f64 GRegs:$rs1), (f64 GRegs:$rs2)),
		(FADDrn GRegs:$rs1, GRegs:$rs2)>;
def NFADD : Pat<(fsub (fneg (f64 GRegs:$rs1)), (f64 GRegs:$rs2)),
		(FADDnn GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(fmul (f64 GRegs:$rs1), (fneg (f64 GRegs:$rs2))),
		(FMULrn GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(fdiv (f64 GRegs:$rs1), (fneg (f64 GRegs:$rs2))),
		(FDIVrn GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(fdiv (fneg (f64 GRegs:$rs1)), (f64 GRegs:$rs2)),
		(FDIVnr GRegs:$rs1, GRegs:$rs2)>;

def FABS : ABS<0b000101, 0b00000, 0b00100,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"fabs\t$rd,$rs1",
	[(set GRegs:$rd, (fabs (f64 GRegs:$rs1)))]>;

multiclass FPUrd<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def rd : ALU_RD<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), fpimm64:$imm))]>;
  def rf : ALU_RD<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm32:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (mySHRUNK (fpimm32:$imm))))]>;
  def rk : ALU_RD<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (f64 GRegs:$rs1), (myF64I5 (simm5:$imm))))]>;
}
defm FADD :  FPUrd<"fadd",  0b110001, 0b10001, fadd>;
defm FMUL :  FPUrd<"fmul",  0b110010, 0b10001, fmul>;
defm FDIV :  FPUrd<"fdiv",  0b110011, 0b10001, fdiv>;

let isCompare = 1 in {
def FCMPrd : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm),
	"fcmp\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), fpimm64:$imm))]>;
def FCMPrf : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm32:$imm),
	"fcmp\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), (mySHRUNK (fpimm32:$imm))))]>;
def FCMPrk : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"fcmp\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f64 GRegs:$rs1), (myF64I5 (simm5:$imm))))]>;
}

// Non-commutative ops with constants on the left
def FDIVdr : ALU_DR<0b110011, 0b10101,
	(outs GRegs:$rd), (ins fpimm64:$imm, GRegs:$rs2),
	"fdiv\t$rd,#$imm,$rs2",
	[(set GRegs:$rd, (fdiv fpimm64:$imm, (f64 GRegs:$rs2)))]>;
def FDIVfr : ALU_DR<0b110011, 0b10101,
	(outs GRegs:$rd), (ins fpimm32:$imm, GRegs:$rs2),
	"fdiv\t$rd,#$imm,$rs2",
	[(set GRegs:$rd, (fdiv (mySHRUNK (fpimm32:$imm)), (f64 GRegs:$rs2)))]>;
def FDIVkr : ALU_DR<0b110011, 0b10101,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	"fdiv\t$rd,#$imm,$rs2",
	[(set GRegs:$rd, (fdiv (myF64I5 (simm5:$imm)), (f64 GRegs:$rs2)))]>;
def FSUBdr : ALU_DR<0b110001, 0b10101,
	(outs GRegs:$rd), (ins fpimm64:$imm, GRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (fsub fpimm64:$imm, (f64 GRegs:$rs2)))]>;
def FSUBfr : ALU_DR<0b110001, 0b10101,
	(outs GRegs:$rd), (ins fpimm32:$imm, GRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (fsub (mySHRUNK (fpimm32:$imm)), (f64 GRegs:$rs2)))]>;
def FSUBkr : ALU_DR<0b110001, 0b10101,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	"fadd\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (fsub (myF64I5 (simm5:$imm)), (f64 GRegs:$rs2)))]>;
def FNEG : ALU_DR<0b110001, 0b10101,
	(outs GRegs:$rd), (ins GRegs:$rs2),
	"fadd\t$rd,#0,-$rs2",
	[(set GRegs:$rd, (fneg (f64 GRegs:$rs2)))]>;

// FMA 64-bit FMA - sometimes we get fma and sometime fmad
def FMArr : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs1,$rs2,$rs3",
	[(set GRegs:$rd, (fma (f64 GRegs:$rs1), (f64 GRegs:$rs2), (f64 GRegs:$rs3)))]>;
def FMArn : ALU_RRR<0b000, 0b001,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs1,$rs2,-$rs3",
	[(set GRegs:$rd, (fma (f64 GRegs:$rs1), (f64 GRegs:$rs2), (fneg (f64 GRegs:$rs3))))]>;
def FMAnr : ALU_RRR<0b000, 0b010,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs2,-$rs1,$rs3",	// must swap rs1 and rs2
	[(set GRegs:$rd, (fma (fneg GRegs:$rs1), (f64 GRegs:$rs2), (f64 GRegs:$rs3)))]>;
def FMAnn : ALU_RRR<0b000, 0b010,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs2,-$rs1,-$rs3",	// must swap rs1 and rs2
	[(set GRegs:$rd, (fma (fneg GRegs:$rs1), (f64 GRegs:$rs2), (fneg (f64 GRegs:$rs3))))]>;
def FMArd : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, fpimm64:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (fma (f64 GRegs:$rs1), (f64 GRegs:$rs2), fpimm64:$imm))]>;
def FMArf : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, fpimm32:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (fma (f64 GRegs:$rs1), (f64 GRegs:$rs2), (mySHRUNK (fpimm32:$imm))))]>;
def FMArk : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, i64imm:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (fma (f64 GRegs:$rs1), (f64 GRegs:$rs2), (myF64I5 (simm5:$imm))))]>;
def FMAdr : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (fma (f64 GRegs:$rs1), fpimm64:$imm, (f64 GRegs:$rs3)))]>;
def FMAfr : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm32:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (fma (f64 GRegs:$rs1), (mySHRUNK (fpimm32:$imm)), (f64 GRegs:$rs3)))]>;
def FMAkr : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (fma (f64 GRegs:$rs1), (myF64I5 (simm5:$imm)), (f64 GRegs:$rs3)))]>;

// FMA 64-bit FMAD - preferred by DAGCombiner?
def FMADrr : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs1,$rs2,$rs3",
	[(set GRegs:$rd, (fmad (f64 GRegs:$rs1), (f64 GRegs:$rs2), (f64 GRegs:$rs3)))]>;
def FMADrn : ALU_RRR<0b000, 0b001,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs1,$rs2,-$rs3",
	[(set GRegs:$rd, (fmad (f64 GRegs:$rs1), (f64 GRegs:$rs2), (fneg (f64 GRegs:$rs3))))]>;
def FMADnr : ALU_RRR<0b000, 0b010,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs2,-$rs1,$rs3",	// must swap rs1 and rs2
	[(set GRegs:$rd, (fmad (fneg GRegs:$rs1), (f64 GRegs:$rs2), (f64 GRegs:$rs3)))]>;
def FMADnn : ALU_RRR<0b000, 0b010,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"fmac\t$rd,$rs2,-$rs1,-$rs3",	// must swap rs1 and rs2
	[(set GRegs:$rd, (fmad (fneg GRegs:$rs1), (f64 GRegs:$rs2), (fneg (f64 GRegs:$rs3))))]>;
def FMADrd : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, fpimm64:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (fmad (f64 GRegs:$rs1), (f64 GRegs:$rs2), fpimm64:$imm))]>;
def FMADrf : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, fpimm32:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (fmad (f64 GRegs:$rs1), (f64 GRegs:$rs2), (mySHRUNK (fpimm32:$imm))))]>;
def FMADrk : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, i64imm:$imm),
	"fmac\t$rd,$rs1,$rs2,#$imm",
	[(set GRegs:$rd, (fmad (f64 GRegs:$rs1), (f64 GRegs:$rs2), (myF64I5 (simm5:$imm))))]>;
def FMADdr : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm64:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (fmad (f64 GRegs:$rs1), fpimm64:$imm, (f64 GRegs:$rs3)))]>;
def FMADfr : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, fpimm32:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (fmad (f64 GRegs:$rs1), (mySHRUNK (fpimm32:$imm)), (f64 GRegs:$rs3)))]>;
def FMADkr : ALU_RRR<0b000, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm, GRegs:$rs3),
	"fmac\t$rd,$rs1,#$imm,$rs3",
	[(set GRegs:$rd, (fmad (f64 GRegs:$rs1), (myF64I5 (simm5:$imm)), (f64 GRegs:$rs3)))]>;

// Moves
let isAsCheapAsAMove = 1 in {
def FMOVrd : ALU_RRR<0b000, 0b111,
	(outs GRegs:$rd), (ins fpimm64:$imm),
	"mov\t$rd,#$imm",
	[(set (f64 GRegs:$rd), fpimm64:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
def FMOVrf : ALU_RRR<0b000, 0b111,
	(outs GRegs:$rd), (ins fpimm32:$imm),
	"cvtfd\t$rd,#$imm",
	[(set (f64 GRegs:$rd), (mySHRUNK (fpimm32:$imm)))]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
def FMOVrk : ALU_RRR<0b000, 0b111,
	(outs GRegs:$rd), (ins i64imm:$imm),
	"cvtsd\t$rd,#$imm",
	[(set (f64 GRegs:$rd), (myF64I5 (simm5:$imm)))]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
}
def : Pat<(myCMOV (f64 GRegs:$rs1), (f64 GRegs:$rs2), GRegs:$rs3),
          (CMOVrrr GRegs:$rs1, GRegs:$rs2, GRegs:$rs3)>;
def : Pat<(myMUX (f64 GRegs:$rs1), (f64 GRegs:$rs2), GRegs:$rs3),
          (MUXrrr GRegs:$rs1, GRegs:$rs2, GRegs:$rs3)>;
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
	"cvtdsni\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (ffloor (f64 GRegs:$rs1))))]>;
def CVTdspi : CVT<0b100110, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtdspi\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (fceil (f64 GRegs:$rs1))))]>;
def CVTdstz : CVT<0b100101, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtdstz\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (ftrunc (f64 GRegs:$rs1))))]>;
def CVTdsaz : CVT<0b100100, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cvtdsaz\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (fround (f64 GRegs:$rs1))))]>;

// Rounds
multiclass RNDrr<string OpName, bits<5>OpC, bits<5>OpF,
	    SDPatternOperator OpNode> {
    def rr : RND<OpC, OpF,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	!strconcat(OpName, "\t$rd,$rs1"),
	[(set (f64 GRegs:$rd), (OpNode (f64 GRegs:$rs1)))]>;
}
defm RNDRM : RNDrr<"rndrm", 0b00001, 0b00000, fnearbyint>;
defm RNDAZ : RNDrr<"rndaz", 0b00100, 0b00000, fround>;
defm RNDTZ : RNDrr<"rndtz", 0b00101, 0b00000, ftrunc>;
defm RNDPI : RNDrr<"rndpi", 0b00110, 0b00000, fceil>;
defm RNDNI : RNDrr<"rndni", 0b00111, 0b00000, ffloor>;
def RNDNE : RND<0b00000, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"rndne\t$rd,$rs1",
	[]>;

// Patterns for rounding and conversion
def AssertSext : SDNode<"ISD::AssertSext", SDTUnaryOp>;
def : Pat<(sint_to_fp (i64 (fp_to_sint (f64 GRegs:$Rn)))),
	  (RNDTZrr (f64 GRegs:$Rn))>;
def : Pat<(sint_to_fp (i64 (AssertSext (fp_to_sint (f64 GRegs:$Rn))))),
	  (RNDTZrr (f64 GRegs:$Rn))>;
def : Pat<(fpextend (f32 (sint_to_fp GRegs:$Rn))),
	  (CVTsd GRegs:$Rn)>;

// Transcendentals
//
// For some reason some transcendentals are not pre-defined
def fexp :  SDNode<"ISD::FEXP", SDTFPUnaryOp>;
def flog :  SDNode<"ISD::FLOG", SDTFPUnaryOp>;
def flog10: SDNode<"ISD::FLOG10", SDTFPUnaryOp>;

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
def : Pat<(fdiv (myF64I5 imm1), (fsqrt (f64 GRegs:$bot))),
	(RSQRTu (f64 GRegs:$bot))>;
def : Pat<(fdiv fpimm1, (fsqrt (f64 GRegs:$bot))),
	(RSQRTu (f64 GRegs:$bot))>;
def : Pat<(fdiv (myF64I5 (simm5:$top)), (fsqrt (f64 GRegs:$bot))),
	(FMULrk (RSQRTu (f64 GRegs:$bot)), simm5:$top)>;
def : Pat<(fdiv (mySHRUNK (fpimm32:$top)), (fsqrt (f64 GRegs:$bot))),
	(FMULrf (RSQRTu (f64 GRegs:$bot)),fpimm32:$top)>;
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

//
// 32-bit floating point uses different virtual registers FSRegs
// (The same physical registers are used)
// Thus we need to have many replicated patterns
//
multiclass FSrr<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def rr : ALU_RR<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1), (f32 FSRegs:$rs2)))]>;
  def rn : ALU_RR<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,-$rs2"),
	[]>;
  def nr : ALU_RR<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,$rs2"),
	[]>;
  def nn : ALU_RR<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,-$rs2"),
	[]>;
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
def FCMPFrk : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins FSRegs:$rs1, i64imm:$imm),
	"fcmpf\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1), (myF32I5 (simm5:$imm))))]>;
}

def FSUBF : Pat<(fsub (f32 FSRegs:$rs1), (f32 FSRegs:$rs2)),
		(FADDFrn FSRegs:$rs1, FSRegs:$rs2)>;
def NFADDF: Pat<(fsub (fneg (f32 FSRegs:$rs1)), (f32 FSRegs:$rs2)),
		(FADDFnn FSRegs:$rs1, FSRegs:$rs2)>;
def       : Pat<(fmul (f32 FSRegs:$rs1), (fneg (f32 FSRegs:$rs2))),
		(FMULFrn FSRegs:$rs1, FSRegs:$rs2)>;
def       : Pat<(fdiv (f32 FSRegs:$rs1), (fneg (f32 FSRegs:$rs2))),
		(FDIVFrn FSRegs:$rs1, FSRegs:$rs2)>;
def       : Pat<(fdiv (fneg (f32 FSRegs:$rs1)), (f32 FSRegs:$rs2)),
		(FDIVFnr FSRegs:$rs1, FSRegs:$rs2)>;
def FABSF : ABS<0b000101, 0b00001, 0b00100,
	(outs FSRegs:$rd), (ins FSRegs:$rs1),
	"fabsf\t$rd,$rs1",
	[(set FSRegs:$rd, (fabs (f32 FSRegs:$rs1)))]>;


multiclass FPUrs<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def rs : ALU_RW<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), fpimm32:$imm))]>;
  def rk : ALU_RW<OpC, Flags,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set FSRegs:$rd, (OpNode (f32 FSRegs:$rs1), (myF32I5 (simm5:$imm))))]>;
}
defm FADDF : FPUrs<"faddf", 0b110001, 0b10010, fadd>;
defm FMULF : FPUrs<"fmulf", 0b110010, 0b10010, fmul>;
defm FDIVF : FPUrs<"fdivf", 0b110011, 0b10010, fdiv>;

let isCompare = 1 in {
def FCMPFrs : ALU_RD<0b110100, 0b10001,
	(outs GRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm),
	"fcmpf\t$rd,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myFCMP (f32 FSRegs:$rs1), fpimm32:$imm))]>;
}

def FSUBFsr : ALU_DR<0b110001, 0b10101,
	(outs FSRegs:$rd), (ins fpimm32:$imm, FSRegs:$rs2),
	"faddf\t$rd,#$imm,-$rs2",
	[(set FSRegs:$rd, (fsub fpimm32:$imm, (f32 FSRegs:$rs2)))]>;
def FSUBFkr : ALU_DR<0b110001, 0b10101,
	(outs FSRegs:$rd), (ins i64imm:$imm, FSRegs:$rs2),
	"faddf\t$rd,#$imm,-$rs2",
	[(set FSRegs:$rd, (fsub (myF32I5 (simm5:$imm)), (f32 FSRegs:$rs2)))]>;
def FDIVFsr: ALU_WR<0b110011, 0b10010,
	(outs FSRegs:$rd), (ins fpimm32:$imm, FSRegs:$rs1),
	"fdivf\t$rd,#$imm,$rs1",
	[(set FSRegs:$rd, (fdiv fpimm32:$imm, (f32 FSRegs:$rs1)))]>;
def FDIVFkr : ALU_DR<0b110011, 0b10111,
	(outs FSRegs:$rd), (ins i64imm:$imm, FSRegs:$rs2),
	"fdivf\t$rd,#$imm,$rs2",
	[(set FSRegs:$rd, (fdiv (myF32I5 (simm5:$imm)), (f32 FSRegs:$rs2)))]>;
def FNEGF : ALU_DR<0b110001, 0b10111,
	(outs FSRegs:$rd), (ins FSRegs:$rs2),
	"faddf\t$rd,#0,-$rs2",
	[(set FSRegs:$rd, (fneg (f32 FSRegs:$rs2)))]>;

// FMA 32-bit FMA - sometimes we get fma and somtimes fmad
def FMAFrr : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,$rs2,$rs3",
	[(set FSRegs:$rd, (any_fma (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), (f32 FSRegs:$rs3)))]>;
def FMAFrn : ALU_RRR<0b000, 0b001,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,$rs2,-$rs3",
	[(set FSRegs:$rd, (any_fma (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), (fneg (f32 FSRegs:$rs3))))]>;
def FMAFnr : ALU_RRR<0b000, 0b010,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs2,-$rs1,$rs3",	// must swap rs1 and rs2
	[(set FSRegs:$rd, (any_fma (fneg FSRegs:$rs1), (f32 FSRegs:$rs2), (f32 FSRegs:$rs3)))]>;
def FMAFnn : ALU_RRR<0b000, 0b010,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs2,-$rs1,-$rs3",	// must swap rs1 and rs2
	[(set FSRegs:$rd, (any_fma (fneg FSRegs:$rs1), (f32 FSRegs:$rs2), (fneg (f32 FSRegs:$rs3))))]>;
def FMAFrd : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, fpimm32:$imm),
	"fmacf\t$rd,$rs1,$rs2,#$imm",
	[(set FSRegs:$rd, (any_fma (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), fpimm32:$imm))]>;
def FMAFrk : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, i64imm:$imm),
	"fmacf\t$rd,$rs1,$rs2,#$imm",
	[(set FSRegs:$rd, (any_fma (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), (myF32I5 (simm5:$imm))))]>;
def FMAFdr : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,#$imm,$rs3",
	[(set FSRegs:$rd, (any_fma (f32 FSRegs:$rs1), fpimm32:$imm, (f32 FSRegs:$rs3)))]>;
def FMAFkr : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, i64imm:$imm, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,#$imm,$rs3",
	[(set FSRegs:$rd, (any_fma (f32 FSRegs:$rs1), (myF32I5 (simm5:$imm)), (f32 FSRegs:$rs3)))]>;

// FMA 32-bit FMAD - preferred by DAGCombiner
def FMADFrr : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,$rs2,$rs3",
	[(set FSRegs:$rd, (fmad (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), (f32 FSRegs:$rs3)))]>;
def FMADFrn : ALU_RRR<0b000, 0b001,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,$rs2,-$rs3",
	[(set FSRegs:$rd, (fmad (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), (fneg (f32 FSRegs:$rs3))))]>;
def FMADFnr : ALU_RRR<0b000, 0b010,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs2,-$rs1,$rs3",	// must swap rs1 and rs2
	[(set FSRegs:$rd, (fmad (fneg FSRegs:$rs1), (f32 FSRegs:$rs2), (f32 FSRegs:$rs3)))]>;
def FMADFnn : ALU_RRR<0b000, 0b010,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, FSRegs:$rs3),
	"fmacf\t$rd,$rs2,-$rs1,-$rs3",	// must swap rs1 and rs2
	[(set FSRegs:$rd, (fmad (fneg FSRegs:$rs1), (f32 FSRegs:$rs2), (fneg (f32 FSRegs:$rs3))))]>;
def FMADFrd : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, fpimm32:$imm),
	"fmacf\t$rd,$rs1,$rs2,#$imm",
	[(set FSRegs:$rd, (fmad (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), fpimm32:$imm))]>;
def FMADFrk : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, FSRegs:$rs2, i64imm:$imm),
	"fmacf\t$rd,$rs1,$rs2,#$imm",
	[(set FSRegs:$rd, (fmad (f32 FSRegs:$rs1), (f32 FSRegs:$rs2), (myF32I5 (simm5:$imm))))]>;
def FMADFdr : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, fpimm32:$imm, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,#$imm,$rs3",
	[(set FSRegs:$rd, (fmad (f32 FSRegs:$rs1), fpimm32:$imm, (f32 FSRegs:$rs3)))]>;
def FMADFkr : ALU_RRR<0b000, 0b000,
	(outs FSRegs:$rd), (ins FSRegs:$rs1, i64imm:$imm, FSRegs:$rs3),
	"fmacf\t$rd,$rs1,#$imm,$rs3",
	[(set FSRegs:$rd, (fmad (f32 FSRegs:$rs1), (myF32I5 (simm5:$imm)), (f32 FSRegs:$rs3)))]>;

// Moves
let isAsCheapAsAMove = 1 in {
def FMOVrs : ALU_RRR<0b000, 0b111,
	(outs FSRegs:$rd), (ins fpimm32:$imm),
	"mov\t$rd,#$imm",
	[(set (f32 FSRegs:$rd), fpimm32:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
def FMOVFrk : ALU_RRR<0b000, 0b111,
	(outs FSRegs:$rd), (ins i64imm:$imm),
	"cvtsf\t$rd,#$imm",
	[(set (f32 FSRegs:$rd), (myF32I5 (simm5:$imm)))]>
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
def CVTfd : CVT<0b010001, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfd\t$rd,$rs1",
	[(set (f64 GRegs:$rd), (fpextend (f32 FSRegs:$rs1)))]>;
def CVTdf : CVT<0b110001, 0b00000,
	(outs FSRegs:$rd), (ins GRegs:$rs1),
	"cvtdf\t$rd,$rs1",
	[(set (f32 FSRegs:$rd), (fpround (f64 GRegs:$rs1)))]>;
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
	"cvtfsni\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (ffloor (f32 FSRegs:$rs1))))]>;
def CVTfspi : CVT<0b101110, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfspi\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (fceil (f32 FSRegs:$rs1))))]>;
def CVTfstz : CVT<0b101101, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfstz\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (ftrunc (f32 FSRegs:$rs1))))]>;
def CVTfsaz : CVT<0b101100, 0b00000,
	(outs GRegs:$rd), (ins FSRegs:$rs1),
	"cvtfsaz\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (fp_to_sint (fround (f32 FSRegs:$rs1))))]>;

// Rounds
multiclass RNDFSrr<string OpName, bits<5>OpC, bits<5>OpF,
	    SDPatternOperator OpNode> {
    def rr : RND<OpC, OpF,
	(outs FSRegs:$rd), (ins FSRegs:$rs1),
	!strconcat(OpName, "\t$rd,$rs1"),
	[(set (f32 FSRegs:$rd), (OpNode (f32 FSRegs:$rs1)))]>;
}
defm RNDRMF: RNDFSrr<"rndrmf",0b00001, 0b00100, fnearbyint>;
defm RNDAZF: RNDFSrr<"rndazf",0b00100, 0b00100, fround>;
defm RNDTZF: RNDFSrr<"rndtzf",0b00101, 0b00100, ftrunc>;
defm RNDPIF: RNDFSrr<"rndpif",0b00110, 0b00100, fceil>;
defm RNDNIF: RNDFSrr<"rndnif",0b00111, 0b00100, ffloor>;
def RNDNEF: RND<0b00000, 0b00100,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"rndnef\t$rd,$rs1",
	[]>;

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
def : Pat<(fdiv (myF32I5 imm1), (fsqrt (f32 FSRegs:$bot))),
	(RSQRTFu (f32 FSRegs:$bot))>;
def : Pat<(fdiv fpimm1, (fsqrt (f32 FSRegs:$bot))),
	(RSQRTFu (f32 FSRegs:$bot))>;
def : Pat<(fdiv (myF32I5 (simm5:$top)), (fsqrt (f32 FSRegs:$bot))),
	(FMULFrk (RSQRTFu (f32 FSRegs:$bot)),(simm5:$top))>;
def : Pat<(fdiv fpimm32:$top, (fsqrt (f32 FSRegs:$bot))),
	(FMULFrs (RSQRTFu (f32 FSRegs:$bot)), fpimm32:$top)>;
def : Pat<(fdiv (f32 FSRegs:$top), (fsqrt (f32 FSRegs:$bot))),
	(FMULFrr (f32 FSRegs:$top), (RSQRTFu (f32 FSRegs:$bot)))>;

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
