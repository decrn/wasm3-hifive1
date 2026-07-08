CC      = riscv64-elf-gcc
CFLAGS  = -march=rv32imac -mabi=ilp32 -ffreestanding -nostdlib -O1 -Wall
LDFLAGS = -T link.ld

all: hello.elf

start.o: start.S
	$(CC) $(CFLAGS) -c $< -o $@

hello.o: hello.c
	$(CC) $(CFLAGS) -c $< -o $@

hello.elf: start.o hello.o link.ld
	$(CC) $(CFLAGS) $(LDFLAGS) start.o hello.o -o hello.elf

run: hello.elf
	qemu-system-riscv32 -M sifive_e -nographic -kernel hello.elf

clean:
	rm -f *.o hello.elf

.PHONY: all run clean
