/**
 * Ninja Gaiden Black - static recompilation host entry point.
 *
 * Builds natively for Linux (x86-64) and Windows. Boots the recompiled XBE:
 * load default.xbe, map the Xbox address space, bring up the kernel HLE and
 * thunk table, then call the lifted entry point.
 *
 * XBE details (tools.xbe_parser):
 *   Title:       NINJA GAIDEN Black (Kunai.exe, release_ltcg)
 *   Title ID:    0x5443000D, region NA, XDK 5849
 *   Base addr:   0x00010000, image size 0x00DB6BC0
 *   Entry point: 0x0032B385
 *   Code:        .text 0x369480 bytes + D3D/DSOUND/XPP/XONLINE/XNET/DOLBY...
 *   Libraries:   D3D8LTCG, DSOUND, XAPILIB, XONLINES, LIBCMT/LIBCPMT, XGRAPHCL
 *   Kernel:      145 imports
 *
 * Game data: the extracted disc goes in a directory named "game" next to the
 * executable, or wherever NGB_GAME_DIR (or the first argument) points.
 */

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#else
#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include "platform/win32_compat.h"
#include "platform/vcpu.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <xbox/xboxrecomp.h>
#include "recomp_icall_feedback.h"
#if defined(NGB_NV2A_VK)
#include "nv2a_vk_runtime.h"
#include "nv2a_vk.h"
#include "nv2a_window.h"
#endif
#include <signal.h>

/* ── Global register state (defined in xbox_memory_layout.c) ── */

/* RECOMP_TLS is not optional: the runtime defines these thread-local, and a
 * plain extern would resolve to the TLS template rather than this thread's
 * copy, so the guest would start with every register at zero. */
extern RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_esp;
extern RECOMP_TLS uint32_t g_ebx, g_esi, g_edi;
extern ptrdiff_t g_xbox_mem_offset;

#define NGB_ENTRY_POINT 0x0032B385u

/* Renderer choice (choose_renderer): the GPU model, and whether it has a window. */
static bool s_use_gpu, s_headless;
#define NGB_D3D_DEVICE_PTR 0x003D83C8u   /* D3D8LTCG global device pointer */

extern void xbe_entry_point(void);
extern int recomp_dispatch_init(void);   /* recomp_dispatch.c */

/* ── Crash reporting ───────────────────────────────────────── */

/* Name the host function a fault happened in. Every lifted function is a real
 * symbol (sub_XXXXXXXX), so this is the guest function as well. On Linux the
 * executable is linked with -rdynamic so dladdr can see them; the module
 * offset is printed too, for addr2line when it cannot. */
