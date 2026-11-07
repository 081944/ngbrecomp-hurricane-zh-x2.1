#include "recomp_funcs.h"
#include "costume_switch.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uint8_t guest[0xE00000];
uint32_t eax, ecx, edx, esp, ebx, esi, edi, g_ebp, g_seh_ebp;
RecompXmm g_xmm0, g_xmm1, g_xmm2, g_xmm3, g_xmm4, g_xmm5, g_xmm6, g_xmm7;
double g_fp_stack[8];
int g_fp_top;
static unsigned pauses, resumes, rebuilt, failed, require_early_pause;
static unsigned skeleton_phase;
static void ret(void) { esp += 4; }
void sub_001718E0(void) { if (require_early_pause) assert(MEM8(0xDB9AD5)); ret(); }
void sub_00173880(void) { ret(); }
void sub_00172D60(void) { eax = failed; ret(); }
void sub_002F62C0(void) { ++pauses; MEM8(0xDB9AD5) = 1; eax = 99; ret(); }
void sub_003077E0(void) { ret(); }
void sub_0016C5F0(void) { eax = 1; ret(); }
void sub_0016C690(void) { ret(); }
void sub_0016C570(void) { ret(); }
void sub_002EA5E0(void) {
    ++rebuilt;
    MEMF(0x4B5714) = 99999.0f;
    MEM32(0x4B752C) = 0x12345678; /* Freshly bound resource must survive. */
    ret();
}
void sub_002F6300(void) { ++resumes; MEM8(0xDB9AD5) = 0; ret(); }
void sub_002FE1D0(void) { assert(!"unexpected unfinished mock resource job"); }
void sub_001919F0(void) {
    assert(MEM8(0xDB9AD5)); /* Pose baking must precede gameplay/collision resume. */
    assert(MEM32(0xDA28F4) == 0x4B4E50 && MEM32(0xDA2900) == 0x809980);
    for (unsigned a = 0x80AF78; a <= 0x80AF84; a += 4)
        assert(MEM32(MEM32(a) + 0x400) == 0xFFFFFFFFu);
    skeleton_phase = 1;
    ret();
}
void sub_00188C40(void) {
    assert(skeleton_phase++ == 1);
    MEM32(0x809980 + 0x22A0) = 0xABCDEF;
    MEM16(0x809980 + 0x2644) = 60; /* The native animation may request a long blend. */
    ret();
}
void sub_00188E70(void) {
    assert(skeleton_phase++ == 2);
    MEM8(0x809980 + 0x22BC) = 1; /* Script selected a clip in the new resource. */
    MEM32(0x809980 + 0x170) = 0xBAD000; /* Cursor from the released costume. */
    ret();
}
void sub_0018E8B0(void) {
    assert(skeleton_phase++ == 3);
    assert(MEM8(0x809980 + 0x22BC) == 1);
    MEM8(0x809980 + 0x22BC) = 0;
    MEM32(0x809980 + 0x170) = 0xABC000;
    MEMF(0x809980 + 0x2344) = 1.0f; /* Native clip start, not frame zero. */
    ret();
}
void sub_0018EDF0(void) {
    assert(skeleton_phase++ == 4);
    assert(MEM32(0x809980 + 0x170) == 0xABC000);
    assert(MEMF(0x809980 + 0x2344) == 1.0f);
    assert(MEM16(0x809980 + 0x2644) == 0); /* Direct new pose, never blend old bones. */
    assert(MEMF(0x4B5714) == -17.0f);
    MEMF(0x4B4E50 + 0x1120) = 321.0f; /* A local bone sample, not world position. */
    ret();
}
void sub_00189EB0(void) { assert(skeleton_phase++ == 5); ret(); }
void sub_0018B110(void) { assert(!"unexpected mock skeleton layout"); }
void sub_0018B380(void) { assert(!"unexpected mock skeleton layout"); }
void sub_0018D290(void) { assert(skeleton_phase++ == 6); ret(); }
void sub_00191C30(void) { assert(skeleton_phase++ == 7); ret(); }
static void load(unsigned fallback) {
    esp = 0x10000;
    eax = fallback;
    PUSH32(esp, 0xDEADBEEF);
    sub_002FC6F0();
    assert(esp == 0x10000); /* Both normal and early-resume paths balance the guest stack. */
}
int main(void) {
    MEM16(0xDB9C6E) = 12;
    MEM8(0xD6A100) = 0xFF;
    MEM8(0x589761) = 0xFF;
    MEM8(0x589760) = 1;
    /* Initial loading may bypass the native costume-change hook entirely. */
    assert(ngb_costume_can_request(0xFF));
    for (unsigned p = 0xDA5678; p < 0xDB19F8; p += 0x61C) MEM8(p) = 3;
    MEMF(0x4B5710) = 123.0f;
    MEMF(0x4B5714) = -17.0f;
    MEMF(0x4B5718) = 8.0f;
    MEMF(0x4B571C) = 1.0f;
    for (unsigned i = 0; i < 7; ++i) {
        static const unsigned roots[] = {0x4B5710,0x4B5720,0x4B5730,0x4B6730,0x4B6760,0x4B6EF0,0x4B6F20};
        memcpy(XBOX_PTR(roots[i]), XBOX_PTR(0x4B5710), 16);
    }
    for (unsigned i = 0; i < 4; ++i) {
        unsigned buffer = 0x80AF90 + (i % 3) * 0x430;
        MEM32(0x80AF78 + i * 4) = buffer;
        MEM32(buffer + 0x400) = 1; /* Old model cached under the same frame number. */
    }
    uint8_t task[0x14];
    memset(XBOX_PTR(0xDA28F0), 0x56, sizeof task);
    memcpy(task, XBOX_PTR(0xDA28F0), sizeof task);
    require_early_pause = 1;
    assert(ngb_costume_request(4));
    load(4);
    assert(MEM8(0x589760) == 4 && rebuilt == 1);
    assert(pauses == 1 && resumes == 1 && !MEM8(0xDB9AD5));
    assert(MEMF(0x4B5714) == -17.0f && MEMF(0x4B5724) == -17.0f);
    assert(MEM32(0x4B752C) == 0x12345678);
    assert(skeleton_phase == 8 && MEMF(0x4B4E50 + 0x1120) == 321.0f);
    assert(ngb_costume_can_request(0xFF)==0); /* Native request not consumed yet. */
    uint32_t saved_flags;
    memcpy(&saved_flags, task + 8, sizeof saved_flags);
    assert(MEM8(0xDA28F0) == task[0] && MEM32(0xDA28F8) == saved_flags);
    assert(MEM32(0xDA28F4) == 0x4B4E50 && MEM32(0xDA28FC) == 0x58A3A0 + MEM8(0x4B7640) * 0x210u);
    assert(MEM32(0xDA2900) == 0x809980);
    MEM8(0xD6A100) = 0xFF;
    assert(ngb_costume_can_request(0xFF));
    /* Repeating the equipped costume must not touch an uninitialized snapshot. */
    assert(ngb_costume_request(4));
    load(4);
    assert(pauses == 1 && resumes == 1 && MEMF(0x4B5714) == -17.0f);
    MEM8(0xD6A100) = 0xFF;
    failed = 1;
    assert(ngb_costume_request(5));
    load(5);
    assert(pauses == 2 && resumes == 2 && !MEM8(0xDB9AD5));
    assert(rebuilt == 1 && MEMF(0x4B5714) == -17.0f);
    /* Return to the startup model, even though no original loader call was seen. */
    failed = 0;
    MEM8(0xD6A100) = 0xFF;
    assert(ngb_costume_request(0xFF));
    assert(MEM8(0x589761) == 0xFF && MEM8(0xD6A100) == 1);
    load(1);
    assert(MEM8(0x589760) == 1 && rebuilt == 2);
    assert(pauses == 3 && resumes == 3 && MEMF(0x4B5714) == -17.0f);
    /* Scripted changes retain retail behavior, including their own transforms. */
    failed = require_early_pause = 0;
    MEM8(0x589761) = 0xFF;
    MEM32(0x4B752C) = 0;
    load(6);
    assert(pauses == 4 && resumes == 4 && MEMF(0x4B5714) == 99999.0f);
    puts("costume loader ordering, root restoration, no-op and failure checks passed");
    return 0;
}
