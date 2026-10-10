/* Imhotep - figure actions, group D: the boats of types 0x5C, 0x5D, 0x64, 0x65 (0x004ADF60,
 * 0x004AEE10, 0x004AF970, 0x004AFC40) and their state functions (0x004AD340 .. 0x004AED10).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). The actions work on the current figure
 * (0x00D056F8); the state functions take the figure id (and a `moving` flag of the action, which
 * selects the sprite). None of these types occurs in the reference corpus. */
#include "figure/actions_d.h"

#include "figure/actions_a.h"
#include "figure/actions_c2.h"
#include "figure/actions_d_local.h"
#include "random/crt_rand.h"
#include "state/layout.h"
#include "state/ported.h"


static int32_t type_value(game_state *st, uint32_t f, uint32_t field)
{
    return S32(st, G_TYPE_TABLE + 0x38u * U8(st, f + FIG_TYPE) + field);
}

/* |a - b| per axis into the globals 0x00D3B4FC / 0x00D3B4F8; the larger one */
static int32_t distance_to(game_state *st, int32_t ax, int32_t ay, int32_t bx, int32_t by)
{
    int32_t dx = ax > bx ? ax - bx : (ax < bx ? bx - ax : 0);
    S32(st, G_DIST_X) = dx;
    int32_t dy = ay > by ? ay - by : (ay < by ? by - ay : 0);
    S32(st, G_DIST_Y) = dy;
    return dx >= dy ? dx : dy;
}

static void boat_4AD340(game_state *st, int32_t id)
{
    /* @bin 0x004AD340 */
    GAME_CALL(st, FN_FIGURE_DYING, id);
}

static void boat_4AD350(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004AD350 waiting (+0x36 counts down), then off: towards +0x30/+0x32 over water when
     * +0x130, else to the river exit */
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    *moving = 0;
    S16(st, f + FIG_WAIT_TICKS) -= 1;
    if (S16(st, f + FIG_WAIT_TICKS) > 0) {
        return;
    }
    S16(st, f + FIG_WAIT_TICKS) = 0;
    if (U8(st, f + FIG_U130) != 0) {
        S16(st, f + FIG_SOURCE_X) = S16(st, f + FIG_X);
        S16(st, f + FIG_SOURCE_Y) = S16(st, f + FIG_Y);
        int32_t x, y;
        line_search_water(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, f + FIG_U30), S16(st, f + FIG_U32), &x, &y);
        S16(st, f + FIG_DEST_X) = (int16_t) x;
        S16(st, f + FIG_DEST_Y) = (int16_t) y;
        U8(st, f + FIG_ACTION) = 0x0D;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        return;
    }
    S16(st, f + FIG_SOURCE_X) = S16(st, f + FIG_X);
    S16(st, f + FIG_SOURCE_Y) = S16(st, f + FIG_Y);
    S16(st, f + FIG_DEST_X) = S16(st, G_EXIT_X);
    S16(st, f + FIG_DEST_Y) = S16(st, G_EXIT_Y);
    U8(st, f + FIG_ACTION) = 0x11;
}

static void boat_4AD450(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004AD450 move; arrived: not moving */
    uint32_t f = FIGURE_ADDR(id);
    GAME_CALL(st, FN_FIGURE_MOVE, id, type_value(st, f, 8));
    int8_t d = S8(st, f + FIG_DIRECTION);
    if (d == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    } else if (d == 0x0A) {
        U8(st, f + FIG_ACTION) = 4;
    } else if (d == 8) {
        *moving = 0;
    }
}