static void print_host_symbol(void *rip)
{
#if defined(_WIN32)
    char buf[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO *sym = (SYMBOL_INFO *)buf;
    DWORD64 disp = 0;

    memset(buf, 0, sizeof(buf));
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 255;
    if (SymFromAddr(GetCurrentProcess(), (DWORD64)(uintptr_t)rip, &disp, sym))
        fprintf(stderr, "  in %s+0x%llX\n", sym->Name, (unsigned long long)disp);
#else
    Dl_info di;

    if (dladdr(rip, &di) && di.dli_fname) {
        fprintf(stderr, "  in %s+0x%lX (%s+0x%lX)\n",
                di.dli_sname ? di.dli_sname : "?",
                di.dli_saddr ? (unsigned long)((char *)rip - (char *)di.dli_saddr) : 0ul,
                di.dli_fname,
                (unsigned long)((char *)rip - (char *)di.dli_fbase));
    }
#endif
}

/* Recover the guest call chain: every lifted call pushes its guest return
 * address before jumping, so scanning the guest stack for values inside the
 * code range finds it. Over-reports (stale returns linger below esp), but
 * naming the caller is what a fault report needs. */
static void print_guest_stack(void)
{
    const uint32_t *sp;
    int i, shown = 0;

    if (!g_xbox_mem_offset || !g_esp)
        return;
    sp = (const uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esp);
    fprintf(stderr, "  guest stack (return addresses, innermost first):\n");
    for (i = 0; i < 256 && shown < 24; i++) {
        uint32_t v = sp[i];
        if (v > g_xbox_code_lo && v < g_xbox_code_hi) {
            fprintf(stderr, "    [esp+%-4d] 0x%08X\n", i * 4, v);
            shown++;
        }
    }
    if (!shown) {
        extern volatile uint32_t g_icall_trace[16];
        extern volatile uint32_t g_icall_trace_idx;
        uint32_t k;

        fprintf(stderr, "    (no return addresses in range)\n  recent ICALL targets:");
        for (k = 0; k < 16; k++)
            fprintf(stderr, " %08X", g_icall_trace[(g_icall_trace_idx + k) & 15]);
        fprintf(stderr, "\n");
        for (i = 0; i < 20; i++)
            fprintf(stderr, "    raw[esp+%-4d] 0x%08X\n", i * 4, sp[i]);
    }
}

/* The emulated APU: its registers are unmapped when RECOMP_AC97_READY is set,
 * and every access arrives here to be decoded and serviced. */
typedef struct MCPXAPUState MCPXAPUState;
extern MCPXAPUState *mcpx_apu_init_standalone(uint8_t *ram_ptr);
extern MCPXAPUState *g_apu_state;
extern bool apu_hook_handle_mmio(PCONTEXT ctx, uintptr_t fault_addr,
                                 uint32_t fault_xbox_va, int is_write);
#define APU_TRAP_BASE 0xFE800000u
#define APU_TRAP_END  0xFE880000u

static LONG CALLBACK veh_handler(PEXCEPTION_POINTERS ep)
{
    uintptr_t fault_addr;
    uint32_t xbox_va;
    int is_write;

    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

    fault_addr = ep->ExceptionRecord->ExceptionInformation[1];
    is_write = ep->ExceptionRecord->ExceptionInformation[0] ? 1 : 0;
    xbox_va = (uint32_t)(fault_addr - (uintptr_t)g_xbox_mem_offset);

    if (g_apu_state && xbox_va >= APU_TRAP_BASE && xbox_va < APU_TRAP_END
            && apu_hook_handle_mmio(ep->ContextRecord, fault_addr, xbox_va, is_write))
        return EXCEPTION_CONTINUE_EXECUTION;

    fprintf(stderr, "[CRASH] Access violation at RIP=0x%llX, fault addr=0x%llX (%s)\n",
            (unsigned long long)ep->ContextRecord->Rip,
            (unsigned long long)fault_addr, is_write ? "write" : "read");
    fprintf(stderr, "  Xbox VA of fault: 0x%08X\n", xbox_va);
    fprintf(stderr, "  Xbox regs: eax=%08X ecx=%08X edx=%08X ebx=%08X\n",
            g_eax, g_ecx, g_edx, g_ebx);
    fprintf(stderr, "             esi=%08X edi=%08X esp=%08X\n",
            g_esi, g_edi, g_esp);
    print_host_symbol((void *)(uintptr_t)ep->ContextRecord->Rip);
    print_guest_stack();
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

/* ── Hang sampling (Linux) ─────────────────────────────────── */

#if !defined(_WIN32)
/* `kill -USR1 <pid>` prints where the guest thread is: a host backtrace
 * (every lifted function is a named symbol) and its guest registers. No
 * ptrace, so it works where gdb cannot attach and where gdb would swallow the
 * TF single-steps the register traps depend on. Whichever thread the kernel
 * hands the signal to forwards it to the guest thread. */
static pthread_t s_guest_thread;
static sem_t *s_sample_done;

/* One external `kill -USR1` fans out to every guest thread (the boot thread
 * and everything CreateThread started); each prints its own host backtrace
 * and its own guest registers, which are thread-local. */
static void sample_on_signal(int sig, siginfo_t *si, void *uc)
{
    void *frames[48];
    int n;

    (void)uc;
    if (si->si_code == SI_USER) {
        /* One at a time, so the backtraces do not interleave. */
        static sem_t done;
        static int inited;
        pthread_t th[256];
        int k = compat_thread_list(th, 256);
        if (!inited) { sem_init(&done, 0, 0); inited = 1; }
        s_sample_done = &done;
        th[k++] = s_guest_thread;
        for (int i = 0; i < k; i++) {
            if (pthread_equal(th[i], pthread_self()))
                continue;   /* cannot wait on itself; it is sampled last */
            if (pthread_kill(th[i], sig) == 0) {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += 2;
                sem_timedwait(&done, &ts);
            }
        }
#if defined(NGB_NV2A_VK)
        if (s_use_gpu)
            nv2a_vk_sample_threads();
#endif
        return;
    }
    n = backtrace(frames, 48);
    fprintf(stderr, "[SAMPLE] %s thread %lx prio %d%s: esp=%08X eax=%08X ecx=%08X "
                    "edx=%08X ebx=%08X esi=%08X edi=%08X\n",
            pthread_equal(pthread_self(), s_guest_thread) ? "boot" : "guest",
            (unsigned long)pthread_self(),
            vcpu_self() ? vcpu_get_priority(vcpu_self()) : -99,
            vcpu_holds_cpu() ? " (running)" : "",
            g_esp, g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi);
    backtrace_symbols_fd(frames, n, 2);
    if (s_sample_done)
        sem_post(s_sample_done);
}
#endif

/* ── Paths ─────────────────────────────────────────────────── */

/* Directory holding the executable, with a trailing separator. */
static void exe_dir(char *out, size_t n)
{
    char *slash;
#if defined(_WIN32)
    DWORD len = GetModuleFileNameA(NULL, out, (DWORD)n);
    if (len == 0 || len >= n) { out[0] = 0; return; }
    slash = strrchr(out, '\\');
#else
    ssize_t len = readlink("/proc/self/exe", out, n - 1);
    if (len <= 0) { out[0] = 0; return; }
    out[len] = 0;
    slash = strrchr(out, '/');
#endif
    if (slash) slash[1] = 0; else out[0] = 0;
}

/* Per-title save location (hard disk partition images, saves, cache):
 * NGB_SAVE_DIR, else $XDG_DATA_HOME or ~/.local/share on Linux and
 * %LOCALAPPDATA% on Windows, under NinjaGaidenBlack. */
static void save_dir_path(char *out, size_t n)
{
    const char *env = getenv("NGB_SAVE_DIR");
#if defined(_WIN32)
    const char *base = getenv("LOCALAPPDATA");
    if (env && *env) { snprintf(out, n, "%s", env); return; }
    snprintf(out, n, "%s\\NinjaGaidenBlack", base && *base ? base : ".");
#else
    const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
    if (env && *env) { snprintf(out, n, "%s", env); return; }
    if (xdg && *xdg)
        snprintf(out, n, "%s/NinjaGaidenBlack", xdg);
    else
        snprintf(out, n, "%s/.local/share/NinjaGaidenBlack", home && *home ? home : ".");
#endif
}

static bool load_file(const char *path, void **out_data, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    long size;
    void *data;

    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || !(data = malloc((size_t)size))) {
        fclose(f);
        return false;
    }
    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data);
        fclose(f);
        return false;
    }
    fclose(f);
    *out_data = data;
    *out_size = (size_t)size;
    return true;
}

