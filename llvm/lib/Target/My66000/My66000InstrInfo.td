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
def imm_su : PatLeaf<(imm), [{ return !shouldAvoidImmediate(N); }]>;
def imm1 : Operand<i64>, ImmLeaf<i64,
		[{ return Imm == 1; }]>;
def uimm16 : Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 65535; }]>;
def uimm32 : Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 4294967295; }]>;
def uneed32: Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 65536 && Imm <= 4294967295; }]>;
def uneed64: Operand<i64>, ImmLeaf<i64,
		[{ return (uint64_t)(Imm) >= 4294967296; }]>;
def simm5 :  Operand<i64>, ImmLeaf<i64,
		[{ return isInt<6>(Imm); }]>;
def simm16 : Operand<i64>, ImmLeaf<i64,
		[{ return isInt<16>(Imm); }]>;
def simm32 : Operand<i64>, ImmLeaf<i64,
		[{ return (int64_t)Imm == (int32_t)Imm; }]>;
def sneed32: Operand<i64>, ImmLeaf<i64,
		[{ return (Imm >= -2147483648 && Imm <-32768) ||
			  (Imm <=  2147483647 && Imm > 32767); }]>;
def sneed64: Operand<i64>, ImmLeaf<i64,
		[{ return (Imm < -2147483648 || Imm > 2147483647); }]>;

def shfamt : Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 63; }]>;
def shfinx: Operand<i64>, ImmLeaf<i64,
		[{ return Imm >= 0 && Imm <= 3; }]>;
def gbladr : Operand<iPTR>;
def brtarget : Operand<OtherVT>;
def calltarget : Operand<OtherVT>;
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
def SDT_CALL : 		SDTypeProfile<0, 1, [SDTCisPtrTy<0>]>;
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
                        SDTypeProfile<0, -1, [SDTCisPtrTy<0>]>,
			[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def myTAIL:	    SDNode<"My66000ISD::TAIL",
                        SDTypeProfile<0, -1, [SDTCisPtrTy<0>]>,
			[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def myCALLI :	    SDNode<"My66000ISD::CALLI", SDT_CALL,
			[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def myTAILI :	    SDNode<"My66000ISD::TAILI", SDT_CALL,
			[SDNPHasChain, SDNPOptInGlue, SDNPOutGlue, SDNPVariadic]>;
def myCMP :	    SDNode<"My66000ISD::CMP", SDTIntBinOp, []>;
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
def myCMOV:	    SDNode<"My66000ISD::CMOV",
			SDTypeProfile<1, 3,
			  [SDTCisSameAs<0, 1>,SDTCisSameAs<0, 2>,SDTCisVT<3, i64>]>,
			[]>;
def myMUX:	    SDNode<"My66000ISD::MUX",
			SDTypeProfile<1, 3,
			  [SDTCisSameAs<0, 1>,SDTCisSameAs<0, 2>,SDTCisVT<3, i64>]>,
			[]>;
def myWRAPPER:	    SDNode<"My66000ISD::WRAPPER",
			SDTypeProfile<1, 1,
			  [SDTCisSameAs<0, 1>,SDTCisPtrTy<0>]>,
			[]>;
def myMEMCPY:	    SDNode<"My66000ISD::MEMCPY",
			SDTypeProfile<0, 3,
			  [SDTCisVT<0, i64>,SDTCisVT<1, i64>,SDTCisVT<2, i64>]>,
			[SDNPHasChain, SDNPMayStore, SDNPMayLoad]>;

//
// ALU RI
//
multiclass ALUri<string OpName, bits<6>OpC, SDPatternOperator OpNode> {
  def ri : ALU_RI<OpC,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode GRegs:$rs1, simm16:$imm))]>;
}
defm ADD : ALUri<"add", 0b110001, add>;
defm MUL : ALUri<"mul", 0b110010, mul>;
defm UDIV: ALUri<"div", 0b110011, udiv>;
let isCompare = 1 in {
defm CMP : ALUri<"cmp", 0b110100, myCMP>;
}
defm OR  : ALUri<"or",  0b111000, or>;
defm XOR : ALUri<"xor", 0b111001, xor>;
defm AND : ALUri<"and", 0b111010, and>;

