/* Records a driven bike's steps from the running original under wine, exactly, as a debugger:
   a breakpoint on the entry and on the last instruction of the driven step (0x45d580 in
   game.EXE, 0x47ecb0 in MOTO.EXE), the object and a few globals read on each, the game halted
   meanwhile. So no sample tears and none is missed, however fast the game runs (MOTO.EXE has
   no frame cap).

     stepwatch PROFILE FILE STEPS [PACE_MS]

   PROFILE moto (MOTO.EXE) or game (game.EXE). Waits for the first STEPS driven steps, one
   record each (below), then takes its breakpoints out and leaves the game running.
   PACE_MS: holds the game so that a step comes at most every PACE_MS: its own clock then
   gives it that step, as a frame cap would (16: GOG's -FrameRateMax60).

   A record: u32 timeGetTime, u32 object, u32 keys, int arg2, int arg3, float dt (the step's
   arguments), then 8 u32 globals at the entry and 8 at the exit (profile's list), u32 C
   library rand state at the entry, u32 at the exit, the object at the entry, the object at
   the exit. tools/physcheck_cd.c reads them.

   Start of a recording also writes FILE.carac (the 8 carac rows), FILE.cos (the cosine
   table) and FILE.sec (the sections) as they are in memory.

   Build: i686-w64-mingw32-gcc -O2 -o build/stepwatch.exe tools/win/stepwatch.c -lwinmm */
#include <windows.h>
#include <tlhelp32.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name, *exe;
    DWORD entry, exit, objsize;
    DWORD globals[8];
    DWORD carac, cos, land;
    DWORD ptd_tls;      /* where the C library keeps its thread-data TLS index */
} Profile;

static const Profile s_prof[] = {
    { "moto", "moto.exe", 0x47ecb0, 0x47ef3e, 0x31d5,
      { 0x672594 /* dt */, 0x4d23b4 /* countdown */, 0x5011e8 /* clock */, 0x4d2424 /* turbo held */,
        0x4e5968 /* gear latch */, 0x6c39ac /* turbo pressed */, 0x672284 /* pressed before */, 0x4e5960 /* local */ },
      0x4fe898, 0x4ea910, 0x520478, 0 },
    { "game", "game.exe", 0x45d580, 0, 0x3209,
      { 0x581014, 0x4dc0b4, 0x50bbc8, 0, 0, 0, 0, 0 },
      0x509278, 0x5fcb80, 0, 0 },
};

static DWORD find_pid(const char *exe)
{
    PROCESSENTRY32 pe = { sizeof pe };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    DWORD pid = 0;
    if (Process32First(snap, &pe))
        do {
            if (!_stricmp(pe.szExeFile, exe))
                pid = pe.th32ProcessID;
        } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    return pid;
}

static HANDLE s_h;

static void rd(DWORD a, void *o, DWORD n)
{
    SIZE_T got = 0;
    if (!ReadProcessMemory(s_h, (void *)(UINT_PTR)a, o, n, &got) || got != n)
        memset(o, 0xee, n);
}

static void wr(DWORD a, const void *o, DWORD n)
{
    SIZE_T put = 0;
    WriteProcessMemory(s_h, (void *)(UINT_PTR)a, o, n, &put);
    FlushInstructionCache(s_h, (void *)(UINT_PTR)a, n);
}

static void dump(const char *base, const char *ext, DWORD a, DWORD n)
{
    char name[600];
    unsigned char *b = malloc(n);
    FILE *f;
    rd(a, b, n);
    snprintf(name, sizeof name, "%s.%s", base, ext);
    f = fopen(name, "wb");
    if (f) {
        fwrite(b, 1, n, f);
        fclose(f);
    }
    free(b);
}

/* the C library's rand state: its thread data, whose TLS index the routine that fetches it
   (0x4af340 in MOTO.EXE) keeps at DWORD [0x4af347 + ...]; found at run time from the TEB */
