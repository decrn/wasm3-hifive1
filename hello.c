/*
 * hello.c
 *
 * First real bare-metal test: print "Hello, world!" over UART0 on the
 * FE310/HiFive1, running under QEMU's sifive_e machine model.
 *
 * Register offsets from the FE310-G000 manual (UART0 base 0x10013000):
 *   txdata  0x00  bit31 = TX FIFO full, bits[7:0] = byte to send
 *   txctrl  0x08  bit0  = txen (must be set or QEMU's UART model
 *                          silently drops writes)
 *
 * No baud-rate divisor setup: QEMU's sifive_uart model doesn't emulate
 * bit-level timing, so `div` isn't needed for output to appear here.
 * Real hardware will need it configured for a real terminal to make
 * sense of the bitstream -- that's a later step.
 */

#include <stdint.h>
#include "wasm3.h"

#define UART0_BASE   0x10013000UL
#define UART_TXDATA  (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART_TXCTRL  (*(volatile unsigned int *)(UART0_BASE + 0x08))

#define UART_TXCTRL_TXEN (1u << 0)
#define UART_TXDATA_FULL (1u << 31)

static void uart_init(void) {
    UART_TXCTRL = UART_TXCTRL_TXEN;
}

static void uart_putc(char c) {
    while (UART_TXDATA & UART_TXDATA_FULL) {
        /* spin while the TX FIFO is full */
    }
    UART_TXDATA = (unsigned char)c;
}

// not static!! abort() (see shim.c) needs to write to UART
void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

static void uart_put_hex32(unsigned int v) {
    static const char hex[] = "0123456789abcdef";
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(v >> i) & 0xF]);
    }
}

void report_trap(unsigned int mcause, unsigned int mepc, unsigned int mtval,
                  unsigned int orig_a0, unsigned int orig_a1, unsigned int orig_a2) {
    uart_puts("\n*** TRAP *** mcause=0x");
    uart_put_hex32(mcause);
    uart_puts(" mepc=0x");
    uart_put_hex32(mepc);
    uart_puts(" mtval=0x");
    uart_put_hex32(mtval);
    uart_puts("\n    a0=0x");
    uart_put_hex32(orig_a0);
    uart_puts(" a1=0x");
    uart_put_hex32(orig_a1);
    uart_puts(" a2=0x");
    uart_put_hex32(orig_a2);
    uart_puts("\n");
}

/*
 * Deliberate .data/.bss test: data_test has a non-zero initializer,
 * so its correct value can only come from the flash->RAM copy-down
 * in start.S. bss_test relies on C's implicit-zero guarantee, which
 * only holds if start.S actually zeroed it -- RAM is not zero at
 * power-on by default. If either print says FAIL, the copy-down or
 * zeroing logic (or the linker script symbols it depends on) is
 * broken.
 */
static int data_test = 42;
static int bss_test;

/*
 * Hand-encoded minimal wasm module: exports a function "add" of type
 * (i32, i32) -> i32, body is local.get 0; local.get 1; i32.add; end.
 * No imports, no memory -- deliberately nothing here that needs a
 * host function, so this only exercises wasm3's core parse/compile/
 * execute pipeline, not the WASI shim.
 */
static const uint8_t add_wasm[] = {
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, /* \0asm, version 1 */
    0x01, 0x07, 0x01, 0x60, 0x02, 0x7F, 0x7F, 0x01, 0x7F, /* type: (i32,i32)->i32 */
    0x03, 0x02, 0x01, 0x00,                               /* func 0 uses type 0 */
    0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, /* export "add" -> func 0 */
    0x0A, 0x09, 0x01, 0x07, 0x00,                         /* code section, 1 func, body size 7, 0 locals */
    0x20, 0x00,                                           /* local.get 0 */
    0x20, 0x01,                                           /* local.get 1 */
    0x6A,                                                  /* i32.add */
    0x0B                                                   /* end */
};


/* No printf -- minimal decimal printer for values we get back from wasm. */
static void uart_put_uint(unsigned int v) {
    char buf[11]; /* max "4294967295" + NUL */
    int i = 10;
    buf[10] = '\0';
    if (v == 0) {
        uart_puts("0");
        return;
    }
    while (v > 0 && i > 0) {
        buf[--i] = '0' + (char)(v % 10);
        v /= 10;
    }
    uart_puts(&buf[i]);
}

static void run_add_test(void) {
    uart_puts("wasm3 env: ");
    IM3Environment env = m3_NewEnvironment();
    uart_puts(env != NULL ? "OK\n" : "FAIL\n");

    uart_puts("wasm3 runtime: ");
    IM3Runtime runtime = m3_NewRuntime(env, 2048, NULL);
    uart_puts(runtime != NULL ? "OK\n" : "FAIL\n");

    uart_puts("parse module: ");
    IM3Module module;
    M3Result result = m3_ParseModule(env, &module, add_wasm, sizeof(add_wasm));
    uart_puts(result == NULL ? "OK\n" : "FAIL\n");
    if (result) { uart_puts(result); uart_puts("\n"); return; }

    uart_puts("load module: ");
    result = m3_LoadModule(runtime, module);
    uart_puts(result == NULL ? "OK\n" : "FAIL\n");
    if (result) { uart_puts(result); uart_puts("\n"); return; }

    uart_puts("find function: ");
    IM3Function function;
    result = m3_FindFunction(&function, runtime, "add");
    uart_puts(result == NULL ? "OK\n" : "FAIL\n");
    if (result) { uart_puts(result); uart_puts("\n"); return; }

    uart_puts("call add(5, 3): ");
    result = m3_CallV(function, 5, 3);
    uart_puts(result == NULL ? "OK\n" : "FAIL\n");
    if (result) { uart_puts(result); uart_puts("\n"); return; }

    uint32_t sum = 0;
    result = m3_GetResultsV(function, &sum);
    uart_puts("result: ");
    uart_put_uint(sum);
    uart_puts("\n");
}

void main(void) {
    uart_init();
    uart_puts("Hello, world!\n");

    uart_puts("data test: ");
    uart_puts(data_test == 42 ? "OK\n" : "FAIL\n");

    uart_puts("bss test: ");
    uart_puts(bss_test == 0 ? "OK\n" : "FAIL\n");

    run_add_test();

    while (1) {
        /* nothing left to do */
    }
}