//
// ALU RR
//
multiclass ALUrr<string OpName, bits<6>OpC, bits<5>Flags, ValueType VT,
		   SDPatternOperator OpNode> {
  def rr : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[(set (VT GRegs:$rd), (OpNode (VT GRegs:$rs1), (VT GRegs:$rs2)))]>;
  def rn : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,-$rs2"),
	[]>;
  def nr : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,$rs2"),
	[]>;
  def nn : ALU_RR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,-$rs2"),
	[]>;
}
defm ADD  : ALUrr<"add",  0b100000, 0b00000, i64, add>;
defm MUL  : ALUrr<"mul",  0b100001, 0b00000, i64, mul>;
defm UDIV : ALUrr<"div",  0b100010, 0b00000, i64, udiv>;
defm SDIV : ALUrr<"divs", 0b100010, 0b00010, i64, sdiv>;
let isCompare = 1 in {
defm CMP  : ALUrr<"cmp",  0b100100, 0b00000, i64, myCMP>;
}
defm OR   : ALUrr<"or",   0b101000, 0b00000, i64, or>;
defm XOR  : ALUrr<"xor",  0b101001, 0b00000, i64, xor>;
defm AND  : ALUrr<"and",  0b101010, 0b00000, i64, and>;
defm UMAX : ALUrr<"max",  0b100110, 0b00000, i64, umax>;
defm UMIN : ALUrr<"min",  0b100111, 0b00000, i64, umin>;
defm SMAX : ALUrr<"maxs", 0b100110, 0b00010, i64, smax>;
defm SMIN : ALUrr<"mins", 0b100111, 0b00010, i64, smin>;
defm SLL  : ALUrr<"sll",  0b101101, 0b00000, i64, shl>;
defm SRL  : ALUrr<"srl",  0b101100, 0b00000, i64, srl>;
defm SRA  : ALUrr<"sra",  0b101100, 0b00010, i64, sra>;

def SUB   : Pat<(sub GRegs:$rs1, GRegs:$rs2),
		(ADDrn GRegs:$rs1, GRegs:$rs2)>;
def NADD  : Pat<(ineg (add GRegs:$rs1, GRegs:$rs2)),
		(ADDnn GRegs:$rs1, GRegs:$rs2)>;

// both a*-b and -a*b are expressed as -(a*b)
def       : Pat<(ineg (mul GRegs:$rs1, GRegs:$rs2)),
		(MULrn GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(udiv GRegs:$rs1, (ineg GRegs:$rs2)),
		(UDIVrn GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(udiv (ineg GRegs:$rs1), GRegs:$rs2),
		(UDIVnr GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(sdiv GRegs:$rs1, (ineg GRegs:$rs2)),
		(SDIVrn GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(sdiv (ineg GRegs:$rs1), GRegs:$rs2),
		(SDIVnr GRegs:$rs1, GRegs:$rs2)>;
def       : Pat<(sdiv (ineg GRegs:$rs1), (ineg GRegs:$rs2)),
		(SDIVnn GRegs:$rs1, GRegs:$rs2)>;
def NOR   : Pat<(not (or GRegs:$rs1, GRegs:$rs2)),
		(ANDnn GRegs:$rs1, GRegs:$rs2)>;
def ORN   : Pat<(or (not GRegs:$rs1), GRegs:$rs2),
		(ORnr GRegs:$rs1, GRegs:$rs2)>;
def NAND  : Pat<(not (and GRegs:$rs1, GRegs:$rs2)),
		(ORnn GRegs:$rs1, GRegs:$rs2)>;
def ANDN  : Pat<(and GRegs:$rs1, (not GRegs:$rs2)),
		(ANDrn GRegs:$rs1, GRegs:$rs2)>;
def XORN  : Pat<(xor (not GRegs:$rs1), GRegs:$rs2),
		(XORnr GRegs:$rs1, GRegs:$rs2)>;
//
// ALU RW - have 16-bit immediate versions
//
multiclass ALUrw<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def rw : ALU_RW<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode GRegs:$rs1, sneed32:$imm))]>;
}
defm ADD  : ALUrw<"add",  0b100000, 0b10000, add>;
defm MUL  : ALUrw<"mul",  0b100001, 0b10000, mul>;
defm UDIV : ALUrw<"div",  0b100010, 0b10000, udiv>;
defm SDIV : ALUrw<"divs", 0b100010, 0b10010, sdiv>;
let isCompare = 1 in {
defm CMP  : ALUrw<"cmp",  0b100100, 0b10000, myCMP>;
}
defm OR   : ALUrw<"or",   0b101000, 0b10000, or>;
defm XOR  : ALUrw<"xor",  0b101001, 0b10000, xor>;
defm AND  : ALUrw<"and",  0b101010, 0b10000, and>;
defm SLL  : ALUrw<"sll",  0b101101, 0b00000, shl>;
defm SRL  : ALUrw<"srl",  0b101100, 0b00000, srl>;
defm SRA  : ALUrw<"sra",  0b101100, 0b00010, sra>;

//
// ALU XR - don't have 16-bit immediate versions
// Because they commute and have a version with 5-bit immediate
// as the first operand, we generate the immediate that way.
//
multiclass ALUxr<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def xr : ALU_WR<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,#$imm,$rs1"),
	[(set GRegs:$rd, (OpNode GRegs:$rs1, simm32:$imm))]>;
}
defm UMAX : ALUxr<"max",  0b100110, 0b00000, umax>;
defm UMIN : ALUxr<"min",  0b100111, 0b00000, umin>;
defm SMAX : ALUxr<"maxs", 0b100110, 0b00010, smax>;
defm SMIN : ALUxr<"mins", 0b100111, 0b00010, smin>;

