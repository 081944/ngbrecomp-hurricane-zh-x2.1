/* VEH over sigaction: a trapped page answered through mmio_emulate, writes
 * seen with the right value and direction, and a TF single-step delivered as
 * EXCEPTION_SINGLE_STEP that re-protects the page afterwards. */
#include "platform/win32_compat.h"
#include "platform/mmio_decode.h"
#include <stdio.h>
#include <stdint.h>

static uint8_t *s_page;
/* volatile: changed from inside the signal handler. */
static volatile uint32_t s_last_write, s_last_write_off;
static volatile int s_reads, s_writes, s_steps, s_step_armed;
static volatile int s_fault_write, s_direction_errors;

static uint64_t dev_read(void *dev, uint32_t off, int size)
{
    (void)dev; (void)size;
    s_reads++;
#if defined(__aarch64__)
    if (s_fault_write) s_direction_errors++;
#endif
    return 0xC0DE0000u | off;
}

static void dev_write(void *dev, uint32_t off, uint64_t val, int size)
{
    (void)dev; (void)size;
    s_writes++;
#if defined(__aarch64__)
    if (!s_fault_write) s_direction_errors++;
#endif
    s_last_write = (uint32_t)val;
    s_last_write_off = off;
}

static LONG mmio_handler(PEXCEPTION_POINTERS ep)
{
    uintptr_t a;
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    a = ep->ExceptionRecord->ExceptionInformation[1];
    s_fault_write = (int)ep->ExceptionRecord->ExceptionInformation[0];
    if (a < (uintptr_t)s_page || a >= (uintptr_t)s_page + 4096)
        return EXCEPTION_CONTINUE_SEARCH;
    if (s_step_armed) {
        DWORD old;
        VirtualProtect(s_page, 4096, PAGE_READWRITE, &old);
        ep->ContextRecord->EFlags |= 0x100u;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return mmio_emulate(ep->ContextRecord, (uint32_t)(a - (uintptr_t)s_page),
                        NULL, dev_read, dev_write)
           ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

static LONG step_handler(PEXCEPTION_POINTERS ep)
{
    DWORD old;
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP || !s_step_armed)
        return EXCEPTION_CONTINUE_SEARCH;
    s_steps++;
    s_step_armed = 0;
    VirtualProtect(s_page, 4096, PAGE_NOACCESS, &old);
    ep->ContextRecord->EFlags &= ~0x100u;
    return EXCEPTION_CONTINUE_EXECUTION;
}

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", \
    __FILE__, __LINE__, #c); return 1; } } while (0)

int main(void)
{
    volatile uint32_t *reg;
    uint32_t v;
    DWORD old;

    s_page = VirtualAlloc(NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(s_page != NULL);
    CHECK(VirtualProtect(s_page, 4096, PAGE_NOACCESS, &old));
    CHECK(AddVectoredExceptionHandler(0, step_handler) != NULL);
    CHECK(AddVectoredExceptionHandler(1, mmio_handler) != NULL);
    reg = (volatile uint32_t *)(s_page + 0x100);

    v = *reg;                                    /* 8B: read */
    CHECK(s_reads == 1 && v == 0xC0DE0100u);
    *reg = 0x12345678u;                          /* 89: write */
    CHECK(s_writes == 1 && s_last_write == 0x12345678u && s_last_write_off == 0x100);
    reg[1] = 7;                                  /* C7: imm write */
    CHECK(s_writes == 2 && s_last_write == 7 && s_last_write_off == 0x104);

#if defined(__aarch64__)
    /* Exercise Linux's ESR write flag and the decoder's narrow, wide and
     * SIMD forms through real page faults, not manufactured contexts. */
    {
        volatile uint8_t *r8 = s_page + 0x120;
        volatile uint16_t *r16 = (volatile uint16_t *)(s_page + 0x124);
        volatile uint64_t *r64 = (volatile uint64_t *)(s_page + 0x128);
        CHECK(*r8 == 0x20);
        *r8 = 0xA5;
        CHECK(s_last_write == 0xA5 && s_last_write_off == 0x120);
        CHECK(*r16 == 0x124);
        *r16 = 0xABCD;
        CHECK(s_last_write == 0xABCD && s_last_write_off == 0x124);
        CHECK(*r64 == 0xC0DE0128u);
        *r64 = 0x1234567887654321ULL;
        CHECK(s_last_write == 0x87654321u && s_last_write_off == 0x128);
        uint64_t vector[2];
        void *address = s_page + 0x140;
        __asm__ volatile("ldr q0, [%0]\nstr q0, [%1]"
                         : : "r"(address), "r"(vector) : "v0", "memory");
        CHECK(vector[0] == 0xC0DE0140u && vector[1] == 0xC0DE0148u);
        __asm__ volatile("ldr q0, [%0]\nstr q0, [%1]"
                         : : "r"(vector), "r"(address) : "v0", "memory");
        CHECK(s_last_write == 0xC0DE0148u && s_last_write_off == 0x148);
        CHECK(s_direction_errors == 0);
    }
#endif

#if defined(__x86_64__)
    /* Step a real store through the unprotected page, then re-trap it. */
    s_step_armed = 1;
    reg[2] = 0xAB;
    CHECK(s_steps == 1 && s_page[0x108] == 0xAB);
    v = reg[3];
    CHECK(s_reads == 2 && v == 0xC0DE010Cu);

    /* Immediate forms through the shared decoder: TEST (F7 /0), OR (83 /1),
     * CMP (81 /7) -- a poll loop's test and a read-modify-write. */
    {
        uint8_t nz, below;
        __asm__ volatile("testl $0x100, (%1)\n\tsetnz %0"
                         : "=q"(nz) : "r"(reg) : "cc", "memory");
        CHECK(nz == 1);
        __asm__ volatile("testl $0x1, (%1)\n\tsetnz %0"
                         : "=q"(nz) : "r"(reg) : "cc", "memory");
        CHECK(nz == 0);
        __asm__ volatile("orl $1, (%0)" : : "r"(reg) : "cc", "memory");
        CHECK(s_last_write == 0xC0DE0101u && s_last_write_off == 0x100);
        __asm__ volatile("cmpl $0xC0DE0200, (%1)\n\tsetb %0"
                         : "=q"(below) : "r"(reg) : "cc", "memory");
        CHECK(below == 1);
    }
    /* ALU r, r/m (23 /r): the register takes the result. */
    {
        uint32_t x = 0xFFFF0F00u;
        __asm__ volatile("andl (%1), %0" : "+r"(x) : "r"(reg) : "cc", "memory");
        CHECK(x == (0xFFFF0F00u & 0xC0DE0100u));
    }

#endif
    printf("veh_posix_test: OK (%d reads, %d writes, %d steps)\n",
           s_reads, s_writes, s_steps);
    return 0;
}