static void boat_4AD4C0(game_state *st, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x004AD4C0 the boat lands its troops at (x, y): +0x16C soldiers of the kind +0x174, in
     * three groups (shares 0x004CDE40 of the kind's tables), each split into formations of at most
     * 16 (0x004B6FF0), soldiers created there (0x004B4800) */
    uint32_t f = FIGURE_ADDR(id);
    int32_t kind = S32(st, f + FIG_U174);
    int32_t total = S32(st, f + FIG_U16C);
    int32_t e178 = S32(st, f + FIG_U178);
    int32_t e17c = S32(st, f + FIG_U17C);
    int16_t serial = (int16_t) (S16(st, G_INVASION_SERIAL) + 1);
    S16(st, G_INVASION_SERIAL) = serial;
    if (serial > 0x7D00) {
        S16(st, G_INVASION_SERIAL) = 1;
    }
    int32_t pa, pb, pc;
    if (kind == 0x0C) {
        pa = S32(st, G_PROPS + FIGTYPE_U24 + 0x38u * 0x36u);
        pb = S32(st, G_PROPS + FIGTYPE_U24 + 0x38u * 0x37u);
        pc = S32(st, G_PROPS + FIGTYPE_U24 + 0x38u * 0x38u);
    } else if (kind == 0x0D) {
        pa = S32(st, G_PROPS + FIGTYPE_U24 + 0x38u * 0x63u);
        pb = 0;
        pc = 0;
    } else {
        pa = S32(st, G_PROPS + FIGTYPE_U24 + 0x38u * 0x2Cu);
        pb = S32(st, G_PROPS + FIGTYPE_U24 + 0x38u * 0x2Bu);
        pc = S32(st, G_PROPS + FIGTYPE_U24 + 0x38u * 0x2Du);
    }
    GAME_CALL(st, FN_CALC_PERCENTAGE, total, pa); /* @bin 0x004AD556 its result is overwritten */
    int32_t g1 = GAME_CALL(st, FN_CALC_PERCENTAGE, total, pb);
    int32_t g2 = GAME_CALL(st, FN_CALC_PERCENTAGE, total, pc);
    int32_t groups[3] = {total - g2 - g1, g1, g2};
    int32_t nsub[3] = {0, 0, 0};
    int32_t sub[3][4] = {{0}};
    for (int k = 0; k < 3; k++) {
        int32_t v = groups[k];
        if (v <= 0) {
            continue;
        }
        if (v <= 0x10) {
            nsub[k] = 1;
        } else if (v <= 0x20) {
            nsub[k] = 2;
            sub[k][1] = v / 2;
            v -= v / 2;
        } else {
            nsub[k] = 3;
            sub[k][2] = v / 3;
            sub[k][1] = v / 3;
            v -= 2 * (v / 3);
        }
        sub[k][0] = v;
    }
    int32_t formations = 0;
    for (int k = 0; k < 3; k++) {
        if (nsub[k] <= 0) {
            continue;
        }
        int32_t type, size = 10;
        if (kind == 0x0C) {
            type = k == 1 ? 0x37 : (k == 2 ? 0x38 : 0x36);
        } else if (kind == 0x0D) {
            type = 0x63;
            size = 8;
        } else {
            type = k == 1 ? 0x2B : (k == 2 ? 0x2D : 0x2C);
        }
        for (int s = 0; s < nsub[k]; s++) {
            int32_t form = GAME_CALL(st, FN_FORMATION_CREATE_0X4B6FF0, type, size, 0, x, y);
            if (form <= 0) {
                continue;
            }
            uint32_t rec = G_FORMATION_BASE + 0x90u * (uint32_t) form;
            S16(st, rec + FM_U44) = (int16_t) e178;
            U8(st, rec + FM_U71) = 0;
            U8(st, rec + FM_KIND) = (uint8_t) kind;
            S16(st, rec + FM_EVENT) = (int16_t) e17c;
            S16(st, rec + FM_INVASION) = S16(st, G_INVASION_SERIAL);
            int32_t j = 0;
            for (; j < sub[k][s]; j++) {
                int32_t n = GAME_CALL(st, FN_FIGURE_CREATE, 0, type, x, y, 0);
                uint32_t nf = FIGURE_ADDR(n);
                U8(st, nf + FIG_U0D) = 0;
                U8(st, nf + FIG_ACTION) = 9;
                S16(st, nf + FIG_WAIT_TICKS) = (int16_t) (10 * (j + 20 * formations + 1));
                S16(st, nf + FIG_FORMATION) = (int16_t) form;
                GAME_CALL(st, FN_FIGURE_ASSIGN_SLOT, n);
                U8(st, nf + FIG_INVISIBLE) = 1;
            }
            formations++;
            S16(st, f + FIG_WAIT_TICKS) = (int16_t) (10 * (j + 20 * formations + 1));
        }
    }
    U8(st, f + FIG_U130) = 0;
    U8(st, f + FIG_ACTION) = 8;
}

static void boat_4ADB40(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004ADB40 sail to the landing point; there, the troops land on the nearest free land
     * tile around +0x30/+0x32 */
    uint32_t f = FIGURE_ADDR(id);
    GAME_CALL(st, FN_FIGURE_MOVE, id, type_value(st, f, 8));
    int8_t d = S8(st, f + FIG_DIRECTION);
    if (d == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        return;
    }
    if (d == 0x0A) {
        U8(st, f + FIG_ACTION) = 4;
        return;
    }
    if (d != 8) {
        return;
    }
    *moving = 0;
    int32_t x = S16(st, f + FIG_U30), y = S16(st, f + FIG_U32);
    spiral_free_land(st, &x, &y);
    uint32_t tile = U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x;
    uint32_t t = U32(st, G_GRID_TERRAIN + 4u * tile);
    if (t & 0x203u) {
        S16(st, f + FIG_WAIT_TICKS) = 0x32;
        U8(st, f + FIG_U130) = 0;
        U8(st, f + FIG_ACTION) = 8;
    }
    if (t & 4u) {
        if ((t & 0x40u) == 0) {
            S16(st, f + FIG_WAIT_TICKS) = 0x32;
            U8(st, f + FIG_U130) = 0;
            U8(st, f + FIG_ACTION) = 8;
        }
    } else if (t & 0xC108u) {
        GAME_CALL(st, FN_BUILDING_DESTROY_AT_0X469140, x, y, (int32_t) tile);
    }
    boat_4AD4C0(st, id, x, y);
    GAME_CALL(st, FN_EVENT_MESSAGE_0X4463C0, S32(st, f + FIG_U17C), S32(st, f + FIG_GRID_OFFSET));
}

static void boat_4ADC80(game_state *st, int32_t id)
{
    /* @bin 0x004ADC80 sail between the river entry and exit */
    uint32_t f = FIGURE_ADDR(id);
    GAME_CALL(st, FN_FIGURE_MOVE, id, type_value(st, f, 8));
    int8_t d = S8(st, f + FIG_DIRECTION);
    if (d == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        return;
    }
    if (d == 0x0A) {
        U8(st, f + FIG_ACTION) = 4;
        return;
    }
    if (d != 8) {
        return;
    }
    int16_t x = S16(st, f + FIG_X);
    int16_t ex = S16(st, G_ENTRY_X), ey = S16(st, G_ENTRY_Y);
    if (x == ex && S16(st, f + FIG_Y) == ey) {
        S16(st, f + FIG_DEST_X) = S16(st, G_EXIT_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_EXIT_Y);
        U8(st, f + FIG_ACTION) = 0x11;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        return;
    }
    if (x == S16(st, G_EXIT_X) && S16(st, f + FIG_Y) == S16(st, G_EXIT_Y)) {
        if (U8(st, f + FIG_U130) == 0) {
            U8(st, f + FIG_ACTION) = 4; /* left the map */
            return;
        }
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        U8(st, f + FIG_ACTION) = 8;
        return;
    }
    S16(st, f + FIG_DEST_X) = ex;
    S16(st, f + FIG_DEST_Y) = ey;
    U8(st, f + FIG_ACTION) = 0x11;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
}

/* @bin jump tables 0x004ADF38 / 0x004AE630 by the direction to the enemy (0x0051CBA0): the three
 * line searches of 0x004A4850 */
static const int32_t flee_offsets[8][6] = {
    {0, -10, 10, -10, -10, -10}, {0, -10, 10, -10, 10, 0}, {10, -10, 10, 0, 10, 10},
    {10, 0, 10, 10, 0, 10},      {10, 10, 0, 10, -10, 10}, {0, 10, -10, 0, -10, 10},
    {-10, 0, -10, 10, -10, -10}, {-10, 0, -10, -10, 0, -10},
};

/* flee from the nearest enemy (0x004A5560): a target over water away from it. *px, *py start as
 * the original's argument slots (see the callers). */
static int boat_flee_from_enemy(game_state *st, int32_t id, int32_t *px, int32_t *py)
{
    uint32_t f = FIGURE_ADDR(id);
    int32_t e = GAME_CALL(st, FN_BOAT_NEAREST, id, 0x12, 1);
    if (e == 0) {
        return 0;
    }
    uint32_t ef = FIGURE_ADDR(e);
    uint32_t d = (uint8_t) GAME_CALL(st, FN_ROUTE_DIRECTION, S16(st, ef + FIG_X), S16(st, ef + FIG_Y), S16(st, f + FIG_X),
                                     S16(st, f + FIG_Y));
    const int32_t *o = flee_offsets[d <= 6 ? d : 7];
    boat_flee_target(st, px, py, o[0], o[1], o[2], o[3], o[4], o[5], id);
    S16(st, f + FIG_DEST_X) = (int16_t) *px;
    S16(st, f + FIG_DEST_Y) = (int16_t) *py;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    return 1;
}

static void boat_4ADD90(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004ADD90 under attack: every 75 updates (+0x36), flee; +0x15C 9 while fleeing */
    uint32_t f = FIGURE_ADDR(id);
    int32_t phase = S32(st, f + FIG_U15C);
    if (phase == 9) {
        boat_4AD450(st, id, moving);
        if (S8(st, f + FIG_DIRECTION) == 8) {
            S32(st, f + FIG_U15C) = 0x10;
            *moving = 0;
        }
        return;
    }
    if (phase != 0x10) {
        return;
    }
    *moving = 0;
    S16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) < 0x4B) {
        return;
    }
    S16(st, f + FIG_WAIT_TICKS) = 0;
    /* @bin 0x004ADED3 the slots are this function's arguments: x the `moving` pointer (not
     * reproducible; only read when no line search finds water), y the id */
    int32_t x = 0, y = id;
    if (boat_flee_from_enemy(st, id, &x, &y)) {
        S32(st, f + FIG_U15C) = 9;
    }
}

