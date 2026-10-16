/* hurricane_zh.h -- the recomp's own text in Chinese (Hurricane x2.1), when the game's text language is the
 * Japanese column (x2.1's Chinese). The table: tools/hurricane/zh_text.tsv -> src/hurricane_zh_data.c
 * (tools/hurricane/make_zh.py). */
#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    const char *ctx, *en, *zh;    /* zh: UTF-8 */
    const char *sjis;             /* for the game's text table: its encoding, or NULL */
    int new_glyphs;               /* sjis uses a character added to the glyph page */
} NgbZhText;

typedef struct {
    const char *en, *zh;
    int w, h;
    const unsigned char *a;       /* coverage 0-255, w x h */
} NgbZhArt;

extern const NgbZhText ngb_zh_texts[];
extern const int ngb_zh_text_count;
extern const NgbZhArt ngb_zh_arts[];
extern const int ngb_zh_art_count;
extern const int ngb_zh_b1_count, ngb_zh_new_count;
extern const unsigned char ngb_zh_b1_last[2], ngb_zh_new_code[][2], ngb_zh_new_cell[][256], ngb_zh_blank;
extern const uint32_t ngb_zh_pal_fnv;

/* The game shows its Japanese column (x2.1: Chinese). NGB_NO_ZH=1: never. */
int ngb_zh_on(void);
/* en's Chinese (ctx "" or a context) while ngb_zh_on(), else NULL. */
const char *ngb_zh(const char *en);
const char *ngb_zh_ctx(const char *ctx, const char *en);
const NgbZhText *ngb_zh_find(const char *ctx, const char *en);
/* A word drawn as a picture, while ngb_zh_on(). */
const NgbZhArt *ngb_zh_art(const char *en);
/* Any byte above 0x7F (text that is Chinese already). */
int ngb_zh_is_wide(const char *s);

/* UTF-8 text in the embedded font (Source Han Sans CN Bold, cut): coverage into m (w x h, max), the baseline at
 * (x, base), px as stb_truetype's pixel height of the Latin faces it stands beside, squeezed sideways by sx.
 * Returns the advance; m NULL: only measures. */
float ngb_zh_text(uint8_t *m, int w, int h, const char *s, float px, float sx, float x, float base);

/* An entry of the game's text table (its English and Japanese pointers at en_slot, jp_slot) that shows en:
 * both point at en's Chinese (a guest copy in the game's encoding) while ngb_zh_on(), else at the English. */
void ngb_zh_engine_text(uint32_t en_slot, uint32_t jp_slot, const char *en);
/* Each frame: the characters x2.1's glyph page lacks, added to it once it is loaded (and again if reloaded). */
void ngb_zh_font_tick(void);
