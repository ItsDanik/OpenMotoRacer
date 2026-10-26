/* Sample pieces of the running original's memory at a steady rate (under wine, as memtool).
     memwatch PERIOD_MS COUNT FILE ADDR:LEN [ADDR:LEN ...]
   Every PERIOD_MS, COUNT times: a record of u32 timeGetTime, then each piece's LEN bytes, in
   the order given, appended to FILE. A record whose bytes equal the previous one's is still
   written (the reader can drop repeats). Addresses and lengths in hex (0x..) or decimal.
   Build: i686-w64-mingw32-gcc -O2 -o build/memwatch.exe tools/win/memwatch.c -lwinmm */
#include <windows.h>
#include <tlhelp32.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HANDLE open_game(void)
{
    PROCESSENTRY32 pe = { sizeof pe };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snap, &pe))
        do {
            if (!_stricmp(pe.szExeFile, "game.exe")) {
                CloseHandle(snap);
                return OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
            }
        } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: memwatch PERIOD_MS COUNT FILE ADDR:LEN [ADDR:LEN ...]\n");
        return 2;
    }
    HANDLE h = open_game();
    if (!h) { fprintf(stderr, "game.exe not running\n"); return 1; }
    DWORD period = strtoul(argv[1], 0, 0), count = strtoul(argv[2], 0, 0);
    int n = argc - 4;
    DWORD *addr = malloc(n * sizeof *addr), *len = malloc(n * sizeof *len), total = 0;
    for (int i = 0; i < n; i++) {
        char *c = strchr(argv[4 + i], ':');
        if (!c) { fprintf(stderr, "bad piece %s\n", argv[4 + i]); return 2; }
        addr[i] = strtoul(argv[4 + i], 0, 0);
        len[i] = strtoul(c + 1, 0, 0);
        total += len[i];
    }
    unsigned char *buf = malloc(4 + total);
    FILE *f = fopen(argv[3], "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", argv[3]); return 1; }
    timeBeginPeriod(1);
    DWORD next = timeGetTime();
    for (DWORD k = 0; k < count; k++) {
        DWORD now = timeGetTime();
        memcpy(buf, &now, 4);
        DWORD at = 4;
        for (int i = 0; i < n; i++) {
            SIZE_T got = 0;
            if (!ReadProcessMemory(h, (void *)(UINT_PTR)addr[i], buf + at, len[i], &got) || got != len[i])
                memset(buf + at, 0xee, len[i]);
            at += len[i];
        }
        fwrite(buf, 1, 4 + total, f);
        next += period;
        now = timeGetTime();
        if ((int)(next - now) > 0) Sleep(next - now);
    }
    fclose(f);
    timeEndPeriod(1);
    return 0;
}
