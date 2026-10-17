// Cpu: the RV32I hart of the software model. Instruction-accurate: step()
// executes one whole instruction; cycles are not modelled.
//
// ISA: RV32I base integer only (RISC-V Unprivileged spec, chapter "RV32I Base
// Integer Instruction Set"). No extensions: no M, no C, no Zicsr, no Zifencei.
//
// Supported (37 instructions + FENCE):
//   LUI AUIPC JAL JALR
//   BEQ BNE BLT BGE BLTU BGEU
//   LB LH LW LBU LHU  SB SH SW
//   ADDI SLTI SLTIU XORI ORI ANDI SLLI SRLI SRAI
//   ADD SUB SLL SLT SLTU XOR SRL SRA OR AND
//   FENCE: retires and does nothing (single hart, no caches).
//
// step() returns false, and changes nothing (pc, registers, memory), when the
// instruction cannot retire. With no trap machinery yet, every case where the
// spec raises an exception stops the CPU instead:
//   - illegal or unsupported encoding: every field is checked (opcode, funct3,
//     funct7, and the fixed-zero bits of shift immediates), so MUL, CSR
//     instructions, FENCE.I, 0x00000000 and 0xFFFFFFFF are rejected;
//   - ECALL and EBREAK;
//   - load or store address not aligned to its size (LH/LHU/SH: 2, LW/SW: 4);
//   - JAL, JALR or a *taken* branch whose target is not a multiple of 4.
//     JALR clears bit 0 of the target first, as the spec says.
//
// Registers: x0 always reads 0 and ignores writes. reset() clears x1..x31 to 0
// so runs are deterministic; software must not rely on it (the RTL register
// file will have no reset).
//
// TraceRecord filled by a successful step():
//   pc, insn  : address and word of the retired instruction
//   rd        : destination register, 0 if none was written (writes to x0,
//               stores, branches, FENCE)
//   rd_value  : value written to rd, 0 when rd == 0
//   mem_write : true for SB/SH/SW only
//   mem_addr  : store address
//   mem_size  : 1, 2 or 4 bytes
//   mem_data  : the stored bytes, zero-extended (SB of 0x12345678 gives 0x78)
//   Fields not listed for an instruction are 0 / false.
#pragma once

#include "emu/bus.hpp"
#include "emu/machine.hpp"

#include <array>
#include <cstdint>

namespace emu {

class Cpu {
public:
  explicit Cpu(Bus& bus);

  // Sets pc and clears x1..x31. `pc` must be a multiple of 4.
  void reset(uint32_t pc);

  // Executes one instruction. See the header comment for when it returns false.
  bool step(TraceRecord& out);

  uint32_t pc() const;

  // Register access for tests and the debugger. Index 0..31.
  uint32_t reg(uint8_t index) const;
  void set_reg(uint8_t index, uint32_t value);   // writes to x0 are ignored

private:
  // Starting state; add what your implementation needs.
  Bus& bus_;
  uint32_t pc_ = 0;
  std::array<uint32_t, 32> regs_{};
};

} // namespace emu
