#!/usr/bin/env python3
"""InstallShield 3 archives (the retail CD's DISK1/data.z): list or unpack.

    python3 -I tools/is3.py "Moto Racer CD/DISK1/data.z"            list
    python3 -I tools/is3.py "Moto Racer CD/DISK1/data.z" -x DIR     unpack into DIR

Header: u32 0x8c655d13, u16 file count at 0x0c, u32 table offset at 0x29, u16 folder count
at 0x31. The table: the folders (u16 files, u16 record size, u16 name length, name), then
the files (u8, u16 folder, u32 size, u32 packed size, u32 offset, u32 DOS date, u32, u16
record size, u32 attributes, u8 name length, name). A file whose packed size equals its size
is stored as it is (all of Moto Racer's); otherwise it is one PKWARE DCL "implode" stream,
unpacked by explode() below (the method of zlib's contrib/blast; untested here).
"""
import argparse, struct
from pathlib import Path


class _Bits:
    def __init__(self, d):
        self.d, self.p, self.buf, self.n = d, 0, 0, 0

    def bits(self, need):
        while self.n < need:
            self.buf |= self.d[self.p] << self.n
            self.p += 1
            self.n += 8
        v = self.buf & ((1 << need) - 1)
        self.buf >>= need
        self.n -= need
        return v


def _huff(rep):
    """A canonical code from blast's compact lengths; returns (count[], symbol[])."""
    lens = []
    for b in rep:
        lens += [b & 15] * ((b >> 4) + 1)
    count = [0] * 14
    for l in lens:
        count[l] += 1
    offs = [0] * 14
    for l in range(1, 13):
        offs[l + 1] = offs[l] + count[l]
    sym = [0] * len(lens)
    for s, l in enumerate(lens):
        if l:
            sym[offs[l]] = s
            offs[l] += 1
    return count, sym


def _decode(bt, h):
    count, sym = h
    code = first = index = 0
    for l in range(1, 14):
        code |= bt.bits(1) ^ 1          # codes are stored bit-inverted
        c = count[l]
        if code < first + c:
            return sym[index + (code - first)]
        index += c
        first = (first + c) << 1
        code <<= 1
    raise ValueError("bad code")


_LITLEN = bytes([11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8, 9, 7,
                 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11, 7, 9, 11, 12, 6, 7, 22, 5, 7, 24, 6, 11,
                 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12, 8, 12, 5, 38, 5, 38, 5,
                 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27, 44, 253, 253, 253, 252, 252,
                 252, 13, 12, 45, 12, 45, 12, 61, 12, 45, 44, 173])
_LENLEN = bytes([2, 35, 36, 53, 38, 23])
_DISTLEN = bytes([2, 20, 53, 230, 247, 151, 248])
_BASE = [3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264]
_EXTRA = [0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8]
_HLIT, _HLEN, _HDIST = _huff(_LITLEN), _huff(_LENLEN), _huff(_DISTLEN)


def explode(d):
    bt = _Bits(d)
    lit, dictb = bt.bits(8), bt.bits(8)
    assert lit in (0, 1) and 4 <= dictb <= 6, (lit, dictb)
    out = bytearray()
    while True:
        if bt.bits(1):
            s = _decode(bt, _HLEN)
            ln = _BASE[s] + bt.bits(_EXTRA[s])
            if ln == 519:
                return bytes(out)
            sh = 2 if ln == 2 else dictb
            dist = (_decode(bt, _HDIST) << sh) + bt.bits(sh) + 1
            for _ in range(ln):
                out.append(out[-dist])
        else:
            out.append(_decode(bt, _HLIT) if lit else bt.bits(8))


def read_is3(path):
    d = Path(path).read_bytes()
    assert struct.unpack_from("<I", d, 0)[0] == 0x8C655D13
    nfiles, = struct.unpack_from("<H", d, 0x0C)
    toc, = struct.unpack_from("<I", d, 0x29)
    ndirs, = struct.unpack_from("<H", d, 0x31)
    p, dirs = toc, []
    for _ in range(ndirs):
        _, size, nl = struct.unpack_from("<HHH", d, p)
        dirs.append(d[p + 6:p + 6 + nl].decode("latin-1"))
        p += size
    files = []
    for _ in range(nfiles):
        di, usize, csize, off, dt, _, size = struct.unpack_from("<HIIIIIH", d, p + 1)
        nl = d[p + 29]
        name = d[p + 30:p + 30 + nl].decode("latin-1")
        files.append((dirs[di], name, usize, d[off:off + csize]))
        p += size
    return files


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("z")
    ap.add_argument("-x", metavar="DIR")
    a = ap.parse_args()
    for folder, name, usize, packed in read_is3(a.z):
        rel = (folder + "\\" + name if folder else name).replace("\\", "/")
        if not a.x:
            print(f"{usize:10d} {len(packed):10d}  {rel}")
            continue
        data = packed if len(packed) == usize else explode(packed)   # stored or imploded
        assert len(data) == usize, (rel, len(data), usize)
        dst = Path(a.x) / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(data)
        print(rel, usize)


if __name__ == "__main__":
    main()
