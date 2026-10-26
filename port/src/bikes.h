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

extern Bike g_bikes[BIKES];
extern int g_bike;              /* 0x584499: player 1's bike, 0..7 (-1 none chosen yet) */
extern char g_family[2][64];    /* 0x51d068, 0x51d0a8: the family scripts, plain and Pocket */

int bikes_load(int track);      /* 0x40a4d0's reading part */

#endif
