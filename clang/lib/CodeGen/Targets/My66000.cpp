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

  bool isPromotableTypeForABI(QualType Ty) const;
  ABIArgInfo classifyReturnType(QualType RetTy) const;
  ABIArgInfo classifyArgumentType(QualType Ty) const;
  bool isHomogeneousAggregateBaseType(QualType Ty) const override;
  bool isHomogeneousAggregateSmallEnough(const Type *Ty,
                                         uint64_t Members) const override;
  void computeInfo(CGFunctionInfo &FI) const override;
  RValue EmitVAArg(CodeGenFunction &CGF, Address VAListAddr, QualType Ty,
		   AggValueSlot Slot) const override;
};
} // end anonymous namespace

// Return true if the ABI requires Ty to be passed sign- or zero-
// extended to 64 bits.
bool
My66000ABIInfo::isPromotableTypeForABI(QualType Ty) const {
  // Treat an enum type as its underlying type.
  if (const auto *ED = Ty->getAsEnumDecl())
    Ty = ED->getIntegerType();

  // Promotable integer types are required to be promoted by the ABI.
  if (isPromotableIntegerTypeForABI(Ty))
    return true;

  // In addition to the usual promotable integer types, we also need to
  // extend all 32-bit types, since the ABI requires promotion to 64 bits.
  if (const BuiltinType *BT = Ty->getAs<BuiltinType>())
    switch (BT->getKind()) {
    case BuiltinType::Int:
    case BuiltinType::UInt:
      return true;
    default:
      break;
    }

  if (const auto *EIT = Ty->getAs<BitIntType>())
    if (EIT->getNumBits() < 64)
      return true;

  return false;
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


ABIArgInfo My66000ABIInfo::classifyArgumentType(QualType Ty) const {
  Ty = useFirstFieldIfTransparentUnion(Ty);

  if (Ty->isAnyComplexType())
    return ABIArgInfo::getDirect();

  if (const auto *EIT = Ty->getAs<BitIntType>())
    if (EIT->getNumBits() > 128)
      return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace(),
				     /*ByVal=*/true);

  if (isAggregateTypeForABI(Ty)) {
    // FIXME CXXABI
    uint64_t ABIAlign = 8;	// FIXME - is this correct?
    uint64_t TyAlign = getContext().getTypeAlignInChars(Ty).getQuantity();

    const Type *Base = nullptr;
    uint64_t Members = 0;
    if (isHomogeneousAggregate(Ty, Base, Members)) {
      llvm::Type *BaseTy = CGT.ConvertType(QualType(Base, 0));
      llvm::Type *CoerceTy = llvm::ArrayType::get(BaseTy, Members);
      return ABIArgInfo::getDirect(CoerceTy);
    }

    uint64_t Bits = getContext().getTypeSize(Ty);
    if (Bits > 0 && Bits <= 8 * 64) {
      llvm::Type *CoerceTy;

      // Types up to 8 bytes are passed as integer type (which will be
      // properly aligned in the argument save area doubleword).
      if (Bits <= 64)
        CoerceTy =
            llvm::IntegerType::get(getVMContext(), llvm::alignTo(Bits, 8));
      // Larger types are passed as arrays, with the base type selected
      // according to the required alignment in the save area.
      else {
        uint64_t RegBits = ABIAlign * 8;
        uint64_t NumRegs = llvm::alignTo(Bits, RegBits) / RegBits;
        llvm::Type *RegTy = llvm::IntegerType::get(getVMContext(), RegBits);
        CoerceTy = llvm::ArrayType::get(RegTy, NumRegs);
      }
      return ABIArgInfo::getDirect(CoerceTy);
    }

    // All other aggregates are passed ByVal.
    return ABIArgInfo::getIndirect(
        CharUnits::fromQuantity(ABIAlign),
        /*AddrSpace=*/getDataLayout().getAllocaAddrSpace(),
        /*ByVal=*/true, /*Realign=*/TyAlign > ABIAlign);
   }
  return (isPromotableTypeForABI(Ty)
              ? ABIArgInfo::getExtend(Ty, CGT.ConvertType(Ty))
              : ABIArgInfo::getDirect());
}

ABIArgInfo My66000ABIInfo::classifyReturnType(QualType RetTy) const {

  if (RetTy->isVoidType())
    return ABIArgInfo::getIgnore();

  // The rules for return and argument types are the same, so defer to
  // classifyArgumentType.
  return classifyArgumentType(RetTy);
}

void My66000ABIInfo::computeInfo(CGFunctionInfo &FI) const {

  FI.getReturnInfo() = classifyReturnType(FI.getReturnType());

  bool IsRetIndirect = FI.getReturnInfo().getKind() == ABIArgInfo::Indirect;

  for (auto &Arg : FI.arguments())
    Arg.info = classifyArgumentType(Arg.type);
}

RValue My66000ABIInfo::EmitVAArg(CodeGenFunction &CGF, Address VAListAddr,
                                QualType Ty, AggValueSlot Slot) const {
  CharUnits SlotSize = CharUnits::fromQuantity(8);

  // Empty records are ignored for parameter passing purposes.
  if (isEmptyRecord(getContext(), Ty, true))
    return Slot.asRValue();

  auto TInfo = getContext().getTypeInfoInChars(Ty);

  // Arguments bigger than 2*Xlen bytes are passed indirectly.
  bool IsIndirect = TInfo.Width > 2 * SlotSize;

  return emitVoidPtrVAArg(CGF, VAListAddr, Ty, IsIndirect, TInfo,
                          SlotSize, /*AllowHigherAlign=*/true, Slot);
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
