/*
 * link.ld
 *
 * Linker script for QEMU's `sifive_e` machine.
 *
 * IMPORTANT: sifive_e's synthesized Mask ROM reset vector is fixed --
 * on reset the CPU always executes "lui t0,0x20400; jr t0" (see
 * hw/riscv/sifive_e.c in QEMU), i.e. it unconditionally jumps into the
 * flash-mapped XIP region at 0x20400000, completely independent of
 * your ELF's own entry point. -kernel happily loads an ELF linked
 * anywhere, but if _start isn't actually at 0x20400000, the boot ROM
 * jumps into empty flash and the CPU faults on illegal (all-zero)
 * instructions before ever reaching your code -- with no error
 * printed anywhere obvious. Ask me how I know...
 *
 * So: .text/.rodata go in the flash-mapped region at 0x20400000.
 * The stack, .data, and .bss live in the 16KB DTIM SRAM at
 * 0x80000000, since flash is modeled as ROM (writable only by QEMU's
 * loader at startup, not by the guest CPU at runtime).
 *
 * .data needs two addresses: a VMA (where the CPU reads/writes it,
 * in RAM) and an LMA (where its initial byte values are actually
 * stored, in flash -- "AT> FLASH" below). start.S copies LMA -> VMA
 * before main() runs. .bss has no LMA at all (zero doesn't need to
 * be stored anywhere); start.S just zeroes its VMA range directly.
 * QEMU's -kernel loader would happily preload RAM directly and mask
 * the need for any of this, but real hardware won't.
 *
 * .sdata/.sbss (RISC-V's gp-relative "small data" sections) are
 * folded into .data/.bss below rather than given their own output
 * sections. We disabled their generation at compile time too
 * (-msmall-data-limit=0), but catching them here defensively means
 * nothing silently lands in untracked, never-copied/zeroed memory
 * again if that ever lapses.
 */
OUTPUT_ARCH("riscv")
ENTRY(_start)
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x20400000, LENGTH = 1M
  RAM   (rwx) : ORIGIN = 0x80000000, LENGTH = 16K
}
SECTIONS
{
  .text : {
    *(.text.start)
    *(.text*)
  } > FLASH
  .rodata : {
    *(.rodata*)
    *(.srodata*)
  } > FLASH
  .data : {
    . = ALIGN(4);
    _data_start = .;
    *(.data*)
    *(.sdata*)
    . = ALIGN(4);
    _data_end = .;
  } > RAM AT> FLASH
  _data_lma = LOADADDR(.data);
  .bss : {
    . = ALIGN(4);
    _bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
  } > RAM
  . = ALIGN(16);
  _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
