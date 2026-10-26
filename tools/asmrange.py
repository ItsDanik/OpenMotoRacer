#!/usr/bin/env python3
"""asmrange.py FROM TO [asm]: the lines of build/mr.asm (objdump) with FROM <= address < TO
(int3 padding left out)."""
import sys
lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
path = sys.argv[3] if len(sys.argv) > 3 else 'build/mr.asm'
with open(path) as f:
    for line in f:
        h = line.split(':', 1)[0].strip()
        try:
            a = int(h, 16)
        except ValueError:
            continue
        if a >= hi:
            break
        if a >= lo and 'int3' not in line:
            print(line, end='')
