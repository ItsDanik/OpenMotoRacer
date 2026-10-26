#!/usr/bin/env python3
"""Moto Racer's 3D models (.3DF + .MAP) drawn the way the software renderer draws them.

    python3 -I tools/model.py model DIR/NAME PAGE.RAW out.png [yaw]   one model, alone
    python3 -I tools/model.py dsi BKF MS out.png      the Delphine logo (page 25) at MS ms
    python3 -I tools/model.py title BKF MS out.png [gp]   the title (page 14) at MS ms

BKF is the unpacked banks folder (build/bkf). A reference for the port, written from the
game's code (docs/NOTES.md §4.9 has the addresses and the proofs). Doubles instead of
the game's x87 floats, so a pixel may differ where a value sits on a rounding edge.

Loading (0x4216a0): 3DF vertices x 96.0; each face's indices are reversed by the builder
(0x422c80/0x422be0) and back again (0x423170), the MAP's (u, v) k goes with the file's
index k; then 0x422070 mirrors x (x -> -x) and 0x423170 reverses every face once more
(winding). Drawing kind from the MAP flags (models loaded with the last argument 1):
textured 16, +0x10000 -> 38 (colour 0 see-through), +0x200000 -> 21 (texels halved) or
39 (both); 0x40000 -> 13 (never drawn). 0x420250 centres a model on its bounding box,
0x4204b0 scales it.

Texture pages (0x401160): 24-bit RAW, R G B bytes -> RGB565 as (R>>3)<<11 | (G>>3)<<6 |
(B>>3): green keeps 5 bits, its lowest bit is 0.

Per frame (0x48c030): rotation M = Rz(+8)·Ry(+4)·Rx(+0) for row vectors (angles through the
16384-entry cosine table 0x5fcb80), world = v·M + pos; camera (0x46e140 look-at, at
0x5161c0) cam = (world - campos)·C; projection (0x49b2c0) X*1911/Z + 320, 240 - Y*1911/Z
rounded, only for Z >= 256. Faces (0x49b7f0): back faces and faces wholly beyond one screen
side are dropped, the farthest vertex's Z (+1024 for MAP flag 0x400000) / 4 picks one of
7999 buckets. The buckets are drawn nearest first (0x49ad90, menu mode) into per-row lists
of the still-free pixels (an s-buffer): opaque spans fill and close them, see-through spans
(kinds 38/39) are only recorded and drawn last, farthest first (0x4af130). Each face is a
fan (0,1,2), (0,2,3), cut at Z = 256 (0x49e510), each triangle filled affinely (0x49dde0:
13-bit fixed point edges, one du/dx dv/dx a triangle), texel = page[v>>13 & 255][u>>13 & 255].
"""
import math, struct, sys
from pathlib import Path

W, H = 640, 480
CX, CY = 320, 240            # 0x60d3c4, 0x624f88: half the 640x480 viewport (0x499ba0)
FOCAL = 1911.0               # 0x44eee000, set by the menu (0x40a003) through 0x49a0e0/0x49a100
NEAR = 256.0                 # 0x625774, the menu's (0x40a037)
TO_INDEX = 2607.5945876176133  # double 0x4d1bf8: 16384 / 2pi


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


COS = [f32(math.cos(k * 2 * math.pi / 16384)) for k in range(16384)]  # 0x4901e0


def ftol(x):                 # 0x4b3048: truncation toward zero
    return int(x)


def cdiv(a, b):              # C integer division
    q = abs(a) // abs(b)
    return q if (a >= 0) == (b >= 0) else -q


def trig(angle):
    n = ftol(f32(angle) * TO_INDEX)
    return COS[n & 0x3FFF], COS[(n - 0x1000) & 0x3FFF]


# ---------------------------------------------------------------- loading

def load_page(path):
    """A 256x256 24-bit RAW texture page as 65536 RGB565 texels (0x401160)."""
    d = Path(path).read_bytes()
    assert d[:6] == b"mhwanh"
    w, h, npal = struct.unpack_from(">HHH", d, 8)
    assert npal == 0 and w == 256, "only the 24-bit pages are handled"
    px = d[32:32 + w * h * 3]
    return [(px[i] >> 3) << 11 | (px[i + 1] >> 3) << 6 | (px[i + 2] >> 3)
            for i in range(0, len(px), 3)]


