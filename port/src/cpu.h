#ifndef CPU_H
#define CPU_H

/* The computer riders (NOTES §4.21, build/notes/cpu_riders.md): their step 0x46bf80 with
   the driving 0x46b120 (along the racing line, a target 16 sections ahead, 9 for a dirt
   bike) and the lane choice 0x46cd20 (three lanes 240 units apart; a faster rider moves
   out to pass, or slows behind), and their speeds at the start (0x47bc60: Level.Spt's
   ComputerSpeed by the difficulty, shuffled).

   A computer bike is a BikePhys with driven = 0 (bikephys.c); it uses the game's C library
   rand() (g_bikeworld.crt), as the player's crashes do. */
#include "bikephys.h"

/* 0x45d540 for a computer bike (obj+8 == 0): 0x46bf80. dt = ms x 0.1, countdown =
   0x4dc0b4. g_bikeworld.rider[], .nriders, .reverse must be set (as for bikephys_step). */
void cpu_step(BikePhys *b, float dt, int countdown);

/* 0x45e940, the step of a rider moved from outside (Link's remote players; here the time
   attack's ghost): the body put on target, the point it came from before (+0x2df5, +0x2e01),
   b->speed and b->fwd as given (+0x124, +0x198). The road, the lean, the wheels, the tumble and
   the dials as a computer bike's. */
void cpu_passive_step(BikePhys *b, const float target[3], const float before[3], float dt,
                      int countdown);

/* 0x47bc60 (Solo): each computer rider's top speed +0xb4 = ComputerSpeed (km/h) / 3.6 x
   (easy 0.9, medium 1, hard 1.05) x (1 / FactorPocket when Pocket), then the riders' speeds
   swapped at random (opponents / 2 times). rider[2 .. n - 1] are the opponents; seed is
   the C library's srand() of 0x47bf70 (timeGetTime in the game). */
void cpu_start(BikePhys *rider[], int n, int difficulty, int pocket, unsigned seed, int shuffle);

#endif