static DWORD s_tls_index = 0xffffffff;

static DWORD rand_state(HANDLE thread, const CONTEXT *c)
{
    typedef LONG (WINAPI *Query)(HANDLE, int, void *, ULONG, ULONG *);
    static Query query;
    struct { LONG status; void *teb; void *pid; void *tid; ULONG_PTR mask; LONG prio, base; } info;
    DWORD teb, ptd = 0, v = 0;
    (void)c;
    if (s_tls_index == 0xffffffff)
        return 0;
    if (!query)
        query = (Query)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread");
    if (!query || query(thread, 0, &info, sizeof info, 0) != 0)
        return 0;
    teb = (DWORD)(ULONG_PTR)info.teb;
    rd(teb + 0xe10 + 4 * s_tls_index, &ptd, 4);
    if (!ptd)
        return 0;
    rd(ptd + 0x14, &v, 4);
    return v;
}

int main(int argc, char **argv)
{
    const Profile *p = 0;
    unsigned char cc = 0xcc, e0 = 0, x0 = 0;
    DWORD pid, steps, done = 0, pace = 0, last = 0, cur_tid = 0;
    int i, attached = 0, in_step = 0;
    FILE *f;
    unsigned char *rec;
    DWORD head = 24 + 32 + 32 + 8;
    HANDLE threads[256];
    DWORD tids[256];
    int nthreads = 0;
    if (argc < 4) {
        fprintf(stderr, "usage: stepwatch moto|game FILE STEPS [PACE_MS]\n");
        return 2;
    }
    for (i = 0; i < (int)(sizeof s_prof / sizeof s_prof[0]); i++)
        if (!strcmp(s_prof[i].name, argv[1]))
            p = &s_prof[i];
    if (!p || !p->exit) {
        fprintf(stderr, "no such profile\n");
        return 2;
    }
    steps = strtoul(argv[3], 0, 0);
    if (argc > 4)
        pace = strtoul(argv[4], 0, 0);
    pid = find_pid(p->exe);
    if (!pid) {
        fprintf(stderr, "%s not running\n", p->exe);
        return 1;
    }
    s_h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!s_h) {
        fprintf(stderr, "cannot open the process\n");
        return 1;
    }
    {   /* the TLS index, from the fetch routine's own code: mov/push DWORD [index] */
        unsigned char code[32];
        DWORD a = 0;
        rd(0x4af340, code, sizeof code);
        for (i = 0; i + 6 < (int)sizeof code; i++)
            if ((code[i] == 0xff && code[i + 1] == 0x35) || (code[i] == 0xa1)) {
                memcpy(&a, code + i + (code[i] == 0xa1 ? 1 : 2), 4);
                if (a > 0x400000 && a < 0x800000)
                    break;
                a = 0;
            }
        if (a)
            rd(a, &s_tls_index, 4);
    }
    f = fopen(argv[2], "wb");
    if (!f) {
        fprintf(stderr, "cannot write %s\n", argv[2]);
        return 1;
    }
    rec = malloc(head + 2 * p->objsize);
    timeBeginPeriod(1);
    if (!DebugActiveProcess(pid)) {
        fprintf(stderr, "cannot attach: %lu\n", GetLastError());
        return 1;
    }
    DebugSetProcessKillOnExit(FALSE);
    rd(p->entry, &e0, 1);
    rd(p->exit, &x0, 1);
    while (done < steps) {
        DEBUG_EVENT ev;
        DWORD how = DBG_CONTINUE;
        if (!WaitForDebugEvent(&ev, 20000)) {
            fprintf(stderr, "no event for 20 s (%lu steps so far)\n", done);
            break;
        }
        switch (ev.dwDebugEventCode) {
        case CREATE_PROCESS_DEBUG_EVENT:
            if (nthreads < 256) {
                tids[nthreads] = ev.dwThreadId;
                threads[nthreads++] = ev.u.CreateProcessInfo.hThread;
            }
            break;
        case CREATE_THREAD_DEBUG_EVENT:
            if (nthreads < 256) {
                tids[nthreads] = ev.dwThreadId;
                threads[nthreads++] = ev.u.CreateThread.hThread;
            }
            break;
        case EXIT_PROCESS_DEBUG_EVENT:
            fprintf(stderr, "the game ended\n");
            done = steps;
            break;
        case EXCEPTION_DEBUG_EVENT: {
            DWORD code = ev.u.Exception.ExceptionRecord.ExceptionCode;
            DWORD at = (DWORD)(UINT_PTR)ev.u.Exception.ExceptionRecord.ExceptionAddress;
            if (code != EXCEPTION_BREAKPOINT) {
                how = DBG_EXCEPTION_NOT_HANDLED;
                break;
            }
            if (!attached) {            /* the attach's own break */
                attached = 1;
                wr(p->entry, &cc, 1);
                break;
            }
            if (at == p->entry || at == p->exit) {
                HANDLE th = 0;
                CONTEXT c;
                int k;
                for (k = 0; k < nthreads; k++)
                    if (tids[k] == ev.dwThreadId)
                        th = threads[k];
                if (!th)
                    th = OpenThread(THREAD_ALL_ACCESS, FALSE, ev.dwThreadId);
                c.ContextFlags = CONTEXT_FULL | CONTEXT_SEGMENTS;
                GetThreadContext(th, &c);
                c.Eip = at;
                if (at == p->entry) {
                    DWORD args[4], obj = c.Ecx, now = timeGetTime();
                    rd(c.Esp + 4, args, 16);
                    memcpy(rec, &now, 4), memcpy(rec + 4, &obj, 4);
                    memcpy(rec + 8, args, 16);
                    for (k = 0; k < 8; k++)
                        rd(p->globals[k], rec + 24 + 4 * k, 4);
                    {
                        DWORD r = rand_state(th, &c);
                        memcpy(rec + 24 + 64, &r, 4);
                    }
                    rd(obj, rec + head, p->objsize);
                    if (!done) {
                        DWORD sec = 0, n = 0;
                        dump(argv[2], "carac", p->carac, 8 * 0x124);
                        dump(argv[2], "cos", p->cos, 0x10000);
                        rd(p->land, &sec, 4), rd(p->land + 4, &n, 4);
                        if (sec && n > 0 && n < 100000)
                            dump(argv[2], "sec", sec, n * 0xfc);
                    }
                    wr(p->entry, &e0, 1);
                    wr(p->exit, &cc, 1);
                    in_step = 1;
                } else {
                    DWORD obj;
                    memcpy(&obj, rec + 4, 4);
                    for (k = 0; k < 8; k++)
                        rd(p->globals[k], rec + 24 + 32 + 4 * k, 4);
                    {
                        DWORD r = rand_state(th, &c);
                        memcpy(rec + 24 + 64 + 4, &r, 4);
                    }
                    rd(obj, rec + head + p->objsize, p->objsize);
                    fwrite(rec, 1, head + 2 * p->objsize, f);
                    done++;
                    wr(p->exit, &x0, 1);
                    in_step = 0;
                    if (done < steps)
                        wr(p->entry, &cc, 1);
                    if (pace) {
                        DWORD now = timeGetTime();
                        if (last && now - last < pace)
                            Sleep(pace - (now - last));
                        last = timeGetTime();
                    }
                }
                SetThreadContext(th, &c);
            }
            break;
        }
        default:
            break;
        }
        cur_tid = ev.dwThreadId;
        ContinueDebugEvent(ev.dwProcessId, cur_tid, how);
    }
    (void)in_step;
    wr(p->entry, &e0, 1);
    wr(p->exit, &x0, 1);
    DebugActiveProcessStop(pid);
    fclose(f);
    timeEndPeriod(1);
    printf("%lu steps\n", done);
    return 0;
}
