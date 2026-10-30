#ifndef BIKEPHYS_H
#define BIKEPHYS_H

/* A rider's bike in the race: the step of a driven bike (0x45d540 -> 0x45d580) and what it
   calls, the contact with the track (0x468ca0), crashes (0x467e30, 0x4683e0, 0x4681d0) and
   bike against bike (0x47ecd0). The map is build/notes/bike_physics.md; the field names
   follow it, each with its offset in the game's rider record (0x581218 + k x 0x3209).

   Arithmetic as the game's x87 at its 53-bit default: every intermediate a double, every
   store a float. Build without fused multiply-add (-ffp-contract=off) and on SSE2 (no x87
   80-bit), or the last bits change.

   Keys (obj+0x31e9, NOTES §4.14 / 0x431250 and 0x432700), the original's default keys:
     bit0 accelerate (Up)        bit1 brake (Down)       bit2 left (Left)
     bit3 right (Right)          bit4 gear up (A)        bit5 gear down (Z)
   and action 6, turbo / stunt (Space), which is not a bit but turbo_held / turbo_pressed.
   How the key handlers set the bits (a press clears its opposite) is bikephys_key().

   Use: bikephys_track() after each land_load; at the race start (0x411010) g_bikeworld.crt
   = 0 (srand(0)), .reverse, .mode, .followed, .rider[number] = the riders; bikephys_init()
   for each rider at its grid place; then each race frame, for each driven bike,
   bikephys_step(), and after all of them bikephys_collide().

   What the drawing reads: pos, m (the bike's matrix, rows X Y Z), frame (the frame's
   matrix), lean, pitch, susp, steer, spin_rear / spin_front, rear / front (the wheel
   points), wheelie, sec, crashed with tumble / tumble_pt, kmh_shown, rpm, gear, events (the
   sounds). The game rewrites the front point +0x1b4 after the step by the last bit (most
   likely its drawing); the physics never reads it before 0x469c60 makes it again. */

#include "pose.h"

enum {
    BK_ACCEL = 1, BK_BRAKE = 2, BK_LEFT = 4, BK_RIGHT = 8, BK_GEAR_UP = 16, BK_GEAR_DOWN = 32
};