static void boat_4AE1F0(game_state *st, int32_t id)
{
    /* @bin 0x004AE1F0 waiting (+0x36), then to the river entry */
    uint32_t f = FIGURE_ADDR(id);
    S16(st, f + FIG_WAIT_TICKS) -= 1;
    if (S16(st, f + FIG_WAIT_TICKS) > 0) {
        return;
    }
    S16(st, f + FIG_WAIT_TICKS) = 0;
    U8(st, f + FIG_INVISIBLE) = 0;
    S16(st, f + FIG_SOURCE_X) = S16(st, f + FIG_X);
    S16(st, f + FIG_SOURCE_Y) = S16(st, f + FIG_Y);
    S16(st, f + FIG_DEST_X) = S16(st, G_ENTRY_X);
    S16(st, f + FIG_DEST_Y) = S16(st, G_ENTRY_Y);
    U8(st, f + FIG_ACTION) = 0x11;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
}

static void boat_4AE270(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004AE270 */
    GAME_CALL(st, FN_FIGURE_MOVE_HURRIED, id, 0);
    *moving = 1;
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    if (S8(st, f + FIG_DIRECTION) == 9 || S8(st, f + FIG_DIRECTION) == 0x0A) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    }
}

static int32_t clamp7(int32_t v)
{
    return v >= 7 ? 7 : (v < 0 ? 0 : v);
}

