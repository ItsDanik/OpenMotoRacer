#!/usr/bin/env python3
"""C++ virtual-function tables of a PE program: runs of code pointers in the read-only
data, each with the functions that store its address (the constructors and destructors).

    python3 -I tools/vtables.py build/mr_layout.json build/mr_flat.bin build/decomp/INDEX.txt [names.txt] > build/vtables.txt

A table is a run of pointer sites (relocations), 4 bytes apart, every one into the code,
found in a section that is not code. Its users are code pointer sites whose value is the
table's first entry address (mov [ecx], offset vtable).
"""
import bisect, json, struct, sys
from collections import defaultdict

L = json.load(open(sys.argv[1]))
img = open(sys.argv[2], "rb").read()
starts = sorted(int(l.split()[0], 16) for l in open(sys.argv[3]))
names = {}
if len(sys.argv) > 4:
    for l in open(sys.argv[4]):
        p = l.split()
        if len(p) >= 2 and not l.startswith("#"):
            names[int(p[0], 16)] = p[1]
CODE = [(o["base"], o["base"] + o["vsize"]) for o in L["objects"] if o["flags"] & 4]
in_code = lambda a: any(lo <= a < hi for lo, hi in CODE)
sites = sorted(L["fixup_sites"])
siteset = set(sites)
val = lambda s: struct.unpack_from("<I", img, s)[0]
fn = lambda a: starts[bisect.bisect_right(starts, a) - 1]
nm = lambda a: names.get(a, "%x" % a)

tables, s_iter = [], iter(sites)
run = []
for s in sites:
    if in_code(s) or not in_code(val(s)):
        if len(run) >= 2:
            tables.append(run)
        run = []
        continue
    if run and s != run[-1] + 4:
        if len(run) >= 2:
            tables.append(run)
        run = []
    run.append(s)
if len(run) >= 2:
    tables.append(run)

users = defaultdict(set)
tstarts = {t[0] for t in tables}
for s in sites:
    if in_code(s):
        v = val(s)
        if v in tstarts:
            users[v].add(fn(s))
for t in tables:
    if t[0] not in users:
        continue              # a table nobody installs: a jump table or a list of handlers
    print("vtable %x  (%d entries)  set by: %s" % (t[0], len(t), " ".join(nm(u) for u in sorted(users[t[0]]))))
    for i, s in enumerate(t):
        print("   +%02x  %s" % (i * 4, nm(val(s))))
