/*
 * hurricane_zh.c -- the recomp's own text in Chinese for the Hurricane x2.1 build.
 *
 * x2.1 puts its Chinese in the game's Japanese column; the game reads that column while [0xBCE030] is 1
 * (0x21B170: ([0xDA4978] == 0), the language word of the saved settings; the text lookup 0x1E6AE0 takes
 * the column as its argument, +4 English, +8 Japanese). The recomp's own text follows it:
 *
 * - in the game's text table (src/pc_menu.c, src/main_menu.c) its Japanese column is the Chinese in the
 *   game's encoding: x2.1's charmap -- the characters it lacks are added to the common glyph page B1
 *   (texture 0xBC, 256 x 2048 P8, 2048 cells of which x2.1 uses 2039): their codes appended to block 1 of
 *   jpfont.map (pointers at [0x8D4438]; each block's codes end in 0x20 zero bytes, so the nine codes fit
 *   without moving anything), their cells written into the page, each as x2.1's build draws its glyphs.
 *   All of that only when the page and the map are x2.1's as tools/hurricane/make_zh.py read them from the
 *   disc; if not, the strings that need the new glyphs go back to English;
 * - in the pictures and pages drawn here (src/pause_tab.c, src/ingame_menu.c) in an embedded font, the
 *   game's own text face (x2.1: Source Han Sans CN Bold), and its picture words as x2.1 draws its own.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbox_memory_layout.h"
#include "hurricane_zh.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

extern const unsigned char ngb_font_cjk[];

#define TEXT_LANG   0xBCE030      /* byte: 1 the Japanese column */
#define MAP_BLOCKS  0x8D4438      /* jpfont.map's block pointers, 0 ended */
#define TEX_TABLE   0x8DBB28      /* per texture number {header, palette} */
#define TEX_B1      0xBC          /* the sprite of glyph page B1 */

int ngb_zh_on(void)
{
    static int off = -1;
    if (off < 0)
        off = getenv("NGB_NO_ZH") != NULL;
    return !off && MEM8(TEXT_LANG) == 1;
}

const NgbZhText *ngb_zh_find(const char *ctx, const char *en)
{
    if (!en)
        return NULL;
    for (int i = 0; i < ngb_zh_text_count; i++)
        if (!strcmp(ngb_zh_texts[i].en, en) && !strcmp(ngb_zh_texts[i].ctx, ctx))
            return &ngb_zh_texts[i];
    return NULL;
}

const char *ngb_zh_ctx(const char *ctx, const char *en)
{
    if (!ngb_zh_on())
        return NULL;
    const NgbZhText *t = ngb_zh_find(ctx, en);
    return t ? t->zh : NULL;
}

const char *ngb_zh(const char *en) { return ngb_zh_ctx("", en); }

const NgbZhArt *ngb_zh_art(const char *en)
{
    if (!ngb_zh_on() || !en)
        return NULL;
    for (int i = 0; i < ngb_zh_art_count; i++)
        if (!strcmp(ngb_zh_arts[i].en, en))
            return &ngb_zh_arts[i];
    return NULL;
}

int ngb_zh_is_wide(const char *s)
{
    for (; s && *s; s++)
        if ((unsigned char)*s >= 0x80)
            return 1;
    return 0;
}

/* ---------- the embedded font ---------- */

static stbtt_fontinfo s_font;
static int s_font_ok = -1;

static int font_ready(void)
{
    if (s_font_ok < 0)
        s_font_ok = stbtt_InitFont(&s_font, ngb_font_cjk, stbtt_GetFontOffsetForIndex(ngb_font_cjk, 0)) ? 1 : 0;
    return s_font_ok;
}

static int utf8_next(const char **ps)
{
    const unsigned char *s = (const unsigned char *)*ps;
    int c = *s, n = c < 0x80 ? 0 : c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
    if (n)
        c &= 0x3F >> n;
    s++;
    for (int i = 0; i < n && (*s & 0xC0) == 0x80; i++, s++)
        c = c << 6 | (*s & 0x3F);
    *ps = (const char *)s;
    return c;
}

/* The ideographs about as tall as the Latin capitals beside them (px * 0.66): the em at px * 0.78. */
#define EM_K 0.78f

