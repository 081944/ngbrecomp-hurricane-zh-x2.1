#include "costume_switch.h"
#include <string.h>

void ngb_costume_pose_save(NgbCostumePose *pose, const uint8_t *actor)
{
    memcpy(pose->basis, actor, sizeof pose->basis);
    static const unsigned roots[] = {
        0x8C0, 0x8D0, 0x8E0, 0x18E0, 0x1910, 0x20A0, 0x20D0
    };
    for (unsigned i = 0; i < sizeof roots / sizeof roots[0]; ++i)
        memcpy(pose->position[i], actor + roots[i], sizeof pose->position[i]);
}

void ngb_costume_pose_restore(const NgbCostumePose *pose, uint8_t *actor)
{
    static const unsigned roots[] = {
        0x8C0, 0x8D0, 0x8E0, 0x18E0, 0x1910, 0x20A0, 0x20D0
    };
    memcpy(actor, pose->basis, sizeof pose->basis);
    for (unsigned i = 0; i < sizeof roots / sizeof roots[0]; ++i)
        memcpy(actor + roots[i], pose->position[i], sizeof pose->position[i]);
}

void ngb_costume_observe(NgbCostumeState *state, unsigned area, unsigned fallback)
{
    if (state->area != area) {
        state->valid = state->pending = 0;
        state->area = area;
    }
    if (fallback < 8 && (!state->pending || fallback != state->target)) {
        state->fallback = fallback;
        state->valid = 1;
    }
    state->pending = 0;
}

void ngb_costume_seed(NgbCostumeState *state, unsigned area, unsigned override, unsigned active)
{
    /* Initial character loading need not pass through the costume-change hook.
     * With no override, the loaded model is the area's authoritative default.
     * Never infer a default from a manually selected or still-loading model. */
    if (!state->pending && override == 0xFF && active < 8 &&
        (!state->valid || state->area != area))
        ngb_costume_observe(state, area, active);
}

int ngb_costume_ready(const NgbCostumeState *state, unsigned area, unsigned costume, unsigned trigger)
{
    if (state->pending || trigger != 0xFF || (costume > 7 && costume != 0xFF)) return 0;
    return costume != 0xFF || (state->valid && state->area == area);
}

int ngb_costume_submit(NgbCostumeState *state, unsigned area, unsigned costume, unsigned *trigger)
{
    if (!ngb_costume_ready(state, area, costume, 0xFF)) return 0;
    if (state->area != area) state->valid = 0;
    state->area = area;
    state->target = costume == 0xFF ? state->fallback : costume;
    state->pending = 1;
    *trigger = state->target;
    return 1;
}
