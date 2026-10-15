/* xemu's settings, reduced to what the NV2A code reads. The host fills
 * g_config before the GPU starts (host/nv2a_host.c). */
#ifndef NV2A_SHIM_XEMU_SETTINGS_H
#define NV2A_SHIM_XEMU_SETTINGS_H
#include "qemu/osdep.h"
typedef enum CONFIG_DISPLAY_RENDERER {
    CONFIG_DISPLAY_RENDERER_NULL,
    CONFIG_DISPLAY_RENDERER_OPENGL,
    CONFIG_DISPLAY_RENDERER_VULKAN,
    CONFIG_DISPLAY_RENDERER__COUNT
} CONFIG_DISPLAY_RENDERER;
struct config {
    struct {
        CONFIG_DISPLAY_RENDERER renderer;
        struct {
            bool validation_layers;
            bool debug_shaders;
            bool assert_on_validation_msg;
            const char *preferred_physical_device;
        } vulkan;
        struct { int surface_scale; } quality;
    } display;
};
extern struct config g_config;
static inline void xemu_settings_set_string(const char **str, const char *new_str)
{
    *str = g_strdup(new_str);
}
#endif
