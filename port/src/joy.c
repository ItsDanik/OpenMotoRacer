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
static SDL_GameController *g_ctl[JOY_MAX];
static int g_fake_on, g_fake_x, g_fake_y, g_fake_pad, g_fake_t[2];
static unsigned g_fake_b, g_fake_cross;

/* a trigger counts as a held button past this; with analog on it starts sooner */
#define TRIGGER_DIGITAL 0x2000
#define TRIGGER_ANALOG 0x400
#define PAD_DEAD 0x1800         /* the sticks of pads rest further from their centre */

static int trigger_on(void)
{
    return g_analog ? TRIGGER_ANALOG : TRIGGER_DIGITAL;
}

/* a gamepad, through SDL's table of layouts: the sticks and triggers by name */
static void pad_pos(int j, int *x, int *y, unsigned *buttons)
{
    static const SDL_GameControllerButton map[PAD_LT] = {
        SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X,
        SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_BACK,
        SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_GUIDE,
        SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK
    };
    SDL_GameController *c = g_ctl[j];
    int i, lt, rt;
    SDL_GameControllerUpdate();
    *x = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX) + 0x8000;
    *y = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY) + 0x8000;
    *buttons = 0;
    for (i = 0; i < PAD_LT; i++)
        if (SDL_GameControllerGetButton(c, map[i]))
            *buttons |= 1u << i;
    lt = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    rt = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    g_joy[j].trigger[0] = lt < 0 ? 0 : lt;
    g_joy[j].trigger[1] = rt < 0 ? 0 : rt;
    g_joy[j].cross = (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_UP) ? 1u : 0)
                     | (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ? 2u : 0)
                     | (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ? 4u : 0)
                     | (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ? 8u : 0);
}

/* a joystick SDL has no layout for: axis 0 and 1, the first hat pushing them to the ends */
static void raw_pos(SDL_Joystick *d, int *x, int *y, unsigned *buttons)
{
    int i, n;
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
}

/* joyGetPosEx: the raw position and buttons; 0 when the joystick does not answer */
static int pos(int j, int *x, int *y, unsigned *buttons)
{
    if (j == 0 && g_fake_on) {
        *x = g_fake_x, *y = g_fake_y, *buttons = g_fake_b;
        if (g_fake_pad) {
            g_joy[0].trigger[0] = g_fake_t[0], g_joy[0].trigger[1] = g_fake_t[1];
            g_joy[0].cross = g_fake_cross;
        }
    } else {
        SDL_Joystick *d = j >= 0 && j < JOY_MAX ? g_dev[j] : 0;
        if (!d || !SDL_JoystickGetAttached(d))
            return 0;
        if (g_ctl[j])
            pad_pos(j, x, y, buttons);
        else
            raw_pos(d, x, y, buttons);
    }
    if (joy_is_pad(j)) {        /* the triggers as buttons; the stick kept without the cross */
        Joy *J = &g_joy[j];
        if (J->trigger[0] >= trigger_on())
            *buttons |= 1u << PAD_LT;
        if (J->trigger[1] >= trigger_on())
            *buttons |= 1u << PAD_RT;
        J->stick[0] = *x, J->stick[1] = *y;
        if (J->cross & 1)
            *y = 0;
        else if (J->cross & 2)
            *y = 0xffff;
        if (J->cross & 4)
            *x = 0;
        else if (J->cross & 8)
            *x = 0xffff;
    }
    return 1;
}

int joy_is_pad(int j)
{
    if (j == 0 && g_fake_on)
        return g_fake_pad;
    return j >= 0 && j < JOY_MAX && g_ctl[j];
}

const char *joy_button_name(int code)
{
    static const char *const name[PAD_BUTTONS] = {
        "A", "B", "X", "Y", "LB", "RB", "Back", "Start", "Guide", "L3", "R3", "LT", "RT"
    };
    return code >= 0 && code < PAD_BUTTONS ? name[code] : "?";
}

void joy_rumble(int low, int high, int ms)
{
    if (g_dev[0])
        SDL_JoystickRumble(g_dev[0], (Uint16)low, (Uint16)high, (Uint32)ms);
}

static int nbuttons(int j)
{
    if (joy_is_pad(j))
        return PAD_BUTTONS;
    if (j == 0 && g_fake_on)
        return 32;
    if (!g_dev[j])
        return 0;
    return SDL_JoystickNumButtons(g_dev[j]) > 32 ? 32 : SDL_JoystickNumButtons(g_dev[j]);
}

void joy_close(void)
{
    int j;
    for (j = 0; j < JOY_MAX; j++) {
        if (g_ctl[j])
            SDL_GameControllerClose(g_ctl[j]);
        else if (g_dev[j])
            SDL_JoystickClose(g_dev[j]);
        g_ctl[j] = 0;
        g_dev[j] = 0;
    }
}

