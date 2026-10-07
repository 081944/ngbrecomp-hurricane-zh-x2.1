/*
 * launcher.c -- the settings screen shown before the game starts.
 *
 * Its own small SDL window with an SDL_Renderer (the game's window is a
 * Vulkan one, created afterwards), driven by keyboard, mouse or controller
 * so it works the same at a desk and on a Steam Deck. Text comes from a
 * bitmap font baked into launcher_font.h (tools/font/gen_font.py).
 *
 * The choices live in $XDG_CONFIG_HOME/NinjaGaidenBlack/settings.ini; the
 * environment variables of the same meaning (NGB_WIDESCREEN, NGB_FULLSCREEN,
 * NGB_VSYNC, NV2A_SCALE) still override them, for testing.
 */
#include <SDL.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "launcher.h"
#include "launcher_font.h"

/* ---------- settings file ---------- */

static void config_path(char *out, size_t n, int make_dir)
{
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    char dir[1024];
    if (xdg && *xdg)
        snprintf(dir, sizeof dir, "%s/NinjaGaidenBlack", xdg);
    else
        snprintf(dir, sizeof dir, "%s/.config/NinjaGaidenBlack", home ? home : ".");
    if (make_dir) {
        char parent[1024];
        snprintf(parent, sizeof parent, "%s", dir);
        char *slash = strrchr(parent, '/');
        if (slash) { *slash = 0; mkdir(parent, 0755); }
        mkdir(dir, 0755);
    }
    snprintf(out, n, "%s/settings.ini", dir);
}

static const char *const ASPECT_NAMES[ASPECT_COUNT] = { "4:3", "16:9", "16:10", "stretch" };

void ngb_settings_load(NgbSettings *s)
{
    char path[1100], line[256];
    FILE *f;

    s->aspect = ASPECT_16_9;
    s->width = 1280;
    s->height = 720;
    s->fullscreen = 0;
    s->scale = 1;
    s->vsync = 0;
    s->skip_intro = 0;
    s->show_launcher = 1;

    config_path(path, sizeof path, 0);
    f = fopen(path, "r");
    if (!f)
        return;
    while (fgets(line, sizeof line, f)) {
        char key[64], val[128];
        if (sscanf(line, " %63[^= ] = %127[^\r\n]", key, val) != 2)
            continue;
        if (!strcmp(key, "aspect")) {
            for (int i = 0; i < ASPECT_COUNT; i++)
                if (!strcmp(val, ASPECT_NAMES[i]))
                    s->aspect = i;
        } else if (!strcmp(key, "window")) {
            int w, h;
            if (sscanf(val, "%dx%d", &w, &h) == 2 && w >= 320 && h >= 200) {
                s->width = w;
                s->height = h;
            }
        } else if (!strcmp(key, "fullscreen")) {
            s->fullscreen = atoi(val) != 0;
        } else if (!strcmp(key, "render_scale")) {
            int v = atoi(val);
            s->scale = v < 1 ? 1 : v > 4 ? 4 : v;
        } else if (!strcmp(key, "vsync")) {
            s->vsync = atoi(val) != 0;
        } else if (!strcmp(key, "skip_intro")) {
            s->skip_intro = atoi(val) != 0;
        } else if (!strcmp(key, "show_settings_at_startup")) {
            s->show_launcher = atoi(val) != 0;
        }
    }
    fclose(f);
}

void ngb_settings_save(const NgbSettings *s)
{
    char path[1100];
    FILE *f;

    config_path(path, sizeof path, 1);
    f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "settings: cannot write %s: %s\n", path, strerror(errno));
        return;
    }
    fprintf(f, "# Ninja Gaiden Black settings (the settings screen writes this file)\n");
    fprintf(f, "aspect = %s\n", ASPECT_NAMES[s->aspect]);
    fprintf(f, "window = %dx%d\n", s->width, s->height);
    fprintf(f, "fullscreen = %d\n", s->fullscreen);
    fprintf(f, "render_scale = %d\n", s->scale);
    fprintf(f, "vsync = %d\n", s->vsync);
    fprintf(f, "skip_intro = %d\n", s->skip_intro);
    fprintf(f, "show_settings_at_startup = %d\n", s->show_launcher);
    fclose(f);
}

double ngb_settings_aspect_ratio(const NgbSettings *s)
{
    switch (s->aspect) {
    case ASPECT_4_3:   return 4.0 / 3.0;
    case ASPECT_16_10: return 16.0 / 10.0;
    case ASPECT_STRETCH: return -1.0;
    default:           return 16.0 / 9.0;
    }
}

