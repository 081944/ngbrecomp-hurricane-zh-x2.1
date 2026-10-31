/*
 * intercept.c -- Intercept, the Hurricane Pack's guard counter, back (Gameplay > Intercept, #109).
 *
 * Hurricane Pack 1 gave Ryu the Intercept: guard tapped just as an attack lands, the attack is turned
 * aside, and an instant charged attack can follow. Black kept its Technique scroll, Ryu's move for it and
 * the karma it pays on Very Hard and Master Ninja, and dropped one instruction: in sub_0001ACD0, which
 * turns the pad into Ryu's next action (bl), a guard press (5) with an attack about to land
 * (sub_0003E400) and guard pressed this frame (sub_0001B940, button 3, 1 frame) but not the 14 before it
 * (0x0E) -- a tap, not a held guard -- was followed by `bl = 0x0D`, the Intercept; Black's code still
 * branches around the place it was (a `jne` to the next instruction). Here it is put back. The guard press is
 * only an Intercept while Ryu has its scroll (sub_0003E400 asks the inventory for item 0x26), which Black
 * gives but then never used: with the option on he needs none, and his inventory and saves are left alone.
 *
 * Two kinds: Black, the move Black's own data has for it -- Ryu holds his ground; and Hurricane Pack, the
 * Hurricane Pack's -- he is pushed back, with a longer window for the charged attack after. The second is
 * four of Ryu's move records and five bytes of his loaded character data (chr_dat2, at *0x4B7528) changed,
 * set once a frame while chosen and put back otherwise, and only when they hold one kind's values (another
 * character's data is never touched).
 *
 * While it is on, an Intercept pays its karma (1000, at most 50 a battle: sub_002EBB00) on
 * every difficulty, not only Very Hard and Master Ninja, and the battle's results show it again -- an
 * "Intercept" line under Karma, Combo, Kill and Time, drawn by sub_00208120 before Ultimate Techniques from
 * the counts Black still keeps (0x8DB734, 0x8DB738); Black only stopped drawing them.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

int ngb_pc_setting(int which);   /* src/ingame_menu.c: 31 Intercept, 0 off, 1 Black, 2 Hurricane Pack */

/* Ryu's Intercept moves in chr_dat2: four 12-byte records 0x30 apart from 0xF120, their bytes 2-3 the move
 * and 6-7 its kind; and the kind again at five more places, each after a 0x17 */
static const uint16_t k_move[2][4] = { { 0x08EC, 0x08ED, 0x08EE, 0x08EF },    /* Black */
                                       { 0x0099, 0x0701, 0x0797, 0x0891 } };  /* Hurricane Pack */
static const uint8_t k_kind[2][2] = { { 0xBF, 0xFF }, { 0xB8, 0x02 } };
static const uint32_t k_kind_at[5] = { 0x10ED8, 0x1146D, 0x1179F, 0x11AB5, 0x11DE4 };

/* Which kind the data holds now: 0 Black, 1 Hurricane Pack, -1 neither (not Ryu's, or not loaded) */
static int intercept_kind(uint32_t d)
{
    if (MEM32(d) != 0x5F726863u || MEM32(d + 4) != 0x32746164u)   /* "chr_dat2" */
        return -1;
    for (int k = 0; k < 2; k++) {
        int ok = 1;
        for (int i = 0; i < 4 && ok; i++)
            ok = MEM16(d + 0xF122 + 0x30 * i) == k_move[k][i] && MEM8(d + 0xF126 + 0x30 * i) == k_kind[k][0] &&
                 MEM8(d + 0xF127 + 0x30 * i) == k_kind[k][1];
        for (int i = 0; i < 5 && ok; i++)
            ok = MEM8(d + k_kind_at[i]) == k_kind[k][0] && MEM8(d + k_kind_at[i] - 1) == 0x17;
        if (ok)
            return k;
    }
    return -1;
}

/* Once a frame (src/cheats.c): Ryu's data holding the chosen kind's moves */
void ngb_intercept_frame(void)
{
    uint32_t d = MEM32(0x4B7528);
    if (d < 0x10000)
        return;
    int want = ngb_pc_setting(31) == 2, have = intercept_kind(d);
    if (have < 0 || have == want)
        return;
    for (int i = 0; i < 4; i++) {
        MEM16(d + 0xF122 + 0x30 * i) = k_move[want][i];
        MEM8(d + 0xF126 + 0x30 * i) = k_kind[want][0];
        MEM8(d + 0xF127 + 0x30 * i) = k_kind[want][1];
    }
    for (int i = 0; i < 5; i++)
        MEM8(d + k_kind_at[i]) = k_kind[want][0];
}

/**
 * sub_0001ACD0
 * Original: 0x0001ACD0 - 0x0001AFD0 (768 bytes, 231 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_0001ACD0(void)
{
    RECOMP_ENTRY(0x0001ACD0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int _cf = 0; /* carry flag */
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_0001ACD0: ;
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0xC);
    PUSH32(esp, esi);
    esi = MEM32(0xDB9E9C);
    SET_LO8(eax, MEM8(esi + 0x6B0));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    PUSH32(esp, edi);
    SET_LO8(ebx, 1);
    if (CMP_EQ(_fa, _fb)) goto loc_0001ACF1; /* je: equal / zero */

loc_0001ACEA: ;
    SET_LO8(ebx, 0xC);
    goto loc_0001AE58;

loc_0001ACF1: ;
    edi = MEM32(0xDBA5C0);
    _fa = (uint32_t)(MEM16(edi + 0x1C6)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(edi + 0x1C6), 0 (16-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AD08; /* jne: not equal / not zero */

loc_0001AD01: ;
    SET_LO8(ebx, 2);
    goto loc_0001AE58;

loc_0001AD08: ;
    eax = ZX8(MEM8(esi + 6));
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xE) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xE (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_0001AF19; /* ja: above (unsigned >) */

loc_0001AD16: ;
    { uint32_t _jt = MEM32(eax * 4 + 0x1AF90); /* switch: 15 entries, 13 targets */
    if (_jt == 0x0001AD1Du) goto loc_0001AD1D;
    if (_jt == 0x0001AD24u) goto loc_0001AD24;
    if (_jt == 0x0001AD39u) goto loc_0001AD39;
    if (_jt == 0x0001AD40u) goto loc_0001AD40;
    if (_jt == 0x0001AD73u) goto loc_0001AD73;
    if (_jt == 0x0001AD7Au) goto loc_0001AD7A;
    if (_jt == 0x0001ADB5u) goto loc_0001ADB5;
    if (_jt == 0x0001ADCBu) goto loc_0001ADCB;
    if (_jt == 0x0001ADF7u) goto loc_0001ADF7;
    if (_jt == 0x0001AE47u) goto loc_0001AE47;
    if (_jt == 0x0001AEF8u) goto loc_0001AEF8;
    if (_jt == 0x0001AF65u) goto loc_0001AF65;
    if (_jt == 0x0001AF87u) goto loc_0001AF87;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_0001AD1D: ;
    SET_LO8(ebx, 3);
    goto loc_0001AE58;

loc_0001AD24: ;
    SET_LO8(eax, MEM8(esi + 0x201));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AF19; /* jne: not equal / not zero */

loc_0001AD32: ;
    SET_LO8(ebx, 6);
    goto loc_0001AE49;

loc_0001AD39: ;
    SET_LO8(ebx, 0xB);
    goto loc_0001AE58;

loc_0001AD40: ;
    SET_LO8(eax, MEM8(esi + 0x201));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0001AD73; /* je: equal / zero */

loc_0001AD4A: ;
    SET_LO8(ecx, MEM8(esi + 0x202));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ecx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ecx) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_0001AD73; /* ja: above (unsigned >) */

loc_0001AD54: ;
    if (CMP_NE(_fa, _fb)) goto loc_0001AF19; /* jne: not equal / not zero */

loc_0001AD5A: ;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(eax, MEM16(0xDB9C70));
    PUSH32(esp, eax);
    PUSH32(esp, 0x0001AD68u); RECOMP_ABI_CALL(0x0003A100u, sub_0003A100); /* call 0x0003A100 */

loc_0001AD68: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0001AF19; /* je: equal / zero */

loc_0001AD73: ;
    SET_LO8(ebx, 7);
    goto loc_0001AE58;

loc_0001AD7A: ;
    SET_LO8(eax, MEM8(esi + 0x201));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AD8B; /* jne: not equal / not zero */

loc_0001AD84: ;
    SET_LO8(ebx, 7);
    goto loc_0001AE58;

loc_0001AD8B: ;
    SET_LO8(ecx, MEM8(esi + 0x202));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ecx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ecx) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_0001ADB5; /* ja: above (unsigned >) */

loc_0001AD95: ;
    if (CMP_NE(_fa, _fb)) goto loc_0001AF19; /* jne: not equal / not zero */

loc_0001AD9B: ;
    _cf = 0; /* xor clears CF */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(ecx, MEM16(0xDB9C70));
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0001ADAAu); RECOMP_ABI_CALL(0x0003A100u, sub_0003A100); /* call 0x0003A100 */

