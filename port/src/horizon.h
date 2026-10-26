#ifndef HORIZON_H
#define HORIZON_H

/* The race's far background (build/notes/track_horizon.md): a 512-pixel panorama cut in
   two tiles (Bank\HorizHi.raw at 640x480, 0x410350), blitted around the camera's heading
   at the height its pitch gives, a colour filled above and below (0x402960). Drawn
   before the 3D, which covers it. */

int horizon_load(const char *folder);   /* "data\ile02\" */
void horizon_free(void);
/* c: the camera's +0x68, t: +0x8c, cam_y: +0x9c; offset and scale from the .trk; fx the
   focal length (0x6d20c4) */
void horizon_draw(const float c[3][3], const float t[3], float cam_y, int offset,
                  float scale, float fx);

#endif