typedef struct BikePhys {
    /* who drives */
    int driven;                 /* +0x08: driven by keys / a joystick (else the computer) */
    int human;                  /* +0x04: a local player */
    int forced, forced_keys;    /* +0x0c, +0x10: keys added while pushed (+0x10 or-ed in) */
    float radius;               /* +0x18: the turn radius in metres (FLT_MAX: none) */
    int bike;                   /* +0x78: the carac row 0..7 (g_bikes) */
    int number;                 /* +0x2dd0: the rider's number 1..24 */
    int state;                  /* +0x2de5: the race state (0, 1 racing; else brakes) */

    /* the body (obj+0x24, the class at 0x48xxxx) */
    float pos[3];               /* +0x30 (body+0x0c): 128 units a metre */
    float pos1[3], pos2[3];     /* +0x3c, +0x48: one and two steps ago */
    float pos_dt;               /* +0x54: the step's dt */
    int gear;                   /* +0x7c: 1..NbSpeed */
    int shift_timer;            /* +0x80: ms */
    int shift;                  /* +0x84: 0, 1 shifting up, 2 shifting down */
    int automatic;              /* +0x88: 1 automatic box, 0 manual (0x4dc0a0) */
    int pending;                /* +0x8c: gears still to change */
    float table[7];             /* +0x90: [0] the engine at the last shift, [g] the road
                                   speed the automatic box leaves gear g at (0x48e3a0) */
    int brakes_grip;            /* +0xac: 0 = a player's dirt bike braking while steering */
    int brake_ms;               /* +0xb0: how long the brake has been held */
    float top;                  /* +0xb4: RealSpeedMax (+2 with the manual box) */
    float power;                /* +0xb8: 1.0, 1.2 or 2.0 with the turbo */
    float m[12];                /* +0xbc: rows X (right), Y (up), Z (forward), T (0) */
    float engine;               /* +0xec: 0..MoteurVitesseMax */
    float air;                  /* +0xf0: 0 on the ground, +2.592 a step in the air */
    float half;                 /* +0xf4: smoothed half change of the forward speed */
    float accel;                /* +0xf8: forward acceleration, m/s^2 */
    float move[3];              /* +0xfc: this step's displacement */
    float steer_rate, steer;    /* +0x10c, +0x110: the handlebar (+-RoueLimiteAngulaire) */
    int kmh_shown, kmh;         /* +0x114, +0x118: the speedometer, km/h */
    int rpm;                    /* +0x11c: the rev counter */
    float cpu_accel;            /* +0x120: computer bikes' shown acceleration */
    float speed;                /* +0x124: m/s */
    float speed_in;             /* +0x128: the speed as the engine step found it */
    float wheel;                /* +0x12c: the driven wheel's road speed, m/s */
    float dir[3];               /* +0x130: unit velocity direction */
    int slide;                  /* +0x13c: a dirt bike's brake slide */
    float m1[12];               /* +0x140: the matrix one step ago */
    float push_dir[3];          /* +0x174: an extra push (a wall, a bike) */
    float push;                 /* +0x180: its speed, decays 33.3 m/s a second */
    float skid_w[2];            /* +0x184, +0x188: skid marks' width (effects) */
    float vdir[3];              /* +0x18c: the smoothed direction of motion */
    float fwd[3];               /* +0x198: computer bikes: the unit Z row (where they point) */
    float cpu_speed;            /* +0x1a4: computer bikes' wanted speed, m/s */
    float front[3], front1[3], front2[3];   /* +0x1b4, +0x1c0, +0x1cc: front wheel point */
    float front_air;            /* +0x274: the front wheel's time off the ground */
    float rear[3], rear1[3], rear2[3];      /* +0x338, +0x344, +0x350: rear wheel point */
    float rear_air;             /* +0x3f8 */
    float third[3], third1[3], third2[3];   /* +0x4bc, +0x4c8, +0x4d4 */

    int sliding, sliding1;      /* +0x2da0, +0x2da4: sideways now / last step */
    float lon0;                 /* +0x2da8: forward speed at the step's start */
    int events;                 /* +0x2dac: 1 wall scrape, 2 wall crash, 4/8/0x10/0x20
                                   bike hits, 0x40 crash bounce, 0x80 landing (sounds) */
    float rear_h;               /* +0x2db0: the road under the rear wheel */
    int ground, ground1;        /* +0x2db8, +0x2dbc: on the ground now / last step */
    int rear_touch, front_touch;/* +0x2dc0, +0x2dc4 */
    int keys1;                  /* +0x2dc8: last step's keys */
    float lean, lean1;          /* +0x2e1d, +0x2e21: the lean, radians */
    int analog;                 /* +0x2e29: steered by a joystick */
    float wall_spin;            /* +0x2e31: spin still to give after a wall hit */
    float dive;                 /* +0x2e39: the frame's dive */
    float spin_rear, spin_front;/* +0x2e3d, +0x2e49: wheel turn angles */
    float frame[12];            /* +0x2e55: the frame's matrix (drawing, pitch) */
    int turbo_req;              /* +0x2e85: a copy of the turbo key */
    float pitch;                /* +0x2e8d: the frame's pitch on the suspension */
    float susp;                 /* +0x2e91: the rear suspension */
    float kick, phase, amp;     /* +0x2e95, +0x2e99, +0x2e9d: the spring */
    float pitch_s;              /* +0x2ea1: smoothed pitch */
    float brake_dive;           /* +0x2ea5: <= 0 */
    float wheelie;              /* +0x307d: the turbo's front wheel lift, 0..pi/2 */
    float turbo_time;           /* +0x3081: dt units the turbo has been held */
    float tumble[12];           /* +0x3085: the crashed bike's matrix (drawing) */
    float tumble_pt[3];         /* +0x30b5: and where it is */
    int crashed;                /* +0x30c1 */
    float crash_timer;          /* +0x30c5: 400 at the start */
    float tumble_dir[3];        /* +0x30c9 */
    float tumble_speed, tumble_speed0;  /* +0x30d5, +0x30d9 */
    float roll_side;            /* +0x30dd: +-pi/2 */
    float roll[3];              /* +0x30e1: the tumble's turn rates */
    int detail;                 /* +0x319d: draw the full skeleton this step */
    float crash_v[3];           /* +0x31a1 */
    int crash_flag;             /* +0x31ad */
    int sec;                    /* +0x31c1: the section */
    float wall_n[3];            /* +0x31c5: the last wall's normal */
    float hit_timer;            /* +0x31d1: 200 after a hit */
    float lane_x;               /* +0x31d5: computer bikes: across the racing line, units */
    int lane;                   /* +0x31d9: 0..2, -1 (computer bikes: the lane they ride) */
    int turbo_held;             /* +0x31f9: action 6 held */
    int turbo_pressed;          /* +0x3201: pressed since the pose last looked */
    int turbo_pressed1;         /* +0x3205 */

    /* the rider's body (pose.c) */
    Pose pose;                  /* the bones' angles (+0x648 + slot x 100 of a detail) */
    float dist;                 /* +0x634: the distance over the zoom */
    int lod;                    /* +0x638 */
    int pose_prev;              /* +0x304d: the pose a stop mixes from */
    float stand;                /* +0x3051: standing up (+) or sitting back (-) */
    unsigned stunt_at;          /* +0x3055 */
    int stunt_on;               /* +0x305d */
    int stunt_next;             /* +0x3061: the next one in the list */
    int stunt_k;                /* +0x3065: this one's pose */
    int look;                   /* +0x3069: the pose looked over the shoulder with */
    int rival;                  /* +0x306d: the nearest rider last step */
    float look_dot;             /* +0x3071 */
    unsigned look_at;           /* +0x3075 */
    float look_len;             /* +0x3079: ms */
} BikePhys;

