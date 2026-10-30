#ifndef LAND_H
#define LAND_H
#include "r3d.h"
#include <stdint.h>

/* A race track as the race draws it (build/notes/track.md): the .trk (FileLand, read by
   0x40d650), its texture pages, and which of its pieces a camera sees (0x40ce60). */

#define LAND_SECDECOR 16        /* decor objects a section names (+0xb4, at most 16) */

typedef struct {                /* 0xfc bytes at the track object +0 */
    float pos[3];               /* +0x00 */
    float axes[3][3];           /* +0x0c: X across, Y up, Z along the road */
    int block;                  /* +0x3c: its road model */
    float plane[4][4];          /* +0x40: the start plane first (n, d) */
    float line;                 /* +0xb0: the racing line, across from pos */
    int ndecor;                 /* +0xd4 */
    uint16_t decor[LAND_SECDECOR];      /* +0xb4 */
    float edge[2];              /* +0xf0, +0xf4: the road's sides, across */
    int checkpoint, time;       /* +0xd8, +0xda */
    int event, ground_off, ground_on, flag;     /* +0xec, +0xe8, +0xe4, +0xe0 */
} Section;

typedef struct {                /* a library entry: 0x4dc0d8/dc/e0 */
    char name[64];
    Model m[3];                 /* near, medium, far */
    float (*v0)[3];             /* the near model's own points (+0x1c); its +4 is what an
                                   animation of kind 7 shows */
} LandLib;

typedef struct {                /* a decor object (class 0x4d0110) */
    float pos[3], ang[3];       /* +0x20, +0x14 */
    int lib, anim;
    int camera;                 /* its name is CAMERA.. or camera.. (0x40d4f0) */
} Decor;

typedef struct {                /* a moving piece of decor: 0x190 bytes at 0x5f6780 */
    int type, sub;              /* +0, +4: 1..5 an angle or a place back and forth or round
                                   and round, 7 the model's points, 8 a place, then a wait */
    int decor;                  /* +8 */
    float t;                    /* +0x1c: its clock, in the step's units */
    float a, b, c;              /* +0x20 .. +0x28 */
    float period;               /* +0x2c */
    float from[3], to[3];       /* +0x30, +0x3c */
    int nframes, npoints;       /* +0x48; kind 7's */
    float (**frame)[3];         /* +0x50: nframes of npoints */
} DecorAnim;

typedef struct {
    int version;
    int nanim;
    DecorAnim *anim;            /* 0x5f6778: the .trk's "AnimDecor" block */
    int nsec, nblock, nlib, ndecor;
    int ncam;                   /* 0x51a40c: trackside cameras (0x473240) */
    Section *sec;
    Model *block;
    LandLib *lib;
    Decor *decor;
    int horizon_offset;         /* 0x4db2b0 */
    float horizon_scale;        /* 0x4db2b4 */
    int bank1_rgb;              /* bank 1's number & 4 (0x51d16c): the bikes' page 1 in RGB */
    /* Level.Spt after the banks (0x427e80, NOTES §4.13.1.3) */
    int step_global, yscale;    /* 0x4dbfe0, 0x516aa8 */
    int start[3];               /* 0x51bfd8: StartPosition */
    int optimise;               /* 0x5d2eac: ComputeLandOptimise */
    float detail[9][5];         /* 0x51d1b8: LenBPlane .. gouraudZ, a column per detail */
    float ai[13][3];            /* 0x5427b8..: RATTRAPAGE_SPEED .. SPEED_FACTOR */
    int ncomputer;              /* 0x4dbfcc: NbComputer */
    float computer_speed[32];   /* 0x51be98: ComputerSpeed, NbComputer of them */
    int time_skill[3];          /* 0x515fe0: TimeSkill, ms by difficulty */
    int ncheck;                 /* 0x4dbfd0: NbCheckPoint */
    int check_time[32];         /* 0x509228: TimeCheckPoint, ms */
    float factor_pocket;        /* 0x4dbfd8 */
    int audio_track;            /* 0x4db67c: the race's music (0: a random one) */
} Land;

extern Land g_land;

/* the track g_tracks[track]: Level.Spt for the file names, the .trk, pages 2..19 */
int land_load(int track, char *err, int errsize);
void land_free(void);
int land_section(const float pos[3], int hint);         /* 0x40c740 / 0x40c7e0; hint -1 */
int land_walk(const float pos[3], int hint, float *dist);       /* 0x40c7e0 */
/* 0x40ce60: the road blocks and decor a camera (C at +0x68, pos at +0x98) sees from the
   section sec, into the renderer */
int land_section_seen(int sec, const float cm[3][3], int other);
void land_draw(int sec, const float c[3][3], const float pos[3]);
/* 0x48b7a0: the moving decor, the step's dt (ms x 0.1) on */
void land_anim_step(float dt);

#endif