float ngb_zh_text(uint8_t *m, int w, int h, const char *s, float px, float sx, float x, float base)
{
    if (!font_ready())
        return 0;
    float scale = stbtt_ScaleForMappingEmToPixels(&s_font, px * EM_K), x0 = x;
    static uint8_t g[256 * 256];
    while (*s) {
        int cp = utf8_next(&s), adv, lsb, x1, y1, x2, y2;
        const char *peek = s;
        int next = *peek ? utf8_next(&peek) : 0;
        float fx = x - floorf(x);
        stbtt_GetCodepointHMetrics(&s_font, cp, &adv, &lsb);
        if (m) {
            stbtt_GetCodepointBitmapBoxSubpixel(&s_font, cp, scale * sx, scale, fx, 0, &x1, &y1, &x2, &y2);
            int gw = x2 - x1, gh = y2 - y1;
            if (gw > 0 && gh > 0 && gw <= 256 && gh <= 256) {
                stbtt_MakeCodepointBitmapSubpixel(&s_font, g, gw, gh, gw, scale * sx, scale, fx, 0, cp);
                for (int yy = 0; yy < gh; yy++)
                    for (int xx = 0; xx < gw; xx++) {
                        int X = (int)floorf(x) + x1 + xx, Y = (int)lroundf(base) + y1 + yy;
                        if (X >= 0 && Y >= 0 && X < w && Y < h && g[yy * gw + xx] > m[Y * w + X])
                            m[Y * w + X] = g[yy * gw + xx];
                    }
            }
        }
        x += adv * scale * sx;
        if (next)
            x += stbtt_GetCodepointKernAdvance(&s_font, cp, next) * scale * sx;
    }
    return x - x0;
}

/* ---------- the game's text table ---------- */

/* Its entries ours or the recomp's: both columns. The menus here read the English column even in the Japanese
 * mode (as the original, whose menus are English there; x2.1 put its Chinese in the strings both columns share,
 * docs/17 of the localization), so the column does not choose: each frame both point at the Chinese while
 * ngb_zh_on() -- once the glyph page has what the string needs -- else at the English. */
/* (96 until 0.9.45: the PC pages' 99 strings since 0.9.40 filled it, and the main menu's two stayed English) */
static struct { uint32_t en_slot, jp_slot, en, zh; int new_glyphs; } s_ent[256];
static int s_nent, s_glyphs;      /* s_glyphs: 1 the page has the added cells, -1 it cannot */

void ngb_zh_engine_text(uint32_t en_slot, uint32_t jp_slot, const char *en)
{
    const NgbZhText *t = ngb_zh_find("", en);
    if (!t || !t->sjis)
        return;
    if (s_nent == (int)(sizeof s_ent / sizeof s_ent[0])) {
        fprintf(stderr, "[ZH] engine text table full: \"%s\" stays English\n", en);
        return;
    }
    uint32_t n = (uint32_t)strlen(t->sjis) + 1, va = xbox_HeapAlloc(n, 4);
    if (!va)
        return;
    memcpy((void *)XBOX_PTR(va), t->sjis, n);
    s_ent[s_nent].en_slot = en_slot;
    s_ent[s_nent].jp_slot = jp_slot;
    s_ent[s_nent].en = MEM32(en_slot);
    s_ent[s_nent].zh = va;
    s_ent[s_nent].new_glyphs = t->new_glyphs;
    s_nent++;
}

static void engine_sync(void)
{
    int zh = ngb_zh_on();
    for (int i = 0; i < s_nent; i++) {
        uint32_t v = zh && (!s_ent[i].new_glyphs || s_glyphs > 0) ? s_ent[i].zh : s_ent[i].en;
        if (MEM32(s_ent[i].en_slot) != v)
            MEM32(s_ent[i].en_slot) = v;
        if (MEM32(s_ent[i].jp_slot) != v)
            MEM32(s_ent[i].jp_slot) = v;
    }
}

static void glyphs_failed(const char *why)
{
    fprintf(stderr, "[ZH] glyph page not as x2.1's (%s): the text needing new characters stays English\n", why);
    s_glyphs = -1;
}

/* ---------- the glyph page ---------- */

static void swizzle_masks(unsigned w, unsigned h, uint32_t *mx, uint32_t *my)
{
    uint32_t bit = 1, m = 1;
    *mx = *my = 0;
    for (;;) {
        int more = 0;
        if (bit < w) { *mx |= m; m <<= 1; more = 1; }
        if (bit < h) { *my |= m; m <<= 1; more = 1; }
        if (!more)
            break;
        bit <<= 1;
    }
}

static uint32_t deposit(uint32_t v, uint32_t mask)
{
    uint32_t r = 0;
    for (uint32_t b = 1; mask; b <<= 1) {
        uint32_t low = mask & -mask;
        if (v & b)
            r |= low;
        mask &= mask - 1;
    }
    return r;
}

