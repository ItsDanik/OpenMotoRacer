#!/usr/bin/env python3
"""The track's section planes as the game builds them at load (0x40e5c0), bit for bit.

    python3 -I tools/trkplanes.py TRACK.trk [SECTIONS.bin]

Each section (0xfc bytes in memory: pos +0, the 3x4 matrix +0xc (rows X, Y, Z, then the
offset T at +0x30), the walls +0x60 / +0x70 from the file) gets four planes (n.x, n.y, n.z, d;
distance = n.p + d):
  +0x40  start plane: n = normalise((0,0,1).M + T), through pos (the file's plane 0 is replaced)
  +0xa0  the vertical split along the diagonal Lj-Ri, through Lj
  +0x80  road triangle Li, Lj, Ri: n = normalise((Lj-Li) x (Ri-Li)), through Li
  +0x90  road triangle Rj, Lj, Ri: n = normalise((Lj-Rj) x (Ri-Rj)), through Rj
with Li/Ri where the section's own cross line meets its left (+0x60) / right (+0x70) wall,
Lj/Rj where the next section's cross line meets this section's walls.
The FPU runs with a 53-bit mantissa (the process default): a Python float is a register
value, f32() a store to a float. With SECTIONS.bin (the running game's section array, e.g.
read with tools/win/memtool.c from [0x52c108]) every plane is compared bit for bit.
"""
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import track  # noqa: E402

SEC = 0xfc


def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def mat_vec(v, M):
    """0x410010: v.M + T (M = 12 floats: X, Y, Z rows, T), each component stored as float."""
    return [f32(((M[6 + k] * v[2] + M[3 + k] * v[1]) + M[k] * v[0]) + M[9 + k]) for k in range(3)]


def vadd(a, b):
    """0x40ffc0: a + b, each stored as float."""
    return [f32(a[k] + b[k]) for k in range(3)]


def normalise(v):
    """0x40fcd0 (and the same code inline): length stored as float, 1/length unrounded."""
    ln = f32(math.sqrt((v[0] * v[0] + v[1] * v[1]) + v[2] * v[2]))
    if ln == 0.0:
        return list(v)
    inv = 1.0 / ln
    return [f32(v[0] * inv), f32(v[1] * inv), f32(inv * v[2])]


def plane_through(n, p):
    """0x4b0f20: the plane with normal n through p."""
    return [n[0], n[1], n[2], f32(-((n[1] * p[1] + n[2] * p[2]) + n[0] * p[0]))]


def line_plane(pl, A, B):
    """0x4b0f60: where the line A->B meets the plane (A if A ~ B or the line is parallel)."""
    eps = f32(1e-5)
    if abs(A[0] - B[0]) <= eps and abs(A[1] - B[1]) <= eps and abs(A[2] - B[2]) <= eps:
        return list(A)
    dx = B[0] - A[0]
    dy, dz = f32(B[1] - A[1]), f32(B[2] - A[2])
    dx32 = f32(dx)
    den = f32((pl[0] * dx + pl[1] * dy) + pl[2] * dz)
    if den == 0.0:
        return list(A)
    t = -((((pl[0] * A[0] + pl[2] * A[2]) + pl[1] * A[1]) + pl[3]) / den)
    return [f32(dx32 * t + A[0]), f32(dy * t + A[1]), f32(dz * t + A[2])]


def cross_planar(a, b):
    """The inline cross product of 0x40e5c0: a x b, each component stored as float."""
    return [f32(a[1] * b[2] - a[2] * b[1]), f32(a[2] * b[0] - a[0] * b[2]),
            f32(a[0] * b[1] - a[1] * b[0])]


def build(secs):
    """secs: list of dict(pos, M (12 floats), p60, p70). Returns per section {off: plane}."""
    n = len(secs)
    out = []
    left, right, zero = [-2048.0, 0.0, 0.0], [2048.0, 0.0, 0.0], [0.0, 0.0, 0.0]
    for i, s in enumerate(secs):
        Mi, pos = s['M'], s['pos']
        nxt = secs[(i + 1) % n]
        Mj, posj = nxt['M'], nxt['pos']
        p40 = plane_through(normalise(mat_vec([0.0, 0.0, 1.0], Mi)), pos)
        Li = line_plane(s['p60'], vadd(pos, mat_vec(zero, Mi)), vadd(pos, mat_vec(left, Mi)))
        Lj = line_plane(s['p60'], vadd(posj, mat_vec(zero, Mj)), vadd(posj, mat_vec(left, Mj)))
        Rj = line_plane(s['p70'], vadd(posj, mat_vec(zero, Mj)), vadd(posj, mat_vec(right, Mj)))
        Ri = line_plane(s['p70'], vadd(pos, Mi[9:12]), vadd(pos, mat_vec(right, Mi)))
        na0 = normalise([f32(Ri[2] - Lj[2]), 0.0, f32(-(Ri[0] - Lj[0]))])
        pa0 = plane_through(na0, Lj)
        e1 = [f32(Lj[k] - Li[k]) for k in range(3)]
        e2 = [f32(Ri[k] - Li[k]) for k in range(3)]
        p80 = plane_through(normalise(cross_planar(e1, e2)), Li)
        c = [f32(Ri[k] - Rj[k]) for k in range(3)]
        d = [f32(Lj[k] - Rj[k]) for k in range(3)]
        p90 = plane_through(normalise(cross_planar(d, c)), Rj)
        out.append({0x40: p40, 0x80: p80, 0x90: p90, 0xa0: pa0})
    return out


def from_track(path):
    t = track.read_track(path)
    secs = []
    for s in t['sections']:
        M = list(s['axes'][0]) + list(s['axes'][1]) + list(s['axes'][2]) + list(s['offset'])
        secs.append(dict(pos=list(s['pos']), M=M, p60=list(s['planes'][2]), p70=list(s['planes'][3])))
    return secs


def main():
    secs = from_track(sys.argv[1])
    planes = build(secs)
    if len(sys.argv) < 3:
        for i, p in enumerate(planes[:4]):
            print(i, {hex(k): [round(x, 6) for x in v] for k, v in p.items()})
        return
    mem = Path(sys.argv[2]).read_bytes()
    assert len(mem) >= len(secs) * SEC, 'dump too short'
    inputs_bad = 0
    good = {k: 0 for k in (0x40, 0x80, 0x90, 0xa0)}
    first_bad = {}
    for i, s in enumerate(secs):
        rec = mem[i * SEC:(i + 1) * SEC]
        mi = struct.unpack_from('<3f12f', rec, 0) + struct.unpack_from('<8f', rec, 0x60)
        if list(mi) != s['pos'] + s['M'] + s['p60'] + s['p70']:
            inputs_bad += 1
        for off, pl in planes[i].items():
            want = rec[off:off + 16]
            have = struct.pack('<4f', *pl)
            if want == have:
                good[off] += 1
            elif off not in first_bad:
                first_bad[off] = (i, struct.unpack('<4f', want), pl)
    print('%d sections; inputs different from the file: %d' % (len(secs), inputs_bad))
    for off in good:
        print('plane +0x%02x: %d of %d bit-exact' % (off, good[off], len(secs)))
    for off, (i, w, h) in first_bad.items():
        print('  first miss +0x%02x section %d: game %s, here %s' % (off, i, w, h))


if __name__ == '__main__':
    main()
