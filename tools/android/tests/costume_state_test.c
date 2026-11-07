#include "costume_switch.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void pose_checks(void)
{
    uint8_t actor[0x2800], before[0x2800];
    memset(actor, 0xA5, sizeof actor);
    static const unsigned roots[] = {0x8C0,0x8D0,0x8E0,0x18E0,0x1910,0x20A0,0x20D0};
    float position[4] = { 123.25f, -17.5f, 6.0f, 1.0f };
    for (unsigned i = 0; i < sizeof roots / sizeof roots[0]; ++i) {
        float sample[4] = { position[0] + i, position[1] - i, position[2] + 2 * i, 1.0f };
        memcpy(actor + roots[i], sample, sizeof sample);
    }
    NgbCostumePose pose;
    ngb_costume_pose_save(&pose, actor);
    /* Model loading may replace pointers and disturb world/root histories. */
    memset(actor, 0x3C, sizeof actor);
    memcpy(before, actor, sizeof before);
    ngb_costume_pose_restore(&pose, actor);
    for (unsigned i = 0; i < sizeof roots / sizeof roots[0]; ++i) {
        float sample[4] = { position[0] + i, position[1] - i, position[2] + 2 * i, 1.0f };
        assert(!memcmp(actor + roots[i], sample, sizeof sample));
        memcpy(before + roots[i], sample, sizeof sample);
    }
    memcpy(before, pose.basis, sizeof pose.basis);
    /* Every other byte, including fresh resources, velocities, health and
     * action state, must remain the loader's value, not a stale snapshot. */
    assert(!memcmp(before, actor, sizeof actor));
}

int main(void)
{
    pose_checks();
    NgbCostumeState state = {0};
    unsigned trigger = 99;
    /* FF is the normal idle value after the native world task consumes a request. */
    assert(ngb_costume_ready(&state, 10, 4, 0xFF));
    assert(!ngb_costume_ready(&state, 10, 0xFF, 0xFF));
    ngb_costume_observe(&state, 10, 1);
    assert(ngb_costume_ready(&state, 10, 0xFF, 0xFF));
    assert(!ngb_costume_ready(&state, 10, 4, 2)); /* Do not overwrite a script's pending change. */
    assert(ngb_costume_submit(&state, 10, 4, &trigger) && trigger == 4);
    assert(!ngb_costume_submit(&state, 10, 5, &trigger));
    ngb_costume_observe(&state, 10, 4); /* Our request must not replace the script default. */
    assert(state.valid && state.fallback == 1 && !state.pending);
    assert(ngb_costume_submit(&state, 10, 0xFF, &trigger) && trigger == 1);
    ngb_costume_observe(&state, 10, 1);
    assert(state.fallback == 1);
    assert(!ngb_costume_ready(&state, 11, 0xFF, 0xFF));
    ngb_costume_observe(&state, 11, 3);
    assert(ngb_costume_submit(&state, 11, 0xFF, &trigger) && trigger == 3);
    ngb_costume_observe(&state, 11, 3);
    for (unsigned id = 0; id < 8; id++) {
        assert(ngb_costume_submit(&state, 11, id, &trigger) && trigger == id);
        ngb_costume_observe(&state, 11, id);
        assert(state.fallback == 3);
    }
    assert(!ngb_costume_submit(&state, 11, 8, &trigger));
    assert(!ngb_costume_submit(&state, 11, 256, &trigger));
    /* Saved-game startup can load a model without observing any script call. */
    NgbCostumeState startup = {0};
    ngb_costume_seed(&startup, 0xFFFF, 0xFF, 1);
    assert(startup.valid && startup.fallback == 1);
    assert(ngb_costume_submit(&startup, 0xFFFF, 4, &trigger));
    ngb_costume_seed(&startup, 0xFFFF, 0xFF, 4); /* In-flight model isn't a baseline. */
    assert(startup.pending && startup.fallback == 1);
    ngb_costume_observe(&startup, 0xFFFF, 4);
    ngb_costume_seed(&startup, 0xFFFF, 4, 4);
    assert(ngb_costume_submit(&startup, 0xFFFF, 0xFF, &trigger) && trigger == 1);
    ngb_costume_observe(&startup, 0xFFFF, 1);
    assert(startup.valid && startup.fallback == 1);
    ngb_costume_seed(&startup, 13, 4, 4); /* Never guess from a manual override. */
    assert(!ngb_costume_ready(&startup, 13, 0xFF, 0xFF));
    ngb_costume_seed(&startup, 13, 0xFF, 0xFF); /* Model not loaded yet. */
    assert(!ngb_costume_ready(&startup, 13, 0xFF, 0xFF));
    ngb_costume_seed(&startup, 13, 0xFF, 2);
    assert(ngb_costume_ready(&startup, 13, 0xFF, 0xFF) && startup.fallback == 2);
    /* An area transition supersedes an old user request and establishes a new script default. */
    assert(ngb_costume_submit(&state, 11, 6, &trigger));
    ngb_costume_observe(&state, 12, 0);
    assert(state.valid && state.fallback == 0 && !state.pending);
    puts("costume one-shot and default restoration checks passed");
    return 0;
}
