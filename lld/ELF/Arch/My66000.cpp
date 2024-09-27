//===- My66000.cpp --------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Symbols.h"
#include "Target.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {

class My66000 final : public TargetInfo {
public:
  My66000();
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
  int64_t getImplicitAddend(const uint8_t *buf, RelType type) const override;
};

} // end anonymous namespace

My66000::My66000() {
    // FIXME - what goes here?
}

RelExpr My66000::getRelExpr(RelType type, const Symbol &s,
                            const uint8_t *loc) const {
  switch (type) {
  case R_MY66000_NONE:
    return R_NONE;
  case R_MY66000_PCREL8_S2:
  case R_MY66000_PCREL16_S2:
  case R_MY66000_PCREL26_S2:
  case R_MY66000_PCREL32_S2:
  case R_MY66000_PCREL64_S2:
  case R_MY66000_PCREL32:
  case R_MY66000_PCREL64:
    return R_PC;
  default:
    return R_ABS;
  }
}

static void writeMaskedBits32le(uint8_t *p, int32_t v, uint32_t mask) {
  write32le(p, (read32le(p) & ~mask) | v);
}

static void writeMaskedBits16le(uint8_t *p, int16_t v, uint16_t mask) {
  write16le(p, (read16le(p) & ~mask) | v);
}

int64_t My66000::getImplicitAddend(const uint8_t *buf, RelType type) const {
  switch (type) {
  case R_MY66000_NONE:
  case R_MY66000_PCREL8_S2:
  case R_MY66000_PCREL16_S2:
  case R_MY66000_PCREL26_S2:
    return 0;
  case R_MY66000_PCREL32_S2:
  case R_MY66000_PCREL32:
    return SignExtend64<32>(read32le(buf));
  case R_MY66000_PCREL64_S2:
  case R_MY66000_PCREL64:
    return read64le(buf);
  default:
    internalLinkerError(getErrorLocation(buf),
                        "cannot read addend for relocation " + toString(type));
    return 0;
  }
}

void My66000::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_MY66000_PCREL8_S2:
    checkInt(loc, val, 10, rel);
    *loc = val >> 2;
    break;
  case R_MY66000_PCREL16_S2:
    checkInt(loc, val, 18, rel);
    writeMaskedBits16le(loc, (val & 0x0003FFFC) >> 2, 0x0003FFFC >> 2);
    break;
  case R_MY66000_PCREL26_S2:
    checkInt(loc, val, 28, rel);
    writeMaskedBits32le(loc, (val & 0x0FFFFFFC) >> 2, 0x0FFFFFFC >> 2);
    break;
  case R_MY66000_PCREL32_S2:
    break;
  case R_MY66000_PCREL64_S2:
    break;
  case R_MY66000_8:
    checkIntUInt(loc, val, 8, rel);
    *loc = val;
    break;
  case R_MY66000_16:
    checkIntUInt(loc, val, 16, rel);
    write16le(loc, val);
    break;
    break;
  case R_MY66000_32:
  case R_MY66000_PCREL32:
    checkIntUInt(loc, val, 32, rel);
    write32le(loc, val);
    break;
  case R_MY66000_64:
  case R_MY66000_PCREL64:
    write64le(loc, val);
    break;
  default:
    error(getErrorLocation(loc) + "unrecognized relocation " +
          toString(rel.type));
  }
}

TargetInfo *elf::getMy66000TargetInfo() {
  static My66000 target;
  return &target;
}
