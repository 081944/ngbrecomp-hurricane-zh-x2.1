/* A VGA register file: enough of QEMU's VGACommonState for the NV2A's PRMCIO,
 * PRMVIO and PRMDIO blocks (CRTC, sequencer, graphics and attribute
 * controllers behind their index ports) and for the display path, which asks
 * it for the line offset and resolution. The resolution is not derived from
 * CRTC timings: the host sets it from the mode AvSetDisplayMode was given,
 * because no Xbox BIOS runs here to program those timings. */
#ifndef NV2A_SHIM_VGA_INT_H
#define NV2A_SHIM_VGA_INT_H
#include "qemu/osdep.h"
#include "hw/display/vga_regs.h"

typedef struct VGADisplayParams {
    uint32_t line_offset;
    uint32_t start_addr;
    uint32_t line_compare;
} VGADisplayParams;

typedef struct VGACommonState VGACommonState;
struct VGACommonState {
    uint8_t sr_index, sr[256];
    uint8_t gr_index, gr[256];
    uint8_t ar_index, ar[21];
    int ar_flip_flop;
    uint8_t cr_index, cr[256];
    uint8_t msr, fcr, st00, st01;
    bool enable_overlay;
    /* Set by the host from the display mode. */
    int width, height;
    uint32_t line_offset;
    void (*get_params)(VGACommonState *s, VGADisplayParams *params);
    void (*get_resolution)(VGACommonState *s, int *pwidth, int *pheight);
    int (*get_bpp)(VGACommonState *s);
};

typedef struct GraphicHwOps { void (*gfx_update)(void *opaque); } GraphicHwOps;

uint32_t vga_ioport_read(void *opaque, uint32_t addr);
void vga_ioport_write(void *opaque, uint32_t addr, uint32_t val);
static inline void vga_invalidate(VGACommonState *s) { (void)s; }
static inline void vga_invalidate_scanlines(VGACommonState *s, int y1, int y2)
{ (void)s; (void)y1; (void)y2; }
#endif