/* ---------- drawing ---------- */

#define LW 960            /* logical size of the screen */
#define LH 600

static SDL_Texture *s_font;
static int s_glyph_x[95];

static int font_init(SDL_Renderer *r)
{
    int total = 0;
    for (int i = 0; i < 95; i++) {
        s_glyph_x[i] = total;
        total += launcher_font_w[i];
    }
    uint32_t *px = calloc((size_t)total * LAUNCHER_FONT_H_PX, 4);
    if (!px)
        return -1;
    for (int i = 0; i < 95; i++) {
        int w = launcher_font_w[i];
        const uint8_t *g = launcher_font_px + launcher_font_off[i];
        for (int y = 0; y < LAUNCHER_FONT_H_PX; y++)
            for (int x = 0; x < w; x++)
                px[y * total + s_glyph_x[i] + x] =
                    ((uint32_t)g[y * w + x] << 24) | 0x00FFFFFFu;
    }
    s_font = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STATIC, total, LAUNCHER_FONT_H_PX);
    if (!s_font) {
        free(px);
        return -1;
    }
    SDL_UpdateTexture(s_font, NULL, px, total * 4);
    SDL_SetTextureBlendMode(s_font, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(s_font, SDL_ScaleModeLinear);
    free(px);
    return 0;
}

static int text_width(const char *t, float size)
{
    float w = 0;
    for (; *t; t++) {
        int c = (unsigned char)*t;
        if (c < 32 || c > 126) c = '?';
        w += launcher_font_w[c - 32] * size / LAUNCHER_FONT_H_PX;
    }
    return (int)w;
}

/* align: 0 left, 1 centre, 2 right of x */
static void draw_text(SDL_Renderer *r, const char *t, int x, int y, float size,
                      SDL_Color col, int align)
{
    float fx = (float)x;
    if (align == 1) fx -= text_width(t, size) / 2.0f;
    if (align == 2) fx -= (float)text_width(t, size);
    SDL_SetTextureColorMod(s_font, col.r, col.g, col.b);
    SDL_SetTextureAlphaMod(s_font, col.a);
    for (; *t; t++) {
        int c = (unsigned char)*t;
        if (c < 32 || c > 126) c = '?';
        int w = launcher_font_w[c - 32];
        SDL_Rect src = { s_glyph_x[c - 32], 0, w, LAUNCHER_FONT_H_PX };
        SDL_FRect dst = { fx, (float)y, w * size / LAUNCHER_FONT_H_PX, size };
        SDL_RenderCopyF(r, s_font, &src, &dst);
        fx += dst.w;
    }
}

static void fill(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c)
{
    SDL_Rect rc = { x, y, w, h };
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, &rc);
}

/* ---------- the screen ---------- */

static const int SIZES[][2] = {
    { 960, 720 }, { 1024, 768 }, { 1280, 720 }, { 1280, 800 }, { 1280, 960 },
    { 1440, 1080 }, { 1600, 900 }, { 1920, 1080 }, { 1920, 1200 },
    { 2560, 1440 }, { 2560, 1600 }, { 3840, 2160 },
};
#define NSIZES (int)(sizeof SIZES / sizeof SIZES[0])

enum { ROW_ASPECT, ROW_SIZE, ROW_DISPLAY, ROW_SCALE, ROW_VSYNC, ROW_SKIP,
       ROW_SHOW, ROW_START, ROW_QUIT, NROWS };

static int size_index(const NgbSettings *s)
{
    for (int i = 0; i < NSIZES; i++)
        if (SIZES[i][0] == s->width && SIZES[i][1] == s->height)
            return i;
    return -1;
}

static void change(NgbSettings *s, int row, int dir)
{
    switch (row) {
    case ROW_ASPECT:
        s->aspect = (s->aspect + dir + ASPECT_COUNT) % ASPECT_COUNT;
        break;
    case ROW_SIZE: {
        int i = size_index(s);
        if (i < 0) i = dir > 0 ? -1 : NSIZES;
        i = (i + dir + NSIZES) % NSIZES;
        s->width = SIZES[i][0];
        s->height = SIZES[i][1];
        break;
    }
    case ROW_DISPLAY: s->fullscreen = !s->fullscreen; break;
    case ROW_SCALE:   s->scale = (s->scale - 1 + dir + 4) % 4 + 1; break;
    case ROW_VSYNC:   s->vsync = !s->vsync; break;
    case ROW_SKIP:    s->skip_intro = !s->skip_intro; break;
    case ROW_SHOW:    s->show_launcher = !s->show_launcher; break;
    default: break;
    }
}

