#include "emu/cpu.hpp"

namespace emu {

// Stub: the implementation is yours. See cpu.hpp for the contract.

Cpu::Cpu(Bus& bus) : bus_(bus) {}

void Cpu::reset(uint32_t /*pc*/) {}

bool Cpu::step(TraceRecord& /*out*/) {
  return false;
}

uint32_t Cpu::pc() const {
  return 0;
}

uint32_t Cpu::reg(uint8_t /*index*/) const {
  return 0;
}

void Cpu::set_reg(uint8_t /*index*/, uint32_t /*value*/) {}

} // namespace emu
