/* Page 15, the name entry (NOTES §4.11): the alphabet as 3D letters in a 7x4 grid that
   flies in and then ripples, a spinning frame round the chosen cell, the name typed so
   far along the bottom. Letters, Backspace, the arrows, Space, Return or the mouse; a
   cheat word sets its flag and starts the page again. */
#include "config.h"
#include "exe.h"
#include "menu.h"
#include "obj3d.h"
#include "pages.h"
#include "rand.h"
#include "sound.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define PIC(id) (&g_pics[id])
#define TWO_PI_F 6.28318548f            /* 0x40c90fdb */
enum { P_TILE3 = 2, P_ENTERNAM = 0x69, DELETE = 26, OK = 27, CELLS = 28, MAXLEN = 8 };

static Model g_model[CELLS], g_cadre_m, g_curseur_m;
static Obj3d g_letter[CELLS];   /* 0x5f1880 */
static Obj3d g_cadre, g_curseur;    /* 0x4f1288, 0x4f128c */
static Obj3d g_phase, g_amp;    /* 0x5f6480, 0x5f63e8: the ripple */
static Obj3d g_spinner;         /* 0x5f1690: the chosen letter's turn (kept between visits) */
static Obj3d g_slot[MAXLEN];    /* 0x5f6530: the name's letters */
static float g_grid[CELLS][3];  /* 0x5f18f0 */
static float g_slotpos[MAXLEN + 1][3];      /* 0x5f6378 */
static int g_cursor;            /* 0x5f647c */
static int g_len;               /* 0x5f1a44 */
static char g_name[MAXLEN + 1]; /* 0x5f6518 */
static int g_intro;             /* 0x5f6528 */
static int g_confirm;           /* 0x5f675c */
static const float g_zero[3], g_above[3] = { 0.0f, 800.0f, 500.0f };

/* r / 32767 x 2 - 1 (0x4d1a9c, 2.0, 1.0) of the menu's random stream (0x458cd0) */
static double rand_unit(void)
{
    return (float)((double)rand_next(1) * 3.0518509447574615e-05 * 2.0 - 1.0);
}

/* r / 32767 x 2pi x 3: a letter thrown away turns to this */
static float rand_turn(void)
{
    return (float)((double)(float)((double)rand_next(1) * 3.0518509447574615e-05
                                   * 6.283185307179586) * 3.0);
}

static void set_cursor_target(void)
{
    obj_move(&g_curseur, g_slotpos[g_len], 0.2f, OBJ_LINEAR);
}

/* 0x487a70: letter i into the name: its letter flies from the grid to its place, and the
   grid's letter drops in again from above */
static void type(int i)
{
    if (g_len < MAXLEN) {
        Obj3d *s = &g_slot[g_len];
        s->model = &g_model[i];
        obj_set(s, g_letter[i].ang, g_grid[i]);
        obj_move(s, g_slotpos[g_len], 1.0f, OBJ_EASE);
        obj_turn(s, g_zero, 1.0f, OBJ_EASE);
        g_name[g_len] = (char)('A' + i);
        obj_set_pos(&g_letter[i], g_above);
        obj_move(&g_letter[i], g_grid[i], 1.5f, OBJ_EASE);
        g_len++;
        g_cursor = i;
        set_cursor_target();
    }
    g_confirm = 0;
}

/* the last letter thrown back up, turning */
static void delete(void)
{
    float a[3];
    int k;
    if (g_len <= 0)
        return;
    g_name[--g_len] = 0;
    obj_move(&g_slot[g_len], g_above, 1.0f, OBJ_EASE);
    for (k = 0; k < 3; k++)
        a[k] = rand_turn();
    obj_turn(&g_slot[g_len], a, 1.0f, OBJ_EASE);
    g_cursor = DELETE;
    if (g_len < MAXLEN)
        set_cursor_target();
}

static void free_models(void)
{
    int i;
    for (i = 0; i < CELLS; i++)
        model_free(&g_model[i]);
    model_free(&g_cadre_m);
    model_free(&g_curseur_m);
}

