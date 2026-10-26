/* The race's camera (NOTES §4.20 §2): the chase 0x46f840 behind the followed bike, its
   four distances (F5), the look back (Tab: the bike-mounted preset 1) and the orbit to
   the front once the bike has finished. The on-board camera (F6) and the TV director of
   the replays are not written yet. */
#include "camera.h"
#include "menu.h"
#include "r3d.h"
#include <math.h>
#include <string.h>

#define ANGLE_K 2607.594587618272       /* 16384 / 2 pi: the cosine table's index */
#define PI 3.141592653589793

static struct {
    float ang[3];               /* +0x38: the camera's angles (the yaw +0x3c) */
    float pos[3];               /* +0x98 */
    float target[3];            /* +0xa4 */
    float lerp[3];              /* +0xc8: the chase's lerped point */
    float c[3][3];              /* +0x68 */
    int preset;                 /* +0x21c: 1 looks back (Tab) */
    int dist;                   /* +0x220: the chase distance, 0..3 (F5) */
} g_cam;

static const float g_extra[4] = { 0.0f, 128.0f, 256.0f, 512.0f };       /* +0x224 */
static const float g_preset_off[2][3] = { { 0, -160, 1680 }, { 0, 160, -20 } };   /* +0x11c */
static const float g_preset_ang[2][3] = { { 0, 0, 0 }, { 0, 3.14159265f, 0 } };  /* +0x17c */
#define CHASE_Y 64.0f           /* +0x1dc */
#define OFF_Y 191.0f            /* +0x54 */
#define OFF_Z -484.0f           /* +0x58 */

/* 0x490220: the heading of a matrix of rows (its T is 0) */
static double heading(const float *m)
{
    double fx = m[6], fy = m[7], fz = m[8], ux = m[3], uy = m[4];
    double l = sqrt(fx * fx + fy * fy + fz * fz);
    if (l < 1e-4)
        return PI / 2;
    return atan2((ux * fy - fx * uy) / l, fz / l);
}

/* 0x403a80: A then B, for row vectors */
static void mul(float o[3][3], const float a[3][3], const float b[3][3])
{
    float r[3][3];
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            r[i][j] = (float)((double)a[i][0] * b[0][j] + (double)a[i][1] * b[1][j]
                              + (double)a[i][2] * b[2][j]);
    memcpy(o, r, sizeof r);
}

/* v M + P, as the chase sums it: ((y M1 + x M0) + z M2) + P */
static void place(float o[3], const float v[3], const float m[3][3], const float p[3])
{
    int i;
    for (i = 0; i < 3; i++)
        o[i] = (float)((((double)v[1] * m[1][i] + (double)v[0] * m[0][i])
                        + (double)v[2] * m[2][i]) + p[i]);
}

/* W = R(0, psi, 0) R(the camera's angles): the bike's heading only */
static void chase_frame(float w[3][3], const BikePhys *b)
{
    float yaw[3] = { 0.0f, 0.0f, 0.0f }, ry[3][3], rc[3][3];
    yaw[1] = (float)-heading(b->m);
    r3d_rotation(ry, yaw, ANGLE_K);
    r3d_rotation(rc, g_cam.ang, ANGLE_K);
    mul(w, (const float (*)[3])ry, (const float (*)[3])rc);
}

void camera_reset(const BikePhys *b)
{
    float w[3][3], e[3];
    memset(g_cam.ang, 0, sizeof g_cam.ang);     /* 0x46da40 */
    g_cam.preset = 0;
    /* where the chase rests ([guess]: 0x46d760 leaves +0xc8 at 0, the race does not
       set it again; the port starts the camera at rest instead of sweeping in) */
    chase_frame(w, b);
    e[0] = 0.0f, e[1] = CHASE_Y, e[2] = OFF_Z + g_extra[g_cam.dist];
    place(g_cam.lerp, e, (const float (*)[3])w, b->pos);
    place(g_cam.target, g_preset_off[0], (const float (*)[3])w, b->pos);
    memcpy(g_cam.pos, g_cam.lerp, sizeof g_cam.pos);
    camera_step(b, 1.6f, 1, 0);
}

