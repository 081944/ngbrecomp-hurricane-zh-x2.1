/* The game's one-shot costume loader. Keep its resource transition on its original world task. */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "costume_switch.h"
#include <stdio.h>
#include <string.h>

static void pose_call(void (*fn)(void))
{
    PUSH32(esp, 0x002FC810u);
    fn();
}

static void rebuild_skeleton(void)
{
    /* Slot zero's animation task (001914D0) keeps these globals and a separate
     * 2650-byte cache. Costume reloads retain the actor/model IDs, so the usual
     * ID comparison does not refresh its resource-dependent geometry or frames. */
    uint8_t task_state = MEM8(0xDA28F0);
    uint32_t task_flags = MEM32(0xDA28F8);
    uint32_t regs[] = {g_eax, g_ecx, g_edx, g_esp, g_ebx, g_esi, g_edi, g_ebp, g_seh_ebp};
    RecompXmm xmm[] = {g_xmm0, g_xmm1, g_xmm2, g_xmm3, g_xmm4, g_xmm5, g_xmm6, g_xmm7};
    double fp[8];
    int fp_top = g_fp_top;
    memcpy(fp, g_fp_stack, sizeof fp);
    MEM8(0xDA28F0) = 0;
    MEM8(0xDA28F8) = 0;
    MEM32(0xDA28F4) = 0x4B4E50;
    MEM32(0xDA28FC) = 0x58A3A0 + MEM8(0x4B7640) * 0x210u;
    MEM32(0xDA2900) = 0x809980;

    /* Each cached sample is keyed by frame number, not resource identity.
     * Invalidate all three rotating buffers before sampling the new model. */
    for (unsigned a = 0x80AF78; a <= 0x80AF84; a += 4) {
        uint32_t buffer = MEM32(a);
        if (buffer >= 0x80AF90 && buffer + 0x430 <= 0x80BFC4)
            MEM32(buffer + 0x400) = 0xFFFFFFFFu;
    }
    pose_call(sub_001919F0); /* Rebind model metadata and native skeletal constraints. */
    MEM8(0x4B75FA) = 0;     /* 27AA: the reset is performed synchronously below. */
    MEM8(0x809980 + 0x2649) = 0;
    pose_call(sub_00188C40); /* Resolve the current animation again in the new resource. */
    pose_call(sub_00188E70); /* Execute its initial pose commands. */
    MEM16(0x809980 + 0x2644) = 0; /* No interpolation from a different skeleton. */
    MEM16(0x809980 + 0x2646) = 0;
    MEM8(0x809980 + 0x2648) = 0;
    MEM8(0x809980 + 0x264A) = 0;
    /* The script's 22BC flag requests a clip bind, not a blend. Consume it via
     * the native path before sampling: it replaces keyframe cursors into the
     * released resource and selects the script's starting frame. */
    pose_call(sub_0018E8B0);
    pose_call(sub_0018EDF0); /* Direct sample, rather than the 00190620 blend path. */
    switch (MEM8(0x809980 + 0x234D)) {
    case 0: pose_call(sub_00189EB0); break;
    case 1: pose_call(sub_0018B110); break;
    default: pose_call(sub_0018B380); break;
    }
    pose_call(sub_0018D290);
    pose_call(sub_00191C30);
    fprintf(stderr, "[COSTUME] skeleton rebuilt script=%08X blend=%u\n",
            MEM32(0x809980 + 0x22A0), MEM16(0x809980 + 0x2644));

    /* Preserve scheduling state, but keep the freshly rebound actor/script/
     * cache pointers. Restoring the whole record reintroduces the old model
     * on the next animation tick. */
    MEM8(0xDA28F0) = task_state;
    MEM32(0xDA28F8) = task_flags;
    g_eax = regs[0]; g_ecx = regs[1]; g_edx = regs[2]; g_esp = regs[3];
    g_ebx = regs[4]; g_esi = regs[5]; g_edi = regs[6]; g_ebp = regs[7]; g_seh_ebp = regs[8];
    g_xmm0 = xmm[0]; g_xmm1 = xmm[1]; g_xmm2 = xmm[2]; g_xmm3 = xmm[3];
    g_xmm4 = xmm[4]; g_xmm5 = xmm[5]; g_xmm6 = xmm[6]; g_xmm7 = xmm[7];
    memcpy(g_fp_stack, fp, sizeof fp);
    g_fp_top = fp_top;
}

