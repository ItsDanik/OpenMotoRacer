#!/usr/bin/env python3
"""Moto Racer's .BPT "binary scripts" (the .Spt settings and lists) as text.

    python3 -I tools/bpt.py "Moto Racer/data/CONFIG.BPT"

A .BPT is the tokens of a text script: a string is a u32 length (plain) and its characters
(0x424e60), a number is 4 bytes, read as a float (0x424c00) or an int (0x425080) as the
reader expects. The characters and numbers are enciphered (0x45a4a0): XOR with the low byte
of rand(6) (0x4b1160, Park-Miller, multiplier 16807, a result above 0x7fff taken mod
0x7fff), the generator seeded with 0x1a2b3c4d when the file is opened (0x4242a0 ->
0x4b1140); the lengths are not and do not advance it. Which token comes next is known only
to the reader, so this tool guesses: a plausible length followed by printable characters is
a string, anything else a number (shown as int and float).
"""
import struct
import sys
from pathlib import Path

M32 = 0xFFFFFFFF


def s32(x):
    x &= M32
    return x - (1 << 32) if x & 0x80000000 else x


def keystream(n, seed=0x1A2B3C4D):
    s, out = seed, bytearray()
    for _ in range(n):
        u = s32((s32(s) >> 16) * 0x41A7)
        v = s32(((u & 0x7FFF) << 16) + (s & 0xFFFF) * 0x41A7 + (u >> 15))
        if v < 0:
            v = s32(v - 0x7FFFFFFF)
        s = v & M32
        r = v % 0x7FFF if v > 0x7FFF else v
        out.append(r & 0xFF)
    return out


def decipher(d):
    return bytes(a ^ b for a, b in zip(d, keystream(len(d))))


class Key:
    def __init__(self, seed=0x1A2B3C4D):
        self.s = seed

    def byte(self):
        s = self.s
        u = s32((s32(s) >> 16) * 0x41A7)
        v = s32(((u & 0x7FFF) << 16) + (s & 0xFFFF) * 0x41A7 + (u >> 15))
        if v < 0:
            v = s32(v - 0x7FFFFFFF)
        self.s = v & M32
        return (v % 0x7FFF if v > 0x7FFF else v) & 0xFF


def tokens(d):
    """(kind, value) for each token, by the guess described above."""
    k, p, out = Key(), 0, []
    while p + 4 <= len(d):
        n, = struct.unpack_from("<I", d, p)
        if 0 < n < 512 and p + 4 + n <= len(d):
            save = k.s
            txt = bytes(c ^ k.byte() for c in d[p + 4:p + 4 + n])
            if all(32 <= c < 127 or c in (9, 10, 13) for c in txt):
                out.append(("s", txt.decode("latin-1")))
                p += 4 + n
                continue
            k.s = save
        raw = bytes(c ^ k.byte() for c in d[p:p + 4])
        i, = struct.unpack("<i", raw)
        f, = struct.unpack("<f", raw)
        out.append(("n", (i, f)))
        p += 4
    if p != len(d):
        out.append(("tail", d[p:]))
    return out


if __name__ == "__main__":
    for kind, v in tokens(Path(sys.argv[1]).read_bytes()):
        if kind == "s":
            print(repr(v))
        elif kind == "n":
            print(f"  {v[0]}  ({v[1]:g})")
        else:
            print("tail", v.hex())
