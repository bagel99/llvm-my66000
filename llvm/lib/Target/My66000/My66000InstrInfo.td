//===- My66000InstrInfo.td - Instr Description for My66000 -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "My66000InstrFormats.td"

//
// Operand definitions for DAG matching
//
def imm1 : Operand<i64>, ImmLeaf<i64,
		[{ return Imm == 1; }]>;
def uimm8 : Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 255; }]>;
def kimm8 : Operand<i64>, ImmLeaf<i64,
		[{ return (Imm >= 0 && Imm <= 0x0F) ||
			  (Imm >= 0xF0 && Imm <= 0xFF); }]>;
def uimm16 : Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 65535; }]>;
def uimm32 : Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 4294967295; }]>;
def uneed32: Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 65536 && Imm <= 4294967295; }]>;
def uneed64: Operand<i64>, ImmLeaf<i64,
		[{ return (uint64_t)(Imm) >= 4294967296; }]>;
def simm16 : Operand<i64>, ImmLeaf<i64,
		[{ return isInt<16>(Imm); }]>;
def simm32 : Operand<i64>, ImmLeaf<i64,
		[{ return isInt<32>(Imm); }]>;
// FIXME - if 16-versions are found first, can we relax the range?
def sneed64: Operand<i64>, ImmLeaf<i64,
		[{ return (Imm < -2147483648 || Imm > 2147483647); }]>;

def simm5 :  Operand<i64>, ImmLeaf<i64,
               [{ return isInt<6>(Imm); }]>;
def simmp5 :  Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 31; }]>;
def simmn5 :  Operand<i64>, ImmLeaf<i64,
		[{ return iImm < 0 && Imm >= -31; }]>;
def shfamt : Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 63; }]>;
def shfinx: Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 3; }]>;
def gbladr : Operand<iPTR>;
def brtarget : Operand<OtherVT>;
def calltarget : Operand<iPTR>;
def ccode : Operand<i64>;

// Operand definitions for machine instructions
def s16imm64 : Operand<i64> {
  let PrintMethod = "printS16ImmOperand";
}
def s32imm64 : Operand<i64> {
  let PrintMethod = "printS32ImmOperand";
}


//
// Type profiles
//
def SDT_BRcond :	SDTypeProfile<0, 3, [SDTCisVT<0, OtherVT>,
					     SDTCisVT<1, i64>,
					     SDTCisVT<2, i64>]>;
def SDT_BRcc :		SDTypeProfile<0, 3, [SDTCisVT<0, OtherVT>,
					     SDTCisVT<1, i64>,
					     SDTCisVT<2, i64>]>;
def SDT_CALL : 		SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SDT_BR_JT :		SDTypeProfile<0, 4, [SDTCisVT<0, i64>,
					     SDTCisVT<1, i64>,
					     SDTCisVT<2, i64>,
					     SDTCisVT<3, OtherVT>]>;

//
// My66000 specific DAG Nodes.
//

def callseq_start : SDNode<"ISD::CALLSEQ_START",
			SDCallSeqStart<[ SDTCisVT<0, i64>,
                                         SDTCisVT<1, i64> ]>,
			[SDNPHasChain, SDNPOutGlue]>;
def callseq_end :   SDNode<"ISD::CALLSEQ_END",
			SDCallSeqEnd<[ SDTCisVT<0, i64>,
                                       SDTCisVT<1, i64> ]>,
			[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def myRET :	    SDNode<"My66000ISD::RET", SDTNone,
			[SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def myBRcond :	    SDNode<"My66000ISD::BRcond", SDT_BRcond,
			[SDNPHasChain]>;
def myBRcc :	    SDNode<"My66000ISD::BRcc", SDT_BRcc,
			[SDNPHasChain]>;
def myBRbit :	    SDNode<"My66000ISD::BRbit", SDT_BRcc,
			[SDNPHasChain]>;
def myJT8 :	    SDNode<"My66000ISD::JT8", SDT_BR_JT, [SDNPHasChain]>;
def myJT16 :	    SDNode<"My66000ISD::JT16", SDT_BR_JT, [SDNPHasChain]>;
def myJT32 :	    SDNode<"My66000ISD::JT32", SDT_BR_JT, [SDNPHasChain]>;
def myCALL :        SDNode<"My66000ISD::CALL",
                        SDTypeProfile<0, 1, [SDTCisVT<0, iPTR>]>,
			[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def myTAIL:	    SDNode<"My66000ISD::TAIL",
                        SDTypeProfile<0, 1, [SDTCisVT<0, iPTR>]>,
			[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def myCALLI :	    SDNode<"My66000ISD::CALLI", SDT_CALL,
			[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def myTAILI :	    SDNode<"My66000ISD::TAILI", SDT_CALL,
			[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def cmp :	    SDNode<"My66000ISD::CMP", SDTIntBinOp, []>;
def myEXT :	    SDNode<"My66000ISD::EXT",
			SDTypeProfile<1, 3,
			[SDTCisVT<0, i64>,SDTCisVT<1, i64>,SDTCisVT<2, i64>,
			 SDTCisVT<3, i64>]>,
			[]>;
def myEXTS :	    SDNode<"My66000ISD::EXTS",
			SDTypeProfile<1, 3,
			[SDTCisVT<0, i64>,SDTCisVT<1, i64>,SDTCisVT<2, i64>,
			 SDTCisVT<3, i64>]>,
			[]>;
def cmov:	    SDNode<"My66000ISD::CMOV",
			SDTypeProfile<1, 3,
			  [SDTCisSameAs<0, 1>,SDTCisSameAs<0, 2>,SDTCisVT<3, i64>]>,
			[]>;
def mux:	    SDNode<"My66000ISD::MUX",
			SDTypeProfile<1, 3,
			  [SDTCisSameAs<0, 1>,SDTCisSameAs<0, 2>,SDTCisVT<3, i64>]>,
			[]>;
def myWRAPPER:	    SDNode<"My66000ISD::WRAPPER",
			SDTypeProfile<1, 1,
			  [SDTCisSameAs<0, 1>,SDTCisPtrTy<0>]>,
			[]>;
def memcpy:	    SDNode<"My66000ISD::MEMCPY",
			SDTypeProfile<0, 3,
			  [SDTCisPtrTy<0>,
			   SDTCisVT<1, i64>,
			   SDTCisVT<2, i64>]>,
			[SDNPHasChain, SDNPMayStore, SDNPMayLoad]>;
def memset:	    SDNode<"My66000ISD::MEMSET",
			SDTypeProfile<0, 3,
			  [SDTCisPtrTy<0>,
			   SDTCisVT<1, i64>,
			   SDTCisVT<2, i64>]>,
			[SDNPHasChain, SDNPMayStore]>;

//
// ARITHMETIC AND LOGICAL 16-bit IMMEDIATE
//
multiclass ALUri<string OpName, bits<6>OpC, SDPatternOperator OpNode> {
  def ri : ALU_RI<OpC,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), simm16:$imm))]>;
}
let IsSignExtendingOp = 1 in {
defm ADD : ALUri<"add", 0b110001, add>;
defm MUL : ALUri<"mul", 0b110010, mul>;
defm UDIV: ALUri<"div", 0b110011, udiv>;
}
let isCompare = 1 in {
defm CMP : ALUri<"cmp", 0b110100, cmp>;
}
defm OR  : ALUri<"or",  0b111000, or>;
defm XOR : ALUri<"xor", 0b111001, xor>;
defm AND : ALUri<"and", 0b111010, and>;

//
// ARITHMETIC INTEGER XOP1 64-bits
//
multiclass AOPrr<string OpName, bits<7>OpC, SDPatternOperator OpNode> {
  def rr : XOP1_R<OpC, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), (i64 GRegs:$rs2)))]>;
  def rn : XOP1_R<OpC, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,-$rs2"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), (ineg (i64 GRegs:$rs2))))]>;
  def nr : XOP1_R<OpC, 0b0010,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,$rs2"),
	[(set GRegs:$rd, (OpNode (ineg (i64 GRegs:$rs1)), (i64 GRegs:$rs2)))]>;
  def nn : XOP1_R<OpC, 0b0011,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,-$rs2"),
	[(set GRegs:$rd, (OpNode (ineg (i64 GRegs:$rs1)), (ineg (i64 GRegs:$rs2))))]>;
  def rw : XOP1_W<OpC, 0b1000,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), simm32:$imm))]>;
  def rd : XOP1_D<OpC, 0b1100,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), sneed64:$imm))]>;
}
let IsSignExtendingOp = 1 in {
defm ADD  : AOPrr<"add",  0b0001000, add>;
defm MUL  : AOPrr<"mul",  0b0010000, mul>;
defm UDIV : AOPrr<"div",  0b0011000, udiv>;
defm SDIV : AOPrr<"divs", 0b0011100, sdiv>;
defm UMAX : AOPrr<"max",  0b0110000, umax>;
defm SMAX : AOPrr<"maxs", 0b0110100, smax>;
defm UMIN : AOPrr<"min",  0b0111000, umin>;
defm SMIN : AOPrr<"mins", 0b0111100, smin>;
}
let isCompare = 1 in {
defm CMP  : AOPrr<"cmp",  0b0100000, cmp>;
}

