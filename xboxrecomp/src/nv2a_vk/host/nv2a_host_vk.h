/*
 * Where xemu's Vulkan renderer calls out to this host for presentation.
 *
 * xemu hands its display image to an OpenGL UI through external memory. Here
 * the renderer presents straight to a window: the host supplies the window's
 * instance extensions and surface, device selection requires a queue that can
 * present to it, and pgraph_vk_sync() -- which already renders the scanout
 * surface into the display image -- presents that image. Everything that
 * touches the queue stays on the PFIFO thread, as the renderer assumes.
 */
#ifndef NV2A_HOST_VK_H
#define NV2A_HOST_VK_H
#include <stdbool.h>
#include <stdint.h>
#include <volk.h>

struct PGRAPHState;

/* Instance extensions the window system needs (NULL, 0 when headless). */
const char *const *nv2a_host_vk_instance_extensions(uint32_t *count);
/* Called once the instance exists: create the window surface. */
void nv2a_host_vk_instance_created(VkInstance instance);
/* Can this queue family present to the window? True when headless. */
bool nv2a_host_vk_can_present(VkPhysicalDevice dev, uint32_t queue_family);
/* After render_display(): show r->display.image. PFIFO thread. */
void nv2a_host_vk_present(struct PGRAPHState *pg);
/* Before the device goes away. */
void nv2a_host_vk_finalize(struct PGRAPHState *pg);
#endif