def load_model(base, centre=False, scale=(1.0, 1.0, 1.0), page_add=0):
    """base: path without extension. Returns (vertices, faces)."""
    d = Path(str(base) + ".3DF").read_bytes()
    ver, nv = struct.unpack_from("<HH", d, 0)
    assert ver == 1
    verts = [[f32(c * 96.0) for c in struct.unpack_from("<3f", d, 4 + 12 * i)] for i in range(nv)]
    p, idx = 4 + 12 * nv, []
    while True:
        n, = struct.unpack_from("<H", d, p)
        if n == 0:
            break
        idx.append(list(struct.unpack_from("<%dH" % n, d, p + 4)))
        p += 4 + 2 * n
    m = Path(str(base) + ".MAP").read_bytes()
    assert m[1] & 0x80, "MAP version too old"
    faces = []
    for k, ix in enumerate(idx):
        r = m[0x200 + 0x80 * k:0x280 + 0x80 * k]
        mkind, = struct.unpack_from("<H", r, 0)
        us = struct.unpack_from("<4i", r, 0x22)
        vs = struct.unpack_from("<4i", r, 0x32)
        flags, = struct.unpack_from("<I", r, 0x48)
        colour, = struct.unpack_from("<H", r, 0x4e)
        if mkind < 3:
            kind = 0
            if flags & 0x10000: kind = 2
            if flags & 0x40000: kind = 0xd
            if flags & 0x80000: kind = 0xc
            uv = [(0, 0)] * len(ix)
        else:
            assert mkind == len(ix)
            kind = 0x26 if flags & 0x10000 else 0x10
            if flags & 0x200000:
                kind = 0x27 if flags & 0x10000 else 0x15
            if flags & 0x40000: kind = 0xd
            if flags & 0x80000: kind = 8
            uv = [(us[j], vs[j]) for j in range(len(ix))]
        fl = 4 | (1 if flags & 0x20000 else 0) | (8 if flags & 0x100000 else 0)
        if mkind >= 3 and flags & 0x400000:
            fl |= 2
        faces.append(dict(kind=kind, page=(flags & 0xFFFF) + page_add, colour=colour,
                          idx=ix[::-1], uv=uv[::-1], flags=fl))
    for v in verts:                       # 0x422070: the mirror
        v[0] = -v[0]
    if centre:                            # 0x420250
        c = [(min(v[i] for v in verts) + max(v[i] for v in verts)) * 0.5 for i in range(3)]
        for v in verts:
            for i in range(3):
                v[i] -= c[i]
    for v in verts:                       # 0x4204b0
        for i in range(3):
            v[i] *= scale[i]
    return verts, faces


# ---------------------------------------------------------------- the camera and objects

def mat_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def look_at(pos, target):
    """0x46e140: the camera matrix C (row vectors: cam = (world - pos)·C)."""
    d = [target[i] - pos[i] for i in range(3)]
    l = math.sqrt(sum(c * c for c in d))
    nx, ny, nz = (c / l for c in d)
    h = math.sqrt(nx * nx + nz * nz)
    a = [[nz / h, 0, nx / h], [0, 1, 0], [-nx / h, 0, nz / h]]
    b = [[1, 0, 0], [0, h, ny], [0, -ny, h]]
    return mat_mul(a, b)


def object_matrix(ang):
    """0x48c030: M = Rz(ang[2])·Ry(ang[1])·Rx(ang[0]) for row vectors."""
    ca, sa = trig(ang[0])
    cb, sb = trig(ang[1])
    cc, sc = trig(ang[2])
    return [[cc * cb, sc * ca + cc * sb * sa, sc * sa - cc * sb * ca],
            [-sc * cb, cc * ca - sc * sb * sa, cc * sa + sc * sb * ca],
            [sb, -cb * sa, cb * ca]]


# ---------------------------------------------------------------- the renderer