loc_0001ADAA: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0001AF19; /* je: equal / zero */

loc_0001ADB5: ;
    _fa = (uint32_t)(MEM8(esp + 0x1C)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(esp + 0x1C), 0x10 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    SET_LO8(ebx, 8);
    if (TEST_Z(_fa, _fb)) goto loc_0001AE58; /* je: equal / zero */

loc_0001ADC2: ;
    _fa = (uint32_t)(MEM8(esi + 6)) & 0xFFu; _fb = (uint32_t)(0xE) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 6), 0xE (8-bit) */
    _cf = (int)(_fa < _fb);
    goto loc_0001AE54;

loc_0001ADCB: ;
    SET_LO8(eax, MEM8(esi + 0x201));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0001ADF7; /* je: equal / zero */

loc_0001ADD5: ;
    SET_LO8(ecx, MEM8(esi + 0x202));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ecx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ecx) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_0001ADF7; /* ja: above (unsigned >) */

loc_0001ADDF: ;
    if (CMP_NE(_fa, _fb)) goto loc_0001ADFB; /* jne: not equal / not zero */

loc_0001ADE1: ;
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(edx, MEM16(0xDB9C70));
    PUSH32(esp, edx);
    PUSH32(esp, 0x0001ADF0u); RECOMP_ABI_CALL(0x0003A100u, sub_0003A100); /* call 0x0003A100 */

loc_0001ADF0: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0001ADFB; /* je: equal / zero */

loc_0001ADF7: ;
    SET_LO8(ebx, 9);
    goto loc_0001AE58;

loc_0001ADFB: ;
    SET_LO8(eax, MEM8(esi + 0x201));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(MEM8(esi + 0x202)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), MEM8(esi + 0x202) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_0001AF19; /* jne: not equal / not zero */

loc_0001AE0D: ;
    SET_LO16(edx, MEM16(edi + 0x1D0));
    PUSH32(esp, 0x0001AE19u); RECOMP_ABI_CALL(0x001551E0u, sub_001551E0); /* call 0x001551E0 */

loc_0001AE19: ;
    eax = ZX16(LO16(eax));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 2 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_0001AE26; /* je: equal / zero */

loc_0001AE21: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xD (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_0001AE30; /* jne: not equal / not zero */

loc_0001AE26: ;
    _fa = (uint32_t)(MEM16(esi + 0x1FA)) & 0xFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esi + 0x1FA), 0xD (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_0001AE3E; /* jb: below (unsigned <) */

loc_0001AE30: ;
    _fa = (uint32_t)(MEM16(esi + 0x1FA)) & 0xFFFFu; _fb = (uint32_t)(6) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esi + 0x1FA), 6 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_0001AF19; /* jae: above or equal (unsigned >=) */

loc_0001AE3E: ;
    eax = MEM32(0x47A568);

loc_0001AE43: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0001ADF7; /* je: equal / zero */

loc_0001AE47: ;
    SET_LO8(ebx, 5);

loc_0001AE49: ;
    _fa = (uint32_t)(MEM8(esp + 0x1C)) & 0xFFu; _fb = (uint32_t)(8) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(esp + 0x1C), 8 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (TEST_Z(_fa, _fb)) goto loc_0001AE58; /* je: equal / zero */

loc_0001AE50: ;
    _fa = (uint32_t)(MEM8(esi + 6)) & 0xFFu; _fb = (uint32_t)(0xD) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 6), 0xD (8-bit) */
    _cf = (int)(_fa < _fb);

loc_0001AE54: ;
    if (CMP_EQ(_fa, _fb)) goto loc_0001AE58; /* je: equal / zero */

loc_0001AE56: ;
    SET_LO8(ebx, 1);

loc_0001AE58: ;
    edi = MEM32(0xDBA5F8);
    _fa = (uint32_t)(MEM16(edi + 0x2774)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(edi + 0x2774), 0 (16-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AEF1; /* jne: not equal / not zero */

loc_0001AE6C: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(5) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ebx), 5 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_0001AEA7; /* jne: not equal / not zero */

loc_0001AE71: ;
    PUSH32(esp, 0x26);
    ecx = edi;
    PUSH32(esp, 0x0001AE7Au); RECOMP_ABI_CALL(0x0003E400u, sub_0003E400); /* call 0x0003E400 */

loc_0001AE7A: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb) && !ngb_pc_setting(31)) goto loc_0001AEA7; /* je: equal / zero */

loc_0001AE81: ;
    PUSH32(esp, 1);
    edx = 3;
    PUSH32(esp, 0x0001AE8Du); RECOMP_ABI_CALL(0x0001B940u, sub_0001B940); /* call 0x0001B940 */

loc_0001AE8D: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0001AEA7; /* je: equal / zero */

loc_0001AE94: ;
    PUSH32(esp, 0xE);
    edx = 3;
    PUSH32(esp, 0x0001AEA0u); RECOMP_ABI_CALL(0x0001B940u, sub_0001B940); /* call 0x0001B940 */

loc_0001AEA0: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AEA7; /* jne: not equal / not zero */
    if (ngb_pc_setting(31))
        SET_LO8(ebx, 0xD);   /* the step Black removed (its `jne` jumped over nothing): an Intercept */