//
// ALU RX - don't have 16-bit immediate versions and don't commute
//
multiclass ALUrx<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def rx : ALU_RW<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode GRegs:$rs1, simm32:$imm))]>;
}
defm SDIV : ALUrx<"divs", 0b100010, 0b10010, sdiv>;

//
// ALU RD
//
multiclass ALUrd<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def rd : ALU_RD<OpC, Flags,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
	[(set GRegs:$rd, (OpNode GRegs:$rs1, sneed64:$imm))]>;
}
defm ADD  : ALUrd<"add",  0b100000, 0b10000, add>;
defm MUL  : ALUrd<"mul",  0b100001, 0b10000, mul>;
defm UDIV : ALUrd<"div",  0b100010, 0b00000, udiv>;
defm SDIV : ALUrd<"divs", 0b100010, 0b00010, sdiv>;
defm OR   : ALUrd<"or",   0b101000, 0b10000, or>;
defm XOR  : ALUrd<"xor",  0b101001, 0b10000, xor>;
defm AND  : ALUrd<"and",  0b101010, 0b10000, and>;
let isCompare = 1 in {
defm CMP  : ALUrd<"cmp",  0b100100, 0b10000, myCMP>;
}

//
// ALU WR
// Only non-commutative operations need apply
//
multiclass ALUwr<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def wr : ALU_WR<OpC, Flags,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,#$imm,$rs2"),
	[(set GRegs:$rd, (OpNode simm32:$imm, GRegs:$rs2))]>;
}
defm UDIV : ALUwr<"div",  0b100010, 0b10000, udiv>;
defm SDIV : ALUwr<"divs", 0b100010, 0b10010, sdiv>;
defm SLL  : ALUwr<"sll",  0b101101, 0b10000, shl>;
defm SRL  : ALUwr<"srl",  0b101100, 0b00000, srl>;
def SUBwr : ALU_WR<0b100000, 0b00100,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	"add\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (sub simm32:$imm, GRegs:$rs2))]>;

//
// ALU DR
// Only non-commutative operations need apply
//
multiclass ALUdr<string OpName, bits<6>OpC, bits<5>Flags,
		   SDPatternOperator OpNode> {
  def dr : ALU_DR<OpC, Flags,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs1),
	!strconcat(OpName, "\t$rd,#$imm,$rs1"),
	[(set GRegs:$rd, (OpNode sneed64:$imm, GRegs:$rs1))]>;
}
defm UDIV : ALUdr<"div",  0b100010, 0b10000, udiv>;
defm SDIV : ALUdr<"divs", 0b100010, 0b10010, sdiv>;
def SUBdr : ALU_DR<0b100000, 0b00100,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	"add\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (sub sneed64:$imm, GRegs:$rs2))]>;

//
// MOVs
//
// Immediate 16-bit signed extended
let isAsCheapAsAMove = 1 in {
def MOVri : ALU_RI<0b111011,
	(outs GRegs:$rd), (ins s16imm64:$imm),
	"mov\t$rd,#$imm",
	[(set GRegs:$rd, simm16:$imm)]>
	{ let rs1 = 0b00000; }
}

// Immediate 32-bit signed extended
let isAsCheapAsAMove = 1 in {
def MOVrw : ALU_RRR<0b000, 0b101,
	(outs GRegs:$rd), (ins s32imm64:$imm),
	"mov\t$rd,#$imm",
	[(set GRegs:$rd, simm32:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
}

// Immediate 64-bit
let isAsCheapAsAMove = 1 in {
def MOVrd : ALU_RRR<0b000, 0b111,
	(outs GRegs:$rd), (ins i64imm:$imm),
	"mov\t$rd,#$imm",
	[(set GRegs:$rd, sneed64:$imm)]>
	{ let rs1 = 0b00000; let rs2 = 0b00000; let rs3 = 0b00000; }
}

// Reg to Reg never matches, just built to copyPhysReg
def MOVrr : ALU_RRR<0b000, 0b010,
	(outs GRegs:$rd), (ins GRegs:$rs2),
	"mov\t$rd,$rs2",
	[]>
	{ let rs1 = 0b00000; let rs3 = 0b00000; }

def CMOVrrr : ALU_RRR<0b001, 0b001,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"cmov\t$rd,$rs3,$rs1,$rs2",
	[(set (i64 GRegs:$rd), (myCMOV GRegs:$rs1, GRegs:$rs2, GRegs:$rs3))]>;
def MUXrrr :  ALU_RRR<0b001, 0b000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2, GRegs:$rs3),
	"mux\t$rd,$rs3,$rs1,$rs2",
	[(set (i64 GRegs:$rd), (myMUX GRegs:$rs1, GRegs:$rs2, GRegs:$rs3))]>;
def MUXrrw :  ALU_RRR<0b001, 0b100,
	(outs GRegs:$rd), (ins GRegs:$rs1, s32imm64:$imm, GRegs:$rs3),
	"mux\t$rd,$rs3,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myMUX GRegs:$rs1, simm32:$imm, GRegs:$rs3))]>;
def MUXrrd :  ALU_RRR<0b001, 0b110,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm, GRegs:$rs3),
	"mux\t$rd,$rs3,$rs1,#$imm",
	[(set (i64 GRegs:$rd), (myMUX GRegs:$rs1, sneed64:$imm, GRegs:$rs3))]>;

