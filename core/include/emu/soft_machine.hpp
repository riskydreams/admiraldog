// SoftMachine: the software model of admiraldog, the reference ("oracle")
// that the RTL is checked against.
//
// Instruction-accurate: each step_instruction() retires one whole instruction;
// cycles inside an instruction are not modelled.
//
// Components (CPU, bus, VDC...) are plain classes owned by value; the bus holds
// references to its devices, nothing is shared-owned. See machine.hpp for the
// interface contract.
#pragma once

#include "emu/machine.hpp"

namespace emu {

class SoftMachine final : public Machine {
public:
  void reset() override;
  bool step_instruction(TraceRecord& out) override;
  uint8_t peek(uint32_t addr) override;
};

} // namespace emu