loc_0001AEA7: ;
    _fa = (uint32_t)(MEM16(edi + 0x2774)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(edi + 0x2774), 0 (16-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AEF1; /* jne: not equal / not zero */

loc_0001AEB1: ;
    SET_LO8(eax, MEM8(esi + 0x640));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AEF1; /* jne: not equal / not zero */

loc_0001AEBB: ;
    SET_LO8(eax, MEM8(esi + 0x6B0));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AEF1; /* jne: not equal / not zero */

loc_0001AEC5: ;
    PUSH32(esp, 1);
    PUSH32(esp, 0);
    eax = 0x148;
    PUSH32(esp, 0x0001AED3u); RECOMP_ABI_CALL(0x0001B9A0u, sub_0001B9A0); /* call 0x0001B9A0 */

loc_0001AED3: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0001AEF1; /* je: equal / zero */

loc_0001AEDA: ;
    PUSH32(esp, 0xE);
    PUSH32(esp, 0);
    eax = 0x148;
    PUSH32(esp, 0x0001AEE8u); RECOMP_ABI_CALL(0x0001B9A0u, sub_0001B9A0); /* call 0x0001B9A0 */

loc_0001AEE8: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    SET_LO8(eax, 0xD);
    if (CMP_EQ(_fa, _fb)) goto loc_0001AEF3; /* je: equal / zero */

loc_0001AEF1: ;
    SET_LO8(eax, LO8(ebx));

loc_0001AEF3: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_0001AEF8: ;
    SET_LO8(eax, MEM8(esi + 0x640));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AF3A; /* jne: not equal / not zero */

loc_0001AF02: ;
    PUSH32(esp, 1);
    edx = 3;
    PUSH32(esp, 0x0001AF0Eu); RECOMP_ABI_CALL(0x0001B940u, sub_0001B940); /* call 0x0001B940 */

loc_0001AF0E: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AE47; /* jne: not equal / not zero */

loc_0001AF19: ;
    SET_LO8(eax, MEM8(esi + 0x640));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AF3A; /* jne: not equal / not zero */

loc_0001AF23: ;
    SET_LO8(eax, MEM8(esi + 0x6B0));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AF3A; /* jne: not equal / not zero */

loc_0001AF2D: ;
    eax = MEM32(0x47A568);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AE47; /* jne: not equal / not zero */

loc_0001AF3A: ;
    ecx = MEM32(esp + 0x18);
    PUSH32(esp, ecx);
    eax = ebp;
    PUSH32(esp, 0x0001AF46u); RECOMP_ABI_CALL(0x0003DF70u, sub_0003DF70); /* call 0x0003DF70 */

loc_0001AF46: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0001AE58; /* je: equal / zero */

loc_0001AF51: ;
    eax = ebp;
    PUSH32(esp, 0x0001AF58u); RECOMP_ABI_CALL(0x00031FA0u, sub_00031FA0); /* call 0x00031FA0 */

loc_0001AF58: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001AE58; /* jne: not equal / not zero */

loc_0001AF60: ;
    goto loc_0001AE47;

loc_0001AF65: ;
    SET_LO8(eax, MEM8(esi + 0x640));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_0001ADF7; /* jne: not equal / not zero */

loc_0001AF73: ;
    PUSH32(esp, 1);
    edx = 3;
    PUSH32(esp, 0x0001AF7Fu); RECOMP_ABI_CALL(0x0001B940u, sub_0001B940); /* call 0x0001B940 */

loc_0001AF7F: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_0001AE43;

loc_0001AF87: ;
    SET_LO8(ebx, 0xD);
    goto loc_0001AE58;

    edi = edi;
    _fa = (uint32_t)(MEM32(ebp + -1390608383)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebp + -1390608383), ebp (32-bit) */
    _cf = (int)(_fa < _fb);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(eax)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(eax) = MEM32(eax) + eax;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    { uint32_t _rc = ecx; ecx = 1;
    { int32_t _st = RECOMP_DF_STEP(1);
    while (ecx != 0) {
        _flags = (LO8(eax) == MEM8(edi));
        _cf = (LO8(eax) < MEM8(edi));
        edi += _st; ecx--;
        if (!_flags) break;
    } } /* repe scasb */
    ecx = _rc; }
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(eax)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(eax) = MEM32(eax) + eax;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _cf = 0; /* logical op clears CF */
    SET_LO8(eax, LO8(eax) & 0xAD);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* and result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(eax)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(eax) = MEM32(eax) + eax;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    if (!_cf) (void)0; /* goto loc_0001AF4F - dead code, label not in function */ /* jae: above or equal (unsigned >=) */

loc_0001AFA2: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(eax)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(eax) = MEM32(eax) + eax;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_HI8(ecx, 0xAD);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(eax)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(eax) = MEM32(eax) + eax;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    { int64_t _r = (int64_t)(int32_t)eax * (int64_t)(int32_t)MEM32(ebp + -1388314623);
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); _fa = eax; _fas = (int32_t)eax; _fb = (_r != (int64_t)(int32_t)_r); }
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(eax)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(eax) = MEM32(eax) + eax;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    if (_flags /* jp: parity */) (void)0; /* goto loc_0001AF5F - dead code, label not in function */

loc_0001AFB2: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(eax)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(eax) = MEM32(eax) + eax;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

/**
 * sub_002EBB00
 * Original: 0x002EBB00 - 0x002EBB3B (59 bytes, 14 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_002EBB00(void)
{
    RECOMP_ENTRY(0x002EBB00u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_002EBB00: ;
    _fa = (uint32_t)(MEM8(0x589752)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0x589752), 1 (8-bit) */
    if (CMP_BE(_fa, _fb) && !ngb_pc_setting(31)) goto loc_002EBB3A; /* (Intercept on: every difficulty) */

loc_002EBB09: ;
    _fa = (uint32_t)(MEM8(0x5893E8)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0x5893E8), 2 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002EBB3A; /* jne: not equal / not zero */

loc_002EBB12: ;
    eax = MEM32(0x5893D0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x5893D4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(0x5893D4) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_002EBB3A; /* jae: above or equal (unsigned >=) */

loc_002EBB1F: ;
    eax = MEM32(0xDB9330);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xC350) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xC350 (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_002EBB3A; /* jae: above or equal (unsigned >=) */

loc_002EBB2B: ;
    _fb = (uint32_t)(0x3E8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x3E8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(0xDB9330) = eax;
    g_seh_ebp = ebp; sub_00206B80(); return; /* tail jmp 0x00206B80 */

loc_002EBB3A: ;
    esp += 4; return; /* ret */

}

/**
 * sub_00208120
 * Original: 0x00208120 - 0x0020968E (5486 bytes, 1048 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_00208120(void)
{
    RECOMP_ENTRY(0x00208120u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int _cf = 0; /* carry flag */

loc_00208120: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    _cf = 0; /* logical op clears CF */
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0xF4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(esp) < (uint32_t)(0xF4));
    esp = esp - 0xF4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(0x41CCD8);
    MEM32(esp + 0xF0) = eax;
    eax = MEM32(0xD6AF20);
    { int64_t _r = (int64_t)(int32_t)(eax) * (int64_t)(int32_t)(0x1AC);
      eax = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    PUSH32(esp, ebx);
    _fb = (uint32_t)(0xD6AF40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0xD6AF40)) >> 32) & 1);
    eax = eax + 0xD6AF40;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    MEM32(esp + 0x1C) = eax;
    SET_LO16(eax, MEM16(0x8DB6DC));
    _cf = 0; /* xor clears CF */
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(edi)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(edi) (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_0020967B; /* je: equal / zero */

loc_00208160: ;
    xmm2 = XMM_ZERO(); /* xorps self = zero */
    eax = ZX16(LO16(eax));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 4 (32-bit) */
    _cf = (int)(_fa < _fb);
    MEM16(esp + 0xF6) = LO16(edi);
    MEMF(esp + 0xE0) = xmm2.f[0]; /* movss */
    MEMF(esp + 0xDC) = xmm2.f[0]; /* movss */
    MEMF(esp + 0xD8) = xmm2.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_0020869B; /* je: equal / zero */

loc_00208192: ;
    if (CMP_LE(_fas, _fbs)) goto loc_002085FE; /* jle: less or equal (signed <=) */

loc_00208198: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(6) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 6 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_002085FE; /* jg: greater (signed >) */

loc_002081A1: ;
    eax = ZX16(MEM16(0x8DB6E0));
    xmm5 = XMM_SCALAR(MEMF(0x41492C)); /* movss */
    edx = MEM32(0x8DB6F8);
    SET_LO16(esi, MEM16(0x58A38C));
    ecx = 0xB4;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(eax));
    ecx = ecx - eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA (32-bit) */
    _cf = (int)(_fa < _fb);
    xmm3.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0 = xmm3; /* movaps */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414C9C); /* mulss */
    MEMF(0x8DB704) = xmm0.f[0]; /* movss */
    if (CMP_G(_fas, _fbs)) goto loc_00208247; /* jg: greater (signed >) */

loc_002081E0: ;
    xmm4 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    SET_LO16(eax, MEM16(0x8DB6E6));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(edi)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(edi) (16-bit) */
    _cf = (int)(_fa < _fb);
    xmm0 = xmm3; /* movaps */
    xmm0.f[0] = xmm0.f[0] * xmm5.f[0]; /* mulss */
    xmm1 = xmm4; /* movaps */
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */
    MEMF(0x8DB714) = xmm1.f[0]; /* movss */
    MEMF(0x8DB710) = xmm1.f[0]; /* movss */
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_002082AD; /* jne: not equal / not zero */

loc_00208221: ;
    _fa = (uint32_t)(LO16(esi)) & 0xFFFFu; _fb = (uint32_t)(0x200) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(esi), 0x200 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_002082AD; /* jb: below (unsigned <) */

loc_0020822C: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, edi (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_00208233; /* ja: above (unsigned >) */

loc_00208230: ;
    xmm1 = xmm2; /* movaps */

loc_00208233: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, edi (32-bit) */
    _cf = (int)(_fa < _fb);
    MEMF(0x8DB718) = xmm1.f[0]; /* movss */
    if (CMP_A(_fa, _fb)) goto loc_002082B5; /* ja: above (unsigned >) */

loc_0020823F: ;
    MEMF(esp + 0x5C) = xmm2.f[0]; /* movss */
    goto loc_002082BB;

loc_00208247: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5F) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5F (32-bit) */
    _cf = (int)(_fa < _fb);
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x54) = xmm2.f[0]; /* movss */
    if (CMP_GE(_fas, _fbs)) goto loc_00208260; /* jge: greater or equal (signed >=) */

loc_0020825A: ;
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */

loc_00208260: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x64 (32-bit) */
    _cf = (int)(_fa < _fb);
    MEMF(esp + 0x58) = xmm2.f[0]; /* movss */
    if (CMP_GE(_fas, _fbs)) goto loc_00208271; /* jge: greater or equal (signed >=) */

loc_0020826B: ;
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */

loc_00208271: ;
    SET_LO16(eax, MEM16(0x8DB6E6));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(3) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 3 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_00208298; /* je: equal / zero */

loc_0020827D: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, edi (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_00208298; /* ja: above (unsigned >) */

loc_00208281: ;
    _fa = (uint32_t)(LO16(esi)) & 0xFFFFu; _fb = (uint32_t)(0x200) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(esi), 0x200 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_00208298; /* jb: below (unsigned <) */

loc_00208288: ;
    xmm4 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x5C) = xmm2.f[0]; /* movss */
    goto loc_002082BB;

loc_00208298: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x69) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x69 (32-bit) */
    _cf = (int)(_fa < _fb);
    xmm4 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    if (CMP_L(_fas, _fbs)) goto loc_002082B5; /* jl: less (signed <) */

loc_002082A5: ;
    MEMF(esp + 0x5C) = xmm2.f[0]; /* movss */
    goto loc_002082BB;

loc_002082AD: ;
    MEMF(0x8DB718) = xmm1.f[0]; /* movss */

loc_002082B5: ;
    MEMF(esp + 0x5C) = xmm0.f[0]; /* movss */

