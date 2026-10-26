#!/usr/bin/env python3
"""A Windows PE program laid out as one flat file at its real addresses, with the layout map
the toolkit reads (the counterpart of unle.py for LE programs).

    python3 -I tools/pe2flat.py "Moto Racer/game.EXE" -o build/mr_flat.bin --map build/mr_layout.json

The flat file holds every section at its virtual address (offset = address, from 0), the
part a section's file data does not cover left as zeros (its BSS). The image is loaded at
its preferred base, so the relocation records need no applying: they only list where the
program holds an address (`fixup_sites`), so that a pointer is never confused with a
constant. Section flags are given in LE's terms (1 read, 2 write, 4 execute) so that
strmap.py, seeds.py and strrefs.py work unchanged.
"""
import argparse, json, struct
from pathlib import Path

import pefile


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--map", required=True)
    a = ap.parse_args()

    pe = pefile.PE(a.exe)
    base = pe.OPTIONAL_HEADER.ImageBase
    top = base + pe.OPTIONAL_HEADER.SizeOfImage
    img = bytearray(top)
    hdr_size = pe.OPTIONAL_HEADER.SizeOfHeaders
    img[base:base + hdr_size] = pe.__data__[:hdr_size]

    objects = []
    for i, s in enumerate(pe.sections, 1):
        va = base + s.VirtualAddress
        raw = s.get_data()[:max(s.Misc_VirtualSize, 0) or s.SizeOfRawData]
        img[va:va + len(raw)] = raw
        c = s.Characteristics
        flags = (1 if c & 0x40000000 else 0) | (2 if c & 0x80000000 else 0) \
            | (4 if c & 0x20000000 else 0)
        vsize = max(s.Misc_VirtualSize, s.SizeOfRawData)
        objects.append({"index": i, "name": s.Name.rstrip(b"\0").decode(), "base": va,
                        "vsize": vsize, "rawsize": s.SizeOfRawData,
                        "npages": (vsize + 0xfff) // 0x1000, "flags": flags})
        print(f"  {objects[-1]['name']:8} base=0x{va:08X} vsize=0x{vsize:06X} "
              f"raw=0x{s.SizeOfRawData:06X} flags={flags}")

    sites = []
    for blk in getattr(pe, "DIRECTORY_ENTRY_BASERELOC", []):
        for e in blk.entries:
            if e.type == 3:                       # IMAGE_REL_BASED_HIGHLOW
                sites.append(base + e.rva)
    sites.sort()

    imports = {}
    for d in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
        for f in d.imports:
            imports[f.address] = d.dll.decode() + "!" + (
                f.name.decode() if f.name else f"#{f.ordinal}")

    entry = base + pe.OPTIONAL_HEADER.AddressOfEntryPoint
    Path(a.out).write_bytes(img)
    Path(a.map).write_text(json.dumps({
        "objects": objects, "entry_va": entry, "image_base": base,
        "fixups": {"applied": 0, "skipped": 0}, "fixup_sites": sites,
        "imports": {hex(k): v for k, v in sorted(imports.items())}}))
    print(f"entry 0x{entry:X}, {len(sites):,} pointer sites, {len(imports)} imports")
    print(f"wrote {a.out} ({len(img):,} bytes) and {a.map}")


if __name__ == "__main__":
    main()