def SUB   : Pat<(sub (i64 GRegs:$rs1), (i64 GRegs:$rs2)),
		(ADDrn GRegs:$rs1, GRegs:$rs2)>;
def NADD  : Pat<(ineg (add (i64 GRegs:$rs1), (i64 GRegs:$rs2))),
		(ADDnn GRegs:$rs1, GRegs:$rs2)>;

//
// ARITHMETIC INTEGER XOP1 32-bits
//

//
// ARITHMETIC INTEGER XOP1 NON-COMMUTATIVE EXTRA PATTERNS
//
multiclass NOPrr<string OpName, bits<7>OpC, SDPatternOperator OpNode> {
  def wr : XOP1_W<OpC, 0b1001,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd, (OpNode simm32:$imm, (i64 GRegs:$rs2)))]>;
  def wn : XOP1_W<OpC, 0b1011,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,-$rs2"),
	[(set GRegs:$rd, (OpNode simm32:$imm, (ineg (i64 GRegs:$rs2))))]>;
  def nw : XOP1_W<OpC,0b1010,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,-$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (ineg (i64 GRegs:$rs1)), simm32:$imm))]>;
  def dr : XOP1_W<OpC, 0b1101,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd, (OpNode sneed64:$imm, (i64 GRegs:$rs2)))]>;
  def dn : XOP1_D<OpC, 0b1111,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,-$rs2"),
	[(set GRegs:$rd, (OpNode sneed64:$imm, (ineg (i64 GRegs:$rs2))))]>;
  def nd : XOP1_D<OpC, 0b1110,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,-$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (ineg (i64 GRegs:$rs1)), sneed64:$imm))]>;
}
let IsSignExtendingOp = 1 in {
defm UDIV : NOPrr<"div",  0b1100110, udiv>;
defm SDIV : NOPrr<"divs", 0b1100111, sdiv>;
def SUBwr : XOP1_W<0b1100010, 0b1001,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	"add\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (sub simm32:$imm, (i64 GRegs:$rs2)))]>;
def SUBdr : XOP1_D<0b1100010, 0b1101,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	"add\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (sub sneed64:$imm, (i64 GRegs:$rs2)))]>;
}
let isCompare = 1 in {
defm CMP  : NOPrr<"cmp",  0b1101000, cmp>;
}

multiclass LOPrr<string OpName, bits<7>OpC, SDPatternOperator OpNode> {
  def rr : XOP1_R<OpC, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), (i64 GRegs:$rs2)))]>;
  def rn : XOP1_R<OpC, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,-$rs2"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), (not (i64 GRegs:$rs2))))]>;
  def nr : XOP1_R<OpC, 0b0010,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,$rs2"),
	[(set GRegs:$rd, (OpNode (not (i64 GRegs:$rs1)), (i64 GRegs:$rs2)))]>;
  def nn : XOP1_R<OpC, 0b0011,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,-$rs2"),
	[(set GRegs:$rd, (OpNode (not (i64 GRegs:$rs1)), (not (i64 GRegs:$rs2))))]>;
  def rw : XOP1_W<OpC, 0b1000,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), simm32:$imm))]>;
  def rd : XOP1_D<OpC, 0b1100,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), sneed64:$imm))]>;
}

defm OR   : LOPrr<"or",   0b0010000, or>;
defm XOR  : LOPrr<"xor",  0b0010010, xor>;
defm AND  : LOPrr<"and",  0b0010100, and>;

def NOR   : Pat<(not (or (i64 GRegs:$rs1), (i64 GRegs:$rs2))),
		(ANDnn GRegs:$rs1, GRegs:$rs2)>;
def NAND  : Pat<(not (and (i64 GRegs:$rs1), (i64 GRegs:$rs2))),
		(ORnn GRegs:$rs1, GRegs:$rs2)>;
