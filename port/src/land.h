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
} LandLib;

typedef struct {                /* a decor object (class 0x4d0110) */
    float pos[3], ang[3];       /* +0x20, +0x14 */
    int lib, anim;
} Decor;

typedef struct {
    int version;
    int nsec, nblock, nlib, ndecor;
    Section *sec;
    Model *block;
    LandLib *lib;
    Decor *decor;
    int horizon_offset;         /* 0x4db2b0 */
    float horizon_scale;        /* 0x4db2b4 */
} Land;

extern Land g_land;

/* the track g_tracks[track]: Level.Spt for the file names, the .trk, pages 2..19 */
int land_load(int track, char *err, int errsize);
void land_free(void);
int land_section(const float pos[3], int hint);         /* 0x40c740 / 0x40c7e0; hint -1 */
/* 0x40ce60: the road blocks and decor a camera (C at +0x68, pos at +0x98) sees from the
   section sec, into the renderer */
void land_draw(int sec, const float c[3][3], const float pos[3]);

#endif
