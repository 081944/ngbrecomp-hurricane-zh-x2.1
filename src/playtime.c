/*
 * playtime.c -- Total Play Time, kept sane.
 *
 * The game keeps the time played as milliseconds at 0xD7D2C0 (loaded from
 * the save, 0xDB9380) and reports it (sub_001DB330) as that plus the
 * kernel tick count, capped at 999:59. Builds before the tick count started
 * at the console's boot handed the game the PC's uptime, so saves made with
 * them hold the cap, and the game shows 999:59 for ever after. A total at
 * the cap is taken for that and started again from 0; no real save gets
 * there (the lifted function is renamed playtime_get, behind a wrapper).
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <stdio.h>

#define PLAYTIME_CAP 3599940000u          /* 999:59 in ms */

static void playtime_get(void);

void sub_001DB330(void)
{
    if (MEM32(0xD7D2C0) >= PLAYTIME_CAP) {
        fprintf(stderr, "[PLAYTIME] the saved total was the 999:59 cap (an old build's): from 0\n");
        MEM32(0xD7D2C0) = 0;
        if (MEM32(0xDB9380) >= PLAYTIME_CAP)
            MEM32(0xDB9380) = 0;
    }
    playtime_get();
}

/**
 * sub_001DB330
 * Original: 0x001DB330 - 0x001DB4A1 (369 bytes, 115 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void playtime_get(void)
{
    RECOMP_ENTRY(0x001DB330u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_001DB330: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(0x41CCD8);
    ecx = MEM32(0xD7D2C0);
    MEM32(esp + 8) = eax;
    eax = 0x95217CB1u;
    { uint64_t _r = (uint64_t)eax * (uint64_t)ecx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); }
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    esi = edx;
    esi = esi >> 0x15;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = ZX16(LO16(esi));
    eax = ebx;
    eax = (uint32_t)((int32_t)eax * (int32_t)0xFFC91180u);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = 0x45E7B273;
    { uint64_t _r = (uint64_t)eax * (uint64_t)ecx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); }
    PUSH32(esp, edi);
    edi = edx;
    edi = edi >> 0xE;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebp = ZX16(LO16(edi));
    edx = ebp;
    edx = (uint32_t)((int32_t)edx * (int32_t)0xEA60);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = 0x10624DD3;
    { uint64_t _r = (uint64_t)eax * (uint64_t)ecx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); }
    edx = edx >> 6;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO16(esi)) & 0xFFFFu; _fb = (uint32_t)(0x3E7) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(esi), 0x3E7 (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_001DB394; /* jbe: below or equal (unsigned <=) */

loc_001DB389: ;
    SET_LO16(eax, 0x3E7);
    MEM16(esp + 0x10) = LO16(eax);
    goto loc_001DB39C;

loc_001DB394: ;
    MEM16(esp + 0x10) = LO16(ebx);
    SET_LO16(eax, LO16(ebx));

