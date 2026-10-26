#!/usr/bin/env python3
"""A reference for Moto Racer's player-bike step (build/notes/bike_physics.md), for checking
a port against the original's recorded states.

    python3 -I tools/bikephys.py table  build/bkf SPEED 1      the automatic box's upshift table

As a library (the checks build/phys/chk_engine.py, chk_integ.py, chk_steer.py use it):
Carac (a carac row), Body (0x48ef30 engine, 0x48d620 integration, the gearbox, the torque and
rev helpers), steer_466240 (steering angle and yaw), rot_40fd40 / rot_45cb00 / matmul_403a80 /
angles_46d270 / rotate_45cb00 (the bike's rotation through the cosine table), obj_body and
load_record (states recorded from the running original by tools/win/memwatch.c).

The FPU runs at the process default (53-bit mantissa): a Python float is a register value,
f32() is a store to a float. Each function names the game's routine it follows.
REC is a file of tools/win/memwatch.c with the pieces 0x4e9808:4 0x4e980c:4 0x50bbc8:4
0x4dc0b4:4 0x5429b0:4 0x584421:0x3209 (build/phys/drive.sh).
"""
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import bpt  # noqa: E402


def f32(x):
    """A store to a float (round to nearest)."""
    return struct.unpack('<f', struct.pack('<f', x))[0]


def bits(x):
    """A float's bits as a signed int, as the game's integer compares of floats read them."""
    return struct.unpack('<i', struct.pack('<f', x))[0]


# ---------------------------------------------------------------- the carac row
class Carac:
    """One row of 0x509278 (0x124 bytes) from <family>/CARACn.BPT, by the parser 0x42a580."""
    NAMES = {0x14: 'Frein', 0x18: 'MVM', 0x1c: 'Masse', 0x20: 'RetardCadre',
             0x24: 'RoueAccAng', 0x28: 'RoueMaxAccAng', 0x2c: 'RoueVitRetour',
             0x30: 'AngleSusp', 0x34: 'RoueLimAng', 0x38: 'AxeDist', 0x3c: 'RayonAv',
             0x40: 'RayonAr', 0x44: 'Rayon', 0x48: 'RSM', 0x4c: 'VMB', 0x50: 'RotTransfert',
             0x54: 'DivTransfert', 0x58: 'HauteurCadre', 0x5c: 'RessorDec', 0x60: 'RessorMin',
             0x64: 'RessorFactor', 0x68: 'FourcheAv', 0x6c: 'FourcheAr', 0x70: 'NbSpeed',
             0x74: 'RM', 0x90: 'RCM', 0x94: 'CM', 0x98: 'C1000', 0x9c: 'CRM', 0xa0: 'ZoneRouge',
             0xa4: 'MinAdhW'}

    def __init__(self, bkf, family, n, nbspeed_ok=True):
        toks = [v for k, v in bpt.tokens((Path(bkf) / family / ('CARAC%d.BPT' % n)).read_bytes())
                if k == 'n']
        self.raw = {}
        for k, (iv, fv) in enumerate(toks[:0x49]):
            self.raw[k * 4] = iv if k < 5 or k == 0x48 else fv
        self.type = self.raw[0]
        if self.type == 1:                       # 0x42a7c3: dirt bikes' RoueAccelerationAngulaire
            v = self.raw[0x24]
            if bits(v) > 0x40c00000:
                self.raw[0x24] = 6.0
            elif bits(v) > 0x40400000:
                self.raw[0x24] = 3.0
        if not nbspeed_ok:                       # 0x42a8b0: 0x4dc1f4 == 0 (never in GOG)
            self.raw[0x70] = f32(self.raw[0x70] - 1.0)
        for off, name in self.NAMES.items():
            setattr(self, name, self.raw[off])
        self.ratio = [None] + [self.raw[0x74 + 4 * g] for g in range(1, 7)]
        self.dirt = self.type == 1


ENGINE_BRAKE = None   # float table 0x4f17dc + 4g, read by load_exe()


def load_exe(flat='build/mr_flat.bin'):
    global ENGINE_BRAKE
    b = open(flat, 'rb').read()
    ENGINE_BRAKE = [struct.unpack_from('<f', b, 0x4f17dc + 4 * g)[0] for g in range(8)]


