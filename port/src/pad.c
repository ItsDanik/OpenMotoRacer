/* A gamepad in the menus and the race's dialogs: the cross moves between the choices as the
   arrow keys do, the left stick moves the pointer, A presses, B and Back go back; Start
   presses in a menu, and in a race pauses and goes on. Everything goes through the routes the
   keyboard and the mouse take. */
#include "pad.h"
#include "clock.h"
#include "dialog.h"
#include "input.h"
#include "joy.h"
#include "menu.h"
#include "pages.h"
#include "race.h"
#include "video.h"

#define REPEAT_FIRST 350        /* ms: the cross held, as a held key repeats */
#define REPEAT_NEXT 110
#define POINTER_SPEED 800.0     /* pixels a second, the stick right over (640 wide) */

int g_pad_listen;

static unsigned g_last, g_last_cross;
static int g_quiet;
static int g_repeat_vk;
static unsigned g_repeat_at, g_last_now;
static int g_a_mouse, g_a_key, g_on_pointer;
static double g_px, g_py;

static void key(int vk, int down)
{
    input_key(vk, down);
    race_key(vk, down);
    if (down) {
        menu_key(vk);
        if (vk == VK_RETURN)
            g_menu_char = 13;
    } else
        menu_key_up(vk);
}

static void tap(int vk)
{
    key(vk, 1);
    key(vk, 0);
}

static void mouse_move(int x, int y)
{
    menu_mouse_move(x, y);
    dialog_mouse_move(x, y);
}

static void mouse_left(int down)
{
    menu_mouse_button(1, down);
    dialog_mouse_button(1, down);
}

/* A: a click when the pointer is on something, else Return */
static void press(int down)
{
    if (down) {
        int on = g_racing ? g_on_pointer : g_menu.mouse_mode && g_menu.sel != -1;
        if (on) {
            g_a_mouse = 1;
            mouse_left(1);
        } else {
            /* the name page takes the letter under its cursor from Space */
            g_a_key = !g_racing && page_name_picks() ? VK_SPACE : VK_RETURN;
            key(g_a_key, 1);
        }
    } else if (g_a_mouse) {
        g_a_mouse = 0;
        mouse_left(0);
    } else if (g_a_key) {
        key(g_a_key, 0);
        g_a_key = 0;
    }
}

static void arrow(int vk)
{
    if (page_top5_typing())
        page_top5_letter(vk);
    else
        tap(vk);
}

static void cross(unsigned now)
{
    static const int vk[4] = { VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT };
    unsigned hit = now & ~g_last_cross;
    int i;
    for (i = 0; i < 4; i++)
        if (hit & (1u << i)) {
            arrow(vk[i]);
            g_on_pointer = 0;
            g_repeat_vk = vk[i];
            g_repeat_at = g_now + REPEAT_FIRST;
        }
    if (!now)
        g_repeat_vk = 0;
    else if (g_repeat_vk && g_now >= g_repeat_at) {
        arrow(g_repeat_vk);
        g_repeat_at = g_now + REPEAT_NEXT;
    }
}

/* the left stick moves the pointer: slowly near the middle, for the small buttons */
static void pointer(const Joy *J, unsigned ms)
{
    double dx = (J->stick[0] - 0x8000) / 32768.0, dy = (J->stick[1] - 0x8000) / 32768.0;
    double k = POINTER_SPEED * ms / 1000.0;
    if (dx == 0.0 && dy == 0.0) {
        g_px = g_mx, g_py = g_my;
        return;
    }
    g_px += dx * (dx < 0 ? -dx : dx) * k;
    g_py += dy * (dy < 0 ? -dy : dy) * k;
    g_px = g_px < 0 ? 0 : g_px > g_scr_w - 1 ? g_scr_w - 1 : g_px;
    g_py = g_py < 0 ? 0 : g_py > g_scr_h - 1 ? g_scr_h - 1 : g_py;
    if ((int)g_px != g_mx || (int)g_py != g_my) {
        mouse_move((int)g_px, (int)g_py);
        g_on_pointer = 1;
    }
}

void pad_quiet(void)
{
    g_quiet = 1;
}

void pad_frame(void)
{
    Joy *J = &g_joy[0];
    unsigned b, hit, ms = g_now - g_last_now;
    int menus, resting;
    g_last_now = g_now;
    if (!joy_is_pad(0) || !joy_read(0))
        return;
    b = J->buttons;
    hit = b & ~g_last;
    resting = !(b & ~(1u << PAD_LT | 1u << PAD_RT)) && !J->cross && J->stick[0] == 0x8000
              && J->stick[1] == 0x8000;
    if (g_pad_listen) {         /* Start: not this time */
        if (hit & 1u << PAD_START)
            g_pad_listen = 0, g_quiet = 1;
    } else if (g_quiet)
        g_quiet = !resting;
    else {
        menus = !g_racing || dialog_holds() || dialog_open();   /* (a demo goes on under its own) */
        if (hit & 1u << PAD_START)
            if (g_racing)
                tap(VK_ESCAPE);
        if (g_racing && !menus && g_playback && hit & (1u << PAD_A | 1u << PAD_B))
            tap(VK_ESCAPE);     /* a demo gives way, as to a key */
        if (menus) {
            if ((hit & 1u << PAD_A) || (!g_racing && (hit & 1u << PAD_START)))
                press(1);
            if (hit & (1u << PAD_B | 1u << PAD_BACK))
                tap(VK_ESCAPE);
            cross(J->cross);
            pointer(J, ms);
        }
        if (!(b & (1u << PAD_A | 1u << PAD_START)))
            press(0);
    }
    g_last = b;
    g_last_cross = J->cross;
}
