/* The race's small keys from 0x431250 that have something of their own to show (NOTES
   §4.20 §2.6, §5b). */
#include "racekeys.h"
#include "config.h"
#include "exe.h"
#include "font.h"
#include "hud.h"
#include "sound.h"
#include "text.h"
#include "trackmap.h"
#include "video.h"
#include <stdio.h>
#include <string.h>

/* 0x409720: * up, / down, on the line the modifiers pick: Shift the wave's, Ctrl the CD's,
   neither the master's. The word and the bar for two seconds, a character each step; the
   lines' steps are the mixer's 32. */
void racekeys_volume(int up, int shift, int ctrl)
{
    static int id;                      /* 0x4e98ec */
    char s[64], *p = s;
    int line = shift ? 2 : ctrl ? 1 : 0, label = shift ? 77 : ctrl ? 78 : 76, v, i;
    if (!sound_mixing()) {              /* (0x5d2958 == 0) */
        snprintf(s, sizeof s, "[%s]", exe_text(75));
        hud_say(&id, s, 2000, 0x22, 0, 0x1e, FX_BLINK, 0x20, 4);
        return;
    }
    v = g_volume[line] + (up ? 1 : -1);
    if (v >= 0 && v <= 32) {            /* 0x470d90, 0x470e90 */
        g_volume[line] = v;
        sound_set_volumes(g_volume);
    }
    v = g_volume[line];
    p += snprintf(p, sizeof s - 36, "[%s\x82", exe_text(label));
    for (i = 0; i < 32; i++)
        *p++ = i < v ? '\x81' : '\x80';
    strcpy(p, "\x83 ]");
    hud_say(&id, s, 2000, 0x22, 0, 0x1e, FX_NONE, 0x20, 4);
}

/* 0x484380: the screen's size for six seconds, or if it is showing, gone. The first text
   (129, the Link's) needs 0x4dbffc == 0: never here. The memory figures of DirectDraw that
   the game has on the last line (VIDEO, TXT) mean nothing here and are left out. */
void racekeys_info(void)
{
    static int id;                      /* 0x4e98f0 */
    char s[128];
    int n;
    TextArgs a;
    if (text_find(id)) {
        text_leave(id);
        return;
    }
    n = snprintf(s, sizeof s, "[  %dX%d]", g_half_rows ? 640 : g_scr_w, video_surface_h());
    if (g_mode169)
        n += snprintf(s + n, sizeof s - (size_t)n, "\n[%s", exe_text(143));
    if (video_fullscreen())
        n += snprintf(s + n, sizeof s - (size_t)n, "\n[%s", exe_text(148));
    if (behaviour_cd())                 /* 0x4a5f20: the 1997 CD's, 8M its largest */
        snprintf(s + n, sizeof s - (size_t)n, "\n[  VIDEO 8M]");
    memset(&a, 0, sizeof a);
    a.text = s;
    a.lifetime = 6000;
    a.font = 6;
    a.x = 2;
    a.y = 2;
    a.align = 0x81;
    a.motion = 4;
    a.id_out = &id;
    a.dodge_x = a.dodge_y = 5;
    a.vx = -1;
    a.k = 1.0;
    text_add(&a);
}

/* What 0x4592a0, the screen set up again, does for the race: every text away (the dials
   and the dashboard's are drawn anew each picture), the map made again, the word above. */
void racekeys_new_screen(void)
{
    text_clear();                       /* 0x417750 x 3 */
    trackmap_build();                   /* 0x40ed10 */
    racekeys_info();
}

/* 0x4866d0: the small fonts on or off, then the screen again. (Open dialogs are laid out
   again there, 0x4348f0, but while one is open they take this key themselves.) */
void racekeys_small_fonts(void)
{
    g_small_fonts = !g_small_fonts;
    racekeys_new_screen();
}
