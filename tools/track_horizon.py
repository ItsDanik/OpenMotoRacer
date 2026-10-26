#!/usr/bin/env python3
"""The race's far background (the horizon picture), drawn the way 0x402960 draws it.

Self-contained (no imports from tools/). See docs/NOTES.md §4.13.3 for the addresses.

    draw_horizon(px, raw_path, C, T, cam_y=..., offset=..., scale=..., fx=394.0)

px      list of 640*480 RGB565 ints, painted in place (and returned)
raw_path HORIZHI.RAW (ScreenMode 2, 640x480), HORIZMED.RAW (mode 1), HORIZLOW.RAW (mode 0)
C       the camera's 3x3 as the game stores it at camera +0x68..+0x88 (rows: C[0] = +0x68..,
        C[1] = +0x74.., C[2] = +0x80..); row-vector convention cam = (world - campos)·C
T       camera +0x8c..+0x94 (the extra translation; 0 in the cases seen so far)
cam_y   camera +0x9c (world y of the camera position)
offset  the TRK's int at 0x4db2b0 (signed), scale the TRK's float at 0x4db2b4
        (both read by the TRK loader 0x40d650 just before the "AnimDecor" block)
fx      0x6d20c4: (W*394)/640 at the race (0x4592a0) times 0x6d1f74 (1.0)
"""
import math, struct

def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]

COS = [f32(math.cos(k * 2 * math.pi / 16384)) for k in range(16384)]   # 0x5fcb80 (0x4901e0)
Q = COS[0x3000]                       # 0x608b80, ~-4.4e-8 (cos 3pi/2 through the table)
K = 2607.594587618272                 # double 0x4d00d0 = 16384 / 2pi
PI100 = 314.1592653589793             # double 0x4d00d8
TWO_PI = 6.283185307179586            # double 0x4d00e0
HALF_PI = 1.5707963267948966          # double 0x4d00e8
FAR = 50000.0                         # float 0x4d00f0

# sprite rectangles (x1, y1, x2, y2), inclusive, set up by 0x480930 ("Horizon 1/2 ...")
SPRITES = {2: ((0, 0, 255, 127), (0, 128, 255, 255)),     # HiRes, 0x4eaa3c / 0x4eaa40
           1: ((0, 0, 203, 101), (0, 102, 203, 203)),     # MedRes, 0x4eaa44 / 0x4eaa48
           0: ((0, 0, 255, 63), (0, 0, 255, 63))}         # LowRes, 0x4eaa4c twice


def ftol(x):                          # 0x4b3048: truncation toward zero
    return int(x)


def load_raw565(path):
    """0x477b30: the RAW into a 16-bit surface. 24-bit: R,G,B bytes -> (R>>3)<<11 |
    (G>>2)<<5 | B>>3 (green 6 bits: shifts 0x4ee9b8.. = 8 - bit count, set by 0x473c3c).
    Paletted: index 0 -> 0; an entry with R<8, G<4, B<8 gets R = 8 (keeps it off the
    colour key 0)."""
    d = open(path, "rb").read()
    assert d[:6] == b"mhwanh"
    w, h, npal = struct.unpack_from(">HHH", d, 8)
    out = []
    if npal == 0:
        p = 32
        for _ in range(w * h):
            r, g, b = d[p], d[p + 1], d[p + 2]
            out.append((r >> 3) << 11 | (g >> 2) << 5 | b >> 3)
            p += 3
    else:
        pal = [list(d[32 + 3 * i:35 + 3 * i]) for i in range(npal)]
        for e in pal:
            if e[0] < 8 and e[1] < 4 and e[2] < 8:
                e[0] = 8
        base = 32 + 3 * npal
        for i in range(w * h):
            k = d[base + i]
            if k == 0:
                out.append(0)
            else:
                r, g, b = pal[k]
                out.append((r >> 3) << 11 | (g >> 2) << 5 | b >> 3)
    return w, h, out


