#ifndef OBJ3D_H
#define OBJ3D_H
#include "r3d.h"

/* The menu's 3D object (0x94 bytes, NOTES §4.11.3): a model at angles and a place, each
   able to move by itself over a time, timed by the game's clock. */
enum { OBJ_LINEAR = 0, OBJ_EASE = 1, OBJ_SPIN = 2 };

typedef struct {
    float ang[3];               /* +0x00 */
    float pos[3];               /* +0x0c */
    const Model *model;         /* +0x18 */
    int moving, turning;        /* +0x20, +0x24 */
    int restart;                /* +0x28: a spin's clock starts again at the next update */
    float a0[3], ad[3];         /* +0x2c, +0x40: the turn's start and change */
    unsigned aend, adur, astart;    /* +0x4c, +0x54, +0x58 */
    int amode;                  /* +0x5c */
    float p0[3], pd[3];         /* +0x60, +0x6c */
    unsigned pend, pdur, pstart;    /* +0x78, +0x80, +0x84 */
    int pmode;                  /* +0x88 */
} Obj3d;

void obj_set(Obj3d *o, const float ang[3], const float pos[3]);    /* 0x47a340 */
void obj_set_angles(Obj3d *o, const float ang[3]);                 /* 0x47a380 */
void obj_set_pos(Obj3d *o, const float pos[3]);                    /* 0x47a3a0 */
/* 0x47a820, 0x47a8b0: to a target in `seconds`; OBJ_SPIN takes a change per `seconds`
   and never ends */
void obj_turn(Obj3d *o, const float ang[3], float seconds, int mode);
void obj_move(Obj3d *o, const float pos[3], float seconds, int mode);
void obj_update(Obj3d *o);                                         /* 0x47a3d0 */
void obj_draw(const Obj3d *o);                                     /* 0x479eb0 */

#endif
