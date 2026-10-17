// Bus: how the CPU sees memory and devices.
//
// One virtual call per access. Maximum speed is a non-goal; a plain interface
// keeps the CPU easy to read, to debug and to test against a flat RAM.
//
// Contract:
//   - Addresses are byte addresses. Multi-byte values are little-endian.
//   - The CPU only issues naturally aligned 16- and 32-bit accesses; alignment
//     is the CPU's job, not the bus's.
//   - The bus never fails: an access to nothing reads some value and a write
//     to nothing is dropped. What that value is belongs to the machine spec.
#pragma once

#include <cstdint>

namespace emu {

class Bus {
public:
  virtual ~Bus() = default;

  virtual uint8_t  read8(uint32_t addr) = 0;
  virtual uint16_t read16(uint32_t addr) = 0;
  virtual uint32_t read32(uint32_t addr) = 0;

  virtual void write8(uint32_t addr, uint8_t value) = 0;
  virtual void write16(uint32_t addr, uint16_t value) = 0;
  virtual void write32(uint32_t addr, uint32_t value) = 0;
};

} // namespace emu