static NgbCostumeState s_costume;
static unsigned s_trace_frames;
static void costume_observe(unsigned fallback)
{
    unsigned area = MEM16(0xDB9C6E);
    ngb_costume_observe(&s_costume, area, fallback);
    fprintf(stderr, "[COSTUME] native update fallback=%02X override=%02X area=%04X\n",
            fallback, MEM8(0x589761), area);
}

int ngb_costume_can_request(uint16_t costume)
{
    if (MEM8(0x4B75DE) == 0 && MEM8(0xD6A100) == 0xFF)
        ngb_costume_seed(&s_costume, MEM16(0xDB9C6E), MEM8(0x589761), MEM8(0x589760));
    return MEM8(0x4B75DE) == 0 &&
           ngb_costume_ready(&s_costume, MEM16(0xDB9C6E), costume, MEM8(0xD6A100));
}

int ngb_costume_request(uint16_t costume)
{
    if (!ngb_costume_can_request(costume)) return 0;
    unsigned trigger;
    if (!ngb_costume_submit(&s_costume, MEM16(0xDB9C6E), costume, &trigger)) return 0;
    MEM8(0x589761) = (uint8_t)costume;
    MEM8(0xD6A100) = (uint8_t)trigger;
    s_trace_frames = 180;
    return 1;
}

void ngb_costume_frame_tick(void)
{
    if (!s_trace_frames) return;
    fprintf(stderr, "[COSTUME_TRACE] left=%u task=(%08X %08X %08X %08X) anim=(%u,%u,%u,%u) root=(%.3f %.3f %.3f) bones=(%.3f %.3f %.3f)\n",
            s_trace_frames,
            MEM32(0xDA28F0), MEM32(0xDA28F4), MEM32(0xDA28FC), MEM32(0xDA2900),
            MEM16(0x809980 + 0x2644), MEM16(0x809980 + 0x2646),
            MEM8(0x809980 + 0x2648), MEM8(0x809980 + 0x264A),
            MEMF(0x4B5710), MEMF(0x4B5714), MEMF(0x4B5718),
            MEMF(0x4B4E50 + 0x1120), MEMF(0x4B4E50 + 0x1150), MEMF(0x4B4E50 + 0x18E0));
    --s_trace_frames;
}

/**
 * sub_002FC6F0
 * Original: 0x002FC6F0 - 0x002FC847 (343 bytes, 101 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_002FC6F0(void)
{
    RECOMP_ENTRY(0x002FC6F0u);
    /* Only a panel request gets an in-place world-transform restore. Scripted
     * costume changes may intentionally move the player. */
    int panel_reload = s_costume.pending && s_costume.area == MEM16(0xDB9C6E) &&
                       s_costume.target == LO8(eax);
    int panel_suspended = 0;
    NgbCostumePose pose;
    costume_observe(LO8(eax));
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_002FC6F0: ;
    SET_LO8(ecx, MEM8(0x589761));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0xFF (8-bit) */
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    if (CMP_AE(_fa, _fb)) goto loc_002FC700; /* jae: above or equal (unsigned >=) */

loc_002FC6FE: ;
    SET_LO8(eax, LO8(ecx));

loc_002FC700: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xFF (8-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_002FC843; /* jae: above or equal (unsigned >=) */

loc_002FC708: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(MEM8(0x589760)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), MEM8(0x589760) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002FC843; /* je: equal / zero */

loc_002FC714: ;
    if (panel_reload) {
        ngb_costume_pose_save(&pose, (const uint8_t *)XBOX_PTR(0x4B4E50));
        /* The retail path suspends gameplay after beginning resource teardown.
         * A live change needs that suspension before touching the old model.
         * Preserve EAX: it still carries the requested costume here. */
        uint32_t target = eax;
        PUSH32(esp, 0x002FC714u);
        sub_002F62C0();
        eax = target;
        panel_suspended = 1;
    }
    ecx = 0xDB5400;
    MEM8(0x589760) = LO8(eax);
    PUSH32(esp, 0x002FC723u); RECOMP_ABI_CALL(0x001718E0u, sub_001718E0); /* call 0x001718E0 */

loc_002FC723: ;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebx);
    PUSH32(esp, 0xDB5400);
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    PUSH32(esp, 0x002FC733u); RECOMP_ABI_CALL(0x00173880u, sub_00173880); /* call 0x00173880 */

