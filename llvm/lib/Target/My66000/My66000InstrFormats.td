//===- My66000InstrFormats.td - My66000 Instruction Formats *- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/*
 * All My66000 instructions have a 6-bit major opcode in bits 26-31.
 */

class My66000Inst<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  field bits<32> Inst;
  bits<6> Opcode = 0;
  let Namespace = "My66000";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString   = asmstr;
  let Pattern = pattern;
  let Inst{31-26} = Opcode;

  // Target specific flags
  // Indicates that the result can be considered sign extended from bit 31.
  bit IsSignExtendingOp = 0;
  let TSFlags{1} = IsSignExtendingOp;
  // Indicates that the result can be considered zero extended from bit 31.
  bit IsZeroExtendingOp = 0;
  let TSFlags{2} = IsZeroExtendingOp;
  bit HasIndexReg = 0;
  let TSFlags{3} = HasIndexReg;
}

class Pseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  let isPseudo = 1;
}

// FIXME - just for RET, BRI right now
class Simple<bits<6> major, bits<26> minor, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  let Opcode = major;
  let Inst{25-0} = minor;
}

class ALU_RI<bits<6> major, dag outs, dag ins, string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<16> imm;		// 16-bit immediate
  let Opcode = major;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-0}  = imm;
  let hasSideEffects = 0;
  let Size = 4;
}
class MOV_RI<bits<6> major, bits<5> minor, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<16> imm;		// 16-bit immediate
  let Opcode = major;
  let Inst{25-21} = rd;
  let Inst{20-16} = minor;
  let Inst{15-0}  = imm;
  let hasSideEffects = 0;
  let Size = 4;
}
class XOP<bits<6>opcode, bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  let Opcode = opcode;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-9}  = minor;
  let Inst{8-5}   = flags;
  let Inst{4-0}   = rs2;
  let hasSideEffects = 0;
}

//
// XOP0 integer
//
class XOP0_RR<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let Size = 4;
}
class XOP0_RW<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 8;
}
class XOP0_RD<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 12;
}
class XOP0_WR<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 8;
}
class XOP0_DR<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 12;
}

//
// XOP1 integer
//
class XOP1_RR<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let Size = 4;
}
class XOP1_RW<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 8;
}
class XOP1_RD<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 12;
}
class XOP1_WR<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 8;
}
class XOP1_DR<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001000, minor, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 12;
}

//
// XOP3 floating point
//
class XOP3_RR<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001011, minor, flags, outs, ins, asmstr, pattern> {
  let Size = 4;
}
class XOP3_RW<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001011, minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 8;
}
class XOP3_RD<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001011, minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 12;
}
class XOP3_WR<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001011, minor, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 8;
}
class XOP3_DR<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001011, minor, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 12;
}
/* FIXME
class XOP3_RK<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001011, minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = imm;
  let Size = 4;
}
*/

//
// XOP3 CVT - FIXME experimental
//
class CVT_R<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001011, minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 4;
}
class CVT_W<bits<7> minor, bits<4>flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP<0b001011, minor, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let rs2 = 0b00000;
  let Size = 8;
}

// SHIFTS rs1,#o,#w
class SHF_RI<bits<4> minor, dag outs, dag ins, string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<6> o;
  bits<6> w;
  let Opcode = 0b110101;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-10} = w;
  let Inst{9-6} = minor;
  let Inst{5-0} = o;
  let hasSideEffects = 0;
  let Size = 4;
}

// SHIFTS rs1,#o
class XOM7_R<bits<7> minor, bits<2>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<6> o;
  let Opcode = 0b000111;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-9} = minor;
  let Inst{8-7} = flags;
  let Inst{6} = 0;
  let Inst{5-0} = o;
  let hasSideEffects = 0;
  let Size = 4;
}

// SHIFTS, rs1,rs2
class XOP0_SHF<bits<6> minor, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  let Opcode = 0b001000;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-12} = flags;
  let Inst{11-6}  = minor;
  let Inst{5}     = 0;
  let Inst{4-0}   = rs2;
  let hasSideEffects = 0;
}

