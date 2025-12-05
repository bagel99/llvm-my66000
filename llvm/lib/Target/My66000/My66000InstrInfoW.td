//
// 32-bit extension
//
def imm0 : Operand<i64>, ImmLeaf<i64,
		[{ return Imm == 0; }]>;

// Note: not sure why "ineg X" is not in DAG rather than "sub imm0,X"

multiclass AWOPrr<string OpName, bits<7>OpC> {
  def rr : XOP1_R<OpC, 0b0000,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,$rs2"),
	[]>;
  def rn : XOP1_R<OpC, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,$rs1,-$rs2"),
	[]>;
  def nr : XOP1_R<OpC, 0b0010,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,$rs2"),
	[]>;
  def nn : XOP1_R<OpC, 0b0011,
	(outs GRegs:$rd), (ins GRegs:$rs1, GRegs:$rs2),
	!strconcat(OpName, "\t$rd,-$rs1,-$rs2"),
	[]>;
//  def rw : XOP1_W<OpC, 0b1000,
//	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
//	!strconcat(OpName, "\t$rd,$rs1,#$imm"),
//	[]>;
}
let IsSignExtendingOp = 1 in {
defm ADDUW : AWOPrr<"addw", 0b0001001>;
defm ADDSW : AWOPrr<"addws", 0b0001101>;
defm MULUW : AWOPrr<"mulw", 0b0010001>;
defm MULSW : AWOPrr<"mulws", 0b0010101>;
defm DIVUW : AWOPrr<"divw", 0b0011001>;
defm DIVSW : AWOPrr<"divws", 0b0011101>;
defm MAXUW : AWOPrr<"maxw", 0b0110001>;
defm MAXSW : AWOPrr<"maxws", 0b0110101>;
defm MINUW : AWOPrr<"minw", 0b0111001>;
defm MINSW : AWOPrr<"minws", 0b0111101>;
}
let isCompare = 1 in {
defm CMPW  : AWOPrr<"cmpw",  0b0100001>;
}

