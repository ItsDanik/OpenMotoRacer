/* Page 3, the main menu (NOTES §4.4.5.2): the scrolling chequered background, the title
   bar sliding down, Solo and Link sliding in from the sides, then Options and Exit
   rising from below; the Pocket and Reverse switches when the name-entry cheats have
   unlocked them. */
#include "choices.h"
#include "clock.h"
#include "menu.h"
#include "pages.h"
#include "r3d.h"
#include "sound.h"

#define PIC(id) (&g_pics[id])
enum {
    P_TILE = 0, P_TITLE = 3, P_SOLO = 4, P_SOLO_HL = 5, P_LINK = 6, P_LINK_HL = 7,
    P_EXIT = 0x49, P_OPTIONS = 0x4b, P_POCKET_HL = 0x58, P_POCKET = 0x59,
    P_REVERSE_HL = 0x5a, P_REVERSE = 0x5b, P_OFF = 0x5c, P_ON = 0x5d, P_ON2 = 0x5e,
    P_OFF2 = 0x5f
};

static int g_rise_started;      /* 0x542380: Options and Exit are rising */
static int g_voice_played;      /* 0x53f554 */
static unsigned g_go_at;        /* 0x53f78c: the voice's end, -1 none */
static int g_pocket_button, g_reverse_button;   /* 0x53f848, 0x53f4e8: -2 none */

static int pocket_unlocked(void) { return g_pocket_unlocked || g_pocket_unlocked2; }
static int reverse_unlocked(void) { return g_reverse_unlocked || g_reverse_unlocked2; }

void page_main_start(void)
{
    int y, next = 4;
    bitmap_place(PIC(P_TITLE), 0, -80, 0);
    bitmap_place(PIC(P_SOLO), -140, 116, 0);
    bitmap_place(PIC(P_LINK), 780, 116, 0);
    bitmap_place(PIC(P_OPTIONS), 240, 500, 0);
    bitmap_place(PIC(P_EXIT), 580, 500, 0);
    bitmap_place(PIC(P_POCKET), reverse_unlocked() ? 352 : 227, 288, 0);
    bitmap_place(PIC(P_REVERSE), pocket_unlocked() ? 72 : 202, 293, 0);
    bitmap_slide(PIC(P_TITLE), 0, 0, 1.0f, 1, 0);
    y = pocket_unlocked() || reverse_unlocked() ? 116 : 177;
    bitmap_slide(PIC(P_SOLO), 56, y, 1.0f, 1, 0);
    bitmap_slide(PIC(P_LINK), 363, y, 1.0f, 1, 0);
    g_rise_started = 0;
    g_voice_played = 0;
    g_go_at = 0xffffffffu;
    menu_clear_buttons();
    /* a 2x2 grid: Solo | Link over Options | Exit */
    menu_add_button(PIC(P_SOLO), PIC(P_SOLO_HL), 0xff0201ffu, -1, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_LINK), PIC(P_LINK_HL), 0xff03ff00u, -1, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_OPTIONS), 0, 0x00ff03ffu, -1, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_EXIT), 0, 0x01ffff02u, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
    g_reverse_button = g_pocket_button = -2;
    if (pocket_unlocked()) {
        menu_add_button(PIC(P_POCKET), PIC(P_POCKET_HL), 0xffffffffu, -1, -1, 0, 0, 0, -1, -1, 0);
        g_pocket_button = 4;
        next = 5;
    }
    if (reverse_unlocked()) {
        menu_add_button(PIC(P_REVERSE), PIC(P_REVERSE_HL), 0xffffffffu, -1, -1, 0, 0, 0, -1, -1, 0);
        g_reverse_button = next;
    }
    /* the championship of this difficulty (Data\champA.spt for easy, B, C): when it has
       been read, the music goes on and "make your choice" is said after 600 ms */
    if (champ_load()) {
        music_play(3, 3);
        sound_queue(1, 600, 0x57);
    }
}

/* a switch's two labels: "off" and "on" pictures side by side */
static void draw_switch(int on, int x1, int x2, int y)
{
    Bitmap *a = PIC(on ? P_ON : P_OFF), *b = PIC(on ? P_ON2 : P_OFF2);
    bitmap_place(a, x1, y, 0);
    bitmap_place(b, x2, y, 0);
    bitmap_step(a, 0, 0);
    bitmap_draw(a, BM_OWN, 100, 0);
    bitmap_step(b, 0, 0);
    bitmap_draw(b, BM_OWN, 100, 0);
}

void page_main_frame(void)
{
    int pressed, page = -1, solo = 0;
    menu_begin_frame();
    if (!PIC(P_SOLO)->sliding && !g_rise_started) {
        bitmap_slide(PIC(P_OPTIONS), 240, 420, 0.7f, 1, 0);
        bitmap_slide(PIC(P_EXIT), 510, 430, 0.7f, 1, 0);
        g_rise_started = 1;
    }
    menu_hit();
    bitmap_step(PIC(P_TILE), 1, 1);
    bitmap_draw(PIC(P_TILE), BM_OWN, 100, 0);
    bitmap_step(PIC(P_TITLE), 0, 0);
    bitmap_draw(PIC(P_TITLE), BM_OWN, 100, 0);
    menu_draw_buttons();
    if (pocket_unlocked()) {
        if (reverse_unlocked())
            draw_switch(g_pocket, 373, 453, 362);
        else
            draw_switch(g_pocket, 248, 328, 362);
    }
    if (reverse_unlocked()) {
        if (pocket_unlocked())
            draw_switch(g_reverse, 120, 200, 363);
        else
            draw_switch(g_reverse, 250, 330, 363);
    }
    menu_transition();
    r3d_render();
    menu_end_frame();

    /* Solo and Link say their name first and go on when it has been said (1 s); with no
       sound they go on at once */
    pressed = menu_pressed();
    if (pressed == 0 && !g_voice_played && sound_play(0x55, 0) != -1) {
        g_voice_played = 1;
        g_go_at = (unsigned)(int)((double)g_now + 1000.0);
    }
    if (pressed == 1 && !g_voice_played && sound_play(0x56, 0) != -1) {
        g_voice_played = 1;
        g_go_at = (unsigned)(int)((double)g_now + 1000.0);
    }
    if (g_go_at != 0xffffffffu && g_now <= g_go_at)
        return;
    switch (pressed) {
    case 0:
        solo = 1;
        menu_press_done();
        g_solo = 1;
        break;
    case 1:
        /* The game goes to page 2, its network games; the port has none (no network, by
           decision). The button says its name, as any press does, and stays. */
        menu_press_done();
        g_voice_played = 0;
        g_go_at = 0xffffffffu;
        break;
    case 2:
        menu_press_done();
        menu_capture();
        page = 11;
        break;
    case 3:
        menu_press_done();
        page = 4;
        break;
    }
    if (page >= 0) {
        menu_change_page(page);
        return;
    }
    if (pressed == g_pocket_button) {
        menu_press_done();
        g_pocket ^= 1;
    }
    if (pressed == g_reverse_button) {
        menu_press_done();
        g_reverse ^= 1;
    }
    if (solo) {
        /* the glass, then the game asks for its CD and checks it (0x442740: the length
           of the CD's track 2, a copy protection; page 26 when it fails). The player's
           copy is taken as it is: page 5. */
        menu_capture();
        menu_change_page(5);
    }
}

/* page 4: the menu torn down and the window closed (0x40bc40) */
void page_quit_frame(void)
{
    g_quit = 1;
}