def chans(p):                          # 0x478300..: channel bytes of a 565 pixel
    return ((p & 0xF800) >> 11 << 3) & 0xFF, ((p & 0x7E0) >> 5 << 2) & 0xFF, ((p & 0x1F) << 3) & 0xFF


def pack(r, g, b):
    return ((r & 0xFF) >> 3) << 11 | ((g & 0xFF) >> 2) << 5 | (b & 0xFF) >> 3


def fill_colours(w, pix, rect):
    """0x4782e0..0x4785af: +0x46 (drawn above the picture) = mean of the first n pixels of
    row y1 from x1 rightwards, n = clamp(x2-x1, 1, 10); +0x48 (drawn below) = the sums of n2
    pixels of row y2 from x2 leftwards, n2 = clamp(y2-y1, 1, 10), ADDED TO the top mean
    (the accumulators are not reset), divided by n2, each channel then taken & 0xff."""
    x1, y1, x2, y2 = rect
    n = min(max(x2 - x1, 1), 10)
    acc = [0, 0, 0]
    for i in range(n):
        for c, v in enumerate(chans(pix[y1 * w + x1 + i])):
            acc[c] += v
    acc = [a // n for a in acc]
    top = pack(*acc)
    n2 = min(max(y2 - y1, 1), 10)
    for i in range(n2):
        for c, v in enumerate(chans(pix[y2 * w + x2 - i])):
            acc[c] += v
    acc = [a // n2 for a in acc]
    bottom = pack(*acc)
    return top, bottom


def blit(px, W, H, pw, pix, rect, x, y):
    """0x477400 -> 0x4788b0 -> 0x478990: an opaque copy (no colour key: sprite +0x3a = 0),
    clipped to the viewport 0..W-1, 0..H-1; drawn only if the clipped rectangle has
    left < right and top < bottom (inclusive coordinates), so a 1-pixel sliver is not drawn."""
    x1, y1, x2, y2 = rect
    L, T, R, B = x, y, x + (x2 - x1), y + (y2 - y1)
    sl, st = x1, y1
    if T < 0:
        st += -T; T = 0
    if B > H - 1:
        B = H - 1
    if L < 0:
        sl += -L; L = 0
    if R > W - 1:
        R = W - 1
    if not (T < B and L < R):
        return
    for yy in range(T, B + 1):
        srow = (st + yy - T) * pw + sl - L
        drow = yy * W
        for xx in range(L, R + 1):
            px[drow + xx] = pix[srow + xx]


def draw_horizon(px, raw_path, C, T=(0.0, 0.0, 0.0), cam_y=0.0, offset=0, scale=1.0,
                 fx=394.0, W=640, H=480, mode=2, vtop=0):
    pw, ph, pix = load_raw565(raw_path)
    A, Bs = SPRITES[mode]                 # esi = sprite 1, [esp+0x28] = sprite 2
    col46, col48 = fill_colours(pw, pix, A)
    cy = H // 2 + vtop                    # 0x624f88 (0x49a010, no letterbox)
    vbottom = vtop + H - 1                # 0x625784

    # ---- the heading: atan2 of the camera's third row (+ the translation) [asm 0x402990..]
    vx = f32(C[2][0] + T[0]); vy = f32(C[2][1] + T[1]); vz = f32(C[2][2] + T[2])
    L = f32(math.sqrt(vy * vy + vz * vz + vx * vx))
    if L != 0.0:
        nx, nz = f32(vx / L), f32(vz / L)
        yaw = f32(math.atan2(nz / L, nx / L))      # fpatan(st1 = nz/L, st0 = nx/L)
    else:
        yaw = 0.0
    if struct.unpack("<I", struct.pack("<f", yaw))[0] > 0x80000000:   # negative
        y = yaw
        while True:
            y += PI100
            if not y < 0.0:
                break
        yaw = f32(y)
    ym = math.fmod(yaw, TWO_PI)           # 0x4b30ca = _CIfmod, kept as a double
    n = ftol((ym - HALF_PI) * K)
    c = COS[n & 0x3FFF]; s = COS[(n - 0x1000) & 0x3FFF]
    a = f32(-(Q * c)); b = f32(c * COS[0])
    # w = (s, a, b): the horizontal world direction of that heading, into camera space
    px_ = f32(C[0][0] * s + C[1][0] * a + C[2][0] * b + T[0])
    py_ = f32(C[0][1] * s + C[1][1] * a + C[2][1] * b + T[1])
    pz_ = f32(C[0][2] * s + C[1][2] * a + C[2][2] * b + T[2])
    L2 = math.sqrt(py_ * py_ + pz_ * pz_ + px_ * px_)
    sphi = f32(py_ / L2) if L2 else 0.0
    phi = 0.0 if L2 == 0.0 else f32(math.asin(sphi))   # 0x4b3092 = _CIasin

    # ---- x: k periods of the two-tile strip (2*tile width) per turn
    tw = A[2] - A[0] + 1                  # ebp = +0x24 - +0x1c + 1
    th = A[3] - A[1] + 1                  # [esp+0x70] = +0x28 - +0x20 + 1
    k = ((W - 1) * 6) // 640              # 0x62e448 = W-1: 5 at 640, 4 at 512, 2 at 320
    P = TWO_PI / k
    q = ftol(ym / P)
    r = ym - q * P
    m = ftol((2 * tw) * (r / P))
    x = -(2 * tw + m)

    # ---- y: a ground point 50000 ahead and cam_y below, turned by the pitch phi
    i = ftol(phi * K)
    cf = COS[i & 0x3FFF]; sf = COS[(i - 0x1000) & 0x3FFF]
    cf1, sf1 = COS[0] * cf, COS[0] * sf
    a1 = cf1 - (sf * Q) * Q
    b1 = (cf * Q) * Q + sf1
    Yp = cam_y * a1 + (-sf1) * FAR
    Zp = cam_y * b1 + FAR * cf1
    v = ftol(fx / Zp * Yp * scale)
    ytop = v - th - offset + vtop + cy

    # ---- tiles: skip whole tiles left of the screen, counting; odd -> sprite 1 first
    cnt = 0
    while x + tw < 0:
        x += tw; cnt += 1
    first, second = (A, Bs) if cnt & 1 else (Bs, A)
    while x <= W - 1:
        blit(px, W, H, pw, pix, first, x, ytop); x += tw
        blit(px, W, H, pw, pix, second, x, ytop); x += tw

    # ---- colour fills (IDirectDrawSurface::Blt COLORFILL, rectangles end-exclusive)
    top = max(ytop + th, vtop)            # below the picture: +0x48, rows top .. vbottom-1
    for yy in range(top, vbottom):
        for xx in range(0, W):
            px[yy * W + xx] = col48
    bot = min(ytop, vbottom)              # above the picture: +0x46, rows vtop .. bot-1
    for yy in range(vtop, bot):
        for xx in range(0, W):
            px[yy * W + xx] = col46
    return px


if __name__ == "__main__":
    import sys
    from PIL import Image
    raw = sys.argv[1] if len(sys.argv) > 1 else "build/bkf/ILE02/BANK/HORIZHI.RAW"
    out = sys.argv[2] if len(sys.argv) > 2 else "build/png/tracks/horizon_test.png"
    yaw = float(sys.argv[3]) if len(sys.argv) > 3 else 0.0
    # a camera at height 300 looking along world direction yaw, level (look-at style matrix)
    cyw, syw = math.cos(yaw), math.sin(yaw)
    C = [[cyw, 0.0, syw], [0.0, 1.0, 0.0], [-syw, 0.0, cyw]]
    px = [0] * (640 * 480)
    draw_horizon(px, raw, C, cam_y=300.0, offset=5, scale=1.0)
    b = bytearray()
    for c in px:
        r, g, bl = c >> 11, (c >> 5) & 63, c & 31
        b += bytes((r << 3 | r >> 2, g << 2 | g >> 4, bl << 3 | bl >> 2))
    Image.frombytes("RGB", (640, 480), bytes(b)).save(out)
    print(out)
