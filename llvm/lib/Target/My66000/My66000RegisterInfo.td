//=- My66000RegisterInfo.td - Describe the My66000 Registers -*- tablegen -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
class My66000Reg<bits<5> enc, string n, list<string> alt = []>
    : Register<n> {
  let HWEncoding{4-0} = enc;
  let AltNames = alt;
  let Namespace = "My66000";
}

def R0	: My66000Reg<0,  "r0"  >, DwarfRegNum<[0]>;
def R1	: My66000Reg<1,  "r1"  >, DwarfRegNum<[1]>;
def R2	: My66000Reg<2,  "r2"  >, DwarfRegNum<[2]>;
def R3	: My66000Reg<3,  "r3"  >, DwarfRegNum<[3]>;
def R4	: My66000Reg<4,  "r4"  >, DwarfRegNum<[4]>;
def R5	: My66000Reg<5,  "r5"  >, DwarfRegNum<[5]>;
def R6	: My66000Reg<6,  "r6"  >, DwarfRegNum<[6]>;
def R7	: My66000Reg<7,  "r7"  >, DwarfRegNum<[7]>;
def R8	: My66000Reg<8,  "r8"  >, DwarfRegNum<[8]>;
def R9	: My66000Reg<9,  "r9"  >, DwarfRegNum<[9]>;
def R10	: My66000Reg<10, "r10" >, DwarfRegNum<[10]>;
def R11	: My66000Reg<11, "r11" >, DwarfRegNum<[11]>;
def R12	: My66000Reg<12, "r12" >, DwarfRegNum<[12]>;
def R13	: My66000Reg<13, "r13" >, DwarfRegNum<[13]>;
def R14	: My66000Reg<14, "r14" >, DwarfRegNum<[14]>;
def R15	: My66000Reg<15, "r15" >, DwarfRegNum<[15]>;
def R16	: My66000Reg<16, "r16" >, DwarfRegNum<[16]>;
def R17	: My66000Reg<17, "r17" >, DwarfRegNum<[17]>;
def R18	: My66000Reg<18, "r18" >, DwarfRegNum<[18]>;
def R19	: My66000Reg<19, "r19" >, DwarfRegNum<[19]>;
def R20	: My66000Reg<20, "r20" >, DwarfRegNum<[20]>;
def R21	: My66000Reg<21, "r21" >, DwarfRegNum<[21]>;
def R22	: My66000Reg<22, "r22" >, DwarfRegNum<[22]>;
def R23	: My66000Reg<23, "r23" >, DwarfRegNum<[23]>;
def R24	: My66000Reg<24, "r24" >, DwarfRegNum<[24]>;
def R25	: My66000Reg<25, "r25" >, DwarfRegNum<[25]>;
def R26	: My66000Reg<26, "r26" >, DwarfRegNum<[26]>;
def R27	: My66000Reg<27, "r27" >, DwarfRegNum<[27]>;
def R28	: My66000Reg<28, "r28" >, DwarfRegNum<[28]>;
def R29	: My66000Reg<29, "r29" >, DwarfRegNum<[29]>;
def R30	: My66000Reg<30, "r30" >, DwarfRegNum<[30]>;
def SP	: My66000Reg<31, "sp" >,  DwarfRegNum<[31]>;


// Register classes.
// The order of registers represents the preferred allocation sequence.
// Registers are listed in the order caller-save, callee-save, specials.
// Must list all the registers in the class.
def GRegs : RegisterClass<"My66000", [i64,f64], 64,
  (add
    R30,		// may be used as frame pointer
    R29,R28,R27,R26,R25,R24,R23,R22,R21,R20,R19,R18,R17,R16,
    (sequence "R%u",  1, 15),
    SP,			// ABI stack pointer
    R0			// hardware link register
  )>;

def FSRegs : RegisterClass<"My66000", [f32], 32,
  (add
    R30,		// may be used as frame pointer
    R29,R28,R27,R26,R25,R24,R23,R22,R21,R20,R19,R18,R17,R16,
    (sequence "R%u",  1, 15),
    SP,			// ABI stack pointer
    R0			// hardware link register
  )>;