/* ── Entry ─────────────────────────────────────────────────── */

/* Runtime switches this title needs to get past hardware it waits on, until
 * the hardware is modelled. Set only when absent, so any of them can still be
 * overridden (or turned off with an empty value) from the environment.
 *
 *   RECOMP_VBLANK      D3D blocks on its vblank event; raise the NV2A vblank
 *                      interrupt at 60 Hz.
 *   RECOMP_AC97_READY  DirectSound programs the APU directly; trap its
 *                      registers into the emulated APU.
 *   RECOMP_DSP_ACK     DirectSound's effects-image download posts command 3 to
 *                      the GP DSP mailbox and spins until the DSP clears it
 *                      (sub_0037DAAE). The DSP is not emulated, so complete it
 *                      at once. The mailbox is +0x810 into a contiguous block
 *                      whose address is deterministic for this build. */
static void set_default(const char *name, const char *value)
{
    if (getenv(name))
        return;
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 0);
#endif
}

/* NGB_RENDERER: "vulkan" (default when built with the GPU model), "null"
 * (the GPU model without a window -- for headless runs), or "none" (no GPU
 * model: the plain-RAM aperture and the fence mirror, as before it existed). */

static void choose_renderer(void)
{
    const char *r = getenv("NGB_RENDERER");
#if defined(NGB_NV2A_VK)
    s_use_gpu = !(r && !strcmp(r, "none"));
    s_headless = r && !strcmp(r, "null");
#else
    (void)r;
#endif
}

