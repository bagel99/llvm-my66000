//===-- My66000TargetInfo.h - My66000 Target Implementation ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_TARGETINFO_MY66000TARGETINFO_H
#define LLVM_LIB_TARGET_MY66000_TARGETINFO_MY66000TARGETINFO_H

namespace llvm {

class Target;

Target &getTheMy66000Target();

}

#endif
