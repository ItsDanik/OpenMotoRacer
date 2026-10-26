/* Pages 20 and 21, the championship's Load (start 0x450600, frame 0x450800) and Save
   (start 0x450d20, frame 0x450f40): eight slots, Data\Champ<n>.sav, each starting with its
   title (30 bytes) followed by the championship's state. Load comes from SELECT RACE in a
   championship, Save from TOP 5 between two races. The files are kept where saves.h says. */
#include "choices.h"
#include "config.h"
#include "font.h"
#include "menu.h"
#include "pages.h"
#include "r3d.h"
#include "saves.h"
#include <stdio.h>
#include <string.h>

#define PIC(id) (&g_pics[id])
enum {
    P_CANCEL = 0x47, P_CONTINUE = 0x4d, P_TILE = 0x71, P_TITLE = 0x73, P_BAR = 0x75,
    P_LOAD = 0x76, P_LOAD_HL = 0x77, P_SAVE = 0x78, P_SAVE_HL = 0x79, P_LINE = 0x7a,
    P_LINE_HL = 0x7b, P_LINE_HL2 = 0x7c
};
#define SLOTS 8
#define TITLE_LEN 0x1e

static char g_slot[SLOTS][0x20];        /* 0x5409c8: the saves' titles */

static void slot_file(char *out, size_t size, int i)
{
    snprintf(out, size, "Champ%1d.sav", i);
}

/* both starts: the pictures, and each slot's title ("" when there is no save) */
static void start(int save)
{
    static const unsigned char white[3] = { 255, 255, 255 };
    char name[32];
    int i, x = save ? 29 : 129;
    bitmap_place(PIC(P_TITLE), 0, 0, 0);
    bitmap_place(PIC(P_BAR), 129, 158, 0);
    bitmap_place(PIC(save ? P_SAVE : P_LOAD), 168, 164, 0);
    bitmap_place(PIC(save ? P_SAVE_HL : P_LOAD_HL), 168, 164, 0);
    bitmap_place(PIC(P_LINE), x, 158, 0);
    bitmap_place(PIC(P_LINE_HL), x, 180, 0);
    bitmap_place(PIC(P_LINE_HL2), x, 212, 0);
    bitmap_place(PIC(P_CANCEL), save ? 176 : 270, 411, 0);
    if (save)
        bitmap_place(PIC(P_CONTINUE), 447, 411, 0);
    for (i = 0; i < SLOTS; i++) {
        FILE *f;
        slot_file(name, sizeof name, i);
        g_slot[i][0] = 0;
        if ((f = save_open(name, "rb"))) {
            size_t n = fread(g_slot[i], 1, TITLE_LEN, f);
            g_slot[i][n < TITLE_LEN ? n : TITLE_LEN] = 0;
            g_slot[i][0x1f] = 0;
            fclose(f);
        }
    }
    font_set_colors(font_get(12), white, white);
}

void page_load_champ_start(void)
{
    start(0);
}

void page_save_champ_start(void)
{
    start(1);
}

