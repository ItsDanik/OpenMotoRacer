#ifndef BIKE3D_H
#define BIKE3D_H

/* The bike and its rider in 3D (NOTES §4.15): the family named by g_fam (bikes_load) */
int bike3d_load(void);          /* the banks (pages 0, 1), the skeleton, the pose, wheels */
void bike3d_free(void);
/* page 10's camera (0x46dab0, 0x46e140): round the bike at angle theta, looking at target */
void bike3d_menu_camera(float theta, const float target[3]);
/* 0x45f290 at detail 0, page 10's way: the bike's measures from carac row `bike`, its
   livery `skin` (palette 0..7) */
void bike3d_draw(int bike, int skin);

#endif
