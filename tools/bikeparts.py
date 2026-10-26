#!/usr/bin/env python3
"""Moto Racer's bike + rider parts: skeletons (.SKL), poses (.ROT), and the editor's
leftovers (.PIV, .SNI) - read the way game.EXE reads them.

    python3 -I tools/bikeparts.py check [BKF]           every SKL/ROT/PIV/SNI of the bike banks
    python3 -I tools/bikeparts.py skl FILE.SKL           one skeleton, bone by bone
    python3 -I tools/bikeparts.py rot FILE.ROT           one pose
    python3 -I tools/bikeparts.py menu BKF FAMILY SKIN THETA out.png [ref.png]
                                                        the bike of page 10 (CHOOSE BIKE)

BKF is the unpacked banks folder (default build/bkf); FAMILY is SPEED, SPEEDPKT, KTMCROSS
or KTMPKT; SKIN the displayed bike 0..7 (its palette); THETA the turntable angle (camera
+0x3c, radians). With ref.png (a 640x480 capture) the bike is drawn over the capture and a
second picture *_diff.png shows where the two differ. The addresses and the proofs are in
build/notes/bike_object.md.

.SKL (read by 0x462c60 through 0x458d40 = i16, 0x458d70 = f32; little endian):
    i16 n; if n < 0 (every shipped file: -1) the "new" format: i16 n again, scale 3.5;
    otherwise the old format, scale 10.0 (coordinates are i16 / 10 truncated).  n <= 18.
    i16 parent[n]                 (file order; -1 = root)
    new format only: i16 slot[n]  (file bone i lives in bone slot slot[i] of the object)
    i16 nv; nv vertices: 3 f32 x scale (old: 3 i16 / 10, truncated)
    per file bone i: pivot 3 f32 x scale (old: 3 i16), i16 first vertex, i16 vertex count,
                     i16 face count, then the faces
    then one more face group (i16 count, faces) kept in slot n (no vertices of its own)
    face (36 bytes): i16 unused, i16 kind (3 -> draw kind 1, 4 -> draw kind 2, other
                     values kept), i16 colour (RGB555), 5 x (i16 vertex, i16 u, i16 v)
.ROT (0x45b340): "ROT\\0", 3 i16 ignored, then up to 20 bones of 3 i16 angles,
    angle = i16 x 2pi/4096 (double 0x4d0e68), stored as float.
"""
import math, struct, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

FAMILIES = ("SPEED", "SPEEDPKT", "KTMCROSS", "KTMPKT")
ROT_UNIT = 0.0015339807878856412       # double 0x4d0e68 = 2pi/4096


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


class Reader:
    def __init__(self, data):
        self.d, self.p = data, 0

    def i16(self):                      # 0x458d40 returns -1 at the end of the file
        if self.p + 2 > len(self.d):
            self.p = len(self.d) + 1
            return -1
        v, = struct.unpack_from("<h", self.d, self.p)
        self.p += 2
        return v

    def f32(self):                      # 0x458d70
        if self.p + 4 > len(self.d):
            self.p = len(self.d) + 1
            return 0.0
        v, = struct.unpack_from("<f", self.d, self.p)
        self.p += 4
        return v


# ---------------------------------------------------------------- .SKL

def kind_of(k, crashed=False):
    """0x462c30: the file's kind to the face's draw kind (0x4ea4d0 is never set here)."""
    if k == 3:
        return 3 if crashed else 1
    if k == 4:
        return 2
    return k


def read_faces(r, count):
    faces = []
    for _ in range(count):
        r.i16()                                  # not used
        kind = r.i16()
        colour = r.i16() & 0xFFFF
        idx, uv = [], []
        for _ in range(5):
            idx.append(r.i16() & 0xFFFF)
            uv.append((r.i16(), r.i16()))
        faces.append(dict(file_kind=kind, kind=kind_of(kind), colour=colour, idx=idx,
                          uv=uv))
    return faces