static void bringup_defaults(void)
{
    /* With the GPU model, vblank comes from its CRTC. */
    if (!s_use_gpu)
        set_default("RECOMP_VBLANK", "1");
    set_default("RECOMP_AC97_READY", "1");
    set_default("RECOMP_DSP_ACK", "0x82BF8810");
}

#if defined(RECOMP_ICALL_FEEDBACK)
/* A bring-up run usually ends at a timeout or ^C, not an orderly exit, and
 * atexit does not run on a signal. Dump on the way out of those too. */
static void feedback_on_signal(int sig)
{
    RECOMP_ICALL_FEEDBACK_DUMP();
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

/* ── Boot (guest thread) ───────────────────────────────────── */

static char s_game_dir[4096];

/* Everything from loading the XBE to the entry point runs on the thread that
 * will execute guest code: the guest register file and the TIB the kernel
 * sets up are per-thread. */
static void *guest_main(void *unused)
{
    char xbe_path[4200];
    void *xbe_data = NULL;
    size_t xbe_size = 0;

    (void)unused;
#if !defined(_WIN32)
    s_guest_thread = pthread_self();
    /* The boot thread runs the title's main thread inline: it is a guest
     * thread like the ones the title creates (platform/vcpu.h). */
    vcpu_attach(VCPU_PRIO_NORMAL);
#endif
    snprintf(xbe_path, sizeof(xbe_path), "%s/default.xbe", s_game_dir);
    if (!load_file(xbe_path, &xbe_data, &xbe_size)) {
        fprintf(stderr, "Cannot read %s\n"
                        "Put the extracted disc in a 'game' directory next to the "
                        "executable, or pass its path.\n", xbe_path);
        exit(1);
    }
    printf("XBE loaded: %zu bytes\n", xbe_size);

    if (!xbox_MemoryLayoutInit(xbe_data, xbe_size)) {
        fprintf(stderr, "Failed to initialize the Xbox memory layout.\n");
        exit(1);
    }
    g_xbox_mem_offset = xbox_GetMemoryOffset();
    printf("Xbox memory mapped. Offset: 0x%llX\n",
           (unsigned long long)g_xbox_mem_offset);

#if defined(NGB_NV2A_VK)
    if (s_use_gpu) {
        Nv2aVkOptions o = { .headless = s_headless };
        if (xbox_Nv2aVkStart(&o) != 0) {
            fprintf(stderr, "Cannot start the GPU.\n");
            exit(1);
        }
    } else
#endif
    {
        /* No GPU model: D3D8LTCG's pushbuffer fence never advances on its
         * own. The device pointer lives at 0x3D83C8; +0x2C is the write
         * sequence and +0x30 points at where the GPU reports how far it has
         * read. Mirror one into the other so space and fence waits
         * (sub_003CAA10) complete instead of spinning forever. */
        xbox_Nv2aMirrorFence(NGB_D3D_DEVICE_PTR, 0x2C, 0x30);
    }

    /* The APU and its register trap are only useful together: a trap with no
     * model declines every access, a model nothing traps into sees nothing. */
    if (getenv("RECOMP_AC97_READY")) {
        g_apu_state = mcpx_apu_init_standalone((uint8_t *)xbox_GetMemoryBase());
        fprintf(stderr, "[BOOT] emulated APU %s\n",
                g_apu_state ? "up" : "FAILED to initialise");
    }

    xbox_kernel_init();
    {
        extern void xbox_path_init(const char *game_dir, const char *save_dir);
        char save_dir[4096];
        save_dir_path(save_dir, sizeof(save_dir));
        printf("Save directory: %s\n", save_dir);
        xbox_path_init(s_game_dir, save_dir);
    }
    xbox_kernel_bridge_init();
    g_esp = XBOX_STACK_TOP;

    /* Flat dispatch table: without it every indirect call binary-searches the
     * function table. Optional -- the search still works if this fails. */
    if (!recomp_dispatch_init())
        fprintf(stderr, "[BOOT] flat dispatch unavailable; "
                        "indirect calls will use the binary search\n");

    /* Inert unless RECOMP_WATCHDOG_SECS is set; must run on the guest thread. */
    xbox_WatchdogStart();

    printf("Entry point: 0x%08X, ESP: 0x%08X\nStarting game...\n",
           NGB_ENTRY_POINT, g_esp);
    xbe_entry_point();

    printf("Game returned.\n");
    exit(0);
    return NULL;
}

/* ── Entry (host main thread: window and events) ───────────── */

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : getenv("NGB_GAME_DIR");

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    choose_renderer();
    bringup_defaults();

    if (dir && *dir) {
        snprintf(s_game_dir, sizeof(s_game_dir), "%s", dir);
    } else {
        char base[4096];
        exe_dir(base, sizeof(base));
        snprintf(s_game_dir, sizeof(s_game_dir), "%sgame", base);
    }

    printf("=== Ninja Gaiden Black - Static Recompilation ===\n");
    printf("Game directory: %s\n", s_game_dir);

#if defined(_WIN32)
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
#endif
    AddVectoredExceptionHandler(1, veh_handler);
#if !defined(_WIN32)
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = sample_on_signal;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        sigaction(SIGUSR1, &sa, NULL);
    }
