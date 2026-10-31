/* What a gamepad sends, as the game sees it through SDL: a line each time something moves. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    int seconds = argc > 1 ? atoi(argv[1]) : 20, i, n, a;
    SDL_GameController *pad = 0;
    SDL_Joystick *d;
    Sint16 last[8];
    int last_lt = -1, last_rt = -1;
    unsigned last_b = 0;
    Uint32 end;
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        printf("SDL: %s\n", SDL_GetError());
        return 1;
    }
    n = SDL_NumJoysticks();
    printf("%d devices\n", n);
    for (i = 0; i < n; i++) {
        printf("  %d %s, %s\n", i, SDL_JoystickNameForIndex(i),
               SDL_IsGameController(i) ? "a gamepad" : "not a gamepad");
        if (SDL_IsGameController(i) && !pad)
            pad = SDL_GameControllerOpen(i);
    }
    if (!pad) {
        printf("No gamepad: wake it (press a button) and run this again.\n");
        return 1;
    }
    printf("Layout: %s\n", SDL_GameControllerMapping(pad));
    d = SDL_GameControllerGetJoystick(pad);
    memset(last, 0, sizeof last);
    printf("Pull LT slowly all the way and let go, then RT the same. %d seconds.\n", seconds);
    end = SDL_GetTicks() + (Uint32)seconds * 1000;
    while (SDL_GetTicks() < end) {
        int lt, rt, moved = 0;
        unsigned b = 0;
        SDL_GameControllerUpdate();
        lt = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        rt = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        for (i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
            if (SDL_GameControllerGetButton(pad, (SDL_GameControllerButton)i))
                b |= 1u << i;
        for (a = 0; a < SDL_JoystickNumAxes(d) && a < 8; a++) {
            Sint16 v = SDL_JoystickGetAxis(d, a);
            if (abs(v - last[a]) > 1500)
                last[a] = v, moved = 1;
        }
        if (lt != last_lt || rt != last_rt || b != last_b || moved) {
            printf("LT %5d  RT %5d  buttons %05x  axes", lt, rt, b);
            for (a = 0; a < SDL_JoystickNumAxes(d) && a < 8; a++)
                printf(" %6d", SDL_JoystickGetAxis(d, a));
            printf("\n");
            fflush(stdout);
            last_lt = lt, last_rt = rt, last_b = b;
        }
        SDL_Delay(10);
    }
    return 0;
}
