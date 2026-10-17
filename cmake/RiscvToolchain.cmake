# Finds a Clang with the RISC-V backend, ld.lld and llvm-objcopy, used to build
# RISC-V test programs. Apple Clang has no RISC-V backend, so Homebrew's LLVM
# is searched first. Sets EMU_RISCV_TOOLCHAIN_FOUND.
#
# Override with -DEMU_RISCV_CLANG=/path/to/clang (and the other two variables).
set(_emu_llvm_hints
  /opt/homebrew/opt/llvm/bin
  /usr/local/opt/llvm/bin
  /usr/lib/llvm-20/bin /usr/lib/llvm-19/bin /usr/lib/llvm-18/bin)

find_program(EMU_RISCV_CLANG NAMES clang HINTS ${_emu_llvm_hints} NO_DEFAULT_PATH)
find_program(EMU_RISCV_CLANG NAMES clang)
find_program(EMU_RISCV_LLD NAMES ld.lld HINTS ${_emu_llvm_hints} /opt/homebrew/bin)
find_program(EMU_RISCV_OBJCOPY NAMES llvm-objcopy HINTS ${_emu_llvm_hints})

set(EMU_RISCV_TOOLCHAIN_FOUND FALSE)
if(EMU_RISCV_CLANG AND EMU_RISCV_LLD AND EMU_RISCV_OBJCOPY)
  execute_process(COMMAND ${EMU_RISCV_CLANG} -print-targets
                  OUTPUT_VARIABLE _emu_targets ERROR_QUIET)
  if(_emu_targets MATCHES "riscv32")
    set(EMU_RISCV_TOOLCHAIN_FOUND TRUE)
  endif()
endif()

if(EMU_RISCV_TOOLCHAIN_FOUND)
  message(STATUS "RISC-V toolchain: ${EMU_RISCV_CLANG}")
else()
  message(WARNING
    "No RISC-V toolchain (clang with riscv32 target, ld.lld, llvm-objcopy): "
    "ISA tests are disabled. On macOS: brew install llvm lld")
endif()
