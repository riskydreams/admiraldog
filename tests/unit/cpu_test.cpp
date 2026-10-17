// Cpu unit tests: what riscv-tests does not check.
//
// riscv-tests (tests/isa) checks the result of every RV32I instruction. These
// tests check the rest of the contract in cpu.hpp:
//   - the TraceRecord each instruction produces (lockstep compares it with the RTL);
//   - instructions that must not retire, and that they change nothing;
//   - misaligned data accesses and jump/branch targets.
#include <doctest/doctest.h>

#include "emu/cpu.hpp"
#include "flat_ram_bus.hpp"

#include <array>
#include <cstdint>

namespace {

// ---- Instruction encoders (RISC-V Unprivileged spec, "Base Instruction Formats")

constexpr uint32_t u(int32_t v) { return static_cast<uint32_t>(v); }

constexpr uint32_t r_type(uint32_t f7, uint32_t rs2, uint32_t rs1, uint32_t f3, uint32_t rd,
                          uint32_t op) {
  return f7 << 25 | rs2 << 20 | rs1 << 15 | f3 << 12 | rd << 7 | op;
}
constexpr uint32_t i_type(int32_t imm, uint32_t rs1, uint32_t f3, uint32_t rd, uint32_t op) {
  return (u(imm) & 0xFFF) << 20 | rs1 << 15 | f3 << 12 | rd << 7 | op;
}
constexpr uint32_t s_type(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t f3, uint32_t op) {
  return (u(imm) >> 5 & 0x7F) << 25 | rs2 << 20 | rs1 << 15 | f3 << 12 | (u(imm) & 0x1F) << 7 | op;
}
constexpr uint32_t b_type(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t f3) {
  return (u(imm) >> 12 & 1) << 31 | (u(imm) >> 5 & 0x3F) << 25 | rs2 << 20 | rs1 << 15 |
         f3 << 12 | (u(imm) >> 1 & 0xF) << 8 | (u(imm) >> 11 & 1) << 7 | 0x63;
}
constexpr uint32_t j_type(int32_t imm, uint32_t rd) {
  return (u(imm) >> 20 & 1) << 31 | (u(imm) >> 1 & 0x3FF) << 21 | (u(imm) >> 11 & 1) << 20 |
         (u(imm) >> 12 & 0xFF) << 12 | rd << 7 | 0x6F;
}

constexpr uint32_t addi(uint32_t rd, uint32_t rs1, int32_t imm) { return i_type(imm, rs1, 0, rd, 0x13); }
constexpr uint32_t add(uint32_t rd, uint32_t rs1, uint32_t rs2) { return r_type(0, rs2, rs1, 0, rd, 0x33); }
constexpr uint32_t lb(uint32_t rd, uint32_t rs1, int32_t imm) { return i_type(imm, rs1, 0, rd, 0x03); }
constexpr uint32_t lh(uint32_t rd, uint32_t rs1, int32_t imm) { return i_type(imm, rs1, 1, rd, 0x03); }
constexpr uint32_t lw(uint32_t rd, uint32_t rs1, int32_t imm) { return i_type(imm, rs1, 2, rd, 0x03); }
constexpr uint32_t lhu(uint32_t rd, uint32_t rs1, int32_t imm) { return i_type(imm, rs1, 5, rd, 0x03); }
constexpr uint32_t sb(uint32_t rs2, uint32_t rs1, int32_t imm) { return s_type(imm, rs2, rs1, 0, 0x23); }
constexpr uint32_t sh(uint32_t rs2, uint32_t rs1, int32_t imm) { return s_type(imm, rs2, rs1, 1, 0x23); }
constexpr uint32_t sw(uint32_t rs2, uint32_t rs1, int32_t imm) { return s_type(imm, rs2, rs1, 2, 0x23); }
constexpr uint32_t beq(uint32_t rs1, uint32_t rs2, int32_t imm) { return b_type(imm, rs2, rs1, 0); }
constexpr uint32_t bne(uint32_t rs1, uint32_t rs2, int32_t imm) { return b_type(imm, rs2, rs1, 1); }
constexpr uint32_t jal(uint32_t rd, int32_t imm) { return j_type(imm, rd); }
constexpr uint32_t jalr(uint32_t rd, uint32_t rs1, int32_t imm) { return i_type(imm, rs1, 0, rd, 0x67); }

// Known encodings (llvm-mc --triple=riscv32 -show-encoding) pin the encoders down.
static_assert(add(3, 1, 2) == 0x002081B3);      // add  x3, x1, x2
static_assert(addi(1, 0, 5) == 0x00500093);     // addi x1, x0, 5
static_assert(sw(2, 1, 4) == 0x0020A223);       // sw   x2, 4(x1)
static_assert(lw(3, 1, -4) == 0xFFC0A183);      // lw   x3, -4(x1)
static_assert(beq(1, 2, -8) == 0xFE208CE3);     // beq  x1, x2, -8
static_assert(jal(1, 2048) == 0x001000EF);      // jal  x1, 2048
static_assert(jalr(1, 5, -1) == 0xFFF280E7);    // jalr x1, -1(x5)

// ---- Test rig

// x1..x31 hold a recognisable pattern: x5 = 0x05050505, x31 = 0x1F1F1F1F.
constexpr uint32_t pattern(uint32_t i) { return 0x01010101u * i; }

constexpr uint32_t kProgram = 0x100;   // where the instruction under test sits

struct Rig {
  emu::test::FlatRamBus bus;
  emu::Cpu cpu{bus};
  emu::TraceRecord trace;

