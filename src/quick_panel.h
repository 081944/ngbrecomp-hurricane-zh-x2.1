/* Touch requests are checked on the guest thread against its current inventory. */
#pragma once
#include "quick_select.h"

enum { PANEL_DOWN = 1, PANEL_UP, PANEL_CANCEL };
typedef struct {
    unsigned epoch;
    int active, held, category;
    uint16_t item;
    uint64_t since;
} NgbQuickPanel;

typedef struct {
    int pending, category;
    uint16_t item;
    uint64_t until;
} NgbPanelCommand;

int ngb_panel_submit(NgbPanelCommand *command, uint16_t item, int category, uint64_t now);
int ngb_panel_pending(NgbPanelCommand *command, uint64_t now);
int ngb_panel_take(NgbPanelCommand *command, int ready, uint64_t now, uint16_t *item, int *category);

void ngb_panel_update(NgbQuickPanel *panel, int active, uint64_t now);
int ngb_panel_event(NgbQuickPanel *panel, uint64_t event, const NgbQuickMenu *menu,
                    uint64_t now, uint16_t *item, int *category);
int ngb_panel_freeze(const NgbQuickPanel *panel, const NgbQuickMenu *menu);

#ifdef __ANDROID__
/* Return 1 while touch selection owns the optional pause, 2 for one released button. */
int ngb_android_quick_update(const NgbQuickMenu *menu, int enabled, uint16_t *item, int *category);
#endif
