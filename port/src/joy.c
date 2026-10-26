/* The joysticks (NOTES §4.22): detection 0x4189c0, reading 0x418ba0 / 0x418da0, the
   rescale past the dead zone 0x418e40 / 0x418f30, the directions 0x418d20, page 12's
   press 0x418c60, the race's bits 0x411b30 and the dialogs' arrows 0x411a10. */
#include "joy.h"
#include "config.h"
#include <SDL.h>

Joy g_joy[JOY_MAX];
int g_njoy;
int g_joy_dead = -1;

static SDL_Joystick *g_dev[JOY_MAX];
static int g_fake_on, g_fake_x, g_fake_y;
static unsigned g_fake_b;

/* joyGetPosEx: the raw position and buttons; 0 when the joystick does not answer */
static int pos(int j, int *x, int *y, unsigned *buttons)
{
    SDL_Joystick *d;
    int i, n;
    if (j == 0 && g_fake_on) {
        *x = g_fake_x, *y = g_fake_y, *buttons = g_fake_b;
        return 1;
    }
    d = j >= 0 && j < JOY_MAX ? g_dev[j] : 0;
    if (!d || !SDL_JoystickGetAttached(d))
        return 0;
    SDL_JoystickUpdate();
    *x = SDL_JoystickNumAxes(d) > 0 ? SDL_JoystickGetAxis(d, 0) + 0x8000 : 0x8000;
    *y = SDL_JoystickNumAxes(d) > 1 ? SDL_JoystickGetAxis(d, 1) + 0x8000 : 0x8000;
    if (SDL_JoystickNumHats(d) > 0) {
        Uint8 h = SDL_JoystickGetHat(d, 0);
        if (h & SDL_HAT_LEFT)
            *x = 0;
        else if (h & SDL_HAT_RIGHT)
            *x = 0xffff;
        if (h & SDL_HAT_UP)
            *y = 0;
        else if (h & SDL_HAT_DOWN)
            *y = 0xffff;
    }
    *buttons = 0;
    n = SDL_JoystickNumButtons(d);
    for (i = 0; i < n && i < 32; i++)
        if (SDL_JoystickGetButton(d, i))
            *buttons |= 1u << i;
    return 1;
}

static int nbuttons(int j)
{
    if (j == 0 && g_fake_on)
        return 32;
    if (!g_dev[j])
        return 0;
    return SDL_JoystickNumButtons(g_dev[j]) > 32 ? 32 : SDL_JoystickNumButtons(g_dev[j]);
}

void joy_close(void)
{
    int j;
    for (j = 0; j < JOY_MAX; j++)
        if (g_dev[j]) {
            SDL_JoystickClose(g_dev[j]);
            g_dev[j] = 0;
        }
}

void joy_detect(void)
{
    int j, i, dead = g_joy_dead < 0 ? 0x800 : g_joy_dead;
    joy_close();
    for (j = 0; j < JOY_MAX; j++)
        g_joy[j].present = g_joy[j].working = 0;
    if (!SDL_WasInit(SDL_INIT_JOYSTICK))
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    g_njoy = SDL_WasInit(SDL_INIT_JOYSTICK) ? SDL_NumJoysticks() : 0;
    if (g_njoy > JOY_MAX)
        g_njoy = JOY_MAX;
    for (j = 0; j < g_njoy; j++)
        g_dev[j] = SDL_JoystickOpen(j);
    if (g_fake_on && g_njoy < 1)
        g_njoy = 1;
    for (j = 0; j < g_njoy; j++) {
        Joy *J = &g_joy[j];
        int x, y;
        unsigned b;
        if (!pos(j, &x, &y, &b))
            continue;
        J->present = J->working = 1;
        for (i = 0; i < 3; i++)
            J->centre[i] = 0x8000, J->dead[i] = dead;
        J->nbuttons = nbuttons(j);
    }
}

int joy_known(int j)
{
    if (j < 0 || j >= JOY_MAX)
        return 0;
    if (!g_joy[j].working)
        joy_detect();
    return g_joy[j].working;
}

/* 0x418e40: past the dead zone the rest of the travel spread over the whole half */
static int rescale(int v, int c, int dz, int lo, int hi)
{
    if (v <= c + dz && v >= c - dz)
        return c;
    if (v > c)
        return c + (int)((double)(v - c - dz) / (double)(unsigned)(hi - c - dz)
                         * (double)(unsigned)(hi - c));
    return lo + (int)((double)(v - lo) / (double)(unsigned)(c - lo - dz)
                      * (double)(unsigned)(c - lo));
}