loc_002082BB: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1E) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x1E (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_002082CD; /* jg: greater (signed >) */

loc_002082C0: ;
    xmm3.f[0] = xmm3.f[0] * MEMF(0x414C24); /* mulss */
    xmm0 = xmm3; /* movaps */
    goto loc_002082E8;

loc_002082CD: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3C) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x3C (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_0020830C; /* jg: greater (signed >) */

loc_002082D2: ;
    ebx = ecx + -30;
    xmm1.f[0] = (float)(int32_t)ebx; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x414C24); /* mulss */
    xmm0 = xmm4; /* movaps */
    xmm0.f[0] = xmm0.f[0] - xmm1.f[0]; /* subss */

loc_002082E8: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(edi)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(edi) (16-bit) */
    _cf = (int)(_fa < _fb);
    MEMF(esp + 0x6C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x68) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_00208304; /* jne: not equal / not zero */

loc_002082F9: ;
    _fa = (uint32_t)(LO16(esi)) & 0xFFFFu; _fb = (uint32_t)(0x200) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(esi), 0x200 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_00208304; /* jb: below (unsigned <) */

loc_00208300: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, edi (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_BE(_fa, _fb)) goto loc_00208318; /* jbe: below or equal (unsigned <=) */

loc_00208304: ;
    MEMF(esp + 0x70) = xmm0.f[0]; /* movss */
    goto loc_0020831E;

loc_0020830C: ;
    MEMF(esp + 0x6C) = xmm2.f[0]; /* movss */
    MEMF(esp + 0x68) = xmm2.f[0]; /* movss */

loc_00208318: ;
    MEMF(esp + 0x70) = xmm2.f[0]; /* movss */

loc_0020831E: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5A) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5A (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_00208371; /* jl: less (signed <) */

loc_00208323: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5F) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5F (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_00208371; /* jg: greater (signed >) */

loc_00208328: ;
    edx = ecx + -90;
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4148EC); /* mulss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x41566C); /* mulss */
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x18), 0x0E7Fu, 32); /* cvttss2si */
    SET_LO16(esi, MEM16(0x58A38C));
    edx = MEM32(0x8DB6F8);
    _cf = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    MEM16(esp + 0xF2) = LO16(eax);
    SET_LO16(eax, MEM16(0x8DB6E6));
    goto loc_00208379;

loc_00208371: ;
    MEM16(esp + 0xF2) = LO16(edi);

loc_00208379: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5A) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5A (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_002083C8; /* jl: less (signed <) */

loc_0020837E: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x64 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_002083C8; /* jg: greater (signed >) */

loc_00208383: ;
    eax = ecx + -90;
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm5.f[0]; /* mulss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4158E4); /* mulss */
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x18), 0x0E7Fu, 32); /* cvttss2si */
    SET_LO16(esi, MEM16(0x58A38C));
    edx = MEM32(0x8DB6F8);
    _cf = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    MEM16(esp + 0xF4) = LO16(eax);
    SET_LO16(eax, MEM16(0x8DB6E6));
    goto loc_002083D0;

loc_002083C8: ;
    MEM16(esp + 0xF4) = LO16(edi);

loc_002083D0: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(edi)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(edi) (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_002083E0; /* jne: not equal / not zero */

loc_002083D5: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, edi (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_002083E0; /* ja: above (unsigned >) */

loc_002083D9: ;
    _fa = (uint32_t)(LO16(esi)) & 0xFFFFu; _fb = (uint32_t)(0x200) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(esi), 0x200 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_0020843B; /* jae: above or equal (unsigned >=) */

loc_002083E0: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5A) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5A (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_00208433; /* jl: less (signed <) */

loc_002083E5: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x69) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x69 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_00208433; /* jg: greater (signed >) */

loc_002083EA: ;
    edx = ecx + -90;
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414D14); /* mulss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x41551C); /* mulss */
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x18), 0x0E7Fu, 32); /* cvttss2si */
    SET_LO16(esi, MEM16(0x58A38C));
    edx = MEM32(0x8DB6F8);
    _cf = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    MEM16(esp + 0xF6) = LO16(eax);
    SET_LO16(eax, MEM16(0x8DB6E6));
    goto loc_0020843B;

loc_00208433: ;
    MEM16(esp + 0xF6) = LO16(edi);

loc_0020843B: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5A) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5A (32-bit) */
    _cf = (int)(_fa < _fb);
    xmm1 = XMM_SCALAR(MEMF(0x4149C4)); /* movss */
    if (CMP_L(_fas, _fbs)) goto loc_00208486; /* jl: less (signed <) */

loc_00208448: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x6E) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x6E (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_00208486; /* jg: greater (signed >) */

loc_0020844D: ;
    eax = ecx + -90;
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm3 = xmm0; /* movaps */
    xmm3.f[0] = xmm3.f[0] * xmm0.f[0]; /* mulss */
    xmm3.f[0] = xmm3.f[0] * MEMF(0x41546C); /* mulss */
    MEMF(esp + 0x18) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x18), 0x0E7Fu, 32); /* cvttss2si */
    SET_LO16(esi, MEM16(0x58A38C));
    edx = MEM32(0x8DB6F8);
    SET_LO16(eax, MEM16(0x8DB6E6));

loc_00208486: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5F) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5F (32-bit) */
    _cf = (int)(_fa < _fb);
    xmm0 = XMM_SCALAR(MEMF(0x414ABC)); /* movss */
    if (CMP_L(_fas, _fbs)) goto loc_002084C0; /* jl: less (signed <) */

loc_00208493: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA0 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_002084C0; /* jg: greater (signed >) */

loc_0020849B: ;
    ebx = ecx + -95;
    xmm3.f[0] = (float)(int32_t)ebx; /* cvtsi2ss */
    xmm3.f[0] = xmm3.f[0] * MEMF(0x4158E0); /* mulss */
    xmm5 = xmm4; /* movaps */
    xmm5.f[0] = xmm5.f[0] - xmm3.f[0]; /* subss */
    xmm5.f[0] = xmm5.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0xD8) = xmm5.f[0]; /* movss */
    goto loc_002084C9;

loc_002084C0: ;
    MEMF(esp + 0xD8) = xmm2.f[0]; /* movss */

loc_002084C9: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x64 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_002084FB; /* jl: less (signed <) */

loc_002084CE: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA0 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_002084FB; /* jg: greater (signed >) */

loc_002084D6: ;
    ebx = ecx + -100;
    xmm3.f[0] = (float)(int32_t)ebx; /* cvtsi2ss */
    xmm3.f[0] = xmm3.f[0] * MEMF(0x414988); /* mulss */
    xmm5 = xmm4; /* movaps */
    xmm5.f[0] = xmm5.f[0] - xmm3.f[0]; /* subss */
    xmm5.f[0] = xmm5.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0xE4) = xmm5.f[0]; /* movss */
    goto loc_00208504;

loc_002084FB: ;
    MEMF(esp + 0xE4) = xmm2.f[0]; /* movss */

loc_00208504: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(edi)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(edi) (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_00208514; /* jne: not equal / not zero */

loc_00208509: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, edi (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_00208514; /* ja: above (unsigned >) */

loc_0020850D: ;
    _fa = (uint32_t)(LO16(esi)) & 0xFFFFu; _fb = (uint32_t)(0x200) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(esi), 0x200 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_0020854F; /* jae: above or equal (unsigned >=) */

loc_00208514: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x69) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x69 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_00208546; /* jl: less (signed <) */

loc_00208519: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA0 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_00208546; /* jg: greater (signed >) */

loc_00208521: ;
    ebx = ecx + -105;
    xmm3.f[0] = (float)(int32_t)ebx; /* cvtsi2ss */
    xmm3.f[0] = xmm3.f[0] * MEMF(0x4158DC); /* mulss */
    xmm5 = xmm4; /* movaps */
    xmm5.f[0] = xmm5.f[0] - xmm3.f[0]; /* subss */
    xmm5.f[0] = xmm5.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0xDC) = xmm5.f[0]; /* movss */
    goto loc_0020854F;

loc_00208546: ;
    MEMF(esp + 0xDC) = xmm2.f[0]; /* movss */

loc_0020854F: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA0 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_00208569; /* jge: greater or equal (signed >=) */

loc_00208557: ;
    MEMF(0x8DB724) = xmm4.f[0]; /* movss */
    MEMF(0x8DB720) = xmm4.f[0]; /* movss */
    goto loc_002085A8;

loc_00208569: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xB4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xB4 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_00208598; /* jg: greater (signed >) */