// both a*-b and -a*b are expressed as -(a*b)
def       : Pat<(ineg (mul (i64 GRegs:$rs1), (i64 GRegs:$rs2))),
		(MULrn GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(udiv (i64 GRegs:$rs1), (ineg (i64 GRegs:$rs2))),
		(UDIVrn GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(udiv (ineg GRegs:$rs1), (i64 GRegs:$rs2)),
		(UDIVnr GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(sdiv (i64 GRegs:$rs1), (ineg (i64 GRegs:$rs2))),
		(SDIVrn GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(sdiv (ineg (i64 GRegs:$rs1)), (i64 GRegs:$rs2)),
		(SDIVnr GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(sdiv (ineg (i64 GRegs:$rs1)), (ineg(i64 GRegs:$rs2))),
		(SDIVnn GRegs:$rs1, GRegs:$rs2)>;

//
// Shifts with immediate offset and width
// Generated by DAGToDAG
def SLLri : SHF_RI<0b1010,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"sll\t$rd,$rs1,#$o,#$w",
	[]>;
def SRLri : SHF_RI<0b1000,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"srl\t$rd,$rs1,#$o,#$w",
	[]>;
let IsSignExtendingOp = 1 in {
def SLAri : SHF_RI<0b1011,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"sla\t$rd,$rs1,#$o,#$w",
	[]>;
def SRAri : SHF_RI<0b1001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"sra\t$rd,$rs1,#$o,#$w",
	[]>;
}
def RORri : SHF_RI<0b1011,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"ror\t$rd,$rs1,#$o,#$w",
	[]>;
def ROLri : SHF_RI<0b1011,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"rol\t$rd,$rs1,#$o,#$w",
	[]>;
def : Pat<(myEXT (i64 GRegs:$rs1), shfamt:$imm1, shfamt:$imm2),
	  (SRLri GRegs:$rs1, shfamt:$imm1, shfamt:$imm2)>;
def : Pat<(myEXTS (i64 GRegs:$rs1), shfamt:$imm1, shfamt:$imm2),
	  (SRAri GRegs:$rs1, shfamt:$imm1, shfamt:$imm2)>;

//
// Shifts with immediate offset only, full width
// FIXME - XOM7 shifts have the full flags
multiclass SHFrri<string OpName, bits<6>OpC, SDPatternOperator OpNode> {
  def rri : XOM7_R<OpC, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$o),
	!strconcat(OpName, "\t$rd,$rs1,#$o"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), shfamt:$o))]>;
}

let IsSignExtendingOp = 1 in {
defm SRA : SHFrri<"sra", 0b111001, sra>;
}
defm SRL : SHFrri<"srl", 0b111000, srl>;
defm SLL : SHFrri<"sll", 0b111010, shl>;
//defm SLA : SHFrri<"sla", 0b111011, ???>;	// no LLVM operator?
defm ROR : SHFrri<"ror", 0b111100, rotr>;
defm ROL : SHFrri<"rol", 0b111101, rotl>;

//
// Shifts with offset and widths in a register, full flags
multiclass SHFrr<string OpName, bits<7>OpC, SDPatternOperator OpNode> {
  def rr : XOP0_R<OpC, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[(set GRegs:$rd, (OpNode (i64 GRegs:$rs1), (i64 GRegs:$rs2)))]>;
  def wr : XOP0_W<OpC, 0b1001,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd, (OpNode simm32:$imm, (i64 GRegs:$rs2)))]>;
  def dr : XOP0_D<OpC, 0b1001,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd, (OpNode sneed64:$imm, (i64 GRegs:$rs2)))]>;
}

let IsSignExtendingOp = 1 in {
defm SRA  : SHFrr<"sra",  0b1001000, sra>;
}
defm SRL  : SHFrr<"srl",  0b1000000, srl>;
defm SLL  : SHFrr<"sll",  0b1010000, shl>;
defm ROR  : SHFrr<"ror",  0b1100000, rotr>;
defm ROL  : SHFrr<"rol",  0b1101000, rotl>;

//
// MOVs
//
let IsSignExtendingOp = 1 in {
// Immediate 16-bit signed extended
let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
def MOVri : ALU_RI<0b111011,
	(outs GRegs:$rd), (ins s16imm64:$imm),
	"mov\t$rd,#$imm",
	[(set GRegs:$rd, simm16:$imm)]>;
}

// Immediate 32-bit signed extended
let isAsCheapAsAMove = 1 in {
def MOVrw : XOP6_W<0b0111100, 0b00000, 0b0010,
	(outs GRegs:$rd), (ins i64imm:$imm),
	"mov\t$rd,#$imm",
	[(set GRegs:$rd, simm32:$imm)]>;
}

// Immediate 64-bit
let isAsCheapAsAMove = 1 in {
def MOVrd : XOP6_W<0b0111100, 0b00000, 0b0011,
	(outs GRegs:$rd), (ins i64imm:$imm),
	"mov\t$rd,#$imm",
	[(set GRegs:$rd, sneed64:$imm)]>;
}

// Reg to Reg never matches, just built to copyPhysReg
def MOVrr : XOP6_W<0b0111100, 0b00000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs2),
	"mov\t$rd,$rs2",
	[]>;
}

//
// MUX, CMOV
//
// FIXME - there is only CMOV now
multiclass MUXr<string OpName, SDPatternOperator OpNode> {
  def rr : MUX_R<0b00,  0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	!strconcat(OpName, "\t$rd,$rs3,$rs1,$rs2"),
	[(set (i64 GRegs:$rd),
	    (OpNode (i64 GRegs:$rs1), (i64 GRegs:$rs2), GRegs:$rs3))]>;
  def rn : MUX_R<0b00,  0b0100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	!strconcat(OpName, "\t$rd,$rs3,$rs1,$rs2"),
	[(set (i64 GRegs:$rd),
	    (OpNode (i64 GRegs:$rs1), (ineg (i64 GRegs:$rs2)), GRegs:$rs3))]>;
  def nr : MUX_R<0b00,  0b1000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	!strconcat(OpName, "\t$rd,$rs3,$rs1,$rs2"),
	[(set (i64 GRegs:$rd),
	    (OpNode (ineg (i64 GRegs:$rs1)), (i64 GRegs:$rs2), GRegs:$rs3))]>;
  def nn : MUX_R<0b00,  0b1100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	!strconcat(OpName, "\t$rd,$rs3,$rs1,$rs2"),
	[(set (i64 GRegs:$rd),
	    (OpNode (ineg (i64 GRegs:$rs1)), (ineg (i64 GRegs:$rs2)), GRegs:$rs3))]>;
  def rw : MUX_W<0b00, 0b0010,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm, GRegs:$rs3),
	!strconcat(OpName, "\t$rd,$rs3,$rs1,#$imm"),
	[(set (i64 GRegs:$rd),
	    (OpNode (i64 GRegs:$rs1), simm32:$imm, GRegs:$rs3))]>;
  def rd : MUX_D<0b00, 0b0011,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm, GRegs:$rs3),
	!strconcat(OpName, "\t$rd,$rs3,$rs1,#$imm"),
	[(set (i64 GRegs:$rd),
	    (OpNode (i64 GRegs:$rs1), sneed64:$imm, GRegs:$rs3))]>;
  def wr : MUX_W<0b00, 0b0110,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2, GRegs:$rs3),
	!strconcat(OpName, "\t$rd,$rs3,#$imm,$rs2"),
	[(set (i64 GRegs:$rd),
	    (OpNode simm32:$imm, (i64 GRegs:$rs2), GRegs:$rs3))]>;
  def dr : MUX_W<0b00, 0b0111,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2, GRegs:$rs3),
	!strconcat(OpName, "\t$rd,$rs3,#$imm,$rs2"),
	[(set (i64 GRegs:$rd),
	    (OpNode sneed64:$imm, (i64 GRegs:$rs2), GRegs:$rs3))]>;
}

defm CMOVr : MUXr<"cmov", cmov>;

// Generated by DAGToDAG
// FIXME
def INSrrw : INS<0b010, 0b100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs: $rs2, i32imm:$w, i32imm:$o),
	"ins\t$rd,$rs1,$rs2,#$o,#$w",
	[]>;

//
// Misc 1 operand instructions
//
def CTLZ : XOP6_R<0b1001000, 0b00000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"clz\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (ctlz (i64 GRegs:$rs1)))]>;
def CTLO : XOP6_R<0b1001000, 0b00000, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"clz\t$rd,-$rs1",
	[(set (i64 GRegs:$rd), (ctlz (xor (i64 GRegs:$rs1) ,-1)))]>;
