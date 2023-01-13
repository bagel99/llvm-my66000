//===-- My66000TargetObjectFile.cpp - My66000 object files --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "My66000TargetObjectFile.h"
#include "My66000Subtarget.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;


void My66000TargetObjectFile::Initialize(MCContext &Ctx, const TargetMachine &TM){
  TargetLoweringObjectFileELF::Initialize(Ctx, TM);

  BSSSection = Ctx.getELFSection(".bss", ELF::SHT_NOBITS,
				ELF::SHF_ALLOC | ELF::SHF_WRITE);
  BSSSectionLarge = Ctx.getELFSection(".bss", ELF::SHT_NOBITS,
				ELF::SHF_ALLOC | ELF::SHF_WRITE);
  DataSection = Ctx.getELFSection(".data", ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC | ELF::SHF_WRITE);
  DataSectionLarge = Ctx.getELFSection(".data", ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC | ELF::SHF_WRITE);
  DataRelROSection = Ctx.getELFSection(".rodata", ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC | ELF::SHF_WRITE);
  DataRelROSectionLarge = Ctx.getELFSection(".rodata", ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC | ELF::SHF_WRITE);
  ReadOnlySection = Ctx.getELFSection(".rodata", ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC);
  ReadOnlySectionLarge = Ctx.getELFSection(".rodata", ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC);
  MergeableConst4Section = Ctx.getELFSection(".rodata",
				ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC | ELF::SHF_MERGE, 4);
  MergeableConst8Section = Ctx.getELFSection(".rodata",
				ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC | ELF::SHF_MERGE, 8);
  MergeableConst16Section = Ctx.getELFSection(".rodata",
				ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC | ELF::SHF_MERGE, 16);
  CStringSection = Ctx.getELFSection(".rodata",
				ELF::SHT_PROGBITS,
				ELF::SHF_ALLOC | ELF::SHF_MERGE | ELF::SHF_STRINGS);
  // TextSection       - see MObjectFileInfo.cpp
  // StaticCtorSection - see MObjectFileInfo.cpp
  // StaticDtorSection - see MObjectFileInfo.cpp
 }

static unsigned getMy66000SectionType(SectionKind K) {
  if (K.isBSS())
    return ELF::SHT_NOBITS;
  return ELF::SHT_PROGBITS;
}

static unsigned getMy66000SectionFlags(SectionKind K, bool IsCPRel) {
  unsigned Flags = 0;

  if (!K.isMetadata())
    Flags |= ELF::SHF_ALLOC;

  if (K.isText())
    Flags |= ELF::SHF_EXECINSTR;
//  else if (IsCPRel)
//    Flags |= ELF::MY66000_SHF_CP_SECTION;
//  else
 //   Flags |= ELF::MY66000_SHF_DP_SECTION;

  if (K.isWriteable())
    Flags |= ELF::SHF_WRITE;

  if (K.isMergeableCString() || K.isMergeableConst4() ||
      K.isMergeableConst8() || K.isMergeableConst16())
    Flags |= ELF::SHF_MERGE;

  if (K.isMergeableCString())
    Flags |= ELF::SHF_STRINGS;

  return Flags;
}

MCSection *My66000TargetObjectFile::getExplicitSectionGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {
  StringRef SectionName = GO->getSection();
  // Infer section flags from the section name if we can.
  bool IsCPRel = SectionName.startswith(".cp.");
  if (IsCPRel && !Kind.isReadOnly())
    report_fatal_error("Using .cp. section for writeable object.");
  return getContext().getELFSection(SectionName, getMy66000SectionType(Kind),
                                    getMy66000SectionFlags(Kind, IsCPRel));
}

MCSection *My66000TargetObjectFile::SelectSectionForGlobal(
    const GlobalObject *GO, SectionKind Kind, const TargetMachine &TM) const {

  bool UseCPRel = GO->hasLocalLinkage();

  if (Kind.isText())                    return TextSection;
  if (UseCPRel) {
    if (Kind.isMergeable1ByteCString()) return CStringSection;
    if (Kind.isMergeableConst4())       return MergeableConst4Section;
    if (Kind.isMergeableConst8())       return MergeableConst8Section;
    if (Kind.isMergeableConst16())      return MergeableConst16Section;
  }
  Type *ObjType = GO->getValueType();
  auto &DL = GO->getParent()->getDataLayout();
  if (TM.getCodeModel() == CodeModel::Small || !ObjType->isSized() ||
      DL.getTypeAllocSize(ObjType) < CodeModelLargeSize) {
    if (Kind.isReadOnly())              return UseCPRel? ReadOnlySection
                                                       : DataRelROSection;
    if (Kind.isBSS() || Kind.isCommon())return BSSSection;
    if (Kind.isData())
      return DataSection;
    if (Kind.isReadOnlyWithRel())       return DataRelROSection;
  } else {
    if (Kind.isReadOnly())              return UseCPRel? ReadOnlySectionLarge
                                                       : DataRelROSectionLarge;
    if (Kind.isBSS() || Kind.isCommon())return BSSSectionLarge;
    if (Kind.isData())
      return DataSectionLarge;
    if (Kind.isReadOnlyWithRel())       return DataRelROSectionLarge;
  }

  assert((Kind.isThreadLocal() || Kind.isCommon()) && "Unknown section kind");
  report_fatal_error("Target does not support TLS or Common sections");
}

MCSection *My66000TargetObjectFile::getSectionForConstant(const DataLayout &DL,
                                                        SectionKind Kind,
                                                        const Constant *C,
                                                        Align &Alignment) const {
  if (Kind.isMergeableConst4())           return MergeableConst4Section;
  if (Kind.isMergeableConst8())           return MergeableConst8Section;
  if (Kind.isMergeableConst16())          return MergeableConst16Section;
  assert((Kind.isReadOnly() || Kind.isReadOnlyWithRel()) &&
         "Unknown section kind");
  // We assume the size of the object is never greater than CodeModelLargeSize.
  // To handle CodeModelLargeSize changes to AsmPrinter would be required.
  return ReadOnlySection;
}
