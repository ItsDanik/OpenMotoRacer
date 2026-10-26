#!/usr/bin/env python3
"""Moto Racer's .BKF banks: list them or unpack them into a folder.

    python3 -I tools/bkf.py "Moto Racer/data/DEMO.BKF"              list
    python3 -I tools/bkf.py "Moto Racer/data/DEMO.BKF" -x build/bkf  unpack to build/bkf/DEMO/...

Layout (NOTES §3.1): u32 count, then count records of 0x2c bytes: the name (0x24 bytes,
zero-ended, the rest of the field is left-over memory), u32 offset from the file's start,
u32 size. The files follow the records, in the records' order, with no gaps.
"""
import argparse, struct
from pathlib import Path


def read_bkf(path):
    d = Path(path).read_bytes()
    n, = struct.unpack_from("<I", d, 0)
    out = []
    for i in range(n):
        r = 4 + i * 0x2C
        name = d[r:r + 0x24].split(b"\0")[0].decode("latin-1")
        off, size = struct.unpack_from("<II", d, r + 0x24)
        out.append((name, off, size, d[off:off + size]))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bkf", nargs="+")
    ap.add_argument("-x", metavar="DIR", help="unpack into DIR/<bank name>/")
    a = ap.parse_args()
    for p in a.bkf:
        ents = read_bkf(p)
        end = max((o + s for _, o, s, _ in ents), default=4)
        print(f"{p}: {len(ents)} files, ends at 0x{end:x} of 0x{Path(p).stat().st_size:x}")
        for name, off, size, data in ents:
            if not a.x:
                print(f"  0x{off:08x} {size:9d}  {name}")
                continue
            dst = Path(a.x) / Path(p).stem.upper() / name.replace("\\", "/")
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(data)


if __name__ == "__main__":
    main()