static void boat_4AE2C0(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004AE2C0 ram: when +0x164 is 0, damage the boat in front (0x004A5C10) by an attack
     * value (+0x150 / +0x158 by +0x134) over a table of the two directions, both turn aside */
    GAME_CALL(st, FN_FIGURE_MOVE_HURRIED, id, 1);
    *moving = 1;
    uint32_t f = FIGURE_ADDR(id);
    int32_t wait = S32(st, f + FIG_U164);
    if (wait != 0) {
        wait -= 1;
        S32(st, f + FIG_U164) = wait < 0 ? 0 : (wait >= 0x0F ? 0x0F : wait);
    } else {
        int32_t e = GAME_CALL(st, FN_BOAT_RAM_TARGET, id);
        if (e != 0) {
            uint32_t ef = FIGURE_ADDR(e);
            int32_t bonus = 0;
            if (U8(st, ef + FIG_TYPE) == 0x4E && U8(st, ef + FIG_ACTION) == 0x14) {
                bonus = 0x14;
            }
            int32_t edir = S8(st, ef + FIG_DIRECTION) < 8 ? S8(st, ef + FIG_DIRECTION) : S8(st, ef + FIG_PREV_DIRECTION);
            /* @bin 0x004AE34A the divisor table 0x005DC24C [enemy direction][own direction] */
            int32_t div = U8(st, G_RAM_DIVISOR + (uint32_t) (S8(st, f + FIG_DIRECTION) + edir * 8));
            int32_t attack = S32(st, f + FIG_U134) >= S32(st, G_PROPS + FIGTYPE_U28 + 0x38u * U8(st, f + FIG_TYPE))
                                 ? S32(st, f + FIG_U150)
                                 : S32(st, f + FIG_U158);
            int32_t damage = (attack - bonus) / div;
            GAME_CALL(st, FN_BOAT_RAM_SOUND, div);
            S32(st, ef + FIG_HIT_POINTS) -= damage;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            int32_t r = (int32_t) (U32(st, G_R_1_7) & 2u) - 1; /* @bin 0x004AE3B1 -1 or +1 */
            int32_t nd = clamp7(S8(st, f + FIG_DIRECTION) + r);
            int32_t ed = clamp7(S8(st, ef + FIG_DIRECTION) - r);
            U8(st, f + FIG_DIRECTION) = (uint8_t) nd;
            U8(st, ef + FIG_DIRECTION) = (uint8_t) ed;
            S32(st, f + FIG_U164) = 0x0F;
            U8(st, f + FIG_ACTION) = 0x0A;
            S16(st, f + FIG_TARGET) = 0;
            GAME_CALL(st, FN_FIGURE_SPAWN_RING, S16(st, ef + FIG_X), S16(st, ef + FIG_Y), 0, 0);
        }
    }
    int8_t d = S8(st, f + FIG_DIRECTION);
    if (d == 8) {
        U8(st, f + FIG_ACTION) = 0x0A;
        S32(st, f + FIG_U15C) = 0x10;
    } else if (d == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    } else if (d == 0x0A) {
        U8(st, f + FIG_ACTION) = 4;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    }
}

static void boat_4AE490(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004AE490 under attack: flee every 75 updates; +0x15C 0x0B while fleeing */
    uint32_t f = FIGURE_ADDR(id);
    int32_t phase = S32(st, f + FIG_U15C);
    if (phase == 0x0B) {
        boat_4AE270(st, id, moving);
        if (S8(st, f + FIG_DIRECTION) == 8) {
            S32(st, f + FIG_U15C) = 0x10;
        }
        return;
    }
    if (phase != 0x10) {
        return;
    }
    S16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) < 0x4B) {
        return;
    }
    S16(st, f + FIG_WAIT_TICKS) = 0;
    /* @bin 0x004AE5C8 the slots are this function's arguments: x the id, y the `moving` pointer
     * (not reproducible; only read when no line search finds water) */
    int32_t x = id, y = 0;
    if (!boat_flee_from_enemy(st, id, &x, &y)) {
        U8(st, f + FIG_ACTION) = 0x11;
        return;
    }
    S32(st, f + FIG_U15C) = 0x0B;
}

