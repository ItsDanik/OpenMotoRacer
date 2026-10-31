#!/usr/bin/env python3
"""The crash soak: the port built with the address and undefined-behaviour sanitizers, run
without a window over every track, mode, page and the CD, each in its own home folder.

    python3 tools/soak.py [--jobs N] [--only WORD] [--short] [--out DIR] [--bin FILE] [--msan]

Builds build/asan/openmotoracer unless --bin is given. One log a run in DIR (default
build/soak/logs); at the end every distinct complaint with how often it came and its first
run. A run that ends otherwise than 0 counts too. Exit status 1 when there is a complaint.
--only WORD runs the runs whose name holds WORD; --short a tenth of the frames. --msan: clang's
MemorySanitizer for uninitialised values, with tools/sdlstub.c in place of SDL, whose memory it
cannot follow (logs in build/soak/msan).
"""
import argparse
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GOG = ROOT / "Moto Racer"
CD = ROOT / "Moto Racer CD"
# the lists' tracks: list.bpt (0) has eight; list01.bpt (1) adds Fun Fair, Sea of Sand first
TRACKS = [(0, t) for t in range(8)] + [(1, 0), (1, 1)]
SHAPES = [(0, 0), (1, 0), (0, 1), (1, 1)]   # reverse, pocket
KEYS = ("return escape space tab backspace left right up down shift ctrl minus plus f1 f5 f6 f7 "
        "f10 f11 f12 pause pgup pgdn home end a b q s v y n 1 2").split()
GOOD = ("return", "return", "return", "escape", "left", "right", "up", "down", "tab")


