#!/usr/bin/env python3
"""A port picture against a capture of the original, in 5-6-5 terms (what the game drew).
   tools/cmpshot.py PORT.bmp ORIG.png [TILE.png]
Prints the differing pixels and their box. With TILE.png (the page's background tile,
decoded by tools/lez.py), also where that tile is scrolled in each picture (x, y of its
top-left corner modulo 128, from a patch of plain background), for OMR_TILE."""
import sys
import numpy as np
from PIL import Image


def q(path):
    a = np.asarray(Image.open(path).convert("RGB")).astype(int)
    return (a[..., 0] >> 3) * 2048 + (a[..., 1] >> 2) * 32 + (a[..., 2] >> 3)


def phase(img, tile):
    """the (ox, oy) for which img[y, x] == tile[(y+oy)%128, (x+ox)%128] on the most
    pixels; tried on every 32x32 window until one matches whole"""
    T = np.tile(tile, (2, 2))
    for y0 in range(0, img.shape[0] - 32, 16):
        for x0 in range(0, img.shape[1] - 32, 16):
            p = img[y0:y0 + 32, x0:x0 + 32]
            for oy in range(128):
                ry = (y0 + oy) % 128
                rows = T[ry:ry + 32]
                for ox in range(128):
                    rx = (x0 + ox) % 128
                    if (rows[:, rx:rx + 32] == p).all():
                        return ox, oy
    return None


a, b = q(sys.argv[1]), q(sys.argv[2])
d = a != b
if d.any():
    ys, xs = np.nonzero(d)
    print(f"{d.sum()} pixels differ, x {xs.min()}..{xs.max()}, y {ys.min()}..{ys.max()}")
else:
    print("same")
if len(sys.argv) > 3:
    t = q(sys.argv[3])
    print("tile: port", phase(a, t), "original", phase(b, t))