  Rig() {
    cpu.reset(kProgram);
    for (uint8_t i = 1; i < 32; ++i) cpu.set_reg(i, pattern(i));
    REQUIRE(cpu.pc() == kProgram);
    REQUIRE(cpu.reg(5) == pattern(5));
    REQUIRE(cpu.reg(31) == pattern(31));
  }

  // Places `insn` at the program address and executes it.
  bool exec(uint32_t insn) {
    bus.poke32(kProgram, insn);
    return cpu.step(trace);
  }

  std::array<uint32_t, 32> regs() const {
    std::array<uint32_t, 32> r{};
    for (uint8_t i = 0; i < 32; ++i) r[i] = cpu.reg(i);
    return r;
  }
};

} // namespace

// ---- State access

TEST_CASE("cpu: reset sets pc and clears x1..x31") {
  emu::test::FlatRamBus bus;
  emu::Cpu cpu{bus};
  cpu.set_reg(7, 0x1234);
  cpu.reset(0x40);
  CHECK(cpu.pc() == 0x40);
  for (uint8_t i = 0; i < 32; ++i) CHECK(cpu.reg(i) == 0);
}

TEST_CASE("cpu: x0 reads 0 and ignores set_reg") {
  Rig rig;
  rig.cpu.set_reg(0, 0xFFFFFFFF);
  CHECK(rig.cpu.reg(0) == 0);
}

TEST_CASE("cpu: two CPUs do not share state") {
  emu::test::FlatRamBus bus_a, bus_b;
  emu::Cpu a{bus_a}, b{bus_b};
  a.reset(0);
  b.reset(0);
  a.set_reg(1, 111);
  b.set_reg(1, 222);
  CHECK(a.reg(1) == 111);
  CHECK(b.reg(1) == 222);
}

// ---- TraceRecord of retired instructions

TEST_CASE("cpu: ADDI trace names the written register and value") {
  Rig rig;
  REQUIRE(rig.exec(addi(1, 0, 5)));
  CHECK(rig.trace.pc == kProgram);
  CHECK(rig.trace.insn == addi(1, 0, 5));
  CHECK(rig.trace.rd == 1);
  CHECK(rig.trace.rd_value == 5);
  CHECK_FALSE(rig.trace.mem_write);
  CHECK(rig.cpu.pc() == kProgram + 4);
  CHECK(rig.cpu.reg(1) == 5);
}

TEST_CASE("cpu: a write to x0 is reported as no write") {
  Rig rig;
  REQUIRE(rig.exec(addi(0, 5, 1)));
  CHECK(rig.trace.rd == 0);
  CHECK(rig.trace.rd_value == 0);
  CHECK(rig.cpu.reg(0) == 0);
}

TEST_CASE("cpu: trace fields from a previous step do not leak") {
  Rig rig;
  rig.trace.rd = 9;
  rig.trace.rd_value = 0xDEAD;
  rig.trace.mem_write = true;
  rig.trace.mem_addr = 0x1;
  rig.trace.mem_size = 4;
  rig.trace.mem_data = 0xBEEF;
  REQUIRE(rig.exec(beq(1, 2, 8)));   // not taken; writes nothing at all
  CHECK(rig.trace.rd == 0);
  CHECK(rig.trace.rd_value == 0);
  CHECK_FALSE(rig.trace.mem_write);
  CHECK(rig.trace.mem_addr == 0);
  CHECK(rig.trace.mem_size == 0);
  CHECK(rig.trace.mem_data == 0);
}

