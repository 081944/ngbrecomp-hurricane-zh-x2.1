/*
 * d3d.c -- the hook every Direct3D entry point of the title passes through
 * (src/d3d/d3d_hooks.c, docs/renderer.md).
 *
 * d3d_hook(fn) returns nonzero when the call was handled here, so the
 * recompiled body is skipped. With NGB_RENDERER=d3d the Direct3D renderer
 * (src/d3d/renderer) takes the calls it implements; otherwise every call
 * goes to the recompiled body and from there to the NV2A emulation.
 *
 * NGB_D3D_TRACE=N logs the first N calls of each function with its
 * arguments as its convention passes them (registers first, then the
 * stack), and once a second how often each was called.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define D3D_FUNCS_TABLE
#include "d3d_funcs.h"

typedef int (*D3dHandler)(void);
D3dHandler d3d_handlers[D3DF_COUNT];   /* filled by the renderer when it is on */

static int s_trace = -1;
static unsigned s_calls[D3DF_COUNT];
static unsigned s_window[D3DF_COUNT];

static uint32_t reg(const char *r)
{
    if (!r) return 0;
    switch (r[1]) {
    case 'a': return eax;
    case 'b': return r[2] == 'x' ? ebx : g_ebp;
    case 'c': return ecx;
    case 'd': return r[2] == 'x' ? edx : edi;
    case 's': return esi;
    }
    return 0;
}

static void trace(int fn)
{
    const D3dFunc *f = &d3d_funcs[fn];
    if (s_calls[fn]++ < (unsigned)s_trace) {
        fprintf(stderr, "[D3D] %-46s", f->name);
        for (int i = 0; i < 4; i++)
            if (f->regs[i])
                fprintf(stderr, " a%d(%s)=%08X", i + 1, f->regs[i], reg(f->regs[i]));
        fprintf(stderr, " stack:");
        int n = f->stack >= 0 ? f->stack / 4 : 4;
        for (int i = 0; i < n && i < 6; i++)
            fprintf(stderr, " %08X", MEM32(esp + 4 + 4 * i));
        fprintf(stderr, "  (from %08X)\n", MEM32(esp));
    }
    s_window[fn]++;
    static time_t last;
    time_t now = time(NULL);
    if (now != last) {
        last = now;
        fprintf(stderr, "[D3D] calls/s:");
        for (int i = 0; i < D3DF_COUNT; i++)
            if (s_window[i]) {
                fprintf(stderr, " %s=%u", d3d_funcs[i].name, s_window[i]);
                s_window[i] = 0;
            }
        fprintf(stderr, "\n");
    }
}

/* NGB_D3D_PUSHDUMP=N: the methods of the first N push buffers run. A
 * D3DPushBuffer: +4 data (physical), +0xC size in bytes. */
static void push_dump(void)
{
    static int left = -1;
    if (left < 0) {
        const char *e = getenv("NGB_D3D_PUSHDUMP");
        left = e ? atoi(e) : 0;
    }
    if (!left)
        return;
    left--;
    uint32_t pb = MEM32(esp + 4), data = MEM32(pb + 4) | 0x80000000u, size = MEM32(pb + 0xC);
    fprintf(stderr, "[PUSH] buffer %08X: data %08X, %u bytes, fixup %08X\n", pb, data, size, MEM32(esp + 8));
    for (uint32_t o = 0; o + 4 <= size && o < 4096;) {
        uint32_t h = MEM32(data + o);
        uint32_t count = (h >> 18) & 0x7FF, method = h & 0x1FFC, sub = (h >> 13) & 7;
        int ni = (h & 0x40000000) != 0;
        if ((h & 0xE0000003) == 0x20000000) {
            fprintf(stderr, "   %04X jump %08X\n", o, h & 0x1FFFFFFC);
            break;
        }
        fprintf(stderr, "   %04X sub %u method %04X x%u%s:", o, sub, method, count, ni ? " (same)" : "");
        for (uint32_t i = 0; i < count && i < 8; i++)
            fprintf(stderr, " %08X", MEM32(data + o + 4 + 4 * i));
        fprintf(stderr, count > 8 ? " ...\n" : "\n");
        o += 4 + 4 * count;
    }
}

