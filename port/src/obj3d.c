/* The menu's 3D object (NOTES §4.11.3): set, turn and move over a time (0x47a340..0x47a8b0),
   the step each frame (0x47a3d0) and the drawing (0x479eb0). Times are the game's ms. */
#include "obj3d.h"
#include "clock.h"
#include <math.h>
#include <string.h>

#define TWO_PI_F 6.28318548f            /* 0x4d18a0 */
#define PI 3.141592653589793            /* 0x4d1880 */

void obj_set(Obj3d *o, const float ang[3], const float pos[3])
{
    memcpy(o->ang, ang, sizeof o->ang);
    memcpy(o->pos, pos, sizeof o->pos);
    o->moving = o->turning = 0;
}

void obj_set_angles(Obj3d *o, const float ang[3])
{
    memcpy(o->ang, ang, sizeof o->ang);
    o->turning = 0;
}

void obj_set_pos(Obj3d *o, const float pos[3])
{
    memcpy(o->pos, pos, sizeof o->pos);
    o->moving = 0;
}

/* ftol(seconds x 1000.0): the double 0x4d18a8 */
static unsigned ms_of(float seconds)
{
    return (unsigned)(int)((double)seconds * 1000.0);
}

void obj_turn(Obj3d *o, const float ang[3], float seconds, int mode)
{
    int k;
    for (k = 0; k < 3; k++) {
        o->a0[k] = o->ang[k];
        o->ad[k] = mode == OBJ_SPIN ? ang[k] : ang[k] - o->ang[k];
    }
    o->adur = ms_of(seconds);
    o->astart = g_now;
    o->aend = g_now + o->adur;
    o->amode = mode;
    o->turning = 1;
}

void obj_move(Obj3d *o, const float pos[3], float seconds, int mode)
{
    int k;
    for (k = 0; k < 3; k++) {
        o->p0[k] = o->pos[k];
        o->pd[k] = mode == OBJ_SPIN ? pos[k] : pos[k] - o->pos[k];
    }
    o->pdur = ms_of(seconds);
    o->pstart = g_now;
    o->pend = g_now + o->pdur;
    o->pmode = mode;
    o->moving = 1;
}

/* a spun angle back into [-2pi, 2pi] by taking 2pi off as often as needed */
static float wrap(float x)
{
    uint32_t bits;
    double a;
    int neg;
    memcpy(&bits, &x, 4);
    neg = bits > 0x80000000u;
    a = neg ? -(double)x : x;
    if (a > TWO_PI_F) {
        while (a > TWO_PI_F)
            a -= TWO_PI_F;
        a = (float)a;
    }
    return (float)(neg ? -a : a);
}

/* one value along a turn or a move: linear, eased by a sine, or spun without end */
static void step(float *v, const float *v0, const float *d, unsigned start, unsigned end,
                 unsigned dur, int mode, int *active, int is_angle)
{
    unsigned now = g_now < end || mode == OBJ_SPIN ? g_now : end;
    int k;
    if (mode == OBJ_SPIN) {
        /* (the game's spun place uses now - duration: a slip no page meets) */
        unsigned since = is_angle ? g_now - start : g_now - dur;
        for (k = 0; k < 3; k++) {
            float x = (float)((double)d[k] * (double)(int)since / (double)(float)dur + v0[k]);
            v[k] = is_angle ? wrap(x) : x;
        }
        return;
    }
    {
        double r = (float)((double)(int)(now - start) / (double)(float)dur);
        if (mode == OBJ_EASE)
            r = (sin(r * PI - PI / 2) + 1.0) * 0.5;
        for (k = 0; k < 3; k++)
            v[k] = (float)((double)d[k] * r + v0[k]);
    }
    if (now == end)
        *active = 0;
}

void obj_update(Obj3d *o)
{
    if (o->restart) {
        o->astart = g_now;
        o->restart = 0;
    }
    if (o->turning)
        step(o->ang, o->a0, o->ad, o->astart, o->aend, o->adur, o->amode, &o->turning, 1);
    if (o->moving)
        step(o->pos, o->p0, o->pd, o->pstart, o->pend, o->pdur, o->pmode, &o->moving, 0);
}

void obj_draw(const Obj3d *o)
{
    if (o->model)
        r3d_menu_object(o->model, o->ang, o->pos);
}
