"""The race's bike lighting and shadow, as a reference for the C port (NOTES §4.15.7).

  python3 -I tools/bike_light.py wheels build/bkf FAMILY [X Y Z]
      the light words the lit wheel drawer (0x48c6d0) gives the family's two wheels
      with the wheel placed at X Y Z in the world (identity rotation)

Only the comparison "light word < 0x3e000" ever reaches a pixel: every span routine
the bikes use draws either the full colour or (c >> 1) & 0x7bef. The 8..24 levels
that 0x422540 computes are therefore not visible; they are kept here for the record.
"""
import math
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import model  # noqa: E402

f32, ftol = model.f32, model.ftol

LIGHT = (-0.5, -1.0, 0.0)    # 0x52c228..0x52c230, set by the race load 0x410120
UNLIT = 0x1f << 13           # 0x3e000: the unlit span routines
MASK = 0x7bef                # word 0x50270c


def face_normal(a, b, c):
    """0x41f920: the unit normal of corners 0, 1, 2 (no test for a zero length)."""
    nx = f32((c[2] - a[2]) * (b[1] - a[1]) - (b[2] - a[2]) * (c[1] - a[1]))
    ny = f32((b[2] - a[2]) * (c[0] - a[0]) - (c[2] - a[2]) * (b[0] - a[0]))
    nz = f32((c[1] - a[1]) * (b[0] - a[0]) - (b[1] - a[1]) * (c[0] - a[0]))
    k = 1.0 / math.sqrt(nx * nx + nz * nz + ny * ny)
    return [f32(nx * k), f32(ny * k), f32(nz * k)]


def vertex_normals(verts, faces):
    """0x41fa10 (the wheel models only): every corner of a face (3, or 4 for a quad)
    adds the face's normal to its vertex; the sum times 1/count, then made unit length
    unless zero. A vertex no face uses gets 0 * (1/0): NaN in the game [asm]."""
    acc = [[0.0, 0.0, 0.0] for _ in verts]
    cnt = [0] * len(verts)
    for f in faces:
        ix = f["idx"]
        n = face_normal(verts[ix[0]], verts[ix[1]], verts[ix[2]])
        for i in ix[:4]:
            for j in range(3):
                acc[i][j] = f32(acc[i][j] + n[j])
            cnt[i] += 1
    out = []
    for s, c in zip(acc, cnt):
        if c == 0:
            out.append([math.nan] * 3)
            continue
        k = 1.0 / c
        s = [f32(x * k) for x in s]
        ln = math.sqrt(s[2] * s[2] + s[1] * s[1] + s[0] * s[0])
        if ln != 0.0:
            s = [f32(x / ln) for x in s]
        out.append(s)
    return out


def light_words(normals, rot, pos, light=LIGHT):
    """0x422540: rot = the model-to-world 3x3 (rows), pos = its translation.
    The light vector is taken as a point: brought into the model's frame by the
    inverse of (rot, pos), made unit, negated; per vertex d = n . l,
    a = trunc(16 - 16 d), b = trunc((a + 1) / 2) + 8, at most 31; word = b << 13."""
    rel = [light[i] - pos[i] for i in range(3)]
    p = [sum(rot[i][j] * rel[j] for j in range(3)) for i in range(3)]   # (light - T) . R^T
    ln = math.sqrt((p[1] * p[1] + p[2] * p[2]) + p[0] * p[0])
    if ln != 0.0:
        p = [x / ln for x in p]
    lx, ly, lz = (-p[0], -p[1], -p[2])
    words = []
    for n in normals:
        d = (n[2] * lz + n[1] * ly) + n[0] * lx
        if math.isnan(d):
            words.append(None)
            continue
        a = ftol(16.0 - d * 16.0)
        b = ftol((a + 1.0) * 0.5) + 8
        words.append(min(b, 31) << 13)
    return words


def section_light(lit_flag):
    """0x465d60: 0x1f where the track section's +0xe0 is 0, else 0x10 (0x1f off a track)."""
    return 0x10 if lit_flag else 0x1f


def half(c):
    """The 'lit' spans (0x4a8340, 0x4ab840, kinds 0/42 colour, kind 21/39)."""
    return (c >> 1) & MASK


def shadow_pixel(dst, texel):
    """Kind 44/62 (0x4a57f0, 0x4a5890): texel 0 leaves the screen, else a 50 % blend."""
    return dst if texel == 0 else half(dst) + half(texel)


def main(argv):
    if len(argv) >= 3 and argv[0] == "wheels":
        bkf, fam = Path(argv[1]), argv[2]
        pos = [float(x) for x in argv[3:6]] if len(argv) >= 6 else [0.0, 0.0, 1000.0]
        rot = [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]
        for name in ("ROUEAR", "ROUEAV"):
            verts, faces = model.load_model(bkf / fam / name, centre=True)
            w = light_words(vertex_normals(verts, faces), rot, pos)
            levels = Counter(None if x is None else x >> 13 for x in w)
            lit = sum(1 for x in w if x is not None and x < UNLIT)
            print(f"{fam} {name}: {len(w)} vertices, levels {dict(sorted(levels.items(), key=str))},"
                  f" below 0x3e000: {lit}; vertex 0 -> {'half' if w[0] is not None and w[0] < UNLIT else 'full'}")
        return 0
    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
