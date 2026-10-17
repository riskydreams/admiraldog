// FlatRamBus: a test bus with flat RAM from address 0 and one "tohost" word.
//
// Not the admiraldog memory map: it exists so the CPU can be tested before the
// machine is specified. It also notices accesses to unmapped addresses, which
// a CPU bug often causes.
//
//   0x0000_0000 .. size-1   RAM, little-endian
//   kToHostAddr             write-only word; a 32-bit write records the value
//   anything else           reads 0, writes dropped, `stray_access` set
#pragma once

#include "emu/bus.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace emu::test {

class FlatRamBus final : public Bus {
public:
  static constexpr uint32_t kToHostAddr = 0x0001'0000;  // see tests/isa/env/link.ld

  explicit FlatRamBus(std::size_t size = 0x1'0000) : ram_(size, 0) {}

  // Copies `image` to RAM starting at `addr`. Returns false if it does not fit.
  bool load(uint32_t addr, std::span<const uint8_t> image) {
    if (addr > ram_.size() || image.size() > ram_.size() - addr) return false;
    for (std::size_t i = 0; i < image.size(); ++i) ram_[addr + i] = image[i];
    return true;
  }

  // Stores one 32-bit word, for tests that place instructions by hand.
  void poke32(uint32_t addr, uint32_t value) { store(addr, value, 4); }

  uint8_t  read8(uint32_t addr) override  { return static_cast<uint8_t>(fetch(addr, 1)); }
  uint16_t read16(uint32_t addr) override { return static_cast<uint16_t>(fetch(addr, 2)); }
  uint32_t read32(uint32_t addr) override { return fetch(addr, 4); }

  void write8(uint32_t addr, uint8_t value) override   { store(addr, value, 1); }
  void write16(uint32_t addr, uint16_t value) override { store(addr, value, 2); }
  void write32(uint32_t addr, uint32_t value) override {
    if (addr == kToHostAddr) { tohost_ = value; return; }
    store(addr, value, 4);
  }

  std::optional<uint32_t> tohost() const { return tohost_; }
  bool stray_access() const { return stray_access_; }

private:
  bool in_ram(uint32_t addr, uint32_t size) const {
    return addr < ram_.size() && size <= ram_.size() - addr;
  }

  uint32_t fetch(uint32_t addr, uint32_t size) {
    if (!in_ram(addr, size)) { stray_access_ = true; return 0; }
    uint32_t value = 0;
    for (uint32_t i = 0; i < size; ++i) value |= uint32_t{ram_[addr + i]} << (8 * i);
    return value;
  }

  void store(uint32_t addr, uint32_t value, uint32_t size) {
    if (!in_ram(addr, size)) { stray_access_ = true; return; }
    for (uint32_t i = 0; i < size; ++i) ram_[addr + i] = static_cast<uint8_t>(value >> (8 * i));
  }

  std::vector<uint8_t> ram_;
  std::optional<uint32_t> tohost_;
  bool stray_access_ = false;
};

} // namespace emu::test
