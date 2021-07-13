//===-- My66000TargetStreamer.h - My66000 Target Streamer -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MY66000_MY66000TARGETSTREAMER_H
#define LLVM_LIB_TARGET_MY66000_MY66000TARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"

namespace llvm {
class My66000TargetStreamer : public MCTargetStreamer {
public:
  My66000TargetStreamer(MCStreamer &S);
  ~My66000TargetStreamer() override;
  virtual void emitCCTopData(StringRef Name) = 0;
  virtual void emitCCTopFunction(StringRef Name) = 0;
  virtual void emitCCBottomData(StringRef Name) = 0;
  virtual void emitCCBottomFunction(StringRef Name) = 0;
};
}

#endif
