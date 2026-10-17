# ISA tests: the rv32ui programs from riscv-tests, built with our own CSR-free
# environment (env/) and run one per CTest entry by isa_runner.
include(RiscvToolchain)
if(NOT EMU_RISCV_TOOLCHAIN_FOUND)
  return()
endif()

set(RISCV_TESTS_DIR ${PROJECT_SOURCE_DIR}/tests/vectors/riscv-tests)
if(NOT EXISTS ${RISCV_TESTS_DIR}/isa/rv32ui)
  message(WARNING "riscv-tests missing: run `git submodule update --init`. ISA tests disabled.")
  return()
endif()

add_executable(isa_runner isa_runner.cpp)
target_link_libraries(isa_runner PRIVATE emu_core)
target_include_directories(isa_runner PRIVATE ${PROJECT_SOURCE_DIR}/tests/support)
set_target_properties(isa_runner PROPERTIES CXX_EXTENSIONS OFF)
emu_set_warnings(isa_runner)
emu_enable_sanitizers(isa_runner)

# Not RV32I, so not built:
#   fence_i  needs Zifencei
#   ma_data  needs misaligned loads/stores, which our CPU rejects
set(EMU_ISA_EXCLUDED fence_i ma_data)

file(GLOB _rv32ui_sources CONFIGURE_DEPENDS ${RISCV_TESTS_DIR}/isa/rv32ui/*.S)
set(_env_dir ${CMAKE_CURRENT_SOURCE_DIR}/env)
set(_images "")

foreach(src IN LISTS _rv32ui_sources)
  get_filename_component(name ${src} NAME_WE)
  if(name IN_LIST EMU_ISA_EXCLUDED)
    continue()
  endif()

  set(elf ${CMAKE_CURRENT_BINARY_DIR}/rv32ui/${name}.elf)
  set(bin ${CMAKE_CURRENT_BINARY_DIR}/rv32ui/${name}.bin)
  add_custom_command(
    OUTPUT ${bin}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/rv32ui
    COMMAND ${EMU_RISCV_CLANG}
            --target=riscv32-unknown-elf -march=rv32i -mabi=ilp32 -mno-relax
            -nostdlib -nostartfiles -static
            --ld-path=${EMU_RISCV_LLD}
            -I${_env_dir} -I${RISCV_TESTS_DIR}/isa/macros/scalar
            -T ${_env_dir}/link.ld
            ${src} -o ${elf}
    COMMAND ${EMU_RISCV_OBJCOPY} -O binary ${elf} ${bin}
    DEPENDS ${src} ${_env_dir}/riscv_test.h ${_env_dir}/link.ld
    COMMENT "Building rv32ui-${name}"
    VERBATIM)
  list(APPEND _images ${bin})

  add_test(NAME rv32ui-${name} COMMAND isa_runner ${bin})
  set_tests_properties(rv32ui-${name} PROPERTIES LABELS "unit;isa")
endforeach()

add_custom_target(isa_images ALL DEPENDS ${_images})