static void put_u32(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static int get_u32(const unsigned char *p)
{
    return (int)((unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16
                 | (unsigned)p[3] << 24);
}

/* the state after the title, in the order 0x450f40 writes it */
enum { F_REVERSE, F_POCKET, F_DIFFICULTY, F_OPPONENTS, F_RACE, F_LAPS, F_RIDERS, NFIELDS };

/* 1 loaded, 0 the championship could not be read, -1 no save there */
static int load_slot(int i)
{
    unsigned char b[NFIELDS * 4];
    char name[32];
    FILE *f;
    slot_file(name, sizeof name, i);
    if (!(f = save_open(name, "rb")))
        return -1;
    if (fread(g_slot[i], 1, TITLE_LEN, f) != TITLE_LEN || fread(b, 4, NFIELDS, f) != NFIELDS
        || fread(g_champ_table, 0xf9, CHAMP_ROWS, f) != CHAMP_ROWS) {
        fclose(f);
        return 0;
    }
    fclose(f);
    g_reverse = get_u32(b + 4 * F_REVERSE);
    g_pocket = get_u32(b + 4 * F_POCKET);
    g_difficulty = get_u32(b + 4 * F_DIFFICULTY);
    g_opponents = get_u32(b + 4 * F_OPPONENTS);
    g_champ_race = get_u32(b + 4 * F_RACE);
    g_champ_new = g_champ_race == 0;
    g_laps = get_u32(b + 4 * F_LAPS);
    g_riders = get_u32(b + 4 * F_RIDERS);
    /* the championship of that difficulty */
    return champ_load();
}

static void save_slot(int i)
{
    static const char level[3] = { 'L', 'M', 'H' };
    const ChampRace *r = &g_champ[g_champ_race];
    unsigned char b[NFIELDS * 4];
    char title[96], name[32], *p;
    FILE *f;
    /* "DAN ON SPEED BAY (PRM)": the player, the next race and how it is run (the
       original writes what its buffer held after the title's end; zeros here) */
    memset(title, 0, sizeof title);
    snprintf(title, sizeof title, "%s ON %s (", g_player_name, r->track);
    for (p = title; *p; p++)
        if (*p == '_')
            *p = ' ';
    if (r->pocket)
        strcat(title, "P");
    if (r->reverse)
        strcat(title, "R");
    if (g_difficulty >= 0 && g_difficulty <= 2) {
        size_t n = strlen(title);
        title[n] = level[g_difficulty];
        title[n + 1] = 0;
    }
    strcat(title, ")");
    title[0x1f] = 0;
    memcpy(g_slot[i], title, 0x20);

    put_u32(b + 4 * F_REVERSE, (unsigned)g_reverse);
    put_u32(b + 4 * F_POCKET, (unsigned)g_pocket);
    put_u32(b + 4 * F_DIFFICULTY, (unsigned)g_difficulty);
    put_u32(b + 4 * F_OPPONENTS, (unsigned)g_opponents);
    put_u32(b + 4 * F_RACE, (unsigned)g_champ_race);
    put_u32(b + 4 * F_LAPS, (unsigned)g_laps);
    put_u32(b + 4 * F_RIDERS, (unsigned)g_riders);
    slot_file(name, sizeof name, i);
    if (!(f = save_open(name, "wb"))) {
        fprintf(stderr, "openmotoracer: %s/%s could not be written\n", saves_home(), name);
        return;
    }
    fwrite(title, 1, TITLE_LEN, f);
    fwrite(b, 4, NFIELDS, f);
    fwrite(g_champ_table, 0xf9, CHAMP_ROWS, f);
    fclose(f);
}

/* both frames up to the presses: the hovered slot gets the highlighted line */
static void draw(int save)
{
    int i, hit;
    unsigned nb = 0x0002ffffu;
    menu_begin_frame();
    hit = g_menu.sel;
    bitmap_step(PIC(P_TILE), 1, 1);
    bitmap_draw(PIC(P_TILE), BM_OWN, 100, 0);
    bitmap_draw(PIC(P_TITLE), BM_OWN, 100, 0);
    menu_clear_buttons();
    if (save)
        menu_add_button(PIC(P_CONTINUE), 0, 0x01ffff09u, -1, -1, 0, 0, 0, -1, -1, 0);
    else
        menu_add_button(PIC(P_CANCEL), 0, 0x08ffffffu, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
    bitmap_draw(PIC(P_BAR), BM_OWN, 100, 0);
    if (save)
        bitmap_draw(PIC(hit >= 1 ? P_SAVE_HL : P_SAVE), BM_OWN, 100, 0);
    else
        bitmap_draw(PIC(hit >= 1 ? P_LOAD_HL : P_LOAD), BM_OWN, 100, 0);
    /* the slots, one under the other: the arrows go down to the next and up to the one
       before, the first's up and the last's down to Cancel; keypad 1 to 8 */
    for (i = 0; i < SLOTS; i++) {
        int y = 197 + 22 * i;
        int pic = hit - i == 1 ? (save ? P_LINE_HL2 : P_LINE_HL) : P_LINE;
        menu_add_button(PIC(pic), 0, nb, 0x61 + i, -1, 0, 0, 0, 129, y, 0);
        nb = y == 329 ? 0x0700ffffu : nb + 0x01010000u;
    }
    menu_draw_buttons();
    for (i = 0; i < SLOTS; i++)
        font_draw(font_get(12), g_slot[i], 137, 198 + 22 * i, 0, 0);
}

static void finish(void)
{
    menu_transition();
    r3d_render();
    menu_end_frame();
}

void page_load_champ_frame(void)
{
    int pressed;
    draw(0);
    pressed = menu_pressed();
    if (pressed == 0) {
        menu_press_done();
        menu_capture();
        menu_change_page(6);
    } else if (pressed > 0 && pressed < 10) {
        menu_press_done();
        /* (a slot with no save does nothing; a championship that fails to read leaves
           the frame unfinished, as 0x450800 does) */
        int r = load_slot(pressed - 1);
        if (r == 0)
            return;
        if (r > 0)
            menu_change_page(6);
    }
    finish();
}

void page_save_champ_frame(void)
{
    int pressed;
    draw(1);
    pressed = menu_pressed();
    if (pressed == 0) {
        menu_press_done();
        menu_change_page(9);
    } else if (pressed > 0 && pressed < 10) {
        menu_press_done();
        save_slot(pressed - 1);
    }
    finish();
}
