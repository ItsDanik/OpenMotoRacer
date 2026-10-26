#!/usr/bin/env python3
"""Moto Racer's race tracks (data/ileNN/*.trk): read, summarise, draw from above.

    python3 -I tools/track.py info  TRK                 summary of one track
    python3 -I tools/track.py top   TRK out.png         top view (road blocks, centre line)
    python3 -I tools/track.py tops  DATA OUTDIR         top view of every data/ile*/ *.trk

The reader follows the game's loader 0x40d650 (docs/NOTES.md §4.13 has the addresses).
File layout, all little-endian, version v (every shipped track is v = 5):

    "TRK\\0", i32 v, i32 nsec
    nsec sections (0x40d650 loop; 0xfc bytes in memory):
        3f pos (+0x00)  12f axes X,Y,Z + offset (+0x0c)  u32 block (+0x3c)
        4 planes of 4f (+0x40 +0x50 +0x60 +0x70; rebuilt at load by 0x40e5c0)
        f line (+0xb0)  [v>0] i32 n, n x u16 decor (+0xd4, +0xb4)  [v>3] f left, f right (+0xf0, +0xf4)
    i32 nblock, nblock models (0x422a20): i32 nv, nv x (3f, u32), i32 nf, nf faces (0x423240):
        u16 kind (+0), u16 flags (+0x38), u16 colour565 (+2), u16 page (+0x36),
        5 u16 corners (+0x2c), 5 x (f u, f v) texels (+4)
    [v>0] i32 nlib, nlib x (i32 len, name, 3 models: near / medium / far)
          i32 ndecor, ndecor x (i32 len, name, 3f pos, 3f angles, i32 lib, [v>4] i32 anim)
    [v>1] i32 nsec, nsec x (i16 checkpoint, u16 time, u16 ?, [v>3] i32 event, i32 ground off,
          i32 ground on, i32 flag)
    [v>2] u32, u32, "AnimDecor\\0" block (0x48b7e0)
"""
import math, os, struct, sys
from pathlib import Path


class Reader:
    def __init__(self, data):
        self.d, self.p = data, 0

    def get(self, fmt):
        v = struct.unpack_from("<" + fmt, self.d, self.p)
        self.p += struct.calcsize("<" + fmt)
        return v

    def one(self, fmt):
        return self.get(fmt)[0]

    def bytes(self, n):
        b = self.d[self.p:self.p + n]
        self.p += n
        return b


def read_model(r):
    """0x422a20 (with faces, last argument 1) and 0x423240 for a version < 6 face."""
    nv = r.one("i")
    verts = []
    for _ in range(nv):
        x, y, z, extra = r.get("3fI")          # the u32 is read into a local and dropped
        verts.append((x, y, z))
    nf = r.one("i")
    faces = []
    for _ in range(nf):
        kind, flags, colour, page = r.get("4H")
        idx = r.get("5H")
        uv = r.get("10f")
        n = 4 if idx[4] != 0xFFFF else 3      # slot 3 = slot 0 closes a triangle
        faces.append(dict(kind=kind, flags=flags, colour=colour, page=page,
                          idx=list(idx[:n]) if idx[4] != 0xFFFF else list(idx[:3]),
                          raw=idx, uv=[(uv[2 * k], uv[2 * k + 1]) for k in range(5)]))  # texels (0x4232e0 then x 1/256)
    return verts, faces


def read_track(path):
    r = Reader(Path(path).read_bytes())
    assert r.bytes(4) == b"TRK\0", path
    t = dict(path=str(path))
    v = t["version"] = r.one("i")
    secs = []
    for i in range(r.one("i")):
        s = dict(pos=r.get("3f"))
        m = r.get("12f")
        s["axes"] = (m[0:3], m[3:6], m[6:9])  # X (across), Y (up), Z (along the track)
        s["offset"] = m[9:12]
        s["block"] = r.one("I")
        s["planes"] = [r.get("4f") for _ in range(4)]
        s["line"] = r.one("f")
        s["decor"] = list(r.get("%dH" % r.one("i"))) if v > 0 else []
        s["edges"] = r.get("2f") if v > 3 else (-1024.0, 1024.0)
        secs.append(s)
    t["sections"] = secs
    t["blocks"] = [read_model(r) for _ in range(r.one("i"))]
    t["lib"], t["decor"] = [], []
    if v > 0:
        for _ in range(r.one("i")):
            name = r.bytes(r.one("i")).split(b"\0")[0].decode("latin-1")
            t["lib"].append(dict(name=name, models=[read_model(r) for _ in range(3)]))
        for _ in range(r.one("i")):
            name = r.bytes(r.one("i")).split(b"\0")[0].decode("latin-1")
            pos, ang = r.get("3f"), r.get("3f")
            lib = r.one("i")
            anim = r.one("i") if v > 4 else -1
            t["decor"].append(dict(name=name, pos=pos, angles=ang, lib=lib, anim=anim))
    if v > 1:
        for s in secs[:r.one("i")]:
            s["checkpoint"], s["time"], s["unknown_de"] = r.get("hHH")
            if v > 3:
                s["event"], s["ground_off"], s["ground_on"], s["flag_e0"] = r.get("4i")
        if v > 2:
            t["globals_4db2b0"] = r.get("2I")
            t["anims"] = read_animdecor(r)
    t["end"] = r.p
    t["size"] = len(r.d)
    return t