static void boat_4AE650(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004AE650 fight: find an enemy boat (0x004BCF40, range 0xE4), go next to it (+0x82),
     * attack while in range (0x004B45C0 every +0x6B ticks) */
    uint32_t f = FIGURE_ADDR(id);
    int32_t phase = S32(st, f + FIG_U15C);
    if (phase == 0x10) {
        int32_t e = GAME_CALL(st, FN_WARSHIP_FIND_TARGET_OF, id, 0xE4);
        if (e == 0) {
            U8(st, f + FIG_ACTION) = 0x11;
            return;
        }
        uint32_t ef = FIGURE_ADDR(e);
        uint32_t tile = U32(st, G_MAP_START) + (uint32_t) S16(st, ef + FIG_X) + 228u * (uint32_t) S16(st, ef + FIG_Y);
        int on_land = U8(st, G_GRID_ROUTING_WATER + tile) == 0xFF;
        /* @bin 0x004AE6EB jump tables 0x004AEA60 / 0x004AEA58: types 0x0B .. 0x0D, 0x4D, 0x4E */
        uint8_t et = U8(st, ef + FIG_TYPE);
        if ((et >= 0x0B && et <= 0x0D) || et == 0x4D || et == 0x4E) {
            if (crt_rand(st) & 1) {
                U8(st, f + FIG_U80) = (uint8_t) ((S32(st, G_MONTH) + 1) % 12);
            }
        }
        et = U8(st, ef + FIG_TYPE);
        if (et == 0x0C || et == 0x0B || et == 0x0D) {
            S16(st, f + FIG_DEST_X) = S16(st, f + FIG_X);
            S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_Y);
        } else if (!on_land) {
            S16(st, f + FIG_DEST_X) = S16(st, ef + FIG_X);
            S16(st, f + FIG_DEST_Y) = S16(st, ef + FIG_Y);
            *moving = 1;
        } else {
            int32_t tx = S16(st, ef + FIG_X), ty = S16(st, ef + FIG_Y);
            if (S16(st, f + FIG_X) < S16(st, ef + FIG_X)) {
                tx -= 1;
            }
            if (S16(st, f + FIG_Y) < S16(st, ef + FIG_Y)) {
                ty -= 1;
            }
            int32_t slot;
            if (figure_route_slot(st, e, &slot) != 0
                && GAME_CALL(st, FN_ROUTE_PATH_TIES, slot, S16(st, f + FIG_X), S16(st, f + FIG_Y), tx, ty, 0) != 0) {
                S16(st, f + FIG_DEST_X) = (int16_t) tx;
                S16(st, f + FIG_DEST_Y) = (int16_t) ty;
            } else {
                S16(st, f + FIG_DEST_X) = S16(st, f + FIG_X);
                S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_Y);
            }
        }
        S32(st, f + FIG_U15C) = 9;
        U8(st, f + FIG_DIRECTION) = U8(st, f + FIG_PREV_DIRECTION);
        S16(st, f + FIG_TARGET) = (int16_t) e;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        return;
    }
    if (phase != 9) {
        return;
    }
    int16_t t = S16(st, f + FIG_TARGET);
    uint32_t tf = FIGURE_ADDR(t);
    int32_t dist = distance_to(st, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, tf + FIG_X), S16(st, tf + FIG_Y));
    int in_range = 0;
    if (dist < type_value(st, f, 0)) {
        in_range = 1;
        U8(st, f + FIG_U6B) += 1;
        if ((int32_t) U8(st, f + FIG_U6B) > type_value(st, f, 4)) {
            U8(st, f + FIG_U6B) = 0;
            U8(st, f + FIG_ATTACK_IMAGE) = t != 0 ? 1 : 0;
        } else {
            U8(st, f + FIG_ATTACK_IMAGE) = 0;
        }
    } else {
        U8(st, f + FIG_ATTACK_IMAGE) = 0;
    }
    if (U8(st, f + FIG_ATTACK_IMAGE) != 0) {
        if (U8(st, f + FIG_ATTACK_IMAGE) == 1) {
            GAME_CALL(st, FN_MISSILE_CREATE, id, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, tf + FIG_X),
                      S16(st, tf + FIG_Y), 0x47);
        }
        U8(st, f + FIG_ATTACK_IMAGE) += 1;
        if (U8(st, f + FIG_ATTACK_IMAGE) > 0x64) {
            U8(st, f + FIG_ATTACK_IMAGE) = 0;
        }
    }
    uint8_t tt = U8(st, FIGURE_ADDR(S16(st, f + FIG_TARGET)) + FIG_TYPE);
    if (tt >= 0x0B && tt <= 0x0D) {
        if (in_range) {
            *moving = 0;
        }
    } else {
        boat_4AE2C0(st, id, moving);
    }
    S16(st, f + FIG_WAIT_TICKS) += 1;
    if (S8(st, f + FIG_DIRECTION) == 8 || S16(st, f + FIG_WAIT_TICKS) >= 0x32) {
        S16(st, f + FIG_WAIT_TICKS) = 0;
        S32(st, f + FIG_U15C) = 0x10;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    }
}

static int8_t boat_4AEAB0(game_state *st, int32_t *px, int32_t *py)
{
    /* @bin 0x004AEAB0 the first of three points (0x00784DC4 / 0x00784DD0) that is set: the
     * nearest free water tile around it */
    for (uint32_t k = 0; k < 3; k++) {
        if (S32(st, G_U784DC4 + 4u * k) != -1 && S32(st, G_U784DD0 + 4u * k) != -1) {
            *px = S32(st, G_U784DC4 + 4u * k);
            *py = S32(st, G_U784DD0 + 4u * k);
            spiral_free_water(st, px, py);
            return 1;
        }
    }
    return 0;
}

static void boat_4AEB00(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004AEB00 */
    GAME_CALL(st, FN_FIGURE_MOVE_HURRIED, id, 0);
    *moving = 1;
    uint32_t f = FIGURE_ADDR(id);
    S16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) >= crt_rand(st) % 30 + 0x4B) {
        S16(st, f + FIG_WAIT_TICKS) = 0;
        U8(st, f + FIG_ACTION) = 0x0A;
        S32(st, f + FIG_U15C) = 0x10;
    }
    int8_t d = S8(st, f + FIG_DIRECTION);
    if (d == 9 || d == 0x0A) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        return;
    }
    if (d != 8) {
        return;
    }
    /* @bin 0x004AEB9F the slots: x this function's id argument, y a local */
    int32_t x = id, y = 0;
    int8_t found = boat_4AEAB0(st, &x, &y);
    int16_t fx = S16(st, f + FIG_X);
    int16_t ex = S16(st, G_ENTRY_X), ey = S16(st, G_ENTRY_Y);
    if (fx == ex && S16(st, f + FIG_Y) == ey) {
        if (found) {
            S16(st, f + FIG_DEST_X) = (int16_t) x;
            S16(st, f + FIG_DEST_Y) = (int16_t) y;
        } else {
            S16(st, f + FIG_DEST_X) = S16(st, G_EXIT_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_EXIT_Y);
        }
    } else if (found && fx == x && S16(st, f + FIG_Y) == y) {
        S16(st, f + FIG_DEST_X) = S16(st, G_EXIT_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_EXIT_Y);
    } else {
        S16(st, f + FIG_DEST_X) = ex;
        S16(st, f + FIG_DEST_Y) = ey;
    }
    U8(st, f + FIG_ACTION) = 0x11;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
}

