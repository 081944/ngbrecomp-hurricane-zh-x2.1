/* Start the xemu NV2A model inside a recompiled title (host/nv2a_runtime.c).
 * Call after xbox_MemoryLayoutInit and before the guest runs; the window, if
 * any, must already exist (nv2a_window_create). */
#ifndef NV2A_VK_RUNTIME_H
#define NV2A_VK_RUNTIME_H
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Nv2aVkOptions {
    int surface_scale;     /* 0 = NV2A_SCALE or 1 */
    double refresh_hz;     /* 0 = 60 */
    bool headless;         /* no window: null renderer */
} Nv2aVkOptions;

int xbox_Nv2aVkStart(const Nv2aVkOptions *opt);

#ifdef __cplusplus
}
#endif
#endif