//
// Shifts with immediate counts
//
// Generated by DAGToDAG
def SLLri : SHF_RI<0b1010,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"sll\t$rd,$rs1,<$w:$o>",
	[]>;
def SRLri : SHF_RI<0b1000,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"srl\t$rd,$rs1,<$w:$o>",
	[]>;
def SRAri : SHF_RI<0b1001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"sra\t$rd,$rs1,<$w:$o>",
	[]>;
// Patterns for simple shifts
def : Pat<(shl GRegs:$rs1, shfamt:$imm),
	  (SLLri GRegs:$rs1, 0, shfamt:$imm)>;
def : Pat<(srl GRegs:$rs1, shfamt:$imm),
	  (SRLri GRegs:$rs1, 0, shfamt:$imm)>;
def : Pat<(sra GRegs:$rs1, shfamt:$imm),
	  (SRAri GRegs:$rs1, 0, shfamt:$imm)>;
def : Pat<(myEXT GRegs:$rs1, shfamt:$imm1, shfamt:$imm2),
	  (SRLri GRegs:$rs1, shfamt:$imm1, shfamt:$imm2)>;
def : Pat<(myEXTS GRegs:$rs1, shfamt:$imm1, shfamt:$imm2),
	  (SRAri GRegs:$rs1, shfamt:$imm1, shfamt:$imm2)>;

// Generated by DAGToDAG
def INSrrw : ALU_RRR<0b010, 0b100,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs: $rs2, i32imm:$w, i32imm:$o),
	"ins\t$rd,$rs1,$rs2,#<$w:$o>",
	[]>;

//
// Misc 1 operand instructions
//
def CTLZ : R1<0b001001, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"ctlz\t$rd,$rs1",
	[(set GRegs:$rd, (ctlz GRegs:$rs1))]>;
def CTTZ : R1<0b001001, 0b00010,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cttz\t$rd,$rs1",
	[(set GRegs:$rd, (cttz GRegs:$rs1))]>;
def CTLO : R1<0b001001, 0b01000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"ctlz\t$rd,-$rs1",
	[(set GRegs:$rd, (ctlz (xor GRegs:$rs1 ,-1)))]>;
def CTTO : R1<0b001001, 0b01010,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"cttz\t$rd,-$rs1",
	[(set GRegs:$rd, (cttz (xor GRegs:$rs1 ,-1)))]>;
def POPO : R1<0b001001, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"pop\t$rd,$rs1",
	[(set GRegs:$rd, (ctpop GRegs:$rs1))]>;
def POPZ : R1<0b001001, 0b01000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"pop\t$rd,-$rs1",
	[(set GRegs:$rd, (ctpop (xor GRegs:$rs1 ,-1)))]>;
def IABS : ABS<0b000101, 0b00000, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rs1),
	"abs\t$rd,$rs1",
	[(set GRegs:$rd, (abs GRegs:$rs1))]>;

//
// Experimental BITR instruction
//
def BITR : SHF_RI<0b1100,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$w, i64imm:$o),
	"bitr\t$rd,$rs1,<$w:$o>",
	[]>;
def : Pat<(bswap GRegs:$rs1),
	  (BITR GRegs:$rs1, 8, 0)>;
def : Pat<(srl (bswap GRegs:$rs1), shfamt:$imm),
	  (BITR GRegs:$rs1, 8, shfamt:$imm)>;
def : Pat<(bitreverse GRegs:$rs1),
	  (BITR GRegs:$rs1, 1, 0)>;
