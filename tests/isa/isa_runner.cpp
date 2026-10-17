// isa_runner <test.bin>
//
// Runs one riscv-tests program (built with tests/isa/env) on the SoftMachine CPU
// over a FlatRamBus, and reports through the exit code: 0 pass, 1 fail.
// Each program is a separate CTest entry, e.g. `ctest --preset dev -R rv32ui-add`.
#include "emu/cpu.hpp"
#include "flat_ram_bus.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

namespace {

constexpr uint64_t kMaxSteps = 100'000;  // every rv32ui test needs far fewer

int fail(const char* name, const char* why, const emu::Cpu& cpu) {
  std::printf("%s: FAIL (%s, pc=0x%08x)\n", name, why, cpu.pc());
  return 1;
}

} // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: isa_runner <test.bin>\n");
    return 2;
  }
  const char* name = argv[1];

  std::ifstream file(name, std::ios::binary);
  const std::vector<uint8_t> image{std::istreambuf_iterator<char>(file), {}};
  if (!file.good() && !file.eof()) {
    std::fprintf(stderr, "%s: cannot read file\n", name);
    return 2;
  }

  emu::test::FlatRamBus bus;
  if (image.empty() || !bus.load(0, image)) {
    std::fprintf(stderr, "%s: empty image or too large for test RAM\n", name);
    return 2;
  }

  emu::Cpu cpu{bus};
  cpu.reset(0);

  emu::TraceRecord trace;
  for (uint64_t step = 0; step < kMaxSteps; ++step) {
    if (!cpu.step(trace)) return fail(name, "cpu stopped: instruction did not retire", cpu);
    if (bus.stray_access()) return fail(name, "access outside test RAM", cpu);
    if (auto value = bus.tohost()) {
      if (*value == 1) {
        std::printf("%s: PASS (%llu instructions)\n", name,
                    static_cast<unsigned long long>(step + 1));
        return 0;
      }
      std::printf("%s: FAIL (test case %u)\n", name, *value >> 1);
      return 1;
    }
  }
  return fail(name, "no result after max steps", cpu);
}
