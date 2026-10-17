cmake_minimum_required(VERSION 3.28)

project(admiraldog
  VERSION 0.1.0
  DESCRIPTION "A simple computer with an RV32I core and a character-based VDC"
  LANGUAGES CXX)

# Project options. Presets set these; the defaults suit a plain configure.
option(EMU_WERROR "Treat compiler warnings as errors" OFF)
set(EMU_SANITIZE "" CACHE STRING "Sanitizers for project targets, e.g. address;undefined")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(Warnings)
include(Sanitizers)

# BUILD_TESTING (default ON) comes from CTest.
include(CTest)
include(Dependencies)

add_subdirectory(core)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
