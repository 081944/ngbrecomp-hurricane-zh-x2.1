/*
 * movie_skip.c -- "Skip intro movie": sub_00344BE0 by hand.
 *
 * sub_00344BE0 is CRI Sofdec's mwPlyGetStat, which the title polls every
 * frame while a movie plays (sub_0031DA50); 3 (PLAYEND) makes it run its
 * own end-of-movie path, which also stops the narration (movie/ninja.aix)
 * and the subtitles. The body is the lifted one, kept here (listed in
 * src/manual_functions.json so the pipeline does not generate it), with one
 * change: while a movie the player chose to skip is playing, "playing"
 * reads as "ended".
 *
 * Skipped with Skip Intros: the boot movie (2_VI_WIDE_B.sfd / 4_VI_STD_B.sfd).
 * Not the New Game prologue (ninja_gaiden.sfd): it is the story's start, and
 * with Skip Any Cutscene Start ends it like any movie (#28). Which movie is playing
 * comes from the movie file the title last read (ngb_movie_read, from the
 * kernel's read hook): it opens every movie at boot to probe them, so the
 * last one opened says nothing.
 *
 * And any movie, by pressing Start (ngb_skip_button, src/keyboard.c),
 * with Skip Any Cutscene on (Gameplay, setting 34, off unless chosen: the game's own Start does not end the prologue; it also skips
 * the in-game cutscenes, src/cutscene_skip.c) -- once every button has been
 * let go since it began, so neither the press that started it nor one held
 * through it ends it.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <math.h>
#include <string.h>
#include <strings.h>

int ngb_skip_intro;
static volatile int s_skip_current;
static int s_released;   /* the buttons let go since this movie began */
int ngb_skip_button(void);
int ngb_pc_setting(int which);   /* src/ingame_menu.c: 34 Skip Any Cutscene */

void ngb_movie_read(const char *host_path)
{
    const char *base = NULL, *dir = NULL;   /* the name and the folder it is in (either separator) */
    for (const char *c = host_path; *c; c++)
        if (*c == '/' || *c == '\\') {
            dir = base;
            base = c;
        }
    if (!base || !dir || strncasecmp(dir + 1, "movie", 5) || base - dir != 6 || !strstr(base, ".sfd"))
        return;
    base++;
    s_released = 0;
    s_skip_current = ngb_skip_intro &&
        (!strcasecmp(base, "2_VI_WIDE_B.sfd") || !strcasecmp(base, "4_VI_STD_B.sfd"));
}

static int ngb_movie_skip_now(void)
{
    if (s_skip_current)
        return 1;
    if (!ngb_pc_setting(34))
        return 0;
    if (!ngb_skip_button())
        s_released = 1;
    else if (s_released)
        s_skip_current = 1;   /* (until the next movie is read) */
    return s_skip_current;
}

void sub_00344BE0(void)
{
    RECOMP_ENTRY(0x00344BE0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_00344BE0: ;
    ecx = MEM32(esp + 4);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00344BED; /* je: equal / zero */

loc_00344BE8: ;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ecx), 1 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00344C04; /* je: equal / zero */

loc_00344BED: ;
    PUSH32(esp, 0xFFFFFFF4u);
    PUSH32(esp, 0x00344BF4u); RECOMP_ABI_CALL(0x00346900u, sub_00346900); /* call 0x00346900 */

loc_00344BF4: ;
    PUSH32(esp, 0x3E669C);
    PUSH32(esp, 0x00344BFEu); RECOMP_ABI_CALL(0x00346CB0u, sub_00346CB0); /* call 0x00346CB0 */

loc_00344BFE: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    esp += 4; return; /* ret */

loc_00344C04: ;
    eax = MEM32(ecx + 4);
    {   /* preparing or playing: the game's widescreen mode for it (src/pc_menu.c); ended: not */
        extern void ngb_movie_playing(int playing);
        ngb_movie_playing(eax == 1 || eax == 2);
    }
    if (eax == 2 && ngb_movie_skip_now()) {
        eax = 3;                 /* MWPLY_STAT_PLAYEND: as if it had played */
        esp += 4; return;
    }
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 2 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00344C36; /* jne: not equal / not zero */

loc_00344C0C: ;
    eax = MEM32(ecx + 0x3C);
    PUSH32(esp, eax);
    PUSH32(esp, 0x00344C15u); RECOMP_ABI_CALL(0x0034E4B0u, sub_0034E4B0); /* call 0x0034E4B0 */

loc_00344C15: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 4 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00344C31; /* je: equal / zero */

loc_00344C1D: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(6) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 6 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00344C31; /* je: equal / zero */

loc_00344C22: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    SET_LO8(ecx, (CMP_GE(_fas, _fbs)) ? 1 : 0); /* setge */
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    ecx = ecx & 3;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    eax = ecx;
    esp += 4; return; /* ret */

loc_00344C31: ;
    eax = 2;

loc_00344C36: ;
    esp += 4; return; /* ret */

}

/*
 * sub_00331FD0 (0x00331FD0 - 0x00331FED): set one channel's output volume of a CRI stream, the volume in
 * tenths of a dB (sub_00331940 multiplies it by 10 into the mixer's hundredths). The Team Ninja splash,
 * played as three stereo streams when the speakers are 5.1 (the host mixer reports them on a six-channel
 * device), sets its six channels here from the game's volume table -- in hundredths, where every other
 * use of the table divides by 10 first -- so they came out 20 times too quiet in dB, -96 dB: no sound
 * (#104). From that call (its return address 0x0031D870) the volume is divided by 10, as at the others.
 */
void sub_00331FD0(void)
{
    RECOMP_ENTRY(0x00331FD0u);
    uint32_t ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */
    if (MEM32(esp) == 0x0031D870u)
        MEM32(esp + 0xC) = (uint32_t)((int32_t)MEM32(esp + 0xC) / 10);
    PUSH32(esp, esi);
    PUSH32(esp, 0x00331FD6u); RECOMP_ABI_CALL(0x001A4580u, sub_001A4580); /* call 0x001A4580 */
    ecx = MEM32(esp + 0x10);   /* the volume */
    eax = MEM32(esp + 0xC);    /* the channel */
    esi = MEM32(esp + 8);      /* the stream */
    PUSH32(esp, 0x00331FE7u); RECOMP_ABI_CALL(0x00331940u, sub_00331940); /* call 0x00331940 */
    POP32(esp, esi);
    g_seh_ebp = ebp; sub_001A4580(); return; /* tail jmp 0x001A4580 */
}