class Renderer:
    def __init__(self, background=None):
        self.px = list(background) if background else [0] * (W * H)
        self.buckets = {}
        self.free = [[[0, W]] for _ in range(H)]      # 0x623bd8: free spans a row
        self.deferred = []

    def add(self, model, pages, angles, pos, campos, cam):
        verts, faces = model
        m = [[f32(c) for c in row] for row in object_matrix(angles)]
        cam = [[f32(c) for c in row] for row in cam]
        n = [[f32(c) for c in row] for row in mat_mul(m, cam)]
        d = [f32(pos[k] - campos[k]) for k in range(3)]
        t = [f32(sum(d[k] * cam[k][j] for k in range(3))) for j in range(3)]
        # 0x49b2c0 runs with the x87 at 24-bit precision: every step rounded to a float
        nx = [f32(n[k][0] * FOCAL) for k in range(3)] + [f32(t[0] * FOCAL)]
        ny = [f32(n[k][1] * FOCAL) for k in range(3)] + [f32(t[1] * FOCAL)]
        nz = [n[k][2] for k in range(3)] + [t[2]]
        tv = []
        for v in verts:
            X, Y, Z = (f32(f32(f32(f32(v[0] * r[0]) + f32(v[1] * r[1])) + f32(v[2] * r[2])) + r[3])
                       for r in (nx, ny, nz))
            sx = sy = None
            rz = f32(1.0 / Z)
            if Z >= NEAR:
                sx = round(f32(f32(X * rz) + CX))
                sy = round(f32(CY - f32(Y * rz)))
            tv.append((X, Y, Z, sx, sy))
        for f in faces:                       # 0x49b7f0, last argument 1
            if f["kind"] == 0xd:
                continue
            s = f["idx"] + [f["idx"][0]] * (4 - len(f["idx"]))
            p = [tv[i] for i in s]
            far = p[0]
            for q in p[1:]:
                if far[2] < q[2]:
                    far = q
            if not far[2] > NEAR:
                continue
            if any(q[2] <= NEAR for q in p):
                a, b, c = p[1], p[2], p[0]
                f6, f2, f3, f4 = a[1] - b[1], a[2] - b[2], a[1] - c[1], a[2] - c[2]
                area = (far[2] * (f6 * (a[0] - c[0]) - (a[0] - b[0]) * f3)
                        + ((a[0] - b[0]) * f4 - f2 * (a[0] - c[0])) * far[1]
                        + far[0] * (f2 * f3 - f6 * f4))
            else:
                if all(q[3] < 0 for q in p) or all(q[3] > W - 1 for q in p):
                    continue
                if all(q[4] < 0 for q in p) or all(q[4] > H - 1 for q in p):
                    continue
                v0, v1, v2 = p[0], p[1], p[2]
                area = (v2[4] - v1[4]) * (v1[3] - v0[3]) + (v0[4] - v1[4]) * (v2[3] - v1[3])
            if not area > 1.0:
                continue
            key = far[2] + (1024.0 if f["flags"] & 2 else 0.0)
            bucket = ftol(key * 0.25)
            if bucket < 0:
                continue
            bucket = min(bucket, 0x1F3E)
            self.buckets.setdefault(bucket, []).append((f, tv, pages[f["page"]]))

    def render(self):                         # 0x49ad90, menu mode: nearest bucket first
        for b in sorted(self.buckets):
            for f, tv, tex in reversed(self.buckets[b]):
                self.face(f, tv, tex)
        for batch in reversed(self.deferred):  # 0x4af130
            for y, x0, x1, u, v, du, dv, kind, tex in reversed(batch):
                self.span(y, x0, x1, u, v, du, dv, kind, tex)

    def face(self, f, tv, tex):               # 0x49e510
        n = len(f["idx"])
        uvi = [(ftol(u * 8192.0), ftol(v * 8192.0)) for u, v in f["uv"]]   # ftol(u/256 * 2^21)
        for k in range(1, n - 1):
            tri = [(tv[f["idx"][j]], uvi[j]) for j in (0, k, k + 1)]
            behind = [p[0][2] < NEAR for p in tri]
            if not any(behind):
                self.triangle(tri, f["kind"], tex)
            elif behind.count(True) == 2:       # one vertex in front
                r = behind.index(False)
                F, B1, B2 = tri[r], tri[(r + 1) % 3], tri[(r + 2) % 3]
                self.triangle([F, clip(B1, F), clip(B2, F)], f["kind"], tex)
            elif behind.count(True) == 1:       # two in front
                r = behind.index(True)
                B, F1, F2 = tri[r], tri[(r + 1) % 3], tri[(r + 2) % 3]
                P1, P2 = clip(B, F1), clip(B, F2)
                self.triangle([F1, F2, P1], f["kind"], tex)
                self.triangle([F2, P2, P1], f["kind"], tex)

    def triangle(self, tri, kind, tex):       # 0x49dde0
        (x0, y0, u0, v0), (x1, y1, u1, v1), (x2, y2, u2, v2) = [
            (p[0][3], p[0][4], p[1][0], p[1][1]) for p in tri]
        while y1 < y0 or y2 < y0:
            x0, y0, u0, v0, x1, y1, u1, v1, x2, y2, u2, v2 = \
                x1, y1, u1, v1, x2, y2, u2, v2, x0, y0, u0, v0
        A, B = y1 - y2, y0 - y2
        D = ftol((x0 - x2) * A - (x1 - x2) * B)
        if D < 1:
            return
        du = ftol((u0 - u2) / D * A - (u1 - u2) / D * B)
        dv = ftol((v0 - v2) / D * A - (v1 - v2) / D * B)
        deferred = kind in (0x26, 0x27)
        batch = []
        rows = []                             # (y0, count, xl, sl, xr, sr, ul, dul, vl, dvl)
        if y2 < y1:                           # the vertex 2 side has the bend
            if not (y0 < y1 and y1 >= 0):
                return
            sr = cdiv((x1 - x0) * 0x2000, y1 - y0)
            h = y2 - y0
            if h >= 1 and y2 >= 0:
                sl = cdiv((x2 - x0) * 0x2000, h)
                rows.append((y0, h, x0 * 0x2000, sl, x0 * 0x2000, sr,
                             u0, cdiv(u2 - u0, h), v0, cdiv(v2 - v0, h)))
            h = y1 - y2
            if h >= 1:
                sl = cdiv((x1 - x2) * 0x2000, h)
                rows.append((y2, h, x2 * 0x2000, cdiv((x1 - x2) * 0x2000, h),
                             x0 * 0x2000 + (y2 - y0) * sr, sr,
                             u2, cdiv(u1 - u2, h), v2, cdiv(v1 - v2, h)))
        else:
            if not (y0 < y2 and y2 >= 0):
                return
            h = y2 - y0
            sl = cdiv((x2 - x0) * 0x2000, h)
            dul, dvl = cdiv(u2 - u0, h), cdiv(v2 - v0, h)
            h1 = y1 - y0
            if h1 >= 1 and y1 >= 0:
                rows.append((y0, h1, x0 * 0x2000, sl, x0 * 0x2000,
                             cdiv((x1 - x0) * 0x2000, h1), u0, dul, v0, dvl))
            h2 = y2 - y1
            if h2 >= 1:
                rows.append((y1, h2, x0 * 0x2000 + (y1 - y0) * sl, sl, x1 * 0x2000,
                             cdiv((x2 - x1) * 0x2000, h2),
                             u0 + (y1 - y0) * dul, dul, v0 + (y1 - y0) * dvl, dvl))
        for y, cnt, xl, sl, xr, sr, ul, dul, vl, dvl in rows:
            if y < 0:                         # rows above the screen skipped
                xl -= y * sl; xr -= y * sr; ul -= y * dul; vl -= y * dvl
                cnt += y; y = 0
            cnt = min(cnt, H - y)
            for _ in range(cnt):
                self.row(y, xl >> 13, xr >> 13, ul, vl, du, dv, kind, tex, deferred, batch)
                xl += sl; xr += sr; ul += dul; vl += dvl; y += 1
        if batch:
            self.deferred.append(batch)

    def row(self, y, x0, x1, u, v, du, dv, kind, tex, deferred, batch):   # 0x49d9e0 / 0x49dbf0
        free = self.free[y]
        out = []
        for a, b in free:
            s, e = max(a, x0), min(b, x1)
            if s < e:
                us, vs = u + (s - x0) * du, v + (s - x0) * dv
                if deferred:
                    batch.append((y, s, e, us, vs, du, dv, kind, tex))
                else:
                    self.span(y, s, e, us, vs, du, dv, kind, tex)
                    if a < s: out.append([a, s])
                    if e < b: out.append([e, b])
                    continue
            out.append([a, b])
        self.free[y] = out

    def span(self, y, x0, x1, u, v, du, dv, kind, tex):
        base = y * W
        for x in range(x0, x1):
            if kind == 0x10:                                  # 0x4a5ab0
                c = tex[((v >> 13) & 0xFF) << 8 | ((u >> 13) & 0xFF)]
            else:                                             # 0x4a5e10/0x4a5330/0x4a5580
                c = tex[((v >> 13) * 256 + (u >> 13)) & 0xFFFF]
            if kind in (0x26, 0x27) and c == 0:
                pass
            else:
                if kind in (0x15, 0x27):
                    c = (c >> 1) & 0x7BEF
                self.px[base + x] = c
            u += du; v += dv

    def png(self, path):
        from PIL import Image
        b = bytearray()
        for c in self.px:
            r, g, bl = c >> 11, (c >> 5) & 63, c & 31
            b += bytes((r << 3 | r >> 2, g << 2 | g >> 4, bl << 3 | bl >> 2))
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        Image.frombytes("RGB", (W, H), bytes(b)).save(path)


