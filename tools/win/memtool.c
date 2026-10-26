/* Read and write the running original's memory under wine (the kernel's ptrace rule keeps
   /proc/<pid>/mem closed; wine's ReadProcessMemory goes through the wine server).
     memtool read  ADDR LEN FILE       LEN bytes at ADDR into FILE
     memtool deref PTR  LEN FILE       LEN bytes at the address stored at PTR
     memtool poke  ADDR HEXBYTES       write bytes
   Addresses and lengths in hex or decimal. Finds the process by the name game.exe.
   Build: i686-w64-mingw32-gcc -O2 -o build/memtool.exe tools/win/memtool.c */
#include <windows.h>
#include <tlhelp32.h>
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
                return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
            }
        } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    return NULL;
}

static int dump(HANDLE h, DWORD addr, DWORD len, const char *file)
{
    char *buf = malloc(len);
    SIZE_T got = 0;
    if (!ReadProcessMemory(h, (void *)(UINT_PTR)addr, buf, len, &got) || got != len) {
        fprintf(stderr, "read %08lx+%lx failed (%lu)\n", addr, len, GetLastError());
        return 1;
    }
    FILE *f = fopen(file, "wb");
    fwrite(buf, 1, len, f);
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    HANDLE h = open_game();
    if (!h) { fprintf(stderr, "game.exe not running\n"); return 1; }
    if (argc == 5 && !strcmp(argv[1], "read"))
        return dump(h, strtoul(argv[2], 0, 0), strtoul(argv[3], 0, 0), argv[4]);
    if (argc == 5 && !strcmp(argv[1], "deref")) {
        DWORD p = 0;
        ReadProcessMemory(h, (void *)(UINT_PTR)strtoul(argv[2], 0, 0), &p, 4, NULL);
        return dump(h, p, strtoul(argv[3], 0, 0), argv[4]);
    }
    if (argc == 4 && !strcmp(argv[1], "poke")) {
        unsigned char b[256]; int n = 0;
        for (const char *s = argv[3]; s[0] && s[1] && n < 256; s += 2) { char t[3] = { s[0], s[1], 0 }; b[n++] = (unsigned char)strtoul(t, 0, 16); }
        SIZE_T put = 0;
        return !WriteProcessMemory(h, (void *)(UINT_PTR)strtoul(argv[2], 0, 0), b, n, &put);
    }
    fprintf(stderr, "usage: memtool read ADDR LEN FILE | deref PTR LEN FILE | poke ADDR HEX\n");
    return 2;
}
