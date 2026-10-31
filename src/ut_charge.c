/*
 * ut_charge.c -- Essence-Only Charging (Gameplay, #120): Ultimate Techniques charged by essence alone.
 *
 * Holding attack charges an Ultimate Technique. While Ryu is in a charging action (kind 0xB9: the
 * player block's +4, 0x4968B0 for the first player), sub_00023520 counts its frames in 0x5895E4 and
 * raises the charge level, 0x5895E2, each time the count reaches one of two marks (0x5895E6 and 0x5895E8,
 * about 100 and 220 frames, set by sub_00029140); the release then picks the plain attack, the
 * Ultimate Technique or the stronger one by that level (script opcode 0x83 in sub_00016300). Essence
 * absorbed while charging (sub_001E2EB0) raises the level at once and sets the count just past the mark
 * that level would have taken (0x65 or 0xDD). The Hurricane Packs brought the charge by time; before
 * them only essence charged.
 *
 * With the option on, the count's steps of a few frames are taken back each frame, so it never reaches a mark
 * by itself; essence's jumps are left alone, and so is everything else about the charge.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

int ngb_pc_setting(int which);   /* src/ingame_menu.c: 33 Essence-Only Charging */

/* Once a frame (src/cheats.c) */
void ngb_ut_charge_frame(void)
{
    static uint16_t last;
    uint16_t t = MEM16(0x5895E4);
    /* a step of a frame or a few, not essence's jump nor a new charge's 0 */
    if (ngb_pc_setting(33) && t > last && t - last < 8)
        MEM16(0x5895E4) = t = last;
    last = t;
}
