/* a do-nothing SDL for --shot runs under clang's MemorySanitizer, which cannot follow the real
   library's memory */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int SDL_Init(Uint32 f) { (void)f; return 0; }
int SDL_InitSubSystem(Uint32 f) { (void)f; return 0; }
Uint32 SDL_WasInit(Uint32 f) { (void)f; return 0; }
void SDL_Quit(void) {}
const char *SDL_GetError(void) { return "stub"; }
int SDL_setenv(const char *n, const char *v, int o) { return setenv(n, v, o); }
int SDL_ShowCursor(int t) { (void)t; return 0; }
int SDL_PollEvent(SDL_Event *e) { (void)e; return 0; }
Uint64 SDL_GetPerformanceCounter(void) { return 0; }
Uint64 SDL_GetPerformanceFrequency(void) { return 1000; }
int SDL_GetSystemRAM(void) { return 1024; }
char *SDL_GetBasePath(void) { return strdup("./"); }
void SDL_free(void *p) { free(p); }
int SDL_ShowSimpleMessageBox(Uint32 f, const char *t, const char *m, SDL_Window *w) { (void)f, (void)t, (void)m, (void)w; return 0; }
SDL_Window *SDL_CreateWindow(const char *t, int x, int y, int w, int h, Uint32 f) { (void)t, (void)x, (void)y, (void)w, (void)h, (void)f; return 0; }
SDL_Renderer *SDL_CreateRenderer(SDL_Window *w, int i, Uint32 f) { (void)w, (void)i, (void)f; return 0; }
SDL_Texture *SDL_CreateTexture(SDL_Renderer *r, Uint32 f, int a, int w, int h) { (void)r, (void)f, (void)a, (void)w, (void)h; return 0; }
void SDL_DestroyRenderer(SDL_Renderer *r) { (void)r; }
void SDL_DestroyTexture(SDL_Texture *r) { (void)r; }
void SDL_DestroyWindow(SDL_Window *r) { (void)r; }
int SDL_RenderClear(SDL_Renderer *r) { (void)r; return 0; }
int SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *a, const SDL_Rect *b) { (void)r, (void)t, (void)a, (void)b; return 0; }
void SDL_RenderPresent(SDL_Renderer *r) { (void)r; }
int SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h) { (void)r, (void)w, (void)h; return 0; }
int SDL_SetWindowFullscreen(SDL_Window *w, Uint32 f) { (void)w, (void)f; return 0; }
int SDL_UpdateTexture(SDL_Texture *t, const SDL_Rect *r, const void *p, int pitch) { (void)t, (void)r, (void)p, (void)pitch; return 0; }
SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *p, int w, int h, int d, int pitch, Uint32 f) {
    (void)d, (void)f;
    /* every pixel of the finished picture must be defined */
    int y, x;
    unsigned sum = 0;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            sum += ((unsigned short *)p)[y * pitch / 2 + x];
    if (sum == 0x12345678)
        fprintf(stderr, "sum\n");
    return calloc(1, sizeof(SDL_Surface));
}
void SDL_FreeSurface(SDL_Surface *s) { free(s); }
SDL_RWops *SDL_RWFromFile(const char *f, const char *m) { (void)f, (void)m; return calloc(1, sizeof(SDL_RWops)); }
int SDL_SaveBMP_RW(SDL_Surface *s, SDL_RWops *r, int c) { (void)s; if (c) free(r); return 0; }
int SDL_NumJoysticks(void) { return 0; }
SDL_Joystick *SDL_JoystickOpen(int i) { (void)i; return 0; }
void SDL_JoystickClose(SDL_Joystick *j) { (void)j; }
SDL_bool SDL_JoystickGetAttached(SDL_Joystick *j) { (void)j; return SDL_FALSE; }
Sint16 SDL_JoystickGetAxis(SDL_Joystick *j, int a) { (void)j, (void)a; return 0; }
Uint8 SDL_JoystickGetButton(SDL_Joystick *j, int a) { (void)j, (void)a; return 0; }
Uint8 SDL_JoystickGetHat(SDL_Joystick *j, int a) { (void)j, (void)a; return 0; }
int SDL_JoystickNumAxes(SDL_Joystick *j) { (void)j; return 0; }
int SDL_JoystickNumButtons(SDL_Joystick *j) { (void)j; return 0; }
int SDL_JoystickNumHats(SDL_Joystick *j) { (void)j; return 0; }
void SDL_JoystickUpdate(void) {}
void SDL_LockAudioDevice(SDL_AudioDeviceID d) { (void)d; }
void SDL_UnlockAudioDevice(SDL_AudioDeviceID d) { (void)d; }
SDL_AudioDeviceID SDL_OpenAudioDevice(const char *n, int c, const SDL_AudioSpec *w, SDL_AudioSpec *o, int a) { (void)n, (void)c, (void)w, (void)o, (void)a; return 0; }
void SDL_PauseAudioDevice(SDL_AudioDeviceID d, int p) { (void)d, (void)p; }
void SDL_CloseAudioDevice(SDL_AudioDeviceID d) { (void)d; }
