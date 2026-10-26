/* The front end's machinery (menu\menu.cpp, menuTools.cpp; NOTES §4.4): the menu object,
   its pages, the page change, the buttons and how the keyboard and mouse work them. */
#include "menu.h"
#include "clock.h"
#include "pages.h"
#include "video.h"
#include <stdio.h>
#include <string.h>

Menu g_menu;
Bitmap g_pics[MENU_PICS];
unsigned g_mouse;
int g_mx, g_my;
int g_pocket_unlocked, g_pocket_unlocked2, g_reverse_unlocked, g_reverse_unlocked2;
int g_pocket, g_reverse;
int g_quit;

static int g_mods;              /* 0x4dbfc8: 1 Alt, 2 Shift, 4 Ctrl */
static int g_navigated;         /* 0x504e78: Tab or the arrows were used on this page */
static int g_changing;          /* 0x504e74: a page change is under way */
static int g_capture_pending;   /* 0x4dbd00: do the glass transition at the next start */
static int g_blink;             /* 0x4dbd08 */
static uint16_t g_capture[256 * 192];   /* texture slot 2: the old page, for the glass */

/* the page table 0x4dbb38: frame, start and end routines (0 = none); pages the port has
   not written yet have none and end the program when reached */
static const struct {
    void (*frame)(void), (*start)(void), (*end)(void);
    const char *what;
} pages[MENU_PAGES] = {
    [3] = { page_main_frame, page_main_start, 0, "main menu" },
    [4] = { page_quit_frame, 0, 0, "quit" },
};

/* the game has routines for every page but 0 */
static const char *const page_names[MENU_PAGES] = {
    "nothing", "loading", "network", "main menu", "quit", "solo", "track choice", "7",
    "8", "times", "10", "options", "12", "3D card", "title", "name entry", "16", "17", "18",
    "19", "championship saves", "championship saves", "22", "23", "podium",
    "Delphine logo", "solo (26)"
};

int sound_play(int id, int flags)
{
    (void)id;
    (void)flags;
    return -1;                  /* no sound yet: as the original with the effects off */
}

/* 0x40a9a0: the menu's resources, loaded at the first entry */
static int load_resources(char *err, int errsize)
{
    int id;
    for (id = 0; id < MENU_PICS; id++)
        if (!g_pics[id].pix && !bitmap_load_id(&g_pics[id], id)) {
            snprintf(err, errsize, "The menu picture %d could not be read.", id);
            return 0;
        }
    return 1;
}

/* 0x409f70 */
static void set_page(int page)
{
    if (!g_menu.active) {
        /* the first entry (or back from a race): 640x480, focal length 1911 (for the 3D
           pages, not yet), the resources */
        g_menu.active = 1;
        g_menu.sel = g_menu.prev_sel = g_menu.pressed = -1;
        g_menu.mouse_mode = 0;
    }
    g_menu.last_input = g_now;
    g_menu.started[page] = 0;
    g_menu.page = page;
    g_menu.key = 0;
    g_mouse = 0;
    g_menu.prev_mouse = 0;
}

void menu_change_page(int page)
{
    /* (0x409eb0 also deletes the text sprites and frees unpacked pictures) */
    g_menu.sel = -1;
    g_navigated = 0;
    g_menu.mouse_mode = 1;
    g_changing = 1;
    if (g_menu.started[g_menu.page]) {
        if (pages[g_menu.page].end)
            pages[g_menu.page].end();
        g_menu.started[g_menu.page] = 0;
    }
    set_page(page);
}

int menu_start(int page, char *err, int errsize)
{
    memset(&g_menu, 0, sizeof g_menu);
    g_menu.sel = -1;
    if (!load_resources(err, errsize))
        return 0;
    menu_change_page(page);
    return 1;
}

/* 0x479260: the back buffer into a 256x192 texture, keeping columns and rows 0, 2 of every
   5 (0, 2, 5, 7, 10, ...) */
static void capture_screen(void)
{
    int x, y;
    for (y = 0; y < 192; y++) {
        const uint16_t *row = g_screen + ((y >> 1) * 5 + (y & 1) * 2) * g_pitch;
        for (x = 0; x < 256; x++)
            g_capture[y * 256 + x] = row[(x >> 1) * 5 + (x & 1) * 2];
    }
}

void menu_capture(void)
{
    g_capture_pending = 1;
    capture_screen();
}

