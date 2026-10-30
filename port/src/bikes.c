/* The bike family of a track (0x40a4d0): Level.Spt (0x427e80) for the family's script,
   the family's script (0x429e20) for the eight carac<n>.spt, each read by 0x42a580 into
   the bike table, and the names of its banks, skeletons, models and animations (g_fam). */
#include "bikes.h"
#include "choices.h"
#include "menu.h"
#include "script.h"
#include <stdio.h>
#include <string.h>

Bike g_bikes[BIKES];
Family g_fam;
int g_bike;
char g_family[2][64];

/* Level.Spt: NewMotorEngine, then FileLand, FileFrame3d, FileDecor, its reverse, FileNet,
   FileMap, FileMoto (the family), the Pocket family, ... */
static int level_load(int track)
{
    Script s;
    char path[0x120], skip[64];
    int i;
    snprintf(path, sizeof path, "%sLevel.Spt", g_tracks[track].folder);
    if (!script_open(&s, path))
        return 0;
    script_int(&s);
    for (i = 0; i < 6; i++)
        script_str(&s, skip, sizeof skip);
    script_str(&s, g_family[0], sizeof g_family[0]);
    script_str(&s, g_family[1], sizeof g_family[1]);
    i = !s.bad;
    script_close(&s);
    return i;
}

/* the family: two texture banks, two skeletons, seven models (a .3df and its .map each),
   then the rider's animations until a name without a dot ("MotoCarac"); a name "_<digit>_"
   begins a stunt of that many poses, two numbers after it (its time, the hand skeleton), and
   any name beginning with "_" loses its first three letters. Then the eight bikes. */
static int family_load(const char *path, char carac[BIKES][64])
{
    Script s;
    char name[64];
    int i;
    if (!script_open(&s, path))
        return 0;
    memset(&g_fam, 0, sizeof g_fam);
    for (i = 0; i < 2; i++)
        script_str(&s, g_fam.bank[i], sizeof g_fam.bank[i]);
    for (i = 0; i < 2; i++)
        script_str(&s, g_fam.skl[i], sizeof g_fam.skl[i]);
    for (i = 0; i < FM_COUNT * 2; i++)
        script_str(&s, g_fam.model[i / 2][i % 2], sizeof g_fam.model[0][0]);
    for (;;) {
        script_str(&s, name, sizeof name);
        if (s.bad || !strchr(name, '.'))
            break;
        if (g_fam.nrot >= FAMILY_ROTS)
            continue;
        if (name[0] == '_' && name[1] >= '0' && name[1] <= '9') {
            FamilyStunt *t = &g_fam.stunt[g_fam.nrot];
            t->frames = name[1] - '0';
            g_fam.stunt_list[g_fam.nstunts++] = g_fam.nrot;
            t->ms = script_int(&s);
            t->hand = script_int(&s);
        }
        memcpy(g_fam.rot[g_fam.nrot++], name[0] == '_' ? name + 3 : name, sizeof name - 3);
    }
    for (i = 0; i < BIKES; i++)
        script_str(&s, carac[i], 64);
    i = !s.bad;
    script_close(&s);
    return i;
}

static int carac_load(const char *path, Bike *b)
{
    Script s;
    int i;
    if (!script_open(&s, path))
        return 0;
    b->type = script_int(&s);
    for (i = 0; i < 4; i++)
        b->gfx[i] = script_int(&s);
    for (i = 0; i < CARAC_VALUES - 6; i++)
        b->phys[i] = script_float(&s);
    b->perso = script_int(&s);
    i = !s.bad;
    script_close(&s);
    return i;
}

int bikes_load(int track)
{
    char carac[BIKES][64];
    int i;
    if (track < 0 || track >= g_ntracks || !level_load(track)
        || !family_load(g_family[g_pocket != 0], carac))
        return 0;
    for (i = 0; i < BIKES; i++)
        if (!carac_load(carac[i], &g_bikes[i]))
            return 0;
    return 1;
}