static void boat_4AEC80(game_state *st, int32_t id, int32_t *moving)
{
    /* @bin 0x004AEC80 leave the map (reads the current figure after moving id) */
    GAME_CALL(st, FN_FIGURE_MOVE_HURRIED, id, 0);
    *moving = 1;
    int32_t c = cur(st);
    uint32_t f = FIGURE_ADDR(c);
    int8_t d = S8(st, f + FIG_DIRECTION);
    if (d == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, c);
    } else if (d == 0x0A || d == 8) {
        U8(st, f + FIG_STATE) = 2;
    }
}

static void boat_4AECE0(game_state *st)
{
    /* @bin 0x004AECE0 every 100 updates (+0x92) the current figure's route is released */
    int32_t c = cur(st);
    uint32_t f = FIGURE_ADDR(c);
    U8(st, f + FIG_U92) += 1;
    if (U8(st, f + FIG_U92) >= 0x64) {
        U8(st, f + FIG_U92) = 0;
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, c);
    }
}

static void boat_4AED10(game_state *st, int32_t id)
{
    /* @bin 0x004AED10 every few updates (+0x6B against the type's +4), aim at an enemy in range
     * (0x004BCF40) and shoot (0x004B45C0, +0x6A the shot's progress) */
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_U6B) += 1;
    if ((int32_t) U8(st, f + FIG_U6B) > type_value(st, f, 4)) {
        U8(st, f + FIG_U6B) = 0;
        if (GAME_CALL(st, FN_WARSHIP_FIND_TARGET_OF, id, type_value(st, f, 0)) != 0) {
            U8(st, f + FIG_ATTACK_IMAGE) = 1;
            U8(st, f + FIG_DIRECTION) = (uint8_t) GAME_CALL(st, FN_DIRECTION_TO_FINE, S16(st, f + FIG_X), S16(st, f + FIG_Y),
                                                  S32(st, G_ROAD_ACCESS_X), S32(st, G_ROAD_ACCESS_Y));
        } else {
            U8(st, f + FIG_ATTACK_IMAGE) = 0;
        }
    }
    uint8_t a = U8(st, f + FIG_ATTACK_IMAGE);
    if (a == 0) {
        return;
    }
    if (a == 1) {
        GAME_CALL(st, FN_MISSILE_CREATE, id, S16(st, f + FIG_X), S16(st, f + FIG_Y), S32(st, G_ROAD_ACCESS_X),
                  S32(st, G_ROAD_ACCESS_Y), 0x47);
    }
    U8(st, f + FIG_ATTACK_IMAGE) += 1;
    if (U8(st, f + FIG_ATTACK_IMAGE) > 0x64) {
        U8(st, f + FIG_ATTACK_IMAGE) = 0;
    }
}

/* ---- the actions ---- */

/* the common start: +0x128 (hit points) gone: dying; 0x004CDE60 of +0x128, +0x12C in 1 .. 15:
 * under attack (state `attacked`) */
static void boat_start(game_state *st, uint8_t attacked, int set_36)
{
    uint32_t f = fig(st);
    next_frame(st, f);
    if (S32(st, f + FIG_HIT_POINTS) <= 0) {
        U8(st, f + FIG_ACTION) = 4;
    }
    int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, f + FIG_HIT_POINTS), S32(st, f + FIG_HIT_POINTS_MAX));
    if (r <= 0x0F && r > 0) {
        f = fig(st);
        U8(st, f + FIG_ACTION) = attacked;
        S32(st, f + FIG_U15C) = 0x10;
        if (set_36) {
            S16(st, f + FIG_WAIT_TICKS) = 0x64;
        }
    }
}

/* types 0x5D, 0x64: on the turn given by 0x0078D7C0 (+0x80 + 4 modulo 12), leave by the exit */
static void boat_turn(game_state *st)
{
    uint32_t f = fig(st);
    if ((U8(st, f + FIG_U80) + 4) % 12 == S32(st, G_MONTH)) {
        U8(st, f + FIG_ACTION) = 0x12;
        S16(st, f + FIG_DEST_X) = S16(st, G_EXIT_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_EXIT_Y);
    }
}

/* the states of types 0x5C, 0x65 (jump tables 0x004AE1B8 / 0x004AFE74) */
static void boat_states_a(game_state *st, int32_t *moving)
{
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    switch (U8(st, f + FIG_ACTION)) {
    case 4: boat_4AD340(st, id); break;
    case 8: boat_4AD350(st, id, moving); break;
    case 9: boat_4AD450(st, id, moving); break;
    case 0x0A: boat_4ADD90(st, id, moving); break;
    case 0x0C: GAME_CALL(st, FN_NOTHING, id); break;
    case 0x0D: boat_4ADB40(st, id, moving); break;
    case 0x11: boat_4ADC80(st, id); break;
    default: U8(st, f + FIG_ACTION) = 0x11; break;
    }
}

