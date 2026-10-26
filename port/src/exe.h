#ifndef EXE_H
#define EXE_H

/* The player's own program, laid out at its addresses: the game's fixed tables are read
   from it, never copied into the port. 0 and a message if it is missing or unknown. */
int exe_load(char *err, int errsize);

/* the game's tables, by name: each build has them at its own address */
enum {
    X_BITMAP_TABLE,     /* the menu pictures (NOTES §4.7) */
    X_LANG_FOLDERS,     /* "data\\MenuUs\\", ... */
    X_COLOURS,          /* the named colours, R,G,B every 4 bytes (NOTES §4.8.3.5) */
    X_RAND_SEEDS,       /* the 8 random streams' seeds (0x4b1160) */
    X_COUNT
};
unsigned exe_addr(int what);

/* the bytes at the program's address va (len of them inside the image), else 0 */
const unsigned char *exe_at(unsigned va, unsigned len);
unsigned exe_u32(unsigned va);
const char *exe_str(unsigned va);       /* a zero-ended string there, else "" */

#endif
