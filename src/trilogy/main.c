/*
 * NinjaTrilogy -- the NES Ninja Gaiden that the arcade cabinet in Han's Bar and the main menu's
 * ARCADE NINJA GAIDEN run (#18). On the Xbox that is a second program on the disc
 * (NinjaTrilogy.xbe): the game hands over to it with XLaunchNewImage, the console reboots into it,
 * and it hands back to default.xbe the same way, passing launch data both ways. Here it is
 * recompiled on its own (tools/pipeline_trilogy.sh) and this is its host: the boot of src/main.c
 * without what belongs to Ninja Gaiden Black (its Direct3D renderer, menus, settings). It draws
 * with the GPU model's own NV2A renderer: its Direct3D runs as it was built.
 *
 * Started by the game (src/launch.c) with NGB_GAME_DIR, NGB_SAVE_DIR and the launch data page in
 * RECOMP_LAUNCH_PAGE; or by hand: NinjaTrilogy <game dir> [<save dir>].
 */
#if defined(_WIN32)
#include <windows.h>
#include <unistd.h>
#include <pthread.h>
#include "platform/vcpu.h"
#else
#include <limits.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include "platform/win32_compat.h"
#include "platform/vcpu.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <xbox/xboxrecomp.h>
#include "usb/ohci.h"
#include "usb/usb_gamepad.h"
#include "nv2a_vk_runtime.h"
#include "nv2a_window.h"
#include "ngb_launch.h"

#define TRILOGY_XBE "NinjaTrilogy.xbe"

/* RECOMP_TLS: the runtime's guest registers are thread-local (as in src/main.c) */
extern RECOMP_TLS uint32_t g_esp;
extern ptrdiff_t g_xbox_mem_offset;
extern void xbox_InputInit(void);
extern void xbe_entry_point(void);
extern int recomp_dispatch_init(void);
extern void xbox_kernel_init(void);
extern void xbox_kernel_bridge_init(void);
typedef struct MCPXAPUState MCPXAPUState;
extern MCPXAPUState *mcpx_apu_init_standalone(uint8_t *ram_ptr);
extern MCPXAPUState *g_apu_state;
extern bool apu_hook_handle_mmio(PCONTEXT ctx, uintptr_t fault_addr, uint32_t fault_xbox_va, int is_write);
#define APU_TRAP_BASE 0xFE800000u
#define APU_TRAP_END  0xFE880000u
extern RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi;

static char s_game_dir[4096], s_save_dir[4096];

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

/* The sound chip's and the controller ports' registers trap here to be serviced (as in src/main.c);
 * any other access violation is reported. */
