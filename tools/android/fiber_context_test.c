/* Round-trip the game's real host context switch, including FP control state. */
#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

void ngb_ctx_switch(void **save, void *load);
static void *main_sp, *worker_sp;
static volatile int count, failed;

static void worker(void)
{
    if (fesetround(FE_DOWNWARD)) abort();
    for (;;) {
        if (fegetround() != FE_DOWNWARD) failed = 1;
        count++;
        ngb_ctx_switch(&worker_sp, main_sp);
    }
}

int main(void)
{
    const size_t size = 1024 * 1024;
    void *stack = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack == MAP_FAILED) return 1;
    uint64_t *sp = (uint64_t *)((char *)stack + size);
#if defined(__aarch64__)
    sp -= 176 / sizeof(*sp);
    memset(sp, 0, 176);
    sp[11] = (uint64_t)(uintptr_t)worker;
#elif defined(__x86_64__)
    *--sp = 0;
    *--sp = (uint64_t)(uintptr_t)worker;
    for (int i = 0; i < 6; i++) *--sp = 0;
    *--sp = 0;
    ((uint32_t *)sp)[0] = 0x1F80;
    ((uint16_t *)sp)[2] = 0x037F;
#else
#error "Unsupported context ABI"
#endif
    worker_sp = sp;
    if (fesetround(FE_UPWARD)) return 1;
    for (int i = 0; i < 10000; i++) {
        ngb_ctx_switch(&main_sp, worker_sp);
        if (count != i + 1 || fegetround() != FE_UPWARD || failed) {
            fprintf(stderr, "Context or FP state lost on switch %d\n", i);
            return 1;
        }
    }
    munmap(stack, size);
    puts("fiber_context_test: OK (10000 switches, independent FP rounding)");
    return 0;
}