def CTTZ : XOP6_R<0b1001000, 0b00011, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"ctz\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (cttz (i64 GRegs:$rs1)))]>;
def CTTO : XOP6_R<0b1001000, 0b00011, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"ctz\t$rd,-$rs1",
	[(set (i64 GRegs:$rd), (cttz (xor (i64 GRegs:$rs1) ,-1)))]>;
def FL1  : XOP6_R<0b1001000, 0b00001, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"fl1\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (xor (ctlz (i64 GRegs:$rs1)), 63))]>;
def POPO : XOP6_R<0b1000000, 0b00000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"pop\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (ctpop (i64 GRegs:$rs1)))]>;
def POPZ : XOP6_R<0b1000000, 0b00000, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"pop\t$rd,-$rs1",
	[(set (i64 GRegs:$rd), (ctpop (xor (i64 GRegs:$rs1) ,-1)))]>;
def IABS : XOP6_R<0b0111000, 0b00000, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"abs\t$rd,$rs1",
	[(set (i64 GRegs:$rd), (abs (i64 GRegs:$rs1)))]>;

// FIXME - move up to shifts
def BITR : SHF_RI<0b1100,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"bitr\t$rd,$rs1,#$o,#$w",
	[]>;
def : Pat<(bswap (i64 GRegs:$rs1)),
	  (BITR GRegs:$rs1, 8, 0)>;
def : Pat<(srl (bswap (i64 GRegs:$rs1)), shfamt:$imm),
	  (BITR GRegs:$rs1, 8, shfamt:$imm)>;
def : Pat<(bitreverse (i64 GRegs:$rs1)),
	  (BITR GRegs:$rs1, 1, 0)>;
def : Pat<(srl (bitreverse (i64 GRegs:$rs1)), shfamt:$imm),
	  (BITR GRegs:$rs1, 1, shfamt:$imm)>;

//
// Load/Store
//
// Addressing modes.
def ADDRrr : ComplexPattern<iPTR, 4, "SelectADDRrr", [], []>;
def ADDRri : ComplexPattern<iPTR, 2, "SelectADDRri", [frameindex], []>;

def MEMri : Operand<iPTR> {
  let PrintMethod = "printMEMriOperand";
  let MIOperandInfo = (ops ptr_rc, i64imm);
//  let ParserMatchClass = My66000MEMriAsmOperand;
}
def MEMrr : Operand<iPTR> {
  let PrintMethod = "printMEMrrOperand";
  let MIOperandInfo = (ops ptr_rc, ptr_rc, i64imm, i64imm);
//  let ParserMatchClass = My66000MEMrrAsmOperand;
}

//
// Load (multiclass Load used by floating point also)
//
multiclass Load<string OpName, bits<6>OpCri, bits<7>OpCrr,
		ValueType VT, SDPatternOperator OpNode> {
  def ri : LDST_BD<OpCri,
	(outs GRegs:$rd), (ins MEMri:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(set (VT GRegs:$rd), (OpNode ADDRri:$addr))]>;
  def rr : LDST_RX<OpCrr, 0b00,
	(outs GRegs:$rd), (ins MEMrr:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(set (VT GRegs:$rd), (OpNode ADDRrr:$addr))]>;
}
let mayLoad = 1, IsSignExtendingOp = 1 in {
  defm LDD  : Load<"ldd",  0b100011, 0b1100000, i64, load>;
  defm LDSB : Load<"ldsb", 0b100100, 0b0000010, i64, sextloadi8>;
  defm LDSH : Load<"ldsh", 0b100101, 0b0100010, i64, sextloadi16>;
  defm LDSW : Load<"ldsw", 0b100110, 0b1000010, i64, sextloadi32>;
}
let mayLoad = 1, IsZeroExtendingOp = 1 in {
  defm LDUB : Load<"ldub", 0b100000, 0b0000000, i64, zextloadi8>;
  defm LDUH : Load<"lduh", 0b100001, 0b0100000, i64, zextloadi16>;
  defm LDUW : Load<"lduw", 0b100010, 0b1000000, i64, zextloadi32>;
}
// Map integer extload's to sextloads.
def : Pat<(i64 (extloadi32 ADDRrr:$src)), (LDSWrr ADDRrr:$src)>;
def : Pat<(i64 (extloadi16 ADDRrr:$src)), (LDSHrr ADDRrr:$src)>;
def : Pat<(i64 (extloadi8  ADDRrr:$src)), (LDSBrr ADDRrr:$src)>;
def : Pat<(i64 (extloadi32 ADDRri:$src)), (LDSWri ADDRri:$src)>;
def : Pat<(i64 (extloadi16 ADDRri:$src)), (LDSHri ADDRri:$src)>;
def : Pat<(i64 (extloadi8  ADDRri:$src)), (LDSBri ADDRri:$src)>;

//
// Load address
//
def LArr : LDST_RX<0b1100010, 0b00,
	(outs GRegs:$rd), (ins MEMrr:$addr),
	"la\t$rd,[$addr]",
	[]>;

def : Pat<(add (add (i64 GRegs:$rb), (shl (i64 GRegs:$rx), shfinx:$sc)), imm:$disp),
	  (LArr GRegs:$rb, GRegs:$rx, shfinx:$sc, imm:$disp)>;
def : Pat<(add (i64 GRegs:$rb), (shl (i64 GRegs:$rx), shfinx:$sc)),
	  (LArr GRegs:$rb, GRegs:$rx, shfinx:$sc, 0)>;
// FIXME - can the following be done with a Pat<>?
def LAg  : LDST_RX<0b1100010, 0b11,
	(outs GRegs:$rd), (ins gbladr:$addr),
	"la\t$rd,[ip,$addr]",
	[(set GRegs:$rd, (myWRAPPER tglobaladdr:$addr))]>;
def LAgx : LDST_RX<0b1100010, 0b11,
	(outs GRegs:$rd), (ins GRegs:$rx, gbladr:$addr),
	"la\t$rd,[ip,$rx,$addr]",
	[(set GRegs:$rd, (add (i64 GRegs:$rx), (myWRAPPER tglobaladdr:$addr)))]>;
def LAgxs: LDST_RX<0b1100010, 0b11,
	(outs GRegs:$rd), (ins GRegs:$rx, shfinx:$sc, gbladr:$addr),
	"la\t$rd,[ip,$rx<<$sc,$addr]",
	[(set GRegs:$rd,
	 (add (shl (i64 GRegs:$rx), shfinx:$sc), (myWRAPPER tglobaladdr:$addr)))]>;
def LAb  : LDST_RX<0b1100010, 0b11,
	(outs GRegs:$rd), (ins gbladr:$addr),
	"la\t$rd,[ip,$addr]",
	[(set GRegs:$rd, (myWRAPPER tblockaddress:$addr))]>;