loc_00208571: ;
    ebx = ecx + -160;
    xmm3.f[0] = (float)(int32_t)ebx; /* cvtsi2ss */
    xmm3.f[0] = xmm3.f[0] * xmm1.f[0]; /* mulss */
    xmm0 = xmm4; /* movaps */
    xmm0.f[0] = xmm0.f[0] - xmm3.f[0]; /* subss */
    MEMF(0x8DB724) = xmm0.f[0]; /* movss */
    MEMF(0x8DB720) = xmm0.f[0]; /* movss */
    goto loc_002085A8;

loc_00208598: ;
    MEMF(0x8DB724) = xmm2.f[0]; /* movss */
    MEMF(0x8DB720) = xmm2.f[0]; /* movss */

loc_002085A8: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(edi)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(edi) (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_002085C5; /* jne: not equal / not zero */

loc_002085AD: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, edi (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_002085C5; /* ja: above (unsigned >) */

loc_002085B1: ;
    _fa = (uint32_t)(LO16(esi)) & 0xFFFFu; _fb = (uint32_t)(0x200) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(esi), 0x200 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_002085C5; /* jb: below (unsigned <) */

loc_002085B8: ;
    MEMF(0x8DB728) = xmm2.f[0]; /* movss */
    goto loc_0020865E;

loc_002085C5: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA0 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_002085DA; /* jge: greater or equal (signed >=) */

loc_002085CD: ;
    MEMF(0x8DB728) = xmm4.f[0]; /* movss */
    goto loc_0020865E;

loc_002085DA: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xB4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xB4 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_002085B8; /* jg: greater (signed >) */

loc_002085E2: ;
    _fb = (uint32_t)(0xFFFFFF60u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(0xFFFFFF60u)) >> 32) & 1);
    ecx = ecx + 0xFFFFFF60u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm4.f[0] = xmm4.f[0] - xmm0.f[0]; /* subss */
    MEMF(0x8DB728) = xmm4.f[0]; /* movss */
    goto loc_0020865E;

loc_002085FE: ;
    MEM16(esp + 0xF2) = LO16(edi);
    MEMF(esp + 0x54) = xmm2.f[0]; /* movss */
    MEMF(esp + 0x68) = xmm2.f[0]; /* movss */
    MEMF(esp + 0xD8) = xmm2.f[0]; /* movss */

loc_0020861B: ;
    MEMF(esp + 0xDC) = xmm2.f[0]; /* movss */

loc_00208624: ;
    MEM16(esp + 0xF4) = LO16(edi);
    MEM16(esp + 0xF6) = LO16(edi);
    MEMF(esp + 0x58) = xmm2.f[0]; /* movss */
    MEMF(esp + 0x5C) = xmm2.f[0]; /* movss */
    MEMF(esp + 0x6C) = xmm2.f[0]; /* movss */
    MEMF(esp + 0x70) = xmm2.f[0]; /* movss */
    MEMF(esp + 0xE0) = xmm2.f[0]; /* movss */
    MEMF(esp + 0xE4) = xmm2.f[0]; /* movss */

loc_0020865E: ;
    eax = MEM32(0x8DB72C);
    ebx = 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_002087F7; /* jne: not equal / not zero */

loc_00208670: ;
    ecx = 0x8DB734;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020867Au); RECOMP_ABI_CALL(0x002080A0u, sub_002080A0); /* call 0x002080A0 */

loc_0020867A: ;
    ecx = 0x8DB748;
    MEMF(esp + 0xE8) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020868Du); RECOMP_ABI_CALL(0x002080A0u, sub_002080A0); /* call 0x002080A0 */

loc_0020868D: ;
    MEMF(esp + 0xEC) = xmm0.f[0]; /* movss */
    goto loc_00208809;

loc_0020869B: ;
    edx = ZX16(MEM16(0x8DB6E0));
    xmm3 = XMM_SCALAR(MEMF(0x41492C)); /* movss */
    ecx = 0x9B;
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(edx));
    ecx = ecx - edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA (32-bit) */
    _cf = (int)(_fa < _fb);
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4150AC); /* mulss */
    MEMF(0x8DB704) = xmm1.f[0]; /* movss */
    if (CMP_G(_fas, _fbs)) goto loc_002086E4; /* jg: greater (signed >) */

loc_002086CD: ;
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * xmm3.f[0]; /* mulss */
    MEMF(esp + 0x54) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    goto loc_002086FD;

loc_002086E4: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x64 (32-bit) */
    _cf = (int)(_fa < _fb);
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x54) = xmm2.f[0]; /* movss */
    if (CMP_GE(_fas, _fbs)) goto loc_002086FD; /* jge: greater or equal (signed >=) */

loc_002086F7: ;
    MEMF(esp + 0x54) = xmm1.f[0]; /* movss */

loc_002086FD: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_0020871A; /* jg: greater (signed >) */

loc_00208702: ;
    xmm4 = xmm0; /* movaps */
    xmm4.f[0] = xmm4.f[0] * xmm3.f[0]; /* mulss */
    xmm5 = xmm1; /* movaps */
    xmm5.f[0] = xmm5.f[0] - xmm4.f[0]; /* subss */
    MEMF(0x8DB710) = xmm5.f[0]; /* movss */
    goto loc_00208737;

loc_0020871A: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3C) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x3C (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_00208737; /* jl: less (signed <) */

loc_0020871F: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x46) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x46 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_00208737; /* jg: greater (signed >) */

loc_00208724: ;
    eax = ecx + -60;
    xmm4.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm4.f[0] = xmm4.f[0] * xmm3.f[0]; /* mulss */
    MEMF(0x8DB710) = xmm4.f[0]; /* movss */

loc_00208737: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1E) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x1E (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_0020874C; /* jg: greater (signed >) */

loc_0020873C: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414C24); /* mulss */
    MEMF(esp + 0x68) = xmm0.f[0]; /* movss */
    goto loc_00208775;

loc_0020874C: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3C) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x3C (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_0020876F; /* jg: greater (signed >) */

loc_00208751: ;
    edx = ecx + -30;
    xmm4.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm4.f[0] = xmm4.f[0] * MEMF(0x414C24); /* mulss */
    xmm0 = xmm1; /* movaps */
    xmm0.f[0] = xmm0.f[0] - xmm4.f[0]; /* subss */
    MEMF(esp + 0x68) = xmm0.f[0]; /* movss */
    goto loc_00208775;

loc_0020876F: ;
    MEMF(esp + 0x68) = xmm2.f[0]; /* movss */

loc_00208775: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5A) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5A (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_002087B1; /* jl: less (signed <) */

loc_0020877A: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x64 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_002087B1; /* jg: greater (signed >) */

loc_0020877F: ;
    eax = ecx + -90;
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm3.f[0]; /* mulss */
    xmm3 = xmm0; /* movaps */
    xmm3.f[0] = xmm3.f[0] * xmm0.f[0]; /* mulss */
    xmm3.f[0] = xmm3.f[0] * MEMF(0x41566C); /* mulss */
    MEMF(esp + 0x18) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x18), 0x0E7Fu, 32); /* cvttss2si */
    _cf = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    MEM16(esp + 0xF2) = LO16(eax);
    goto loc_002087B9;

loc_002087B1: ;
    MEM16(esp + 0xF2) = LO16(edi);

loc_002087B9: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x64 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_0020861B; /* jl: less (signed <) */

loc_002087C2: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x9B) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x9B (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_0020861B; /* jg: greater (signed >) */

loc_002087CE: ;
    _fb = (uint32_t)(0xFFFFFF9Cu) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(0xFFFFFF9Cu)) >> 32) & 1);
    ecx = ecx + 0xFFFFFF9Cu;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4158DC); /* mulss */
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4149DC); /* mulss */
    MEMF(esp + 0xDC) = xmm1.f[0]; /* movss */
    goto loc_00208624;

loc_002087F7: ;
    MEMF(esp + 0xE8) = xmm2.f[0]; /* movss */
    MEMF(esp + 0xEC) = xmm2.f[0]; /* movss */

loc_00208809: ;
    ecx = MEM32(0xC0EB78);
    esi = MEM32(0x8DB6D8);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    _fb = (uint32_t)(0x66) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(0x66)) >> 32) & 1);
    ecx = ecx + 0x66;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = SX16(LO16(ecx));
    PUSH32(esp, 2);
    _cf = 0; /* xor clears CF */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0xBCE030));
    _fb = (uint32_t)(0x5A) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x5A)) >> 32) & 1);
    esi = esi + 0x5A;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x1CC);
    eax = SX16(LO16(esi));
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    eax = 0x62;
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208853u); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_00208853: ;
    ecx = eax;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x90;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208864u); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_00208864: ;
    xmm0 = XMM_SCALAR(MEMF(0x45A9FC)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x3E9F0C); /* mulss */
    edx = esp + 0x90;
    MEMF(esp + 0xB0) = xmm0.f[0]; /* movss */
    MEMF(esp + 0xAC) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edx);
    MEMF(esp + 0xA8) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x002088A2u); RECOMP_ICALL_SAFE(0x001E6D30u, _icall_esp); /* manual call 0x001E6D30 */
    }

