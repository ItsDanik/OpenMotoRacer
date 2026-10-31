#!/usr/bin/env python3
# Two runs of tools/shots.sh compared: the sums, then the pictures of the sums that differ, in
# 5-6-5 (the game's own colours) against the PC's own pictures made again into PCDIR.
#   tools/shotdiff.py SUMS_A SUMS_B DIR_B PCDIR
# DIR_B holds the pictures of the run B; PCDIR, when it holds a picture of the same name, the PC's.
import os
import sys
import numpy as np
from PIL import Image


def sums(path):
    out = {}
    for line in open(path, errors="replace"):
        w = line.split()
        if len(w) == 2:
            out[w[0]] = w[1]
    return out


def q(f):
    a = np.asarray(Image.open(f).convert("RGB")).astype(int)
    return (a[..., 0] >> 3) * 2048 + (a[..., 1] >> 2) * 32 + (a[..., 2] >> 3)


a, b = sums(sys.argv[1]), sums(sys.argv[2])
same = [n for n in a if a[n] == b.get(n)]
print(f"pictures: {len(same)} of {len(a)} the same sum as the PC's")
for n in a:
    if a[n] == b.get(n):
        continue
    pc = os.path.join(sys.argv[4], n + ".bmp")
    there = os.path.join(sys.argv[3], n + ".bmp")
    if n in b and os.path.exists(pc) and os.path.exists(there):
        print(f"  {n}: {int((q(pc) != q(there)).sum())} pixels differ in 5-6-5")
    else:
        print(f"  {n}: {b.get(n, 'no sum')}")
