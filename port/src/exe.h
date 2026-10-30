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
    X_SFX_MENU,         /* the sounds' {name, id} tables (NOTES §4.10.2): the menu's, */
    X_SFX_MENU_VOICES,  /* the menu's voices, */
    X_SFX_PODIUM,       /* the podium's */
    X_CHEATS,           /* the name entry's {name, variable, value} (0x4f1250) */
    X_CHEAT_COUNT,      /* how many (0x4f1248) */
    X_STRINGS,          /* the words in six languages {id, en, fr, de, it, sp, sw} (0x4e2920) */
    X_SFX_DIVERS,       /* the race's sounds: its effects, */
    X_SFX_KTM,          /* the dirt bikes' engines, */
    X_SFX_NINJA,        /* the road bikes' engines, */
    X_SFX_VOICES,       /* the race's voices */
    X_GEN_SHORT,        /* the victory sequence's scripts: the short list (0x4f1640), */
    X_GEN_LONG,         /* the long one (0x4f16a0) */
    X_COUNT
};
unsigned exe_addr(int what);

/* the bytes at the program's address va (len of them inside the image), else 0 */
const unsigned char *exe_at(unsigned va, unsigned len);
unsigned exe_u32(unsigned va);
const char *exe_str(unsigned va);       /* a zero-ended string there, else "" */

/* the game's word number id in the menus' language (0x415a70 fills text[] at 0x53b5e8:
   the language's, else the English) */
const char *exe_text(int id);

#endif