/* The globals the step reads (the game's addresses) */
typedef struct {
    float dt;                   /* 0x581014: this frame's step (ms x 0.1) */
    int countdown;              /* 0x4dc0b4: the start's countdown runs */
    int reverse;                /* 0x4dc140: 1, or -1 when the track runs backwards */
    float frame_ms;             /* 0x4dc150: the frame time smoothed by 1/8 (16 at 62.5 Hz) */
    int gear_latch;             /* 0x4ea4c8: one gear change a key press (all bikes) */
    int mode;                   /* 0x4dc094: 0 practice (no bike against bike) */
    int followed;               /* 0x508834: the rider the camera follows (1) */
    int replay;                 /* 0x4e9834: 1 or 2 when a replay or demo plays */
    int snap;                   /* 0x4ea510: set when a crashed bike gets up */
    unsigned crt;               /* the C library's rand() state (msvcrt, 0x4b3600) */
    BikePhys *rider[25];        /* the records by number, for 0x4681d0 and 0x47ecd0 */
    int nriders;                /* 0x509bac */
    unsigned clock;             /* 0x50bbc8: ms */
    void (*pose)(BikePhys *);   /* 0x467b30, where the step calls it (none in the checks) */
} BikeWorld;

extern BikeWorld g_bikeworld;

double bikephys_road_y(int sec, const float p[3]);      /* 0x40c8b0 */
/* the section planes the physics uses (0x40e5c0) for g_land; again after each land_load */
int bikephys_track(void);
void bikephys_track_free(void);

/* what 0x45bf70 (+0x48bce0), 0x45bdf0, 0x45bb30 and the grid (0x47bf70, 0x45c7c0) leave:
   m the rows X, Y, Z; human: a local player (its box is 0x4dc0a0's: automatic) */
void bikephys_init(BikePhys *b, int bike, const float m[3][3], const float pos[3],
                   int section, int human, int automatic);
/* 0x45d540 for a driven bike: keys as above, the joystick 0..0xffff (0x8000 centre), dt =
   ms x 0.1 (0x47df40), countdown = 0x4dc0b4; time_left (0x4dc0a8) is not read by the
   step and is kept for the caller's symmetry */
void bikephys_step(BikePhys *b, unsigned keys, int joy_x, int joy_y, float dt, int countdown,
                   int time_left);
/* 0x47ecd0, after every bike's step: riders 1..n-1 of all[] (all[0] unused) */
void bikephys_collide(BikePhys *all[], int n, float dt);

/* the key handlers 0x431250 (down) and 0x432700 (up) on obj+0x31e9: action 0..5 as the
   bits above, 6 the turbo; race_running = 0x5429b0 */
void bikephys_key(BikePhys *b, unsigned *keys, int action, int down, int race_running);

/* 0x45ca00: the frame's pitch from its matrix (+0x2e55) */
double bikephys_frame_pitch(const BikePhys *b);

/* the C library's rand() as the game's (msvcrt: x 214013 + 2531011, bits 16..30) */
int bikephys_crt_rand(void);

/* 0x412960's part of the winner's lap: the computer takes the bike at 80 % of its top speed;
   and the bike given back (0x412a50, 0x411010), its top speed as it was */
void bikephys_winner_take(BikePhys *b);
void bikephys_winner_give_back(BikePhys *b, float top);

/* drawing helpers: the matrix as rows, the lean, the frame's pitch */
static inline const float *bikephys_row(const BikePhys *b, int r) { return b->m + 3 * r; }

#endif