# ---------------------------------------------------------------- the engine (body = obj+0x24)
class Body:
    """The fields of the 0x48xxxx class that the engine routines use (body offsets)."""

    def __init__(self, c, automatic=True, countdown=False, protect_ok=True, f8=True):
        self.c = c
        self.gear = 1            # +0x58
        self.timer = 0           # +0x5c
        self.state = 0           # +0x60
        self.automatic = 1 if automatic else 0   # +0x64
        self.pending = 0         # +0x68
        self.rpm_shift = 0.0     # +0x6c
        self.table = [0.0] * 7   # +0x6c + 4g, g = 1..6
        self.top = f32(c.RSM + (0.0 if automatic else 2.0))   # +0x90
        self.power = 1.0         # +0x94
        self.rpm = 0.0           # +0xc8
        self.wheel = 0.0         # +0x108
        self.slide = 0           # +0x118
        self.countdown = countdown
        self.g4dc1fc = 1 if protect_ok else 0
        self.g4dc1f8 = 1 if f8 else 0
        self.m = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0]   # +0x98
        self.air = 0.0           # +0xcc
        self.speed = 0.0         # +0x100
        self.prev_speed = 0.0    # +0x104
        self.dir = [0.0, 0.0, 0.0]   # +0x10c
        self.brakes_grip = 1     # +0x88
        self.brake_ms = 0        # +0x8c
        self.driven = True       # owner obj+8

    # ---- vectors on the body's matrix: R0 = m[0:3] (+0x98), R1 = m[3:6], R2 = m[6:9], T = m[9:12]
    @staticmethod
    def normalised(v):                      # 0x40fcd0
        x, y, z = v
        ln = f32(math.sqrt((x * x + y * y) + z * z))
        if ln != 0.0:
            inv = 1.0 / ln
            return [f32(x * inv), f32(y * inv), f32(z * inv)]
        return list(v)

    def inverse(self):                      # 0x40fe80: transpose, translation -R.T
        m = self.m
        t0 = f32(-((m[2] * m[11] + m[1] * m[10]) + m[9] * m[0]))
        t1 = f32(-((m[5] * m[11] + m[4] * m[10]) + m[9] * m[3]))
        t2 = f32(-((m[8] * m[11] + m[7] * m[10]) + m[9] * m[6]))
        return [m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8], t0, t1, t2]

    @staticmethod
    def mulvec(M, v):                       # 0x410010
        return [f32(((M[6] * v[2] + M[3] * v[1]) + M[0] * v[0]) + M[9]),
                f32(((M[7] * v[2] + M[4] * v[1]) + M[1] * v[0]) + M[10]),
                f32(((M[8] * v[2] + M[5] * v[1]) + M[2] * v[0]) + M[11])]

    def local_velocity(self):               # inline in 0x48ef30 / 0x48cbe0 ...
        d = self.normalised(self.dir)
        v = [f32(d[0] * self.speed), f32(d[1] * self.speed), f32(d[2] * self.speed)]
        return self.mulvec(self.inverse(), v)

    def lateral(self):                      # 0x48d0f0 (returned unrounded)
        d = self.normalised(self.dir)
        v = [f32(d[0] * self.speed), f32(d[1] * self.speed), f32(d[2] * self.speed)]
        M = self.inverse()
        return ((M[6] * v[2] + M[3] * v[1]) + M[0] * v[0]) + M[9]

    def grip(self, lat=None):               # the inline grip factor / 0x48eb40
        c = self.c
        if c.dirt:
            a = abs(self.lateral() if lat is None else lat)
            if a > 0.01 and a < 20.0:
                return f32(a * -0.05 + 1.0)
            return 1.0
        return c.MinAdhW if bits(c.MinAdhW) > 0x3f800000 else 1.0

    def set_forward(self, w):               # 0x48cbe0
        if self.top < w:
            w = self.top
        elif -self.top > w:
            w = f32(-self.top)
        lv = self.local_velocity()
        lx, ly = lv[0], lv[1]
        m = self.m
        nx = f32(((m[3] * ly + m[0] * lx) + m[6] * w) + m[9])
        ny = f32(((m[1] * lx + m[4] * ly) + m[7] * w) + m[10])
        nz = f32((m[8] * w + (m[5] * ly + m[2] * lx)) + m[11])
        sp = f32(math.sqrt((ny * ny + nz * nz) + nx * nx))
        self.speed = sp
        n = [nx, ny, nz]
        if bits(sp) & 0x7fffffff:
            inv = 1.0 / sp
            n = [f32(nx * inv), f32(ny * inv), f32(nz * inv)]
        self.dir = n if sp > 1e-5 else [0.0, 0.0, 0.0]

    def wheel_to_forward(self):             # forward <- blend from the wheel
        lv = self.local_velocity()
        k = self.grip()
        self.set_forward(f32((self.wheel - lv[2]) * k + lv[2]))

    def forward_to_wheel(self):             # wheel <- blend from forward
        lv = self.local_velocity()
        k = self.grip()
        self.wheel = f32((lv[2] - self.wheel) * k + self.wheel)

    def clamp_wheel(self):
        if self.wheel > self.top:
            self.wheel = self.top
        else:
            mt = f32(-self.top)
            if mt > self.wheel:
                self.wheel = mt

    # 0x48dd90: wheel from the engine speed
    def wheel_from_rpm(self, rpm, gear, wheel):
        c = self.c
        if self.state != 0:
            return rpm, wheel
        if gear < 1:
            gear = self.gear
        if float(gear) > c.NbSpeed or rpm < 0.0001:
            return rpm, wheel
        w80 = rpm / c.MVM * c.ratio[gear] * c.VMB
        w = 0.0 if w80 < 0.0 else f32(w80)
        lim = self.top if self.g4dc1fc else f32(self.top - 9.0)
        if w > lim:
            w = lim
            rpm = f32(c.MVM / (c.ratio[gear] * c.VMB) * lim)
        return rpm, w

    # 0x48ef30: engine, brake, slope, drag; d = the bike's forward (T + R2, normalised)
    def engine(self, d, t, scale, dt):
        c = self.c
        free = self.countdown or self.state != 0 or self.slide != 0
        air = (bits(self.air) & 0x7fffffff) != 0
        hold = t < 0 or self.countdown
        cpu = not self.driven
        # 1-2: over the top speed
        if self.speed > self.top:
            self.speed = self.top
            if not air:
                self.forward_to_wheel()
            self.clamp_wheel()
            if not free:
                self.rpm, self.wheel = self.rpm_from_wheel(self.wheel, self.gear, self.rpm)
        if self.wheel > self.top:
            self.wheel = self.top
            if not air:
                self.wheel_to_forward()
            if not free:
                self.rpm, self.wheel = self.rpm_from_wheel(self.wheel, self.gear, self.rpm)
        dt10 = f32(dt * 10.0)
        if dt10 > 500.0:
            dt10 = 500.0
        saved = self.speed
        if t >= 0:
            self.brake_ms = 0
            target80 = (float(t) / float(scale)) * c.MVM
            target = f32(target80)
            if not (target80 < self.rpm) and t > 0:
                if free or air:
                    self.rpm = f32(c.MVM * dt10 * 0.0006 + self.rpm)
                    if self.rpm < 0.0:
                        self.rpm = 0.0
                    if self.rpm > c.MVM:
                        self.rpm = c.MVM
                    if air:
                        self.rpm, self.wheel = self.wheel_from_rpm(self.rpm, self.gear, self.wheel)
                else:
                    idle80 = c.MVM / c.RM * 1200.0
                    if idle80 > self.rpm:
                        self.rpm = f32(idle80)
                    T = self.torque(self.rpm)
                    F = f32(((((c.RM / c.MVM) * self.rpm) * 2.0) * 0.05235987755982988) * T)
                    if self.wheel > 0.0001:
                        a = f32(F / self.wheel)
                    else:
                        a = f32(F * 10000.0)
                    acc = f32(dt10 * a / c.Masse * 0.001)
                    cap = dt10 * 0.03
                    if cap < acc:
                        acc = f32(cap)
                    self.wheel = f32(self.wheel + acc)
                    if self.wheel > self.top:
                        self.wheel = self.top
                    r, self.wheel = self.rpm_from_wheel(self.wheel, self.gear, self.rpm)
                    self.rpm = f32(r)
                    if r > c.MVM:
                        self.rpm = c.MVM
                    elif self.rpm > target:
                        self.rpm = target
                    self.rpm, self.wheel = self.wheel_from_rpm(self.rpm, self.gear, self.wheel)
                    if not air:
                        self.wheel_to_forward()
            else:
                if free or air:
                    self.rpm = f32((dt10 * -0.0006 + 1.0) * self.rpm)
                    if self.rpm < 0.0:
                        self.rpm = 0.0
                    if self.rpm > c.MVM:
                        self.rpm = c.MVM
                    if air:
                        self.rpm, self.wheel = self.wheel_from_rpm(self.rpm, self.gear, self.wheel)
                elif c.MVM / c.RM * 1000.0 < self.rpm:
                    eb = ENGINE_BRAKE[self.gear]
                    self.rpm = f32(eb * self.rpm * dt10 * -0.001 + self.rpm)
                    if self.rpm < 0.0:
                        self.rpm = 0.0
                    self.rpm, self.wheel = self.wheel_from_rpm(self.rpm, self.gear, self.wheel)
                    if not air:
                        self.wheel_to_forward()
        self.slide = 0
        if hold:
            if self.brakes_grip == 0 and not cpu and not self.countdown:
                self.brake_ms = 0
                if c.dirt:
                    self.slide = 1
            else:
                b = c.Frein
                if bits(b) < bits(10.0):
                    b = 10.0
                elif b > 140.0:
                    b = 140.0
                if float(self.brake_ms) < 200.0 and not cpu:
                    b = f32(b * f32(1.0 / 6.0))
                dec = f32(b * 0.2777777777777778 * dt10 * 0.001)
                if bits(self.wheel) > 0:
                    self.wheel = f32(self.wheel - dec)
                    if self.wheel < 0:
                        self.wheel = 0.0
                elif (bits(self.wheel) & 0xffffffff) > 0x80000000:
                    self.wheel = f32(self.wheel + dec)
                    if self.wheel > 0:
                        self.wheel = 0.0
                if not free:
                    self.rpm, self.wheel = self.rpm_from_wheel(self.wheel, self.gear, self.rpm)
                if not air:
                    self.wheel_to_forward()
                self.brake_ms = int(float(self.brake_ms) + dt10)
        if not air:
            fwd = self.local_velocity()[2]
            if not hold or self.speed > 0.1:
                g80 = -d[1] * dt10 * 0.004905
                g = f32(g80)
                if g > 0.55:
                    g = f32(0.55)
                fwd = f32(g + fwd)
            r = f32(0.02)
            if c.dirt:
                r = f32(r * 0.5)
            rr = f32(r * dt10 * 0.00981)
            if bits(self.speed) < bits(20.0):
                rr = f32(rr * self.speed * 0.05)
            if (bits(fwd) & 0xffffffff) > 0x80000000:
                s80 = rr + fwd
                fwd = 0.0 if s80 > 0 else f32(s80)
            else:
                fwd = f32(fwd - rr)
                if fwd < 0:
                    fwd = 0.0
            self.set_forward(fwd)
            self.forward_to_wheel()
            self.clamp_wheel()
            if not free:
                self.rpm, self.wheel = self.rpm_from_wheel(self.wheel, self.gear, self.rpm)
        k = 1.6 if air else 0.3
        dd = self.speed * self.speed * dt10 * f32(k)
        dd = f32(dd / c.Masse * 0.001)
        self.speed = f32(self.speed - dd)
        if self.speed < 0:
            self.speed = 0.0
        if not air:
            lv = self.local_velocity()
            k = self.grip()
            self.wheel = f32((lv[2] - self.wheel) * k + self.wheel)
            self.clamp_wheel()
            if not free:
                self.rpm, self.wheel = self.rpm_from_wheel(self.wheel, self.gear, self.rpm)
        self.prev_speed = saved

    # 0x48d620: integrate one step; radius = obj+0x18 (turn radius, metres), flag = dirt on a straight
    def integrate(self, dt, flag, radius):
        c = self.c
        dtk = f32(dt * f32(0.01))
        old = self.speed
        m = self.m
        Fv = self.mulvec(m, [0.0, 0.0, 1.0])
        F = self.normalised(Fv)
        self.dir = F
        V = self.V
        P, Pm = self.P, self.Pm
        if self.air > 2.592:                               # in the air
            V = self.normalised(V)
            V = [f32(V[0] * self.speed), f32(V[1] * self.speed), f32(V[2] * self.speed)]
            V[1] = f32(V[1] - dtk * 24.525)
            self.speed = f32(math.sqrt((V[1] * V[1] + V[2] * V[2]) + V[0] * V[0]))
            V = self.normalised(V)
            if bits(Pm) & 0x7fffffff:
                P = [f32(P[0] * Pm), f32(P[1] * Pm), f32(P[2] * Pm)]
                P[1] = f32(dtk * -24.525 + P[1])
                Pm = f32(math.sqrt((P[1] * P[1] + P[2] * P[2]) + P[0] * P[0]))
                k = 1.0 / Pm
                P = [f32(P[0] * k), f32(P[1] * k), f32(k * P[2])]
            k = (self.speed * 128.0) * dtk
            d = [f32(V[0] * k), f32(V[1] * k), f32(k * V[2])]
            if self.top < self.speed:
                self.speed = self.top
        else:                                              # on the ground
            cc = (V[1] * F[1] + V[2] * F[2]) + V[0] * F[0]
            if cc < 0.0:
                self.speed = f32(self.speed * abs(f32(cc)))
            k = (11.0 if c.dirt else 6.0) / abs(radius)
            g = f32(1.0 - k)
            if bits(g) > 0x3f800000:
                g = 1.0
            elif g < 0.5:
                g = 0.5
            r80 = self.speed / self.top * 2.0
            r = f32(r80)
            if r80 > 0.9:
                r = f32(0.9)
            if r < 0.85:
                r = f32(0.85)
            w = 1.0 - r
            V = [f32(F[0] * w + V[0] * r), F[1], f32(w * F[2] + V[2] * r)]
            V = self.normalised(V)
            k = ((self.speed * g) * 128.0) * dtk
            d = [f32(V[0] * k), f32(V[1] * k), f32(k * V[2])]
            if flag == 0 or bits(d[1]) <= 0:
                d[1] = f32(dtk * -1255.68 + d[1])
            if bits(Pm) & 0x7fffffff:
                c2 = (P[2] * F[2] + P[1] * F[1]) + P[0] * F[0]
                P = [P[0], -F[1] if c2 < 0.0 else F[1], P[2]]
                P = self.normalised(P)
        kP = (Pm * 128.0) * dtk
        d = [f32(P[0] * kP + d[0]), f32(P[1] * kP + d[1]), f32(kP * P[2] + d[2])]
        self.pos = [f32(d[0] + self.pos[0]), f32(d[1] + self.pos[1]), f32(d[2] + self.pos[2])]
        self.move = d
        if abs(f32(old - self.speed)) > 0.001:
            if (bits(self.air) & 0x7fffffff) == 0:
                a = f32(self.longitudinal())
                self.wheel = f32((a - self.wheel) * self.grip() + self.wheel)
            self.clamp_wheel()
            r, self.wheel = self.rpm_from_wheel(self.wheel, self.gear, self.rpm)
            self.rpm = f32(r)
        dec = f32(dtk * 33.333333333333336)
        if bits(Pm) > 0:
            Pm = f32(Pm - dec)
            if (bits(Pm) & 0xffffffff) > 0x80000000:
                Pm = 0.0
        elif (bits(Pm) & 0xffffffff) > 0x80000000:
            Pm = f32(Pm + dec)
            if bits(Pm) > 0:
                Pm = 0.0
        self.V, self.P, self.Pm = V, P, Pm

    def longitudinal(self):                 # 0x48ca10 (returned unrounded)
        d = self.normalised(self.dir)
        v = [f32(d[0] * self.speed), f32(d[1] * self.speed), f32(d[2] * self.speed)]
        M = self.inverse()
        return ((M[8] * v[2] + M[5] * v[1]) + M[2] * v[0]) + M[11]

    # ---- the gearbox
    def gear_up(self):                      # 0x48e0a0
        c = self.c
        if self.state != 0:
            return
        if c.NbSpeed <= float(self.gear):
            self.gear = int(c.NbSpeed)
        elif float(self.gear + self.pending) < c.NbSpeed:
            self.pending += 1
            self.timer = 200 if self.automatic else 150
            self.state = 1

    def gear_down(self, auto_timer=300):    # 0x48e120 (0x48e550 uses the same with 300 / 200)
        if self.state not in (0, 2):
            return
        if self.gear < 2:
            self.gear = 1
        elif self.gear + self.pending > 1:
            self.pending -= 1
            if self.state != 2:
                self.timer = auto_timer if self.automatic else 200
                self.state = 2

    def auto_box(self):                     # 0x48e550
        c = self.c
        if not self.automatic:
            if (bits(self.table[1]) & 0x7fffffff) == 0:
                self.build_table()
            return
        if self.slide or self.state != 0:
            return
        if bits(self.rpm) < 1:
            self.gear = 1
            return
        if self.countdown:
            return
        if (bits(self.table[1]) & 0x7fffffff) == 0:
            self.build_table()
        g = self.gear
        if float(g) < c.NbSpeed and self.table[g] < self.wheel:
            self.gear_up()
        elif g > 1 and self.table[g - 1] - 2.0 > self.wheel and self.state in (0, 2):
            self.gear_down()

    def shift_step(self, ms):               # 0x48e6e0
        c = self.c
        if self.state == 0:
            return
        self.timer -= ms
        if self.timer >= 1:
            return
        if self.state == 1:
            self.pending = 0
            self.state = 0
            if float(self.gear) < c.NbSpeed:
                self.gear += 1
                if not self.countdown:
                    self.rpm, self.wheel = self.rpm_from_wheel(self.wheel, self.gear, self.rpm)
                    self.rpm = f32(self.rpm)
                    self.rpm, self.wheel = self.wheel_from_rpm(self.rpm, self.gear, self.wheel)
                    if (bits(self.air) & 0x7fffffff) == 0:
                        self.wheel_to_forward()
                self.rpm_shift = self.rpm
            else:
                self.gear = int(c.NbSpeed)
        elif self.state == 2:
            n = abs(self.pending)
            self.state = 0
            self.pending = 0
            self.rpm_shift = self.rpm
            if self.gear < 2:
                self.gear = 1
            else:
                n = min(n, self.gear - 1)
                if n > 0:
                    if self.automatic:
                        while self.gear - n > 1 and self.table[self.gear - n - 1] - 2.0 > self.wheel:
                            n += 1
                    self.gear -= n
                    if not self.countdown:
                        self.rpm, self.wheel = self.rpm_from_wheel(self.wheel, self.gear, self.rpm)
                        self.rpm = f32(self.rpm)
                        self.rpm, self.wheel = self.wheel_from_rpm(self.rpm, self.gear, self.wheel)
        else:
            self.state = 0
            self.pending = 0

    def gear_keys(self, keys, dt, on_ground, latch):   # the gearbox part of 0x466660
        """Returns the new value of the shared latch 0x4ea4c8."""
        if on_ground:
            self.auto_box()
        if self.driven:
            up, down = keys & 0x10, keys & 0x20
            if up and down:
                latch = 1
            elif up:
                if latch == 0:
                    if self.automatic == 0:
                        self.gear_up()
                    latch = 1
            elif down:
                if latch == 0:
                    if self.automatic == 0:
                        self.gear_down()
                    latch = 1
            else:
                latch = 0
        self.shift_step(int(dt * f32(10.0)))
        return latch

    # 0x48deb0: engine speed from the wheel
    def rpm_from_wheel(self, wheel, gear, rpm):
        c = self.c
        if self.state != 0:
            return rpm, wheel
        if gear < 1:
            gear = self.gear
        if bits(wheel) < 0:
            r = rpm
        else:
            r = f32(c.MVM / (c.ratio[gear] * c.VMB) * wheel)
        if bits(r) < 0:
            r = 0.0
        if c.MVM < r:
            r = c.MVM
            wheel = f32(c.ratio[gear] * c.VMB / c.MVM * r)
        return r, wheel

    # 0x48ee30: torque at engine speed e (returned unrounded)
    def torque(self, e):
        c = self.c
        R80 = c.RM / c.MVM * e
        R = f32(R80)
        if R80 < c.RCM:
            slope80 = (c.CM - c.C1000) / (c.RCM - 1000.0)
            slope = f32(slope80)
            icpt = f32(slope80 * -1000.0 + c.C1000)
        else:
            slope80 = (c.CRM - c.CM) / (c.RM - c.RCM)
            slope = f32(slope80)
            icpt = f32(c.CM - c.RCM * slope80)
        T80 = (slope * R + icpt) * 9.81
        T = f32(T80)
        if T80 < 0.2:
            T = f32(0.2)
        if self.g4dc1f8 and self.automatic == 0:
            T = f32(T * 1.1)
        return self.power * T

    # 0x48e3a0: the automatic box's upshift table
    def build_table(self):
        c = self.c
        n = int(c.NbSpeed)
        top = c.RSM
        p = c.ratio[n] * c.VMB
        if p < top:
            top = f32(p)
        g, v = 1, 0.0
        if bits(c.NbSpeed) > 0x3f800000:
            while v < top and float(g) < c.NbSpeed:
                cc = v
                r1, cc = self.rpm_from_wheel(cc, g, 0.0)
                r1 = f32(r1)
                r2, cc = self.rpm_from_wheel(cc, g + 1, 0.0)
                r2 = f32(r2)
                p1 = f32(self.torque(r1) * r1)
                if self.torque(r2) * r2 > p1 or (g < 4 and c.MVM * 0.97 < r1):
                    self.table[g] = v
                    g += 1
                v = f32(v + 0.1)
        while float(g) < c.NbSpeed:
            self.table[g] = top
            g += 1




