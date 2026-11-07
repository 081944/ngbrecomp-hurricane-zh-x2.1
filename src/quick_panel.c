#include "quick_panel.h"

int ngb_panel_pending(NgbPanelCommand *command, uint64_t now)
{
    if (command->pending && now >= command->until) command->pending = 0;
    return command->pending;
}

int ngb_panel_submit(NgbPanelCommand *command, uint16_t item, int category, uint64_t now)
{
    if (category < 1 || category > NGB_PANEL_CATEGORIES || ngb_panel_pending(command, now)) return 0;
    command->item = item;
    command->category = category;
    command->until = now + 250;
    command->pending = 1;
    return 1;
}

int ngb_panel_take(NgbPanelCommand *command, int ready, uint64_t now, uint16_t *item, int *category)
{
    if (!ngb_panel_pending(command, now) || !ready) return 0;
    *item = command->item;
    *category = command->category;
    command->pending = 0;
    return 1;
}

void ngb_panel_update(NgbQuickPanel *p, int active, uint64_t now)
{
    if (!p->epoch) p->epoch = 1;
    if (p->active != active) {
        p->active = active;
        p->epoch = (p->epoch + 1) & 0xFFFFFFu;
        if (!p->epoch) p->epoch = 1;
        p->held = 0;
    }
    /* A missing release (screen off, interrupted gesture) must never leave a pause held. */
    if (p->held && now - p->since > 5000) p->held = 0;
}

int ngb_panel_event(NgbQuickPanel *p, uint64_t event, const NgbQuickMenu *menu,
                    uint64_t now, uint16_t *item, int *category)
{
    unsigned epoch = (unsigned)(event >> 32);
    unsigned kind = (unsigned)(event >> 24) & 255;
    int c = (int)(event >> 16) & 255;
    uint16_t id = (uint16_t)event;
    if (epoch != p->epoch) return 0;
    if (kind == PANEL_CANCEL) { p->held = 0; return 0; }
    int same = p->held && p->category == c && p->item == id;
    if (kind == PANEL_UP || kind == PANEL_DOWN) p->held = 0;
    if (!p->active || !menu || !menu->ready || c < 1 || c > NGB_PANEL_CATEGORIES) return 0;
    const NgbQuickView *v = &menu->views[c - 1];
    const NgbQuickEntry *entry = 0;
    for (int i = 0; i < v->count; i++)
        if (v->entries[i].id == id && v->entries[i].usable) entry = &v->entries[i];
    if (!entry) return 0;
    if (kind == PANEL_DOWN) {
        p->held = 1; p->item = id; p->category = c; p->since = now;
    } else if (kind == PANEL_UP && same && now - p->since <= 5000) {
        *item = id; *category = c;
        return 1;
    }
    return 0;
}

int ngb_panel_freeze(const NgbQuickPanel *p, const NgbQuickMenu *menu)
{
    return p->active && p->held && menu && menu->ready && menu->views[p->category - 1].freeze;
}
