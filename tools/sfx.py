#!/usr/bin/env python3
"""Moto Racer's sound effects (SFX.BKF, data\\sfx\\...): list them or turn them into .wav.

    python3 -I tools/sfx.py "Moto Racer/data/SFX.BKF"                 list every sound
    python3 -I tools/sfx.py "Moto Racer/data/SFX.BKF" -x build/sfx    plain .wav files
    python3 -I tools/sfx.py build/bkf/SFX/16_BITS/MENU/SELECT.WAV     loose files work too

The files are RIFF WAVE files, enciphered: when the first byte is 'r' (0x72) instead of
'R', the loader 0x440d30 deciphers each block it reads with 0x440d00:
    plain[i] = stored[i] - 4 * (i + 8)        (mod 256, i counted from the block's start)
and it reads three blocks, each counted from i = 0 again:
    1. 0x24 bytes: 'RIFF', size, 'WAVE', 'fmt ', fmt size, then the 16 bytes of the
       WAVEFORMAT (tag, channels, rate, bytes/s, block align, bits);
    2. after skipping (fmt size - 16) bytes (not deciphered), 8 bytes: 'data', size;
    3. the size bytes of samples.
It insists on 'RIFF', 'WAVE', 'fmt ' and 'data' at those places (no other chunk may come
between fmt and data). The DirectSound buffer is made from tag 1 (PCM), the file's channels,
rate and bits; nothing else in the header is used.
"""
import argparse, struct, sys, wave
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bkf  # noqa: E402


def unmask(b):
    return bytes((x - 4 * (i + 8)) & 0xFF for i, x in enumerate(b))


def decode(d):
    """(channels, rate, bits, pcm bytes) from a stored file, as 0x440d30 reads it."""
    enc = d[:1] == b"r"
    f = (lambda b: unmask(b)) if enc else (lambda b: b)
    h = f(d[:0x24])
    riff, _, wav, fmt, fsize = struct.unpack_from("<4sI4s4sI", h)
    tag, ch, rate, _, _, bits = struct.unpack_from("<HHIIHH", h, 0x14)
    p = 0x24 + fsize - 16
    c = f(d[p:p + 8])
    did, dsize = struct.unpack_from("<4sI", c)
    if (riff, wav, fmt, did) != (b"RIFF", b"WAVE", b"fmt ", b"data"):
        raise ValueError("not a RIFF/WAVE/fmt/data file")
    pcm = f(d[p + 8:p + 8 + dsize])
    if len(pcm) != dsize:
        raise ValueError("data runs past the end")
    return ch, rate, bits, pcm, tag, enc, len(d) - (p + 8 + dsize)


def stats(ch, rate, bits, pcm):
    """Length in ms, peak (0..1), zero crossings per second of the first channel."""
    if bits == 8:
        s = [x - 128 for x in pcm[::ch]]
        full = 128
    else:
        n = len(pcm) // 2
        v = struct.unpack(f"<{n}h", pcm[:n * 2])
        s = list(v[::ch])
        full = 32768
    if not s:
        return 0, 0, 0
    zc = sum(1 for a, b in zip(s, s[1:]) if (a < 0) != (b < 0))
    sec = len(s) / rate
    return 1000 * sec, max(abs(x) for x in s) / full, zc / sec


def sources(paths):
    for p in paths:
        p = Path(p)
        if p.suffix.upper() == ".BKF":
            for name, _, _, data in bkf.read_bkf(p):
                if name.upper().endswith(".WAV"):
                    yield name.replace("\\", "/"), data
        else:
            yield p.name, p.read_bytes()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="+", help="SFX.BKF or loose .WAV files")
    ap.add_argument("-x", metavar="DIR", help="write plain .wav files under DIR")
    a = ap.parse_args()
    for name, d in sources(a.src):
        try:
            ch, rate, bits, pcm, tag, enc, tail = decode(d)
        except Exception as e:
            print(f"{name}: {e}")
            continue
        ms, peak, zc = stats(ch, rate, bits, pcm)
        print(f"{name:34} {'enc' if enc else 'raw'} tag {tag} {ch}ch {rate:5d} Hz {bits:2d} bit"
              f" {len(pcm):7d} B {ms:7.0f} ms peak {peak:.2f} zc/s {zc:6.0f}"
              + (f" +{tail} B after data" if tail else ""))
        if a.x:
            dst = Path(a.x) / name
            dst.parent.mkdir(parents=True, exist_ok=True)
            with wave.open(str(dst), "wb") as w:
                w.setnchannels(ch)
                w.setsampwidth(bits // 8)
                w.setframerate(rate)
                w.writeframes(pcm)


if __name__ == "__main__":
    main()