def : Pat<(srl (bitreverse GRegs:$rs1), shfamt:$imm),
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
// Load
//
multiclass Load<string OpName, bits<6>OpCri, bits<6>OpCrr,
		ValueType VT, SDPatternOperator OpNode> {
  def ri : LDST_BD<OpCri,
	(outs GRegs:$rd), (ins MEMri:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(set (VT GRegs:$rd), (OpNode ADDRri:$addr))]>;
  def rr : LDST_RX<OpCrr, 0b00000,
	(outs GRegs:$rd), (ins MEMrr:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(set (VT GRegs:$rd), (OpNode ADDRrr:$addr))]>;
}
let mayLoad = 1 in {
  defm LDD  : Load<"ldd",  0b100011, 0b000011, i64, load>;
  defm LDUW : Load<"lduw", 0b100010, 0b000010, i64, zextloadi32>;
  defm LDUH : Load<"lduh", 0b100001, 0b000001, i64, zextloadi16>;
  defm LDUB : Load<"ldub", 0b100000, 0b000000, i64, zextloadi8>;
  defm LDSW : Load<"ldsw", 0b100110, 0b000110, i64, sextloadi32>;
  defm LDSH : Load<"ldsh", 0b100101, 0b000101, i64, sextloadi16>;
  defm LDSB : Load<"ldsb", 0b100100, 0b000100, i64, sextloadi8>;
}
// Map integer extload's to zextloads.
def : Pat<(i64 (extloadi32 ADDRrr:$src)), (LDUWrr ADDRrr:$src)>;
def : Pat<(i64 (extloadi16 ADDRrr:$src)), (LDUHrr ADDRrr:$src)>;
def : Pat<(i64 (extloadi8  ADDRrr:$src)), (LDUBrr ADDRrr:$src)>;
def : Pat<(i64 (extloadi32 ADDRri:$src)), (LDUWri ADDRri:$src)>;
def : Pat<(i64 (extloadi16 ADDRri:$src)), (LDUHri ADDRri:$src)>;
def : Pat<(i64 (extloadi8  ADDRri:$src)), (LDUBri ADDRri:$src)>;

//
// Load address
//
def LArr : LDST_RX<0b000111, 0b00000,
	(outs GRegs:$rd), (ins MEMrr:$addr),
	"la\t$rd,[$addr]",
	[]>;

def : Pat<(add (add GRegs:$rb, (shl GRegs:$rx, shfinx:$sc)), imm:$disp),
	  (LArr GRegs:$rb, GRegs:$rx, shfinx:$sc, imm:$disp)>;
def : Pat<(add GRegs:$rb, (shl GRegs:$rx, shfinx:$sc)),
	  (LArr GRegs:$rb, GRegs:$rx, shfinx:$sc, 0)>;
// FIXME - can the following be done with a Pat<>?
def LAg  : LDST_RX<0b000111, 0b00000,
	(outs GRegs:$rd), (ins gbladr:$addr),
	"la\t$rd,[ip,$addr]",
	[(set GRegs:$rd, (myWRAPPER tglobaladdr:$addr))]>;
def LAgx : LDST_RX<0b000111, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rx, gbladr:$addr),
	"la\t$rd,[ip,$rx,$addr]",
	[(set GRegs:$rd, (add GRegs:$rx, (myWRAPPER tglobaladdr:$addr)))]>;
def LAgxs: LDST_RX<0b000111, 0b00000,
	(outs GRegs:$rd), (ins GRegs:$rx, shfinx:$sc, gbladr:$addr),
	"la\t$rd,[ip,$rx<<$sc,$addr]",
	[(set GRegs:$rd,
	 (add (shl GRegs:$rx, shfinx:$sc), (myWRAPPER tglobaladdr:$addr)))]>;
def LAb  : LDST_RX<0b000111, 0b00000,
	(outs GRegs:$rd), (ins gbladr:$addr),
	"la\t$rd,[ip,$addr]",
	[(set GRegs:$rd, (myWRAPPER tblockaddress:$addr))]>;

//
// Store
//
multiclass Store<string OpName, bits<6>OpCri, bits<6>OpCrr,
		 ValueType VT, SDPatternOperator OpNode> {
  def ri : LDST_BD<OpCri,
	(outs), (ins GRegs:$rd, MEMri:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(OpNode (VT GRegs:$rd), ADDRri:$addr)]>;
  def rr : LDST_RX<OpCrr, 0b00000,
	(outs), (ins GRegs:$rd, MEMrr:$addr),
	!strconcat(OpName, "\t$rd,[$addr]"),
	[(OpNode (VT GRegs:$rd), ADDRrr:$addr)]>;
}
let mayStore = 1 in {
  defm STD : Store<"std", 0b101011, 0b001011, i64, store>;
  defm STW : Store<"stw", 0b101010, 0b001010, i64, truncstorei32>;
  defm STH : Store<"sth", 0b101001, 0b001001, i64, truncstorei16>;
  defm STB : Store<"stb", 0b101000, 0b001000, i64, truncstorei8>;
}

def LDM : LDST_BD<0b010011,
	(outs), (ins GRegs:$rd, GRegs:$rs, MEMrr:$addr),
	"ldm\t$rd,$rs,[$addr]",
	[]>;
def STM : LDST_BD<0b010100,
	(outs), (ins GRegs:$rd, GRegs:$rs, MEMrr:$addr),
	"stm\t$rd,$rs,[$addr]",
	[]>;

// ENTER/EXIT
let mayStore = 1 in {
def ENTER : ALU_RI<0b101100,
	(outs), (ins GRegs:$rd, GRegs:$rb, i16imm:$f, i16imm:$imm),
	"enter\t$rd,$rb,$f,$imm",
	[]>;
}
let mayLoad = 1 in {
def EXIT : ALU_RI<0b101100,
	(outs), (ins GRegs:$rd, GRegs:$rb, i16imm:$f, i16imm:$imm),
	"exit\t$rd,$rb,$f,$imm",
	[]>;
}