TEST_CASE("cpu: store traces address, size and zero-extended data") {
  Rig rig;
  rig.cpu.set_reg(1, 0x200);
  rig.cpu.set_reg(2, 0xDEADBEEF);

  SUBCASE("SW") {
    REQUIRE(rig.exec(sw(2, 1, 4)));
    CHECK(rig.trace.mem_addr == 0x204);
    CHECK(rig.trace.mem_size == 4);
    CHECK(rig.trace.mem_data == 0xDEADBEEF);
    CHECK(rig.bus.read32(0x204) == 0xDEADBEEF);
  }
  SUBCASE("SH") {
    REQUIRE(rig.exec(sh(2, 1, -2)));
    CHECK(rig.trace.mem_addr == 0x1FE);
    CHECK(rig.trace.mem_size == 2);
    CHECK(rig.trace.mem_data == 0xBEEF);
    CHECK(rig.bus.read16(0x1FE) == 0xBEEF);
  }
  SUBCASE("SB at an odd address") {
    REQUIRE(rig.exec(sb(2, 1, 3)));
    CHECK(rig.trace.mem_addr == 0x203);
    CHECK(rig.trace.mem_size == 1);
    CHECK(rig.trace.mem_data == 0xEF);
    CHECK(rig.bus.read32(0x200) == 0xEF000000);
  }
  CHECK(rig.trace.mem_write);
  CHECK(rig.trace.rd == 0);
  CHECK(rig.cpu.pc() == kProgram + 4);
}

TEST_CASE("cpu: load traces the register write, not a memory write") {
  Rig rig;
  rig.bus.poke32(0x200, 0x00000080);
  rig.cpu.set_reg(1, 0x200);
  REQUIRE(rig.exec(lb(3, 1, 0)));
  CHECK(rig.trace.rd == 3);
  CHECK(rig.trace.rd_value == 0xFFFFFF80);
  CHECK_FALSE(rig.trace.mem_write);
}

TEST_CASE("cpu: JAL traces the link register") {
  Rig rig;
  REQUIRE(rig.exec(jal(1, 16)));
  CHECK(rig.trace.rd == 1);
  CHECK(rig.trace.rd_value == kProgram + 4);
  CHECK(rig.cpu.pc() == kProgram + 16);
}

TEST_CASE("cpu: FENCE retires as a no-op") {
  Rig rig;
  const auto before = rig.regs();
  REQUIRE(rig.exec(0x0FF0000F));   // fence iorw, iorw
  CHECK(rig.trace.rd == 0);
  CHECK_FALSE(rig.trace.mem_write);
  CHECK(rig.cpu.pc() == kProgram + 4);
  CHECK(rig.regs() == before);
}

// ---- Instructions that must not retire

TEST_CASE("cpu: unsupported encodings do not retire and change nothing") {
  struct Case { uint32_t insn; const char* what; };
  const Case cases[] = {
    {0x00000000, "all zeros"},
    {0xFFFFFFFF, "all ones"},
    {0x00000073, "ecall"},
    {0x00100073, "ebreak"},
    {0x022081B3, "mul x3, x1, x2 (M extension)"},
    {0x0000100F, "fence.i (Zifencei)"},
    {0x34009073, "csrw mscratch, x1 (Zicsr)"},
    {0x0000B183, "ld x3, 0(x1) (RV64)"},
    {0x0020B023, "sd x2, 0(x1) (RV64)"},
    {0x0000F183, "load with funct3 = 7"},
    {0x0020C023, "store with funct3 = 4"},
    {r_type(0x20, 2, 1, 1, 3, 0x33), "SLL with funct7 = 0x20"},
    {r_type(0x20, 2, 1, 4, 3, 0x33), "XOR with funct7 = 0x20"},
    {r_type(0x40, 2, 1, 0, 3, 0x33), "ADD with funct7 = 0x40"},
    {r_type(0x01, 2, 1, 5, 3, 0x33), "SRL with funct7 = 0x01"},
    {i_type(0x020, 1, 1, 3, 0x13), "SLLI with shamt[5] set (RV64 only)"},
    {i_type(0x420, 1, 5, 3, 0x13), "SRAI with shamt[5] set (RV64 only)"},
    {i_type(0x200, 1, 5, 3, 0x13), "SRLI with imm[11:5] = 0x10"},
    {i_type(0x400, 1, 1, 3, 0x13), "SLLI with imm[11:5] = 0x20"},
    {b_type(8, 2, 1, 2), "branch with funct3 = 2"},
    {b_type(8, 2, 1, 3), "branch with funct3 = 3"},
    {i_type(0, 1, 1, 3, 0x67), "JALR with funct3 = 1"},
    {0x0000007F, "unknown opcode 0x7F"},
    {0x0000001B, "addiw x0, x0, 0 (RV64 OP-IMM-32)"},
  };

  for (const auto& c : cases) {
    CAPTURE(c.what);
    Rig rig;
    const auto before = rig.regs();
    CHECK_FALSE(rig.exec(c.insn));
    CHECK(rig.cpu.pc() == kProgram);
    CHECK(rig.regs() == before);
  }
}