loc_002088A2: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    _fb = (uint32_t)(0xFFFFFFF2u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0xFFFFFFF2u)) >> 32) & 1);
    esi = esi + 0xFFFFFFF2u;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = esp + 0x34;
    MEM32(esp + 0x34) = 0x13C;
    MEM16(esp + 0x38) = LO16(esi);
    MEM16(esp + 0x3A) = 0x4F;
    MEM16(esp + 0x3C) = 0x1EB;
    MEM8(esp + 0x3E) = 0;
    MEM8(esp + 0x3F) = LO8(ebx);
    MEM8(esp + 0x40) = LO8(ebx);
    MEM32(esp + 0x44) = 0;
    MEM32(esp + 0x5C) = 0xFFFFFFFFu;
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020891Fu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_0020891F: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    esi = MEM32(0x8DB6D8);
    eax = MEM32(0xC0EB78);
    ecx = MEM32(0x589560);
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x45A9FC)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4158D8); /* mulss */
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x8DB70C)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0xD6AF04); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    PUSH32(esp, 0);
    _fb = (uint32_t)(0x93) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x93)) >> 32) & 1);
    esi = esi + 0x93;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x63) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0x63)) >> 32) & 1);
    eax = eax + 0x63;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x808181FFu);
    MEM32(esp + 0x3C) = 0x93;
    MEM16(esp + 0x40) = LO16(esi);
    MEM16(esp + 0x42) = LO16(eax);
    MEM16(esp + 0x44) = 0x1EB;
    MEM8(esp + 0x46) = 0;
    MEM8(esp + 0x47) = LO8(ebx);
    MEM8(esp + 0x48) = LO8(ebx);
    MEM32(esp + 0x4C) = 0;
    MEM32(esp + 0x64) = 0xFFFFFFFFu;
    MEMF(esp + 0x5C) = xmm0.f[0]; /* movss */
    PUSH32(esp, ecx);
    SET_LO8(edx, 9);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x002089CFu); RECOMP_ABI_CALL(0x001E9B80u, sub_001E9B80); /* call 0x001E9B80 */

loc_002089CF: ;
    xmm1 = XMM_SCALAR(MEMF(0x4158D4)); /* movss */
    SET_LO8(ecx, LO8(eax));
    _fb = (uint32_t)(0x20) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x20)) >> 32) & 1);
    esp = esp + 0x20;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM8(esp + 0x4F) = LO8(ecx);
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x18), 0x0E7Fu, 32); /* cvttss2si */
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    edx = 5;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edx) < (uint32_t)(eax));
    edx = edx - eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(edx)) >> 32) & 1);
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 2 (8-bit) */
    _cf = (int)(_fa < _fb);
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEM32(esp + 0x20) = 0x13D;
    MEM16(esp + 0x24) = LO16(esi);
    MEM16(esp + 0x26) = 0x4F;
    MEM16(esp + 0x28) = 0x1EB;
    MEM8(esp + 0x2A) = 0;
    MEM8(esp + 0x2B) = LO8(ebx);
    MEM8(esp + 0x2C) = LO8(ebx);
    MEM32(esp + 0x30) = 0;
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x48) = 0xFFFFFFFFu;
    if (CMP_AE(_fa, _fb)) goto loc_00208A54; /* jae: above or equal (unsigned >=) */

loc_00208A4F: ;
    xmm0 = xmm1; /* movaps */
    goto loc_00208A70;

loc_00208A54: ;
    eax = ZX8(LO8(ecx));
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x45A9FC); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4158D0); /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */

loc_00208A70: ;
    xmm0.f[0] = xmm0.f[0] + MEMF(0xC0EB74); /* addss */
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4152E4); /* mulss */
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    edi = esp + 0x20;
    MEMF(esp + 0x40) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208AABu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00208AAB: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x18)); /* movss */
    MEMF(esp + 0x78) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x78), 0x0E7Fu, 32); /* cvttss2si */
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    _fb = (uint32_t)(esi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(esi)) >> 32) & 1);
    eax = eax + esi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    _cf = 0; /* xor clears CF */
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esp + 0x20) = 0x13E;
    MEM16(esp + 0x24) = LO16(eax);
    MEM16(esp + 0x26) = 0x4F;
    MEM16(esp + 0x28) = 0x1EB;
    MEM8(esp + 0x2A) = 0;
    MEM8(esp + 0x2B) = LO8(ebx);
    MEM8(esp + 0x2C) = LO8(ebx);
    MEM32(esp + 0x30) = esi;
    MEM32(esp + 0x48) = 0xFFFFFFFFu;
    MEMF(esp + 0x40) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208B33u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00208B33: ;
    edx = ZX8(MEM8(esp + 0x4F));
    ecx = MEM32(0x8DB6D8);
    xmm2 = XMM_ZERO(); /* xorps self = zero */
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414DA0); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x3EC240); /* addss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4148C0); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415240); /* mulss */
    _fb = (uint32_t)(0x7F) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(0x7F)) >> 32) & 1);
    ecx = ecx + 0x7F;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp + 0x78) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEM32(esp + 0x7C) = ecx;
    MEM32(esp + 0x80) = 0x27;
    MEM16(esp + 0x86) = 0x6C;
    MEM16(esp + 0x88) = 0x1EE;
    MEM8(esp + 0x8B) = LO8(ebx);
    MEM8(esp + 0x8C) = LO8(ebx);
    MEMF(esp + 0x9C) = xmm1.f[0]; /* movss */
    MEMF(esp + 0xA4) = xmm2.f[0]; /* movss */
    MEM32(esp + 0xA8) = 0xFFFFFFFFu;
    MEM8(esp + 0x8A) = 0x11;
    MEM32(esp + 0x90) = 5;
    MEMF(esp + 0x94) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x98) = xmm1.f[0]; /* movss */
    /* nop */

loc_00208BF0: ;
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x78), 0x0E7Fu, 32); /* cvttss2si */
    ecx = MEM32(esp + 0x7C);
    xmm0 = XMM_SCALAR(MEMF(esp + esi * 4 + 0xD8)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0xD6AF04); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(ecx)) >> 32) & 1);
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = esp + 0x80;
    MEM16(esp + 0x84) = LO16(eax);
    MEMF(esp + 0xA0) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208C32u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00208C32: ;
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(5) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 5 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_00208BF0; /* jl: less (signed <) */

loc_00208C38: ;
    esi = MEM32(0x8DB6D8);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB720); /* mulss */
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, 2);
    _fb = (uint32_t)(0x5A) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x5A)) >> 32) & 1);
    esi = esi + 0x5A;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x1CC);
    edx = SX16(LO16(esi));
    PUSH32(esp, 0x7D);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    eax = 0x63;
    MEMF(esp + 0x90) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208C81u); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_00208C81: ;
    ecx = eax;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x90;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208C92u); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_00208C92: ;
    xmm0 = XMM_SCALAR(MEMF(0x3E9F0C)); /* movss */
    ecx = esp + 0x90;
    MEMF(esp + 0xB0) = xmm0.f[0]; /* movss */
    MEMF(esp + 0xAC) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x8C)); /* movss */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ecx);
    MEMF(esp + 0xA8) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208CCBu); RECOMP_ICALL_SAFE(0x001E6D30u, _icall_esp); /* manual call 0x001E6D30 */
    }

loc_00208CCB: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB720); /* mulss */
    _fb = (uint32_t)(0xFFFFFFF2u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0xFFFFFFF2u)) >> 32) & 1);
    esi = esi + 0xFFFFFFF2u;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = esp + 0x34;
    MEM32(esp + 0x34) = 0x139;
    MEM16(esp + 0x38) = LO16(esi);
    MEM16(esp + 0x3A) = 0x66;
    MEM16(esp + 0x3C) = 0x1EB;
    MEM8(esp + 0x3E) = 0;
    MEM8(esp + 0x3F) = LO8(ebx);
    MEM8(esp + 0x40) = LO8(ebx);
    MEM32(esp + 0x44) = 0;
    MEM32(esp + 0x5C) = 0xFFFFFFFFu;
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208D50u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00208D50: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    eax = MEM32(esp + 0x30);
    xmm1 = XMM_SCALAR(MEMF(eax + 0x14)); /* movss */
    esi = MEM32(0x8DB6D8);
    edi = MEM32(0x5893DC);
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x4158D8)); /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0x48) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(eax + 0x18)); /* movss */
    SET_LO8(eax, MEM8(0x58A386));
    _fb = (uint32_t)(0x93) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x93)) >> 32) & 1);
    esi = esi + 0x93;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _cf = (int)((LO8(eax)) != 0);
    SET_LO8(eax, (uint32_t)(-(int32_t)LO8(eax)));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* neg result */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB710); /* mulss */
    MEM32(esp + 0x34) = 0x93;
    MEM16(esp + 0x38) = LO16(esi);
    MEM16(esp + 0x3A) = 0x7A;
    MEM16(esp + 0x3C) = 0x1EB;
    MEM8(esp + 0x3E) = 0;
    MEM8(esp + 0x3F) = LO8(ebx);
    MEM8(esp + 0x40) = LO8(ebx);
    MEM32(esp + 0x44) = 0;
    MEM32(esp + 0x5C) = 0xFFFFFFFFu;
    MEMF(esp + 0x4C) = xmm1.f[0]; /* movss */
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */
    eax = _cf ? 0xFFFFFFFF : 0; _fb = 0; /* sbb self (CF extend), OF clear */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _cf = 0; /* logical op clears CF */
    eax = eax & edi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = ZX16(LO16(eax));
    PUSH32(esp, 0);
    PUSH32(esp, 0x80FF0000u);
    PUSH32(esp, edx);
    SET_LO8(edx, 3);
    edi = esp + 0x40;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208E1Au); RECOMP_ABI_CALL(0x001E9B80u, sub_001E9B80); /* call 0x001E9B80 */