// Because ADDRrr rejects anything ADDRri can handle, we need a pattern for both.
multiclass StImm<string OpName, bits<6>Op, SDPatternOperator OpNode> {
  def ii : LDST_RX<Op, 0b00000,
	(outs), (ins i64imm:$imm, MEMri:$addr),
	!strconcat(OpName, "\t#$imm,[$addr]"),
	[(OpNode imm_su:$imm, ADDRri:$addr)]>;
  def ir : LDST_RX<Op, 0b00000,
	(outs), (ins i64imm:$imm, MEMrr:$addr),
	!strconcat(OpName, "\t#$imm,[$addr]"),
	[(OpNode imm_su:$imm, ADDRrr:$addr)]>;
}
let mayStore = 1 in {
  defm STD : StImm<"std", 0b011011, store>;
  defm STW : StImm<"stw", 0b011010, truncstorei32>;
  defm STH : StImm<"sth", 0b011001, truncstorei16>;
  defm STB : StImm<"stb", 0b011011, truncstorei8>;
}

//
// Floating point
//
include "My66000InstrInfoFP.td"

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

let isBranch = 1, isIndirectBranch = 1, isTerminator = 1, isBarrier = 1 in {
def BRI : Simple<0b001101, 0b00000000000100000000000000,
       (outs), (ins GRegs:$rs),
       "jmp\t$rs",
       [(brind GRegs:$rs)]>;
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

let isCall = 1, isBarrier = 1, Defs = [R0] in {
def CALL :  BR<0b011111,
	(outs), (ins calltarget:$adr),
	"call\t$adr",
	[(myCALL tglobaladdr:$adr)]>;
def CALLI : HR<0b00000, 0b10,
	(outs), (ins GRegs:$rs),
	"calli\t$rs",
	[(myCALLI i64:$rs)]>;
}
let isCall = 1, isTerminator = 1, isReturn = 1, isBarrier = 1 in {
def TAIL :  BR<0b011110,
	(outs), (ins calltarget:$adr),
	"br\t$adr",
	[(myTAIL tglobaladdr:$adr)]>;
}
def : Pat<(myCALL texternalsym:$adr), (CALL texternalsym:$adr)>;
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
def addcarry :	SDNode<"ISD::ADDCARRY", SDT_ADDCIO, []>;
def subcarry :	SDNode<"ISD::SUBCARRY", SDT_ADDCIO, []>;
def umul_lohi : SDNode<"ISD::UMUL_LOHI", SDTIntBinHiLoOp, []>;
def smul_lohi : SDNode<"ISD::SMUL_LOHI", SDTIntBinHiLoOp, []>;
def srl_parts : SDNode<"ISD::SRL_PARTS", SDT_ISHFW, []>;
def shl_parts : SDNode<"ISD::SHL_PARTS", SDT_ISHFW, []>;
def sra_parts : SDNode<"ISD::SRA_PARTS", SDT_ISHFW, []>;

let usesCustomInserter = 1 in {
def UADDOrr : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, GRegs:$rhs),
	"# UADD0rr",
	[(set GRegs:$sum, GRegs:$co, (uaddo GRegs:$lhs, GRegs:$rhs))]>;
def UADDOri : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs),
	"# UADD0ri",
	[(set GRegs:$sum, GRegs:$co, (uaddo GRegs:$lhs, simm16:$rhs))]>;
def USUBOrr : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, GRegs:$rhs),
	"# UADD0rr",
	[(set GRegs:$sum, GRegs:$co, (usubo GRegs:$lhs, GRegs:$rhs))]>;
def USUBOri : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs),
	"# UADD0ri",
	[(set GRegs:$sum, GRegs:$co, (usubo GRegs:$lhs, simm16:$rhs))]>;
def ADDCARRYrr : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, GRegs:$rhs, GRegs:$ci),
	"# ADDCARRYrr",
	[(set GRegs:$sum, GRegs:$co, (addcarry GRegs:$lhs, GRegs:$rhs, GRegs:$ci))]>;
def ADDCARRYri : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs, GRegs:$ci),
	"# ADDCARRYri",
	[(set GRegs:$sum, GRegs:$co, (addcarry GRegs:$lhs, simm16:$rhs, GRegs:$ci))]>;
def SUBCARRYrr : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, GRegs:$rhs, GRegs:$ci),
	"# SUBCARRYrr",
	[(set GRegs:$sum, GRegs:$co, (subcarry GRegs:$lhs, GRegs:$rhs, GRegs:$ci))]>;
def SUBCARRYri : Pseudo<
	(outs GRegs:$sum, GRegs:$co), (ins GRegs:$lhs, i64imm:$rhs, GRegs:$ci),
	"# ADDCARRYri",
	[(set GRegs:$sum, GRegs:$co, (subcarry GRegs:$lhs, simm16:$rhs, GRegs:$ci))]>;