def read_skl(data):
    r = Reader(data)
    n = r.i16()
    new = n < 0
    if new:
        n = r.i16()
    scale = 3.5 if new else 10.0
    if n > 18:
        raise ValueError("more than 18 bones (%d)" % n)
    parent = [r.i16() for _ in range(n)]
    slot = [r.i16() for _ in range(n)] if new else list(range(n))
    nv = r.i16()
    verts = []
    for _ in range(nv):
        if new:
            x, y, z = r.f32(), r.f32(), r.f32()
            verts.append((f32(x * scale), f32(y * scale), f32(z * scale)))
        else:
            verts.append(tuple(float(int(r.i16() / scale)) for _ in range(3)))
    bones = []
    for i in range(n):
        if new:
            x, y, z = r.f32(), r.f32(), r.f32()
            piv = (f32(x * scale), f32(y * scale), f32(z * scale))
        else:
            piv = tuple(float(int(r.i16() / scale)) for _ in range(3))
        first, count, nf = r.i16(), r.i16(), r.i16()
        bones.append(dict(file=i, slot=slot[i], parent=parent[i], pivot=piv, first=first,
                          count=count, faces=read_faces(r, max(nf, 0))))
    extra = read_faces(r, max(r.i16(), 0))
    return dict(new=new, n=n, parent=parent, slot=slot, verts=verts, bones=bones,
                extra=extra, used=r.p, size=len(data))


def check_skl(s):
    """Problems with a parsed skeleton, as strings."""
    bad = []
    if s["used"] != s["size"]:
        bad.append("read %d of %d bytes" % (s["used"], s["size"]))
    if sorted(s["slot"]) != sorted(set(s["slot"])):
        bad.append("slot map not one-to-one")
    nv = len(s["verts"])
    acc = 0
    for b in s["bones"]:
        if b["count"] and not (0 <= b["first"] and b["first"] + b["count"] <= nv):
            bad.append("bone %d vertices out of range" % b["file"])
        # the drawer transforms the bones' ranges in file order into one output array
        # and every face indexes that array: the ranges must follow each other
        if b["count"] and b["first"] != acc:
            bad.append("bone %d range not contiguous" % b["file"])
        acc += b["count"]
    for f in [f for b in s["bones"] for f in b["faces"]] + s["extra"]:
        n = corners(f)
        if any(i >= acc for i in f["idx"][:n]) or f["idx"][n] != f["idx"][0]:
            bad.append("face %s bad indices" % (f["idx"],))
            break
    return bad


def corners(face):
    """Corner count: 4 when the 5th index is not 0xffff (0x4232e0, 0x423170)."""
    return 4 if face["idx"][4] != 0xFFFF else 3


# ---------------------------------------------------------------- .ROT