def monkey(seed, frames, every, keys=KEYS, ride=False):
    """seeded input: keys, the mouse, clicks"""
    r = random.Random(seed)
    out = []
    f = 20
    while f < frames:
        roll = r.random()
        if ride and roll < 0.5:
            k = r.choice(("left", "right", "down", "up"))
            out.append(f"{f}:hold:{k}")
            out.append(f"{f + r.randrange(10, 120)}:release:{k}")
        elif roll < 0.7:
            out.append(f"{f}:{r.choice(keys)}")
        else:
            out.append(f"{f}:move:{r.randrange(640)},{r.randrange(480)}")
            out.append(f"{f + 2}:click")
        f += r.randrange(every // 2, every * 2)
    return " ".join(sorted(out, key=lambda s: int(s.split(":")[0])))


def runs():
    """(name, env, frames)"""
    out = []
    win_keys = " ".join(f"{f}:return" for f in range(3000, 14000, 450))

    def race(name, data, lst, track, mode, shape, pilot, frames, extra=None, laps=1, manual=0):
        env = {"OMR_PAGE": "1", "OMR_LIST": str(lst),
               "OMR_CHOICES": f"{mode},{track},{laps},{shape[0]},{shape[1]},{manual}"}
        if pilot:
            env["OMR_AUTOPILOT"] = pilot
        if data:
            env["OMR_DATA"] = str(data)
        env.update(extra or {})
        out.append((name, env, frames))

    # every track and shape, ridden by the computer; and won, to the winner's lap and the results;
    # the other modes on the plain shape
    for lst, t in TRACKS:
        for sh, mode in [(s, 1) for s in SHAPES] + [(SHAPES[0], 0), (SHAPES[0], 2)]:
            tag = f"L{lst}T{t}R{sh[0]}P{sh[1]}M{mode}"
            race(f"auto_{tag}", None, lst, t, mode, sh, "1", 9200)
            race(f"win_{tag}", None, lst, t, mode, sh, "win", 14000, {"OMR_INPUT": win_keys})
    # a person's riding: falls, the view keys, the dialogs
    for lst, t in TRACKS:
        for seed in range(3):
            ride = monkey(seed * 100 + t, 7000, 40, ride=True)
            race(f"ride_L{lst}T{t}S{seed}", None, lst, t, 1, SHAPES[seed], None, 7000,
                 {"OMR_INPUT": f"10:hold:up {ride}"}, laps=2, manual=seed & 1)
    # the replay asked for at a step of the race
    for lst, t in TRACKS:
        race(f"watch_L{lst}T{t}", None, lst, t, 1, (0, 0), "1", 5000, {"OMR_WATCH_AT": "2000"})
    # the pages, as they are and with keys and clicks; with each mode's choices
    for page in range(27):
        for mode in (0, 1, 2):
            env = {"OMR_PAGE": str(page), "OMR_CHOICES": f"{mode},0,2"}
            out.append((f"page_{page}_M{mode}", dict(env), 900))
            env["OMR_INPUT"] = monkey(page * 10 + mode, 2500, 60)
            out.append((f"pagekeys_{page}_M{mode}", env, 2500))
    for place in (1, 2, 3, 4):
        out.append((f"podium_{place}", {"OMR_PAGE": "24", "OMR_PODIUM": str(place),
                                        "OMR_CHOICES": "2,0,4"}, 1500))
    # the languages
    for lang in range(6):
        for page in (3, 5, 11, 13, 9):
            out.append((f"lang_{lang}_p{page}", {"OMR_LANG": str(lang), "OMR_PAGE": str(page),
                                                 "OMR_INPUT": monkey(lang + page, 1500, 60)}, 1500))
        race(f"lang_{lang}_race", None, 0, 0, 1, (0, 0), "1", 2500, {"OMR_LANG": str(lang)})
    # from the logo on, anything may be pressed
    for seed in range(40):
        out.append((f"monkey_{seed}", {"OMR_INPUT": monkey(seed, 15000, 45, keys=GOOD + GOOD + tuple(KEYS))}, 15000))
    # the demos, one after the other
    out.append(("demos", {"OMR_PAGE": "14"}, 40000))
    # what the above did not reach (a coverage build tells: gcov): the dialogs' other answers,
    # a record typed in, the demos left, the joystick, the race's Ctrl keys, the still camera
    for i, keys in enumerate(("right return", "left return", "down return", "up return", "n", "y", "escape")):
        ans = " ".join(f"{f}:{k}" for f in range(3000, 14000, 300) for k in [keys.split()[0]]) + " " + \
              " ".join(f"{f + 10}:return" for f in range(3000, 14000, 300))
        race(f"extra_answer_{i}", None, 0, 7, 1, (0, 0), "win", 14000, {"OMR_INPUT": ans})
        race(f"extra_answer_practice_{i}", None, 0, 1, 0, (0, 0), "win", 14000, {"OMR_INPUT": ans})
    # no replay after the results (Escape), in each mode; then the menus onward
    after = " ".join(f"{f}:return" for f in range(10900, 14000, 500))
    for mode in (0, 1, 2):
        race(f"extra_noreplay_M{mode}", None, 0, 0, mode, (0, 0), "win", 14000,
             {"OMR_INPUT": " ".join(f"{f}:escape" for f in range(9000, 10900, 100)) + " " + after})
    # the pause menu's rows, in a demo and in a championship's replay
    for i, keys in enumerate(("return", "down return", "up return", "right return", "left return")):
        k = keys.split()
        seq = " ".join(f"{2500 + 40 * j}:{w}" for j, w in enumerate(k))
        out.append((f"extra_demopause_{i}", {"OMR_PAGE": "14", "OMR_INPUT": f"2400:escape {seq}"}, 3500))
        race(f"extra_replaypause_{i}", None, 0, 0, 2, (0, 0), "1", 3500,
             {"OMR_WATCH_AT": "1500", "OMR_INPUT": "2300:escape " + seq})
    for h in range(150, 340, 15):
        click = f"{{}}:move:320,{h} {{}}:down {{}}:up"
        out.append((f"extra_demoquit_{h}", {"OMR_PAGE": "14", "OMR_INPUT": "2400:escape " + click.format(2600, 2605, 2610)}, 3500))
        race(f"extra_replayquit_{h}", None, 0, 0, 2, (0, 0), "1", 3500,
             {"OMR_WATCH_AT": "1500", "OMR_INPUT": "2300:escape " + click.format(2500, 2505, 2510)})
    typed = " ".join(f"{f}:{'abcdefgh'[(f // 30) % 8]}" for f in range(3000, 14000, 30)) + " " + \
            " ".join(f"{f}:return" for f in range(3015, 14000, 600))
    race("extra_record", None, 0, 1, 1, (0, 0), "win", 14000, {"OMR_INPUT": typed})
    race("extra_record_champ", None, 0, 0, 2, (0, 0), "win", 14000, {"OMR_INPUT": typed})
    for i, k in enumerate(("escape", "return", "space", "q")):
        out.append((f"extra_demo_{i}", {"OMR_PAGE": "14", "OMR_INPUT": " ".join(f"{f}:{k}" for f in range(2200, 12000, 700))}, 12000))
        race(f"extra_watch_{i}", None, 0, 0, 1, (0, 0), "1", 5000,
             {"OMR_WATCH_AT": "1500", "OMR_INPUT": " ".join(f"{f}:{k}" for f in range(1600, 5000, 300))})
    ctrl = "100:hold:up 300:hold:ctrl " + " ".join(f"{300 + 40 * i}:{k}" for i, k in enumerate("svajkumb")) + \
           " 700:release:ctrl 800:f8 900:f8 1000:f8 1100:f12 1200:f11 1300:f12 1400:shift 1500:hold:shift 1510:f12 1520:release:shift"
    race("extra_ctrl", None, 0, 0, 1, (0, 0), None, 2500, {"OMR_INPUT": ctrl})
    joy = " ".join(f"{f}:joy:{(f * 7919) % 65536},{(f * 104729) % 65536},{(f // 40) % 16}" for f in range(20, 4000, 37))
    for dev in (2, 4):
        race(f"extra_joy_{dev}", None, 0, 0, 1, (0, 0), None, 4000, {"OMR_DEVICE": str(dev), "OMR_INPUT": joy})
        out.append((f"extra_joypage_{dev}", {"OMR_DEVICE": str(dev), "OMR_PAGE": "12", "OMR_INPUT": joy + " " + monkey(dev, 4000, 60)}, 4000))
        out.append((f"extra_joymenu_{dev}", {"OMR_DEVICE": str(dev), "OMR_PAGE": "3", "OMR_INPUT": joy}, 4000))
        pad = " ".join(f"{f}:pad:{(f * 7919) % 65536},{(f * 104729) % 65536},{(f // 40) % 8192},{(f * 31) % 32768},{(f * 57) % 32768},{(f // 25) % 16}" for f in range(20, 4000, 7))
        race(f"extra_pad_{dev}", None, 0, 0, 1, (0, 0), None, 4000, {"OMR_DEVICE": str(dev), "OMR_INPUT": pad})
        out.append((f"extra_padpage_{dev}", {"OMR_DEVICE": str(dev), "OMR_PAGE": "11", "OMR_INPUT": pad}, 4000))
        out.append((f"extra_padmenu_{dev}", {"OMR_DEVICE": str(dev), "OMR_PAGE": "3", "OMR_INPUT": pad}, 4000))
    out.append(("extra_view", {"OMR_PAGE": "3", "OMR_RACEVIEW": "4,394.0,56277.69,12255.37,51147.06,-0.5763,0.0487,0.8157,0.0,0.9982,-0.0596,-0.8172,-0.0343,-0.5753"}, 60))
    out.append(("extra_text", {"OMR_PAGE": "3", "OMR_TEXT": "12,100,100,0,1,1,-1,-1,HELLO"}, 200))
    # the CD's files, its MOTO.EXE
    if CD.is_dir():
        for t in range(8):
            for sh in SHAPES:
                race(f"cd_auto_T{t}R{sh[0]}P{sh[1]}", CD, 0, t, 1, sh, "1", 3000)
                race(f"cd_win_T{t}R{sh[0]}P{sh[1]}", CD, 0, t, 2, sh, "win", 12000, {"OMR_INPUT": win_keys})
            ride = monkey(t + 7, 4000, 40, ride=True)
            race(f"cd_ride_T{t}", CD, 0, t, 1, (0, 0), None, 4000, {"OMR_INPUT": f"10:hold:up {ride}"})
        for page in range(27):
            out.append((f"cd_page_{page}", {"OMR_DATA": str(CD), "OMR_PAGE": str(page), "OMR_CHOICES": "1,0,2",
                                            "OMR_INPUT": monkey(page, 1500, 60)}, 1500))
        for seed in range(8):
            out.append((f"cd_monkey_{seed}", {"OMR_DATA": str(CD),
                                              "OMR_INPUT": monkey(seed + 1000, 12000, 45, keys=GOOD + GOOD + tuple(KEYS))}, 12000))
    return out


def complaints(text):
    """the distinct complaints of a log"""
    found = []
    for m in re.finditer(r"([\w./-]+:\d+):\d+: runtime error: (.*)", text):
        msg = re.sub(r"-?\d[\d.e+-]*|0x[0-9a-f]+", "N", m.group(2))
        found.append(f"{Path(m.group(1)).name}:{m.group(1).rsplit(':', 1)[1]} {msg}")
    for m in re.finditer(r"(?:ERROR|WARNING): (\w+Sanitizer: [\w-]+)", text):
        where = re.search(r"#\d+ \S+ in (\S+) \S*port/src/([\w.]+):(\d+)", text[m.end():])
        found.append(f"{m.group(1)} at {where.group(2)}:{where.group(3)} {where.group(1)}" if where else m.group(1))
    for m in re.finditer(r"(?m)^openmotoracer: (.*)$", text):
        found.append("said: " + re.sub(r"\d+", "N", m.group(1)))
    return found


def one(args, spec):
    name, env, frames = spec
    if args.short:
        frames = max(frames // 10, 60)
    work = Path(tempfile.mkdtemp(prefix=name + ".", dir=args.scratch))
    full = dict(os.environ)
    full.update(env)
    full.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", OMR_HOME=str(work / "home"),
                OMR_WAV=str(work / "o.wav"),
                MSAN_OPTIONS="halt_on_error=0",
                ASAN_OPTIONS="detect_leaks=0:strict_string_checks=1:color=never",
                UBSAN_OPTIONS="print_stacktrace=1:color=never")
    log = args.out / f"{name}.log"
    try:
        p = subprocess.run([str(args.bin), "--shot", str(work / "s.bmp"), str(frames)], cwd=ROOT, env=full,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=args.timeout)
        text, rc = p.stdout.decode("latin-1"), p.returncode
    except subprocess.TimeoutExpired as e:
        text, rc = (e.stdout or b"").decode("latin-1"), "timeout"
    log.write_text(f"# {name} frames={frames} rc={rc}\n# " + " ".join(f"{k}={v!r}" for k, v in env.items()) + "\n" + text)
    shutil.rmtree(work, ignore_errors=True)
    c = complaints(text)
    if rc != 0:
        c.append(f"ended with {rc}")
    return name, frames, rc, c


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--only", default="")
    ap.add_argument("--short", action="store_true")
    ap.add_argument("--out", type=Path, default=ROOT / "build/soak/logs")
    ap.add_argument("--scratch", type=Path, default=None)
    ap.add_argument("--bin", type=Path, default=None)
    ap.add_argument("--timeout", type=int, default=1800)
    ap.add_argument("--msan", action="store_true")
    args = ap.parse_args()
    if args.msan:
        cflags = subprocess.run(["pkg-config", "--cflags", "sdl2"], capture_output=True, text=True, check=True).stdout.split()
        (ROOT / "build/msan").mkdir(parents=True, exist_ok=True)
        args.bin = ROOT / "build/msan/omr_msan"
        subprocess.run(["clang", "-std=c11", "-D_GNU_SOURCE", "-O1", "-g", "-fsanitize=memory", "-fsanitize-recover=memory",
                        "-fsanitize-memory-track-origins=2", "-fno-omit-frame-pointer", "-fwrapv", "-ffp-contract=off",
                        "-fsigned-char", "-Wno-everything", *cflags, *map(str, (ROOT / "port/src").glob("*.c")),
                        str(ROOT / "tools/sdlstub.c"), "-lm", "-o", str(args.bin)], check=True)
        if args.out == ROOT / "build/soak/logs":
            args.out = ROOT / "build/soak/msan"
    if not args.bin:
        b = ROOT / "build/asan"
        subprocess.run(["cmake", "-S", ROOT / "port", "-B", b, "-DCMAKE_BUILD_TYPE=Debug",
                        "-DCMAKE_C_FLAGS=-fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -g",
                        "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"], check=True, stdout=subprocess.DEVNULL)
        subprocess.run(["cmake", "--build", b, "-j8"], check=True, stdout=subprocess.DEVNULL)
        args.bin = b / "openmotoracer"
    args.out.mkdir(parents=True, exist_ok=True)
    args.scratch = args.scratch or args.out.parent / "work"
    args.scratch.mkdir(parents=True, exist_ok=True)
    todo = [r for r in runs() if args.only in r[0]]
    seen = Counter()
    first = {}
    total = 0
    with ThreadPoolExecutor(args.jobs) as pool:
        for name, frames, rc, c in pool.map(lambda r: one(args, r), todo):
            total += frames
            for k in set(c):
                seen[k] += 1
                first.setdefault(k, name)
    print(f"{len(todo)} runs, {total} frames, {len(seen)} distinct complaints")
    for k, n in seen.most_common():
        print(f"{n:5d}  {k}   [{first[k]}]")
    sys.exit(1 if seen else 0)


main()