loc_001DB39C: ;
    _fa = (uint32_t)(LO16(edi)) & 0xFFFFu; _fb = (uint32_t)(0x3B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(edi), 0x3B (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_001DB3AD; /* jbe: below or equal (unsigned <=) */

loc_001DB3A2: ;
    SET_LO16(ecx, 0x3B);
    MEM16(esp + 0x12) = LO16(ecx);
    goto loc_001DB3B5;

loc_001DB3AD: ;
    MEM16(esp + 0x12) = LO16(ebp);
    SET_LO16(ecx, LO16(ebp));

loc_001DB3B5: ;
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fb = (uint32_t)(0x3B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(edx), 0x3B (16-bit) */
    MEM16(esp + 0x14) = 0x3B;
    if (CMP_A(_fa, _fb)) goto loc_001DB3C7; /* ja: above (unsigned >) */

loc_001DB3C2: ;
    MEM16(esp + 0x14) = LO16(edx);

loc_001DB3C7: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x3E7) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x3E7 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001DB3F4; /* jne: not equal / not zero */

loc_001DB3CD: ;
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0x3B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), 0x3B (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001DB3F4; /* jne: not equal / not zero */

loc_001DB3D3: ;
    eax = ZX16(LO16(eax));
    eax = (uint32_t)((int32_t)eax * (int32_t)0x3C);
    POP32(esp, edi);
    ecx = ZX16(LO16(ecx));
    POP32(esp, esi);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, ebp);
    eax = (uint32_t)((int32_t)eax * (int32_t)0xEA60);
    POP32(esp, ebx);
    ecx = MEM32(esp + 8);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    g_seh_ebp = ebp; sub_00379C29(); return; /* tail jmp 0x00379C29 */

loc_001DB3F4: ;
    PUSH32(esp, 0x001DB3F9u); RECOMP_ABI_CALL(0x00329B27u, sub_00329B27); /* call 0x00329B27 */

loc_001DB3F9: ;
    ecx = eax;
    eax = 0x95217CB1u;
    { uint64_t _r = (uint64_t)eax * (uint64_t)ecx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); }
    esi = edx;
    esi = esi >> 0x15;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = ZX16(LO16(esi));
    edx = ebx;
    edx = (uint32_t)((int32_t)edx * (int32_t)0xFFC91180u);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = 0x45E7B273;
    { uint64_t _r = (uint64_t)eax * (uint64_t)ecx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); }
    edi = edx;
    edi = edi >> 0xE;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebp = ZX16(LO16(edi));
    eax = ebp;
    eax = (uint32_t)((int32_t)eax * (int32_t)0xEA60);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = 0x10624DD3;
    { uint64_t _r = (uint64_t)eax * (uint64_t)ecx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); }
    edx = edx >> 6;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO16(esi)) & 0xFFFFu; _fb = (uint32_t)(0x3E7) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(esi), 0x3E7 (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_001DB442; /* jbe: below or equal (unsigned <=) */

loc_001DB43E: ;
    SET_LO16(ebx, 0x3E7);

loc_001DB442: ;
    _fa = (uint32_t)(LO16(edi)) & 0xFFFFu; _fb = (uint32_t)(0x3B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(edi), 0x3B (16-bit) */
    SET_LO16(eax, LO16(ebx));
    SET_LO16(esi, 0x3B);
    if (CMP_A(_fa, _fb)) goto loc_001DB452; /* ja: above (unsigned >) */

loc_001DB44F: ;
    SET_LO16(esi, LO16(ebp));

loc_001DB452: ;
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fb = (uint32_t)(0x3B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(edx), 0x3B (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_001DB45C; /* jbe: below or equal (unsigned <=) */

loc_001DB458: ;
    SET_LO16(edx, 0x3B);

loc_001DB45C: ;
    _fb = (uint32_t)(MEM16(esp + 0x10)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(eax, LO16(eax) + MEM16(esp + 0x10));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    _fb = (uint32_t)(MEM16(esp + 0x12)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(esi, LO16(esi) + MEM16(esp + 0x12));
    _fa = (uint32_t)(LO16(esi)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    SET_LO16(ecx, LO16(edx));
    _fb = (uint32_t)(MEM16(esp + 0x14)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ecx, LO16(ecx) + MEM16(esp + 0x14));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x3E7) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x3E7 (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_001DB478; /* jbe: below or equal (unsigned <=) */

loc_001DB474: ;
    SET_LO16(eax, 0x3E7);

loc_001DB478: ;
    eax = ZX16(LO16(eax));
    eax = (uint32_t)((int32_t)eax * (int32_t)0x3C);
    edx = ZX16(LO16(esi));
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = (uint32_t)((int32_t)eax * (int32_t)0x3C);
    POP32(esp, edi);
    ecx = ZX16(LO16(ecx));
    POP32(esp, esi);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = MEM32(esp + 0x10);
    eax = (uint32_t)((int32_t)eax * (int32_t)0x3E8);
    POP32(esp, ebp);
    POP32(esp, ebx);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    g_seh_ebp = ebp; sub_00379C29(); return; /* tail jmp 0x00379C29 */

}

