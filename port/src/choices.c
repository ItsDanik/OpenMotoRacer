/* The menu's choices the race reads (NOTES §4.12), with their values before the first
   menu (the program's data, and race slot 0's defaults 0x47bf30), and the two lists. */
#include "choices.h"
#include "menu.h"
#include "script.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

Track *g_tracks;
int g_ntracks;
ChampRace *g_champ;
int g_nchamp;

int g_solo = 1;
int g_mode;
int g_track;
int g_laps = 3;
int g_time_attack = 1;
int g_gearbox = 1;
int g_difficulty = 1;
int g_opponents;
int g_champ_race;
int g_champ_shown;
int g_champ_on = 1;
int g_list_number;
int g_champ_new;
int g_riders;
char g_list_name[32];
unsigned char g_champ_table[CHAMP_ROWS][0xf9];

/* ListName, PocketAvailable, ReverseAvailable, NumberIslande, then IslandeList: name,
   folder, ON/OFF, laps for each track */
int tracks_load(void)
{
    Script s;
    char path[32], on[16];
    int i, n;
    if (g_list_number)
        snprintf(path, sizeof path, "Data\\list%02d.spt", g_list_number);
    else
        snprintf(path, sizeof path, "Data\\list.spt");
    if (!script_open(&s, path))
        return 0;
    script_str(&s, g_list_name, sizeof g_list_name);
    g_pocket_unlocked = script_int(&s);
    g_reverse_unlocked = script_int(&s);
    n = script_int(&s);
    free(g_tracks);
    g_ntracks = 0;
    if (n <= 0 || !(g_tracks = calloc((size_t)n, sizeof *g_tracks))) {
        g_tracks = 0;
        script_close(&s);
        return 0;
    }
    for (i = 0; i < n; i++) {
        Track *t = &g_tracks[i];
        script_str(&s, t->name, sizeof t->name);
        script_str(&s, t->folder, sizeof t->folder);
        script_str(&s, on, sizeof on);
        t->available = strcasecmp(on, "ON") == 0;
        t->laps = script_int(&s);
    }
    g_ntracks = s.bad ? 0 : n;
    script_close(&s);
    return g_ntracks > 0;
}

/* Data\champ<A + difficulty>.spt (champ<letter><list>.spt with a data disk): NumberEntry,
   then LevelList: track, reverse, pocket, the two unlocks, a track to unlock */
int champ_load(void)
{
    Script s;
    char path[32];
    int i, n;
    if (g_list_number)
        snprintf(path, sizeof path, "Data\\champ%c%02d.spt", 'A' + g_difficulty, g_list_number);
    else
        snprintf(path, sizeof path, "Data\\champ%c.spt", 'A' + g_difficulty);
    if (!script_open(&s, path))
        return 0;
    n = script_int(&s);
    free(g_champ);
    g_nchamp = 0;
    if (n <= 0 || !(g_champ = calloc((size_t)n, sizeof *g_champ))) {
        g_champ = 0;
        script_close(&s);
        return 0;
    }
    for (i = 0; i < n; i++) {
        ChampRace *r = &g_champ[i];
        script_str(&s, r->track, sizeof r->track);
        r->reverse = script_int(&s);
        r->pocket = script_int(&s);
        r->unlock_reverse = script_int(&s);
        r->unlock_pocket = script_int(&s);
        script_str(&s, r->unlock, sizeof r->unlock);
    }
    g_nchamp = s.bad ? 0 : n;
    script_close(&s);
    return 1;
}

/* a track may be chosen when it is available, or with the CDNALSI cheat (or, in Link
   with 0x4dbff8 clear, always) */
int track_usable(int i)
{
    return g_tracks[i].available == 1 || g_island_unlocked;
}

void track_legal(int *track)
{
    int i, start = *track;
    if (g_ntracks <= 0)
        return;
    if (g_mode == MODE_CHAMPIONSHIP) {
        if (g_champ_race >= g_nchamp) {     /* the championship is over */
            g_champ_race = 0;
            g_mode = MODE_SINGLE;
            *track = 0;
        } else
            for (i = 0; i < g_ntracks; i++)
                if (strcmp(g_tracks[i].name, g_champ[g_champ_race].track) == 0) {
                    *track = i;
                    break;
                }
    }
    if (*track < 0 || *track >= g_ntracks)
        *track = 0;
    if (start < 0 || start >= g_ntracks)
        start = *track;
    /* on until a usable one, or round to the track it was called with */
    while (!track_usable(*track)) {
        if (++*track >= g_ntracks)
            *track = 0;
        if (*track == start)
            break;
    }
}