/* 0x46f840 */
static void chase(const BikePhys *b, float dt, int finished)
{
    float w[3][3], e0[3], t0[3], v[3];
    double d, u[3], l;
    int i;
    if (b->crashed) {           /* between the bike and the rider's body, from where it is */
        for (i = 0; i < 3; i++)
            g_cam.target[i] = (float)(((double)b->pos[i] + b->tumble_pt[i]) * 0.5);
        r3d_camera(g_cam.pos, g_cam.target);
        r3d_get_camera(g_cam.c, v);
        return;
    }
    if (g_cam.preset) {         /* R(angles) M inverted: the transpose, T = 0 */
        float r[3][3], m[3][3], rm[3][3];
        memcpy(m, b->frame, sizeof m);
        r3d_rotation(r, g_preset_ang[g_cam.preset], ANGLE_K);
        mul(rm, (const float (*)[3])r, (const float (*)[3])m);
        for (i = 0; i < 3; i++) {
            int j;
            for (j = 0; j < 3; j++)
                g_cam.c[i][j] = rm[j][i];
        }
        memcpy(m, b->m, sizeof m);
        place(g_cam.pos, g_preset_off[g_cam.preset], (const float (*)[3])m, b->pos);
        return;
    }
    d = (double)g_extra[g_cam.dist] + OFF_Z;
    if (finished) {             /* the orbit to the front: -3 pi / 4, 1.5 times as far */
        if (g_cam.ang[1] > -2.356194490192345)
            g_cam.ang[1] = (float)(dt * -0.010471975511965976 + g_cam.ang[1]);
        d -= (g_cam.ang[1] * -0.4244131815783876) * (OFF_Z * -0.5);
    }
    chase_frame(w, b);
    e0[0] = g_preset_off[0][0], e0[1] = CHASE_Y, e0[2] = (float)d;
    place(e0, e0, (const float (*)[3])w, b->pos);
    for (i = 0; i < 3; i++)     /* 0.2 a picture, whatever dt */
        g_cam.lerp[i] = (float)((e0[i] - (double)g_cam.lerp[i]) * 0.2f + g_cam.lerp[i]);
    if (finished) {
        static const float up[3] = { 0.0f, CHASE_Y, 0.0f };
        place(t0, up, (const float (*)[3])w, b->pos);
    } else
        place(t0, g_preset_off[0], (const float (*)[3])w, b->pos);
    g_cam.target[0] = (float)((t0[0] - (double)g_cam.target[0]) * 0.5f + g_cam.target[0]);
    g_cam.target[1] = (float)((t0[1] - (double)g_cam.target[1]) * 0.3333333432674408
                              + g_cam.target[1]);
    g_cam.target[2] = (float)((t0[2] - (double)g_cam.target[2]) * 0.5f + g_cam.target[2]);
    r3d_camera(g_cam.lerp, g_cam.target);
    r3d_get_camera(g_cam.c, v);
    /* the eye pushed to exactly |d| from the bike along the lerped direction, 191 up */
    for (i = 0; i < 3; i++)
        u[i] = (double)g_cam.lerp[i] - b->pos[i];
    l = sqrt((u[1] * u[1] + u[2] * u[2]) + u[0] * u[0]);
    if (l != 0.0)
        for (i = 0; i < 3; i++)
            u[i] = (float)(u[i] / l);
    for (i = 0; i < 3; i++)
        g_cam.pos[i] = (float)(b->pos[i] - u[i] * d);
    g_cam.pos[1] = (float)(g_cam.pos[1] + OFF_Y);
    if (finished)
        g_cam.pos[1] = (float)(g_cam.pos[1]
                               - (OFF_Y * 0.6666666865348816) * (g_cam.ang[1] * -0.4244131815783876));
}

void camera_step(const BikePhys *b, float dt, int countdown, int finished)
{
    float *yaw = &g_cam.ang[1];
    if (countdown) {            /* 0x458620: the yaw comes back to 0, pi / 128 a 10 ms */
        if (*yaw > 0.0f) {
            *yaw = (float)(dt * -0.02454369260617026 + *yaw);
            if (*yaw < 0.0f)
                *yaw = 0.0f;
        } else if (*yaw < 0.0f) {
            *yaw = (float)(dt * 0.02454369260617026 + *yaw);
            if (*yaw > 0.0f)
                *yaw = 0.0f;
        }
    }
    chase(b, dt, finished);
}

void camera_key(int vk, int down)
{
    if (vk == VK_TAB) {         /* 0x431250 / 0x4328c7 */
        g_cam.ang[1] = down ? (float)PI : 0.0f;
        g_cam.preset = down ? 1 : 0;
    } else if (vk == 0x74 && down)              /* F5 */
        g_cam.dist = (g_cam.dist + 1) & 3;
}

void camera_apply(void)
{
    r3d_set_camera((const float (*)[3])g_cam.c, g_cam.pos);
}

const float *camera_eye(void)
{
    return g_cam.pos;
}
