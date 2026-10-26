/* Record the computer riders of the running original, one sample a frame (under wine).
     cpuwatch PERIOD_MS SECONDS FILE
   Every PERIOD_MS: the frame's time stamp 0x50bbc8, then the pieces below, then the time
   stamp again; a sample whose two stamps differ is dropped (the game moved on while it
   was read). Only the last good sample of each frame is written: the state after that
   frame's step (the step comes first in the frame, the drawing and the frame limiter
   after it).
   A record: u32 magic 'CPUW', u32 stamp, the globals (G_* below, 64 bytes), u32 the C
   library's rand() seed (msvcrt's per-thread data of the game's thread: TLS index at
   0x4f3200, +0x14), then riders 1..24, each the bytes [0, 0x4e0) and [0x2d90, 0x3209) of
   the record 0x581218 + k x 0x3209.
   Build: i686-w64-mingw32-gcc -O2 -o build/cpuwatch.exe tools/win/cpuwatch.c -lwinmm */
#include <windows.h>
#include <tlhelp32.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OBJ 0x3209
#define LO 0x4e0
#define HI0 0x2d90
#define RIDER (LO + OBJ - HI0)

static const DWORD globals[16] = {
    0x4e9808, 0x4e980c, 0x4dc0b4, 0x5429b0, 0x509bac, 0x4dc140, 0x4dc144, 0x4dc094,
    0x508834, 0x4e9834, 0x4dc150, 0x52c10c, 0x4dc0a8, 0x581014, 0x542874, 0x4dc060
};

typedef struct {
    LONG ExitStatus;
    PVOID TebBaseAddress;
    DWORD pid, tid;
    ULONG_PTR AffinityMask;
    LONG Priority, BasePriority;
} TBI;
typedef LONG (WINAPI *NtQIT)(HANDLE, int, PVOID, ULONG, PULONG);

static HANDLE h;

static int rd(DWORD a, void *buf, DWORD n)
{
    SIZE_T got = 0;
    return ReadProcessMemory(h, (void *)(UINT_PTR)a, buf, n, &got) && got == n;
}

int main(int argc, char **argv)
{
    PROCESSENTRY32 pe = { sizeof pe };
    THREADENTRY32 te = { sizeof te };
    HANDLE snap;
    DWORD pid = 0, ptd = 0, tls = 0, period, until, last = 0, seed;
    NtQIT q = (NtQIT)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread");
    unsigned char *cur, *keep;
    int have = 0, frames = 0, torn = 0, size;
    FILE *f;
    if (argc < 4) {
        fprintf(stderr, "usage: cpuwatch PERIOD_MS SECONDS FILE\n");
        return 2;
    }
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snap, &pe))
        do
            if (!_stricmp(pe.szExeFile, "game.exe"))
                pid = pe.th32ProcessID;
        while (!pid && Process32Next(snap, &pe));
    CloseHandle(snap);
    if (!pid || !(h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid))) {
        fprintf(stderr, "game.exe not running\n");
        return 1;
    }
    rd(0x4f3200, &tls, 4);
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (Thread32First(snap, &te))
        do {
            HANDLE t;
            TBI bi;
            DWORD p = 0, d[6];
            if (te.th32OwnerProcessID != pid)
                continue;
            t = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
            if (!t)
                continue;
            if (q && q(t, 0, &bi, sizeof bi, 0) == 0 && tls < 64
                && rd((DWORD)(UINT_PTR)bi.TebBaseAddress + 0xe10 + 4 * tls, &p, 4) && p
                && rd(p, d, sizeof d)) {
                fprintf(stderr, "thread %lu: ptd %08lx (tid %lu, seed %08lx)\n",
                        te.th32ThreadID, p, d[0], d[5]);
                if (d[0] == te.th32ThreadID && !ptd)
                    ptd = p;            /* the first thread with the C library's data */
            }
            CloseHandle(t);
        } while (Thread32Next(snap, &te));
    CloseHandle(snap);
    fprintf(stderr, "tls index %lu, ptd %08lx\n", tls, ptd);
    size = 4 + 4 + 64 + 4 + 24 * RIDER;
    cur = malloc(size), keep = malloc(size);
    if (!(f = fopen(argv[3], "wb")))
        return 1;
    period = strtoul(argv[1], 0, 0);
    timeBeginPeriod(1);
    until = timeGetTime() + 1000 * strtoul(argv[2], 0, 0);
    while (timeGetTime() < until) {
        DWORD s0 = 0, s1 = 1, at = 8, k;
        int ok = 1;
        memcpy(cur, "CPUW", 4);
        ok &= rd(0x50bbc8, &s0, 4);
        memcpy(cur + 4, &s0, 4);
        for (k = 0; k < 16; k++, at += 4)
            ok &= rd(globals[k], cur + at, 4);
        seed = 0;
        if (ptd)
            rd(ptd + 0x14, &seed, 4);
        memcpy(cur + at, &seed, 4), at += 4;
        for (k = 1; k <= 24; k++) {
            DWORD a = 0x581218 + k * OBJ;
            ok &= rd(a, cur + at, LO);
            ok &= rd(a + HI0, cur + at + LO, OBJ - HI0);
            at += RIDER;
        }
        ok &= rd(0x50bbc8, &s1, 4);
        if (!ok || s0 != s1)
            torn++;
        else {
            if (have && s0 != last) {
                fwrite(keep, 1, size, f);
                frames++;
            }
            memcpy(keep, cur, size);
            have = 1;
            last = s0;
        }
        Sleep(period);
    }
    if (have)
        fwrite(keep, 1, size, f), frames++;
    fclose(f);
    fprintf(stderr, "%d frames, %d torn samples dropped\n", frames, torn);
    return 0;
}
