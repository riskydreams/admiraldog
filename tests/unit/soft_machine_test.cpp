// Smoke test: proves the build, doctest and CTest are wired together.
// Real behaviour tests arrive with the CPU and the machine spec.
#include <doctest/doctest.h>

#include "emu/soft_machine.hpp"

#include <memory>

TEST_CASE("soft_machine: is usable through the Machine interface") {
  std::unique_ptr<emu::Machine> machine = std::make_unique<emu::SoftMachine>();
  machine->reset();
  CHECK(machine != nullptr);
}

TEST_CASE("soft_machine: two instances can coexist") {
  emu::SoftMachine a;
  emu::SoftMachine b;
  a.reset();
  b.reset();
  CHECK(&a != &b);
}