void page_name_start(void)
{
    static const float spin_x[3] = { TWO_PI_F, 0.0f, 0.0f }, spin_y[3] = { 0.0f, TWO_PI_F, 0.0f };
    static const float campos[3] = { 0.0f, 0.0f, -2400.0f };
    char path[64];
    float s;
    int i, k;
    r3d_focal(1024.0f);
    r3d_menu_camera(g_zero, campos);
    g_confirm = 0;
    for (i = 0; i < 26; i++) {
        snprintf(path, sizeof path, "data\\menu\\font3d\\l%c", 'a' + i);
        model_load_menu(&g_model[i], path);
    }
    model_load_menu(&g_model[DELETE], "data\\menu\\font3d\\backspac");
    model_load_menu(&g_model[OK], "data\\menu\\font3d\\valid");
    model_load_menu(&g_cadre_m, "data\\menu\\font3d\\cadre");
    model_load_menu(&g_curseur_m, "data\\menu\\font3d\\curseur");
    memset(&g_cadre, 0, sizeof g_cadre);
    memset(&g_curseur, 0, sizeof g_curseur);
    g_cadre.model = &g_cadre_m;
    g_curseur.model = &g_curseur_m;
    model_fit(&g_cadre_m, 78.0f);

    memset(&g_phase, 0, sizeof g_phase);
    obj_set(&g_phase, g_zero, g_zero);
    obj_turn(&g_phase, spin_x, 1.0f, OBJ_SPIN);         /* once round a second */
    memset(&g_amp, 0, sizeof g_amp);
    obj_set(&g_amp, g_zero, g_zero);
    obj_turn(&g_spinner, spin_y, 1.0f, OBJ_SPIN);
    obj_turn(&g_cadre, spin_y, 1.0f, OBJ_SPIN);

    /* the letters: all the size of A (62), mirrored back, thrown somewhere behind */
    s = model_fit(&g_model[0], 62.0f);
    for (i = 0; i < CELLS; i++) {
        double a, b, c;
        float ang[3], pos[3];
        if (i)
            model_rescale(&g_model[i], s);
        model_mirror(&g_model[i]);
        a = rand_unit();
        b = rand_unit();
        c = rand_unit();
        ang[0] = (float)(a * 2.0 * 6.283185307179586);
        ang[1] = (float)(b * 2.0 * 6.283185307179586);
        ang[2] = (float)(c * 2.0 * 6.283185307179586);
        pos[0] = (float)(a * 2048.0);
        pos[1] = (float)(b * 512.0 + 2250.0);
        pos[2] = (float)(c * 1024.0 + 4096.0);
        memset(&g_letter[i], 0, sizeof g_letter[i]);
        g_letter[i].model = &g_model[i];
        obj_set(&g_letter[i], ang, pos);
    }
    model_rescale(&g_curseur_m, s);

    /* the grid: 7 columns from -550 to 550, 4 rows from 250 down to -250 */
    for (i = 0; i < CELLS; i++) {
        int r = i / 7, c = i % 7;
        g_grid[i][0] = (float)((double)c * (1.0 / 6) * 1100.0 - 550.0);
        g_grid[i][1] = (float)((double)(3 - r) * (1.0 / 3) * 500.0 - 250.0);
        g_grid[i][2] = 0.0f;
    }
    for (i = 0; i < CELLS; i++) {
        obj_turn(&g_letter[i], g_zero, 2.0f, OBJ_EASE);
        obj_move(&g_letter[i], g_grid[i], 2.0f, OBJ_EASE);
    }

    /* the name's places, waiting above the screen */
    for (k = 0; k < MAXLEN; k++) {
        float ang[3];
        int j;
        memset(&g_slot[k], 0, sizeof g_slot[k]);
        g_slot[k].model = &g_model[k];
        for (j = 0; j < 3; j++)
            ang[j] = rand_turn();
        obj_set(&g_slot[k], ang, g_above);
    }
    for (k = 0; k <= MAXLEN; k++) {
        g_slotpos[k][0] = (float)(k * 140.0 - 490.0);
        g_slotpos[k][1] = -325.0f;
        g_slotpos[k][2] = -600.0f;
    }
    g_cursor = 0;
    g_menu.key = 0;
    g_intro = 1;

    /* the name so far (DefName at first): capitals, anything else an A */
    memset(g_name, 0, sizeof g_name);
    for (g_len = 0; g_len < MAXLEN && g_player_name[g_len]; g_len++) {
        int ch = g_player_name[g_len] & 0xdf;
        if (ch < 'A' || ch > 'Z')
            ch = 'A';
        g_name[g_len] = (char)ch;
        g_slot[g_len].model = &g_model[ch - 'A'];
        obj_set(&g_slot[g_len], g_zero, g_above);
    }
    if (g_len > 0)
        g_cursor = OK;
    obj_set_pos(&g_curseur, g_above);
    obj_turn(&g_curseur, spin_y, 1.0f, OBJ_SPIN);

    bitmap_place(PIC(P_ENTERNAM), 0, 0, 0);
    g_menu.mouse_mode = 0;
    menu_clear_buttons();
    sound_queue(1, 600, 0x36);          /* "enter your name" */
}