// XOP6 - 1OP
class XOP6<bits<7> minor, bits<5> func, bits<4> flags, dag outs, dag ins, string asmstr,
		  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs2;		// src2 register
  let Opcode = 0b001110;
  let Inst{25-21} = rd;
  let Inst{20-16} = func;	// in rs1 position
  let Inst{15-9} =  minor;
  let Inst{8-5}   = flags;
  let Inst{4-0}   = rs2;
}
class XOP6_R<bits<7> minor, bits<5> func, bits<4> flags, dag outs, dag ins, string asmstr,
		  list<dag> pattern>
    : XOP6<minor, func, flags, outs, ins, asmstr, pattern> {
  let Size = 4;
}
class XOP6_W<bits<7> minor, bits<5> func, bits<4> flags, dag outs, dag ins, string asmstr,
		  list<dag> pattern>
    : XOP6<minor, func, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 8;
}
class XOP6_D<bits<7> minor, bits<5> func, bits<4> flags, dag outs, dag ins, string asmstr,
		  list<dag> pattern>
    : XOP6<minor, func, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 12;
}
class MUX<bits<2>size, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  bits<5> rs3;		// src3 register, the control
  let Opcode = 0b010010;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = rs3;
  let Inst{10-9}  = size;
  let Inst{8-5}   = flags;
  let Inst{4-0}   = rs2;
  let hasSideEffects = 0;
}
class MUX_RR<bits<2>size, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<size, flags, outs, ins, asmstr, pattern> {
  let Size = 4;
}
class MUX_RW<bits<2>size, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<size, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 8;
}
class MUX_RD<bits<2>size, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<size, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 12;
}
class MUX_WR<bits<2>size, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<size, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 8;
}
class MUX_DR<bits<2>size, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<size, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 12;
}

// FIXME
class INS<bits<3> minor, bits<3> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  bits<5> rs3;		// src3 register
  let Opcode = 0b001100;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-13} = flags;
  let Inst{12-10} = minor;
  let Inst{9-5}   = rs3;
  let Inst{4-0}   = rs2;
  let hasSideEffects = 0;
  let Size = 4;
}

class FMA<bits<2>minor, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  bits<5> rs3;		// src3 register, the control
  let Opcode = 0b010100;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = rs3;
  let Inst{10-9}  = minor;
  let Inst{8-5}   = flags;
  let Inst{4-0}   = rs2;
  let hasSideEffects = 0;
}

class FMA_RR<bits<2>minor, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<minor, flags, outs, ins, asmstr, pattern> {
  let Size = 4;
}
class FMA_RW<bits<2>minor, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 8;
}
class FMA_RD<bits<2>minor, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<minor, flags, outs, ins, asmstr, pattern> {
  let rs2 = 0b00000;
  let Size = 12;
}
class FMA_WR<bits<2>minor, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<minor, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 8;
}
class FMA_DR<bits<2>minor, bits<4>flags, dag outs, dag ins, string asmstr,
		list<dag> pattern>
    : MUX<minor, flags, outs, ins, asmstr, pattern> {
  let rs1 = 0b00000;
  let Size = 12;
}

class LDST_BD<bits<6> major, dag outs, dag ins, string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rb;		// base register
  bits<16> imm;		// 16-bit immediate
  let Opcode = major;
  let Inst{25-21} = rd;
  let Inst{20-16} = rb;
  let Inst{15-0}  = imm;
  let Size = 4;
}

// XOP5 LD/ST
class XOP5<bits<7> minor, bits<2> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rb;		// base register
  bits<5> rx;		// index register
  bits<2> sc;		// shift size
  let Opcode = 0b001011;
  let Inst{25-21} = rd;
  let Inst{20-16} = rb;
  let Inst{15-9}  = minor;	// includes S and size
  let Inst{8-7}   = sc;		// scale (aka shift amount)
  let Inst{6-5}   = flags;	// ID
  let Inst{4-0}   = rx;
  let HasIndexReg = 1;
}
// XOP5 MM/MS
class XOP5_RR<bits<7> minor, bits<2> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP5<minor, flags, outs, ins, asmstr, pattern> {
  let Size = 4;
}
class XOP5_RW<bits<7> minor, bits<2> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP5<minor, flags, outs, ins, asmstr, pattern> {
  let rx = 0b00000;
  let Size = 8;
}
class XOP5_RD<bits<7> minor, bits<2> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : XOP5<minor, flags, outs, ins, asmstr, pattern> {
  let rx = 0b00000;
  let Size = 12;
}

class BR<bits<6> major, dag outs, dag ins, string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<26> adr;	// 26-bit displacement
  let Opcode = major;
  let Inst{25-0}  = adr;
  let Size = 4;
}

