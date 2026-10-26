#!/usr/bin/env python3
"""Reference for the race cameras of game.EXE (build/notes/race_view.md §2).

  race_camera.py check [DIR]   compare the camera dumps (*.5161c0.bin, the camera
                               block at 0x5161c0) in DIR (default build/trackcap)
                               with the look-at the game builds (0x46e140)
  race_camera.py demo          run the chase step (0x46f840) on a bike turning
                               at 60 and 13 frames a second, print the lag

The chase step here is the game's arithmetic in Python floats; the game uses x87
with float stores and a 16384-entry cosine table (math.cos here).
"""
import glob, math, os, struct, sys

CAM_SIZE = 0x238


def vsub(a, b): return [a[i] - b[i] for i in range(3)]
def vadd(a, b): return [a[i] + b[i] for i in range(3)]
def vscale(a, s): return [x * s for x in a]


def vlen(v):  # the game sums (y²+z²)+x²
    return math.sqrt((v[1] * v[1] + v[2] * v[2]) + v[0] * v[0])


def vmat(v, m):  # row vector times 3x3
    return [v[0] * m[0][j] + v[1] * m[1][j] + v[2] * m[2][j] for j in range(3)]


def mmul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def roty(b):  # R(0,b,0) as the game builds it (rows)
    c, s = math.cos(b), math.sin(b)
    return [[c, 0, -s], [0, 1, 0], [s, 0, c]]


def lookat(pos, target):
    """0x46e140: yaw then pitch, no roll. Returns C (world->view rows)."""
    u = vsub(target, pos)
    n = vlen(u)
    u = vscale(u, 1 / n)
    h = math.sqrt(u[0] * u[0] + u[2] * u[2])
    A = [[u[2] / h, 0, u[0] / h], [0, 1, 0], [-u[0] / h, 0, u[2] / h]]
    B = [[1, 0, 0], [0, h, u[1]], [0, -u[1], h]]
    return mmul(A, B)


def heading(m):
    """0x490220 on a matrix with T = 0: atan2 of row 2 against row 1."""
    F, U = m[2], m[1]
    nf = vlen(F)
    if nf < 1e-4:
        return math.pi / 2
    return math.atan2((U[0] * F[1] - F[0] * U[1]) / nf, F[2] / nf)


class Chase:
    """The chase state: +0xc8 (lerped eye), +0xa4 (target), +0x98 (eye)."""
    OFFSET = (0.0, 191.0, -484.0)          # +0x50..+0x58
    EXTRA = (0.0, 128.0, 256.0, 512.0)     # +0x224, F5 picks one
    AHEAD = (0.0, -160.0, 1680.0)          # preset 0 offset +0x11c
    HEIGHT = 64.0                          # +0x1dc[0]

    def __init__(self, P, bike_m):
        self.dist = 0
        self.yaw = 0.0                     # +0x3c
        self.finish = False                # +0x34
        W = self._w(bike_m)
        d = self.EXTRA[0] + self.OFFSET[2]
        self.c8 = vadd(vmat([0.0, self.HEIGHT, d], W), P)
        self.a4 = vadd(vmat(list(self.AHEAD), W), P)
        self.pos = list(self.c8)

    def _w(self, bike_m):
        psi = -heading(bike_m)
        return mmul(roty(psi), roty(self.yaw))

    def step(self, P, bike_m, dt):
        d = self.EXTRA[self.dist] + self.OFFSET[2]
        if self.finish:
            if self.yaw > -2.356194490192345:
                self.yaw += dt * -0.010471975511965976
            d -= (self.yaw * -0.4244131815783876) * (self.OFFSET[2] * -0.5)
        W = self._w(bike_m)
        e0 = vadd(vmat([0.0, self.HEIGHT, d], W), P)
        old = self.c8
        pos = vadd(old, vscale(vsub(e0, old), 0.2))      # per frame, not dt
        tgt = vadd(vmat([0.0, 64.0, 0.0] if self.finish else list(self.AHEAD), W), P)
        a = self.a4
        self.a4 = [a[0] + (tgt[0] - a[0]) * 0.5,
                   a[1] + (tgt[1] - a[1]) * 0.3333333432674408,
                   a[2] + (tgt[2] - a[2]) * 0.5]
        C = lookat(pos, self.a4)
        u = vsub(pos, P)
        n = vlen(u)
        if n:
            u = vscale(u, 1 / n)
        self.c8 = pos
        eye = vsub(P, vscale(u, d))
        eye[1] += self.OFFSET[1]
        if self.finish:
            eye[1] -= (self.OFFSET[1] * 0.6666666865348816) * (self.yaw * -0.4244131815783876)
        self.pos = eye
        return C, eye


def read_cam(path):
    b = open(path, 'rb').read()[:CAM_SIZE]
    f = lambda o: struct.unpack_from('<f', b, o)[0]
    i = lambda o: struct.unpack_from('<i', b, o)[0]
    v = lambda o: [f(o + 4 * j) for j in range(3)]
    return {
        'k': i(4), 'mode': i(8), 'preset': i(0x21c),
        'C': [[f(0x68 + 4 * (3 * r + c)) for c in range(3)] for r in range(3)],
        'pos': v(0x98), 'tgt': v(0xa4), 'c8': v(0xc8),
    }


def maxdiff(a, b):
    return max(abs(a[r][c] - b[r][c]) for r in range(3) for c in range(3))


def check(d):
    files = sorted(glob.glob(os.path.join(d, '*.5161c0.bin')))
    if not files:
        print('no camera dumps in', d)
        return
    for p in files:
        cam = read_cam(p)
        name = os.path.basename(p).split('.')[0]
        m = cam['mode']
        if cam['preset']:
            print(f"{name:6s} mode {m} preset {cam['preset']} (bike-mounted, not checked)")
            continue
        src = cam['c8'] if m == 7 else cam['pos']
        try:
            err = maxdiff(lookat(src, cam['tgt']), cam['C'])
        except ZeroDivisionError:
            print(f'{name:6s} mode {m} degenerate')
            continue
        lag = vlen(vsub(cam['c8'], cam['pos']))
        print(f"{name:6s} mode {m} look-at from {'+0xc8' if m == 7 else '+0x98'}: "
              f"max error {err:.1e}   |c8-pos| {lag:7.1f}")


def demo():
    for fps in (60, 13):
        P = [0.0, 0.0, 0.0]
        ang = 0.0
        cam = Chase(P, roty(-ang))
        m = roty(-ang)
        speed = 2000.0                    # units a second
        turn = 1.0                        # radians a second
        for f in range(fps * 3):
            t = 1.0 / fps
            ang += turn * t
            m = roty(-ang)                # the bike's forward is its row 2
            P = vadd(P, vscale(m[2], speed * t))
            C, eye = cam.step(P, m, t * 100.0)
        fwd = m[2]
        look = vsub(P, eye)
        bearing = math.degrees(math.atan2(look[0], look[2]) - math.atan2(fwd[0], fwd[2]))
        bearing = (bearing + 180.0) % 360.0 - 180.0
        print(f'{fps:2d} fps: eye {vlen(vsub(eye, P)):.1f} from the bike, '
              f'{bearing:+.1f} degrees off its heading')


if __name__ == '__main__':
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    if len(sys.argv) > 1 and sys.argv[1] == 'demo':
        demo()
    elif len(sys.argv) > 1 and sys.argv[1] == 'check':
        check(sys.argv[2] if len(sys.argv) > 2 else os.path.join(root, 'build', 'trackcap'))
    else:
        print(__doc__)
