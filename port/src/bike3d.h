#ifndef BIKE3D_H
#define BIKE3D_H
#include "pose.h"

/* The bike and its rider in 3D (NOTES §4.15): the family named by g_fam (bikes_load) */
typedef struct {                /* a rider's bike, the fields the drawer reads (0x584421 + k x 0x3209) */
    int bike;                   /* +0x78: its carac row (the measures) */
    int skin;                   /* +0x5fc, +0x170, +0x2f4, +0x478: its livery, 0..7 */
    float m[3][3], t[3];        /* +0xbc, +0xe0: the bike's matrix */
    float pos[3];               /* +0x30 */
    float rear[3], front[3];    /* +0x338, +0x1b4: the wheels' ground points */
    int sec;                    /* +0x31c1: its section */
    /* the race's parts (all 0 in the menu) */
    float lean, pitch, steer;   /* +0x2e1d, +0x2e8d, +0x110 */
    float wheelie;              /* +0x307d: the turbo's front wheel lift */
    float speed;                /* +0x124: m/s, fades the steering out */
    float dive, susp;           /* +0x2e39, +0x2e91: the frame's dive, the rear suspension */
    float spin_rear, spin_front;/* +0x2e3d, +0x2e49: the wheels' turn */
    int crashed;                /* +0x30c1 */
    float roll_side;            /* +0x30dd: the lean while crashed */
    float body[3][3], body_pt[3];   /* +0x3085, +0x30b5: the rider's frame and place; the
                                   drawer writes them while not crashed, the crash moves them */
    Pose pose;                  /* the body's angles; none: the still pose (.ROT 4) */
    int hand;                   /* +0x319d: the hand skeleton, whatever the distance */
} Rider;

/* the banks (pages 0, 1), the skeleton, the pose, wheels; rgb1: page 1 read in RGB */
int bike3d_load(int rgb1);
void bike3d_free(void);
/* page 10's camera (0x46dab0, 0x46e140): round the bike at angle theta, looking at target */
void bike3d_menu_camera(float theta, const float target[3]);
/* 0x45f290 at detail 0, page 10's way: the bike's measures from carac row `bike`, its
   livery `skin` (palette 0..7) */
void bike3d_draw(int bike, int skin);
void bike3d_place(Rider *r, const float p[3]);  /* 0x45c7c0 */
/* 0x45f290 for a rider at a detail: 0 the skeleton, 2..4 the far models; writes body and
   body_pt back while not crashed */
void bike3d_draw_rider(Rider *r, int lod);
/* the drawer's steps 2-6 alone: body and body_pt as a picture would leave them */
void bike3d_body(Rider *r);

/* the on-board view (F6): once a step while it shows, the frame's pitch (0x45ca00) into
   the cockpit's shake (0x4ea4e4); then the handlebar and cockpit, lean = the followed
   rider's (0x462110) */
void bike3d_cockpit_step(double pitch);
void bike3d_cockpit(const Rider *rd, float lean);

#endif