class BC<bits<6> major, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rs1;	// src1 register
  bits<5> cc;	// condition code
  bits<16> adr;	// 16-bit immediate
  let Opcode = major;
  let Inst{25-21} = cc;
  let Inst{20-16} = rs1;
  let Inst{15-0}  = adr;
  let Size = 4;
}

class BB<bits<5> major, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rs1;	// src1 register
  bits<6> cb;	// condition bit
  bits<16> adr;	// 16-bit immediate
  let Inst{31-27} = major;
  let Inst{26-21} = cb;
  let Inst{20-16} = rs1;
  let Inst{15-0}  = adr;
  let Size = 4;
}

class PC<bits<6> major, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rs1;	// src1 register
  bits<5> cc;	// condition code
  bits<6> tn;
  bits<6> fn;
  let Opcode = major;
  let Inst{25-21} = cc;
  let Inst{20-16} = rs1;
  let Inst{15-12} = 0b0001;	// minor
  let Inst{11-6}  = tn;
  let Inst{5-0}   = fn;
  let Size = 4;
}

class PB<bits<5> major, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rs1;	// src1 register
  bits<6> cb;	// condition bit
  bits<6> tn;
  bits<6> fn;
  let Inst{31-27} = major;
  let Inst{26-21} = cb;
  let Inst{20-16} = rs1;
  let Inst{15-12} = 0b0000;	// minor
  let Inst{11-6}  = tn;
  let Inst{5-0}   = fn;
  let Size = 4;
}

class LOOP<bits<2>kind, bits<4> flags, dag outs, dag ins,
		  string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  bits<3> cnd;
  let Opcode = 0b001100;
  let Inst{25-21} = rd;		// Rloop
  let Inst{20-16} = rs1;	// Rinc or Imm
  let Inst{15-14} = kind;	// Kind
  let Inst{13-11} = cnd;	// Cnd
  let Inst{10-9}  = 0b00;	// Size
  let Inst{8-5}   = flags;
  let Inst{4-0}   = rs2;	// Rcmp or Imm
  let hasSideEffects = 0;
}
class LOOP_RR<bits<2>kind, bits<4> flags, dag outs, dag ins,
		  string asmstr, list<dag> pattern>
    : LOOP<kind, flags, outs, ins, asmstr, pattern>
{
  let Size = 4;
}
class LOOP_RW<bits<2>kind, bits<4> flags, dag outs, dag ins,
		  string asmstr, list<dag> pattern>
    : LOOP<kind, flags, outs, ins, asmstr, pattern>
{
  let rs2 = 0b00000;
  let Size = 8;
}
class LOOP_WR<bits<2>kind, bits<4> flags, dag outs, dag ins,
		  string asmstr, list<dag> pattern>
    : LOOP<kind, flags, outs, ins, asmstr, pattern>
{
  let rs1 = 0b00000;
  let Size = 8;
}
class LOOP_RD<bits<2>kind, bits<4> flags, dag outs, dag ins,
		  string asmstr, list<dag> pattern>
    : LOOP<kind, flags, outs, ins, asmstr, pattern>
{
  let rs2 = 0b00000;
  let Size = 12;
}
class LOOP_DR<bits<2>kind, bits<4> flags, dag outs, dag ins,
		  string asmstr, list<dag> pattern>
    : LOOP<kind, flags, outs, ins, asmstr, pattern>
{
  let rs1 = 0b00000;
  let Size = 12;
}
class LOOP_WW<bits<2>kind, bits<4> flags, dag outs, dag ins,
		  string asmstr, list<dag> pattern>
    : LOOP<kind, flags, outs, ins, asmstr, pattern>
{
  let rs1 = 0b00000;
  let rs2 = 0b00000;
  let Size = 12;
}
class LOOP_HH<bits<2>kind, bits<4> flags, dag outs, dag ins,
		  string asmstr, list<dag> pattern>
    : LOOP<kind, flags, outs, ins, asmstr, pattern>
{
  let rs1 = 0b00000;
  let rs2 = 0b00000;
  let Size = 8;
}



class VECF<bits<6> major, dag outs, dag ins, string asmstr,
		  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<21> imm;
  let Opcode = major;
  let Inst{25-21} = rd;
  let Inst{20-0}  = imm;
  let Size = 4;
}

class ENEX<bits<6> major, dag outs, dag ins, string asmstr,		  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rb;
  bits<13> imm;
  bits<3> flags;
  let Opcode = major;
  let Inst{25-21} = rd;
  let Inst{20-16} = rb;
  let Inst{15-3}  = imm;
  let Inst{2-0}   = flags;
  let Size = 4;
}