/* the cell nearest the pointer, the letters' (x, y, 0) as seen through the camera */
static void mouse_cell(void)
{
    int i, best = 0x7fffffff, old = g_cursor;
    for (i = 0; i < CELLS; i++) {
        float v[3] = { g_letter[i].pos[0], g_letter[i].pos[1], 0.0f };
        int sx, sy, d;
        if (!r3d_menu_project(v, &sx, &sy))
            continue;
        d = (g_mx - sx) * (g_mx - sx) + (g_my - sy) * (g_my - sy);
        if (d < best) {
            best = d;
            g_cursor = i;
        }
    }
    if (g_cursor != old)
        g_spinner.restart = 1;
}

static void confirm(void)
{
    unsigned a = exe_addr(X_CHEATS), n = exe_u32(exe_addr(X_CHEAT_COUNT)), i;
    int cheat = 0;
    memcpy(g_player_name, g_name, sizeof g_name);
    for (i = 0; i < n && i < 16; i++) {
        unsigned var = exe_u32(a + 12 * i + 4);
        int *p = var == 0x4dc050 ? &g_reverse_unlocked2 : var == 0x4dc054 ? &g_pocket_unlocked2
               : var == 0x4dc058 ? &g_island_unlocked : 0;
        if (strcasecmp(exe_str(exe_u32(a + 12 * i)), g_player_name) != 0)
            continue;
        if (p)
            *p = (int)exe_u32(a + 12 * i + 8);
        cheat = 1;
    }
    r3d_focal(1911.0f);
    menu_press_done();
    if (cheat)
        menu_change_page(15);           /* again, without the glass */
    else {
        menu_capture();
        menu_change_page(3);
    }
}