void menu_frame(void)
{
    int page = g_menu.page;
    if (!pages[page].frame && !pages[page].start && page != 0) {
        fprintf(stderr, "openmotoracer: the page %d (%s) is not written yet\n", page,
                page_names[page]);
        g_quit = 1;
        return;
    }
    /* the page's start */
    if (!g_menu.started[page]) {
        int i;
        g_menu.nbuttons = 0;
        for (i = 0; i < MENU_PICS; i++)
            g_pics[i].sliding = 0;
        if (pages[page].start) {
            pages[page].start();
            g_changing = 0;
        }
        g_menu.started[page] = 1;
        if (g_capture_pending) {
            /* the glass: the old page's picture breaks and flies at the viewer for 1.2 s
               (the pieces are not drawn yet: they need the 3D renderer) */
            g_menu.tr_running = 1;
            g_menu.tr_start = g_now;
            g_menu.tr_end = g_now + 1200;
            sound_play(0x2a, 0);
            g_capture_pending = 0;
        }
    }
    if (pages[page].frame)
        pages[page].frame();
    /* (the title's 30 s demo time-out goes here, with the title page) */
    /* a held left button presses a repeating button every frame */
    if ((g_mouse & 2) && (g_menu.prev_mouse & 2)) {
        g_menu.last_input = g_now;
        if (g_menu.mouse_mode && g_menu.sel != -1 && (g_menu.b[g_menu.sel].flags & 1))
            g_menu.pressed = g_menu.sel;
    }
    g_menu.prev_mouse = g_mouse;
}

void menu_clear_buttons(void)
{
    g_menu.nbuttons = 0;
}

void menu_add_button(Bitmap *pic, Bitmap *hl, unsigned nb, int hotkey, int group,
                     int value, int *var, int blink, int x, int y, int flags)
{
    Button *b;
    if (g_menu.nbuttons >= MENU_BUTTONS) {
        fprintf(stderr, "openmotoracer: MAX BUTTON reached\n");
        return;
    }
    b = &g_menu.b[g_menu.nbuttons++];
    memset(b, 0, sizeof *b);
    b->kind = 2;
    b->pic = pic;
    b->hl = hl;
    b->nb = nb;
    b->hotkey = hotkey;
    b->group = group;
    b->value = value;
    b->var = var;
    b->blink = blink;
    b->px = x;
    b->py = y;
    b->flags = flags;
}

int menu_hit(void)
{
    int i;
    if (!g_menu.mouse_mode)
        return g_menu.sel;
    for (i = 0; i < g_menu.nbuttons; i++) {
        Button *b = &g_menu.b[i];
        if (b->kind == 2) {
            Bitmap *p = b->pic;
            if (b->px != -1 && b->py != -1)
                bitmap_place(p, b->px, b->py, 0);
            if (p->x <= g_mx && g_mx <= p->x + p->w && p->y <= g_my && g_my <= p->y + p->h)
                return i;
        }
        /* (text buttons: their rectangle; with the text system) */
    }
    return -1;
}

void menu_draw_buttons(void)
{
    int i, g = -1;
    for (i = 0; i < g_menu.nbuttons; i++)
        if (g_menu.b[i].kind == 2)
            bitmap_step(g_menu.b[i].pic, 0, 0);
    if (g_menu.sel != -1)
        g = g_menu.b[g_menu.sel].group;
    g_blink ^= 1;
    for (i = 0; i < g_menu.nbuttons; i++) {
        Button *b = &g_menu.b[i];
        int state = 0;
        if (b->kind != 2)
            continue;           /* (text buttons: with the text system) */
        if (b->px != -1 && b->py != -1)
            bitmap_place(b->pic, b->px, b->py, 0);
        if (g != -1 && b->group == g)
            state = 4;
        if (g_menu.sel == i)
            state |= 2;
        if (b->var && *b->var == b->value)
            state |= 1;
        if (b->blink && g_blink)
            state = 0;
        /* the table 0x40c474 */
        switch (state) {
        case 0:
            bitmap_draw(b->pic, b->hl ? BM_OWN : BM_HALF_KEYED, 100, 0);
            break;
        case 2:
            if (!b->hl) {
                bitmap_draw(b->pic, BM_OWN, 100, 0);
                break;
            }
            /* fall through */
        case 1: case 6: case 7:
            bitmap_place(b->hl, b->pic->x, b->pic->y, 0);
            bitmap_draw(b->hl, BM_OWN, 100, 0);
            break;
        case 4:
            bitmap_draw(b->pic, BM_HALF, 100, 0);
            break;
        case 5:
            bitmap_place(b->hl, b->pic->x, b->pic->y, 0);
            bitmap_draw(b->hl, BM_HALF, 100, 0);
            break;
        default:                /* 3: selected and on: nothing (as the game has it) */
            break;
        }
    }
}