loc_002FC733: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, ebx);
    PUSH32(esp, 0xFFFF);
    PUSH32(esp, 0xDB5400);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, 0x002FC74Au); RECOMP_ABI_CALL(0x00172D60u, sub_00172D60); /* call 0x00172D60 */

loc_002FC74A: ;
    esi = MEM32(0xDB99F4);
    edi = MEM32(0xDB99E4);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002FC83C; /* jne: not equal / not zero */

loc_002FC761: ;
    if (!panel_reload) {
        PUSH32(esp, 0x002FC766u); RECOMP_ABI_CALL(0x002F62C0u, sub_002F62C0); /* call 0x002F62C0 */
    }

loc_002FC766: ;
    eax = MEM32(0x4B752C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    MEM32(0x4B7528) = ebx;
    if (CMP_EQ(_fa, _fb)) goto loc_002FC780; /* je: equal / zero */

loc_002FC775: ;
    PUSH32(esp, 0x002FC77Au); RECOMP_ABI_CALL(0x003077E0u, sub_003077E0); /* call 0x003077E0 */

loc_002FC77A: ;
    MEM32(0x4B752C) = ebx;

loc_002FC780: ;
    eax = MEM32(0x4B7530);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002FC794; /* je: equal / zero */

loc_002FC789: ;
    PUSH32(esp, 0x002FC78Eu); RECOMP_ABI_CALL(0x003077E0u, sub_003077E0); /* call 0x003077E0 */

loc_002FC78E: ;
    MEM32(0x4B7530) = ebx;

loc_002FC794: ;
    SET_LO8(edx, 0); /* xor self */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    MEM32(0x4B7520) = ebx;
    MEM32(0x4B7524) = ebx;
    PUSH32(esp, 0x002FC7A7u); RECOMP_ABI_CALL(0x0016C5F0u, sub_0016C5F0); /* call 0x0016C5F0 */

loc_002FC7A7: ;
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    PUSH32(esp, 0x002FC7AEu); RECOMP_ABI_CALL(0x0016C690u, sub_0016C690); /* call 0x0016C690 */

loc_002FC7AE: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_002FC7B1: ;
    eax = 0xDA5678;

loc_002FC7B6: ;
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(eax), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002FC818; /* jne: not equal / not zero */

loc_002FC7BB: ;
    _fb = (uint32_t)(0x61C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x61C;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xDB19F8) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xDB19F8 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_002FC7B6; /* jl: less (signed <) */

loc_002FC7C7: ;
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    PUSH32(esp, 0x002FC7CEu); RECOMP_ABI_CALL(0x0016C570u, sub_0016C570); /* call 0x0016C570 */

loc_002FC7CE: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_002FC7D1: ;
    eax = 0xDA5678;

loc_002FC7D6: ;
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(eax), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002FC824; /* jne: not equal / not zero */

loc_002FC7DB: ;
    _fb = (uint32_t)(0x61C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x61C;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xDB19F8) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xDB19F8 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_002FC7D6; /* jl: less (signed <) */

loc_002FC7E7: ;
    SET_LO8(edx, 1);
    PUSH32(esp, 0x002FC7EEu); RECOMP_ABI_CALL(0x0016C5F0u, sub_0016C5F0); /* call 0x0016C5F0 */

loc_002FC7EE: ;
    edi = edi;

loc_002FC7F0: ;
    eax = 0xDA5678;

loc_002FC7F5: ;
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(eax), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002FC830; /* jne: not equal / not zero */

loc_002FC7FA: ;
    _fb = (uint32_t)(0x61C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x61C;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xDB19F8) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xDB19F8 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_002FC7F5; /* jl: less (signed <) */

loc_002FC806: ;
    esi = 0x4B4E50;
    PUSH32(esp, 0x002FC810u); RECOMP_ABI_CALL(0x002EA5E0u, sub_002EA5E0); /* call 0x002EA5E0 */

loc_002FC810: ;
    if (panel_reload) {
        fprintf(stderr, "[COSTUME] restore root before=(%.3f %.3f %.3f)\n",
                MEMF(0x4B5710), MEMF(0x4B5714), MEMF(0x4B5718));
        fprintf(stderr, "[COSTUME_ROOTS] before="
                "(%.3f %.3f %.3f) (%.3f %.3f %.3f) (%.3f %.3f %.3f) "
                "(%.3f %.3f %.3f) (%.3f %.3f %.3f) (%.3f %.3f %.3f) (%.3f %.3f %.3f)\n",
                MEMF(0x4B4E50 + 0x8C0), MEMF(0x4B4E50 + 0x8C4), MEMF(0x4B4E50 + 0x8C8),
                MEMF(0x4B4E50 + 0x8D0), MEMF(0x4B4E50 + 0x8D4), MEMF(0x4B4E50 + 0x8D8),
                MEMF(0x4B4E50 + 0x8E0), MEMF(0x4B4E50 + 0x8E4), MEMF(0x4B4E50 + 0x8E8),
                MEMF(0x4B4E50 + 0x18E0), MEMF(0x4B4E50 + 0x18E4), MEMF(0x4B4E50 + 0x18E8),
                MEMF(0x4B4E50 + 0x1910), MEMF(0x4B4E50 + 0x1914), MEMF(0x4B4E50 + 0x1918),
                MEMF(0x4B4E50 + 0x20A0), MEMF(0x4B4E50 + 0x20A4), MEMF(0x4B4E50 + 0x20A8),
                MEMF(0x4B4E50 + 0x20D0), MEMF(0x4B4E50 + 0x20D4), MEMF(0x4B4E50 + 0x20D8));
        ngb_costume_pose_restore(&pose, (uint8_t *)XBOX_PTR(0x4B4E50));
        rebuild_skeleton();
        ngb_costume_pose_restore(&pose, (uint8_t *)XBOX_PTR(0x4B4E50));
        fprintf(stderr, "[COSTUME] restore root after=(%.3f %.3f %.3f)\n",
                MEMF(0x4B5710), MEMF(0x4B5714), MEMF(0x4B5718));
    }
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    g_seh_ebp = ebp; sub_002F6300(); return; /* tail jmp 0x002F6300 */

loc_002FC818: ;
    PUSH32(esp, 1);
    PUSH32(esp, 0x002FC81Fu); RECOMP_ABI_CALL(0x002FE1D0u, sub_002FE1D0); /* call 0x002FE1D0 */

loc_002FC81F: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_002FC7B1;

loc_002FC824: ;
    PUSH32(esp, 1);
    PUSH32(esp, 0x002FC82Bu); RECOMP_ABI_CALL(0x002FE1D0u, sub_002FE1D0); /* call 0x002FE1D0 */

loc_002FC82B: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_002FC7D1;

loc_002FC830: ;
    PUSH32(esp, 1);
    PUSH32(esp, 0x002FC837u); RECOMP_ABI_CALL(0x002FE1D0u, sub_002FE1D0); /* call 0x002FE1D0 */

loc_002FC837: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_002FC7F0;

loc_002FC83C: ;
    SET_LO8(edx, 1);
    PUSH32(esp, 0x002FC843u); RECOMP_ABI_CALL(0x0016C5F0u, sub_0016C5F0); /* call 0x0016C5F0 */

loc_002FC843: ;
    if (panel_suspended) {
        /* The resource lookup can fail before reaching the normal resume tail.
         * The early panel suspension must be balanced on this path as well. */
        ngb_costume_pose_restore(&pose, (uint8_t *)XBOX_PTR(0x4B4E50));
        PUSH32(esp, 0x002FC843u);
        sub_002F6300();
    }
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

}
