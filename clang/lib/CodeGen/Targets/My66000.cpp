//===- My66000.cpp --------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "TargetInfo.h"

using namespace clang;
using namespace clang::CodeGen;

//===----------------------------------------------------------------------===//
// My66000 ABI Implementation
//===----------------------------------------------------------------------===//

namespace {
class My66000ABIInfo : public DefaultABIInfo {
public:
  My66000ABIInfo(CodeGenTypes &CGT) : DefaultABIInfo(CGT) {}

private:
  ABIArgInfo classifyReturnType(QualType RetTy) const;
  ABIArgInfo classifyArgumentType(QualType RetTy) const;
  void computeInfo(CGFunctionInfo &FI) const override;
public:
  Address EmitVAArg(CodeGenFunction &CGF, Address VAListAddr,
                    QualType Ty) const override;
};
} // end anonymous namespace

ABIArgInfo My66000ABIInfo::classifyArgumentType(QualType Ty) const {
  Ty = useFirstFieldIfTransparentUnion(Ty);
  if (Ty->isAnyComplexType())
    return ABIArgInfo::getDirect();
  uint64_t Size = getContext().getTypeSize(Ty);
  if (!isAggregateTypeForABI(Ty)) {
    if (const EnumType *EnumTy = Ty->getAs<EnumType>())
      Ty = EnumTy->getDecl()->getIntegerType();
    if (Size < 64 && Ty->isIntegerType())
      return ABIArgInfo::getExtend(Ty);
    return DefaultABIInfo::classifyReturnType(Ty);
  }
  // Aggregates which are <= 128 are passed in registers if possible
  if (Size <= 128) {
    unsigned Alignment = getContext().getTypeAlign(Ty);
    if (Size <= 64) {
      return ABIArgInfo::getDirect(
          llvm::IntegerType::get(getVMContext(), 64));
    } else if (Alignment == 128) {
      return ABIArgInfo::getDirect(
          llvm::IntegerType::get(getVMContext(), 128));
    } else {
      return ABIArgInfo::getDirect(llvm::ArrayType::get(
          llvm::IntegerType::get(getVMContext(), 64), 2));
    }
  }
  return getNaturalAlignIndirect(Ty, /*ByVal=*/false);
}

ABIArgInfo My66000ABIInfo::classifyReturnType(QualType Ty) const {

  return My66000ABIInfo::classifyArgumentType(Ty);
}


void My66000ABIInfo::computeInfo(CGFunctionInfo &FI) const {
  FI.getReturnInfo() = classifyReturnType(FI.getReturnType());
  for (auto &Arg : FI.arguments())
    Arg.info = classifyArgumentType(Arg.type);
}

Address My66000ABIInfo::EmitVAArg(CodeGenFunction &CGF, Address VAListAddr,
                                QualType Ty) const {
  CharUnits SlotSize = CharUnits::fromQuantity(8);

  // Empty records are ignored for parameter passing purposes.
  if (isEmptyRecord(getContext(), Ty, true)) {
    Address Addr = Address(CGF.Builder.CreateLoad(VAListAddr),
                           getVAListElementType(CGF), SlotSize);
    Addr = CGF.Builder.CreateElementBitCast(Addr, CGF.ConvertTypeForMem(Ty));
    return Addr;
  }

  auto TInfo = getContext().getTypeInfoInChars(Ty);

  // Arguments bigger than 2*Xlen bytes are passed indirectly.
  bool IsIndirect = TInfo.Width > 2 * SlotSize;

  return emitVoidPtrVAArg(CGF, VAListAddr, Ty, IsIndirect, TInfo,
                          SlotSize, /*AllowHigherAlign=*/true);
}


namespace {
class My66000TargetCodeGenInfo : public TargetCodeGenInfo {
public:
  My66000TargetCodeGenInfo(CodeGenTypes &CGT)
      : TargetCodeGenInfo(std::make_unique<My66000ABIInfo>(CGT)) {}
  // My66000 ABI requires the arguments of variadic and prototype-less functions
  // are passed in both registers and memory.
  bool isNoProtoCallVariadic(const CallArgList &args,
                             const FunctionNoProtoType *fnType) const override {
    return true;
  }
};
} // end anonymous namespace

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createMy66000TargetCodeGenInfo(CodeGenModule &CGM, unsigned GRLen,
                                          unsigned FLen) {
  return std::make_unique<My66000TargetCodeGenInfo>(CGM.getTypes(), GRLen,
                                                      FLen);
}
