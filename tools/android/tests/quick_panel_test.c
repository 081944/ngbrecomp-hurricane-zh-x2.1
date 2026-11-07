#include "quick_panel.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint64_t event(const NgbQuickPanel *p, int action, int category, int item)
{
    return (uint64_t)p->epoch << 32 | (uint64_t)action << 24 | (uint64_t)category << 16 | item;
}
int main(void)
{
    assert(ngb_quick_native_slot(1)==0 && ngb_quick_native_slot(2)==1);
    assert(ngb_quick_native_slot(3)==2 && ngb_quick_native_slot(4)==7);
    assert(ngb_quick_native_slot(0)==8 && ngb_quick_native_slot(5)==8);
    NgbQuickMenu menu;
    memset(&menu, 0, sizeof menu);
    menu.ready = 1;
    menu.views[0].count = 1;
    menu.views[0].freeze = 1;
    menu.views[0].entries[0].id = 0xC9;
    menu.views[0].entries[0].usable = 1;
    menu.views[3] = menu.views[0];
    menu.views[3].entries[0].id = 0xBE;
    menu.views[4] = menu.views[0];
    menu.views[4].entries[0].id = 0xFF;
    menu.views[5] = menu.views[0];
    menu.views[5].entries[0].id = 0x50;
    NgbQuickPanel panel = {0};
    uint16_t item = 0; int category = 0;
    NgbPanelCommand command = {0};
    assert(ngb_panel_submit(&command, 0x12B, 1, 10));
    assert(!ngb_panel_take(&command, 0, 20, &item, &category));
    assert(ngb_panel_pending(&command, 20)); /* A temporarily closed gate must not consume the tap. */
    assert(!ngb_panel_submit(&command, 0xBE, 4, 30)); /* Do not overwrite a pending selection. */
    assert(ngb_panel_take(&command, 1, 40, &item, &category));
    assert(item == 0x12B && category == 1 && !ngb_panel_pending(&command, 40));
    assert(!ngb_panel_take(&command, 1, 50, &item, &category));
    assert(ngb_panel_submit(&command, 0xBE, 4, 60));
    assert(!ngb_panel_take(&command, 1, 310, &item, &category)); /* No delayed use after the deadline. */
    assert(!ngb_panel_pending(&command, 310));
    assert(ngb_panel_submit(&command, 0xFF, NGB_QUICK_COSTUMES, 400));
    assert(ngb_panel_take(&command, 1, 410, &item, &category));
    assert(item == 0xFF && category == NGB_QUICK_COSTUMES);
    ngb_panel_update(&panel, 0, 0);
    assert(panel.epoch != 0 && !panel.active);
    ngb_panel_update(&panel, 1, 100);
    assert(!ngb_panel_freeze(&panel, &menu)); /* A visible panel alone never pauses. */
    assert(!ngb_panel_event(&panel, event(&panel, PANEL_DOWN, 1, 0xC9), &menu, 100, &item, &category));
    assert(ngb_panel_freeze(&panel, &menu));
    assert(ngb_panel_event(&panel, event(&panel, PANEL_UP, 1, 0xC9), &menu, 180, &item, &category));
    assert(item == 0xC9 && category == 1 && !ngb_panel_freeze(&panel, &menu));
    assert(!ngb_panel_event(&panel, event(&panel, PANEL_UP, 1, 0xC9), &menu, 190, &item, &category));
    ngb_panel_event(&panel, event(&panel, PANEL_DOWN, 4, 0xBE), &menu, 200, &item, &category);
    ngb_panel_event(&panel, event(&panel, PANEL_CANCEL, 0, 0), &menu, 210, &item, &category);
    assert(!ngb_panel_event(&panel, event(&panel, PANEL_UP, 4, 0xBE), &menu, 220, &item, &category));
    menu.ready = 0;
    unsigned combat_epoch = panel.epoch;
    ngb_panel_update(&panel, 1, 295); /* A charge flash keeps the inventory visible, but locked. */
    assert(panel.active && panel.epoch == combat_epoch);
    assert(menu.views[0].count == 1 && !ngb_panel_freeze(&panel, &menu));
    ngb_panel_event(&panel, event(&panel, PANEL_DOWN, 4, 0xBE), &menu, 300, &item, &category);
    menu.ready = 1;
    ngb_panel_update(&panel, 1, 390);
    assert(panel.epoch == combat_epoch);
    assert(!ngb_panel_event(&panel, event(&panel, PANEL_UP, 4, 0xBE), &menu, 400, &item, &category));
    ngb_panel_event(&panel, event(&panel, PANEL_DOWN, 4, 0xBE), &menu, 500, &item, &category);
    menu.views[3].entries[0].usable = 0; /* Recheck quantities/gauges on release. */
    assert(!ngb_panel_event(&panel, event(&panel, PANEL_UP, 4, 0xBE), &menu, 600, &item, &category));
    ngb_panel_event(&panel, event(&panel, PANEL_DOWN, 1, 0xC9), &menu, 700, &item, &category);
    uint64_t stale = event(&panel, PANEL_UP, 1, 0xC9);
    ngb_panel_update(&panel, 0, 710);
    ngb_panel_update(&panel, 1, 720);
    assert(!ngb_panel_event(&panel, stale, &menu, 730, &item, &category));
    menu.views[0].freeze = 0;
    ngb_panel_event(&panel, event(&panel, PANEL_DOWN, 1, 0xC9), &menu, 800, &item, &category);
    assert(!ngb_panel_freeze(&panel, &menu));
    assert(ngb_panel_event(&panel, event(&panel, PANEL_UP, 1, 0xC9), &menu, 850, &item, &category));
    menu.views[0].freeze = 1;
    ngb_panel_event(&panel, event(&panel, PANEL_DOWN, 1, 0xC9), &menu, 900, &item, &category);
    ngb_panel_update(&panel, 1, 6001);
    assert(!ngb_panel_freeze(&panel, &menu));
    assert(!ngb_panel_event(&panel, event(&panel, PANEL_UP, 1, 0xC9), &menu, 6002, &item, &category));
    assert(!ngb_panel_event(&panel, event(&panel, PANEL_DOWN, 0, 0), &menu, 6100, &item, &category));
    ngb_panel_event(&panel, event(&panel, PANEL_DOWN, NGB_QUICK_COSTUMES, 0xFF), &menu, 6200, &item, &category);
    assert(ngb_panel_event(&panel, event(&panel, PANEL_UP, NGB_QUICK_COSTUMES, 0xFF), &menu, 6250, &item, &category));
    assert(item == 0xFF && category == NGB_QUICK_COSTUMES);
    ngb_panel_event(&panel, event(&panel, PANEL_DOWN, NGB_QUICK_ARMLETS, 0x50), &menu, 6300, &item, &category);
    assert(ngb_panel_event(&panel, event(&panel, PANEL_UP, NGB_QUICK_ARMLETS, 0x50), &menu, 6350, &item, &category));
    assert(item==0x50 && category==NGB_QUICK_ARMLETS);
    /* An outfit omitted by the unlock filter cannot be requested by a forged/stale button. */
    ngb_panel_event(&panel, event(&panel, PANEL_DOWN, NGB_QUICK_COSTUMES, 7), &menu, 6300, &item, &category);
    assert(!ngb_panel_event(&panel, event(&panel, PANEL_UP, NGB_QUICK_COSTUMES, 7), &menu, 6350, &item, &category));
    puts("quick panel guest checks passed");
    return 0;
}
