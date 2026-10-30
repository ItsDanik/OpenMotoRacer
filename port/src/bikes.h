#ifndef BIKES_H
#define BIKES_H

/* The bikes of a track's family (NOTES §4.12.4): the track's Level.Spt names the family's
   script (road or dirt, its Pocket variant), which names the 8 bikes' carac<n>.spt. */
#define BIKES 8
#define CARAC_VALUES 73         /* 0x124 bytes a bike */

typedef struct {                /* 0x509278 + i * 0x124, filled by 0x42a580 */
    int type;                   /* +0x00: MotoType, 2 road, 1 dirt */
    int gfx[4];                 /* +0x04: the menu's bars: acceleration, top speed, grip,
                                   brakes, 0..100 */
    float phys[CARAC_VALUES - 6];   /* +0x14: the physics (MoteurVitesseMax, MotoMasse...) */
    int perso;                  /* +0x120: PersoAnim, the rider: 0 road, 1 dirt */
} Bike;

enum { FM_REAR, FM_FRONT, FM_MEDIUM, FM_LOW, FM_VERYLOW, FM_HANDLEBAR, FM_COCKPIT, FM_COUNT };
#define FAMILY_ROTS 48

typedef struct {                /* a stunt starting at a PilotRot */
    int frames;                 /* 0x580d10 + k x 16: the poses it goes through */
    int ms;                     /* 0x580d14 + k x 16: its time */
    int hand;                   /* 0x580d08 + k x 16: drawn with the hand skeleton */
} FamilyStunt;

typedef struct {                /* the family script's names (0x429e20) */
    char bank[2][64];           /* FileBnk0, FileBnk1 (0x518b20, 0x518b60) */
    char skl[2][64];            /* FileSkl, FileSklHand (0x518ba0, 0x518be0) */
    char model[FM_COUNT][2][64];/* each a .3df and its .map: the wheels (0x518c20..), the
                                   three details, the handlebar and the cockpit */
    char rot[FAMILY_ROTS][64];  /* PilotRot (0x5191e4 + k x 0x40) */
    int nrot;                   /* 0x5191e0 */
    FamilyStunt stunt[FAMILY_ROTS];
    int stunt_list[FAMILY_ROTS];/* 0x580d0c + j x 16: the stunts' places in rot */
    int nstunts;                /* 0x5cf4fc */
} Family;

extern Bike g_bikes[BIKES];
extern Family g_fam;
extern int g_bike;              /* 0x584499: player 1's bike, 0..7 (-1 none chosen yet) */
extern char g_family[2][64];    /* 0x51d068, 0x51d0a8: the family scripts, plain and Pocket */

int bikes_load(int track);      /* 0x40a4d0's reading part */

#endif
