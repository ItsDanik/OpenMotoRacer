/* Keys to the original's window from inside wine, for when the hidden display won't give
   the window the keyboard (at 320x240): keypost VK [VK...], each down and up.
   i686-w64-mingw32-gcc -o build/keypost.exe tools/win/keypost.c */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HWND g_game;

static BOOL CALLBACK one(HWND w, LPARAM l)
{
    char s[64];
    (void)l;
    if (GetWindowTextA(w, s, sizeof s) && strncmp(s, "Moto Racer", 10) == 0) {
        g_game = w;
        return FALSE;
    }
    return TRUE;
}

int main(int argc, char **argv)
{
    int i;
    EnumWindows(one, 0);
    if (!g_game) {
        puts("no window");
        return 1;
    }
    for (i = 1; i < argc; i++) {
        int vk = (int)strtol(argv[i], 0, 0);
        PostMessageA(g_game, WM_KEYDOWN, (WPARAM)vk, 1);
        Sleep(60);
        PostMessageA(g_game, WM_KEYUP, (WPARAM)vk, 0xc0000001);
        Sleep(60);
    }
    return 0;
}
