#pragma once
#include <stdint.h>

/* The script's default is distinct from a user-generated one-shot request. */
typedef struct {
    unsigned area, fallback, valid, pending, target;
} NgbCostumeState;

void ngb_costume_observe(NgbCostumeState *state, unsigned area, unsigned fallback);
void ngb_costume_seed(NgbCostumeState *state, unsigned area, unsigned override, unsigned active);
int ngb_costume_ready(const NgbCostumeState *state, unsigned area, unsigned costume, unsigned trigger);
int ngb_costume_submit(NgbCostumeState *state, unsigned area, unsigned costume, unsigned *trigger);
int ngb_costume_can_request(uint16_t costume);
int ngb_costume_request(uint16_t costume);
void ngb_costume_frame_tick(void);

/* World transforms contain no resource pointers. Bone and animation storage is
 * deliberately left to the native loader. */
typedef struct {
    uint8_t basis[64];
    /* Current root plus the motion history samples consumed after a reload. */
    uint8_t position[7][16];
} NgbCostumePose;
void ngb_costume_pose_save(NgbCostumePose *pose, const uint8_t *actor);
void ngb_costume_pose_restore(const NgbCostumePose *pose, uint8_t *actor);
