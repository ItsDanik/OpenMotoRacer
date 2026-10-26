#ifndef SAVES_H
#define SAVES_H
#include <stdio.h>

/* Where the game's own save files go: bestscor.dat, Champ<n>.sav (and later the
   time-attack ghosts). The original keeps them in its Data folder; the port writes them
   in a folder of the player's (-home, OMR_HOME, else ~/.config/openmotoracer) in the
   game's own format, and reads a file there first, else the one in the game's Data
   folder, so a player's records come along. */
void saves_init(const char *home);
const char *saves_home(void);
/* name as the game has it under Data ("bestscor.dat", "Champ0.sav"); mode "rb" or "wb" */
FILE *save_open(const char *name, const char *mode);

#endif
