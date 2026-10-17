/* Linker script for riscv-tests on the admiraldog test bus.
 * The image is linked at address 0 and loaded as a flat binary.
 * `tohost` is outside the image, at the address FlatRamBus watches
 * (keep in sync with tests/support/flat_ram_bus.hpp). */
OUTPUT_ARCH(riscv)
ENTRY(_start)

tohost = 0x00010000;

SECTIONS
{
  . = 0x00000000;
  .text.init : { *(.text.init) }
  .text      : { *(.text) }
  .data      : { *(.data) }
  .bss       : { *(.bss) }
  _end = .;
}