/* The Direct3D renderer (src/d3d/hle.cpp): the call's registers and stack
 * pointer; nonzero when it took the call (the recompiled body is skipped). */
int d3d_hle_call(int fn, uint32_t regs[8]);
void d3d_hle_after(int fn, const uint32_t regs[8]);
static int s_hle = -1;
/* The renderer takes host locks shared with the NV2A renderer's thread; a
 * guest thread must not be parked by the runtime's scheduler while it
 * holds one (xboxrecomp win32_compat.h: a host section). */
#include "platform/vcpu.h"
#if !XBOX_HAS_VCPU
/* (the runtime's real Windows threads, without the virtual CPU, need no host/guest bracketing) */
static inline void compat_host_enter(void) {}
static inline void compat_host_leave(void) {}
#else
void compat_host_enter(void);
void compat_host_leave(void);
#endif

/* NGB_D3D_WATCHDOG=1: a thread that reports the title's thread held in a
 * hook for over two seconds, and the presenter's frame count (debugging
 * stalls). */
#include <pthread.h>
#include <time.h>
static volatile int s_in_fn = -1;
static volatile long s_in_since;
static int s_watchdog;
static long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}
static void *watchdog(void *arg)
{
    extern unsigned long nv2a_host_presents;
    (void)arg;
    for (;;) {
        struct timespec ts = { 2, 0 };
        nanosleep(&ts, NULL);
        int fn = s_in_fn;
        long since = s_in_since;
        fprintf(stderr, "[D3D] watchdog: presenter frames %lu; title thread %s%s (%ld ms)\n",
                nv2a_host_presents, fn >= 0 ? "in " : "outside the hooks",
                fn >= 0 ? d3d_funcs[fn].name : "", fn >= 0 ? now_ms() - since : 0L);
    }
    return NULL;
}

void ngb_frame_tick(void);   /* src/cheats.c */

/* The game's CPU load (#96). The title times its frame from its start (the cycle counter at 0xD21680)
 * to the point it hands the frame to the GPU (sub_0030E140 -> 0xD21698, a percentage) and, over a
 * limit, sheds work: its HUD and 2D pass are left out for frames and drawn all at once later. Here
 * the renderer does its work inside the title's Direct3D calls, on the title's thread -- time the
 * console's Direct3D never spends -- and on slower PCs a busy fight crossed the limit while the frame
 * still made 60 fps. The time spent in the renderer is taken out: the frame's start moves on by it.
 * NGB_CPU_LOAD=1 keeps the title's own measurement. */
uint64_t xbox_ReadTimeStampCounter(void);
static int s_cpu_load = -1;
uint64_t ngb_load_exclude_begin(void);
void ngb_load_exclude_end(uint64_t t0);
static inline uint64_t host_start(void)
{
    if (s_cpu_load < 0) {
        const char *e = getenv("NGB_CPU_LOAD");
        s_cpu_load = e && atoi(e) != 0;
    }
    return s_cpu_load ? 0 : xbox_ReadTimeStampCounter();
}
static inline void host_end(uint64_t t0)
{
    if (!t0)
        return;
    uint64_t start = (uint64_t)MEM32(0xD21680) | (uint64_t)MEM32(0xD21684) << 32;
    start += xbox_ReadTimeStampCounter() - t0;
    MEM32(0xD21680) = (uint32_t)start;
    MEM32(0xD21684) = (uint32_t)(start >> 32);
}

