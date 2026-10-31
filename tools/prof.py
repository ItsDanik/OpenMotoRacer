#!/usr/bin/env python3
"""What the sampling profiler saw (port/src/prof.c).
usage: prof.py SAMPLES BINARY [LIBDIR]   by function, then by source line. SAMPLES.maps says where the
shared libraries were; LIBDIR holds copies of them from the MiSTer, read for their names.
The tools are the ARM ones (NM and ADDR2LINE in the environment, commands)."""
import bisect, collections, os, re, shlex, struct, subprocess, sys
NM = shlex.split(os.environ.get("NM", "nm"))
A2L = shlex.split(os.environ.get("ADDR2LINE", "addr2line"))
pairs = list(struct.iter_unpack("<II", open(sys.argv[1], "rb").read()))
binary = sys.argv[2]
libdir = sys.argv[3] if len(sys.argv) > 3 else None
def symbols(path, base, dynamic=False):
    out = []
    args = NM + (["-D"] if dynamic else []) + ["-n", path]
    for l in subprocess.run(args, capture_output=True, text=True).stdout.splitlines():
        w = l.split()
        if len(w) == 3 and w[1] in "tTwWiI":
            out.append((int(w[0], 16) + base, w[2].split("@")[0]))
    return out
syms = symbols(binary, 0)
syms = [s for s in syms if s[1] != "data_start"]
regions = []
if os.path.exists(sys.argv[1] + ".maps"):
    first = {}
    for l in open(sys.argv[1] + ".maps"):
        w = l.split()
        lo, hi = (int(x, 16) for x in w[0].split("-"))
        path = w[5] if len(w) > 5 else ""
        offset = int(w[2], 16)
        if path and path not in first:
            first[path] = lo - offset
        regions.append((lo, hi, path))
    if libdir:
        for path, base in first.items():
            copy = os.path.join(libdir, os.path.basename(path))
            if os.path.exists(copy):
                syms += symbols(copy, base, True)
syms.sort()
addrs = [a for a, _ in syms]
top = regions[0][1] if regions else 1 << 32
exe_end = max((hi for lo, hi, p in regions if p and os.path.basename(p).lower().startswith("openmoto")), default=1 << 32)
def name(pc):
    for lo, hi, p in regions:
        if lo <= pc < hi:
            if p and not os.path.basename(p).lower().startswith("openmoto") and not libdir:
                return os.path.basename(p)
            if not p or p.startswith("[") or p.startswith("/dev"):
                return p or "anon"
            break
    i = bisect.bisect_right(addrs, pc) - 1
    return syms[i][1] if i >= 0 else "?"
by = collections.Counter(name(p) for p, _ in pairs)
n = len(pairs)
print(f"{n} samples")
cum = 0
for f, c in by.most_common(45):
    cum += c
    print(f"{100*c/n:6.2f}% {100*cum/n:6.1f}%  {f}")
pcs = collections.Counter(p & ~1 for p, _ in pairs if p < exe_end)
tops = [p for p, _ in pcs.most_common(1500)]
out = subprocess.run(A2L + ["-e", binary] + [hex(p) for p in tops], capture_output=True, text=True).stdout.split("\n")
lines = collections.Counter()
for p, l in zip(tops, out):
    lines[os.path.basename(l.split(" ")[0])] += pcs[p]
print("\nby line:")
for l, c in lines.most_common(60):
    print(f"{100*c/n:6.2f}%  {l}")
print("\nby instruction:")
for p, c in pcs.most_common(25):
    print(f"{100*c/n:6.2f}%  {p:08x} {name(p)}")
memcpys = collections.Counter(l & ~1 for p, l in pairs if name(p & ~1) == "memcpy")
print("\nmemcpy called from:")
for p, c in memcpys.most_common(8):
    print(f"{100*c/n:6.2f}%  {p:08x} {name(p)}")
fr = sys.argv[1] + ".frames"
if os.path.exists(fr):
    F = [[float(x) for x in l.split()] for l in open(fr)]
    NAMES = ["start", "steps", "scene", "render", "2D", "present"]
    d = [[m[i + 1] - m[i] for i in range(5)] + [m[5], m[5] - m[7]] for m in F]
    print(f"\n{len(F)} pictures, ms: " + " ".join(f"{a:>7}" for a in NAMES[1:] + ["work", "held"]))
    for q in (50, 90, 99, 100):
        print(f"  {q:3d}%" + " " * 19 + " ".join(f"{sorted(x[k] for x in d)[min(len(d) - 1, len(d) * q // 100)]:7.2f}" for k in range(7)))
    print("the slowest:")
    for x in sorted(d, key=lambda x: -x[5])[:15]:
        print(" " * 25 + " ".join(f"{v:7.2f}" for v in x))
    w = sorted(x[5] for x in d)
    print("work over 16.7 ms: %d, over 20: %d, over 25: %d of %d" % (sum(v > 16.7 for v in w), sum(v > 20 for v in w), sum(v > 25 for v in w), len(w)))
