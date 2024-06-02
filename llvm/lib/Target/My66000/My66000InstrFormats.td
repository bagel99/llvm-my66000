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
}

class Pseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  let isPseudo = 1;
}

// FIXME - just for RET right now
class Simple<bits<6> major, bits<26> minor, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  let Opcode = major;
  let Inst{25-0} = minor;
}

class ALU_RR<bits<6> minor, bits<5> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  let Opcode = 0b001010;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = flags;
  let Inst{10-5}  = minor;
  let Inst{4-0}   = rs2;
  let Size = 4;
}

class ALU_RW<bits<6> minor, bits<5> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  let Opcode = 0b001010;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = !or(flags,0b10000);
  let Inst{10-5}  = minor;
  let Inst{4-0}   = rs2;
  let Size = 8;
}

class ALU_RD<bits<6> minor, bits<5> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  let Opcode = 0b001010;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = !or(flags,0b10001);
  let Inst{10-5}  = minor;
  let Inst{4-0}   = rs2;
  let Size = 12;
}

class ALU_WR<bits<6> minor, bits<5> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  let Opcode = 0b001010;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = !or(flags,0b10000);
  let Inst{10-5}  = minor;
  let Inst{4-0}   = rs2;
  let Size = 8;
}

class ALU_DR<bits<6> minor, bits<5> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  let Opcode = 0b001011;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = !or(flags,0b10001);
  let Inst{10-5}  = minor;
  let Inst{4-0}   = rs2;
  let Size = 12;
}

class ALU_RRR<bits<3> minor, bits<3> flags, dag outs, dag ins, string asmstr,
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
  let Size = 4;
}

class ALU_RRM<bits<3> minor, bits<3> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  bits<5> rs3;		// src3 register
  let Opcode = 0b001001;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-13} = flags;
  let Inst{12-10} = minor;
  let Inst{9-5}   = rs3;
  let Inst{4-0}   = rs2;
  let Size = 4;
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
  let Size = 4;
}

class SHF_RI<bits<4> minor, dag outs, dag ins, string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<6> o;
  bits<6> w;
  let Opcode = 0b000111;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-12} = minor;
  let Inst{11-6} = w;
  let Inst{5-0} = o;
  let Size = 4;
}

class SHF_RR<bits<4> minor, dag outs, dag ins, string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  bits<6> w;
  let Opcode = 0b000111;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-12} = minor;
  let Inst{11-6} = w;
  let Inst{5} = 0;
  let Inst{4-0} = rs1;
  let Size = 4;
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

class LDST_RX<bits<6> minor, bits<5> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rb;		// base register
  bits<5> rx;		// index register
  let Opcode = 0b001001;
  let Inst{25-21} = rd;
  let Inst{20-16} = rb;
  let Inst{15-11} = flags;
  let Inst{10-5}  = minor;
  let Inst{4-0}   = rx;
}

class BR<bits<6> major, dag outs, dag ins, string asmstr, list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<26> imm;	// 26-bit displacement
  let Opcode = major;
  let Inst{25-0}  = imm;
}

class BC<bits<6> major, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rs1;	// src1 register
  bits<5> cc;	// condition code
  bits<16> imm;	// 16-bit immediate
  let Opcode = major;
  let Inst{25-21} = cc;
  let Inst{20-16} = rs1;
  let Inst{15-0}  = imm;
}

class BB<bits<5> major, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rs1;	// src1 register
  bits<6> cb;	// condition bit
  bits<16> imm;	// 16-bit immediate
  let Inst{31-27} = major;
  let Inst{26-21} = cb;
  let Inst{20-16} = rs1;
  let Inst{15-0}  = imm;
}

class PC<bits<6> major, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rs1;	// src1 register
  bits<5> cc;	// condition code
  bits<12> imm;	// 12-bit immediate
  let Opcode = major;
  let Inst{25-21} = cc;
  let Inst{20-16} = rs1;
  let Inst{15-12} = 0b0001;	// minor
  let Inst{11-0}  = imm;
}

class PB<bits<5> major, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rs1;	// src1 register
  bits<6> cb;	// condition bit
  bits<12> imm;	// 12-bit immediate
  let Inst{31-27} = major;
  let Inst{26-21} = cb;
  let Inst{20-16} = rs1;
  let Inst{15-12} = 0b0000;	// minor
  let Inst{11-0}  = imm;
}

class LOOP_RRR<bits<2> kind, bits<4> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> cnd;		// condition
  bits<5> rs1;		// src1 register
  bits<5> rs2;		// src2 register
  bits<5> rs3;		// src3 register
  let Opcode = 0b010101;
  let Inst{25-21} = cnd;
  let Inst{20-16} = rs1;
  let Inst{15-12} = flags;
  let Inst{11-10} = kind;
  let Inst{9-5}   = rs3;
  let Inst{4-0}   = rs2;
  let Size = 4;
}

class HR<bits<5> minor, bits<2> rw, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  let Opcode = 0b001101;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15}    = 0;
  let Inst{14-13} = rw;
  let Inst{12-5}  = 0b00000000;
  let Inst{4-0}   = minor;
}

class R1<bits<6> minor, bits<5> flags, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  let Opcode = 0b001101;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = flags;
  let Inst{10-5}  = minor;
  let Inst{4-0}   = 0b00000;
}

class CVT<bits<6> func, bits<5> flags, dag outs, dag ins, string asmstr,
		  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  let Opcode = 0b001101;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = flags;
  let Inst{10-6}  = 0b01000;
  let Inst{5-0}   = func;
}

class RND<bits<5> func, bits<5> flags, dag outs, dag ins, string asmstr,
		  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  let Opcode = 0b001101;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = flags;
  let Inst{10-5}  = 0b010100;
  let Inst{4-0}   = func;
}

class TRAN<bits<6> func, bits<5> flags, dag outs, dag ins, string asmstr,
		  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  let Opcode = 0b001101;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = flags;
  let Inst{10-6}  = 0b11000;
  let Inst{5-0}   = func;
}

class ABS<bits<6> minor, bits<5> func, bits<5> flags, dag outs, dag ins, string asmstr,
		  list<dag> pattern>
    : My66000Inst<outs, ins, asmstr, pattern> {
  bits<5> rd;		// dst register
  bits<5> rs1;		// src1 register
  let Opcode = 0b001101;
  let Inst{25-21} = rd;
  let Inst{20-16} = rs1;
  let Inst{15-11} = flags;
  let Inst{10-5}  = minor;
  let Inst{4-0}   = func;
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
