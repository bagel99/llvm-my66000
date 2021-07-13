//===- My66000.td - Describe the My66000 Target Machine ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Descriptions
//===----------------------------------------------------------------------===//

include "My66000RegisterInfo.td"
include "My66000CallingConv.td"
include "My66000InstrInfo.td"

def My66000InstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// My66000 processors supported.
//===----------------------------------------------------------------------===//

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"generic",         []>;

//===----------------------------------------------------------------------===//
// Declare the target which we are implementing
//===----------------------------------------------------------------------===//

def My66000 : Target {
  let InstructionSet = My66000InstrInfo;
}
