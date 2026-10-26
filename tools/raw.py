#!/usr/bin/env python3
"""Moto Racer's .RAW pictures (Image Alchemy "mhwanh") to PNG.

    python3 -I tools/raw.py in.RAW out.png
    python3 -I tools/raw.py -d build/bkf build/png       every .RAW under a folder

Header (32 bytes, big-endian): "mhwanh", u16 version (4), u16 width, u16 height,
u16 palette entries; then the palette (3 bytes an entry, 8-bit), then width*height bytes.
"""
import struct, sys
from pathlib import Path
from PIL import Image


def load(path):
    d = Path(path).read_bytes()
    assert d[:6] == b"mhwanh", path
    ver, w, h, n = struct.unpack_from(">HHHH", d, 6)
    pal = d[32:32 + 3 * n]
    px = d[32 + 3 * n:32 + 3 * n + w * h]
    return w, h, pal, px


def topng(src, dst):
    w, h, pal, px = load(src)
    im = Image.frombytes("P", (w, h), px)
    im.putpalette(pal)
    Path(dst).parent.mkdir(parents=True, exist_ok=True)
    im.save(dst)
    return w, h, len(pal) // 3


if __name__ == "__main__":
    if sys.argv[1] == "-d":
        src, dst = Path(sys.argv[2]), Path(sys.argv[3])
        for p in sorted(src.rglob("*.RAW")):
            try:
                print(p.relative_to(src), *topng(p, dst / p.relative_to(src).with_suffix(".png")))
            except AssertionError:
                print(p.relative_to(src), "not mhwanh")
    else:
        print(*topng(sys.argv[1], sys.argv[2]))
