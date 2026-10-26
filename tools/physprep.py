#!/usr/bin/env python3
"""The inputs tools/physcheck.c needs, from the player's files (written under build/):

    python3 -I tools/physprep.py BKF FAMILY N OUT      one carac row (0x124 bytes, 0x42a580's)

e.g. build/bkf SPEED 1 build/physcheck/carac.bin. The row is as the parser leaves it in
0x509278 (ints at +0x00..+0x10 and +0x120, floats elsewhere), before the dirt bikes' clamp.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import bpt  # noqa: E402


def main():
    bkf, family, n, out = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
    toks = [v for k, v in bpt.tokens((Path(bkf) / family / ('CARAC%d.BPT' % n)).read_bytes())
            if k == 'n']
    row = b''
    for k, (iv, fv) in enumerate(toks[:0x49]):
        row += struct.pack('<i', iv) if k < 5 or k == 0x48 else struct.pack('<f', fv)
    Path(out).write_bytes(row)
    print(out, len(row), 'bytes')


if __name__ == '__main__':
    main()
