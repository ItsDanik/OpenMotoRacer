#ifndef BIKEPHYS_INT_H
#define BIKEPHYS_INT_H

/* The pieces of bikephys.c the computer riders' step (cpu.c) shares with the player's: the
   same routines of the game, called from 0x46bf80 as from 0x45d580. Only for cpu.c (and the
   checks in tools/). Arithmetic as in bikephys.c: doubles in registers, floats when stored. */
#include "bikephys.h"

/* the track as the physics keeps it (0x52c108, 0xfc a section) */
int bp_nsec(void);                                      /* 0x52c10c */
int bp_wrap(int sec);                                   /* 0x40ec30 */
const float *bp_sec_pos(int sec);                       /* +0x00 */
const float *bp_sec_m(int sec);                         /* +0x0c: rows X, Y, Z, then T */
float bp_sec_line(int sec);                             /* +0xb0: the racing line */
int bp_walk(int hint, const float p[3]);                /* 0x40c7e0 */
double bp_road_y(int sec, const float p[3]);            /* 0x40c8b0 */
int bp_walls(int sec, const float a[3], const float b[3], float hit[3], float v[3],
             float n[3]);                               /* 0x40ca80 */
int bp_bends(int sec, int n);                           /* 0x46c7a0 */

/* vectors and matrices */
void bp_mulvec(float o[3], const float v[3], const float *m);           /* 0x410010 */
void bp_cross(float o[3], const float a[3], const float b[3]);          /* 0x46d610 */
void bp_normalise(float v[3]);                                          /* 0x40fcd0 */
void bp_look(float *o, const float a[3], const float b[3]);             /* 0x46bdc0 */
double bp_heading(const float *m);                                      /* 0x490220 */
float bp_wrap_ge(float a);                      /* the inline [0, 2pi) wrap */
void bp_turn_yaw(BikePhys *b, float yaw);       /* 0x40fd40 (0, yaw, 0) then 0x45cb00 */

/* the bike's step pieces */
float bp_carac(const BikePhys *b, int off);     /* the carac row, C+off */
int bp_perso(const BikePhys *b);                /* C+0x120: the dirt rider */
void bp_start_step(BikePhys *b);                /* 0x465fa0 */
void bp_turbo(BikePhys *b, float dt);           /* 0x4660f0 */
void bp_frame_and_lean(BikePhys *b);            /* 0x469c60 */
void bp_wheel_spin(BikePhys *b);                /* 0x466940 (a computer bike: its +0x2dc8) */
void bp_suspension(BikePhys *b);                /* 0x466a80 */
void bp_tumble(BikePhys *b, float dt);          /* 0x4683e0 */
void bp_dials(BikePhys *b);                     /* 0x467930 */

#endif
