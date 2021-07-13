//=- My66000CallingConv.td - Calling conventions for My66000 -*- tablegen -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for My66000 architecture.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// My66000 Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_My66000 : CallingConv<[
  // Promote i8/i16/i32 arguments to i64.
  CCIfType<[i8, i16, i32], CCPromoteToType<i64>>,

// i64,f64,f32 are returned in registers R1-R8
  CCIfType<[i64,f64,f32], CCAssignToReg<[R1, R2, R3, R4, R5, R6, R7, R8]>>,

  // Integer values get stored in stack slots that are 8 bytes in
  // size and 8-byte aligned.
  CCIfType<[i64,f64,f32], CCAssignToStack<8, 8>>
]>;

//===----------------------------------------------------------------------===//
// My66000 Argument Calling Conventions
//===----------------------------------------------------------------------===//
def CC_My66000 : CallingConv<[
 // Pass by value if the byval attribute is given
  CCIfByVal<CCPassByVal<8, 8>>,

  // Promote i8/i16/i32 arguments to i64.
  CCIfType<[i8, i16, i32], CCPromoteToType<i64>>,

  // Arguments passed registers R1-R8
  CCIfType<[i64,f64,f32], CCAssignToReg<[R1, R2, R3, R4, R5, R6, R7, R8]>>,

  // Integer values get stored in stack slots that are 8 bytes in
  // size and 8-byte aligned.
  CCIfType<[i64,f64,f32], CCAssignToStack<8, 8>>
]>;

def CSR_ABI : CalleeSavedRegs<(add R0, (sequence "R%u", 16, 30))>;
