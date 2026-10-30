#ifndef SAVES_H
#define SAVES_H
#include <stdio.h>

/* Where the game's own save files go: bestscor.dat, Champ<n>.sav, the time-attack ghosts
   (in a track's folder: "ile01\\timatak__3.bin"). The original keeps them in its Data folder; the port writes them
   in a folder of the player's (-home, OMR_HOME, else ~/.config/openmotoracer) in the
   game's own format, and reads a file there first, else the one in the game's Data
   folder, so a player's records come along. */
void saves_init(const char *home);
const char *saves_home(void);
/* name as the game has it under Data ("bestscor.dat", "Champ0.sav", "ile01\\x.bin"); mode
   "rb" or "wb" */
FILE *save_open(const char *name, const char *mode);
/* the player's folder's copy alone (the settings: the port's own once written) */
FILE *save_open_own(const char *name, const char *mode);
/* forget a save: the player's folder's copy deleted. The game's own folder is never written,
   so if it holds one too an empty file stays in the player's folder, in front of it. */
void save_forget(const char *name);

#endif