def read_animdecor(r):
    """0x48b7e0."""
    assert r.bytes(10) == b"AnimDecor\0"
    ver = r.one("i")
    out = []
    if ver <= 0:
        return out
    for _ in range(r.one("i")):
        rec = r.bytes(0x20 if ver == 1 else 0x40)
        w0, = struct.unpack_from("<I", rec, 0)
        a = dict(type=(w0 >> 16) & 0xFF, sub=w0 >> 24, decor=w0 & 0xFFFF,
                 params=struct.unpack_from("<%df" % (len(rec) // 4 - 1), rec, 4))
        if ver > 2 and a["type"] == 7:
            nfr = r.one("i")
            if nfr > 0:
                npt = r.one("i")
                a["frames"] = [[r.get("3f") for _ in range(npt)] for _ in range(nfr)]
        out.append(a)
    return out


# ---------------------------------------------------------------- summary

def info(path):
    t = read_track(path)
    secs = t["sections"]
    print(f"{path}: version {t['version']}, read to 0x{t['end']:x} of 0x{t['size']:x}")
    nfaces = sum(len(b[1]) for b in t["blocks"])
    print(f"  {len(secs)} sections, {len(t['blocks'])} road blocks ({nfaces} faces), "
          f"{len(t['lib'])} decor models, {len(t['decor'])} decor objects, "
          f"{len(t.get('anims', []))} animations")
    length = sum(math.dist(secs[i]["pos"], secs[(i + 1) % len(secs)]["pos"]) for i in range(len(secs)))
    gap = math.dist(secs[-1]["pos"], secs[0]["pos"])
    print(f"  centre line {length:.0f} units, last section to first {gap:.0f}")
    xs = [s["pos"][0] for s in secs]; ys = [s["pos"][1] for s in secs]; zs = [s["pos"][2] for s in secs]
    print(f"  x {min(xs):.0f}..{max(xs):.0f}  y {min(ys):.0f}..{max(ys):.0f}  z {min(zs):.0f}..{max(zs):.0f}")
    cps = [(i, s["checkpoint"], s["time"]) for i, s in enumerate(secs) if s.get("checkpoint", -1) >= 0]
    print("  checkpoints (section, number, time ms):", cps)
    from collections import Counter
    kinds = Counter(f["kind"] for b in t["blocks"] for f in b[1])
    pages = Counter(f["page"] for b in t["blocks"] for f in b[1])
    print("  road face kinds:", dict(sorted(kinds.items())))
    print("  road texture pages:", dict(sorted(pages.items())))
    print("  decor models:", " ".join(l["name"] for l in t["lib"]))


# ---------------------------------------------------------------- the top view

def rgb(c):
    r, g, b = c >> 11, (c >> 5) & 63, c & 31
    return (r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2)


def top(path, out, size=1100):
    from PIL import Image, ImageDraw
    t = read_track(path)
    secs = t["sections"]
    pts = [v for vs, _ in t["blocks"] for v in vs] + [s["pos"] for s in secs]
    x0, x1 = min(p[0] for p in pts), max(p[0] for p in pts)
    z0, z1 = min(p[2] for p in pts), max(p[2] for p in pts)
    k = (size - 40) / max(x1 - x0, z1 - z0)
    w, h = int((x1 - x0) * k) + 40, int((z1 - z0) * k) + 40
    # seen from above with x to the right and z up (the game mirrors nothing in the data)
    P = lambda p: (20 + (p[0] - x0) * k, h - 20 - (p[2] - z0) * k)
    im = Image.new("RGB", (w, h), (24, 24, 32))
    dr = ImageDraw.Draw(im)
    faces = []
    for vs, fs in t["blocks"]:
        for f in fs:
            if f["kind"] == 0xD:
                continue
            q = [vs[i] for i in f["idx"]]
            faces.append((sum(p[1] for p in q) / len(q), [P(p) for p in q], rgb(f["colour"])))
    faces.sort(key=lambda f: f[0])              # low first, so bridges lie over what is under
    for _, poly, col in faces:
        dr.polygon(poly, fill=col)
    # road edges from the sections (edges are offsets along the X axis)
    for i, s in enumerate(secs):
        n = secs[(i + 1) % len(secs)]
        for e in (0, 1):
            a = [s["pos"][j] + s["axes"][0][j] * s["edges"][e] for j in range(3)]
            b = [n["pos"][j] + n["axes"][0][j] * n["edges"][e] for j in range(3)]
            if math.dist(s["pos"], n["pos"]) < 2048:
                dr.line([P(a), P(b)], fill=(255, 255, 255), width=1)
    cl = [P(s["pos"]) for s in secs]
    dr.line(cl, fill=(255, 40, 40), width=2)
    for i, s in enumerate(secs):               # racing line (+0xb0 across the section)
        if i % 2 == 0:
            p = [s["pos"][j] + s["axes"][0][j] * s["line"] for j in range(3)]
            x, y = P(p)
            dr.point((x, y), fill=(80, 160, 255))
    for d in t["decor"]:
        x, y = P(d["pos"])
        dr.rectangle([x - 1, y - 1, x + 1, y + 1], fill=(255, 220, 0))
    for i, s in enumerate(secs):
        if s.get("checkpoint", -1) >= 0:
            x, y = P(s["pos"])
            dr.ellipse([x - 6, y - 6, x + 6, y + 6], outline=(0, 255, 0), width=2)
            dr.text((x + 8, y - 6), f"CP{s['checkpoint']}", fill=(0, 255, 0))
    x, y = P(secs[0]["pos"])
    dr.text((x + 8, y + 4), "start", fill=(255, 255, 255))
    a = P(secs[0]["pos"]); b = P(secs[min(8, len(secs) - 1)]["pos"])
    dr.line([a, b], fill=(255, 255, 0), width=4)
    dr.text((10, 5), Path(path).name, fill=(255, 255, 255))
    Path(out).parent.mkdir(parents=True, exist_ok=True)
    im.save(out)


# ---------------------------------------------------------------- the race view

def rgb24_page(raw):
    """0x401160 for a 24-bit 256-wide RAW: RGB565 with green's low bit 0 (NOTES 4.9.3.7)."""
    assert raw[:6] == b"mhwanh"
    w, h, npal = struct.unpack_from(">HHH", raw, 8)
    assert npal == 0 and w == 256, "24-bit pages only"
    px = raw[32:32 + w * h * 3]
    tex = [(px[i] >> 3) << 11 | (px[i + 1] >> 3) << 6 | (px[i + 2] >> 3) for i in range(0, len(px), 3)]
    return tex + [0] * (65536 - len(tex))


def bank_pages(bkf_path):
    """Texture pages k = 2..19 = BANK\\BNK00_kk.RAW (0x410590: pages 0 and 1 are the bike's)."""
    sys.path.insert(0, str(Path(__file__).parent))
    from bkf import read_bkf
    files = {n.upper(): d for n, _, _, d in read_bkf(bkf_path)}
    pages = {}
    for k in range(2, 20):
        d = files.get("BANK\\BNK00_%02d.RAW" % k)
        if d:
            pages[k] = rgb24_page(d)
    return pages, files


def plane_dist(pl, p):                       # 0x4b10c0
    return pl[0] * p[0] + pl[1] * p[1] + pl[2] * p[2] + pl[3]


def find_section(t, campos, hint=None):
    """0x40c740 (nearest centre) as the start, then 0x40c7e0's walk on the +0x40 planes."""
    secs = t["sections"]
    n = len(secs)
    if hint is None:
        hint = min(range(n), key=lambda i: math.dist(secs[i]["pos"], campos))
    i = hint
    if plane_dist(secs[i]["planes"][0], campos) >= 0.0:
        while plane_dist(secs[(i + 1) % n]["planes"][0], campos) >= 0.0:
            i = (i + 1) % n
    else:
        while True:
            i = (i - 1) % n
            if plane_dist(secs[i]["planes"][0], campos) >= 0.0:
                break
    return i


def visible(t, sec, C, nblocks=10):
    """0x40ce60: the road blocks and the sections whose decor is drawn, in the game's order.
    C is the camera matrix (rows as stored at camera +0x68); the camera looks along its third
    column. nblocks is track +0x24, 10 from the constructor 0x40c560."""
    secs = t["sections"]
    fwd = (C[0][2], C[1][2], C[2][2])
    pl = secs[sec]["planes"][0]
    d = fwd[0] * pl[0] + fwd[1] * pl[1] + fwd[2] * pl[2]
    if d >= 0.25:                            # double 0x4d02f0
        ahead, behind = nblocks, 1
    elif d > -0.25:                          # double 0x4d02f8
        ahead = behind = nblocks // 2
    else:
        ahead, behind = 1, nblocks
    nb, n = len(t["blocks"]), len(secs)
    b0 = secs[sec]["block"]
    blocks = [b0 % nb] + [(b0 + k) % nb for k in range(1, ahead + 1)] + \
             [(b0 - k) % nb for k in range(1, behind + 1)]
    seclist = [(sec + k) % n for k in range(0, ahead * 4 + 5)] + \
              [(sec - k) % n for k in range(1, behind * 4 + 5)]
    return blocks, seclist


def decor_matrix(ang):
    """0x403880 [asm]: the decor objects' rotation (row vectors), a, b, c = angles +0, +4, +8."""
    sys.path.insert(0, str(Path(__file__).parent))
    from model import trig
    ca, sa = trig(ang[0])
    cb, sb = trig(ang[1])
    cc, sc = trig(ang[2])
    return [[cc * cb + sc * sb * sa, sc * ca, sc * sa * cb - cc * sb],
            [cc * sb * sa - sc * cb, cc * ca, cc * sa * cb + sc * sb],
            [sb * ca, -sa, ca * cb]]


# props of the draw kinds (0x4995a0 builds 0x6d1f88 and the reverse map 0x624b58) for the
# kinds a track uses; detail 4 adds 0x20 (the perspective-correct twin) when the sort key
# is below 6144 (0x49c590). 16->31, 21->32, 38->53, 39->54, 0 stays 0.
NEAR_TWIN = {16: 31, 21: 32, 38: 53, 39: 54}


def remap_kind(kind, key, detail=4, flags=0):
    """0x49c590's detail-level remap (race only: *0x4dc0c4 == 0)."""
    if detail >= 4:
        return NEAR_TWIN.get(kind, kind) if key < 6144.0 else kind
    if detail == 3:
        return NEAR_TWIN.get(kind, kind) if key < 3072.0 else kind
    if kind in (16, 21, 31, 32):             # detail 0-2: textures dropped -> flat colour
        return kind if (detail == 2 and flags & 8) else 0
    if kind in (38, 39):
        return 37 if not (detail == 2 and flags & 8) else kind
    return kind


class Flat:
    """A texture page that is one colour (kind 0: the face's +0x02 RGB565)."""
    def __init__(self, c):
        self.c = c

    def __getitem__(self, i):
        return self.c


def fallback_renderer(model):
    class Fallback(model.Renderer):
        """Draws the perspective twins (31, 32, 53, 54) as their affine kinds and kind 0 / 37
        as a flat colour: an approximation until the sub-6 path is written."""
        def face(self, f, tv, tex):
            k = {31: 16, 32: 21, 53: 38, 54: 39, 0: 16, 37: 38}.get(f["kind"], f["kind"])
            if f["kind"] in (0, 37):
                tex = Flat(f["colour"])
            super().face(dict(f, kind=k), tv, tex)
    return Fallback


def race_view(trk, bkf, campos, C, focal=394.0, out=None, sec=None, detail=4, decor=True,
              background=None):
    """One frame of the race's land + decor through the menu renderer's rules (0x49b2c0,
    0x49b7f0, 0x49ad90) with the race's constants (0x410e10: near 64; 0x4592a0: focal
    394 x zoom at 640x480)."""
    sys.path.insert(0, str(Path(__file__).parent))
    import model
    from track_raster import RaceRenderer as R
    if os.environ.get("OMR_AFFINE"):         # the first attempt: the menu's spans alone
        R = fallback_renderer(model)
    model.FOCAL, model.NEAR = f32(focal), 64.0
    t = read_track(trk) if isinstance(trk, (str, Path)) else trk
    pages, _ = bank_pages(bkf)
    pages.setdefault(0, [0] * 65536)
    if sec is None:
        sec = find_section(t, campos)
    blocks, seclist = visible(t, sec, C)
    r = R(background)
    r.detail = detail

    def faces_of(fs):
        out_ = []
        for f in fs:
            out_.append(dict(kind=f["kind"], page=f["page"], colour=f["colour"],
                             idx=f["idx"][::-1] if False else f["idx"],
                             uv=f["uv"][:len(f["idx"])], flags=f["flags"]))
        return out_

    cache = {}
    for b in blocks:
        vs, fs = t["blocks"][b]
        if b not in cache:
            cache[b] = ([list(v) for v in vs], faces_of(fs))
        add(r, model, cache[b], pages, [[1, 0, 0], [0, 1, 0], [0, 0, 1]], (0.0, 0.0, 0.0), campos, C, detail)
    if decor and detail != 0:
        for s in seclist:
            for di in t["sections"][s]["decor"]:
                d = t["decor"][di]
                if t["lib"][d["lib"]]["name"][:6] == "CAMERA":   # 0x473240 hides them
                    continue
                vs, fs = t["lib"][d["lib"]]["models"][0]
                add(r, model, ([list(v) for v in vs], faces_of(fs)), pages, decor_matrix(d["angles"]),
                    d["pos"], campos, C, detail)
    r.render()
    if out:
        r.png(out)
    return r


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def add(r, model, mdl, pages, M, pos, campos, C, detail):
    """model.Renderer.add with an explicit rotation M (instead of angles) and the race's
    detail-level kind remap applied per face from its sort key."""
    verts, faces = mdl
    W, H, CX, CY = model.W, model.H, model.CX, model.CY
    FOCAL, NEAR = model.FOCAL, model.NEAR
    cam = [[f32(c) for c in row] for row in C]
    n = [[f32(sum(M[i][k] * cam[k][j] for k in range(3))) for j in range(3)] for i in range(3)]
    d = [f32(pos[k] - campos[k]) for k in range(3)]
    tt = [f32(sum(d[k] * cam[k][j] for k in range(3))) for j in range(3)]
    nx = [f32(n[k][0] * FOCAL) for k in range(3)] + [f32(tt[0] * FOCAL)]
    ny = [f32(n[k][1] * FOCAL) for k in range(3)] + [f32(tt[1] * FOCAL)]
    nz = [n[k][2] for k in range(3)] + [tt[2]]
    tv = []
    for v in verts:
        X, Y, Z = (f32(f32(f32(f32(v[0] * q[0]) + f32(v[1] * q[1])) + f32(v[2] * q[2])) + q[3])
                   for q in (nx, ny, nz))
        sx = sy = None
        if Z >= NEAR:
            rz = f32(1.0 / Z)
            sx = round(f32(f32(X * rz) + CX))
            sy = round(f32(CY - f32(Y * rz)))
        tv.append((X, Y, Z, sx, sy))
    for f in faces:
        if f["kind"] == 0xD:
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
        bucket = int(key * 0.25)
        if bucket < 0:
            continue
        bucket = min(bucket, 0x1F3E)
        g = dict(f)
        g["kind"] = remap_kind(f["kind"], key, detail, f["flags"])
        r.buckets.setdefault(bucket, []).append((g, tv, pages.get(f["page"], pages[0])))


if __name__ == "__main__":
    a = sys.argv
    if len(a) >= 3 and a[1] == "info":
        info(a[2])
    elif len(a) >= 4 and a[1] == "top":
        top(a[2], a[3])
    elif len(a) >= 5 and a[1] == "view":
        # view TRK BKF out.png x y z tx ty tz [focal]: a look-at camera (0x46e140)
        sys.path.insert(0, str(Path(__file__).parent))
        from model import look_at
        x, y, z, tx, ty, tz = map(float, a[5:11])
        race_view(a[2], a[3], (x, y, z), look_at((x, y, z), (tx, ty, tz)),
                  float(a[11]) if len(a) > 11 else 394.0, a[4])
    elif len(a) >= 4 and a[1] == "tops":
        for p in sorted(Path(a[2]).glob("ile*/*.trk")) + sorted(Path(a[2]).glob("ile*/*.TRK")):
            info(p)
            top(p, Path(a[3]) / (p.parent.name + ".png"))
    else:
        sys.exit(__doc__)