void menu_transition(void)
{
    if (!g_menu.tr_running)
        return;
    if (g_now < g_menu.tr_end) {
        g_menu.tr_t = (float)((double)(g_now - g_menu.tr_start) * 0.000833333354f);
        /* (0x479c10 queues the 99 pieces of glass here: with the 3D renderer) */
    } else
        g_menu.tr_running = 0;
}

int menu_pressed(void)
{
    return g_menu.tr_running ? -1 : g_menu.pressed;
}

void menu_press_done(void)
{
    g_menu.key = 0;
    g_menu.pressed = -1;
}

/* 0x49a160's part for the menu: 0x4099a0, the button under the pointer */
void menu_begin_frame(void)
{
    int sel = menu_hit();
    g_menu.sel = sel;
    if (sel != g_menu.prev_sel) {
        g_menu.prev_sel = sel;
        if (sel != -1)
            sound_play(0x29, 0);
    }
}

/* the game's own mouse pointer (Pointeur.lez, 0x55), its corner on the hot spot */
void menu_end_frame(void)
{
    Bitmap *p = &g_pics[0x55];
    bitmap_place(p, g_mx, g_my, 0);
    bitmap_draw(p, BM_OWN, 100, 0);
}

/* 0x40a0f0 */
void menu_key(int vk)
{
    int i;
    if (!g_menu.active)
        return;
    g_menu.last_input = g_now;
    g_menu.key = vk;
    if (vk != VK_SHIFT && vk != VK_CONTROL)
        g_menu.mouse_mode = 0;
    /* hotkeys: the last button with this key wins; Return after Tab or the arrows
       goes to the selection instead */
    if (!g_menu.tr_running && g_menu.pressed == -1)
        for (i = 0; i < g_menu.nbuttons; i++) {
            if (g_menu.b[i].hotkey != vk)
                continue;
            if (g_navigated && vk == VK_RETURN && !(g_mods & 1))
                break;
            g_menu.sel = g_menu.pressed = i;
            g_menu.key = 0;
            g_navigated = 0;
        }
    switch (g_menu.key) {
    case VK_TAB:
        if (g_mods & 2) {
            if (--g_menu.sel < 0)
                g_menu.sel = g_menu.nbuttons - 1;
        } else if (++g_menu.sel == g_menu.nbuttons)
            g_menu.sel = 0;
        g_navigated = 1;
        break;
    case VK_RETURN:
    case VK_SPACE:
        if (!g_menu.tr_running && !(g_mods & 1) && g_menu.sel != -1)
            g_menu.pressed = g_menu.sel;
        break;
    case VK_SHIFT:
        g_mods |= 2;
        break;
    case VK_CONTROL:
        g_mods |= 4;
        break;
    case VK_MENU:
        g_mods |= 1;
        break;
    case VK_LEFT:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_UP:
        if (g_menu.sel == -1)
            g_menu.sel = g_menu.nbuttons == 0;
        else {
            int shift = g_menu.key == VK_LEFT ? 0 : g_menu.key == VK_RIGHT ? 8
                      : g_menu.key == VK_DOWN ? 16 : 24;
            unsigned n = g_menu.b[g_menu.sel].nb >> shift & 0xff;
            if (n != 0xff)
                g_menu.sel = (int)n;
        }
        g_navigated = 1;
        break;
    }
}

void menu_key_up(int vk)
{
    if (vk == VK_SHIFT)
        g_mods &= ~2;
    else if (vk == VK_CONTROL)
        g_mods &= ~4;
    else if (vk == VK_MENU)
        g_mods &= ~1;
}

void menu_mouse_move(int x, int y)
{
    g_mx = x;
    g_my = y;
    if (g_menu.active) {
        g_menu.mouse_mode = 1;
        g_menu.last_input = g_now;
    }
}

void menu_mouse_button(int left, int down)
{
    unsigned bit = left ? 2 : 1;
    if (!down) {
        g_mouse &= ~bit;
        return;
    }
    g_mouse |= bit;
    /* 0x40a460: a button is pressed as the left button goes down */
    if (left && g_menu.active) {
        g_menu.last_input = g_now;
        if (g_menu.mouse_mode && g_menu.sel != -1) {
            g_menu.pressed = g_menu.sel;
            sound_play(3, 0);
        }
    }
}
