#!/usr/bin/env python3
"""Moto Racer's .LEZ pictures: a packed 8-bit paletted TGA. To PNG.

    python3 -I tools/lez.py in.LEZ out.png
    python3 -I tools/lez.py -d build/bkf build/png     every .LEZ under a folder

Unpacker 0x43eb00. Header: "LEZ1", u32 unpacked size, u32 offset of the copy words, u32
offset of the literal bytes, u32 (unused), u32 first flag word; more flag words follow
from 0x18. Flags are read from the top bit down, a sentinel 1 marking the word's end: 1 =
one literal byte, 0 = a u16 copy word: distance = low 10 bits, length = high 6 bits + 3.
The result is a TGA: palette length at +5, width +12, height +14, palette (B,G,R) at 0x12,
then one byte a pixel. The game turns the palette into the screen's 16-bit format.
"""
import struct, sys
from pathlib import Path
from PIL import Image


def unpack(d):
    assert d[:4] == b"LEZ1"
    size, words, lits, _, flags = struct.unpack_from("<5I", d, 4)
    out = bytearray()
    fp = 0x18
    bits = (flags * 2 + 1) & 0xFFFFFFFF
    bit = flags >> 31
    while True:
        if bit:
            out.append(d[lits]); lits += 1
        else:
            w, = struct.unpack_from("<H", d, words); words += 2
            src = len(out) - (w & 0x3FF)
            for i in range((w >> 10) + 3):
                out.append(out[src + i])
        if len(out) >= size:
            return bytes(out[:size])
        bit = bits >> 31
        bits = (bits << 1) & 0xFFFFFFFF
        if bits == 0:
            w, = struct.unpack_from("<I", d, fp); fp += 4
            bit = w >> 31
            bits = (w * 2 + 1) & 0xFFFFFFFF


def load(path):
    t = unpack(Path(path).read_bytes())
    npal, = struct.unpack_from("<h", t, 5)
    w, h = struct.unpack_from("<hh", t, 12)
    pal = t[0x12:0x12 + 3 * npal]
    px = t[0x12 + 3 * npal:0x12 + 3 * npal + w * h]
    return w, h, t[16], t[17], bytes(c for i in range(npal) for c in pal[3 * i:3 * i + 3][::-1]), px


def topng(src, dst):
    w, h, bpp, desc, pal, px = load(src)
    im = Image.frombytes("P", (w, h), px)
    im.putpalette(pal)
    Path(dst).parent.mkdir(parents=True, exist_ok=True)
    im.save(dst)
    return w, h, bpp, hex(desc), len(pal) // 3


if __name__ == "__main__":
    if sys.argv[1] == "-d":
        src, dst = Path(sys.argv[2]), Path(sys.argv[3])
        for p in sorted(src.rglob("*.LEZ")):
            print(p.relative_to(src), *topng(p, dst / p.relative_to(src).with_suffix(".png")))
    else:
        print(*topng(sys.argv[1], sys.argv[2]))
