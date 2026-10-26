#ifndef R3D_H
#define R3D_H
#include <stdint.h>

/* The software 3D renderer (NOTES §4.9.3): texture pages, models, the float32 transform
   and projection (0x48c030, 0x49b2c0), the depth buckets (0x49b7f0), and the drawing
   nearest first into per-row lists of the pixels still free (0x49ad90, an s-buffer). */

#define R3D_PAGES 64

typedef struct {                /* 0x40 bytes in the game */
    int kind;                   /* +0x00: the draw kind (16, 21, 38, 39 textured) */
    int colour;                 /* +0x02: RGB565, for the untextured kinds */
    int page;                   /* +0x36 */
    int flags;                  /* +0x38: bit 1 = sorted 1024 farther */
    int n;                      /* 3 or 4 corners */
    int idx[4];                 /* +0x2c */
    int u[4], v[4];             /* +0x04: texels x 8192 (u/256 x 2^21, 0x49e510) */
} Face;

typedef struct {
    int nv, nf;
    float (*v)[3];
    Face *f;
} Model;

extern float g_cos[16384];      /* 0x5fcb80: cos(k x 2pi / 16384) */
extern uint16_t *g_texpage[R3D_PAGES];   /* page objects 0x4f92f8 + n x 0x338: 256x256 */

void r3d_init(void);
uint16_t *r3d_page(int n);                      /* a page's texels, made if missing */
int r3d_load_page(int n, const char *path);     /* 0x401160 */
void r3d_free_page(int n);

/* 0x4216a0 with the last argument 1: x96, the mirror and the corner order of §4.9.3.1 */
int model_load(Model *m, const char *base);
void model_centre(Model *m);                    /* 0x420250 */
void model_scale(Model *m, float sx, float sy, float sz);   /* 0x4204b0 */
void model_page_add(Model *m, int add);         /* 0x422250 */
void model_free(Model *m);

void r3d_camera(const float pos[3], const float target[3]);  /* 0x46e140 */
void r3d_camera_pos(const float pos[3]);        /* the block's +0x98 alone, not looked again */
void r3d_begin(void);                           /* 0x49a160: empties the lists */
/* 0x48c030: a model at an angle (a, b, c: radians) and a place, through the camera */
void r3d_object(const Model *m, const float ang[3], const float pos[3]);
/* 0x49b2c0 + 0x49b7f0 on points already in the view's frame: v x N + T */
void r3d_view(int nv, const float (*v)[3], int nf, const Face *f, const float n[3][3],
              const float t[3]);
void r3d_render(void);                          /* 0x49ad90 */

/* 0x48c030's rotation for row vectors, M = Rz(c) Ry(b) Rx(a), from the cosine table at
   index = trunc(angle x k) */
void r3d_rotation(float m[3][3], const float ang[3], double k);

#endif