loc_00208E1A: ;
    xmm0 = XMM_SCALAR(MEMF(0x4158D4)); /* movss */
    _fb = (uint32_t)(0x20) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x20)) >> 32) & 1);
    esp = esp + 0x20;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(ecx, LO8(eax));
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x1C), 0x0E7Fu, 32); /* cvttss2si */
    edx = 5;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edx) < (uint32_t)(eax));
    edx = edx - eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(edx)) >> 32) & 1);
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 2 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_00208E49; /* jae: above or equal (unsigned >=) */

loc_00208E41: ;
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    goto loc_00208E63;

loc_00208E49: ;
    eax = ZX8(LO8(ecx));
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4158D0); /* mulss */
    xmm1.f[0] = xmm1.f[0] + xmm0.f[0]; /* addss */
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */

loc_00208E63: ;
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB720); /* mulss */
    PUSH32(esp, ecx);
    ecx = MEM32(esp + 0x1C);
    MEMF(esp) = xmm0.f[0]; /* movss */
    PUSH32(esp, ecx);
    PUSH32(esp, esi);
    esi = 0x66;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208E91u); RECOMP_ABI_CALL(0x002097B0u, sub_002097B0); /* call 0x002097B0 */

loc_00208E91: ;
    edx = MEM32(0x8DB6F0);
    esi = MEM32(0x8DB6D8);
    edi = MEM32(esp + 0xFE);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x60); /* mulss */
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, edx);
    PUSH32(esp, ecx);
    _fb = (uint32_t)(0x90) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x90)) >> 32) & 1);
    esi = esi + 0x90;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x7A) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(0x7A)) >> 32) & 1);
    edi = edi + 0x7A;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = edi;
    eax = esi;
    MEMF(esp) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208ED6u); RECOMP_ABI_CALL(0x002098C0u, sub_002098C0); /* call 0x002098C0 */

loc_00208ED6: ;
    eax = ZX8(LO8(eax));
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415468); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x3EC240); /* addss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4148C0); /* mulss */
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp + 0x1C) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x1C), 0x0E7Fu, 32); /* cvttss2si */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415240); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4149DC); /* mulss */
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x94) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x415448)); /* movss */
    MEMF(esp + 0x98) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x68); /* mulss */
    _fb = (uint32_t)(0xA) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(0xA)) >> 32) & 1);
    edi = edi + 0xA;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = eax + esi + -20;
    MEM16(esp + 0x86) = LO16(edi);
    MEMF(esp + 0x9C) = xmm1.f[0]; /* movss */
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    edi = esp + 0x80;
    MEM32(esp + 0x80) = 0x27;
    MEM16(esp + 0x84) = LO16(ecx);
    MEM16(esp + 0x88) = 0x1EE;
    MEM8(esp + 0x8B) = LO8(ebx);
    MEM8(esp + 0x8C) = LO8(ebx);
    MEMF(esp + 0xA4) = xmm1.f[0]; /* movss */
    MEM32(esp + 0xA8) = 0xFFFFFFFFu;
    MEM8(esp + 0x8A) = 0x11;
    MEM32(esp + 0x90) = 5;
    MEMF(esp + 0xA0) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00208FD1u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00208FD1: ;
    esi = MEM32(0x8DB6D8);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB724); /* mulss */
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, 2);
    _fb = (uint32_t)(0x5A) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x5A)) >> 32) & 1);
    esi = esi + 0x5A;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x1CC);
    edx = SX16(LO16(esi));
    PUSH32(esp, 0x94);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    eax = 0x64;
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020901Au); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_0020901A: ;
    ecx = eax;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x90;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020902Bu); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_0020902B: ;
    xmm0 = XMM_SCALAR(MEMF(0x3E9F0C)); /* movss */
    ecx = esp + 0x90;
    MEMF(esp + 0xB0) = xmm0.f[0]; /* movss */
    MEMF(esp + 0xAC) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x2C)); /* movss */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ecx);
    MEMF(esp + 0xA8) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00209061u); RECOMP_ICALL_SAFE(0x001E6D30u, _icall_esp); /* manual call 0x001E6D30 */
    }

loc_00209061: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB724); /* mulss */
    _fb = (uint32_t)(0xFFFFFFF2u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0xFFFFFFF2u)) >> 32) & 1);
    esi = esi + 0xFFFFFFF2u;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = esp + 0x34;
    MEM32(esp + 0x34) = 0x139;
    MEM16(esp + 0x38) = LO16(esi);
    MEM16(esp + 0x3A) = 0x7D;
    MEM16(esp + 0x3C) = 0x1EB;
    MEM8(esp + 0x3E) = 0;
    MEM8(esp + 0x3F) = LO8(ebx);
    MEM8(esp + 0x40) = LO8(ebx);
    MEM32(esp + 0x44) = 0;
    MEM32(esp + 0x5C) = 0xFFFFFFFFu;
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x002090E6u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_002090E6: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    edx = MEM32(0x5893D8);
    esi = MEM32(0x8DB6D8);
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x4158D8)); /* movss */
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB714); /* mulss */
    PUSH32(esp, 0);
    PUSH32(esp, 0x80FF0000u);
    PUSH32(esp, edx);
    _fb = (uint32_t)(0x93) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x93)) >> 32) & 1);
    esi = esi + 0x93;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(edx, 5);
    MEM32(esp + 0x40) = 0x93;
    MEM16(esp + 0x44) = LO16(esi);
    MEM16(esp + 0x46) = 0x91;
    MEM16(esp + 0x48) = 0x1EB;
    MEM8(esp + 0x4A) = 0;
    MEM8(esp + 0x4B) = LO8(ebx);
    MEM8(esp + 0x4C) = LO8(ebx);
    MEM32(esp + 0x50) = 0;
    MEM32(esp + 0x68) = 0xFFFFFFFFu;
    MEMF(esp + 0x60) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00209188u); RECOMP_ABI_CALL(0x001E9B80u, sub_001E9B80); /* call 0x001E9B80 */

loc_00209188: ;
    xmm0 = XMM_SCALAR(MEMF(0x4158D4)); /* movss */
    _fb = (uint32_t)(0x20) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x20)) >> 32) & 1);
    esp = esp + 0x20;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(ecx, LO8(eax));
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x1C), 0x0E7Fu, 32); /* cvttss2si */
    edx = 5;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edx) < (uint32_t)(eax));
    edx = edx - eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(edx)) >> 32) & 1);
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 2 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_002091B7; /* jae: above or equal (unsigned >=) */

loc_002091AF: ;
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    goto loc_002091D1;

loc_002091B7: ;
    eax = ZX8(LO8(ecx));
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4158D0); /* mulss */
    xmm1.f[0] = xmm1.f[0] + xmm0.f[0]; /* addss */
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */

loc_002091D1: ;
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB724); /* mulss */
    PUSH32(esp, ecx);
    ecx = MEM32(esp + 0x1C);
    MEMF(esp) = xmm0.f[0]; /* movss */
    PUSH32(esp, ecx);
    PUSH32(esp, esi);
    esi = 0x7D;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x002091FFu); RECOMP_ABI_CALL(0x002097B0u, sub_002097B0); /* call 0x002097B0 */

loc_002091FF: ;
    edx = MEM32(0x8DB6F4);
    esi = MEM32(0x8DB6D8);
    edi = MEM32(esp + 0x100);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x64); /* mulss */
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, edx);
    PUSH32(esp, ecx);
    _fb = (uint32_t)(0x90) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x90)) >> 32) & 1);
    esi = esi + 0x90;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x91) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(0x91)) >> 32) & 1);
    edi = edi + 0x91;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = edi;
    eax = esi;
    MEMF(esp) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00209247u); RECOMP_ABI_CALL(0x002098C0u, sub_002098C0); /* call 0x002098C0 */