void joy_detect(void)
{
    int j, i, n, dead;
    joy_close();
    for (j = 0; j < JOY_MAX; j++)
        g_joy[j].present = g_joy[j].working = 0;
    n = SDL_WasInit(SDL_INIT_GAMECONTROLLER) ? SDL_NumJoysticks() : 0;
    /* the gamepads first: a keyboard's or a mouse's extra keys can come before them in the
       system's list, as joysticks that do nothing */
    g_njoy = 0;
    for (i = 0; i < n && g_njoy < JOY_MAX; i++)
        if (SDL_IsGameController(i) && (g_ctl[g_njoy] = SDL_GameControllerOpen(i))) {
            g_dev[g_njoy] = SDL_GameControllerGetJoystick(g_ctl[g_njoy]);
            g_njoy++;
        }
    for (i = 0; i < n && g_njoy < JOY_MAX; i++)
        if (!SDL_IsGameController(i))
            g_dev[g_njoy++] = SDL_JoystickOpen(i);
    if (g_fake_on && g_njoy < 1)
        g_njoy = 1;
    for (j = 0; j < g_njoy; j++) {
        Joy *J = &g_joy[j];
        int x, y;
        unsigned b;
        if (!pos(j, &x, &y, &b))
            continue;
        J->present = J->working = 1;
        J->pad = joy_is_pad(j);
        dead = g_joy_dead >= 0 ? g_joy_dead : J->pad ? PAD_DEAD : 0x800;
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
    if (joy_is_pad(j)) {
        J->stick[0] = rescale(J->stick[0], J->centre[0], J->dead[0], 0, 0xffff);
        J->stick[1] = rescale(J->stick[1], J->centre[1], J->dead[1], 0, 0xffff);
    }
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
    if (joy_is_pad(j)) {        /* these work the menus; a trigger takes a firm press */
        b &= ~(1u << PAD_BACK | 1u << PAD_START | 1u << PAD_GUIDE);
        if (g_joy[j].trigger[0] < TRIGGER_DIGITAL)
            b &= ~(1u << PAD_LT);
        if (g_joy[j].trigger[1] < TRIGGER_DIGITAL)
            b &= ~(1u << PAD_RT);
    }
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

/* how far the trigger an action is bound to is pressed; -1 when it is bound to none */
static int trigger_of(int j, int action)
{
    int code = g_binding[action][1];
    if (code != PAD_LT && code != PAD_RT)
        return -1;
    return g_joy[j].trigger[code - PAD_LT];
}

static unsigned trigger_scale(int v)
{
    int t = v * 255 / 0x7fff;
    return (unsigned)(t < 1 ? 1 : t > 255 ? 255 : t);
}

/* The port's: RT accelerates and LT brakes without being set, beside what the rows say, as
   long as that action is on no trigger and that trigger is on no row */
int joy_spare_trigger(int j, int action)
{
    int code = action == 0 ? PAD_RT : action == 1 ? PAD_LT : -1, i;
    if (code < 0 || !joy_is_pad(j) || g_binding[action][1] == PAD_LT
        || g_binding[action][1] == PAD_RT)
        return -1;
    for (i = 0; i < ACTIONS; i++)
        if (g_binding[i][1] == code)
            return -1;
    return code;
}

unsigned joy_race(int j, int *x, int *y, int *turbo, int countdown, int running, int device)
{
    int gas, brake;
    unsigned pressed;
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
    pressed = bits;
    gas = joy_spare_trigger(j, 0);
    brake = joy_spare_trigger(j, 1);
    if (gas >= 0 && J->trigger[gas - PAD_LT] >= trigger_on())
        J->y = 0x8000, bits |= 1;
    if (brake >= 0 && running && J->trigger[brake - PAD_LT] >= trigger_on())
        J->y = 0x8000, bits |= 2;
    if (bits & 2)
        bits &= ~1u;
    /* the original: joystick 0x4dc11c = 2. The port's: a gamepad under either name, and
       off, the bits alone whatever the name. */
    if (g_analog && (device == 2 || joy_is_pad(j))) {
        /* the throttle on a button: the stick's Y half is ignored */
        if (g_binding[0][1] >= 0 && J->y < 0x8000)
            J->y = 0x8000;
        if (g_binding[1][1] >= 0 && J->y > 0x8000)
            J->y = 0x8000;
        if (joy_is_pad(j)) {
            /* the cross held is the keys, which turn the bars by degrees; not full lock at once */
            if (J->cross & 12)
                J->x = 0x8000;
            if (J->cross & 3)
                J->y = 0x8000;
            /* how hard, in key bits nobody else reads, so that a replay keeps it: not the
               game's Y, whose half way holds the engine to half its revs, and the gearbox
               then never changes up */
            if ((bits & 2) && trigger_of(j, 1) >= 0)
                bits |= trigger_scale(trigger_of(j, 1)) << 8;
            else if ((bits & 2) && brake >= 0 && !(pressed & 2))
                bits |= trigger_scale(J->trigger[brake - PAD_LT]) << 8;
            if ((bits & 1) && trigger_of(j, 0) >= 0)
                bits |= trigger_scale(trigger_of(j, 0)) << 16;
            else if ((bits & 1) && gas >= 0 && !(pressed & 1))
                bits |= trigger_scale(J->trigger[gas - PAD_LT]) << 16;
        }
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

void joy_fake_pad(int x, int y, unsigned buttons, int lt, int rt, unsigned cross)
{
    int first = !g_fake_pad;
    joy_fake(x, y, buttons);
    g_fake_pad = 1;
    g_fake_t[0] = lt, g_fake_t[1] = rt;
    g_fake_cross = cross;
    if (first)
        joy_detect();
}
