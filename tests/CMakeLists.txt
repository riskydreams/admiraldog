# unit_tests: SoftMachine unit tests, headless, run by CTest with label "unit".
add_executable(unit_tests
  unit/main.cpp
  unit/cpu_test.cpp
  unit/soft_machine_test.cpp)

target_link_libraries(unit_tests PRIVATE emu_core doctest::doctest)
target_include_directories(unit_tests PRIVATE support)
set_target_properties(unit_tests PROPERTIES CXX_EXTENSIONS OFF)

emu_set_warnings(unit_tests)
emu_enable_sanitizers(unit_tests)

doctest_discover_tests(unit_tests PROPERTIES LABELS unit)

add_subdirectory(isa)