def UMULHILOrr : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, GRegs:$rhs),
	"# UMULHILOrr",
	[(set GRegs:$lo, GRegs:$hi, (umul_lohi GRegs:$lhs, GRegs:$rhs))]>;
def UMULHILOri : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# UMULHILOri",
	[(set GRegs:$lo, GRegs:$hi, (umul_lohi GRegs:$lhs, simm16:$rhs))]>;
def UMULHILOrw : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# UMULHILOrw",
	[(set GRegs:$lo, GRegs:$hi, (umul_lohi GRegs:$lhs, sneed32:$rhs))]>;
def UMULHILOrd : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# UMULHILOrd",
	[(set GRegs:$lo, GRegs:$hi, (umul_lohi GRegs:$lhs, sneed64:$rhs))]>;
def SMULHILOrr : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, GRegs:$rhs),
	"# SMULHILOrr",
	[(set GRegs:$lo, GRegs:$hi, (smul_lohi GRegs:$lhs, GRegs:$rhs))]>;
def SMULHILOri : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# SMULHILOri",
	[(set GRegs:$lo, GRegs:$hi, (smul_lohi GRegs:$lhs, simm16:$rhs))]>;
def SMULHILOrw : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# SMULHILOrw",
	[(set GRegs:$lo, GRegs:$hi, (smul_lohi GRegs:$lhs, sneed32:$rhs))]>;
def SMULHILOrd : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$lhs, i64imm:$rhs),
	"# SMULHILOrd",
	[(set GRegs:$lo, GRegs:$hi, (smul_lohi GRegs:$lhs, sneed64:$rhs))]>;

def UDIVREMrr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# UDIVREMrr",
	[(set GRegs:$div, GRegs:$rem, (udivrem GRegs:$lhs, GRegs:$rhs))]>;
def UDIVREMri : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# UDIVREMri",
	[(set GRegs:$div, GRegs:$rem, (udivrem GRegs:$lhs, simm16:$rhs))]>;
def UDIVREMrw : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# UDIVREMrw",
	[(set GRegs:$div, GRegs:$rem, (udivrem GRegs:$lhs, sneed32:$rhs))]>;
def UDIVREMwr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# UDIVREMwr",
	[(set GRegs:$div, GRegs:$rem, (udivrem simm32:$lhs, GRegs:$rhs))]>;
def UDIVREMrd : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# UDIVREMrd",
	[(set GRegs:$div, GRegs:$rem, (udivrem GRegs:$lhs, sneed64:$rhs))]>;
def UDIVREMdr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# UDIVREMdr",
	[(set GRegs:$div, GRegs:$rem, (udivrem sneed64:$lhs, GRegs:$rhs))]>;

def SDIVREMrr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMrr",
	[(set GRegs:$div, GRegs:$rem, (sdivrem GRegs:$lhs, GRegs:$rhs))]>;
def SDIVREMrn : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMrn",
	[(set GRegs:$div, GRegs:$rem, (sdivrem GRegs:$lhs, (ineg GRegs:$rhs)))]>;
def SDIVREMnr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMnr",
	[(set GRegs:$div, GRegs:$rem, (sdivrem (ineg GRegs:$lhs), GRegs:$rhs))]>;
def SDIVREMnn : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, GRegs:$rhs),
	"# SDIVREMnn",
	[(set GRegs:$div, GRegs:$rem, (sdivrem (ineg GRegs:$lhs), (ineg GRegs:$rhs)))]>;
def SDIVREMrx : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# SDIVREMrw",
	[(set GRegs:$div, GRegs:$rem, (sdivrem GRegs:$lhs, simm32:$rhs))]>;
def SDIVREMwr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# SDIVREMwr",
	[(set GRegs:$div, GRegs:$rem, (sdivrem simm32:$lhs, GRegs:$rhs))]>;
def SDIVREMrd : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins GRegs:$lhs, i64imm:$rhs),
	"# SDIVREMrd",
	[(set GRegs:$div, GRegs:$rem, (sdivrem GRegs:$lhs, sneed64:$rhs))]>;
def SDIVREMdr : Pseudo<
	(outs GRegs:$div, GRegs:$rem), (ins i64imm:$lhs, GRegs:$rhs),
	"# SDIVREMdr",
	[(set GRegs:$div, GRegs:$rem, (sdivrem sneed64:$lhs, GRegs:$rhs))]>;

def SRL2rr : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$slo, GRegs:$shi, GRegs:$cnt),
	"# SRL2rr",
	[(set GRegs:$lo, GRegs:$hi, (srl_parts GRegs:$slo, GRegs:$shi, GRegs:$cnt))]>;
def SLL2rr : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$slo, GRegs:$shi, GRegs:$cnt),
	"# SHL2rr",
	[(set GRegs:$lo, GRegs:$hi, (shl_parts GRegs:$slo, GRegs:$shi, GRegs:$cnt))]>;
