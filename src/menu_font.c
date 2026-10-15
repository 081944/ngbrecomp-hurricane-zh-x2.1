/*
 * menu_font.c -- which textures hold the menus' letters, for the mouse (src/menu_mouse.c finds the
 * menus' words by the sprites they are written with).
 *
 * The menus write with one font (sub_001E86A0): each character is a sprite -- for the font at
 * 0x4529FC, the glyph number the table at 0x430C48 gives a byte -- cut from a couple of textures of
 * white letters, the ones H, a and 0 come from.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

#define GLYPH_MAP 0x430C48        /* byte -> glyph (= sprite) number */
#define MAX_ATLAS 4

uint32_t ngb_sprite_def(uint32_t id);
uint32_t ngb_font_tex[MAX_ATLAS];   /* the font's textures */
int ngb_font_tex_n;

/* Each time the menus write (the font may be loaded again meanwhile). */
void ngb_menu_font_init(void)
{
    int n = 0;
    for (int q = 0; q < 3; q++) {
        uint32_t d = ngb_sprite_def(MEM32(GLYPH_MAP + 4 * (uint8_t)"Ha0"[q]));
        if (!d)
            continue;
        uint32_t t = MEM32(d + 0x10);
        int seen = 0;
        for (int j = 0; j < n; j++)
            seen |= ngb_font_tex[j] == t;
        if (!seen && n < MAX_ATLAS)
            ngb_font_tex[n++] = t;
    }
    if (n)
        ngb_font_tex_n = n;
}
