#ifndef NGB_MENU_MOUSE_H
#define NGB_MENU_MOUSE_H
#include <stdint.h>

/* A piece of text a game menu drew (src/menu_mouse.c). */
typedef struct {
    uint64_t ms;                  /* when (CLOCK_MONOTONIC ms) */
    float x0, y0, x1, y1;         /* its box, in the 720x480 */
    float bright;                 /* the chosen item is drawn brighter */
    uint32_t caller, str;         /* the code that drew it; its text */
    int kind;                     /* 0 moved with the d-pad, 1 with LT/RT, 2 a
                                   * picture (a list only by the menu engine's row),
                                   * 3 a button prompt (str: the button) */
    unsigned seq;                 /* drawing order */
} NgbTextItem;

#define MOUSE_ITEMS 1024
int ngb_menu_items(NgbTextItem *out, int max);
int ngb_menu_up(void);
/* The menu engine's row on its open page, if it ran just now; else -1. */
int ngb_engine_row(void);
int ngb_engine_rows(int *first);
void ngb_menu_add_item(float x0, float y0, float x1, float y1, float bright,
                       uint32_t group, uint32_t id, int kind);

#endif