def read_rot(data):
    if data[:4] != b"ROT\0":
        raise ValueError("no ROT tag")
    head = struct.unpack_from("<3h", data, 4)
    body = data[10:]
    n = min(len(body) // 6, 20)
    raw = [struct.unpack_from("<3h", body, 6 * k) for k in range(n)]
    ang = [tuple(f32(a * ROT_UNIT) for a in t) for t in raw]
    return dict(head=head, raw=raw, angles=ang, rest=len(body) - 6 * n)


# ---------------------------------------------------------------- editor leftovers

def read_piv(data):
    """.PIV: not read by game.EXE (no reference to the extension or its strings). A dump
    of the modeller's pivot records: u32 count, then records holding a name ("00-corps")
    and the source path of the skeleton (C:\\CUBED\\...\\corps.skl). Only the names and
    paths are listed. [guess]"""
    count, = struct.unpack_from("<I", data, 0)
    names = []
    p = 4
    import re
    for m in re.finditer(rb"[ -~]{6,}", data):
        names.append(m.group().decode("latin-1"))
    return dict(count=count, strings=names)


def read_sni(data):
    """.SNI: not read by game.EXE. "SNI\\0", 3 i16, then "ROT\\0" blocks (an animation
    sequence of the modeller). Lists the blocks' offsets. [guess]"""
    if data[:4] != b"SNI\0":
        raise ValueError("no SNI tag")
    head = struct.unpack_from("<3h", data, 4)
    offs, p = [], 0
    while True:
        p = data.find(b"ROT\0", p)
        if p < 0:
            break
        offs.append(p)
        p += 4
    return dict(head=head, rots=offs)


# ---------------------------------------------------------------- check

def files_of(bkf):
    for fam in FAMILIES + ("PODIUM",):
        d = Path(bkf) / fam
        if not d.is_dir():
            continue
        for f in sorted(d.rglob("*")):
            if f.suffix.upper() in (".SKL", ".ROT", ".PIV", ".SNI"):
                yield fam, f


def check(bkf):
    bad_total = 0
    for fam, f in files_of(bkf):
        data = f.read_bytes()
        name = "%s/%s" % (fam, f.relative_to(Path(bkf) / fam))
        ext = f.suffix.upper()
        try:
            if ext == ".SKL":
                s = read_skl(data)
                bad = check_skl(s)
                nf = sum(len(b["faces"]) for b in s["bones"]) + len(s["extra"])
                kinds = {}
                for b in s["bones"]:
                    for fc in b["faces"] + []:
                        kinds[fc["file_kind"], corners(fc)] = kinds.get((fc["file_kind"], corners(fc)), 0) + 1
                for fc in s["extra"]:
                    kinds[fc["file_kind"], corners(fc)] = kinds.get((fc["file_kind"], corners(fc)), 0) + 1
                flat = sum(1 for b in s["bones"] for fc in b["faces"]
                           if fc["uv"][0] == fc["uv"][1] == fc["uv"][2])
                print("%-28s %s bones %2d  verts %4d  faces %4d (+%d extra)  "
                      "kind,corners %s  flat-uv %d  %s" % (
                          name, "new" if s["new"] else "OLD", s["n"], len(s["verts"]), nf,
                          len(s["extra"]),
                          " ".join("%d,%d:%d" % (k[0], k[1], v) for k, v in sorted(kinds.items())),
                          flat, "OK" if not bad else "BAD " + "; ".join(bad[:3])))
            elif ext == ".ROT":
                r = read_rot(data)
                bad = ["%d bytes left" % r["rest"]] if r["rest"] else []
                print("%-28s head %-16s bones %2d  %s" % (
                    name, r["head"], len(r["raw"]), "OK" if not bad else "BAD " + bad[0]))
            elif ext == ".PIV":
                p = read_piv(data)
                bad = []
                print("%-28s count %d  (editor file) %s" % (
                    name, p["count"], " | ".join(s for s in p["strings"] if "skl" in s.lower())[:90]))
            else:
                s = read_sni(data)
                bad = []
                print("%-28s head %s  ROT blocks %d  (editor file)" % (name, s["head"], len(s["rots"])))
        except Exception as e:  # noqa
            bad = [str(e)]
            print("%-28s BAD %s" % (name, e))
        bad_total += bool(bad)
    print("files with problems: %d" % bad_total)
    return bad_total == 0


def dump_skl(path):
    s = read_skl(Path(path).read_bytes())
    print("%s format, %d bones, %d vertices, read %d of %d bytes" % (
        "new" if s["new"] else "old", s["n"], len(s["verts"]), s["used"], s["size"]))
    for b in s["bones"]:
        print("file %2d slot %2d parent %2d pivot (%8.2f %8.2f %8.2f) verts %4d+%-4d faces %d" % (
            b["file"], b["slot"], b["parent"], *b["pivot"], b["first"], b["count"], len(b["faces"])))
    print("extra group: %d faces" % len(s["extra"]))
    for b in s["bones"][:1]:
        for f in b["faces"][:4]:
            print("  face", f)


def dump_rot(path):
    r = read_rot(Path(path).read_bytes())
    print("head", r["head"])
    for k, (t, a) in enumerate(zip(r["raw"], r["angles"])):
        print("slot %2d  %6d %6d %6d   (%.4f %.4f %.4f)" % (k, *t, *a))


# ---------------------------------------------------------------- the page-10 bike
# A reference for the port, in doubles rounded to floats at the main steps (not the x87
# order of every operation): a pixel may differ on a rounding edge.

def load_bank_page(path, palettes=True):
    """0x401160 with flag 0 on a paletted RAW: 8-bit texels and up to 8 palettes (the RAW's
    own, then "<name>.1" .. "<name>.7"), each RGB565 with green's low bit 0. A 24-bit
    RAW: RGB565 texels (0x401160 with flag 1; with flag 0 a 256x256 24-bit page is not
    read at all)."""
    d = Path(path).read_bytes()
    w, h, npal = struct.unpack_from(">HHH", d, 8)
    conv = lambda r, g, b: (r >> 3) << 11 | (g >> 3) << 6 | (b >> 3)
    if npal == 0:
        px = d[32:32 + w * h * 3]
        return dict(rgb=[conv(px[i], px[i + 1], px[i + 2]) for i in range(0, len(px), 3)])
    pals = [[conv(*d[32 + 3 * k:35 + 3 * k]) for k in range(256)]]
    for n in range(1, 8):
        p = Path(str(path)[:-4] + ".%d" % n)
        if not p.exists():
            break
        e = p.read_bytes()
        pals.append([conv(*e[32 + 3 * k:35 + 3 * k]) for k in range(256)])
    return dict(texels=d[32 + 3 * npal:32 + 3 * npal + w * h], pals=pals)


def rmat(a, b, c):
    """0x40fd40: the row-vector rotation of angles (a, b, c) through the cosine table."""
    import model as M
    ca, sa = M.trig(a)
    cb, sb = M.trig(b)
    cc, sc = M.trig(c)
    t1, t2 = f32(sb * ca), f32(sb * sa)
    return [[f32(cb * cc), f32(t2 * cc + sc * ca), f32(sc * sa - t1 * cc)],
            [f32(-(sc * cb)), f32(ca * cc - t2 * sc), f32(cc * sa + t1 * sc)],
            [sb, f32(-(cb * sa)), f32(cb * ca)]]


def mm(A, B):                        # 0x403a80 on the 3x3 parts: A then B
    return [[f32(sum(A[i][k] * B[k][j] for k in range(3))) for j in range(3)] for i in range(3)]


def vm(v, A, t=(0.0, 0.0, 0.0)):     # 0x410010: v·A + t
    return [f32(sum(v[k] * A[k][j] for k in range(3)) + t[j]) for j in range(3)]


def aff(A, At, B, Bt):               # (A, At) then (B, Bt)
    return mm(A, B), vm(At, B, Bt)


def inv(A, At):                      # the rigid inverse (0x463d90, the draw loop)
    R = [[A[j][i] for j in range(3)] for i in range(3)]
    return R, [f32(-sum(At[k] * A[j][k] for k in range(3))) for j in range(3)]


def angles_of(A):
    """0x46bd00 / 0x46d270: the angles (a, b, c) of a rotation built by rmat."""
    sb = max(-1.0, min(1.0, A[2][0]))
    b = math.asin(sb)
    if abs(math.cos(b)) > 1e-5:
        a = math.atan2(-A[2][1], A[2][2])
        c = math.atan2(-A[1][0], A[0][0])
    else:
        a, c = math.atan2(A[1][2], A[1][1]), 0.0
    return a, b, c


def sub(a, b):
    return [f32(a[i] - b[i]) for i in range(3)]


def add(a, b):
    return [f32(a[i] + b[i]) for i in range(3)]


def scl(s, a):
    return [f32(s * a[i]) for i in range(3)]


def length(a):
    return f32(math.sqrt(sum(c * c for c in a)))


def norm(a):
    l = length(a)
    return [f32(c / l) for c in a] if l else list(a)


def carac(bkf, fam, bike):
    """carac<bike+1>: the floats from +0x14 (phys[0]) on, by their offset."""
    import subprocess
    out = subprocess.run([sys.executable, "-I", str(Path(__file__).parent / "bpt.py"),
                          str(Path(bkf) / fam / ("CARAC%d.BPT" % (bike + 1)))],
                         capture_output=True, text=True).stdout.split("\n")
    vals = [float(l.split("(")[1].rstrip(")")) for l in out if "(" in l]
    return {0x14 + 4 * k: v for k, v in enumerate(vals[5:])}


def load_wheel(base, dirt, pages=None):
    """0x420a50(3df, map, -1, flag = PersoAnim == 1), then centred (0x420250). Kinds: a
    textured face is 0x10 (0x26 see-through) only when flag and its page is not 0, else 1
    (2); 0x200000 -> 0x15 / 0x27. An untextured face's colour is the u16 at MAP +2. A
    textured face whose first three (u, v) are equal becomes kind 0 with the texel as its
    colour (a palette index on a paletted page, RGB565 on a 24-bit one)."""
    import model as M
    verts, faces = M.load_model(base, centre=True)
    m = Path(str(base) + ".MAP").read_bytes()
    for k, f in enumerate(faces):
        r = m[0x200 + 0x80 * k:0x280 + 0x80 * k]
        mkind, c2 = struct.unpack_from("<HH", r, 0)
        f["flags"] &= ~2
        if mkind < 3:
            f["colour"] = c2
            continue
        if f["kind"] in (0x10, 0x26) and not (dirt and f["page"] != 0):
            f["kind"] = 1 if f["kind"] == 0x10 else 2
        pg = (pages or {}).get(f["page"])
        if pg and f["uv"][0] == f["uv"][1] == f["uv"][2]:
            u, v = int(f["uv"][0][0]), int(f["uv"][0][1])
            f["kind"] = 0
            f["colour"] = (pg["texels"] if "texels" in pg else pg["rgb"])[(v << 8) + u]
    return verts, faces


class PalRenderer:
    """model.Renderer with the paletted kinds of 0x49ca70 / 0x4aef20 (see skin_faces)."""

    def __new__(cls, background=None):
        import model as M

        class R(M.Renderer):
            def span(self, y, x0, x1, u, v, du, dv, kind, tex):
                if kind != "p1":
                    return M.Renderer.span(self, y, x0, x1, u, v, du, dv, kind, tex)
                base = y * M.W                    # 0x4a72d0: palette[texel], unlit
                for x in range(x0, x1):
                    self.px[base + x] = tex[((v >> 13) * 256 + (u >> 13)) & 0xFFFF]
                    u += du
                    v += dv
        return R(background)


def skin_faces(faces, page, skin):
    """The faces as 0x49c110 hands them on. On a paletted page (0x49ca70) the texture is
    the page through palette `skin` and the kinds are the paletted ones (0x4aef20):
    0 -> 0x4a3cf0 flat palette[colour]; 1 -> 0x4a7b70 / 0x4a72d0 palette[texel] (unlit at
    light 0x1f); 2 -> 0x4ab070 / 0x4aa6c0 the same skipping a palette colour of 0, recorded
    and drawn after the opaque faces - in the reference renderer: "p1", 0x26 and a flat
    face of the palette colour (green's low bit is already 0, so the 0xFFDF mask of the
    flat drawer changes nothing)."""
    out = []
    for f in faces:
        g = dict(f)
        if page is not None and "pals" in page and f["page"] == 0:
            pal = page["pals"][skin if 0 <= skin < len(page["pals"]) else 0]
            if f["kind"] == 0:
                g["colour"] = pal[f["colour"]] if f["colour"] < 256 else 0
            elif f["kind"] == 1:
                g["kind"] = "p1"
            elif f["kind"] == 2:
                g["kind"] = 0x26
        out.append(g)
    return out


def menu_bike(bkf, fam, skin, theta, background=None):
    import model as M
    bkf = Path(bkf)
    d = bkf / fam
    dirt = fam.startswith("KTM")
    c = carac(bkf, fam, skin)
    page0 = load_bank_page(d / "BNK00_00.RAW")
    pages = {0: page0}
    if dirt:                                       # the second bank, flag = PersoAnim == 1
        pages[1] = load_bank_page(d / "BNK00_01.RAW")
    tex0 = [page0["pals"][min(skin, len(page0["pals"]) - 1)][t] for t in page0["texels"]]
    texmap = {0: tex0, 1: pages.get(1, {}).get("rgb")}

    # ---- the skeleton (0x45bf70 -> 0x462c60), the pose (0x45b480, staticPied.rot)
    s = read_skl((d / "CORPS.SKL").read_bytes())
    n = s["n"]
    slot_of = s["slot"]
    piv = [None] * (n + 1)
    bone = {}
    for b in s["bones"]:
        bone[b["slot"]] = b
    p0 = bone[0]["pivot"]
    verts = [sub(v, p0) for v in s["verts"]]
    for sl, b in bone.items():
        piv[sl] = sub(b["pivot"], p0)
    parent = {b["slot"]: (slot_of[b["parent"]] if b["parent"] >= 0 else None) for b in s["bones"]}
    parent[3] = None                               # unlinked at load
    rot = read_rot((d / "STATICPIED.ROT").read_bytes())
    ang = {k: (rot["angles"][k] if k < len(rot["angles"]) else (0.0, 0.0, 0.0)) for k in range(n)}

    # ---- the object (0x45c7c0(obj, 0, 0, 0)), the menu's state: no lean, pitch, steering,
    # speed, suspension, wheel spin
    B = rmat(0.0, 0.0, 0.0)                        # +0xbc (built from COS[0], COS[0x3000])
    pos = [0.0, 0.0, 0.0]
    rear = vm([0.0, f32(c[0x40] * -96.0), f32(c[0x38] * -48.0)], B, pos)    # +0x338
    front0 = vm([0.0, f32(c[0x3c] * -96.0), f32(c[0x38] * 48.0)], B, pos)   # +0x1b4
    t = p = steer = 0.0
    L298 = mm(rmat(0.0, 0.0, -t), B)
    Mframe = mm(mm(rmat(0.0, 0.0, -t), rmat(-p, 0.0, 0.0)), B)
    Mbody0 = mm(rmat(0.0, 0.0, 0.0), L298)                                   # rear wheel
    s_ang = f32(steer * 1.0 * (2 * math.pi / 8192))
    Mfront = mm(mm(rmat(0.0, 0.0, 0.0), rmat(0.0, s_ang, 0.0)), Mframe)
    dirv = norm(sub(front0, rear))
    front = add(rear, scl(f32(c[0x38] * 96.0), dirv))
    mid = add(rear, scl(f32(c[0x38] * 48.0), dirv))
    y1 = f32(c[0x58] * 96.0)                       # + suspension (+0x2e39) * 40
    h = f32(c[0x40] * 48.0)                        # + c40 * 0.125 * 96 * (+0x2e91)
    Pbody = add(add(mid, vm([0.0, y1, 0.0], Mframe)), vm([0.0, h, 0.0], L298))
    Rrear = rmat(*angles_of(L298))
    head = vm(piv[1], Rrear, Pbody)
    Msteer = mm(rmat(0.0, s_ang, 0.0), Mframe)
    grip = add(head, vm(sub(piv[15], piv[1]), Msteer))
    ln = length(sub(grip, head))
    fdir = norm(sub(grip, head))
    reach = f32(length(sub(front, head)) - f32(c[0x3c] * 96.0))
    axle_f = add(head, scl(reach, fdir))
    q = f32(length(sub(piv[15], piv[1])) * ln / reach)
    piv15s = add(piv[1], scl(q, norm(sub(piv[15], piv[1]))))
    ang[1] = (0.0, f32(steer * 1.0 * -(2 * math.pi / 8192)), 0.0)

    # the swing arm: the angle that turns -z to the rear contact point, about -x (slot 3)
    p3 = vm(piv[3], Rrear, Pbody)
    k = sub(vm([piv[3][0], piv[3][1], f32(piv[3][2] - 1.0)], Rrear, Pbody), p3)
    nn = norm(sub(vm([f32(piv[3][0] - 1.0), piv[3][1], piv[3][2]], Rrear, Pbody), p3))
    a = sub(rear, p3)
    cr = [a[2] * k[1] - a[1] * k[2], a[0] * k[2] - a[2] * k[0], a[1] * k[0] - a[0] * k[1]]
    ka = length(k) * length(a)
    th3 = math.atan2(sum(nn[i] * cr[i] for i in range(3)) / ka, sum(a[i] * k[i] for i in range(3)) / ka)
    phi = f32(c[0x6c] + th3)
    Rphi = rmat(phi, 0.0, 0.0)
    swing_R, swing_T = aff(Rphi, [0.0] * 3, Rrear, Pbody)
    axle_r = vm(piv[16], swing_R, swing_T)

    # ---- bone matrices: 0x463d90(slot 0, Mframe), 0x463d90(slot 3, Mframe), the fork end
    def local(sl, pivot_to=None):
        R = rmat(*ang[sl])
        T = add(vm(scl(-1.0, piv[sl]), R), pivot_to if pivot_to is not None else piv[sl])
        return R, T
    Mb = {}
    Pinv = inv(Mframe, [0.0] * 3)
    def build(sl):
        if sl in Mb:
            return Mb[sl]
        R, T = local(sl)
        par = parent[sl]
        if par is None:
            Mb[sl] = aff(Pinv[0], Pinv[1], R, T)
        else:
            Mb[sl] = aff(*build(par), R, T)
        return Mb[sl]
    for sl in range(n):
        build(sl)
    R15, T15 = local(15, piv15s)
    Mb[15] = aff(*Mb[1], R15, T15)
    # descendants of 15 (none in the shipped skeletons) would keep the old matrix

    # ---- the camera (0x46dab0, 0x46e140)
    ctab, stab = M.trig(theta)
    eye = [f32(1500.0 * stab), 200.0, f32(1500.0 * ctab)]
    C = [[f32(x) for x in row] for row in M.look_at(eye, [0.0, 0.0, 0.0])]
    campos = [eye[0], 240.0, eye[2]]

    r = PalRenderer(background)

    def view(R, T):
        """(R, T) in the world -> the camera: (R·C, (T - campos)·C)."""
        return mm(R, C), vm(sub(T, campos), C)

    # the front wheel (0x48c400(obj+0x1a8, ..., Mfront, axle - front)), then the rear
    for base, Mw, at in (("ROUEAV", Mfront, axle_f), ("ROUEAR", Mbody0, axle_r)):
        wm = load_wheel(d / base, dirt, pages)        # (PKTKTM's family names rouearF.map: not handled)
        wf = skin_faces(wm[1], page0, skin)
        N, T = view(Mw, at)
        r.add_nt((wm[0], wf), {0: tex0, 1: texmap[1]}, N, T)

    # the skeleton's vertices, bone by bone in file order (one output array)
    out_v = []
    for i in range(n):
        sl = slot_of[i]
        b = bone[sl]
        vs = verts[b["first"]:b["first"] + b["count"]]
        if i == 0:
            N, T = view(rmat(*angles_of(Mframe)), Pbody)
        elif i == 3:
            N, T = view(swing_R, swing_T)
        else:
            R, Tt = inv(*Mb[sl])
            N, T = view(R, add(Tt, Pbody))     # Pref = Pbody when not crashed
        out_v.append((vs, N, T))
    # transform each batch, then the faces of slots 0..n (the last is the extra group)
    # every batch with its own matrix into one array in camera space (the faces index it),
    # then projected through an identity view
    camv = [[f32(sum(v[k] * N[k][j] for k in range(3)) + T[j]) for j in range(3)]
            for vs, N, T in out_v for v in vs]
    faces = []
    # the colours as 0x462c60 leaves them on a paletted page 0: a face whose corners 0-2
    # share one (u, v) becomes kind 0 with the texel as its colour - through 0x402020 of
    # the texel taken as RGB555 (= texel & 0xDF) in the bones' loop, raw in the extra
    # group's loop; any other face: the index of its 555->565 colour in palette 0 (or 0)
    pal0 = page0["pals"][0]
    def to565(c):
        return ((c & 0x7C00) << 1) | ((c & 0x3E0) << 1) | (c & 0x1F)
    faces = []
    for sl in range(n + 1):
        fl = bone[sl]["faces"] if sl in bone else s["extra"]
        for f in fl:
            nc = corners(f)
            g = dict(kind=f["kind"], page=0, colour=f["colour"], idx=f["idx"][:nc],
                     uv=[tuple(x) for x in f["uv"][:nc]], flags=4)
            if g["uv"][0] == g["uv"][1] == g["uv"][2]:
                u, v = g["uv"][0]
                g["kind"] = 0
                g["colour"] = page0["texels"][(v << 8) + u] & (0xDF if sl in bone else 0xFF)
            else:
                c = to565(f["colour"])
                g["colour"] = pal0.index(c) if c in pal0 else 0
            faces.append(g)
    sf = skin_faces(faces, page0, skin)
    I = [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]
    r.add_nt((camv, sf), {0: tex0}, I, [0.0, 0.0, 0.0])
    r.render()
    return r, dict(rear=rear, front=front, Pbody=Pbody, head=head, axle_f=axle_f,
                   axle_r=axle_r, phi=phi, th3=th3, reach=reach, q=q, eye=campos)


def menu(argv):
    bkf, fam, skin, theta, out = argv[0], argv[1].upper(), int(argv[2]), float(argv[3]), argv[4]
    ref = argv[5] if len(argv) > 5 else None
    bg = None
    if ref:
        from PIL import Image
        im = Image.open(ref).convert("RGB")
        bg = [(r >> 3) << 11 | (g >> 2) << 5 | (b >> 3) for r, g, b in im.getdata()]
    r, info = menu_bike(bkf, fam, skin, theta, bg)
    for k, v in info.items():
        print(k, v)
    r.png(out)
    if ref:
        black, _ = menu_bike(bkf, fam, skin, theta, None)
        from PIL import Image
        im = Image.open(ref).convert("RGB")
        same = drawn = 0
        diff = Image.new("RGB", (640, 480))
        for i, (rr, gg, bb) in enumerate(im.getdata()):
            c = black.px[i]
            if c:
                drawn += 1
                ok = ((rr >> 3) << 11 | (gg >> 2) << 5 | (bb >> 3)) == c   # exact RGB565
                same += ok
                diff.putpixel((i % 640, i // 640), (0, 160, 0) if ok else (255, 0, 0))
        print("drawn %d, equal to the capture %d (%.2f%%)" % (drawn, same, 100.0 * same / max(drawn, 1)))
        diff.save(out[:-4] + "_diff.png")
    return 0


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 1
    cmd = argv[1]
    if cmd == "check":
        return 0 if check(argv[2] if len(argv) > 2 else "build/bkf") else 1
    if cmd == "skl":
        dump_skl(argv[2])
        return 0
    if cmd == "rot":
        dump_rot(argv[2])
        return 0
    if cmd == "menu":
        return menu(argv[2:])
    print(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