static void row_text(const NgbSettings *s, int row, const char **label, char *val, size_t n)
{
    static const char *const ASPECT_LABEL[ASPECT_COUNT] = {
        "4:3 (original)", "16:9 widescreen", "16:10 (Steam Deck)",
        "Stretch to window" };
    static const char *const SCALE_LABEL[4] = {
        "Native (1x)", "2x", "3x", "4x" };
    val[0] = 0;
    switch (row) {
    case ROW_ASPECT:  *label = "Aspect ratio";  snprintf(val, n, "%s", ASPECT_LABEL[s->aspect]); break;
    case ROW_SIZE:    *label = "Window size";   snprintf(val, n, "%d x %d", s->width, s->height); break;
    case ROW_DISPLAY: *label = "Display";       snprintf(val, n, "%s", s->fullscreen ? "Fullscreen" : "Windowed"); break;
    case ROW_SCALE:   *label = "Render resolution"; snprintf(val, n, "%s", SCALE_LABEL[s->scale - 1]); break;
    case ROW_VSYNC:   *label = "V-Sync";        snprintf(val, n, "%s", s->vsync ? "On" : "Off"); break;
    case ROW_SKIP:    *label = "Skip intro movie"; snprintf(val, n, "%s", s->skip_intro ? "Yes" : "No"); break;
    case ROW_SHOW:    *label = "Show this screen at startup"; snprintf(val, n, "%s", s->show_launcher ? "Yes" : "No"); break;
    case ROW_START:   *label = "Start game"; break;
    case ROW_QUIT:    *label = "Quit"; break;
    }
}

#define ROW_Y0 128
#define ROW_H  44

static int row_at(int y)
{
    if (y < ROW_Y0)
        return -1;
    int r = (y - ROW_Y0) / ROW_H;
    if (r >= ROW_START)            /* a gap before the buttons */
        r = (y - ROW_Y0 - 16) / ROW_H;
    return r >= 0 && r < NROWS ? r : -1;
}

static int row_y(int row)
{
    return ROW_Y0 + row * ROW_H + (row >= ROW_START ? 16 : 0);
}

static void draw(SDL_Renderer *r, const NgbSettings *s, int sel)
{
    const SDL_Color white = { 235, 235, 235, 255 }, grey = { 150, 150, 155, 255 },
                    red = { 200, 32, 40, 255 }, bar = { 120, 16, 22, 200 },
                    dim = { 255, 255, 255, 18 };

    SDL_SetRenderDrawColor(r, 14, 14, 18, 255);
    SDL_RenderClear(r);
    fill(r, 0, 0, LW, 96, (SDL_Color){ 26, 8, 10, 255 });
    fill(r, 0, 94, LW, 2, red);
    draw_text(r, "NINJA GAIDEN BLACK", LW / 2, 18, 44, white, 1);
    draw_text(r, "Settings", LW / 2, 64, 22, grey, 1);

    for (int row = 0; row < NROWS; row++) {
        const char *label;
        char val[96];
        int y = row_y(row);
        row_text(s, row, &label, val, sizeof val);
        if (row >= ROW_START) {
            int w = 260, x = LW / 2 - w / 2;
            fill(r, x, y + 2, w, ROW_H - 6, row == sel ? bar : dim);
            draw_text(r, label, LW / 2, y + 8, 26, row == sel ? white : grey, 1);
            continue;
        }
        if (row == sel)
            fill(r, 60, y + 2, LW - 120, ROW_H - 6, bar);
        else if (row % 2)
            fill(r, 60, y + 2, LW - 120, ROW_H - 6, dim);
        draw_text(r, label, 84, y + 9, 24, row == sel ? white : grey, 0);
        draw_text(r, "<", 560, y + 9, 24, row == sel ? white : grey, 0);
        draw_text(r, val, 736, y + 9, 24, white, 1);
        draw_text(r, ">", LW - 92, y + 9, 24, row == sel ? white : grey, 0);
    }
    draw_text(r, "Up/Down: choose    Left/Right: change    Enter / A: select    Start: play    Esc / B: quit",
              LW / 2, LH - 58, 17, grey, 1);
    draw_text(r, "In game: F11 or Alt+Enter toggles fullscreen.  Run with --settings to see this screen again.",
              LW / 2, LH - 34, 15, grey, 1);
    SDL_RenderPresent(r);
}