#endif
#if defined(RECOMP_ICALL_FEEDBACK)
    RECOMP_ICALL_FEEDBACK_INIT();     /* atexit dump to icall_targets.dump */
    signal(SIGINT, feedback_on_signal);
    signal(SIGTERM, feedback_on_signal);
#endif

#if defined(NGB_NV2A_VK)
    if (s_use_gpu && !s_headless &&
        nv2a_window_create("Ninja Gaiden Black", 1280, 960,
                           getenv("NGB_FULLSCREEN") != NULL,
                           4.0 / 3.0, getenv("NGB_VSYNC") != NULL) != 0) {
        fprintf(stderr, "Cannot open the game window.\n");
        return 1;
    }
#endif

#if defined(_WIN32)
    CloseHandle(CreateThread(NULL, 64u << 20,
                             (LPTHREAD_START_ROUTINE)guest_main, NULL, 0, NULL));
#else
    {
        /* Lifted code nests a host call per guest call; give it room. */
        pthread_attr_t attr;
        pthread_t th;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 64u << 20);
        if (pthread_create(&th, &attr, guest_main, NULL) != 0) {
            fprintf(stderr, "Cannot start the guest thread.\n");
            return 1;
        }
        pthread_attr_destroy(&attr);
    }
#endif

#if defined(NGB_NV2A_VK)
    if (s_use_gpu && !s_headless) {
        while (nv2a_window_pump())
            usleep(2000);
        printf("Window closed.\n");
        _exit(0);
    }
#endif
    for (;;)
        pause();
}
