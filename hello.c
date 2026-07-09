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

void report_trap(unsigned int mcause, unsigned int mepc, unsigned int mtval) {
    uart_puts("\n*** TRAP *** mcause=0x");
    uart_put_hex32(mcause);
    uart_puts(" mepc=0x");
    uart_put_hex32(mepc);
    uart_puts(" mtval=0x");
    uart_put_hex32(mtval);
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

void main(void) {
    uart_init();
    uart_puts("Hello, world!\n");

    uart_puts("data test: ");
    uart_puts(data_test == 42 ? "OK\n" : "FAIL\n");

    uart_puts("bss test: ");
    uart_puts(bss_test == 0 ? "OK\n" : "FAIL\n");

    while (1) {
        /* nothing left to do */
    }
}