//
// Store (multiclass Store used by floating point also)
//
multiclass Store<string OpName, bits<6>OpCri, bits<7>OpCrr,
		 ValueType VT, SDPatternOperator OpNode> {
  def ri : LDST_BD<OpCri,
	(outs), (ins GRegs:$rd, MEMri:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(OpNode (VT GRegs:$rd), ADDRri:$addr)]>;
  def rr : LDST_RX<OpCrr, 0b00,
	(outs), (ins GRegs:$rd, MEMrr:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(OpNode (VT GRegs:$rd), ADDRrr:$addr)]>;
}
let mayStore = 1 in {
  defm STB : Store<"stb", 0b101000, 0b0000100, i64, truncstorei8>;
  defm STH : Store<"sth", 0b101001, 0b0100100, i64, truncstorei16>;
  defm STW : Store<"stw", 0b101010, 0b1000100, i64, truncstorei32>;
  defm STD : Store<"std", 0b101011, 0b1100100, i64, store>;
}

// Because ADDRrr rejects anything ADDRri can handle, we need a pattern for both.
multiclass StImm5<string OpName, bits<7>Op, SDPatternOperator OpNode> {
  def kii : LDST_RX<Op, 0b00,
	(outs), (ins i64imm:$imm, MEMri:$addr),
	!strconcat(OpName, "\t#$imm,[$addr]"),
	[(OpNode simm5:$imm, ADDRri:$addr)]>;
  def kir : LDST_RX<Op, 0b00,
	(outs), (ins i64imm:$imm, MEMrr:$addr),
	!strconcat(OpName, "\t#$imm,[$addr]"),
	[(OpNode simm5:$imm, ADDRrr:$addr)]>;
}
let mayStore = 1 in {
  defm STB : StImm5<"stb", 0b1111011, truncstorei8>;
  defm STH : StImm5<"sth", 0b0111010, truncstorei16>;
  defm STW : StImm5<"stw", 0b1111001, truncstorei32>;
  defm STD : StImm5<"std", 0b1111000, store>;
}
multiclass StImm<string OpName, bits<7>Op, SDPatternOperator OpNode> {
  def wii : LDST_RX<Op, 0b00,
	(outs), (ins i64imm:$imm, MEMri:$addr),
	!strconcat(OpName, "\t#$imm,[$addr]"),
	[(OpNode simm32:$imm, ADDRri:$addr)]>;
  def wir : LDST_RX<Op, 0b00,
	(outs), (ins i64imm:$imm, MEMrr:$addr),
	!strconcat(OpName, "\t#$imm,[$addr]"),
	[(OpNode simm32:$imm, ADDRrr:$addr)]>;
  def dii : LDST_RX<Op, 0b00,
	(outs), (ins i64imm:$imm, MEMri:$addr),
	!strconcat(OpName, "\t#$imm,[$addr]"),
	[(OpNode sneed64:$imm, ADDRri:$addr)]>;
  def dir : LDST_RX<Op, 0b00,
	(outs), (ins i64imm:$imm, MEMrr:$addr),
	!strconcat(OpName, "\t#$imm,[$addr]"),
	[(OpNode sneed64:$imm, ADDRrr:$addr)]>;
}
let mayStore = 1 in {
  defm STB : StImm<"stb", 0b1110011, truncstorei8>;
  defm STH : StImm<"sth", 0b1110010, truncstorei16>;
  defm STW : StImm<"stw", 0b1110001, truncstorei32>;
  defm STD : StImm<"std", 0b1110000, store>;
}


def LDM : LDST_BD<0b010011,
	(outs), (ins GRegs:$rd, GRegs:$rs, MEMrr:$addr),
	"ldm\t$rd,$rs,[$addr]",
	[]>;
def STM : LDST_BD<0b010100,
	(outs), (ins GRegs:$rd, GRegs:$rs, MEMrr:$addr),
	"stm\t$rd,$rs,[$addr]",
	[]>;

// Memory to memory copy, memory set
def MEMCPYr : XOP5_R<0b1010100, 0b0000,
	(outs), (ins GRegs:$dst, GRegs:$src, GRegs:$len),
	"mm\t$dst,$src,$len",
	[(memcpy GRegs:$dst, GRegs:$src, GRegs:$len)]>;
def MEMCPYw : XOP5_R<0b1010100, 0b0010,
	(outs), (ins GRegs:$dst, GRegs:$src, i64imm:$len),
	"mm\t$dst,$src,#$len",
	[(memcpy GRegs:$dst, GRegs:$src, simm32:$len)]>;
def MEMCPYd : XOP5_R<0b1010100, 0b0011,
	(outs), (ins GRegs:$dst, GRegs:$src, i64imm:$len),
	"mm\t$dst,$src,#$len",
	[(memcpy GRegs:$dst, GRegs:$src, sneed64:$len)]>;
def MEMSETrr: XOP5_R<0b1011100, 0b0000,
	(outs), (ins GRegs:$dst, GRegs:$src, GRegs:$len),
	"ms\t$dst,$src,$len",
	[(memset GRegs:$dst, GRegs:$src, GRegs:$len)]>;
def MEMSETrw: XOP5_R<0b1011100, 0b0010,
	(outs), (ins GRegs:$dst, GRegs:$src, i64imm:$len),
	"ms\t$dst,$src,#$len",
	[(memset GRegs:$dst, GRegs:$src, simm32:$len)]>;
def MEMSETrd: XOP5_R<0b1011100, 0b0011,
	(outs), (ins GRegs:$dst, GRegs:$src, i64imm:$len),
	"ms\t$dst,$src,#$len",
	[(memset GRegs:$dst, GRegs:$src, sneed64:$len)]>;
def MEMSETkr: XOP5_R<0b1111100, 0b0000,
	(outs), (ins GRegs:$dst, i64imm:$src, GRegs:$len),
	"ms\t$dst,#$src,$len",
	[(memset GRegs:$dst, simm5:$src, GRegs:$len)]>;
def MEMSETkw: XOP5_R<0b1111100, 0b0010,
	(outs), (ins GRegs:$dst, i64imm:$src, i64imm:$len),
	"ms\t$dst,#$src,#$len",
	[(memset GRegs:$dst, simm5:$src, simm32:$len)]>;
def MEMSETkd: XOP5_R<0b1111100, 0b0011,
	(outs), (ins GRegs:$dst, i64imm:$src, i64imm:$len),
	"ms\t$dst,#$src,#$len",
	[(memset GRegs:$dst, simm5:$src, sneed64:$len)]>;

// ENTER/EXIT
let mayStore = 1 in {
def ENTER : ALU_RI<0b101100,
	(outs), (ins GRegs:$rd, GRegs:$rb, i16imm:$imm, i16imm:$f),
	"enter\t$rd,$rb,$imm,$f",
	[]>;
}
let mayLoad = 1 in {
def EXIT : ALU_RI<0b101100,
	(outs), (ins GRegs:$rd, GRegs:$rb, i16imm:$imm, i16imm:$f),
	"exit\t$rd,$rb,$imm,$f",
	[]>;
}

// Word (32-bit) operations
//
include "My66000InstrInfoW.td"

// Floating point
//
include "My66000InstrInfoFP.td"
include "My66000InstrInfoFS.td"


//
// Flow control
//
def CBOp : Operand<i8> {
  let PrintMethod = "printCBOperand";
}
def CCOp : Operand<i8> {
  let PrintMethod = "printCCOperand";
}

let isTerminator = 1, isReturn = 1, isBarrier = 1 in {
def RET: Simple<0b001101, 0b00000000000100000000000000,
	(outs), (ins),
	"ret",
	[(myRET)]>;
}

