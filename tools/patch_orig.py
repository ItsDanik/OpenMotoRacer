#!/usr/bin/env python3
"""Patches to the private copy of the original (build/orig/game.EXE) so that it runs under
wine on a hidden display, for checking only. Never applied to the player's copy.

  0x45a7c9  je -> jmp: skip "16 bit screen mode required" (the desktop's depth test); in
            full-screen mode the game then sets its own 16-bit mode, which wine emulates.
"""
import sys
from pathlib import Path

PATCHES = [(0x45a7c9, bytes.fromhex("0f84"), bytes.fromhex("90e9"))]
TEXT_VA, TEXT_RAW = 0x401000, 0x400

p = Path(sys.argv[1]); d = bytearray(p.read_bytes())
for va, old, new in PATCHES:
    o = va - TEXT_VA + TEXT_RAW
    if d[o:o + len(old)] == old:
        d[o:o + len(new)] = new
        print(f"0x{va:x} patched")
    elif d[o:o + len(new)] == new:
        print(f"0x{va:x} already patched")
    else:
        sys.exit(f"0x{va:x}: unexpected bytes {d[o:o+len(old)].hex()}")
p.write_bytes(d)