# ---------------------------------------------------------------- recorded states (memwatch)
def obj_body(c, o, f8=True):
    """A Body from the bytes of a bike object (obj+0 .. obj+0x3209) as recorded."""
    F = lambda off: struct.unpack_from('<f', o, off)[0]
    I = lambda off: struct.unpack_from('<i', o, off)[0]
    b = Body(c, automatic=I(0x88) == 1, f8=f8)
    b.gear, b.timer, b.state, b.automatic = I(0x7c), I(0x80), I(0x84), I(0x88)
    b.pending, b.rpm_shift = I(0x8c), F(0x90)
    b.table = [F(0x90)] + [F(0x90 + 4 * g) for g in range(1, 7)]
    b.brakes_grip, b.brake_ms, b.top, b.power = I(0xac), I(0xb0), F(0xb4), F(0xb8)
    b.m = [F(0xbc + 4 * k) for k in range(12)]
    b.rpm, b.air = F(0xec), F(0xf0)
    b.speed, b.prev_speed, b.wheel = F(0x124), F(0x128), F(0x12c)
    b.dir = [F(0x130), F(0x134), F(0x138)]
    b.slide = I(0x13c)
    b.driven = I(0x8) != 0
    b.pos = [F(0x30), F(0x34), F(0x38)]
    b.P = [F(0x174), F(0x178), F(0x17c)]
    b.Pm = F(0x180)
    b.V = [F(0x18c), F(0x190), F(0x194)]
    return b


