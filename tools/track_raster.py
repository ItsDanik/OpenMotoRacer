#!/usr/bin/env python3
"""The race's software-rasterizer paths that the menu does not use, as tools/model.py's
Renderer extended (docs/NOTES.md §4.13.2).

    RaceRenderer(background)      drop-in for model.Renderer (tools/track.py draws with it)

Kinds added to model.Renderer's 0 (flat), 16, 21, 38, 39:
  sub 6, perspective-correct (polygon 0x4a0f90, triangle 0x4a0700, edges 0x4a0260 / 0x4a04d0)
    28 (0x4a5090 ret: closes the s-buffer, draws nothing), 31 (0x4a5c40 texel),
    32 (0x4a5e90 texel halved), 50 (0x4a50b0 ret, see-through: nothing),
    51 (0x4a5150 face colour where texel != 0), 53 (0x4a53b0 texel, 0 = hole),
    54 (0x4a5610 halved, 0 = hole), 62 (0x4a5890 50 % blend with the screen, 0 = hole)
  sub 4, affine (model.py's 0x49e510 path): 37 (0x4a50c0, see-through: face colour where
    texel != 0)
No kind on these paths reads the vertex light (+0x0c): no gouraud.

Doubles stand for the x87 stack (the renderer runs at the program's default 53-bit
precision: only 0x49b2c0 sets 24-bit, and puts it back); every value the game stores in a
DWORD is rounded to a float (f32) where it does.
"""
import struct, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import model                                    # noqa: E402  (model.NEAR is read live)
from model import W, ftol, cdiv                 # noqa: E402

HALF = 0x7BEF            # 0x50270c: (each channel mask without its low bit) >> 1, RGB565
SEG_MAX = 0x2800         # 0x6d1048: see-through segments a frame (0x1c bytes at 0x68b040)
BATCH_MAX = 0x3E8        # 0x6d1f78: see-through batches a frame (0x24 bytes at 0x625790)
SUB6 = {28, 31, 32, 50, 51, 53, 54, 62}         # kind table 0x4f1a70: sub 6
SEE6 = {50, 51, 53, 54, 62}                     # ... with the see-through flag


def f32(x):
    try:
        return struct.unpack("<f", struct.pack("<f", x))[0]
    except OverflowError:
        return float("inf") if x > 0 else float("-inf")


def i32(x):
    x &= 0xFFFFFFFF
    return x - (1 << 32) if x & 0x80000000 else x


class Stencil:
    """Kind 37's page: the face colour (0x624364) where the texel is not 0."""
    def __init__(self, page, colour):
        self.page, self.colour = page, colour


