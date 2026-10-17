# emu_core: the machine model. Depends on nothing but the C++ standard library.
add_library(emu_core STATIC
  src/cpu.cpp
  src/soft_machine.cpp)

target_include_directories(emu_core PUBLIC include)

# C++20 propagates to everything that links emu_core.
target_compile_features(emu_core PUBLIC cxx_std_20)
set_target_properties(emu_core PROPERTIES CXX_EXTENSIONS OFF)

emu_set_warnings(emu_core)
emu_enable_sanitizers(emu_core)
