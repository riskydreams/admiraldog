// Machine: the one interface between the frontend/tests and a machine core.
//
// Everything above this interface (frontend, debugger, tests) is shared.
// Everything below it exists twice: SoftMachine (C++ model, the reference) and,
// later, RtlMachine (the Verilated SystemVerilog design).
//
// Contract:
//   - Deterministic: no wall clock, no unseeded randomness, no global state.
//     Two instances must be able to run side by side in one process.
//   - No exceptions cross this interface.
//   - Kept small on purpose. It grows only when a real need appears
//     (framebuffer, ROM loading and input arrive with the machine spec).
#pragma once

#include <cstdint>

namespace emu {

// One retired instruction, as seen from outside the core.
// Both cores fill it in the same way, so lockstep can compare them field by field.
struct TraceRecord {
  uint32_t pc = 0;          // address of the retired instruction
  uint32_t insn = 0;        // raw 32-bit instruction word
  uint8_t  rd = 0;          // destination register; 0 if no register was written
                            // (a write to x0 is not a write)
  uint32_t rd_value = 0;    // value written to rd; 0 when rd == 0
  bool     mem_write = false;
  uint32_t mem_addr = 0;    // valid only when mem_write
  uint8_t  mem_size = 0;    // 1, 2 or 4 bytes; valid only when mem_write
  uint32_t mem_data = 0;    // stored bytes, zero-extended; valid only when mem_write
};

class Machine {
public:
  virtual ~Machine() = default;

  // Puts the machine in its power-on state.
  virtual void reset() = 0;

  // Executes exactly one instruction and describes it in `out`.
  // Returns false if no instruction could retire (for example an illegal
  // instruction); `out` is then left unspecified.
  virtual bool step_instruction(TraceRecord& out) = 0;

  // Reads one byte as the debugger sees it: no side effects on devices.
  virtual uint8_t peek(uint32_t addr) = 0;
};

} // namespace emu
