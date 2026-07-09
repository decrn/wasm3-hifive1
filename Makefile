CC        = riscv64-elf-gcc

WASM3_DIR = wasm3/source

WASM3_ALL = $(wildcard $(WASM3_DIR)/*.c)
WASM3_API = $(wildcard $(WASM3_DIR)/m3_api_*.c)
WASM3_SRCS = $(filter-out $(WASM3_API),$(WASM3_ALL))

WASM3_OBJS = $(WASM3_SRCS:.c=.o)

# wasm3 typically requires at least 64kB RAM, we have 16kB so we have to mess with some numbers...
# see m3_config.h for the defaults.

#   d_m3FixedHeap            12KB bump-allocator heap, lives in .bss
#   d_m3MaxFunctionStackHeight  compile-time per-function operand-stack
#                               sanity bound (slots, 4 bytes each) total 256 bytes per fn call
#   d_m3CodePageAlignSize    chunk size for compiled "code pages";
#   d_m3HasFloat             toggles the floating point part of the wasm instruction set 
#   d_m3NoFloatDynamic       see above

WASM3_DEFS = -Dd_m3FixedHeap=12288 \
             -Dd_m3MaxFunctionStackHeight=64 \
             -Dd_m3CodePageAlignSize=1024 \
             -Dd_m3VerboseErrorMessages=0 \
             -Dd_m3HasFloat=0 \
             -Dd_m3NoFloatDynamic=0

BASEFLAGS = -march=rv32imac -mabi=ilp32 -ffreestanding -nostdlib -O1 -Wall

ASFLAGS  = $(BASEFLAGS)

CFLAGS   = $(BASEFLAGS)
CFLAGS   += -ffunction-sections -fdata-sections
CFLAGS   += -Iinclude -include string.h
CFLAGS   += -I$(WASM3_DIR)
CFLAGS   += $(WASM3_DEFS)

LDFLAGS   = -T link.ld
LDFLAGS += -Wl,--gc-sections

all: hello.elf

start.o: start.S
	$(CC) $(ASFLAGS) -c $< -o $@

hello.o: hello.c
	$(CC) $(CFLAGS) -c $< -o $@

hello.elf: start.o hello.o shim.o $(WASM3_OBJS) link.ld
	$(CC) $(CFLAGS) $(LDFLAGS) start.o hello.o shim.o $(WASM3_OBJS) -o hello.elf

run: hello.elf
	qemu-system-riscv32 -M sifive_e -nographic -kernel hello.elf

clean:
	rm -f *.o hello.elf

.PHONY: all run clean
