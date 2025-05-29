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
class My66000ABIInfo : public ABIInfo {

public:
  My66000ABIInfo(CodeGenTypes &CGT) : ABIInfo(CGT) {}

  ABIArgInfo classifyReturnType(QualType RetTy) const;
  ABIArgInfo classifyArgumentType(QualType RetTy) const;
  bool isHomogeneousAggregateBaseType(QualType Ty) const override;
  bool isHomogeneousAggregateSmallEnough(const Type *Ty,
                                         uint64_t Members) const override;
  void computeInfo(CGFunctionInfo &FI) const override;
  Address EmitVAArg(CodeGenFunction &CGF, Address VAListAddr,
                    QualType Ty) const override;
};
} // end anonymous namespace

ABIArgInfo My66000ABIInfo::classifyArgumentType(QualType Ty) const {
  Ty = useFirstFieldIfTransparentUnion(Ty);

  if (!isAggregateTypeForABI(Ty)) {
    if (const EnumType *EnumTy = Ty->getAs<EnumType>())
      Ty = EnumTy->getDecl()->getIntegerType();

    if (const auto *EIT = Ty->getAs<BitIntType>())
      if (EIT->getNumBits() > 128)
        return getNaturalAlignIndirect(Ty);

    return ABIArgInfo::getDirect();
  }

  const Type *Base = nullptr;
  uint64_t Members = 0;
  if (isHomogeneousAggregate(Ty, Base, Members)) {
    unsigned Align =
        getContext().getTypeUnadjustedAlignInChars(Ty).getQuantity();
    Align = (Align >= 16) ? 16 : 8;
    return ABIArgInfo::getDirect(
        llvm::ArrayType::get(CGT.ConvertType(QualType(Base, 0)), Members), 0,
        nullptr, true, Align);
  }

  uint64_t Size = getContext().getTypeSize(Ty);
  // Aggregates <= 16 bytes are passed directly in registers or on the stack.
  if (Size <= 128) {
    unsigned Alignment = getContext().getTypeAlign(Ty);
    Size = llvm::alignTo(Size, Alignment);

    // We use a pair of i64 for 16-byte aggregate with 8-byte alignment.
    // For aggregates with 16-byte alignment, we use i128.
    llvm::Type *BaseTy = llvm::Type::getIntNTy(getVMContext(), Alignment);
    return ABIArgInfo::getDirect(
        Size == Alignment ? BaseTy
                          : llvm::ArrayType::get(BaseTy, Size / Alignment));
  }
  return getNaturalAlignIndirect(Ty, /*ByVal=*/false);
}

ABIArgInfo My66000ABIInfo::classifyReturnType(QualType RetTy) const {

  if (RetTy->isVoidType())
    return ABIArgInfo::getIgnore();

 if (!isAggregateTypeForABI(RetTy)) {
    // Treat an enum type as its underlying type.
    if (const EnumType *EnumTy = RetTy->getAs<EnumType>())
      RetTy = EnumTy->getDecl()->getIntegerType();

    if (const auto *EIT = RetTy->getAs<BitIntType>())
      if (EIT->getNumBits() > 128)
        return getNaturalAlignIndirect(RetTy);

    return ABIArgInfo::getDirect();
  }

  const Type *Base = nullptr;
  uint64_t Members = 0;
  if (isHomogeneousAggregate(RetTy, Base, Members))
    // Homogeneous Floating-point Aggregates (HFAs) are returned directly.
    return ABIArgInfo::getDirect();

  uint64_t Size = getContext().getTypeSize(RetTy);
  // Aggregates <= 16 bytes are returned directly in registers or on the stack.
  if (Size <= 128) {
    unsigned Alignment = getContext().getTypeAlign(RetTy);
    Size = llvm::alignTo(Size, Alignment);

    // We use a pair of i64 for 16-byte aggregate with 8-byte alignment.
    // For aggregates with 16-byte alignment, we use i128.
    if (Alignment < 128 && Size == 128) {
      llvm::Type *BaseTy = llvm::Type::getInt64Ty(getVMContext());
      return ABIArgInfo::getDirect(llvm::ArrayType::get(BaseTy, Size / 64));
    }
    return ABIArgInfo::getDirect(llvm::IntegerType::get(getVMContext(), Size));
  }

  return getNaturalAlignIndirect(RetTy);
}

bool My66000ABIInfo::isHomogeneousAggregateBaseType(QualType Ty) const {
  if (const BuiltinType *BT = Ty->getAs<BuiltinType>()) {
    if (BT->isFloatingPoint())
      return true;
  }
  return false;
}

bool My66000ABIInfo::isHomogeneousAggregateSmallEnough(const Type *Base,
                                                       uint64_t Members) const {
  return Members <= 4;
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
    return Address(CGF.Builder.CreateLoad(VAListAddr),
                   CGF.ConvertTypeForMem(Ty), SlotSize);
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
CodeGen::createMy66000TargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<My66000TargetCodeGenInfo>(CGM.getTypes());
}