let isBranch = 1, isTerminator = 1, isBarrier = 1 in {
def BRU : BR<0b011110,
	(outs), (ins brtarget:$adr),
	"br\t$adr",
	[(br bb:$adr)]>;
}

// FIXME - extend to general memory addresses
let isBranch = 1, isIndirectBranch = 1, isTerminator = 1, isBarrier = 1 in {
def BRI : Simple<0b001101, 0b00000000000100000000000000,
       (outs), (ins GRegs:$rs),
       "cala\t[$rs]",
       [(brind GRegs:$rs)]>;
}

let isBranch = 1, isTerminator = 1 in {
def BRC : BC<0b011010,
	(outs), (ins brtarget:$adr, GRegs:$rs1, ccode:$cc),
	"brc\t$cc,$rs1,$adr",
	[(myBRcond bb:$adr, GRegs:$rs1, imm:$cc)]>;
def BRIB : BB<0b01100,
	(outs), (ins brtarget:$adr, GRegs:$rs1, i64imm:$cb),
	"brb1\t$cb,$rs1,$adr",
	[(myBRcc bb:$adr, GRegs:$rs1, shfamt:$cb)]>;
def BRFB : BB<0b01100,
	(outs), (ins brtarget:$adr, GRegs:$rs1, i64imm:$cb),
	"brb1\t$cb,$rs1,$adr",
	[(myBRfcc bb:$adr, GRegs:$rs1, shfamt:$cb)]>;
def BBIT : BB<0b01100,
	(outs), (ins brtarget:$adr, GRegs:$rs1, i64imm:$bit),
	"bb1\t$bit,$rs1,$adr",
	[(myBRbit bb:$adr, GRegs:$rs1, shfamt:$bit)]>;
}

//
// Predication Instructions, never selected
//
let isPredicable = 1 in {
def PRC : PC<0b000110,
	(outs), (ins ccode:$cc, GRegs:$rs1, i8imm:$true, i8imm:$false),
	"pc\t$cc,$rs1,$true,$false",
	[]>;
def PRIB : PB<0b00011,
	(outs), (ins i64imm:$cb, GRegs:$rs1, i8imm:$true, i8imm:$false),
	"pb1\t$cb,$rs1,$true,$false",
	[]>;
def PRFB : PB<0b00011,
	(outs), (ins i64imm:$cb, GRegs:$rs1, i8imm:$true, i8imm:$false),
	"pb1\t$cb,$rs1,$true,$false",
	[]>;
def PBIT : PB<0b00011,
	(outs), (ins i64imm:$bit, GRegs:$rs1, i8imm:$true, i8imm:$false),
	"pb1\t$bit,$rs1,$true,$false",
	[]>;
}


// Jump tables.
def InlineJT8 : Operand<i64> {
  let PrintMethod = "printInlineJT8";
}
def InlineJT16 : Operand<i64> {
  let PrintMethod = "printInlineJT16";
}
def InlineJT32 : Operand<i64> {
  let PrintMethod = "printInlineJT32";
}

// Because JTx can fall through (if out of range) what should we do?
let isBranch = 1, isIndirectBranch = 1, isTerminator = 1, isBarrier = 1 in {
def JT8 : Pseudo<
	(outs), (ins InlineJT8:$t, GRegs:$i, i64imm:$n, brtarget:$dft),
	"jttb\t$i,#$n\n$t",
	[(myJT8 tjumptable:$t, GRegs:$i, uimm16:$n, bb:$dft)]>;
def JT16 : Pseudo<
	(outs), (ins InlineJT16:$t, GRegs:$i, i64imm:$n, brtarget:$dft),
	"jtth\t$i,#$n\n$t",
	[(myJT16 tjumptable:$t, GRegs:$i, uimm16:$n, bb:$dft)]>;
def JT32 : Pseudo<
	(outs), (ins InlineJT32:$t, GRegs:$i, i64imm:$n, brtarget:$dft),
	"jttw\t$i,#$n\n$t",
	[(myJT32 tjumptable:$t, GRegs:$i, uimm16:$n, bb:$dft)]>;
}

let isCall = 1, isBarrier = 1, Defs = [R0] in {
def CALL :  BR<0b011111,
	(outs), (ins calltarget:$adr),
	"call\t$adr",
	[(myCALL tglobaladdr:$adr)]>;
let mayLoad = 1 in {
def CALLXri : LDST_RX<0b0110000, 0b00,
	(outs), (ins MEMri:$addr),
	"calx\t[$addr]",
	[]>;
def CALLXrr : LDST_RX<0b0110000, 0b00,
	(outs), (ins MEMrr:$addr),
	"calx\t[$addr]",
	[]>;
}
def CALLArr : LDST_RX<01110000, 0b00,
	(outs), (ins GRegs:$rs),
	"cala\t[$rs]",
	[(myCALLI i64:$rs)]>;
}
def : Pat<(myCALL texternalsym:$adr), (CALL texternalsym:$adr)>;
def : Pat<(myCALLI (load ADDRri:$addr)), (CALLXri ADDRri:$addr)>;
def : Pat<(myCALLI (load ADDRrr:$addr)), (CALLXrr ADDRrr:$addr)>;

let isCall = 1, isTerminator = 1, isReturn = 1, isBarrier = 1 in {
def TAIL :  BR<0b011110,
	(outs), (ins calltarget:$adr),
	"br\t$adr",
	[(myTAIL tglobaladdr:$adr)]>;
}
def : Pat<(myTAIL texternalsym:$adr), (TAIL texternalsym:$adr)>;
def : Pat<(myTAILI GRegs:$rs), (BRI GRegs:$rs)>;

//
// Experimental CARRY instruction and related stuff
//
let Constraints = "$rd = $rs" in {
def CARRYio : ALU_RI<0b111110,
	(outs GRegs:$rd), (ins GRegs:$rs, i64imm:$imm),
	"carry\t$rd,#$imm",
	[]>;
}
def CARRYo : ALU_RI<0b111110,
	(outs GRegs:$rd), (ins i64imm:$imm),
	"carry\t$rd,#$imm",
	[]>;
def CARRYi : ALU_RI<0b111110,
	(outs), (ins GRegs:$rs, i64imm:$imm),
	"carry\t$rs,#$imm",
	[]>;

def SDT_ADDCO : SDTypeProfile<2, 2,
		[SDTCisVT<0, i64>,SDTCisVT<1, i64>,SDTCisVT<2, i64>,
		 SDTCisVT<3, i64>]>;
def SDT_ADDCIO : SDTypeProfile<2, 3,
		[SDTCisVT<0, i64>,SDTCisVT<1, i64>,SDTCisVT<2, i64>,
		 SDTCisVT<3, i64>,SDTCisVT<4, i64>]>;
def SDT_ISHFW : SDTypeProfile<2, 3,
		[SDTCisVT<0, i64>,SDTCisSameAs<0, 1>, SDTCisSameAs<0, 2>,
		 SDTCisSameAs<0, 3>,SDTCisInt<4>]>;
