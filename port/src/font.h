#ifndef FONT_H
#define FONT_H
#include <stdint.h>

/* The bitmap fonts (NOTES §4.8): FONTS.BPT names 35, each from up to 4 .FNT files. */

enum {
    FT_DIRECT = 1, FT_DIRECT_TRAME = 2, FT_CUSTOM = 3, FT_CUSTOM_TRAME = 4,
    FT_AA8 = 5, FT_AA8_TRANSP = 6, FT_PAL = 7, FT_MOTO = 8
};

typedef struct {
    int type;                       /* +4 */
    char name[16];                  /* +8 */
    char cond[24];                  /* the script's load condition (not used yet) */
    const unsigned char *glyph[256];/* +0x18 */
    uint16_t table[256];            /* +0x418: a pixel value's colour */
    int width[256];                 /* +0x618 */
    int height;                     /* +0xa18 */
    int line, letter;               /* +0xa1c, +0xa20: extra pixels between lines, letters */
    int loaded;                     /* +0x5248 */
    int nfiles;                     /* +0x524c */
    char file[4][130];              /* +0x5250 */
    int first[4], count[4];         /* +0x5454, +0x5458 */
    unsigned char *data[4];         /* the .FNT files, whole */
    const unsigned char *block[4];  /* +0x5484: each file's glyphs */
    int own_palette;                /* +0x547c: Pal and Moto fonts */
    unsigned char fore[3], back[3]; /* +0x5494, +0x5497: R, G, B */
    const unsigned char *pal;       /* +0x549a */
    int npal;                       /* +0x549e */
} Font;

/* the game's named colours (0x4f0dd4, R,G,B every 4 bytes) */
enum { COL_WHITE, COL_BLACK, COL_GREY, COL_CYAN, COL_MAGENTA, COL_YELLOW, COL_RED, COL_BLUE,
       COL_GREEN, COL_GREEN2, COL_ORANGE };
const unsigned char *named_colour(int c);

int font_init(char *err, int errsize);          /* FONTS.BPT and every font's files */
Font *font_get(int n);                          /* 0x4903d0 */
int font_partnered(int n);
extern int g_small_fonts, g_screen_mode;        /* 0x4f0e08, 0x4dc0c8 */
void font_set_colors(Font *f, const unsigned char *fore, const unsigned char *back);
void font_store_rgb(Font *f, const unsigned char *fore, const unsigned char *back);
void font_set_rgb(Font *f, const unsigned char *fore, const unsigned char *back);
void font_measure(const Font *f, const char *text, int *w, int *h, int one_line);
/* 0x493820 in the 640x480 menu: flags place the block (0x492c20) and each line in it,
   level is the effect level in percent */
/* where a block of text goes for these flags, and its size */
void font_place(const Font *f, const char *text, int *x, int *y, int flags, int *w, int *h);
void font_draw(Font *f, const char *text, int x, int y, int flags, int level);

#endif