/* 0 not loaded yet, 1 as x2.1's (cells added), -1 something else */
static int page_add(void)
{
    uint32_t blocks = MEM32(MAP_BLOCKS);
    /* sprite 0xBC: the texture registry (0x452A4C-0x45A7F4, 0x34 bytes an entry: +0 registered, +0x10 its
     * sprites, 24 bytes each, +0x14 the file -- 0x645, demo.afs #3 --, +0x20 the first sprite id, +0x24 how
     * many); the sprite's +0x10 its texture number in the table {header, palette} at 0x8DBB28 */
    uint32_t hdr = 0, pal = 0;
    for (uint32_t e = 0x452A4C; e < 0x45A7F4; e += 0x34) {
        uint32_t id0 = MEM32(e + 0x20), cnt = MEM32(e + 0x24);
        if (MEM32(e) && MEM32(e + 0x14) == 0x645 && id0 <= TEX_B1 && TEX_B1 < id0 + cnt) {
            uint32_t o = MEM32(e + 0x10) + (TEX_B1 - id0) * 24, t = MEM32(o + 0x10);
            if (MEM16(o + 0x14) == 256 && MEM16(o + 0x16) == 2048 && t < 0x516) {
                hdr = MEM32(TEX_TABLE + t * 8);
                pal = MEM32(TEX_TABLE + t * 8 + 4);
            }
            break;
        }
    }
    if (!blocks || !MEM32(blocks) || !MEM32(blocks + 4) || !hdr || !pal)
        return 0;
    uint32_t fmt = MEM32(hdr + 0xC);
    if ((fmt >> 8 & 0xFF) != 0x0B || (fmt >> 20 & 0xF) != 8 || (fmt >> 24 & 0xF) != 11)
        return glyphs_failed("texture 0xBC not 256 x 2048 P8"), -1;
    uint32_t pdata = MEM32(pal + 4) | 0x80000000u, h = 0x811C9DC5u;
    for (int i = 0; i < 0x400; i++)
        h = (h ^ MEM8(pdata + i)) * 0x01000193u;
    if (h != ngb_zh_pal_fnv)
        return glyphs_failed("palette"), -1;
    uint32_t b1 = MEM32(blocks + 4);
    int n = 0;
    while (n < 4096 && MEM8(b1 + 2 * n))
        n++;
    if (n == ngb_zh_b1_count) {                            /* as x2.1's: the codes appended */
        if (MEM8(b1 + 2 * (n - 1)) != ngb_zh_b1_last[0] || MEM8(b1 + 2 * (n - 1) + 1) != ngb_zh_b1_last[1])
            return glyphs_failed("block 1's last code"), -1;
        for (int i = 0; i < 0x20; i++)
            if (MEM8(b1 + 2 * n + i))
                return glyphs_failed("no room after block 1"), -1;
        for (int i = 0; i < ngb_zh_new_count; i++) {
            MEM8(b1 + 2 * (n + i)) = ngb_zh_new_code[i][0];
            MEM8(b1 + 2 * (n + i) + 1) = ngb_zh_new_code[i][1];
        }
    } else if (n == ngb_zh_b1_count + ngb_zh_new_count) {    /* added before */
        for (int i = 0; i < ngb_zh_new_count; i++)
            if (MEM8(b1 + 2 * (ngb_zh_b1_count + i)) != ngb_zh_new_code[i][0] ||
                MEM8(b1 + 2 * (ngb_zh_b1_count + i) + 1) != ngb_zh_new_code[i][1])
                return glyphs_failed("block 1's codes"), -1;
    } else {
        return glyphs_failed("block 1's length"), -1;
    }
    /* the cells: blank in x2.1's page, or ours already */
    uint32_t data = MEM32(hdr + 4) | 0x80000000u, mx, my;
    swizzle_masks(256, 2048, &mx, &my);
    for (int i = 0; i < ngb_zh_new_count; i++) {
        int cell = ngb_zh_b1_count + i, cx = cell % 16 * 16, cy = cell / 16 * 16, same = 1, blank = 1;
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                uint8_t v = MEM8(data + (deposit(cx + x, mx) | deposit(cy + y, my)));
                same &= v == ngb_zh_new_cell[i][y * 16 + x];
                blank &= v == ngb_zh_blank;
            }
        if (same)
            continue;
        if (!blank)
            return glyphs_failed("a free cell is not blank"), -1;
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++)
                MEM8(data + (deposit(cx + x, mx) | deposit(cy + y, my))) = ngb_zh_new_cell[i][y * 16 + x];
    }
    return 1;
}

void ngb_zh_font_tick(void)
{
    static unsigned k;
    static int logged;
    engine_sync();
    if (s_glyphs < 0 || getenv("NGB_NO_ZH") || k++ % 30)
        return;
    int r = page_add();
    if (r > 0)
        s_glyphs = 1;                                  /* (0 again below if reloaded and not yet redone) */
    if (r > 0 && !logged) {
        logged = 1;
        fprintf(stderr, "[ZH] %d characters added to the glyph page (cells %d-%d)\n", ngb_zh_new_count,
                ngb_zh_b1_count, ngb_zh_b1_count + ngb_zh_new_count - 1);
    }
}