def SRA2rr : Pseudo<
	(outs GRegs:$lo, GRegs:$hi), (ins GRegs:$slo, GRegs:$shi, GRegs:$cnt),
	"# SRL2rr",
	[(set GRegs:$lo, GRegs:$hi, (sra_parts GRegs:$slo, GRegs:$shi, GRegs:$cnt))]>;

def ROTLrr : Pseudo<
	(outs GRegs:$dst), (ins GRegs:$lhs, GRegs:$rhs),
	"# ROTLrr",
	[(set GRegs:$dst, (rotl GRegs:$lhs, GRegs:$rhs))]>;
def ROTLri : Pseudo<
	(outs GRegs:$dst), (ins GRegs:$lhs, i64imm:$rhs),
	"# ROTLri",
	[(set GRegs:$dst, (rotl GRegs:$lhs, simm16:$rhs))]>;
def ROTRrr : Pseudo<
	(outs GRegs:$dst), (ins GRegs:$lhs, GRegs:$rhs),
	"# ROTRrr",
	[(set GRegs:$dst, (rotr GRegs:$lhs, GRegs:$rhs))]>;
def ROTRri : Pseudo<
	(outs GRegs:$dst), (ins GRegs:$lhs, i64imm:$rhs),
	"# ROTRri",
	[(set GRegs:$dst, (rotr GRegs:$lhs, simm16:$rhs))]>;
}

// Memory to memory copy
def MEMCPY : ALU_RRM<0b101, 0b000,
	(outs), (ins GRegs:$dst, GRegs:$src, GRegs:$len),
	"mm\t$len,$src,$dst",
	[(myMEMCPY GRegs:$dst, GRegs:$src, GRegs:$len)]>;

//
// Predication Instructions, never selected
//
def PRC : PC<0b000110,
	(outs), (ins ccode:$cc, GRegs:$rs1, i16imm:$inf),
	"pc\t$cc,$rs1,$inf",
	[]>;
def PRIB : PB<0b00011,
	(outs), (ins i64imm:$cb, GRegs:$rs1, i16imm:$inf),
	"pb1\t$cb,$rs1,$inf",
	[]>;
def PRFB : PB<0b00011,
	(outs), (ins i64imm:$cb, GRegs:$rs1, i16imm:$inf),
	"pb1\t$cb,$rs1,$inf",
	[]>;
def PBIT : PB<0b00011,
	(outs), (ins i64imm:$bit, GRegs:$rs1, i16imm:$inf),
	"pb1\t$bit,$rs1,$inf",
	[]>;


//
// Virtual Vector Mode (VVM) Instructions
//
def VEC : VECF<0b111101,
	(outs GRegs:$rd), (ins i16imm:$imm),
	"vec\t$rd,$imm",
	[]>;
let isBranch = 1, isTerminator = 1 in {
def LOOP1rr : ALU_RRR<0b111, 0b000,
	(outs), (ins CBOp:$cnd, GRegs:$rt, GRegs:$ri, GRegs:$rb,
		 GRegs:$ra, brtarget:$adr),
	"loop\t$cnd,$rt,$ri,$rb",
	[]>;
def LOOP1ri : ALU_RRR<0b111, 0b000,
	(outs), (ins CBOp:$cnd, GRegs:$rt, GRegs:$ri, i16imm:$ib,
		 GRegs:$ra, brtarget:$adr),
	"loop\t$cnd,$rt,$ri,#$ib",
	[]>;
def LOOP1ir : ALU_RRR<0b111, 0b000,
	(outs), (ins CBOp:$cnd, GRegs:$rt, i16imm:$ii, GRegs:$rb,
		 GRegs:$ra, brtarget:$adr),
	"loop\t$cnd,$rt,#$ii,$rb",
	[]>;
def LOOP1ii : ALU_RRR<0b111, 0b000,
	(outs), (ins CBOp:$cnd, GRegs:$rt, i16imm:$ii, i16imm:$ib,
		 GRegs:$ra, brtarget:$adr),
	"loop\t$cnd,$rt,#$ii,#$ib",
	[]>;
/*
def LOOP2 : ALU_RRR<0b111, 0b000,
	(outs), (ins CCOp:$cnd, GRegs:$rt, GRegs:$ra),
	"loop\t$cnd,$rt",
	[]>;
def LOOP2i : ALU_RRR<0b111, 0b000,
	(outs), (ins CCOp:$cnd, GRegs:$rt, i16imm:$ii, GRegs:$ra),
	"loop\t$cnd,$rt,#$ii",
	[]>;
def LOOP2r : ALU_RRR<0b111, 0b000,
	(outs), (ins CCOp:$cnd, GRegs:$rt, GRegs:$ri, GRegs:$ra),
	"loop\t$cnd,$rt,$ri",
	[]>;
*/
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