static LONG CALLBACK veh_handler(PEXCEPTION_POINTERS ep)
{
#if defined(_WIN32) && XBOX_HAS_VCPU
    vcpu_win_unredirect(ep);
#endif
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    uintptr_t fault_addr = ep->ExceptionRecord->ExceptionInformation[1];
    int is_write = ep->ExceptionRecord->ExceptionInformation[0] ? 1 : 0;
    uint32_t xbox_va = (uint32_t)(fault_addr - (uintptr_t)g_xbox_mem_offset);
    if (g_apu_state && xbox_va >= APU_TRAP_BASE && xbox_va < APU_TRAP_END &&
        apu_hook_handle_mmio(ep->ContextRecord, fault_addr, xbox_va, is_write))
        return EXCEPTION_CONTINUE_EXECUTION;
    if (xbox_OhciOwnsAddress(xbox_va) && xbox_OhciHandleMmio(ep->ContextRecord, xbox_va))
        return EXCEPTION_CONTINUE_EXECUTION;
    fprintf(stderr, "[CRASH] access violation at RIP=0x%llX, address 0x%llX (%s), guest VA 0x%08X\n"
                    "  guest eax=%08X ecx=%08X edx=%08X ebx=%08X esi=%08X edi=%08X esp=%08X\n",
            (unsigned long long)ep->ContextRecord->Rip, (unsigned long long)fault_addr, is_write ? "write" : "read",
            xbox_va, g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi, g_esp);
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

static void *guest_main(void *unused)
{
    char xbe_path[4200];
    void *xbe_data = NULL;
    size_t xbe_size = 0;

    (void)unused;
#if XBOX_HAS_VCPU
    vcpu_attach(VCPU_PRIO_NORMAL);
#endif
    snprintf(xbe_path, sizeof xbe_path, "%s/" TRILOGY_XBE, s_game_dir);
    if (!load_file(xbe_path, &xbe_data, &xbe_size)) {
        fprintf(stderr, "Cannot read %s\n", xbe_path);
        exit(1);
    }
    printf("XBE loaded: %zu bytes\n", xbe_size);
    if (!xbox_MemoryLayoutInit(xbe_data, xbe_size)) {
        fprintf(stderr, "Failed to initialize the Xbox memory layout.\n");
        exit(1);
    }
    g_xbox_mem_offset = xbox_GetMemoryOffset();

    Nv2aVkOptions o = { .headless = getenv("NGB_RENDERER") && !strcmp(getenv("NGB_RENDERER"), "null") };
    if (xbox_Nv2aVkStart(&o) != 0) {
        fprintf(stderr, "Cannot start the GPU.\n");
        exit(1);
    }
    if (getenv("RECOMP_AC97_READY")) {
        g_apu_state = mcpx_apu_init_standalone((uint8_t *)xbox_GetMemoryBase());
        fprintf(stderr, "[BOOT] emulated APU %s\n", g_apu_state ? "up" : "FAILED to initialise");
    }
    xbox_kernel_init();
    {
        extern void xbox_path_init(const char *game_dir, const char *save_dir);
        printf("Save directory: %s\n", s_save_dir);
        xbox_path_init(s_game_dir, s_save_dir);
    }
    xbox_kernel_bridge_init();
    xbox_OhciInit();
    g_esp = XBOX_STACK_TOP;
    if (!recomp_dispatch_init())
        fprintf(stderr, "[BOOT] flat dispatch unavailable\n");
    printf("Starting " TRILOGY_XBE "...\n");
    xbe_entry_point();
    printf("Program returned.\n");
    exit(0);
    return NULL;
}

int main(int argc, char **argv)
{
    const char *g = getenv("NGB_GAME_DIR"), *s = getenv("NGB_SAVE_DIR");
    snprintf(s_game_dir, sizeof s_game_dir, "%s", argc > 1 ? argv[1] : g ? g : "game");
    if (argc > 2 || s)
        snprintf(s_save_dir, sizeof s_save_dir, "%s", argc > 2 ? argv[2] : s);
    else
        ngb_default_save_dir(s_save_dir, sizeof s_save_dir);
    ngb_launch_log_to_file(s_save_dir, "log-trilogy.txt");
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
#if defined(_WIN32)
    timeBeginPeriod(1);
#endif
    printf("=== Ninja Gaiden (NES) from Ninja Gaiden Black - Static Recompilation ===\n");
    printf("Game directory: %s\n", s_game_dir);
    AddVectoredExceptionHandler(1, veh_handler);

    /* Inherited from Ninja Gaiden Black, which draws with its own Direct3D renderer and has the GPU
     * model only run the pushbuffer: here the GPU model draws. */
#if defined(_WIN32)
    _putenv_s("NV2A_NO_DRAWS", "");
    _putenv_s("NV2A_NO_PREWARM", "");
#else
    unsetenv("NV2A_NO_DRAWS");
    unsetenv("NV2A_NO_PREWARM");
#endif
    if (getenv("NGB_RENDERER") && strcmp(getenv("NGB_RENDERER"), "null"))
#if defined(_WIN32)
        _putenv_s("NGB_RENDERER", "");
#else
        unsetenv("NGB_RENDERER");
#endif
    set_default("RECOMP_AC97_READY", "1");
    set_default("RECOMP_USB", "1");
    set_default("NV2A_TEX_FILTER", "original");
    ngb_launch_init(s_game_dir, s_save_dir, ngb_launch_window_w(), ngb_launch_window_h(), ngb_launch_fullscreen());

    int headless = getenv("NGB_RENDERER") && !strcmp(getenv("NGB_RENDERER"), "null");
    if (!headless && nv2a_window_create("Ninja Gaiden", ngb_launch_window_w(), ngb_launch_window_h(),
                                        ngb_launch_fullscreen(), 4.0 / 3.0, 1) != 0) {
        fprintf(stderr, "Cannot open the window.\n");
        return 1;
    }
    if (!headless)
        xbox_InputInit();

#if defined(_WIN32)
    CloseHandle(CreateThread(NULL, 64u << 20, (LPTHREAD_START_ROUTINE)guest_main, NULL, 0, NULL));
#else
    {
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
    if (!headless) {
        while (nv2a_window_pump())
            usleep(2000);
        printf("Window closed.\n");
        _exit(0);
    }
    for (;;)
#if defined(_WIN32)
        Sleep(INFINITE);
#else
        pause();
#endif
}
