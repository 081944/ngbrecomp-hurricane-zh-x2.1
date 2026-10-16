/*
 * hurricane_flags.c -- the globals the lifter's --flags-out uses (recomp_types.h).
 *
 * The Hurricane x2.1 build has a function that answers in its flags: the menu font's "does this string
 * have Chinese?" (0xDC7A30) ends `test ecx, ecx; popad; ret`, and its two callers branch on ZF. Listed in
 * src/flags_out.json, it stores its compare here before returning and they load it back.
 */
#include <stdint.h>

uint32_t g_flags_out_fa, g_flags_out_fb;
int32_t g_flags_out_fas, g_flags_out_fbs;