def clip(B, F):
    """0x49d8f0 + 0x49e510: the point at Z = NEAR between B (behind) and F (in front)."""
    (XB, YB, ZB, _, _), (uB, vB) = B
    (XF, YF, ZF, _, _), (uF, vF) = F
    X = (NEAR - ZB) * ((XF - XB) / (ZF - ZB)) + XB
    Y = (NEAR - ZB) * ((YF - YB) / (ZF - ZB)) + YB
    dz = ftol(ZF - ZB)
    t = ftol(NEAR - ZB)
    du = cdiv(uF - uB, dz) if dz else uF - uB
    dv = cdiv(vF - vB, dz) if dz else vF - vB
    return (X, Y, NEAR, CX + ftol(X / NEAR), CY - ftol(Y / NEAR)), (uB + t * du, vB + t * dv)


# ---------------------------------------------------------------- the two pages

def dsi(bkf, ms):
    """Page 25 (start 0x4537d0, tables 0x453610, frame 0x453c50) after ms milliseconds."""
    d = Path(bkf) / "PAGES" / "TITLEDSI"
    pages = {0: load_page(d / "BNK00_00.RAW"), 1: load_page(d / "BNK00_01.RAW")}
    models = [load_model(d / "CAMERA", True, (0.055, 0.055, 0.06))]
    models += [load_model(d / n, True, (0.04, 0.04, 0.04)) for n in ("D", "S", "I", "DSINT")]
    dur = [15.0, 14.5, 14.0, 13.5, 16.0, 20.0]
    start = [(50, 80, -1000), (0, 0, 20000), (0, 0, 20000), (0, 0, 20000),
             (20, 0, 60000), (50, 80, -1000)]
    end = [(50, 0, 7000), (200, 0, 200), (-50, 0, 200), (-250, 0, 200),
           (20, -400, -600), (50, 0, 7000)]
    objs = [[(0.0, f32(-1.701696), 0.0), (-30.0, 0.0, -900.0)]]
    objs += [[(0.0, f32(-math.pi / 2), 0.0), (0.0, 0.0, 0.0)] for _ in range(4)]
    objs[3][0] = objs[4][0] = (0.0, f32(math.pi / 2), 0.0)
    campos = [0.0, 0.0, 4500.0]
    cam = look_at(campos, (0.0, 0.0, 0.0))
    for i in range(6):
        prog = ms / (dur[i] * 1000.0)
        t = min(max(prog, 0.0), 1.0)
        if i == 0:
            objs[0][0] = (0.0, (1 - t) * (math.pi / 2) - 1.701696021, (1 - t * t) * (math.pi / 2))
        elif i in (1, 2, 3):
            x, y, z = end[i]
            n = ftol(t * -2 * math.pi * TO_INDEX)
            c, s = COS[n & 0x3FFF], COS[(n - 0x1000) & 0x3FFF]
            L = math.sqrt(x * x + y * y + z * z)
            R = t * t * -2500.0 + L + 2500.0
            x, y, z = x / L * R, y / L * R, z / L * R
            yaw = (1 - abs(t)) * 8 * math.pi + (math.pi / 2 if i == 3 else -math.pi / 2)
            objs[i] = [(0.0, yaw, 0.0), (x * c + z * s, y, -x * s + z * c - 800.0)]
        else:
            p = tuple((end[i][k] - start[i][k]) * t + start[i][k] for k in range(3))
            if i == 5:
                campos = list(p)
            else:
                objs[i][1] = p
    r = Renderer()
    for i in range(5):
        r.add(models[i], pages, objs[i][0], objs[i][1], campos, cam)
    r.render()
    return r


