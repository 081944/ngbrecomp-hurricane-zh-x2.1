/*
 * render_list.c -- room for every node in the game's render lists (#96).
 *
 * Each frame the game builds its render lists (0x954698[i], sub_001CA790 resets them) as trees of
 * 20-byte nodes {callback argument, callback, priority, child, next} walked by sub_00308D50, all
 * cut from one pool of 768 (0x950A78, the next free at 0x954724). sub_00308BF0 inserts a node by
 * priority, allocating it when it gets to an empty place -- and when the pool is used up it returns
 * nothing, and the node is silently left out. The HUD, the rest of the 2D layer and the screen
 * effects go in last (sub_00221210), so in a fight with enough enemies, blood and sparks for 768
 * nodes before them they are what disappears: the HUD for ten or twenty frames of a multi-kill,
 * its sprites piling up in their list until a quieter frame draws them all at once (#96).
 *
 * The same copy of the generated function, with a pool of ours after the game's: 4096 more nodes
 * in guest memory, given out once the game's 768 are gone and reclaimed every frame (the game
 * never frees a node; its pointer goes back to the start). The game's inlined copies of the
 * allocation (some sixty, the entities' and effects' own nodes) keep the limit of 768; what they
 * leave out, they left out on the console too. NGB_NODE_POOL=0 keeps the game's limit alone, and
 * NGB_TEST_NODES=<n> lowers the game's limit in this copy, to see the loss on a quiet PC.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include "xbox_memory_layout.h"   /* xbox_HeapAlloc: guest memory */

#define POOL_BASE  0x950A78u
#define POOL_NEXT  0x954724u
#define POOL_NODES 0x300u
#define NODE_SIZE  0x14u
#define EXTRA_NODES 4096u

static uint32_t s_extra;          /* ours, in guest memory; 0 before the first need, 1 if off */
static uint32_t s_extra_used;
static uint32_t s_limit = POOL_NODES;

void ngb_render_list_frame(void)   /* src/cheats.c, every Swap: the game's pool starts over, so ours */
{
    s_extra_used = 0;
}

static uint32_t extra_node(void)
{
    if (!s_extra) {
        const char *e = getenv("NGB_NODE_POOL");
        s_extra = e && atoi(e) == 0 ? 1 : xbox_HeapAlloc(EXTRA_NODES * NODE_SIZE, 16);
        if (!s_extra) s_extra = 1;
        if (s_extra != 1) fprintf(stderr, "[RLIST] %u render nodes after the game's %u at %08X\n",
                                  EXTRA_NODES, POOL_NODES, s_extra);
    }
    if (s_extra == 1 || s_extra_used >= EXTRA_NODES)
        return 0;
    return s_extra + NODE_SIZE * s_extra_used++;
}

/**
 * sub_00308BF0
 * Original: 0x00308BF0 - 0x00308CE0 (240 bytes, 82 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00308BF0(void)
{
    RECOMP_ENTRY(0x00308BF0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */
    int ours = 0;

loc_00308BF0: ;
    PUSH32(esp, esi);
    esi = MEM32(esp + 8);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    PUSH32(esp, edi);
    if (CMP_NE(_fa, _fb)) goto loc_00308C59; /* jne: not equal / not zero */

loc_00308BFA: ;
    /* an empty place: a node from the game's pool, (next - base) / 20 of its 768 used */
    {
        static int test = -1;
        if (test < 0) {
            const char *t = getenv("NGB_TEST_NODES");
            test = t ? atoi(t) : 0;
            if (test > 0) s_limit = (uint32_t)test;
        }
    }
    edi = MEM32(POOL_NEXT);
    if ((edi - POOL_BASE) / NODE_SIZE < s_limit) goto loc_00308C25;

loc_00308C20: ;
    edi = extra_node();            /* the game's are used up: one of ours (none: left out, as the game had it) */
    ours = 1;
    if (edi) goto loc_00308C25;
    POP32(esp, edi);
    eax = 0; /* xor self */
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_00308C25: ;
    ecx = MEM32(esp + 0x10);       /* the description: callback argument, callback, priority */
    esi = edi;
    MEM32(esi) = MEM32(ecx);
    MEM32(esi + 4) = MEM32(ecx + 4);
    MEM32(esi + 8) = MEM32(ecx + 8);
    MEM32(esi + 0xC) = 0;
    MEM32(esi + 0x10) = 0;
    if (!ours) MEM32(POOL_NEXT) = edi + NODE_SIZE;
    POP32(esp, edi);
    eax = esi;
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_00308C59: ;
    edx = MEM32(esp + 0x14);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 4 (32-bit) */
    eax = MEM32(esp + 0x10);
    if (CMP_A(_fa, _fb)) goto loc_00308CA3; /* ja: above (unsigned >) */

loc_00308C66: ;
    { uint32_t _jt = MEM32(edx * 4 + 0x308CC4); /* switch: 5 entries, 3 targets */
    if (_jt == 0x00308C6Du) goto loc_00308C6D;
    if (_jt == 0x00308C8Eu) goto loc_00308C8E;
    if (_jt == 0x00308CB9u) goto loc_00308CB9;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_00308C6D: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 8)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(eax + 8); /* comiss */

loc_00308C76: ;
    if ((_fca > _fcb)) goto loc_00308CA3; /* ja: above (unsigned >) (xmm0.f[0] vs MEMF(esi + 8)) */

loc_00308C78: ;
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    eax = MEM32(esi + 0x10);
    PUSH32(esp, eax);
    PUSH32(esp, 0x00308C83u); RECOMP_ABI_CALL(0x00308BF0u, sub_00308BF0); /* call 0x00308BF0 */

loc_00308C83: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + 0x10) = eax;
    POP32(esp, edi);
    eax = esi;
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_00308C8E: ;
    ecx = MEM32(esi + 8);
    edi = MEM32(eax + 8);
    PUSH32(esp, ebx);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    SET_LO8(ebx, (CMP_G(_fas, _fbs)) ? 1 : 0); /* setg */
    ecx = ebx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    POP32(esp, ebx);
    if (CMP_EQ(_fa, _fb)) goto loc_00308C78; /* je: equal / zero */

loc_00308CA3: ;
    PUSH32(esp, edx);
    edx = MEM32(esi + 0xC);
    PUSH32(esp, eax);
    PUSH32(esp, edx);
    PUSH32(esp, 0x00308CAEu); RECOMP_ABI_CALL(0x00308BF0u, sub_00308BF0); /* call 0x00308BF0 */

loc_00308CAE: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + 0xC) = eax;
    POP32(esp, edi);
    eax = esi;
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_00308CB9: ;
    xmm0 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(esi + 8); /* comiss */
    goto loc_00308C76;
}
