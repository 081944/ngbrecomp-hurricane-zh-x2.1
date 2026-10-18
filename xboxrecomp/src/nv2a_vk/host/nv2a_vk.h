/*
 * nv2a_vk: xemu's NV2A model with its Vulkan renderer, for a recompiled title
 * whose own D3D drives the GPU through pushbuffers.
 */
#ifndef NV2A_VK_H
#define NV2A_VK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NV2AHostConfig {
    uint8_t *ram;            /* host address of guest physical 0 (= VRAM) */
    size_t ram_size;         /* 64 MiB on a retail Xbox */
    uint8_t *ramin;          /* 1 MiB of instance memory (PRAMIN) */
    void (*set_irq)(int level);   /* the GPU's interrupt line */
    int surface_scale;       /* render-target scale factor, 1 = native */
    double refresh_hz;       /* vblank rate, 0 = 60 */
    bool validation;         /* Vulkan validation layers */
    bool headless;           /* null renderer, no window */
    bool stats;              /* a status line every 5 s */
} NV2AHostConfig;

int nv2a_vk_init(const NV2AHostConfig *cfg);

/* Register access at an offset into the 16 MiB aperture (0xFD000000). */
uint64_t nv2a_vk_mmio_read(uint32_t off, unsigned size);
/* D3D_BlockOnTime's wait: until the GPU's progress (a semaphore word in memory, or DMA_GET) is no more
 * than `ahead` behind `put`, asleep meanwhile. */
void nv2a_vk_wait_progress(const uint32_t *word, uint32_t put, uint32_t ahead);
void nv2a_vk_wait_dma_get(uint32_t put, uint32_t ahead);
void nv2a_vk_mmio_write(uint32_t off, uint64_t val, unsigned size);

/* What the kernel's AvSetDisplayMode would have programmed. */
void nv2a_vk_set_display_mode(uint32_t mode, uint32_t format, uint32_t pitch,
                              uint32_t fb);

/* Make the PFIFO thread print its backtrace (Linux; for hang sampling). */
void nv2a_vk_sample_threads(void);

#ifdef __cplusplus
}
#endif
#endif