def title(bkf, ms, gp=False):
    """Page 14 (start 0x4465f0, tables 0x446460, frame 0x446b80) after ms milliseconds."""
    sys.path.insert(0, str(Path(__file__).parent))
    import lez
    d = Path(bkf) / "PAGES" / ("TITLEGP" if gp else "TITLEMR")
    pages = {3: load_page(d / "BNK00_00.RAW")}
    sc = ([(0.04, 0.0325, 0.0455), (0.04, 0.053, 0.04825), (0.04, 0.0315, 0.0315)] if gp else
          [(0.04, 0.03875, 0.04), (0.04, 0.036, 0.0375), (0.04, 0.033, 0.0395)])
    models = [load_model(d / n, False, s, 3) for n, s in zip(("ELLIPSE", "MOTO", "RACER"), sc)]
    dur = [1.0, 1.2, 1.4]
    if gp:
        start = [(69, 26, -40000), (-12000, -90, -798), (12000, 40, -798)]
        end = [(69, 26, -800), (252, -90, -798), (-265, 40, -798)]
    else:
        start = [(60, 42, -40000), (-12000, 44, -798), (12000, 25, -798)]
        end = [(60, 42, -800), (42, 44, -798), (40, 25, -798)]
    arrived = sum(1 for i in range(3) if ms / (dur[i] * 1000.0) > 1.0)
    name = ["TITLE", "TITLE2", "TITLE3", "TITLE4"][min(arrived, 3)] + ("GP" if gp else "")
    w, h, _, _, pal, px = lez.load(Path(bkf) / "MENU" / "BITMAP" / (name + ".LEZ"))
    c565 = [(pal[3 * i] >> 3) << 11 | (pal[3 * i + 1] >> 2) << 5 | pal[3 * i + 2] >> 3
            for i in range(len(pal) // 3)]
    r = Renderer([c565[p] for p in px])
    campos = (0.0, 0.0, 4500.0)
    cam = look_at(campos, (0.0, 0.0, 0.0))
    for i in range(3):
        t = min(max(ms / (dur[i] * 1000.0), 0.0), 1.0)
        ang = (0.0, f32(math.pi / 2), 0.0)
        if i == 0:
            ang = ((t * 2 * math.pi, f32(-math.pi / 2), f32(math.pi)) if t < 0.5 else
                   (0.0, t * 2 * math.pi + math.pi / 2, 0.0))
        pos = tuple((end[i][k] - start[i][k]) * t + start[i][k] for k in range(3))
        if arrived < i + 1:
            r.add(models[i], pages, ang, pos, campos, cam)
    r.render()
    return r


def single(base, page, out, yaw=None):
    model = load_model(base, True)
    vs = model[0]
    size = max(max(v[i] for v in vs) - min(v[i] for v in vs) for i in range(3))
    k = 2000.0 / size                       # about 300 pixels across at 4500
    model = ([[c * k for c in v] for v in vs], model[1])
    pages = {f["page"]: None for f in model[1]}
    tex = load_page(page)
    pages = {p: tex for p in pages}
    campos = (0.0, 0.0, 4500.0)
    cam = look_at(campos, (0.0, 0.0, 0.0))
    r = Renderer()
    r.add(model, pages, (0.0, f32(-math.pi / 2) if yaw is None else yaw, 0.0),
          (0.0, 0.0, 0.0), campos, cam)
    r.render()
    r.png(out)


if __name__ == "__main__":
    a = sys.argv
    if a[1] == "model":
        single(a[2], a[3], a[4], float(a[5]) if len(a) > 5 else None)
    elif a[1] == "dsi":
        dsi(a[2], float(a[3])).png(a[4])
    elif a[1] == "title":
        title(a[2], float(a[3]), len(a) > 5 and a[5] == "gp").png(a[4])
    else:
        sys.exit(__doc__)
