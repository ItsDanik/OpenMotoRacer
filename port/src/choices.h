#ifndef CHOICES_H
#define CHOICES_H

/* The menu's choices the race reads (NOTES §4.12) and the lists they choose from: the
   tracks (Data\list.spt) and the championship (Data\champ<A..C>.spt). */
enum { MODE_PRACTICE = 0, MODE_SINGLE = 1, MODE_CHAMPIONSHIP = 2 };

typedef struct {                /* 0x130 bytes at *0x4dc088 */
    char name[32];              /* +0x04: "SPEED_BAY" */
    char folder[0x104];         /* +0x24: "data\ile01\" */
    int available;              /* +0x128: "ON" in the list (a championship unlocks more) */
    int laps;                   /* +0x12c: the race's laps */
} Track;

typedef struct {                /* 0x50 bytes at *0x508934 */
    char track[32];             /* +0x00: a track's name */
    int reverse, pocket;        /* +0x20, +0x24: how it is raced */
    int unlock_reverse;         /* +0x28: winning it shows the Reverse switch */
    int unlock_pocket;          /* +0x2c: and the Pocket one */
    char unlock[32];            /* +0x30: a track it makes available, or "NONE" */
} ChampRace;

extern Track *g_tracks;         /* 0x4dc088 */
extern int g_ntracks;           /* 0x51d298 */
extern ChampRace *g_champ;      /* 0x508934 */
extern int g_nchamp;            /* 0x5083b4 */

extern int g_solo;              /* 0x4dbffc: 1 Solo, 0 Link */
extern int g_mode;              /* 0x4dc094: MODE_* in Solo */
extern int g_track;             /* 0x4dc08c: into g_tracks */
extern int g_laps;              /* 0x4dc074 */
extern int g_time_attack;       /* 0x4dc148 */
extern int g_gearbox;           /* 0x4dc0a0: 1 automatic, 0 manual */
extern int g_difficulty;        /* 0x4dc060: 0 easy, 1 medium, 2 hard (Config.Spt) */
extern int g_opponents;         /* 0x4dbfdc: 0 = 24 riders, 1 = 12 (Config.Spt) */
extern int g_champ_race;        /* 0x509ba8: the championship's race being run */
extern int g_champ_shown;       /* 0x4e90c8: page 9 has shown the standings */
extern int g_champ_on;          /* 0x4dc07c: the race end's "the championship goes on" */
extern int g_list_number;
extern int g_champ_new;         /* 0x4e90b0: the championship is at its first race */
extern int g_riders;            /* 0x509bac: riders in the race (the race sets it) */
#define CHAMP_ROWS 8
extern unsigned char g_champ_table[CHAMP_ROWS][0xf9];  /* 0x5d0d58: the standings */       /* 0x4dc048: the data disk's list, 0 none */

int tracks_load(void);          /* 0x428d60 */
int champ_load(void);           /* 0x429a00, from the script the main menu opens */
int track_usable(int i);
void track_legal(int *track);   /* 0x447920 */

#endif