TEST_CASE("cpu: misaligned loads do not retire") {
  struct Case { uint32_t insn; const char* what; };
  const Case cases[] = {
    {lw(3, 1, 1), "LW at +1"},
    {lw(3, 1, 2), "LW at +2"},
    {lw(3, 1, 3), "LW at +3"},
    {lh(3, 1, 1), "LH at +1"},
    {lhu(3, 1, 3), "LHU at +3"},
  };
  for (const auto& c : cases) {
    CAPTURE(c.what);
    Rig rig;
    rig.cpu.set_reg(1, 0x200);
    const auto before = rig.regs();
    CHECK_FALSE(rig.exec(c.insn));
    CHECK(rig.cpu.pc() == kProgram);
    CHECK(rig.regs() == before);
  }
}

TEST_CASE("cpu: misaligned stores do not retire and leave memory alone") {
  struct Case { uint32_t insn; const char* what; };
  const Case cases[] = {
    {sw(2, 1, 1), "SW at +1"},
    {sw(2, 1, 2), "SW at +2"},
    {sh(2, 1, 1), "SH at +1"},
    {sh(2, 1, 3), "SH at +3"},
  };
  for (const auto& c : cases) {
    CAPTURE(c.what);
    Rig rig;
    rig.cpu.set_reg(1, 0x200);
    rig.bus.poke32(0x200, 0x11111111);
    rig.bus.poke32(0x204, 0x22222222);
    CHECK_FALSE(rig.exec(c.insn));
    CHECK(rig.cpu.pc() == kProgram);
    CHECK(rig.bus.read32(0x200) == 0x11111111);
    CHECK(rig.bus.read32(0x204) == 0x22222222);
  }
}

TEST_CASE("cpu: misaligned LB, LBU and SB are fine") {
  Rig rig;
  rig.cpu.set_reg(1, 0x200);
  CHECK(rig.exec(lb(3, 1, 3)));
  Rig rig2;
  rig2.cpu.set_reg(1, 0x200);
  CHECK(rig2.exec(sb(2, 1, 1)));
}

TEST_CASE("cpu: jumps to a target that is not a multiple of 4 do not retire") {
  struct Case { uint32_t insn; const char* what; };
  const Case cases[] = {
    {jal(1, 2), "JAL +2"},
    {jal(1, -6), "JAL -6"},
    {jalr(1, 0, 0x102), "JALR to 0x102"},
    {jalr(1, 5, 1), "JALR to 0x05050505 + 1, bit 0 cleared -> ...06"},
    {beq(0, 0, 2), "BEQ taken, +2"},
    {bne(1, 2, -2), "BNE taken, -2"},
  };
  for (const auto& c : cases) {
    CAPTURE(c.what);
    Rig rig;
    const auto before = rig.regs();
    CHECK_FALSE(rig.exec(c.insn));
    CHECK(rig.cpu.pc() == kProgram);
    CHECK(rig.regs() == before);   // the link register is not written either
  }
}

TEST_CASE("cpu: JALR clears bit 0 of the target") {
  Rig rig;
  rig.cpu.set_reg(5, 0x200);
  REQUIRE(rig.exec(jalr(1, 5, 1)));   // 0x201 -> 0x200
  CHECK(rig.cpu.pc() == 0x200);
  CHECK(rig.cpu.reg(1) == kProgram + 4);
}

TEST_CASE("cpu: a branch not taken ignores a misaligned target") {
  Rig rig;
  REQUIRE(rig.exec(beq(1, 2, 2)));   // x1 != x2: not taken
  CHECK(rig.cpu.pc() == kProgram + 4);
}