int ngb_launcher_run(NgbSettings *s)
{
    SDL_Window *win;
    SDL_Renderer *r;
    SDL_GameController *pad = NULL;
    int sel = ROW_START, result = -1;
    Uint32 repeat_at = 0;
    int held_dir = 0;            /* stick/d-pad auto-repeat: -2 up, 2 down, -1/1 */

    if (!getenv("SDL_VIDEODRIVER"))
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "settings screen: %s\n", SDL_GetError());
        return 1;                /* no screen: just start */
    }
    win = SDL_CreateWindow("Ninja Gaiden Black", SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED, LW, LH,
                           SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    r = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED |
                                          SDL_RENDERER_PRESENTVSYNC) : NULL;
    if (r == NULL && win)
        r = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!win || !r || font_init(r) != 0) {
        fprintf(stderr, "settings screen unavailable: %s\n", SDL_GetError());
        if (r) SDL_DestroyRenderer(r);
        if (win) SDL_DestroyWindow(win);
        return 1;
    }
    SDL_RenderSetLogicalSize(r, LW, LH);
    Uint32 t_open = SDL_GetTicks();
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < SDL_NumJoysticks() && !pad; i++)
        if (SDL_IsGameController(i))
            pad = SDL_GameControllerOpen(i);

    while (result < 0) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            int act = 0, dir = 0, move = 0;
            switch (ev.type) {
            case SDL_QUIT:
                result = 0;
                break;
            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_UP:    case SDLK_w: move = -1; break;
                case SDLK_DOWN:  case SDLK_s: move = 1; break;
                case SDLK_LEFT:  case SDLK_a: dir = -1; break;
                case SDLK_RIGHT: case SDLK_d: dir = 1; break;
                case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE: act = 1; break;
                case SDLK_ESCAPE: result = 0; break;
                default: break;
                }
                break;
            case SDL_CONTROLLERDEVICEADDED:
                if (!pad)
                    pad = SDL_GameControllerOpen(ev.cdevice.which);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                switch (ev.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:    move = -1; held_dir = -2; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  move = 1;  held_dir = 2;  break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  dir = -1;  held_dir = -1; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: dir = 1;   held_dir = 1;  break;
                case SDL_CONTROLLER_BUTTON_A: act = 1; break;
                case SDL_CONTROLLER_BUTTON_START: sel = ROW_START; act = 1; break;
                case SDL_CONTROLLER_BUTTON_B: result = 0; break;
                default: break;
                }
                repeat_at = SDL_GetTicks() + 400;
                break;
            case SDL_CONTROLLERBUTTONUP:
                held_dir = 0;
                break;
            case SDL_CONTROLLERAXISMOTION:
                if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY ||
                    ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                    int v = ev.caxis.value, y = ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY;
                    int nd = v > 20000 ? (y ? 2 : 1) : v < -20000 ? (y ? -2 : -1) : 0;
                    if (nd && nd != held_dir) {
                        if (y) move = nd / 2; else dir = nd;
                        repeat_at = SDL_GetTicks() + 400;
                    }
                    if (nd || (y ? (held_dir == 2 || held_dir == -2)
                                 : (held_dir == 1 || held_dir == -1)))
                        held_dir = nd;
                }
                break;
            case SDL_MOUSEMOTION: {
                /* A pointer already resting over the window reports a
                 * motion at once; only a real move should select. */
                if (SDL_GetTicks() - t_open < 500)
                    break;
                int row = row_at(ev.motion.y);
                if (row >= 0) sel = row;
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                int row = row_at(ev.button.y);
                if (row >= 0) {
                    sel = row;
                    if (row >= ROW_START) act = 1;
                    else dir = (ev.button.button == SDL_BUTTON_RIGHT ||
                                ev.button.x < 560 + 40) ? -1 : 1;
                }
                break;
            }
            default:
                break;
            }
            if (move)
                sel = (sel + move + NROWS) % NROWS;
            if (dir && sel < ROW_START)
                change(s, sel, dir);
            if (act) {
                if (sel == ROW_START) result = 1;
                else if (sel == ROW_QUIT) result = 0;
                else change(s, sel, 1);
            }
        }
        /* Held d-pad or stick: repeat after a pause. */
        if (held_dir && SDL_GetTicks() >= repeat_at) {
            if (held_dir == 2 || held_dir == -2)
                sel = (sel + held_dir / 2 + NROWS) % NROWS;
            else if (sel < ROW_START)
                change(s, sel, held_dir);
            repeat_at = SDL_GetTicks() + 140;
        }
        draw(r, s, sel);
    }

    if (pad)
        SDL_GameControllerClose(pad);
    SDL_DestroyTexture(s_font);
    s_font = NULL;
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    if (result == 1)
        ngb_settings_save(s);
    return result;
}