loc_00209247: ;
    eax = ZX8(LO8(eax));
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415468); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x3EC240); /* addss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4148C0); /* mulss */
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp + 0x1C) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x1C), 0x0E7Fu, 32); /* cvttss2si */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415240); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4149DC); /* mulss */
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x94) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x415448)); /* movss */
    MEMF(esp + 0x98) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x6C); /* mulss */
    _fb = (uint32_t)(0xA) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(0xA)) >> 32) & 1);
    edi = edi + 0xA;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = eax + esi + -20;
    MEM16(esp + 0x86) = LO16(edi);
    MEMF(esp + 0x9C) = xmm1.f[0]; /* movss */
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    edi = esp + 0x80;
    MEM32(esp + 0x80) = 0x27;
    MEM16(esp + 0x84) = LO16(ecx);
    MEM16(esp + 0x88) = 0x1EE;
    MEM8(esp + 0x8B) = LO8(ebx);
    MEM8(esp + 0x8C) = LO8(ebx);
    MEMF(esp + 0xA4) = xmm1.f[0]; /* movss */
    MEM32(esp + 0xA8) = 0xFFFFFFFFu;
    MEM8(esp + 0x8A) = 0x11;
    MEM32(esp + 0x90) = 5;
    MEMF(esp + 0xA0) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00209342u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00209342: ;
    esi = MEM32(0x8DB6D8);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB728); /* mulss */
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, 2);
    _fb = (uint32_t)(0x5A) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x5A)) >> 32) & 1);
    esi = esi + 0x5A;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x1CC);
    edx = SX16(LO16(esi));
    PUSH32(esp, 0xAB);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    eax = 0x65;
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020938Bu); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_0020938B: ;
    ecx = eax;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x90;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020939Cu); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_0020939C: ;
    xmm0 = XMM_SCALAR(MEMF(0x3E9F0C)); /* movss */
    ecx = esp + 0x90;
    MEMF(esp + 0xB0) = xmm0.f[0]; /* movss */
    MEMF(esp + 0xAC) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x2C)); /* movss */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ecx);
    MEMF(esp + 0xA8) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x002093D2u); RECOMP_ICALL_SAFE(0x001E6D30u, _icall_esp); /* manual call 0x001E6D30 */
    }

loc_002093D2: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB728); /* mulss */
    _fb = (uint32_t)(0xFFFFFFF2u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0xFFFFFFF2u)) >> 32) & 1);
    esi = esi + 0xFFFFFFF2u;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = esp + 0x34;
    MEM32(esp + 0x34) = 0x139;
    MEM16(esp + 0x38) = LO16(esi);
    MEM16(esp + 0x3A) = 0x94;
    MEM16(esp + 0x3C) = 0x1EB;
    MEM8(esp + 0x3E) = 0;
    MEM8(esp + 0x3F) = LO8(ebx);
    MEM8(esp + 0x40) = LO8(ebx);
    MEM32(esp + 0x44) = 0;
    MEM32(esp + 0x5C) = 0xFFFFFFFFu;
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00209457u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00209457: ;
    esi = MEM32(0x8DB6D8);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB718); /* mulss */
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x10)) >> 32) & 1);
    esp = esp + 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x93) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x93)) >> 32) & 1);
    esi = esi + 0x93;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x3F0A3D71);
    edx = 0xA8;
    ecx = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00209494u); RECOMP_ABI_CALL(0x00209690u, sub_00209690); /* call 0x00209690 */

loc_00209494: ;
    xmm0 = XMM_SCALAR(MEMF(0x4158D4)); /* movss */
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(ecx, LO8(eax));
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x1C), 0x0E7Fu, 32); /* cvttss2si */
    edx = 5;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edx) < (uint32_t)(eax));
    edx = edx - eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(edx)) >> 32) & 1);
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 2 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_002094C3; /* jae: above or equal (unsigned >=) */

loc_002094BB: ;
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    goto loc_002094DD;

loc_002094C3: ;
    eax = ZX8(LO8(ecx));
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4158D0); /* mulss */
    xmm1.f[0] = xmm1.f[0] + xmm0.f[0]; /* addss */
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */

loc_002094DD: ;
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB728); /* mulss */
    PUSH32(esp, ecx);
    ecx = MEM32(esp + 0x1C);
    MEMF(esp) = xmm0.f[0]; /* movss */
    PUSH32(esp, ecx);
    PUSH32(esp, esi);
    esi = 0x94;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020950Bu); RECOMP_ABI_CALL(0x002097B0u, sub_002097B0); /* call 0x002097B0 */

loc_0020950B: ;
    edx = MEM32(0x8DB6F8);
    esi = MEM32(0x8DB6D8);
    edi = MEM32(esp + 0x102);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x68); /* mulss */
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, edx);
    PUSH32(esp, ecx);
    _fb = (uint32_t)(0x90) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x90)) >> 32) & 1);
    esi = esi + 0x90;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0xA8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(0xA8)) >> 32) & 1);
    edi = edi + 0xA8;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = edi;
    eax = esi;
    MEMF(esp) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00209553u); RECOMP_ABI_CALL(0x002098C0u, sub_002098C0); /* call 0x002098C0 */

loc_00209553: ;
    eax = ZX8(LO8(eax));
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415468); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x3EC240); /* addss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4148C0); /* mulss */
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp + 0x1C) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x1C), 0x0E7Fu, 32); /* cvttss2si */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415240); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4149DC); /* mulss */
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x94) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x415448)); /* movss */
    MEMF(esp + 0x98) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x8DB708); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x70); /* mulss */
    _fb = (uint32_t)(0xA) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(0xA)) >> 32) & 1);
    edi = edi + 0xA;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = eax + esi + -20;
    MEM16(esp + 0x86) = LO16(edi);
    MEMF(esp + 0x9C) = xmm1.f[0]; /* movss */
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    edi = esp + 0x80;
    MEM32(esp + 0x80) = 0x27;
    MEM16(esp + 0x84) = LO16(ecx);
    MEM16(esp + 0x88) = 0x1EE;
    MEM8(esp + 0x8B) = LO8(ebx);
    MEM8(esp + 0x8C) = LO8(ebx);
    MEMF(esp + 0xA4) = xmm1.f[0]; /* movss */
    MEM32(esp + 0xA8) = 0xFFFFFFFFu;
    MEM8(esp + 0x8A) = 0x11;
    MEM32(esp + 0x90) = 5;
    MEMF(esp + 0xA0) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020964Eu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_0020964E: ;
    if (ngb_pc_setting(31) && MEM32(0x8DB72C) == ebx) {   /* the Intercept rows: their number and their karma */
        uint32_t keep[6] = { eax, ecx, edx, ebx, esi, edi };
        PUSH32(esp, 0x42F00000);
        PUSH32(esp, 0x68);
        PUSH32(esp, (uint32_t)MEM16(0x8DB73C) * 1000u);
        PUSH32(esp, 0x8DB734);
        PUSH32(esp, 0x8DB6D8);
        eax = 0x438AD4;
        g_ebp = ebp;
        g_seh_ebp = ebp;
        PUSH32(esp, 0x0020964Eu); RECOMP_ABI_CALL(0x00207E20u, sub_00207E20);
        esp = esp + 0x14;
        PUSH32(esp, 0x42F00000);
        PUSH32(esp, 0x68);
        PUSH32(esp, MEM32(0x8DB740));
        PUSH32(esp, 0x8DB738);
        PUSH32(esp, 0x8DB6D8);
        eax = 0x438B08;
        g_ebp = ebp;
        g_seh_ebp = ebp;
        PUSH32(esp, 0x0020964Eu); RECOMP_ABI_CALL(0x00207E20u, sub_00207E20);
        esp = esp + 0x14;
        eax = keep[0]; ecx = keep[1]; edx = keep[2]; ebx = keep[3]; esi = keep[4]; edi = keep[5];
    }
    _fa = (uint32_t)(MEM32(0x8DB72C)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x8DB72C), ebx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_0020967B; /* jne: not equal / not zero */

loc_00209656: ;
    edx = MEM32(0x8DB758);
    PUSH32(esp, 0x42F00000);
    PUSH32(esp, 0x69);
    PUSH32(esp, edx);
    PUSH32(esp, 0x8DB748);
    PUSH32(esp, 0x8DB6D8);
    eax = 0x438AD4;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00209678u); RECOMP_ABI_CALL(0x00207E20u, sub_00207E20); /* call 0x00207E20 */

loc_00209678: ;
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x14)) >> 32) & 1);
    esp = esp + 0x14;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0020967B: ;
    ecx = MEM32(esp + 0xFC);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00209687u); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_00209687: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

}
