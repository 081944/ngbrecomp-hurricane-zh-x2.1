/* MemoryRegion as the NV2A code uses it: guest RAM (VRAM on the UMA Xbox) and
 * the register blocks' read/write callbacks. Dirty logging is answered by the
 * host (host/nv2a_dirty.c), not by QEMU's memory API. */
#ifndef NV2A_SHIM_MEMORY_H
#define NV2A_SHIM_MEMORY_H
#include "qemu/osdep.h"

enum { DIRTY_MEMORY_VGA = 0, DIRTY_MEMORY_CODE = 1, DIRTY_MEMORY_MIGRATION = 2,
       DIRTY_MEMORY_NV2A = 3, DIRTY_MEMORY_NV2A_TEX = 4,
       /* xboxrecomp: render-target memory, asked separately from vertex
        * data (DIRTY_MEMORY_NV2A) because the answers differ here. */
       DIRTY_MEMORY_NV2A_SURF = 5,
       /* xboxrecomp: "the CPU wrote into a cached surface", standing in for
        * the TCG access callback that xemu uses to invalidate a surface the
        * CPU overwrites (see pgraph/vk texture.c and surface.c). Like _SURF,
        * only writes from outside the GPU thread count. */
       DIRTY_MEMORY_NV2A_SURF_CPU = 6, DIRTY_MEMORY_NUM = 7 };

typedef struct MemoryRegionOps {
    uint64_t (*read)(void *opaque, hwaddr addr, unsigned size);
    void (*write)(void *opaque, hwaddr addr, uint64_t data, unsigned size);
} MemoryRegionOps;

typedef struct MemoryRegion {
    uint8_t *ram_ptr;
    uint64_t size;
} MemoryRegion;

static inline uint64_t memory_region_size(MemoryRegion *mr) { return mr->size; }
static inline void *memory_region_get_ram_ptr(MemoryRegion *mr) { return mr->ram_ptr; }

/* TCG-only CPU access callbacks; tcg_enabled() is always false here. */
typedef struct MemAccessCallback MemAccessCallback;
typedef struct CPUState CPUState;
typedef void (*MemAccessCallbackFn)(void *opaque, MemoryRegion *mr, hwaddr addr,
                                    hwaddr len, bool write);
static inline CPUState *qemu_get_cpu(int index) { (void)index; return NULL; }

/* Work QEMU would run on the vCPU thread with it paused (renderer switching).
 * The guest's threads cannot be borrowed here, so it gets its own thread. */
typedef struct { void *host_ptr; } run_on_cpu_data;
#define RUN_ON_CPU_HOST_PTR(p) ((run_on_cpu_data){ .host_ptr = (p) })
typedef void (*run_on_cpu_func)(CPUState *cpu, run_on_cpu_data data);
void async_safe_run_on_cpu(CPUState *cpu, run_on_cpu_func func,
                           run_on_cpu_data data);
static inline MemAccessCallback *mem_access_callback_insert(
    CPUState *cpu, MemoryRegion *mr, hwaddr offset, hwaddr len,
    MemAccessCallbackFn cb, void *opaque)
{ (void)cpu; (void)mr; (void)offset; (void)len; (void)cb; (void)opaque; return NULL; }
static inline void mem_access_callback_remove_by_ref(CPUState *cpu,
                                                     MemAccessCallback *cb)
{ (void)cpu; (void)cb; }

bool memory_region_test_and_clear_dirty(MemoryRegion *mr, hwaddr addr,
                                        hwaddr size, unsigned client);
void memory_region_set_client_dirty(MemoryRegion *mr, hwaddr addr,
                                    hwaddr size, unsigned client);
#endif
