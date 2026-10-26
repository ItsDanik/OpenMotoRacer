#!/usr/bin/env python3
"""peek.py [flat] SPEC... : values in the flat image (build/mr_flat.bin, offset = address).
SPEC = ADDR[:TYPE[:COUNT]], TYPE s (string, default), f (float), d (double), i (int32),
h (int16), b (byte), x (dword hex). Example: peek.py 0x4d1970:f 0x515fe0:i:3 0x4f0cac"""
import struct, sys
args = sys.argv[1:]
flat = 'build/mr_flat.bin'
if args and not args[0].startswith('0x'):
    flat = args.pop(0)
data = open(flat, 'rb').read()
fmt = {'f': ('<f', 4), 'd': ('<d', 8), 'i': ('<i', 4), 'h': ('<h', 2), 'b': ('<B', 1), 'x': ('<I', 4)}
for spec in args:
    p = spec.split(':')
    a = int(p[0], 16); t = p[1] if len(p) > 1 else 's'; n = int(p[2]) if len(p) > 2 else 1
    if t == 's':
        e = data.index(b'\0', a)
        print(f'{a:#x} s {data[a:e]!r}')
        continue
    f, w = fmt[t]
    vals = [struct.unpack_from(f, data, a + k * w)[0] for k in range(n)]
    if t == 'x':
        vals = [f'{v:#x}' for v in vals]
    raw = data[a:a + w * n].hex()
    print(f'{a:#x} {t} {vals}  [{raw}]')
