#ifndef FILES_H
#define FILES_H
#include <stddef.h>

/* the player's game folder (GOG's "Moto Racer": game.EXE and data/) */
int files_init(const char *data);
const char *files_data(void);

/* rel inside the game folder, any case, '\\' or '/' between parts; 0 if not there */
int files_find(const char *rel, char *out, size_t size);
/* a whole loose file, malloc'd; 0 if missing */
unsigned char *files_read(const char *rel, long *len);

/* a file as the game asks for it ("data\\menu\\bitmap\\title.lez"): from its bank
   (DATA\MENU.BKF, name BITMAP\TITLE.LEZ) first, then loose (0x42ef80) */
unsigned char *dsi_load(const char *path, long *len);

#endif