def uaddo :	SDNode<"ISD::UADDO", SDT_ADDCO, []>;
def usubo :	SDNode<"ISD::USUBO", SDT_ADDCO, []>;
def addcarry :	SDNode<"ISD::UADDO_CARRY", SDT_ADDCIO, []>;
def subcarry :	SDNode<"ISD::USUBO_CARRY", SDT_ADDCIO, []>;
def umul_lohi : SDNode<"ISD::UMUL_LOHI", SDTIntBinHiLoOp, []>;
def smul_lohi : SDNode<"ISD::SMUL_LOHI", SDTIntBinHiLoOp, []>;
def srl_parts : SDNode<"ISD::SRL_PARTS", SDT_ISHFW, []>;
def shl_parts : SDNode<"ISD::SHL_PARTS", SDT_ISHFW, []>;
def sra_parts : SDNode<"ISD::SRA_PARTS", SDT_ISHFW, []>;

// The following are expanded by the ExpandPseudoInsts pass
def UADDOrrc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, GRegs:$rhs),
	"# UADD0rrc",
	[(set GRegs:$sum, GRegs:$co, (uaddo (i64 GRegs:$lhs), (i64 GRegs:$rhs)))]>;
def UADDOric : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs),
	"# UADD0ric",
	[(set GRegs:$sum, GRegs:$co, (uaddo (i64 GRegs:$lhs), simm16:$rhs))]>;
def UADDOrwc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs),
	"# UADD0rwc",
	[(set GRegs:$sum, GRegs:$co, (uaddo (i64 GRegs:$lhs), simm32:$rhs))]>;
def UADDOrdc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs),
	"# UADD0rdc",
	[(set GRegs:$sum, GRegs:$co, (uaddo (i64 GRegs:$lhs), sneed64:$rhs))]>;
def USUBOrrc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, GRegs:$rhs),
	"# UADD0rrc",
	[(set GRegs:$sum, GRegs:$co, (usubo (i64 GRegs:$lhs), (i64 GRegs:$rhs)))]>;
def USUBOric : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs),
	"# UADD0ric",
	[(set GRegs:$sum, GRegs:$co, (usubo (i64 GRegs:$lhs), simm16:$rhs))]>;
def ADDrrbc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, GRegs:$rhs, GRegs:$ci),
	"# ADDrrbc",
	[(set GRegs:$sum, GRegs:$co, (addcarry (i64 GRegs:$lhs), (i64 GRegs:$rhs), GRegs:$ci))]>;
def ADDribc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs, GRegs:$ci),
	"# ADDribc",
	[(set GRegs:$sum, GRegs:$co, (addcarry (i64 GRegs:$lhs), simm16:$rhs, GRegs:$ci))]>;
def ADDrwbc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs, GRegs:$ci),
	"# ADDrwbc",
	[(set GRegs:$sum, GRegs:$co, (addcarry (i64 GRegs:$lhs), simm32:$rhs, GRegs:$ci))]>;
def ADDrdbc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs, GRegs:$ci),
	"# ADDrdbc",
	[(set GRegs:$sum, GRegs:$co, (addcarry (i64 GRegs:$lhs), sneed64:$rhs, GRegs:$ci))]>;
def SUBrrbc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, GRegs:$rhs, GRegs:$ci),
	"# SUBrrbc",
	[(set GRegs:$sum, GRegs:$co, (subcarry (i64 GRegs:$lhs), (i64 GRegs:$rhs), GRegs:$ci))]>;
def SUBribc : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs, GRegs:$ci),
	"# SUBribc",
	[(set GRegs:$sum, GRegs:$co, (subcarry (i64 GRegs:$lhs), simm16:$rhs, GRegs:$ci))]>;
def UMULHILOrrc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, GRegs:$rhs),
	"# UMULHILOrcr",
	[(set GRegs:$lo, GRegs:$hi, (umul_lohi (i64 GRegs:$lhs), (i64 GRegs:$rhs)))]>;
def UMULHILOric : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# UMULHILOric",
	[(set GRegs:$lo, GRegs:$hi, (umul_lohi (i64 GRegs:$lhs), simm16:$rhs))]>;
def UMULHILOrwc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# UMULHILOrwc",
	[(set GRegs:$lo, GRegs:$hi, (umul_lohi (i64 GRegs:$lhs), simm32:$rhs))]>;
def UMULHILOrdc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# UMULHILOrdc",
	[(set GRegs:$lo, GRegs:$hi, (umul_lohi (i64 GRegs:$lhs), sneed64:$rhs))]>;
def SMULHILOrrc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, GRegs:$rhs),
	"# SMULHILOrrc",
	[(set GRegs:$lo, GRegs:$hi, (smul_lohi (i64 GRegs:$lhs), (i64 GRegs:$rhs)))]>;
def SMULHILOric : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# SMULHILOric",
	[(set GRegs:$lo, GRegs:$hi, (smul_lohi (i64 GRegs:$lhs), simm16:$rhs))]>;
def SMULHILOrwc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# SMULHILOrwc",
	[(set GRegs:$lo, GRegs:$hi, (smul_lohi (i64 GRegs:$lhs), simm32:$rhs))]>;
def SMULHILOrdc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# SMULHILOrdc",
	[(set GRegs:$lo, GRegs:$hi, (smul_lohi (i64 GRegs:$lhs), sneed64:$rhs))]>;
def UDIVREMrrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$rs1, GRegs:$rs2),
	"# UDIVREMrrc",
	[]>;
def UDIVREMric : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# UDIVREMric",
	[]>;
def UDIVREMrwc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# UDIVREMrwc",
	[]>;
def UDIVREMwrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# UDIVREMwrc",
	[]>;
def UDIVREMrdc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# UDIVREMrdc",
	[]>;
def UDIVREMdrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# UDIVREMdrc",
	[]>;
def SDIVREMrrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMrrc",
	[]>;
def SDIVREMrnc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMrnc",
	[]>;
def SDIVREMnrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMnrc",
	[]>;
def SDIVREMnnc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMnnc",
	[]>;
def SDIVREMrwc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# SDIVREMrwc",
	[]>;
def SDIVREMwrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# SDIVREMwrc",
	[]>;
def SDIVREMrdc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# SDIVREMrdc",
	[]>;
def SDIVREMdrc : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# SDIVREMdrc",
	[]>;
def SRL2rrbc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$slo, GRegs:$shi, GRegs:$cnt),
	"# SRL2rrbc",
	[(set GRegs:$lo, GRegs:$hi,
	    (srl_parts (i64 GRegs:$slo), (i64 GRegs:$shi), (i64 GRegs:$cnt)))]>;
def SLL2rrbc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$slo, GRegs:$shi, GRegs:$cnt),
	"# SHL2rrbc",
	[(set GRegs:$lo, GRegs:$hi,
	    (shl_parts (i64 GRegs:$slo),  (i64 GRegs:$shi), (i64 GRegs:$cnt)))]>;
def SRA2rrbc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$slo, GRegs:$shi, GRegs:$cnt),
	"# SRL2rrbc",
	[(set GRegs:$lo, GRegs:$hi,
	    (sra_parts (i64 GRegs:$slo), (i64 GRegs:$shi), (i64 GRegs:$cnt)))]>;
def SRL2ribc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$slo, GRegs:$shi, i64imm:$cnt),
	"# SRL2ribc",
	[(set GRegs:$lo, GRegs:$hi,
	    (srl_parts (i64 GRegs:$slo), (i64 GRegs:$shi), shfamt:$cnt))]>;