int joy_read(int j)
{
    Joy *J;
    int x, y;
    unsigned b;
    if (j != 0 && j != 1)
        return 0;
    J = &g_joy[j];
    J->dir = 0;
    if (!pos(j, &x, &y, &b))
        return 0;
    J->buttons = b;
    J->x = rescale(x, J->centre[0], J->dead[0], 0, 0xffff);
    J->y = rescale(y, J->centre[1], J->dead[1], 0, 0xffff);
    if (J->y < J->centre[1] - J->dead[1])      /* 0x418d20 */
        J->dir = 1;
    else if (J->y > J->centre[1] + J->dead[1])
        J->dir = 2;
    if (J->x < J->centre[0] - J->dead[0])
        J->dir |= 4;
    else if (J->x > J->centre[0] + J->dead[0])
        J->dir |= 8;
    return 1;
}

int joy_code(int j)
{
    int x, y, i;
    unsigned b;
    if ((j != 0 && j != 1) || !pos(j, &x, &y, &b))
        return -1;
    g_joy[j].buttons = b;
    for (i = 0; i < g_joy[j].nbuttons; i++, b >>= 1)
        if (b & 1)
            return i;
    if (x < 0x2000)
        return -2;
    if (x > 0xdfff)
        return -3;
    if (y < 0x2000)
        return -4;
    return y < 0xe000 ? -1 : -5;
}

/* 0x431190: the action a button or direction is bound to */
static int action_of(int code)
{
    int i;
    for (i = 0; i < ACTIONS; i++)
        if (g_binding[i][1] == code)
            return i;
    return -1;
}

unsigned joy_race(int j, int *x, int *y, int *turbo, int countdown, int running, int device)
{
    Joy *J = &g_joy[j];
    unsigned bits = 0, d, b;
    int i;
    *turbo = 0;
    if (!joy_read(j)) {
        *x = *y = 0x8000;
        return 0;
    }
    d = J->dir;
    if (d & 1)                  /* up */
        switch (action_of(-4)) {
        case 0: bits = 1; break;
        case 1: bits = 2; J->y = 0xffff - J->y; break;
        case 4: if (!countdown && running) bits = 0x10; break;
        case 5: if (!countdown && running) bits = 0x20; break;
        case 6: if (running) *turbo = 1; break;
        }
    if (d & 2)                  /* down */
        switch (action_of(-5)) {
        case 0: bits |= 1; J->y = 0xffff - J->y; break;
        case 1: bits |= 2; break;
        case 4: if (running) bits |= 0x10; break;
        case 5: if (running) bits |= 0x20; break;
        case 6: if (running) *turbo = 1; break;
        }
    if (d & 4)                  /* left */
        switch (action_of(-2)) {
        case 2: bits |= 4; break;
        case 3: bits |= 8; J->x = 0xffff - J->x; break;
        }
    if (d & 8)                  /* right */
        switch (action_of(-3)) {
        case 2: bits |= 4; J->x = 0xffff - J->x; break;
        case 3: bits |= 8; break;
        }
    b = J->buttons;
    for (i = 0; i < J->nbuttons && b; i++, b >>= 1) {
        if (b & 1)
            switch (action_of(i)) {
            case 0: J->y = 0x8000; bits |= 1; break;
            case 1: if (running) J->y = 0x8000, bits |= 2; break;
            case 2: if (running) J->x = 0x8000, bits |= 4; break;
            case 3: if (running) J->x = 0x8000, bits |= 8; break;
            case 4: if (!countdown && running) bits |= 0x10; break;
            case 5: if (!countdown && running) bits |= 0x20; break;
            case 6: if (running) *turbo = 1; break;
            }
        if (bits & 2)           /* braking lets go of the throttle */
            bits &= ~1u;
    }
    if (device == 2) {          /* the throttle on a button: the stick's Y half is ignored */
        if (g_binding[0][1] >= 0 && J->y < 0x8000)
            J->y = 0x8000;
        if (g_binding[1][1] >= 0 && J->y > 0x8000)
            J->y = 0x8000;
    } else                      /* a joypad: the bits alone */
        J->x = J->y = 0x8000;
    *x = J->x, *y = J->y;
    return bits;
}

void joy_menu(void (*key)(int vk, int down))
{
    static unsigned last_dir, last_b;          /* 0x4dbe30, 0x4dbe34 */
    static const int vk[4] = { 0x26, 0x28, 0x25, 0x27 };
    unsigned ch;
    int i;
    if (!joy_read(0))
        return;
    ch = last_dir ^ g_joy[0].dir;
    for (i = 0; i < 4; i++)
        if (ch & (1u << i))
            key(vk[i], (g_joy[0].dir >> i) & 1);
    if ((last_b ^ g_joy[0].buttons) & 1)
        key(0x0d, g_joy[0].buttons & 1);
    last_dir = g_joy[0].dir;
    last_b = g_joy[0].buttons;
}

void joy_fake(int x, int y, unsigned buttons)
{
    g_fake_on = 1;
    g_fake_x = x, g_fake_y = y, g_fake_b = buttons;
}
