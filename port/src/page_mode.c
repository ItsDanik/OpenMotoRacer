/* Page 5, SELECT MODE (start 0x447250, frame 0x447710; page 26 is the same page, reached
   when the main menu's CD check fails): Practice comes in from the left, Single Race rises
   from below, Championship comes in from the right. (In the GP edition, 0x4dc1c4, the
   Grdprix pictures replace Championship's: not this copy.) */
#include "choices.h"
#include "menu.h"
#include "pages.h"
#include "r3d.h"
#include "sound.h"

#define PIC(id) (&g_pics[id])
enum {
    P_TILE = 0, P_TITLE = 8, P_PRACTICE = 9, P_PRACTICE_HL = 0x0a, P_SINGLE = 0x0b,
    P_SINGLE_HL = 0x0c, P_CHAMP = 0x0d, P_CHAMP_HL = 0x0e, P_BACK = 0x51
};

void page_mode_start(void)
{
    bitmap_place(PIC(P_PRACTICE), -242, 115, 0);
    bitmap_place(PIC(P_SINGLE), 229, 595, 0);
    bitmap_place(PIC(P_CHAMP), 700, 115, 0);
    bitmap_place(PIC(P_BACK), 270, 430, 0);
    menu_clear_buttons();
    /* (Link, out of the port's scope, has Single and Championship only) */
    menu_add_button(PIC(P_BACK), 0, 0x02ffffffu, VK_ESCAPE, -1, 0, 0, 0, -1, -1, 0);
    bitmap_slide(PIC(P_PRACTICE), 18, 115, 1.0f, 1, 0);
    bitmap_slide(PIC(P_SINGLE), 229, 115, 1.0f, 1, 0);
    bitmap_slide(PIC(P_CHAMP), 440, 115, 1.0f, 1, 0);
    /* the three cards side by side, Back under them */
    menu_add_button(PIC(P_PRACTICE), PIC(P_PRACTICE_HL), 0xff0002ffu, -1, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_SINGLE), PIC(P_SINGLE_HL), 0xff000301u, -1, -1, 0, 0, 0, -1, -1, 0);
    menu_add_button(PIC(P_CHAMP), PIC(P_CHAMP_HL), 0xff00ff02u, -1, -1, 0, 0, 0, -1, -1, 0);
    bitmap_place(PIC(P_TITLE), 0, -80, 0);
    bitmap_slide(PIC(P_TITLE), 0, 0, 1.0f, 1, 0);
    music_play(3, 3);
    sound_queue(1, 600, 0x54);          /* "select mode" */
    g_champ_shown = 0;
}

void page_mode_frame(void)
{
    int pressed;
    menu_begin_frame();
    bitmap_step(PIC(P_TILE), -1, 1);
    bitmap_draw(PIC(P_TILE), BM_OWN, 100, 0);
    bitmap_step(PIC(P_TITLE), 0, 0);
    bitmap_draw(PIC(P_TITLE), BM_OWN, 100, 0);
    menu_draw_buttons();
    menu_transition();
    r3d_render();
    menu_end_frame();

    pressed = menu_pressed();
    if (pressed >= 1 && pressed <= 3) {
        menu_press_done();
        g_mode = pressed - 1;
        menu_capture();
        if (g_mode != MODE_PRACTICE && g_mode != MODE_SINGLE) {
            /* a new championship: from its first race, as the list has it */
            g_reverse = 0;
            g_pocket = 0;
            g_track = 0;
            g_champ_race = 0;
        }
        menu_change_page(6);
    } else if (pressed == 0) {
        menu_press_done();
        menu_capture();
        menu_change_page(3);
    }
}