def load_record(path):
    """memwatch records (see the module's doc): one per frame, the last sample of each."""
    d = Path(path).read_bytes()
    rs = 24 + 0x3209
    out, last = [], None
    for k in range(len(d) // rs):
        r = d[k * rs:(k + 1) * rs]
        t, dt, ms, stamp, cd, run = struct.unpack_from('<IfiIii', r, 0)
        rec = dict(dt=dt, ms=ms, stamp=stamp, countdown=cd, running=run, obj=r[24:])
        if stamp == last:
            out[-1] = rec
            continue
        last = stamp
        out.append(rec)
    return out


# ---------------------------------------------------------------- rotations (cosine table)
COS = None   # the 16384 floats at 0x5fcb80 (built at start-up; a dump of it: build/phys/costab.bin)


def load_cos(path='build/phys/costab.bin'):
    global COS
    d = Path(path).read_bytes()
    COS = list(struct.unpack('<16384f', d[:0x10000]))


def cos_sin(a, k=2607.594587618272):
    """index = ftol(a * k) (truncation), cos = COS[i & 0x3fff], sin = COS[(i - 0x1000) & 0x3fff].
    sin(0) is COS[0x3000] (about -1.8e-16), not 0."""
    i = int(a * k)
    return COS[i & 0x3fff], COS[(i - 0x1000) & 0x3fff]


def rot_40fd40(a0, a1, a2):
    """0x40fd40: R(a0, a1, a2) from the table, rows X, Y, Z, T = 0 (exact x87 order)."""
    c0, s0 = cos_sin(a0)
    c1, s1 = cos_sin(a1)
    c2, s2 = cos_sin(a2)
    A = f32(s1 * s0)
    B = f32(s1 * c0)
    return [f32(c1 * c2), f32((s1 * s0) * c2 + s2 * c0), f32(s2 * s0 - B * c2),
            f32(-(s2 * c1)), f32(c0 * c2 - A * s2), f32(c2 * s0 + B * s2),
            f32(s1), f32(-(c1 * s0)), f32(c1 * c0),
            0.0, 0.0, 0.0]


def rot_45cb00(a0, a1, a2):
    """The same rotation as built inline in 0x45cb00 (its own x87 order)."""
    c0, s0 = cos_sin(a0)
    c1, s1 = cos_sin(a1)
    c2, s2 = cos_sin(a2)
    A = f32(s1 * c2)
    B = f32(s2 * s1)
    return [f32(c2 * c1), f32(s2 * c0 + A * s0), f32(s2 * s0 - A * c0),
            f32(-(s2 * c1)), f32(c2 * c0 - B * s0), f32(c2 * s0 + B * c0),
            f32(s1), f32(-(c1 * s0)), f32(c1 * c0),
            0.0, 0.0, 0.0]


def matmul_403a80(A, B):
    """0x403a80(out, A, B): out = A*B (3x4 row-vector matrices), the game's sum orders."""
    return [f32(A[2] * B[6] + (A[1] * B[3] + A[0] * B[0])),
            f32((A[0] * B[1] + A[2] * B[7]) + A[1] * B[4]),
            f32((A[0] * B[2] + A[1] * B[5]) + A[2] * B[8]),
            f32((A[3] * B[0] + A[5] * B[6]) + A[4] * B[3]),
            f32((A[5] * B[7] + A[3] * B[1]) + A[4] * B[4]),
            f32((A[5] * B[8] + A[4] * B[5]) + A[3] * B[2]),
            f32((A[6] * B[0] + A[7] * B[3]) + A[8] * B[6]),
            f32((A[6] * B[1] + A[8] * B[7]) + A[7] * B[4]),
            f32((A[7] * B[5] + A[6] * B[2]) + A[8] * B[8]),
            f32(((A[9] * B[0] + A[11] * B[6]) + A[10] * B[3]) + B[9]),
            f32(((A[9] * B[1] + A[11] * B[7]) + A[10] * B[4]) + B[10]),
            f32(((A[9] * B[2] + A[10] * B[5]) + A[11] * B[8]) + B[11])]


def angles_46d270(M):
    """0x46d270: the angles (a0, a1, a2) of a rotation, inverse of rot_*."""
    c = f32(math.sqrt(1.0 - M[6] * M[6]))
    if c <= f32(1e-5):                         # float 0x4d0e50
        a1 = 1.5707963705062866 if not (struct.unpack('<I', struct.pack('<f', M[6]))[0] > 0x80000000) \
            else -1.5707963705062866
        return 0.0, a1, f32(math.atan2(M[1], M[4]))
    a1 = f32(math.atan2(M[6], c))
    a0 = f32(math.atan2(-(M[7] / c), M[8] / c))
    a2 = f32(math.atan2(f32(-(M[3] / c)), f32(M[0] / c)))
    return a0, a1, a2


def rotate_45cb00(R, M):
    """0x45cb00(obj, R): the bike matrix becomes R*M, then is rebuilt from its angles through the
    table (so the heading moves in steps of 2pi/16384), T = 0."""
    P = matmul_403a80(R, M)
    return rot_45cb00(*angles_46d270(P))


def steer_466240(c, st, keys, joyX, dt):
    """0x466240 without its first call: st holds the obj fields it reads and writes:
    s (+0x110), r (+0x10c), speed (+0x124), accel (+0xf8), turbo (+0x307d), sliding (+0x2da0),
    lat0 (0x48d0f0 at entry, float), driven (+8), fkeys (+0xc ? +0x10 : 0), analog (+0x2e29).
    Returns yaw (the angle given to 0x40fd40 as (0, yaw, 0))."""
    K = 0.5555555555555556
    keys |= st.get('fkeys', 0)
    s, r, v = st['s'], st['r'], st['speed']
    RAA, RLA, RVR = c.RoueAccAng, c.RoueLimAng, c.RoueVitRetour
    if joyX != 0x8000:
        n = 0 if v < K else joyX - 0x8000
        st['analog'] = 1
        s = f32((float(n) * RLA) * 3.0517578125e-05)
    elif keys & 8 and v > K:
        st['analog'] = 0
        if bits(s) < 0 and bits(s) != -0x80000000:
            s = 0.0
        r = f32(RAA * dt + r)
        new = r * dt + s
        s = f32(RLA if RLA <= new else new)
    elif keys & 4 and v > K:
        st['analog'] = 0
        if bits(s) > 0:
            s = 0.0
        r = f32(RAA * dt + r)
        new = s - r * dt
        s = f32(-RLA if -RLA >= new else new)
    else:
        ret = st['accel'] * RVR * 0.16666666666666666
        ret = RVR if ret < RVR else f32(ret)
        r = 0.0
        if bits(s) > 0:
            s = f32(s - ret * dt)
            if bits(s) < 0 and bits(s) != -0x80000000:
                s = 0.0
        elif bits(s) < 0 and bits(s) != -0x80000000:
            s = f32(ret * dt + s)
            if bits(s) > 0:
                s = 0.0
    st['s'], st['r'] = s, r
    if not st['driven']:
        T = f32(0.004)
    else:
        f = v / c.VMB * 0.5
        f = 0.0 if f < 0.0 else min(f, 0.5)
        T = c.RotTransfert - (f * c.RotTransfert) * 0.5
        if bits(st['turbo']) & 0x7fffffff:
            T = T * 0.5
        T = f32(T)
    T2 = f32((T * dt) * f32(0.01))
    m = 1.0
    if st['sliding']:
        m = abs(st['lat0']) * 0.1 + 1.0
        m = 1.0 if m < 1.0 else min(m, 1.6)
        m = f32(m)
    if keys & 2 and keys & 0xc:
        m = f32(1.6)
    return f32((m * T2) * s)


def main():
    if sys.argv[1] == 'table':
        c = Carac(sys.argv[2], sys.argv[3], int(sys.argv[4]))
        b = Body(c)
        b.build_table()
        print(' '.join('%.6g' % t for t in b.table[1:int(c.NbSpeed)]))


if __name__ == '__main__':
    main()