class RaceRenderer(model.Renderer):
    def __init__(self, background=None, focal=None):
        super().__init__(background, model.FOCAL if focal is None else focal)
        self.room = True          # 0x6d20c0: 0 once a see-through table is full
        self.nseg = 0             # 0x6d1048
        self.nbatch = 0           # 0x6d1f78
        self.detail = 4

    # ---------------------------------------------------------------- dispatch (0x4aef20)

    def face(self, f, tv, tex):
        k = f["kind"]
        if k in SUB6:
            return self.face6(f, tv, tex)
        if k == 0x25:
            tex = Stencil(tex, f["colour"] & 0xFFDF)
        if k in (0, 16, 21, 37, 38, 39):
            return super().face(f, tv, tex)
        # other kinds are not used by tracks (5 = 0x4a4bc0, a bare ret)

    def triangle(self, tri, kind, tex):                 # sub 4: count the batches
        n = len(self.deferred)
        super().triangle(tri, kind, tex)
        self.nbatch += len(self.deferred) - n

    def row(self, y, x0, x1, u, v, du, dv, kind, tex, deferred, batch):
        n = len(batch)
        super().row(y, x0, x1, u, v, du, dv, kind, tex, deferred or kind == 0x25, batch)
        self.nseg += len(batch) - n

    def span(self, y, x0, x1, u, v, du, dv, kind, tex):
        if kind in SUB6:                  # a replayed sub-6 segment: u = (u/z, v/z, 1/z)
            return self.span6(y, x0, x1, u, v, kind, tex)
        if kind == 0x25:                  # 0x4a50c0
            base, page, c = y * W, tex.page, tex.colour
            for x in range(x0, x1):
                if page[((v >> 13) * 256 + (u >> 13)) & 0xFFFF]:
                    self.px[base + x] = c
                u += du; v += dv
            return
        super().span(y, x0, x1, u, v, du, dv, kind, tex)

    # ---------------------------------------------------------------- 0x4a0f90

    def face6(self, f, tv, tex):
        """A fan (0,1,2), (0,2,3); per corner u/z, v/z, 1/z; cut at Z = near."""
        near = model.NEAR
        colour = f["colour"] & 0xFFDF                   # 0x624364 (0x402020 on 5:5:5)
        idx = f["idx"]
        for k in range(1, len(idx) - 1):
            pts = []
            for j in (0, k, k + 1):
                p = tv[idx[j]]
                rz = f32(1.0 / p[2]) if p[2] else float("inf")   # vertex +0x18 (0x49b2c0)
                u, v = f["uv"][j]                       # face (u, v) x 256.0: texels
                pts.append((p, rz, f32(f32(u) * rz), f32(f32(v) * rz)))
            behind = [q[0][2] < near for q in pts]
            nb = behind.count(True)
            if nb == 0:
                tris = [pts]
            elif nb == 1:                               # two in front
                r = behind.index(True)
                B, F1, F2 = pts[r], pts[(r + 1) % 3], pts[(r + 2) % 3]
                P1, P2 = self.clip6(B, F1), self.clip6(B, F2)
                tris = [(F1, F2, P1), (F2, P2, P1)]
            elif nb == 2:                               # one in front
                r = behind.index(False)
                F, B1, B2 = pts[r], pts[(r + 1) % 3], pts[(r + 2) % 3]
                tris = [(F, self.clip6(B1, F, True), self.clip6(B2, F))]
            else:
                continue
            for t in tris:
                self.tri6([(q[0][3], q[0][4], q[1], q[2], q[3]) for q in t],
                          f["kind"], tex, colour)

    def clip6(self, B, F, spill=False):
        """0x49d8f0 for the point (model.clip's geometry, 1/z = 1/near), and the texture
        point in texels in float: u = (uF - uB) / (zF - zB) * (near - zB) + uB. With one
        corner in front, the first clip point's uB, vB, zF - zB, near - zB go through
        float temporaries (0x4a15b4, 0x4a15e5, 0x4a1600, 0x4a160a)."""
        near = model.NEAR
        geo, _ = model.clip((B[0], (0, 0)), (F[0], (0, 0)))
        zB, zF = B[0][2], F[0][2]
        dz, t = zF - zB, near - zB
        uB, vB = B[2] / B[1], B[3] / B[1]
        if spill:
            uB, vB, dz, t = f32(uB), f32(vB), f32(dz), f32(t)
        uF, vF = F[2] / F[1], F[3] / F[1]
        u = (uF - uB) / dz * t + uB
        v = (vF - vB) / dz * t + vB
        return (geo, f32(1.0 / near), f32(u / near), f32(v / near))

    # ---------------------------------------------------------------- 0x4a0700

    def tri6(self, p, kind, tex, colour):
        (x0, y0, r0, u0, v0), (x1, y1, r1, u1, v1), (x2, y2, r2, u2, v2) = p
        while y1 < y0 or y0 > y2:
            x0, y0, r0, u0, v0, x1, y1, r1, u1, v1, x2, y2, r2, u2, v2 = \
                x1, y1, r1, u1, v1, x2, y2, r2, u2, v2, x0, y0, r0, u0, v0
        D = ftol((x0 - x2) * (y1 - y2) - (x1 - x2) * (y0 - y2))
        if D < 1:
            return
        A, C = y1 - y2, y2 - y0
        g = (f32(((u0 - u2) * A + (u1 - u2) * C) / D),       # 0x655d1c d(u/z)/dx
             f32(((v0 - v2) * A + (v1 - v2) * C) / D),       # 0x655d18 d(v/z)/dx
             f32(((r0 - r2) * A + (r1 - r2) * C) / D))       # 0x65e1c0 d(1/z)/dx
        g16 = tuple(16.0 * d for d in g)                     # 0x68b020 0x68b02c 0x625734
        see = kind in SEE6
        Hmax = len(self.free)                                # 0x625784 + 1

        def run(y, cnt, xl, sl, xr, sr, e, de):
            """The row loop (0x4a0ce9): one batch of see-through segments a part."""
            batch = []
            if see and not self.room:
                return xl, xr, e
            for _ in range(cnt):
                if self.free[y]:
                    if see:
                        self.row6s(y, xl >> 13, xr >> 13, e, g, batch)
                    else:
                        self.row6(y, xl >> 13, xr >> 13, e, g, g16, kind, tex, colour)
                e = tuple(f32(a + b) for a, b in zip(e, de))
                xl, xr, y = i32(xl + sl), i32(xr + sr), y + 1
            if see and batch:
                self.deferred.append([(yy, s, t, ee, g16, 0, 0, kind, (tex, colour))
                                      for yy, s, t, ee in batch])
                self.nbatch += 1
                if self.nbatch == BATCH_MAX:
                    self.room = False
            return xl, xr, e

        def steps(ea, eb, h):
            return tuple(f32((b - a) / h) for a, b in zip(ea, eb))

        def back(e, de, y):                       # rows above the screen skipped
            return tuple(f32(a - d * y) for a, d in zip(e, de))

        def bottom(cnt, ylast):
            return cnt + (Hmax - ylast) if Hmax < ylast else cnt

        E0, E1, E2 = (u0, v0, r0), (u1, v1, r1), (u2, v2, r2)
        if y1 <= y2:                              # case A: long edge 0-2 on the left
            if y0 >= y2 or y2 < 0:
                return
            sl = cdiv(i32((x2 - x0) << 13), C)
            del_ = steps(E0, E2, C)
            xl, e = i32(x0 << 13), E0
            done = False
            h = y1 - y0
            if h > 0 and y1 >= 0:
                xr, sr, y = i32(x0 << 13), cdiv(i32((x1 - x0) << 13), h), y0
                if y0 < 0:
                    xl = i32(xl - y0 * sl); e = back(E0, del_, y0)
                    xr = i32(xr - y0 * sr); h += y0; y = 0
                xl, xr, e = run(y, bottom(h, y1), xl, sl, xr, sr, e, del_)
                done = True
            h = y2 - y1
            if h <= 0:
                return
            sr = cdiv(i32((x2 - x1) << 13), h)
            xr, y = i32(x1 << 13), y1
            if y1 < 0:
                xl = i32((x0 << 13) - y0 * sl); e = back(e, del_, y0)
                xr = i32(xr - y1 * sr); h += y1; y = 0
            run(y, bottom(h, y2), xl, sl, xr, sr, e, del_)
        else:                                     # case B: long edge 0-1 on the right
            if y1 <= y0 or y1 < 0:
                return
            sr = cdiv(i32((x1 - x0) << 13), y1 - y0)
            xr = i32(x0 << 13)
            h = C
            if h > 0 and y2 >= 0:
                del_ = steps(E0, E2, C)
                sl, xl, e, y = cdiv(i32((x2 - x0) << 13), C), i32(x0 << 13), E0, y0
                if y0 < 0:
                    xl = i32(xl - y0 * sl); e = back(E0, del_, y0)
                    xr = i32(xr - y0 * sr); h += y0; y = 0
                xl, xr, e = run(y, bottom(h, y2), xl, sl, xr, sr, e, del_)
            h = y1 - y2
            if h <= 0:
                return
            sl = cdiv(i32((x1 - x2) << 13), h)
            del_ = steps(E2, E1, h)
            xl, e, y = i32(x2 << 13), E2, y2
            if y2 < 0:
                xl = i32(xl - y2 * sl); e = back(E2, del_, y2)
                xr = i32((x0 << 13) - y0 * sr); h += y2; y = 0
            run(y, bottom(h, y1), xl, sl, xr, sr, e, del_)

    # ---------------------------------------------------------------- 0x4a0260 / 0x4a04d0

    def row6(self, y, xl, xr, e, g, g16, kind, tex, colour):
        """The opaque edge: the row's free list as the game walks it (a reversed span
        xl > xr draws nothing but can split a node, as in 0x4a2f10)."""
        free, out, i = self.free[y], [], 0
        uz, vz, rz = e
        while i < len(free):
            a, b = free[i]
            if a < xl:
                if b < xl:
                    out.append([a, b]); i += 1; continue
                if b < xr:
                    self.span6(y, xl, b, e, g16, kind, (tex, colour))
                    out.append([a, xl]); i += 1; continue
                self.span6(y, xl, xr, e, g16, kind, (tex, colour))
                out.append([a, xl])
                if b > xr:
                    out.append([xr, b])
                out += free[i + 1:]
                break
            if a > xr:
                out += free[i:]
                break
            n = a - xl
            es = (f32(uz + n * g[0]), f32(vz + n * g[1]), f32(rz + n * g[2]))
            if b > xr:
                self.span6(y, a, xr, es, g16, kind, (tex, colour))
                out.append([xr, b]); out += free[i + 1:]
                break
            self.span6(y, a, b, es, g16, kind, (tex, colour))   # node used up, unlinked
            i += 1
            if b == xr:
                out += free[i:]
                break
        self.free[y] = out

    def row6s(self, y, xl, xr, e, g, batch):
        """The see-through edge: segments recorded for later, the free list untouched."""
        if self.nseg == SEG_MAX:
            self.room = False
            return
        uz, vz, rz = e
        for a, b in self.free[y]:
            if a < xl:
                if b < xl:
                    continue
                s, es = xl, e
            else:
                if xr < a:
                    return
                n = a - xl
                s, es = a, (f32(uz + n * g[0]), f32(vz + n * g[1]), f32(rz + n * g[2]))
            self.nseg += 1
            if xr <= b:
                batch.append((y, s, xr, es))
                return
            batch.append((y, s, b, es))
            if self.nseg == SEG_MAX:
                return

    # ---------------------------------------------------------------- the spans

    def span6(self, y, x0, x1, e, g16, kind, tc):
        """0x4a5c40 and its twins: (u, v) = 8192 (u/z, v/z) / (1/z) exactly every 16
        pixels, linear in between; texel = page[((v >> 13) * 256 + (u >> 13)) & 0xFFFF]."""
        n = x1 - x0
        if n <= 0 or kind in (28, 50):
            return
        tex, colour = tc
        px = self.px
        uz, vz, rz = e
        d16u, d16v, d16r = g16
        z = 8192.0 / rz                                       # 0x4f1a24
        u, v = ftol(uz * z), ftol(vz * z)
        p = y * W + x0
        while n:
            if n >= 16:
                uz, vz, rze = f32(d16u + uz), f32(vz + d16v), d16r + rz
                rz = f32(rze)
                z = 8192.0 / rze
                du = (ftol(uz * z) - u) >> 4
                dv = (ftol(vz * z) - v) >> 4
                k = 16
            else:
                k = n
                m = 0.0625 * k                                # 0x4d1fb8
                uz, vz, rze = f32(uz + d16u * m), f32(vz + d16v * m), rz + d16r * m
                rz = f32(rze)
                inv = 1.0 / k                                 # 0x4d1fc0
                z = 8192.0 / rze
                du = ftol((uz * z - u) * inv)
                dv = ftol((vz * z - v) * inv)
            n -= k
            for _ in range(k):
                c = tex[((v >> 13) * 256 + (u >> 13)) & 0xFFFF]
                if kind == 31:
                    px[p] = c
                elif kind == 32:
                    px[p] = (c >> 1) & HALF
                elif c:
                    if kind == 53:
                        px[p] = c
                    elif kind == 54:
                        px[p] = (c >> 1) & HALF
                    elif kind == 51:
                        px[p] = colour
                    elif kind == 62:
                        px[p] = ((px[p] >> 1) & HALF) + ((c >> 1) & HALF)
                p += 1; u += du; v += dv
