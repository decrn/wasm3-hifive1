HiFive1 bare-metal WebAssembly runtime
=======================================

What this is
-------------
A minimal 'WASI-like' host running the wasm3 interpreter on bare-metal
RV32IMAC, targeting a SiFive HiFive1 RevB compatible board with FE310 core w/ PMP.
Developed and tested against QEMU's sifive_e machine model,
with an eventual port to real hardware.

Status
------
Working end-to-end on QEMU (sifive_e):
  - UART0 output ("Hello, world!") confirmed printing.
  - .data/.bss copy-down and zeroing verified in start.S.
  - wasm3 builds for rv32imac with a 12KB fixed-heap allocator inside
    the 16KB SRAM budget.
  - A hand-encoded wasm module (add(i32,i32) -> i32) parses, loads,
    and runs via wasm3: add(5,3) = 8.

Todo:
  - clock_time_get (would prob use the CLINT mtime register)
  - random_get (PRNG based off clock?)
  - A real WASI fd_write shim 
  - RAM budget tuning (I just picked something)
  - Porting the working image to real HiFive1 hardware

Toolchain
---------
  - riscv64-elf-gcc / riscv64-elf-gdb
  - qemu-system-riscv32, -M sifive_e machine model.
  - wasm3 (git submodule), built with float support disabled and a
    small fixed-heap config to fit inside 16KB of RAM.

Layout
------
  link.ld           linker script: FLASH (XIP, 0x20400000) + RAM
                     (DTIM SRAM, 0x80000000, 16K)
  start.S           reset stub: sets sp, installs trap handler,
                     copies .data down, zeroes .bss, calls main()
  hello.c           UART driver + main()/test driver + hand-encoded
                     wasm byte array
  shim.c            libc-shaped functions wasm3 needs (memcpy,
                     memset, memmove, memcmp, strlen/strcmp/strncmp,
                     abort) -- each added only once the linker
                     actually needed it
  include/          stub headers (string.h, stdlib.h, stdio.h,
                     assert.h, inttypes.h) standing in for the
                     newlib headers this toolchain doesn't ship
  wasm3/            git submodule for wasm3 vm
  Makefile

Build & run
-----------
  make          builds hello.elf
  make run      qemu-system-riscv32 -M sifive_e -nographic -kernel hello.elf

Read this to save yourself time
-------------------------------
  - This toolchain ships !no! libc at all -- every libc-shaped symbol
    wasm3 calls must be hand-supplied (shim.c) or at least
    stub-declared (include/).
  - -march needs the explicit _zicsr_zifencei extensions, or the
    csrr/csrw in the trap handler won't assemble (post-2019 ISA spec
    split extracted these out of the base ISA).
  - -nostdlib drops libgcc too, not just libc -- I added -lgcc at the end
    of the link line to fix undefined-reference errors that look
    libc-related but aren't.
  - RISC-V's small-data convention (.sdata/.sbss) can silently place
    globals outside what a hand-written linker script tracks -- fixed
    with -msmall-data-limit=0 plus explicit *(.sdata*)/*(.sbss*)
    rules in link.ld. Caused a real memset(NULL, ...) crash that took
    a while to trace back to this.
  - No trap handler installed (mtvec left unset)
  - QEMU's sifive_e machine has a hardcoded mask-ROM reset vector
    that jumps to a fixed flash address regardless of the ELF entry
    point -- setting the ELF entry point alone is not sufficient.