/* the states of types 0x5D, 0x64 (jump tables 0x004AF104 / 0x004AFC04); 0x5D also aims and
 * shoots (0x004AED10) in most of them */
static void boat_states_b(game_state *st, int32_t *moving, int shoot)
{
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    int aim = 0;
    switch (U8(st, f + FIG_ACTION)) {
    case 4: boat_4AD340(st, id); break;
    case 8: boat_4AE1F0(st, id); aim = 1; break;
    case 9: boat_4AE2C0(st, id, moving); aim = 1; break;
    case 0x0A: boat_4AE650(st, id, moving); break;
    case 0x0B: boat_4AE270(st, id, moving); aim = 1; break;
    case 0x0C: boat_4AE490(st, id, moving); aim = 1; break;
    case 0x11: boat_4AEB00(st, id, moving); aim = 1; break;
    case 0x12: boat_4AEC80(st, id, moving); break;
    default: U8(st, f + FIG_ACTION) = 0x11; break;
    }
    if (shoot && aim) {
        boat_4AED10(st, cur(st));
    }
}

/* the sprite: dying (+0x36 frames), else moving (frame / 3) or still */
static void boat_sprite(game_state *st, int store, uint32_t dying, uint32_t moving_base, uint32_t still,
                        int moving)
{
    uint32_t f = fig(st);
    int32_t dir = S8(st, f + FIG_DIRECTION) < 8 ? S8(st, f + FIG_DIRECTION) : S8(st, f + FIG_PREV_DIRECTION);
    int32_t d;
    if (store) {
        d = sprite_direction(st, dir);
    } else {
        d = dir - S32(st, G_VIEW_ORIENTATION);
        if (d < 0) {
            d += 8;
        }
    }
    if (U8(st, f + FIG_ACTION) == 4) {
        int16_t v = S16(st, f + FIG_WAIT_TICKS);
        if (v < 0x21) {
            S32(st, f + FIG_U04) = S32(st, dying) + v / 3 + d / 8;
        } else {
            int32_t k = v - 0x21;
            S32(st, f + FIG_U04) = S32(st, G_IMAGE_GROUP_588) + (k >= 0x17 ? 0x16 : k);
        }
        return;
    }
    if (moving) {
        S32(st, f + FIG_U04) = S32(st, moving_base) + 8 * (U8(st, f + FIG_ANIM_FRAME) / 3) + d;
    } else {
        S32(st, f + FIG_U04) = S32(st, still) + d;
    }
}

void figure_action_5C(game_state *st)
{
    /* @bin 0x004ADF60 */
    S32(st, CURRENT_CITY(st) + CITY_U4634) += 1; /* @bin 0x004ADF74 boats counted (0x00EA5EA4) */
    int32_t moving = 1;
    U8(st, fig(st) + FIG_BOAT_MODE) = 1;
    boat_start(st, 0x0A, 1);
    boat_4AECE0(st);
    boat_states_a(st, &moving);
    boat_sprite(st, 1, G_IMAGE_GROUP_261, G_IMAGE_GROUP_260, G_IMAGE_GROUP_262, moving && U8(st, fig(st) + FIG_U154) != 2);
}

void figure_action_5D(game_state *st)
{
    /* @bin 0x004AEE10 */
    S32(st, CURRENT_CITY(st) + CITY_U4634) += 1;
    int32_t moving = 0;
    U8(st, fig(st) + FIG_BOAT_MODE) = 1;
    boat_start(st, 0x0C, 0);
    boat_turn(st);
    boat_4AECE0(st);
    boat_states_b(st, &moving, 1);
    boat_sprite(st, 0, G_IMAGE_GROUP_264, G_IMAGE_GROUP_263, G_IMAGE_GROUP_265, moving && U8(st, fig(st) + FIG_U154) != 2);
}

void figure_action_64(game_state *st)
{
    /* @bin 0x004AF970 */
    int32_t moving = 0;
    boat_start(st, 0x0C, 0);
    boat_turn(st);
    boat_4AECE0(st);
    boat_states_b(st, &moving, 0);
    boat_sprite(st, 0, G_IMAGE_GROUP_643, G_IMAGE_GROUP_642, G_IMAGE_GROUP_644, moving && U8(st, fig(st) + FIG_U154) != 2);
}

void figure_action_65(game_state *st)
{
    /* @bin 0x004AFC40 */
    int32_t moving = 1;
    boat_start(st, 0x0A, 1);
    boat_4AECE0(st);
    boat_states_a(st, &moving);
    boat_sprite(st, 1, G_IMAGE_GROUP_647, G_IMAGE_GROUP_646, G_IMAGE_GROUP_648, moving);
}

/* ---- registration: the state functions take (id, int *moving); the wrappers accept a `moving`
 * pointer into the state (tests), else the call is not ported ---- */

#include <string.h>

static int32_t *moving_ptr(game_state *st, int32_t a, uint32_t addr)
{
    uint8_t *p = state_ptr(st, (uint32_t) a, 4);
    if (!p) {
        state_unported(st, addr);
    }
    return (int32_t *) (void *) p;
}

