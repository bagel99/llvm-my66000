//===-- My66000SelectionDAGInfo.cpp - My66000 SelectionDAG Info ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the My66000SelectionDAGInfo class.
//
//===----------------------------------------------------------------------===//

#include "My66000TargetMachine.h"
using namespace llvm;

#define DEBUG_TYPE "my66000-selectiondag-info"

SDValue My66000SelectionDAGInfo::EmitTargetCodeForMemcpy(
    SelectionDAG &DAG, const SDLoc &dl, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool isVolatile, bool AlwaysInline,
    MachinePointerInfo DstPtrInfo, MachinePointerInfo SrcPtrInfo) const {
LLVM_DEBUG(dbgs() << "EmitTargetCodeForMemcpy\n");
  // Ignore Align
  Dst = DAG.getNode(My66000ISD::MEMCPY, dl, MVT::Other, Chain, Dst, Src, Size);
  return Dst;
}

SDValue My66000SelectionDAGInfo::EmitTargetCodeForMemmove(
    SelectionDAG &DAG, const SDLoc &dl, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool isVolatile,
    MachinePointerInfo DstPtrInfo, MachinePointerInfo SrcPtrInfo) const {
LLVM_DEBUG(dbgs() << "EmitTargetCodeForMemmove\n");
  // Ignore Align
  Dst = DAG.getNode(My66000ISD::MEMCPY, dl, MVT::Other, Chain, Dst, Src, Size);
  return Dst;
}

SDValue My66000SelectionDAGInfo::EmitTargetCodeForMemset(
    SelectionDAG &DAG, const SDLoc &dl, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool isVolatile,
    MachinePointerInfo DstPtrInfo) const {
LLVM_DEBUG(dbgs() << "EmitTargetCodeForMemset\n");
  // Ignore Align
  Dst = DAG.getNode(My66000ISD::MEMSET, dl, MVT::Other, Chain, Dst,
		    DAG.getAnyExtOrTrunc(Src, dl, MVT::i64), Size);
  return Dst;
}
