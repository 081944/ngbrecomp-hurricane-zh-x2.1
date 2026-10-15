#ifndef NV2A_SHIM_VGA_REGS_H
#define NV2A_SHIM_VGA_REGS_H
#define VGA_ATT_W        0x3C0
#define VGA_ATT_R        0x3C1
#define VGA_MIS_W        0x3C2
#define VGA_SEQ_I        0x3C4
#define VGA_SEQ_D        0x3C5
#define VGA_PEL_MSK      0x3C6
#define VGA_PEL_IR       0x3C7
#define VGA_PEL_IW       0x3C8
#define VGA_PEL_D        0x3C9
#define VGA_MIS_R        0x3CC
#define VGA_GFX_I        0x3CE
#define VGA_GFX_D        0x3CF
#define VGA_CRT_IC       0x3D4
#define VGA_CRT_DC       0x3D5
#define VGA_IS1_RC       0x3DA
#define VGA_CRT_IM       0x3B4
#define VGA_CRT_DM       0x3B5
#define VGA_IS1_RM       0x3BA
#define VGA_AR_ENABLE_DISPLAY 0x20
#define VGA_MIS_COLOR    0x01
#define VGA_SEQ_CLOCK_MODE 0x01
#define VGA_SR01_SCREEN_OFF 0x20
#define VGA_CRTC_OVERFLOW 0x07
#define VGA_CRTC_MAX_SCAN 0x09
#define VGA_CRTC_LINE_COMPARE 0x18
#endif
