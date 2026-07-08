volatile unsigned int result;
volatile unsigned int counter;
 
static unsigned int fib(unsigned int n) {
    unsigned int a = 0, b = 1;
    for (unsigned int i = 0; i < n; i++) {
        unsigned int t = a + b;
        a = b;
        b = t;
    }
    return a;
}
 
void _start(void) {
    while (1) {
        result = fib(10) * (counter + 1);
        counter++;
    }
}
