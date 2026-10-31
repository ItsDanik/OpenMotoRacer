#!/usr/bin/env python3
"""The crash soak: the port built with the address and undefined-behaviour sanitizers, run
without a window over every track, mode, page and the CD, each in its own home folder.

    python3 tools/soak.py [--jobs N] [--only WORD] [--short] [--out DIR] [--bin FILE]

Builds build/asan/openmotoracer unless --bin is given. One log a run in DIR (default
build/soak/logs); at the end every distinct complaint with how often it came and its first
run. A run that ends otherwise than 0 counts too. Exit status 1 when there is a complaint.
--only WORD runs the runs whose name holds WORD; --short a tenth of the frames.
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
    args = ap.parse_args()
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