// ADDSW
def : Pat<(sext_inreg (add GRegs:$rs1, GRegs:$rs2), i32),
	    (ADDSWrr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(sext_inreg (sub GRegs:$rs1, GRegs:$rs2), i32),
	    (ADDSWrn GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(sext_inreg (add (ineg GRegs:$rs1), GRegs:$rs2), i32),
	    (ADDSWnr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(sext_inreg (sub imm0, (add GRegs:$rs1, GRegs:$rs2)), i32),
	    (ADDSWnn GRegs:$rs1, GRegs:$rs2)>;
let IsSignExtendingOp = 1 in {
def ADDSWrw : XOP1_W<0b0001101, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"addws\t$rd,$rs1,#$imm",
	[(set GRegs:$rd, (sext_inreg (add GRegs:$rs1, simm32:$imm), i32))]>;
def ADDSWnw : XOP1_W<0b0001101, 0b1001,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs1),
	"addws\t$rd,-$rs1,#$imm",
	[(set GRegs:$rd, (sext_inreg (sub simm32:$imm, GRegs:$rs1), i32))]>;
//def ADDSWwn : XOP1_W<0b0001101, 0b1001,
//	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs1),
//	"addws\t$rd,$rs1,#$imm",
//	[(set GRegs:$rd, (sext_inreg (add simm32:$imm, (ineg GRegs:$rs1)), i32))]>;
}

// ADDUW
def : Pat<(and (add GRegs:$rs1, GRegs:$rs2), 4294967295),
	    (ADDUWrr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(and (sub GRegs:$rs1, GRegs:$rs2), 4294967295),
	    (ADDUWrn GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(and (add (ineg GRegs:$rs1), GRegs:$rs2), 4294967295),
	    (ADDUWnr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(and (sub imm0, (add GRegs:$rs1, GRegs:$rs2)), 4294967295),
	    (ADDUWnn GRegs:$rs1, GRegs:$rs2)>;
def ADDUWrw : XOP1_W<0b0001101, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"addw\t$rd,$rs1,#$imm",
	[(set GRegs:$rd, (and (add GRegs:$rs1, simm32:$imm), 4294967295))]>;
def ADDUWnw : XOP1_W<0b0001101, 0b1001,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs1),
	"addw\t$rd,-$rs1,#$imm",
	[(set GRegs:$rd, (and (sub simm32:$imm, GRegs:$rs1), 4294967295))]>;


// MULSW
def : Pat<(sext_inreg (mul GRegs:$rs1, GRegs:$rs2), i32),
	    (MULSWrr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(sext_inreg (sub imm0, (mul GRegs:$rs1, GRegs:$rs2)), i32),
	    (MULSWrn GRegs:$rs1, GRegs:$rs2)>;
let IsSignExtendingOp = 1 in {
def MULSWrw : XOP1_W<0b0010001, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"mulws\t$rd,$rs1,#$imm",
	[(set GRegs:$rd, (sext_inreg (mul GRegs:$rs1, simm32:$imm), i32))]>;
}
//def MULSWnw : XOP1_W<0b0010001, 0b0001,
//	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
//	"mulws\t$rd,-$rs1,#$imm",
//	[(set GRegs:$rd, (sext_inreg
//		(mul (and (sub imm0, GRegs:$rs1), 4294967295), simm32:$imm), i32))]>;


// MULUW
def : Pat<(and (mul GRegs:$rs1, GRegs:$rs2), 4294967295),
	    (MULUWrr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(and (sub imm0, (mul GRegs:$rs1, GRegs:$rs2)), 4294967295),
	    (MULUWrn GRegs:$rs1, GRegs:$rs2)>;
def MULUWrw : XOP1_W<0b0010001, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"mulw\t$rd,$rs1,#$imm",
	[(set GRegs:$rd, (and (mul GRegs:$rs1, simm32:$imm), 4294967295))]>;
def MULUWnw : XOP1_W<0b0010001, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"mulw\t$rd,-$rs1,#$imm",
	[(set GRegs:$rd, (mul (and (sub imm0, GRegs:$rs1), 4294967295),
		simm32:$imm))]>;


// DIVSW
def : Pat<(sext_inreg (sdiv GRegs:$rs1, GRegs:$rs2), i32),
	    (DIVSWrr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(sext_inreg (sdiv GRegs:$rs1,
		(sext_inreg (sub imm0, GRegs:$rs2), i32)), i32),
	    (DIVSWrn GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(sext_inreg (sdiv (sext_inreg (sub imm0, GRegs:$rs1), i32),
		GRegs:$rs2), i32),
	    (DIVSWnr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(sext_inreg (sdiv (sext_inreg (sub imm0, GRegs:$rs1), i32),
		(sext_inreg (sub imm0, GRegs:$rs2), i32)), i32),
	    (DIVSWnn GRegs:$rs1, GRegs:$rs2)>;
let IsSignExtendingOp = 1 in {
def DIVSWrw : XOP1_W<0b0011101, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"divws\t$rd,$rs1,#$imm",
	[(set GRegs:$rd, (sext_inreg (sdiv GRegs:$rs1, simm32:$imm), i32))]>;
def DIVSWnw : XOP1_W<0b0011101, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"divws\t$rd,-$rs1,#$imm",
	[(set GRegs:$rd, (sext_inreg (sdiv
		(sext_inreg (sub imm0, GRegs:$rs1), i32), simm32:$imm), i32))]>;
def DIVSWwr : XOP1_W<0b0011101, 0b0001,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	"divws\t$rd,#$imm,$rs2",
	[(set GRegs:$rd, (sext_inreg (sdiv simm32:$imm, GRegs:$rs2), i32))]>;
def DIVSWwn : XOP1_W<0b0011101, 0b0001,
	(outs GRegs:$rd), (ins i64imm:$imm, GRegs:$rs2),
	"divws\t$rd,#$imm,-$rs2",
	[(set GRegs:$rd, (sext_inreg (sdiv simm32:$imm,
		(sext_inreg (sub imm0, GRegs:$rs2), i32)), i32))]>;
}
//def : Pat<(sdiv (sext_inreg GRegs:$rs1, i32), simm32:$imm),
//	    (DIVSWrw GRegs:$rs1, i64imm:$imm)>;



// DIVUW
def : Pat<(and (udiv GRegs:$rs1, GRegs:$rs2), 4294967295),
	    (DIVUWrr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(and (udiv GRegs:$rs1,
		(and (sub imm0, GRegs:$rs2), 4294967295)), 4294967295),
	    (DIVUWrn GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(and (udiv (and (sub imm0, GRegs:$rs1), 4294967295),
		GRegs:$rs2), 4294967295),
	    (DIVUWnr GRegs:$rs1, GRegs:$rs2)>;
def : Pat<(and (udiv (and (sub imm0, GRegs:$rs1), 4294967295),
		(and (sub imm0, GRegs:$rs2), 4294967295)), 4294967295),
	    (DIVUWnn GRegs:$rs1, GRegs:$rs2)>;


def : Pat<(sext_inreg (smax GRegs:$rs1, GRegs:$rs2), i32),
	   (MAXSWrr GRegs:$rs1, GRegs:$rs2)>;

def : Pat<(sext_inreg (smin GRegs:$rs1, GRegs:$rs2), i32),
	   (MINSWrr GRegs:$rs1, GRegs:$rs2)>;

// CMPW
def CMPWrw : XOP1_W<0b0100000, 0b0001,
	(outs GRegs:$rd), (ins GRegs:$rs1, i64imm:$imm),
	"cmpw\t$rd,$rs1,#$imm",
	[(set GRegs:$rd, (cmp (sext_inreg GRegs:$rs1, i32), simm32:$imm))]>;
def : Pat<(cmp GRegs:$rs1, (and GRegs:$rs2, 4294967295)),
	    (CMPWrr GRegs:$rs1, GRegs:$rs2)>;


// SEXTW
def : Pat<(sext_inreg  GRegs:$rs1, i32),
	  (SRAri $rs1, 32, 0)>;

// ZEXTW
def : Pat<(and GRegs:$rs1, 4294967295),
	  (SRLri $rs1, 32, 0)>;