#define BOAT_RUNNER(name, addr) \
    static int32_t run_##name(game_state *st, const int32_t *a) \
    { \
        int32_t *p = moving_ptr(st, a[1], addr); \
        if (!p) { \
            return 0; \
        } \
        int32_t m; \
        memcpy(&m, p, 4); \
        name(st, a[0], &m); \
        memcpy(p, &m, 4); \
        return 0; \
    }
BOAT_RUNNER(boat_4AD350, FN_BOAT_WAIT_THEN_SAIL)
BOAT_RUNNER(boat_4AD450, FN_BOAT_SAIL)
BOAT_RUNNER(boat_4ADB40, FN_BOAT_TO_LANDING_4ADB40)
BOAT_RUNNER(boat_4ADD90, FN_BOAT_FLEE_4ADD90)
BOAT_RUNNER(boat_4AE270, FN_BOAT_MOVE)
BOAT_RUNNER(boat_4AE2C0, FN_BOAT_RAM)
BOAT_RUNNER(boat_4AE490, FN_BOAT_FLEE_B)
BOAT_RUNNER(boat_4AE650, FN_BOAT_FIGHT)
BOAT_RUNNER(boat_4AEB00, FN_BOAT_PATROL)
BOAT_RUNNER(boat_4AEC80, FN_BOAT_LEAVE)

static int32_t run_boat_4AD340(game_state *st, const int32_t *a)
{
    boat_4AD340(st, a[0]);
    return 0;
}

static int32_t run_boat_4AD4C0(game_state *st, const int32_t *a)
{
    boat_4AD4C0(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_boat_4ADC80(game_state *st, const int32_t *a)
{
    boat_4ADC80(st, a[0]);
    return 0;
}

static int32_t run_boat_4AE1F0(game_state *st, const int32_t *a)
{
    boat_4AE1F0(st, a[0]);
    return 0;
}

static int32_t run_boat_4AEAB0(game_state *st, const int32_t *a)
{
    int32_t *px = moving_ptr(st, a[0], FN_BOAT_RALLY_POINT);
    int32_t *py = px ? moving_ptr(st, a[1], FN_BOAT_RALLY_POINT) : NULL;
    if (!py) {
        return 0;
    }
    int32_t x, y;
    memcpy(&x, px, 4);
    memcpy(&y, py, 4);
    int8_t r = boat_4AEAB0(st, &x, &y);
    memcpy(px, &x, 4);
    memcpy(py, &y, 4);
    return r;
}

static int32_t run_boat_4AECE0(game_state *st, const int32_t *a)
{
    (void) a;
    boat_4AECE0(st);
    return 0;
}

static int32_t run_boat_4AED10(game_state *st, const int32_t *a)
{
    boat_4AED10(st, a[0]);
    return 0;
}

#define BOAT_ACTION(t) \
    static int32_t run_##t(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        figure_action_##t(st); \
        return 0; \
    }
BOAT_ACTION(5C)
BOAT_ACTION(5D)
BOAT_ACTION(64)
BOAT_ACTION(65)

const ported_function figd_boat_ports[] = {
    {0x004ADF60u, "figure_action_5C", 0, 0, 0, run_5C, 0},
    {0x004AEE10u, "figure_action_5D", 0, 0, 0, run_5D, 0},
    {0x004AF970u, "figure_action_64", 0, 0, 0, run_64, 0},
    {0x004AFC40u, "figure_action_65", 0, 0, 0, run_65, 0},
    {0x004AD340u, "boat_dying", 1, 0, 0, run_boat_4AD340, 0},
    {0x004AD350u, "boat_wait_then_sail", 2, 0, 0, run_boat_4AD350, 0},
    {0x004AD450u, "boat_sail", 2, 0, 0, run_boat_4AD450, 0},
    {0x004AD4C0u, "boat_land_troops", 3, 0, 0, run_boat_4AD4C0, 0},
    {0x004ADB40u, "boat_to_landing", 2, 0, 0, run_boat_4ADB40, 0},
    {0x004ADC80u, "boat_river_crossing", 1, 0, 0, run_boat_4ADC80, 0},
    {0x004ADD90u, "boat_flee", 2, 0, 0, run_boat_4ADD90, 0},
    {0x004AE1F0u, "boat_wait_then_enter", 2, 0, 0, run_boat_4AE1F0, 0},
    {0x004AE270u, "boat_move", 2, 0, 0, run_boat_4AE270, 0},
    {0x004AE2C0u, "boat_ram", 2, 0, 0, run_boat_4AE2C0, 0},
    {0x004AE490u, "boat_flee_b", 2, 0, 0, run_boat_4AE490, 0},
    {0x004AE650u, "boat_fight", 2, 0, 0, run_boat_4AE650, 0},
    {0x004AEAB0u, "boat_rally_point", 2, 8, 0, run_boat_4AEAB0, 0},
    {0x004AEB00u, "boat_patrol", 2, 0, 0, run_boat_4AEB00, 0},
    {0x004AEC80u, "boat_leave", 2, 0, 0, run_boat_4AEC80, 0},
    {0x004AECE0u, "boat_route_refresh", 0, 0, 0, run_boat_4AECE0, 0},
    {0x004AED10u, "boat_shoot", 1, 0, 0, run_boat_4AED10, 0},
};
const int figd_boat_ports_count = sizeof(figd_boat_ports) / sizeof(figd_boat_ports[0]);
