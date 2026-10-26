#!/usr/bin/env python3
"""A raw CD image (.BIN, 2352-byte sectors, a MODE1 data track first) to an ISO of its data
track and WAVs of its audio tracks, using the .CUE's track starts.

    python3 -I tools/cdbin.py CD/x.CUE CD/x.BIN build/cd    -> build/cd/data.iso, build/cd/NN.wav
"""
import re, struct, sys
from pathlib import Path

cue, binf, out = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
out.mkdir(parents=True, exist_ok=True)
size = binf.stat().st_size // 2352
tracks = []                       # (number, mode, first sector of INDEX 01)
cur = None
for line in cue.read_text().splitlines():
    m = re.match(r"\s*TRACK (\d+) (\S+)", line)
    if m:
        cur = [int(m[1]), m[2]]
    m = re.match(r"\s*INDEX 01 (\d+):(\d+):(\d+)", line)
    if m:
        tracks.append((cur[0], cur[1], (int(m[1]) * 60 + int(m[2])) * 75 + int(m[3])))
with open(binf, "rb") as f:
    for i, (n, mode, start) in enumerate(tracks):
        end = tracks[i + 1][2] if i + 1 < len(tracks) else size
        if mode.startswith("MODE1"):
            end -= 150 if i + 1 < len(tracks) else 0      # the next track's 2 s pregap
            with open(out / "data.iso", "wb") as o:
                for s in range(start, end):
                    f.seek(s * 2352)
                    o.write(f.read(2352)[16:16 + 2048])
        else:
            f.seek(start * 2352)
            pcm = f.read((end - start) * 2352)
            hdr = b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt " + struct.pack(
                "<IHHIIHH", 16, 1, 2, 44100, 176400, 4, 16) + b"data" + struct.pack("<I", len(pcm))
            (out / f"{n:02d}.wav").write_bytes(hdr + pcm)
        print(n, mode, start, end - start)
