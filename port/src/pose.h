#ifndef POSE_H
#define POSE_H

/* The rider's body (NOTES §4.18 "8.16"): the family's .ROT files, the blend between two of
   them, and what the race makes of them each step. */
#define POSE_BONES 20

typedef struct {                /* 0xf4 bytes: 0x51d2a8 + k x 0xf4 */
    int n;
    float a[POSE_BONES][3];
} Pose;

struct BikePhys;

int pose_load(void);            /* 0x410a10: every PilotRot; 0 when the still pose (4) is missing */
void pose_free(void);
const Pose *pose_rot(int k);
void pose_mix(Pose *o, const Pose *a, const Pose *b, float t);  /* 0x45b4e0 */
void pose_restart(void);        /* 0x47bf70: the winner's salute (0x5dabd0) from its start */
void pose_start(struct BikePhys *b);    /* 0x45bb30's part */
void pose_step(struct BikePhys *b);     /* 0x467b30(b, 1) */

#endif
