#!/usr/bin/env python3
"""Which functions use which Windows import (call/mov through the import table).

    python3 -I tools/imprefs.py build/mr_layout.json build/mr.asm build/decomp/INDEX.txt [name...]

build/mr.asm: objdump -d -M intel --no-show-raw-insn of the program. With names, only those
imports (a substring match); without, every import with its users.
"""
import bisect, json, re, sys
from collections import defaultdict

L = json.load(open(sys.argv[1]))
imps = {int(k, 16): v for k, v in L["imports"].items()}
starts = sorted(int(l.split()[0], 16) for l in open(sys.argv[3]))
want = sys.argv[4:]
users = defaultdict(set)
pat = re.compile(r"^\s*([0-9a-f]+):.*ds:0x([0-9a-f]+)")
for line in open(sys.argv[2]):
    m = pat.match(line)
    if m:
        t = int(m[2], 16)
        if t in imps:
            a = int(m[1], 16)
            i = bisect.bisect_right(starts, a) - 1
            users[imps[t]].add(starts[i] if i >= 0 else a)
for name in sorted(users):
    if want and not any(w.lower() in name.lower() for w in want):
        continue
    print(f"{name:40} " + " ".join(f"{u:x}" for u in sorted(users[name])))