int d3d_hook(int fn)
{
    if (fn == D3DF_D3DDevice_Swap)
        ngb_frame_tick();
    if (s_hle < 0) {
        const char *e = getenv("NGB_RENDERER");
        s_hle = e && (!strcmp(e, "d3d") || !strcmp(e, "mirror"));
        if (getenv("NGB_D3D_WATCHDOG")) {
            pthread_t th;
            s_watchdog = 1;
            if (pthread_create(&th, NULL, watchdog, NULL) == 0)
                pthread_detach(th);
        }
    }
    s_in_fn = fn;
    if (s_watchdog)   /* the clock only for it: two reads a call were 400,000 a second */
        s_in_since = now_ms();
    if (fn == D3DF_D3DDevice_RunPushBuffer)
        push_dump();
    if (s_hle) {
        uint32_t regs[8] = { eax, ecx, edx, ebx, esp, g_ebp, esi, edi };
        uint64_t t0 = host_start();
        compat_host_enter();
        int took = d3d_hle_call(fn, regs);
        compat_host_leave();
        host_end(t0);
        if (took) {
            eax = regs[0];                       /* its return value */
            esp = regs[4];                       /* its stack (ret n) */
            s_in_fn = -1;
            return 1;
        }
    }
    if (s_trace < 0) {
        const char *e = getenv("NGB_D3D_TRACE");
        s_trace = e ? atoi(e) : 0;
    }
    if (s_trace > 0)
        trace(fn);
    s_in_fn = -1;
    return d3d_handlers[fn] ? d3d_handlers[fn]() : 0;
}

/* The same for the title's wait for the GPU (D3D_BlockOnTime, src/d3d/d3d_orig.c): how long the
 * emulated GPU takes to catch up is not the title's CPU time. */
uint64_t ngb_load_exclude_begin(void) { return host_start(); }
void ngb_load_exclude_end(uint64_t t0) { host_end(t0); }

/* After the recompiled body ran (the call was not taken): its results. */
void d3d_after(int fn)
{
    if (s_hle > 0) {
        uint32_t regs[8] = { eax, ecx, edx, ebx, esp, g_ebp, esi, edi };
        s_in_fn = fn;
        if (s_watchdog)
            s_in_since = now_ms();
        uint64_t t0 = host_start();
        compat_host_enter();
        d3d_hle_after(fn, regs);
        compat_host_leave();
        host_end(t0);
        s_in_fn = -1;
    }
}

/* D3DDevice_BeginVisibilityTest (inlined into the title, src/d3d/d3d_hooks.c): the renderer told in a host
 * section like the hooks above, as it takes g_lock -- a guest thread parked while holding it froze the game
 * in scenes with the sun (#40, found by akirahayabusa). */
void d3d_visibility_begin(void);   /* src/d3d/hle.cpp */
void d3d_visibility_hook(void)
{
    compat_host_enter();
    d3d_visibility_begin();
    compat_host_leave();
}

/* The game's GPU load (#50). At the end of each frame the title puts a callback into the push buffer
 * (sub_0030E140: methods 0x1D8C and NOP 7, the callback 0x30E060), and when the GPU reaches it the time
 * since the frame began (0xD21680, the cycle counter) is the GPU's share of the frame (0xD21690 ->
 * 0xD2169C, a percentage). sub_001D5E30 takes the larger of it and the CPU's (0xD21698): over 94% it
 * lowers the quality level (0x8CDA5C) a step a frame -- Ryu's shadow map gives way to the round shadow
 * under him -- and raises it again under 90%. Here the emulated GPU only reads the push buffer, the
 * renderer draws, so how late it gets to the callback says nothing of the drawing: on some PCs it is
 * late now and then, and the shadow came and went every second or so, in nearly every area. The GPU's
 * share is reported as nothing; NGB_GPU_LOAD=1 keeps the title's own measurement. */
void ngb_gpu_frame_reached(void)
{
    static int keep = -1;
    if (keep < 0)
        keep = getenv("NGB_GPU_LOAD") && atoi(getenv("NGB_GPU_LOAD")) != 0;
    if (keep) {
        sub_0030E060();
        return;
    }
    MEM32(0xD21690) = 0;
    MEM32(0xD21694) = 0;
    eax = 0;
    edx = 0;
    esp += 4;   /* ret */
}
