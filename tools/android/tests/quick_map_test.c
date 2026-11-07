#include "quick_select.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t memory[0xE00000];
#define MEM8(a) memory[(a)]
#define MEM16(a) (*(uint16_t *)(memory + (a)))
#define MEM32(a) (*(uint32_t *)(memory + (a)))
#define MEMF(a) (*(float *)(memory + (a)))
static char s_game_dir[4096];
static int xbox_mod_file(const char *rel, char *out, size_t size)
{ (void)rel; (void)out; (void)size; return 0; }
#include "quick_map_under_test.h"

int main(int argc, char **argv)
{
    NgbQuickMenu menu = {0};
    MEM16(0xDB932C) = 0xFFFF;
    make_map(&menu);
    assert(menu.map_id == -1 && menu.map_asset == -1);
    MEM32(0x44D430) = 0x1000;
    MEM32(0x1000) = 0x2000;
    MEM32(0x2008) = 0x3000; MEM32(0x200C) = 2;
    memcpy(memory + 0x3006, "bgm", 4);
    memcpy(memory + 0x3016, "spr", 4);
    MEM32(0x3010) = 0x5000; MEM16(0x3014) = 212;
    for(unsigned i=0;i<212;++i) MEM16(0x5000+i*2) = 0xFFFF;
    for(unsigned row=0;row<52;++row) MEM16(0x5000+(160+row)*2) = 0x600+row;
    for (unsigned language = 0; language < 2; ++language) {
        MEM32(0xDB9890) = language;
        unsigned table = language ? 0x43A5B8 : 0x43A838;
        for (unsigned row = 0; row < 52; ++row) {
            /* Deliberately unordered, like the retail alias rows. */
            MEM32(table + row * 12) = (row + 13) % 52;
            MEM16(table + row * 12 + 8) = 0x600 + row;
            MEM16(table + row * 12 + 10) = 0xD9; /* A text label, not an archive index. */
        }
        for (unsigned id = 0; id < 52; ++id) {
            MEM16(0xDB932C) = id;
            make_map(&menu);
            assert(menu.map_id == (int)id);
            assert(menu.map_asset == (int)(160 + (id + 39) % 52));
        }
    }
    /* Region 3's Japanese file is 071D (satoallj.spr). Its text label 217
     * incorrectly selected zpkikanj.spr before this archive lookup existed. */
    unsigned current_row = 0x43A838 + 42 * 12;
    MEM32(0xDB9890) = 0;
    MEM16(0xDB932C) = 3;
    MEM16(current_row + 8) = 0x71D;
    MEM16(current_row + 10) = 217;
    MEM16(0x3014) = 213; MEM16(0x5000 + 212 * 2) = 0x71D;
    make_map(&menu);
    assert(menu.map_id == 3 && menu.map_asset == 212);
    MEM16(current_row + 8) = 0x600 + 42;
    MEM16(0x3014) = 212;
    /* Missing archive membership disables the button instead of guessing an image. */
    MEM16(0xDB932C) = 3;
    MEM16(0x3014) = 1;
    make_map(&menu);
    assert(menu.map_id == -1 && menu.map_asset == -1);
    MEM16(0x3014) = 212;
    /* No inventory entries or map ownership flags were populated. */
    MEM16(0xDB932C) = 3;
    MEM32(0x439EC0 + 12) = 0x438B40;
    MEMF(0x438B40 + 8) = 256; MEMF(0x438B40 + 12) = 256;
    MEMF(0x438B40 + 16) = 2; MEMF(0x438B40 + 20) = 2;
    MEM16(0x438B40 + 28) = 512; MEM16(0x438B40 + 30) = 512;
    MEMF(0x4B4C00) = MEMF(0x4B4C00 + 40) = 1;
    MEMF(0x4B5710) = 64; MEMF(0x4B5718) = -64;
    make_map(&menu);
    assert(menu.map_position && menu.map_x == .75f && menu.map_y == .25f);
    MEM16(0xDB932C) = 0xFFFF;
    make_map(&menu);
    assert(menu.map_id == -1 && !menu.map_position);
    MEM16(0xDB932C) = 3;
    make_map(&menu);
    assert(menu.map_id == 3 && menu.map_position);
    MEM16(0xDB932C) = 0x1C; MEM16(0xDB9AC2) = 9;
    MEM32(0x439EC0 + 0x32 * 4) = 0x438B40;
    make_map(&menu);
    assert(menu.map_id == 0x1C && menu.map_position && menu.map_x == .75f);
    MEM16(0xDB932C) = 0x2E; MEM16(0xDB9AC2) = 0x1A;
    MEM32(0x439EC0 + 0x33 * 4) = 0x438B40;
    make_map(&menu);
    assert(menu.map_id == 0x2E && menu.map_position && menu.map_y == .25f);
    if (argc > 1) {
        snprintf(s_game_dir, sizeof s_game_dir, "%s", argv[1]);
        /* Every native map sheet, including aliases and localized variants. */
        for (unsigned i = 159; i <= 258; ++i) {
            char name[24];
            assert(ngb_quick_asset_name(i, name));
            Icon *art = load_icon(name);
            assert(art && art->px && art->w > 0 && art->h > 0);
        }
        char name[24];
        assert(ngb_quick_asset_name(212, name) && !strcmp(name,"satoallj.spr"));
        assert(!ngb_quick_asset_name(4096, name) && !*name);
        printf("100 actual map/localized sheets decoded successfully\n");
    }
    puts("map regions, unordered aliases, ownership independence and marker checks passed");
    return 0;
}