void page_name_frame(void)
{
    int i, sound = 0;
    menu_begin_frame();
    g_menu.sel = -1;
    bitmap_step(PIC(P_TILE3), 0, 1);
    bitmap_draw(PIC(P_TILE3), BM_OWN, 100, 0);
    bitmap_step(PIC(P_ENTERNAM), 0, 0);
    bitmap_draw(PIC(P_ENTERNAM), BM_OWN, 100, 0);
    obj_update(&g_spinner);

    if (g_intro) {
        /* the letters fly to their cells; the frame follows the chosen one a frame late */
        for (i = 0; i < CELLS; i++) {
            if (i == g_cursor) {
                obj_set_pos(&g_cadre, g_letter[i].pos);
                obj_update(&g_cadre);
                obj_draw(&g_cadre);
                obj_update(&g_letter[i]);
                g_letter[i].ang[1] = g_spinner.ang[1];
            } else
                obj_update(&g_letter[i]);
            obj_draw(&g_letter[i]);
        }
        obj_update(&g_cadre);
        obj_draw(&g_cadre);
        if (!g_letter[0].turning) {
            static const float amp[3] = { 120.0f, 0.0f, 0.0f };
            obj_move(&g_amp, amp, 2.0f, OBJ_LINEAR);
            for (i = 0; i < g_len; i++) {
                obj_move(&g_slot[i], g_slotpos[i], 1.0f, OBJ_EASE);
                obj_turn(&g_slot[i], g_zero, 1.0f, OBJ_EASE);
            }
            if (g_len < MAXLEN)
                obj_move(&g_curseur, g_slotpos[g_len], 1.0f, OBJ_EASE);
            g_intro = 0;
        }
    } else {
        /* the ripple: depth = cos(phase) x amplitude, the phase pi/6 on a column and
           pi/7 on a row, each step kept as a float */
        float row;
        int r, c;
        obj_update(&g_phase);
        obj_update(&g_amp);
        row = g_phase.ang[0];
        for (r = 0, i = 0; r < 4; r++) {
            float p = row;
            for (c = 0; c < 7; c++, i++) {
                g_grid[i][2] = (float)(cos((double)p) * g_amp.pos[0]);
                obj_update(&g_letter[i]);
                g_letter[i].pos[2] = g_grid[i][2];
                if (i == g_cursor) {
                    obj_update(&g_cadre);
                    obj_set_pos(&g_cadre, g_letter[i].pos);
                    obj_draw(&g_cadre);
                    g_letter[i].ang[1] = g_spinner.ang[1];
                } else
                    g_letter[i].ang[0] = g_letter[i].ang[1] = g_letter[i].ang[2] = 0.0f;
                obj_draw(&g_letter[i]);
                p = (float)((double)p + 0.5235987755982988);
            }
            row = (float)((double)row + 0.4487989505128276);
        }
    }
    for (i = 0; i < MAXLEN; i++) {
        obj_update(&g_slot[i]);
        obj_draw(&g_slot[i]);
    }
    obj_update(&g_curseur);
    obj_draw(&g_curseur);
    menu_transition();
    r3d_render();
    menu_end_frame();

    /* the keys while the mouse is still, else the mouse */
    if (!g_confirm && !g_menu.mouse_mode) {
        int k = g_menu.key;
        if ((k >= 0x21 && k <= 0x24) || (k >= 0x60 && k <= 0x6b) || (k >= 0x6f && k <= 0x7b))
            g_menu.key = 0;
        switch (g_menu.key) {
        case VK_BACK:
            delete();
            g_menu.key = 0;
            break;
        case VK_RETURN:
            if (menu_alt())
                break;
            if (g_len > 0)
                sound = g_confirm = 1;
            g_menu.key = 0;
            break;
        case VK_SPACE:
            if (menu_alt())
                break;
            if (g_cursor < DELETE) {
                if (g_len < MAXLEN)
                    sound = 1;
                type(g_cursor);
            } else if (g_cursor == DELETE) {
                delete();
                sound = 1;
            } else if (g_len > 0)
                sound = g_confirm = 1;
            g_menu.key = 0;
            break;
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
            if (g_menu.key == VK_LEFT && --g_cursor < 0)
                g_cursor = OK;
            if (g_menu.key == VK_UP && (g_cursor -= 7) < 0)
                g_cursor += CELLS;
            if (g_menu.key == VK_RIGHT && ++g_cursor >= CELLS)
                g_cursor = 0;
            if (g_menu.key == VK_DOWN && (g_cursor += 7) >= CELLS)
                g_cursor -= CELLS;
            g_spinner.restart = 1;
            g_menu.key = 0;
            break;
        }
        k = g_menu.key;
        if ((k >= 'a' && k <= 'z') || (k >= 'A' && k <= 'Z')) {
            type((k | 0x20) - 'a');
            g_menu.key = 0;
        }
    } else if (!g_confirm) {
        mouse_cell();
        if (g_mouse == 2) {             /* held down this frame (0x5083b8 == 2) */
            if (g_cursor < DELETE)
                type(g_cursor);
            else if (g_cursor == DELETE)
                delete();
            else if (g_len > 0)
                g_confirm = 1;
            g_mouse = 0;
            sound = 1;
        }
    }
    if (sound)
        sound_play(3, 0);
    if (g_confirm)
        confirm();
}

void page_name_end(void)
{
    int i;
    for (i = 0; i < MAXLEN; i++)
        g_slot[i].model = 0;
    free_models();
}
