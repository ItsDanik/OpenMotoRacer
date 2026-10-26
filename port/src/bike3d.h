#ifndef BIKE3D_H
#define BIKE3D_H

/* The bike and its rider in 3D (NOTES §4.15): the family named by g_fam (bikes_load) */
typedef struct {                /* a rider's bike, the fields the drawer reads (0x584421 + k x 0x3209) */
    int bike;                   /* +0x78: its carac row (the measures) */
    int skin;                   /* +0x5fc, +0x170, +0x2f4, +0x478: its livery, 0..7 */
    float m[3][3], t[3];        /* +0xbc, +0xe0: the bike's matrix */
    float pos[3];               /* +0x30 */
    float rear[3], front[3];    /* +0x338, +0x1b4: the wheels' ground points */
    int sec;                    /* +0x31c1: its section */
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
/* 0x45f290 for a rider at a detail: 0 the skeleton, 2..4 the far models */
void bike3d_draw_rider(const Rider *r, int lod);

#endif