def SLL2ribc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$slo, GRegs:$shi, i64imm:$cnt),
	"# SHL2ribc",
	[(set GRegs:$lo, GRegs:$hi,
	    (shl_parts (i64 GRegs:$slo), (i64 GRegs:$shi), shfamt:$cnt))]>;
def SRA2ribc : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$slo, GRegs:$shi, i64imm:$cnt),
	"# SRL2ribc",
	[(set GRegs:$lo, GRegs:$hi,
	    (sra_parts (i64 GRegs:$slo), (i64 GRegs:$shi), shfamt:$cnt))]>;

let usesCustomInserter = 1 in {
def UDIVREMrr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# UDIVREMrr",
	[(set GRegs:$div, GRegs:$rem, (udivrem (i64 GRegs:$lhs), GRegs:$rhs))]>;
def UDIVREMri : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# UDIVREMri",
	[(set GRegs:$div, GRegs:$rem, (udivrem (i64 GRegs:$lhs), simm16:$rhs))]>;
def UDIVREMrw : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# UDIVREMrw",
	[(set GRegs:$div, GRegs:$rem, (udivrem (i64 GRegs:$lhs), simm32:$rhs))]>;
def UDIVREMwr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# UDIVREMwr",
	[(set GRegs:$div, GRegs:$rem, (udivrem simm32:$lhs, GRegs:$rhs))]>;
def UDIVREMrd : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# UDIVREMrd",
	[(set GRegs:$div, GRegs:$rem, (udivrem (i64 GRegs:$lhs), sneed64:$rhs))]>;
def UDIVREMdr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# UDIVREMdr",
	[(set GRegs:$div, GRegs:$rem, (udivrem sneed64:$lhs, GRegs:$rhs))]>;
def SDIVREMrr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMrr",
	[(set GRegs:$div, GRegs:$rem, (sdivrem (i64 GRegs:$lhs), GRegs:$rhs))]>;
def SDIVREMrn : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMrn",
	[(set GRegs:$div, GRegs:$rem, (sdivrem (i64 GRegs:$lhs), (ineg GRegs:$rhs)))]>;
def SDIVREMnr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMnr",
	[(set GRegs:$div, GRegs:$rem, (sdivrem (ineg (i64 GRegs:$lhs)), GRegs:$rhs))]>;
def SDIVREMnn : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMnn",
	[(set GRegs:$div, GRegs:$rem, (sdivrem (ineg (i64 GRegs:$lhs)), (ineg GRegs:$rhs)))]>;
def SDIVREMrx : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# SDIVREMrw",
	[(set GRegs:$div, GRegs:$rem, (sdivrem (i64 GRegs:$lhs), simm32:$rhs))]>;
def SDIVREMwr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# SDIVREMwr",
	[(set GRegs:$div, GRegs:$rem, (sdivrem simm32:$lhs, GRegs:$rhs))]>;
def SDIVREMrd : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# SDIVREMrd",
	[(set GRegs:$div, GRegs:$rem, (sdivrem (i64 GRegs:$lhs), sneed64:$rhs))]>;
def SDIVREMdr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# SDIVREMdr",
	[(set GRegs:$div, GRegs:$rem, (sdivrem sneed64:$lhs, GRegs:$rhs))]>;
}

//
// Virtual Vector Mode (VVM) Instructions
//
def VEC : VECF<0b111101,
	(outs GRegs:$rd), (ins i16imm:$imm),
	"vec\t$rd,$imm",
	[]>;
// FIXME
let isBranch = 1, isTerminator = 1, Constraints = "$rlout = $rlin" in {
def LOOP1rr : LOOP<0b111, 0b000,
	(outs GRegs:$rlout), (ins CBOp:$cnd, GRegs:$rlin, GRegs:$ri, GRegs:$rb,
		 GRegs:$ra, brtarget:$adr),
	"loop1\t$cnd,$rlout,$ri,$rb",
	[]>;
def LOOP1ri : LOOP<0b111, 0b000,
	(outs GRegs:$rlout), (ins CBOp:$cnd, GRegs:$rlin, GRegs:$ri, i16imm:$ib,
		 GRegs:$ra, brtarget:$adr),
	"loop1\t$cnd,$rlout,$ri,#$ib",
	[]>;
def LOOP1ir : LOOP<0b111, 0b000,
	(outs GRegs:$rlout), (ins CBOp:$cnd, GRegs:$rlin, i16imm:$ii, GRegs:$rb,
		 GRegs:$ra, brtarget:$adr),
	"loop1\t$cnd,$rlout,#$ii,$rb",
	[]>;
def LOOP1ii : LOOP<0b111, 0b000,
	(outs GRegs:$rlout), (ins CBOp:$cnd, GRegs:$rlin, i16imm:$ii, i16imm:$ib,
		 GRegs:$ra, brtarget:$adr),
	"loop1\t$cnd,$rlout,#$ii,#$ib",
	[]>;
def LOOP2ri : LOOP<0b111, 0b000,
	(outs GRegs:$rlout), (ins CBOp:$cnd, GRegs:$rlin, GRegs:$ri, i16imm:$ib,
		 GRegs:$ra, brtarget:$adr),
	"loop2\t$cnd,$rlout,$ri,#$ib",
	[]>;
def LOOP2ii : LOOP<0b111, 0b000,
	(outs GRegs:$rlout), (ins CBOp:$cnd, GRegs:$rlin, i16imm:$ii, i16imm:$ib,
		 GRegs:$ra, brtarget:$adr),
	"loop2\t$cnd,$rlout,#$ii,#$ib",
	[]>;
def LOOP3rr : LOOP<0b111, 0b000,
	(outs GRegs:$rlout), (ins CBOp:$cnd, GRegs:$rlin, GRegs:$ri, GRegs:$rc,
		 GRegs:$ra, brtarget:$adr),
	"loop3\t$cnd,$rlout,$ri,$rc",
	[]>;
def LOOP3ri : LOOP<0b111, 0b000,
	(outs GRegs:$rlout), (ins CBOp:$cnd, GRegs:$rlin, GRegs:$ri, i16imm:$ib,
		 GRegs:$ra, brtarget:$adr),
	"loop3\t$cnd,$rlout,$ri,#$ib",
	[]>;
def LOOP3ii : LOOP<0b111, 0b000,
	(outs GRegs:$rlout), (ins CBOp:$cnd, GRegs:$rlin, i16imm:$ii, i16imm:$ib,
		 GRegs:$ra, brtarget:$adr),
	"loop3\t$cnd,$rlout,#$ii,#$ib",
	[]>;
}


//
// Pseudo Instructions
//
let Defs = [SP], Uses = [SP] in {
def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i64imm:$amt1, i64imm:$amt2),
			      "# ADJCALLSTACKDOWN $amt1, $amt2",
			      [(callseq_start timm:$amt1, timm:$amt2)]>;
def ADJCALLSTACKUP :   Pseudo<(outs), (ins i64imm:$amt1, i64imm:$amt2),
                              "# ADJCALLSTACKUP $amt1",
                              [(callseq_end timm:$amt1, timm:$amt2)]>;
}
