#include "emu/soft_machine.hpp"

namespace emu {

// Stub: the machine is not defined yet (memory map, VDC...).

void SoftMachine::reset() {}

bool SoftMachine::step_instruction(TraceRecord& /*out*/) {
  return false;
}

uint8_t SoftMachine::peek(uint32_t /*addr*/) {
  return 0;
}

} // namespace emu
