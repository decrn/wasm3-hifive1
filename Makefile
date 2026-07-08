CC        = riscv64-elf-gcc

WASM3_DIR = wasm3/source

# exclude m3_api_*.c (this is WASI, libc emu, tracing, ...)
WASM3_SRCS = $(filter-out %/m3_api_wasi.c %/m3_api_libc.c %/m3_api_tracer.c %/m3_api_meta.c %/m3_api_uvwasi.c, \
                $(wildcard $(WASM3_DIR)/*.c))
WASM3_OBJS = $(WASM3_SRCS:.c=.o)

# wasm3 typically requires at least 64kB RAM, we have 16kB so we have to mess with some numbers...
# see m3_config.h for the defaults.

#   d_m3FixedHeap            12KB bump-allocator heap, lives in .bss
#   d_m3MaxFunctionStackHeight  compile-time per-function operand-stack
#                               sanity bound (slots, 4 bytes each) total 256 bytes per fn call
#   d_m3CodePageAlignSize    chunk size for compiled "code pages";
WASM3_DEFS = -Dd_m3FixedHeap=12288 \
             -Dd_m3MaxFunctionStackHeight=64 \
             -Dd_m3CodePageAlignSize=1024 \ 
             -Dd_m3VerboseErrorMessages=0 # need snprintf for this
 
CFLAGS    = -march=rv32imac -mabi=ilp32 -ffreestanding -nostdlib -O1 -Wall -I$(WASM3_DIR) $(WASM3_DEFS)
LDFLAGS   = -T link.ld

all: hello.elf

start.o: start.S
	$(CC) $(CFLAGS) -c $< -o $@

hello.o: hello.c
	$(CC) $(CFLAGS) -c $< -o $@

hello.elf: start.o hello.o shim.o $(WASM3_OBJS) link.ld
	$(CC) $(CFLAGS) $(LDFLAGS) start.o hello.o shim.o $(WASM3_OBJS) -o hello.elf

run: hello.elf
	qemu-system-riscv32 -M sifive_e -nographic -kernel hello.elf

clean:
	rm -f *.o hello.elf

.PHONY: all run clean
