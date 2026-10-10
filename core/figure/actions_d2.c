/* Imhotep - figure actions, group D: type 0x55 (the laborer that leads a stone sledge to a
 * monument, 0x004A98B0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). The action works on the current figure
 * (0x00D056F8) and re-reads it after each call, as the original does. */
#include "figure/actions_d.h"

#include "figure/actions_d_local.h"
#include "state/ported.h"

#define PLAYER 0x00EAAC28u
#define LABOR_OBJECT 0x00D05720u /* @bin `this` of 0x004BEEB0, 0x004BF920, 0x004BF9D0 */
#define TYPE_PYRAMID 0xB7        /* @bin building types of the monuments the laborer serves */

/* @bin 0x004A99CB, 0x004A9DD5, 0x004A9FF3 (jump tables 0x004AB830, 0x004AB850, 0x004AB8B0): for
 * the monument types 0xE5, 0xEA, 0xEB, 0xEC a byte of the monument's object is cleared */
static uint32_t monument_object(int16_t type)
{
    switch (type) {
    case 0xE5: return 0x006F9320u;
    case 0xEA: return 0x006F5648u;
    case 0xEB: return 0x006DE9E0u;
    case 0xEC: return 0x006E9188u;
    default: return 0;
    }
}

static void monument_flag_clear(game_state *st, int16_t type)
{
    uint32_t o = monument_object(type);
    if (o != 0) {
        U8(st, o + 0x4C) = 0;
    }
}

/* the type of the building the laborer works for (+0x5E) */
static int16_t site_type(game_state *st)
{
    return S16(st, BUILDING_ADDR(S16(st, fig(st) + 0x5E)) + 0x10);
}

/* @bin 0x004AA529 .. 0x004AA61F, 0x004AA70D .. 0x004AA81D: the sledge behind the laborer: five
 * rope figures (type 0x60, +0x0B alternating 0, 1) and the sledge (type 0x56, carrying +0x7A),
 * each linked (+0x68) to the one before it */
static void sledge_create(game_state *st, uint32_t tb)
{
    int32_t prev = cur(st);
    int32_t v = 0;
    for (int i = 0; i < 5; i++) {
        uint32_t f = fig(st);
        int32_t n = GAME_CALL(st, 0x004B4800u, U8(st, tb + 1), 0x60, S16(st, f + 0x14), S16(st, f + 0x16), 0);
        uint32_t nf = FIGURE_ADDR(n);
        U8(st, nf + 0x38) = 8;
        S16(st, nf + 0x68) = (int16_t) prev;
        S16(st, nf + 0x5A) = 0;
        S16(st, nf + 0x36) = 0;
        S16(st, nf + 0x44) = 0;
        U8(st, nf + 0x0B) = (uint8_t) v;
        v = 1 - v;
        prev = n;
    }
    uint32_t f = fig(st);
    int32_t n = GAME_CALL(st, 0x004B4800u, U8(st, tb + 1), 0x56, S16(st, f + 0x14), S16(st, f + 0x16), 0);
    uint32_t nf = FIGURE_ADDR(n);
    U8(st, nf + 0x38) = 8;
    U8(st, nf + 0x0B) = U8(st, fig(st) + 0x7A);
    S16(st, nf + 0x68) = (int16_t) prev;
    S16(st, nf + 0x5A) = 0;
    S16(st, nf + 0x36) = 0;
    S16(st, nf + 0x44) = 0;
}

/* @bin 0x004AA07C / 0x004AA3D6: take 0x190 of the resource +0x7A from the storage +0x5E of the
 * player (0x0045B570; its result is what it could not take: given back with 0x0045B2A0). +0x74:
 * the amount carried (0x190 when all of it was there). */
static void take_stone(game_state *st, int32_t site)
{
    uint32_t f = fig(st);
    S16(st, f + 0x74) = 0;
    int32_t r = GAME_CALL(st, 0x0045B570u, S32(st, PLAYER), site, U8(st, f + 0x7A), 0x190);
    if (r == 0x190) {
        S16(st, fig(st) + 0x74) = 0;
    } else if (r == 0) {
        S16(st, fig(st) + 0x74) = 0x190;
    } else {
        GAME_CALL(st, 0x0045B2A0u, S32(st, PLAYER), S16(st, fig(st) + 0x5E), U8(st, fig(st) + 0x7A), 0x190 - r);
        S16(st, fig(st) + 0x74) = 0;
    }
}

/* the same with the site read once (0x004AA3D6 passes ebp to both calls) */
static void take_stone_at(game_state *st, int32_t site)
{
    uint32_t f = fig(st);
    S16(st, f + 0x74) = 0;
    int32_t r = GAME_CALL(st, 0x0045B570u, S32(st, PLAYER), site, U8(st, f + 0x7A), 0x190);
    if (r == 0x190) {
        S16(st, fig(st) + 0x74) = 0;
    } else if (r == 0) {
        S16(st, fig(st) + 0x74) = 0x190;
    } else {
        GAME_CALL(st, 0x0045B2A0u, S32(st, PLAYER), site, U8(st, fig(st) + 0x7A), 0x190 - r);
        S16(st, fig(st) + 0x74) = 0;
    }
}

/* @bin 0x004AA129 nothing taken: back to state 8 */
static void nothing_taken(game_state *st)
{
    uint32_t f = fig(st);
    U8(st, f + 0x0B) = 0;
    U8(st, f + 0x38) = 8;
    GAME_CALL(st, 0x004B3250u, cur(st));
    U8(st, fig(st) + 0x01) = 0;
}

/* @bin 0x004AA2AF: is the building next to the sledge (+0x68's destination) still accepting?
 * (types 0xB7, 0xD5, 0xDE, 0xE1, 0xE3, 0xE4: jump tables 0x004AB90C / 0x004AB8F0) */
static int site_accepts(game_state *st, int16_t b)
{
    int8_t r;
    switch (S16(st, BUILDING_ADDR(b) + 0x10)) {
    case TYPE_PYRAMID: r = (int8_t) GAME_CALL(st, 0x004EEA10u, b); break;
    case 0xD5: r = (int8_t) GAME_CALL(st, 0x00542110u, b); break;
    case 0xDE: r = (int8_t) GAME_CALL(st, 0x004E0530u, b); break;
    case 0xE1: r = (int8_t) GAME_CALLT(st, 0x006FD5D8u, 0x0040EC20u, b); break;
    case 0xE3: r = (int8_t) GAME_CALLT(st, 0x006FD368u, 0x00417590u, b); break;
    case 0xE4: r = (int8_t) GAME_CALLT(st, 0x006DE548u, 0x004E6B20u, b); break;
    default: return 1;
    }
    return r != 0;
}

/* state 0x10 (0x004AA062): load the sledge */
static void state_load(game_state *st, int32_t id, uint32_t home)
{
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x65) = 0;
    U8(st, f + 0x72) = 3;
    if (U8(st, f + 0x7A) == 0x22) {
        /* @bin 0x004AA07C resource 0x22: a sledge without ropes, going to +0x5C */
        take_stone(st, S16(st, f + 0x5E));
        if (S16(st, fig(st) + 0x74) == 0) {
            nothing_taken(st);
            return;
        }
        GAME_CALL(st, 0x004B3250u, cur(st));
        id = cur(st);
        f = FIGURE_ADDR(id);
        int16_t dest = S16(st, f + 0x5C);
        uint32_t db = BUILDING_ADDR(dest);
        U8(st, f + 0x38) = 0x24;
        S16(st, f + 0x5E) = dest;
        S16(st, f + 0x24) = S16(st, db + 0x26);
        S16(st, f + 0x26) = S16(st, db + 0x28);
        S32(st, f + 0x28) = S32(st, db + 0x0C);
        S16(st, f + 0x68) = 0;
        S16(st, f + 0x36) = 0;
        S16(st, f + 0x44) = 0;
        int32_t n = GAME_CALL(st, 0x004B4800u, U8(st, home + 1), 0x56, S16(st, f + 0x14), S16(st, f + 0x16), 0);
        uint32_t nf = FIGURE_ADDR(n);
        id = cur(st);
        U8(st, nf + 0x38) = 8;
        U8(st, nf + 0x0B) = U8(st, FIGURE_ADDR(id) + 0x7A);
        S16(st, nf + 0x68) = (int16_t) id;
        S16(st, nf + 0x5A) = 0;
        S16(st, nf + 0x36) = 0;
        S16(st, nf + 0x44) = 0;
        S16(st, nf + 0x2C) = 0;
        S16(st, nf + 0x2E) = 0;
        U8(st, FIGURE_ADDR(id) + 0x01) = 0;
        return;
    }
    int16_t prev = S16(st, f + 0x68);
    int16_t b = 0;
    if (prev != 0 && S16(st, FIGURE_ADDR(prev) + 0x68) == id) {
        b = S16(st, BUILDING_GRID + 2u * U32(st, FIGURE_ADDR(prev) + 0x28));
    }
    if (b == 0) {
        GAME_CALL(st, 0x004B3250u, id);
        U8(st, fig(st) + 0x0E) = 2;
        return;
    }
    if (!site_accepts(st, b)) {
        GAME_CALL(st, 0x004B3250u, cur(st));
        U8(st, fig(st) + 0x0E) = 2;
        return;
    }
    f = fig(st);
    int16_t site = S16(st, f + 0x5E);
    S16(st, f + 0x36) += 1;
    if (S16(st, f + 0x36) <= 4) {
        U8(st, f + 0x01) = 0;
        return;
    }
    take_stone_at(st, site);
    if (S16(st, fig(st) + 0x74) == 0) {
        nothing_taken(st);
        return;
    }
    GAME_CALL(st, 0x004B3250u, cur(st));
    id = cur(st);
    f = FIGURE_ADDR(id);
    /* @bin 0x004AA48E the building at the sledge's tile (+0x68's +0x20) */
    int16_t t = S16(st, BUILDING_GRID + 2u * U32(st, FIGURE_ADDR(S16(st, f + 0x68)) + 0x20));
    uint32_t tb = BUILDING_ADDR(t);
    int16_t type = S16(st, tb + 0x10);
    if (type == TYPE_PYRAMID) {
        U8(st, f + 0x38) = 0x1E;
        S16(st, f + 0x36) = 0;
        S16(st, f + 0x5E) = t;
        GAME_CALL(st, 0x004F3000u, id, t);
        f = fig(st);
        int at_dest = S16(st, f + 0x14) == S16(st, f + 0x24) && S16(st, f + 0x16) == S16(st, f + 0x26);
        if (!at_dest) {
            GAME_CALL(st, 0x004B2D20u, cur(st));
            if (S16(st, fig(st) + 0x3A) <= 0) {
                GAME_CALL(st, 0x0045B2A0u, S32(st, PLAYER), site, U8(st, fig(st) + 0x7A), 0x190);
                U8(st, fig(st) + 0x38) = 8;
                return;
            }
        }
        sledge_create(st, tb);
        if (at_dest) {
            U8(st, fig(st) + 0x38) = 0x12;
            GAME_CALL(st, 0x004F2CF0u, cur(st));
            GAME_CALL(st, 0x004F1CB0u, cur(st));
        }
    } else {
        uint8_t next;
        switch (type) {
        case 0xD5: next = 0x17; break;
        case 0xE3: next = 0x22; break;
        case 0xE4: next = 0x23; break;
        case 0xE1: next = 0x1F; break;
        case 0xDE: next = 0x1A; break;
        default: next = 0; break;
        }
        if (next == 0) {
            U8(st, f + 0x38) = 8;
        } else {
            U8(st, f + 0x38) = next;
            S16(st, f + 0x24) = S16(st, tb + 0x26);
            S16(st, f + 0x26) = S16(st, tb + 0x28);
            S32(st, f + 0x28) = S32(st, tb + 0x0C);
            S16(st, f + 0x44) = 0;
            sledge_create(st, tb);
        }
    }
    f = fig(st);
    S16(st, f + 0x36) = 0;
    U8(st, f + 0x01) = 0;
}

/* the laborer arrived next to its sledge's helper (+0x68 of type 0x51 in state 0x0A): that one
 * goes on (0x0B); the laborer waits (0x1D) */
static void arrived_wait(game_state *st, uint32_t counter)
{
    uint32_t f = fig(st);
    uint32_t p = FIGURE_ADDR(S16(st, f + 0x68));
    if (U8(st, p + 0x0A) == 0x51 && U8(st, p + 0x38) == 0x0A) {
        int go = 1;
        if (counter != 0) {
            /* @bin 0x004AB0AD a round-robin counter of the monument object (+8, limit +4) */
            S32(st, counter) += 1;
            if (S32(st, counter) < S32(st, counter - 4)) {
                go = 0;
            } else {
                S32(st, counter) = 0;
            }
        }
        if (go) {
            U8(st, p + 0x38) = 0x0B;
            S16(st, p + 0x36) = 0;
        }
    }
    U8(st, f + 0x38) = 0x1D;
    S16(st, f + 0x36) = 0;
}

/* @bin 0x004AB157 / 0x004AAFF9: not arrived: route again (9) or give up (0x0A) */
static void moving_else(game_state *st, int8_t dir)
{
    if (dir == 9) {
        GAME_CALL(st, 0x004B3250u, cur(st));
    } else if (dir == 0x0A) {
        U8(st, fig(st) + 0x38) = 8;
    }
}

/* @bin 0x004AB010, 0x004AB04B, 0x004AB0D3, 0x004AB2E1, 0x004AB505: move to the monument */
static void state_move_to_monument(game_state *st, int32_t id, int32_t speed, uint32_t counter, int route_44)
{
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x65) = 0;
    U8(st, f + 0x72) = 7;
    GAME_CALL(st, 0x004A5DA0u, id, speed);
    int8_t dir = S8(st, fig(st) + 0x11);
    if (dir == 8) {
        arrived_wait(st, counter);
        return;
    }
    if (route_44 && dir == 9) {
        /* @bin 0x004AB57C */
        GAME_CALL(st, 0x004B3250u, cur(st));
        S16(st, fig(st) + 0x44) = 0;
        return;
    }
    moving_else(st, dir);
}

/* @bin 0x004AB16A, 0x004AB5A8, 0x004AB369: move, then state `next` */
static void state_move_then(game_state *st, int32_t id, int32_t speed, uint8_t next, int reroute)
{
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x65) = 0;
    U8(st, f + 0x72) = 7;
    GAME_CALL(st, 0x004A5DA0u, id, speed);
    f = fig(st);
    int8_t dir = S8(st, f + 0x11);
    if (dir == 8) {
        U8(st, f + 0x38) = next;
        S16(st, f + 0x36) = 0;
    } else if (reroute) {
        moving_else(st, dir);
    } else if (dir == 9 || dir == 0x0A) {
        U8(st, f + 0x38) = 8;
    }
}

/* @bin 0x004AB1C6, 0x004AB3B2, 0x004AB5F1: work at the building on the destination tile; every
 * 4th update while working, an effect at the figure's tile */
static void state_work(game_state *st, uint32_t this_ptr, uint32_t work, uint32_t effect)
{
    uint32_t f = fig(st);
    U8(st, f + 0x65) = 0;
    int16_t t = S16(st, BUILDING_GRID + 2u * U32(st, f + 0x28));
    if (t == 0) {
        U8(st, f + 0x38) = 8;
        return;
    }
    if (this_ptr) {
        GAME_CALLT(st, this_ptr, work, t, cur(st), 8);
    } else {
        GAME_CALL(st, work, t, cur(st), 8);
    }
    f = fig(st);
    if (U8(st, f + 0x38) != 8) {
        return;
    }
    U8(st, f + 0x154) += 1;
    if (U8(st, f + 0x154) >= 4) {
        U8(st, f + 0x154) = 0;
        return;
    }
    if (this_ptr) {
        GAME_CALLT(st, this_ptr, effect, S16(st, f + 0x14), S16(st, f + 0x16), cur(st));
    } else {
        GAME_CALL(st, effect, S16(st, f + 0x14), S16(st, f + 0x16), cur(st));
    }
}

int32_t pyramid_level_next(int32_t level)
{
    /* @bin 0x004F3360 */
    level += 1;
    return level == 3 ? 4 : level;
}

int8_t pyramid_block_slot(game_state *st, int32_t b, int32_t *px, int32_t *py, int8_t check)
{
    /* @bin 0x004F3370 among the parts of the pyramid b (its kind +0x54 gives how many, tables
     * 0x0056F7D8 / 0x0056F7EC by the difficulty-like 0x005E4EF4), the free corner (+0x5B: mask of
     * the corners done; table 0x005F6BBC / 0x005F6BCC) of the lowest level (+0x55) nearest to a
     * reference point chosen by the level (table 0x005F6B68) */
    uint32_t bb = BUILDING_ADDR(b);
    int32_t found = 0, best_level = 0, best_dist = 99999;
    int32_t x0 = S16(st, bb + 0x06), y0 = S16(st, bb + 0x08);
    uint32_t kind = U8(st, bb + 0x54);
    int32_t c1 = b + S32(st, 0x005F6B94u + 4u * kind);
    int32_t x1 = S16(st, BUILDING_ADDR(c1) + 0x06), y1 = S16(st, BUILDING_ADDR(c1) + 0x08);
    int32_t limit = S32(st, 0x005E4EF4u) != 5 ? 10 : 5;
    int8_t busy = 0;
    for (int32_t i = b;; i++) {
        int32_t end = b + S32(st, (S32(st, 0x005E4EF4u) == 5 ? 0x0056F7ECu : 0x0056F7D8u) + 4u * kind);
        if (i >= end) {
            break;
        }
        uint32_t ib = BUILDING_ADDR(i);
        if (check && S16(st, ib + 0x64) > 0 && (int8_t) GAME_CALL(st, 0x004EEA10u, i) != 0) {
            busy = 1;
            continue;
        }
        int32_t level = U8(st, ib + 0x55);
        if (level == 3) {
            level = 4;
        }
        int32_t mask = U8(st, ib + 0x5B);
        if (mask == 0x0F) {
            mask = 0;
            level = pyramid_level_next(level);
        }
        if (level > limit) {
            continue;
        }
        int32_t best = 99999, bx = 0, by = 0;
        for (int c = 0; c < 4; c++) {
            if (mask & (1 << c)) {
                continue;
            }
            int32_t sx = S16(st, ib + 0x06) + S32(st, 0x005F6BBCu + 4u * (uint32_t) c);
            int32_t sy = S16(st, ib + 0x08) + S32(st, 0x005F6BCCu + 4u * (uint32_t) c);
            int32_t d;
            switch (S32(st, 0x005F6B68u + 4u * (uint32_t) level)) {
            case 0:
                d = (sy - y0) * (sy - y0) + (sx - x0) * (sx - x0);
                break;
            case 1:
                d = (sy - y1) * (sy - y1) + (sx - x1) * (sx - x1);
                break;
            case 2:
                d = 1000 - (sy - y1) * (sy - y1) - (sx - x1) * (sx - x1);
                break;
            default:
                d = 99999;
                break;
            }
            if (d < best) {
                best = d;
                bx = sx;
                by = sy;
            }
        }
        if (found != 0 && (level > best_level || (level == best_level && best >= best_dist))) {
            continue;
        }
        found = i;
        best_dist = best;
        *px = bx;
        *py = by;
        best_level = level;
    }
    if (found != 0) {
        return 1;
    }
    /* @bin 0x004F3605 a finished-but-capstone pyramid: its own tile + (1, 1) */
    if (busy || S32(st, 0x005E4EF4u) == 5 || U8(st, bb + 0x55) != 0x0A || U8(st, bb + 0x5B) != 0x0F) {
        return 0;
    }
    if (check) {
        for (int32_t j = 0; j < 4; j++) {
            if (S16(st, BUILDING_ADDR(b + j) + 0x64) > 0 && (int8_t) GAME_CALL(st, 0x004EEA10u, b + j) != 0) {
                return 0;
            }
        }
    }
    *px = S16(st, bb + 0x06) + 1;
    *py = S16(st, bb + 0x08) + 1;
    return 1;
}

/* ---- the laborers' work assignment (the object at 0x00D05720, thiscall methods) ---- */

#define BUILDING_MAX 0x0078D790u   /* @bin the highest building id in use */
#define WORK_COUNT 0x0078D7F0u     /* @bin candidates found by 0x004BEEB0 */
#define WORK_INDEX 0x0078D7F4u     /* @bin the candidate being examined */
#define WORK_LIST 0x0078D9A8u      /* @bin candidate building ids (s16, at most 500) */
#define DIST_X 0x00D3B4FCu         /* @bin |dx| of the last distance */
#define DIST_Y 0x00D3B4F8u         /* @bin |dy| of the last distance */
#define BUILDINGS 0x00BCA350u      /* @bin `this` of the building tests 0x00488030, 0x004894A0 */

static int8_t building_test(game_state *st, uint32_t f, int32_t b)
{
    return (int8_t) GAME_CALLT(st, BUILDINGS, f, b);
}

/* |a - b| as the original computes it (also stored in the globals) */
static int32_t tile_distance(game_state *st, uint32_t f, uint32_t b)
{
    int32_t fx = S16(st, f + 0x14), bx = S16(st, b + 0x06);
    int32_t dx = fx > bx ? fx - bx : (fx < bx ? bx - fx : 0);
    S32(st, DIST_X) = dx;
    int32_t fy = S16(st, f + 0x16), by = S16(st, b + 0x08);
    int32_t dy = fy > by ? fy - by : (fy < by ? by - fy : 0);
    S32(st, DIST_Y) = dy;
    return dx >= dy ? dx : dy;
}

/* a building that needs laborers: 0x00488030 and 0x004894A0 true, no laborer assigned (+0xB4)
 * and 0x00459240 0 */
static int labor_wanted(game_state *st, int32_t i)
{
    uint32_t b = BUILDING_ADDR(i);
    return U8(st, b) == 1 && building_test(st, 0x00488030u, i) != 0 && building_test(st, 0x004894A0u, i) != 0
           && S16(st, b + 0xB4) == 0 && GAME_CALL(st, 0x00459240u, i) == 0;
}

/* the monument work tests: (building, figure, commit) -> a byte */
static int8_t monument_work(game_state *st, int16_t type, int32_t b, int32_t fid, int32_t commit)
{
    switch (type) {
    case 0xD5: return (int8_t) GAME_CALL(st, 0x005421D0u, b, fid, commit);
    case 0xE3: return (int8_t) GAME_CALLT(st, 0x006FD368u, 0x004176A0u, b, fid, commit);
    case 0xE4: return (int8_t) GAME_CALLT(st, 0x006DE548u, 0x004E6C10u, b, fid, commit);
    case 0xE1: return (int8_t) GAME_CALLT(st, 0x006FD5D8u, 0x0040ED70u, b, fid, commit);
    case 0xDE: return (int8_t) GAME_CALL(st, 0x004E05F0u, b, fid, commit);
    case 0xE5: return (int8_t) GAME_CALLT(st, 0x006F9320u, 0x00559110u, b, fid, commit);
    case 0xEA: return (int8_t) GAME_CALLT(st, 0x006F5648u, 0x00559110u, b, fid, commit);
    case 0xEB: return (int8_t) GAME_CALLT(st, 0x006DE9E0u, 0x00559110u, b, fid, commit);
    case 0xEC: return (int8_t) GAME_CALLT(st, 0x006E9188u, 0x00559110u, b, fid, commit);
    default: return 0;
    }
}

static void work_add(game_state *st, int32_t *values, int32_t b, int32_t v)
{
    int32_t n = S32(st, WORK_COUNT);
    if (n < 500) {
        S16(st, WORK_LIST + 2u * (uint32_t) n) = (int16_t) b;
        values[n] = v;
        S32(st, WORK_COUNT) = n + 1;
    }
}

void labor_assign(game_state *st, uint32_t self, int32_t fid)
{
    /* @bin 0x004BEEB0 (thiscall) find work for the laborer fid: a building that wants laborers
     * nearby (mode 0 of +0x32FB4), else a monument; the nearest one it can reach */
    uint32_t f = FIGURE_ADDR(fid);
    int32_t values[500];
    for (;;) {
        S32(st, WORK_COUNT) = 0;
        int found_unreachable = 0;
        int32_t best_value = 0;
        int32_t mode = S32(st, self + 0x32FB4u);
        if (mode != 1 && mode != 2 && mode != 3 && S32(st, BUILDING_MAX) >= 1) {
            for (int32_t i = 1; i <= S32(st, BUILDING_MAX); i++) {
                uint32_t b = BUILDING_ADDR(i);
                if (!labor_wanted(st, i) || S16(st, b + 0x20) == 0
                    || GAME_CALL(st, 0x00480180u, S16(st, b + 0x06), S16(st, b + 0x08), U8(st, b + 0x03)) == 0) {
                    continue;
                }
                /* @bin 0x004BEFF0 within 40 tiles, 0x0078D79C <= 0, +0xA7 below 0x30 */
                if (tile_distance(st, f, b) > 0x28 || S32(st, 0x0078D79Cu) > 0 || U8(st, b + 0xA7) >= 0x30) {
                    continue;
                }
                int32_t n = S32(st, WORK_COUNT);
                S16(st, WORK_LIST + 2u * (uint32_t) n) = (int16_t) i;
                n++;
                S32(st, WORK_COUNT) = n >= 500 ? 499 : n;
            }
        }
        if (S32(st, WORK_COUNT) == 0) {
            if (S32(st, BUILDING_MAX) < 1) {
                U8(st, f + 0x0E) = 2;
                break;
            }
            for (int32_t i = 1; i <= S32(st, BUILDING_MAX); i++) {
                uint32_t b = BUILDING_ADDR(i);
                if (U8(st, b) != 1) {
                    continue;
                }
                int16_t type = S16(st, b + 0x10);
                if (type == TYPE_PYRAMID) {
                    if (U8(st, b + 0x55) > 1) {
                        int32_t x, y;
                        if (pyramid_block_slot(st, i, &x, &y, 1) != 0) {
                            work_add(st, values, i, 0);
                        }
                    } else {
                        int32_t r = GAME_CALL(st, 0x004F29E0u, i, 0);
                        if (r != 0) {
                            work_add(st, values, i, r);
                        }
                    }
                    /* @bin 0x004BF118 skip the pyramid's other parts */
                    uint32_t t = S32(st, 0x005E4EF4u) == 5 ? 0x0056F7ECu : 0x0056F7D8u;
                    i += S32(st, t + 4u * U8(st, b + 0x54)) - 1;
                    continue;
                }
                if (type == 0xD5 || type == 0xE3 || type == 0xE4 || type == 0xE1 || type == 0xDE || type == 0xE5
                    || type == 0xEA || type == 0xEB || type == 0xEC) {
                    if (monument_work(st, type, i, fid, 0) != 0) {
                        work_add(st, values, i, 0);
                    }
                }
            }
        }
        if (S32(st, WORK_COUNT) <= 0) {
            U8(st, f + 0x0E) = 2;
            break;
        }
        /* @bin 0x004BF3E6 the nearest candidate (+0xA7 added for the buildings of 0x00488030) */
        int32_t best = 0, best_d = 10000;
        S32(st, WORK_INDEX) = 0;
        for (int32_t k = 0; k < S32(st, WORK_COUNT);) {
            int32_t b = S16(st, WORK_LIST + 2u * (uint32_t) k);
            int32_t v = values[k];
            k++;
            S32(st, WORK_INDEX) = k;
            uint32_t bb = BUILDING_ADDR(b);
            int32_t d = tile_distance(st, f, bb);
            if (building_test(st, 0x00488030u, b) != 0) {
                d += U8(st, bb + 0xA7);
            }
            if (d < best_d) {
                if (building_test(st, 0x00488030u, b) != 0
                    && GAME_CALL(st, 0x00519840u, S16(st, f + 0x14), S16(st, f + 0x16), S16(st, bb + 0x26),
                                 S16(st, bb + 0x28)) == 0) {
                    S16(st, bb + 0xB4) = -1; /* unreachable: excluded, and the search runs again */
                    found_unreachable = 1;
                } else {
                    best_d = d;
                    best = b;
                    best_value = v;
                }
            }
            k = S32(st, WORK_INDEX);
        }
        if (best == 0) {
            if (found_unreachable) {
                continue;
            }
            U8(st, f + 0x0E) = 2;
            break;
        }
        uint32_t bb = BUILDING_ADDR(best);
        if (building_test(st, 0x00488030u, best) != 0) {
            if (road_near(st, bb) == 0) {
                U8(st, f + 0x0E) = 2;
                break;
            }
            S16(st, bb + 0xB4) = (int16_t) fid;
            S16(st, f + 0x5E) = (int16_t) best;
            U8(st, f + 0x38) = 0x0A;
            S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
            /* @bin 0x004BF5E8 the current figure's +0x26, as in the original */
            S16(st, fig(st) + 0x26) = S16(st, ROUTE_DEST_Y);
            S16(st, f + 0x44) = 0;
            break;
        }
        int16_t type = S16(st, bb + 0x10);
        if (type == TYPE_PYRAMID) {
            if (U8(st, bb + 0x55) > 1) {
                S16(st, f + 0x5E) = (int16_t) best;
                S16(st, f + 0x68) = 0;
                U8(st, f + 0x38) = 0x11;
                S16(st, f + 0x36) = 0;
                GAME_CALL(st, 0x004F3000u, fid, best);
                break;
            }
            /* @bin 0x004BF662 the stone for the next level, by the difficulty-like 0x005E4EF4 */
            int8_t r = (int8_t) GAME_CALL(st, 0x004EFD90u,
                                          S16(st, BUILDING_GRID + 2u * U32(st, FIGURE_ADDR(best_value) + 0x20)));
            int32_t g = S32(st, 0x005E4EF4u);
            uint8_t resource;
            if (r != 0) {
                resource = (g == 2 || g == 5) ? 0x0C : 0x18; /* @bin jump table 0x004BF900 */
            } else {
                resource = g == 3 ? 0x18 : (g == 5 ? 0x0C : 0x19);
            }
            int32_t n = GAME_CALL(st, 0x0045BB50u, best, resource, 1);
            if (n == 0) {
                U8(st, f + 0x0E) = 2;
                break;
            }
            U8(st, f + 0x7A) = resource;
            S16(st, f + 0x74) = 0;
            S16(st, f + 0x5E) = (int16_t) n;
            S16(st, f + 0x68) = (int16_t) best_value;
            U8(st, f + 0x38) = 0x0F;
            S16(st, f + 0x36) = 0;
            S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
            S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
            S16(st, f + 0x44) = 0;
            GAME_CALL(st, 0x004B3250u, fid);
            S16(st, FIGURE_ADDR(best_value) + 0x68) = (int16_t) fid;
            break;
        }
        if (type == 0xD5 || type == 0xE3 || type == 0xE4 || type == 0xE1 || type == 0xDE || type == 0xE5
            || type == 0xEA || type == 0xEB || type == 0xEC) {
            if (monument_work(st, type, best, fid, 1) == 0) {
                U8(st, f + 0x0E) = 2;
            }
        }
        break;
    }
    /* @bin 0x004BF8AB the exclusions (+0xB4 = -1) are lifted */
    for (int32_t i = 1; i <= S32(st, BUILDING_MAX); i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b) == 1 && building_test(st, 0x00488030u, i) != 0 && S16(st, b + 0xB4) < 0) {
            S16(st, b + 0xB4) = 0;
        }
    }
}

void labor_arrived(game_state *st, int32_t fid)
{
    /* @bin 0x004BF920 (thiscall, `this` unused) the laborer reached the building it works for */
    uint32_t f = FIGURE_ADDR(fid);
    int16_t target = S16(st, f + 0x5E);
    int16_t home = S16(st, f + 0x5A);
    if (building_test(st, 0x00488030u, target) == 0) {
        U8(st, BUILDING_ADDR(home) + 0xA8) = 0x20;
        return;
    }
    U8(st, f + 0x0E) = 2;
    if (S32(st, 0x0078D79Cu) != 0) {
        return;
    }
    U8(st, BUILDING_ADDR(home) + 0xA7) = 0x20;
    uint32_t t = BUILDING_ADDR(target);
    U8(st, t + 0xA7) = 0x60;
    U8(st, t + 0xA6) += 1;
    S16(st, t + 0xB4) = 0;
    /* @bin 0x004BF9A3 per building type, 40 bytes each at 0x00705EB8 */
    S16(st, t + 0x40) = S16(st, 0x00705EB8u + 40u * (uint32_t) S16(st, t + 0x10));
}

int8_t labor_wanted_anywhere(game_state *st, uint32_t self)
{
    /* @bin 0x004BF9D0 (thiscall) is there a building that wants laborers? (mode 0 only) */
    int32_t mode = S32(st, self + 0x32FB4u);
    if (mode == 1 || mode == 2 || mode == 3) {
        return 0;
    }
    for (int32_t i = 1; i < BUILDING_COUNT; i++) {
        if (labor_wanted(st, i)) {
            return 1;
        }
    }
    return 0;
}

int8_t laborer_sledge_ready(game_state *st, int32_t id)
{
    /* @bin 0x004A9730 the building at the sledge's destination (+0x68's +0x28) still takes stone
     * (types 0xB7, 0xD5, 0xDE, 0xE1, 0xE3, 0xE4: jump tables 0x004A9878 / 0x004A985C); if not, a
     * new sledge partner from 0x004F29E0; none: state 8 (unless the building still has +0xC8) */
    uint32_t f = FIGURE_ADDR(id);
    int16_t p = S16(st, f + 0x68);
    if (p == 0) {
        U8(st, f + 0x38) = 8;
        return 0;
    }
    int16_t b = S16(st, BUILDING_GRID + 2u * U32(st, FIGURE_ADDR(p) + 0x28));
    uint32_t bb = BUILDING_ADDR(b);
    int8_t r;
    switch (S16(st, bb + 0x10)) {
    case TYPE_PYRAMID: r = (int8_t) GAME_CALL(st, 0x004EEA10u, b); break;
    case 0xD5: r = (int8_t) GAME_CALL(st, 0x00542110u, b); break;
    case 0xDE:
        if ((int8_t) GAME_CALL(st, 0x004E0530u, b) != 0) {
            return 1;
        }
        U8(st, f + 0x38) = 8;
        return 0;
    case 0xE1: r = (int8_t) GAME_CALLT(st, 0x006FD5D8u, 0x0040EC20u, b); break;
    case 0xE3: r = (int8_t) GAME_CALLT(st, 0x006FD368u, 0x00417590u, b); break;
    case 0xE4: r = (int8_t) GAME_CALLT(st, 0x006DE548u, 0x004E6B20u, b); break;
    default:
        U8(st, f + 0x38) = 8;
        return 0;
    }
    if (r != 0) {
        return 1;
    }
    int32_t n = GAME_CALL(st, 0x004F29E0u, b, 1);
    if (n != 0) {
        S16(st, f + 0x68) = (int16_t) n;
        S16(st, FIGURE_ADDR(n) + 0x68) = (int16_t) id;
        return 1;
    }
    if (S16(st, bb + 0xC8) == 0) {
        U8(st, f + 0x38) = 8;
    }
    return 0;
}

void figure_action_55(game_state *st)
{
    /* @bin 0x004A98B0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + 0x72) = 1;
    U8(st, f + 0x0C) = 0;
    S16(st, f + 0x42) = 0x200;
    int32_t speed = S32(st, 0x007090ECu + 0x38u * U8(st, f + 0x0A));
    next_frame(st, f);
    U8(st, f + 0x6B) += 1;
    if (U8(st, f + 0x6B) >= 0x78) {
        U8(st, f + 0x6B) = 0;
    }
    uint32_t home = BUILDING_ADDR(S16(st, f + 0x5A));
    if (U8(st, home) != 1) {
        U8(st, f + 0x0E) = 2;
        return;
    }
    /* @bin 0x004A996B jump table 0x004AB7AC for states 4 .. 0x24 */
    switch (U8(st, f + 0x38)) {
    case 4:
        monument_flag_clear(st, site_type(st));
        GAME_CALL(st, 0x004B1D50u, id);
        break;
    case 5:
        GAME_CALL(st, 0x004B17D0u, id);
        f = fig(st);
        U8(st, f + 0x01) += 1;
        if (U8(st, f + 0x01) >= 0x20) {
            U8(st, f + 0x01) = 0;
        }
        break;
    case 6:
        S32(st, f + 0x04) = 0;
        break;
    case 7:
        U8(st, f + 0x01) = 0;
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 0x64) {
            S16(st, f + 0x36) = 0;
            if ((int8_t) GAME_CALL(st, 0x004A35E0u, id) == 0) {
                U8(st, fig(st) + 0x38) = 8;
            }
        }
        break;
    case 8:
        U8(st, f + 0x65) = 1;
        U8(st, f + 0x154) = 0;
        U8(st, f + 0x01) = 0;
        U8(st, f + 0x6B) = 0;
        S16(st, f + 0x68) = 0;
        S16(st, f + 0x36) -= 1;
        if (S16(st, f + 0x36) > 0) {
            break;
        }
        if (road_near(st, home) == 0) {
            U8(st, fig(st) + 0x38) = 4;
            break;
        }
        if (S32(st, 0x0078D7A4u) >= 1) { /* @bin 0x004A9A97 */
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        U8(st, f + 0x38) = 9;
        S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
        S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
        move_to_dest(st, id, f);
        S16(st, fig(st) + 0x44) = 0;
        break;
    case 9:
        U8(st, f + 0x0C) = 1;
        U8(st, f + 0x65) = 1;
        if (GAME_CALL(st, 0x004B3FF0u, id, 1) == 1) {
            labor_assign(st, LABOR_OBJECT, cur(st));
        }
        U8(st, fig(st) + 0x65) = 1;
        break;
    case 0x0A: {
        if (U8(st, BUILDING_ADDR(S16(st, f + 0x5E))) != 1) {
            labor_assign(st, LABOR_OBJECT, id);
            break;
        }
        U8(st, f + 0x72) = 7;
        U8(st, f + 0x65) = 0;
        S16(st, f + 0x44) += 1;
        if (S16(st, f + 0x44) >= 0xC80) {
            U8(st, f + 0x0E) = 2;
        }
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8) {
            labor_arrived(st, id);
        } else if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, id);
        } else if (dir == 0x0A) {
            U8(st, f + 0x0E) = 2;
        }
        break;
    }
    case 0x0C:
        U8(st, f + 0x65) = 0;
        S16(st, f + 0x44) += 1;
        if (S16(st, f + 0x44) >= S16(st, f + 0x42)) {
            if (road_near(st, home) == 0) {
                id = cur(st);
                U8(st, FIGURE_ADDR(id) + 0x0E) = 2;
                GAME_CALL(st, 0x004B1EF0u, id, 1);
                break;
            }
            id = cur(st);
            f = FIGURE_ADDR(id);
            U8(st, f + 0x38) = 0x0D;
            S16(st, f + 0x24) = S16(st, ROUTE_DEST_X);
            S16(st, f + 0x26) = S16(st, ROUTE_DEST_Y);
        }
        GAME_CALL(st, 0x004B1EF0u, id, 1);
        break;
    case 0x0D: {
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8 || dir == 9 || dir == 0x0A) {
            U8(st, f + 0x0E) = 2;
        }
        break;
    }
    case 0x0F: {
        U8(st, f + 0x65) = 0;
        U8(st, f + 0x0C) = 0;
        U8(st, f + 0x72) = 3;
        int16_t prev = S16(st, f + 0x68);
        int ok = 0;
        if (prev != 0) {
            uint32_t p = FIGURE_ADDR(prev);
            ok = S16(st, p + 0x68) == id && S16(st, BUILDING_GRID + 2u * U32(st, p + 0x28)) != 0;
        }
        if (!ok) {
            GAME_CALL(st, 0x004B3250u, id);
            U8(st, fig(st) + 0x0E) = 2;
            break;
        }
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8) {
            U8(st, f + 0x38) = 0x10;
        } else if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, cur(st));
            U8(st, fig(st) + 0x0E) = 2;
            monument_flag_clear(st, site_type(st));
        } else if (dir == 0x0A) {
            U8(st, f + 0x0E) = 2;
            monument_flag_clear(st, site_type(st));
        }
        break;
    }
    case 0x10:
        state_load(st, id, home);
        break;
    case 0x1E: {
        if (laborer_sledge_ready(st, id) == 0) {
            break;
        }
        f = fig(st);
        if (S16(st, f + 0x3A) <= 0) {
            S16(st, f + 0x36) += 1;
            int16_t v = S16(st, f + 0x36);
            if (v % 30 == 0) { /* @bin 0x004AAC15 */
                if (v >= 0x78) {
                    U8(st, f + 0x38) = 8;
                } else {
                    GAME_CALL(st, 0x004B2D20u, cur(st));
                }
            }
        }
        if ((int8_t) GAME_CALL(st, 0x004F2BA0u, cur(st)) == 0) {
            break;
        }
        id = cur(st);
        U8(st, FIGURE_ADDR(id) + 0x38) = 0x11;
    }
    /* fall through - state 0x11 (0x004AAC66) */
    case 0x11: {
        f = FIGURE_ADDR(id);
        U8(st, f + 0x72) = 7;
        U8(st, f + 0x65) = 0;
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 0x1E) {
            S16(st, f + 0x36) = 0;
            int16_t t = S16(st, BUILDING_GRID + 2u * U32(st, f + 0x28));
            if (t != 0 && S16(st, BUILDING_ADDR(t) + 0x10) == TYPE_PYRAMID && U8(st, BUILDING_ADDR(t) + 0x55) > 1) {
                /* @bin 0x004AACF1 the slot itself is not used here */
                int32_t x, y;
                if (pyramid_block_slot(st, t - S16(st, BUILDING_ADDR(t) + 0x12), &x, &y, 1) == 0) {
                    U8(st, fig(st) + 0x38) = 8;
                }
            }
        }
        f = fig(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8) {
            int16_t t = S16(st, BUILDING_GRID + 2u * U32(st, f + 0x28));
            if (t == 0 || S16(st, BUILDING_ADDR(t) + 0x10) != TYPE_PYRAMID) {
                U8(st, f + 0x38) = 8;
            } else if (U8(st, BUILDING_ADDR(t) + 0x55) > 1) {
                if ((int8_t) GAME_CALL(st, 0x004F4320u, cur(st)) == 0) {
                    U8(st, fig(st) + 0x38) = 8;
                } else {
                    U8(st, fig(st) + 0x38) = 0x15;
                }
            } else {
                U8(st, f + 0x38) = 0x12;
                GAME_CALL(st, 0x004F2CF0u, cur(st));
                GAME_CALL(st, 0x004F1CB0u, cur(st));
            }
        } else if (dir == 0x0A) {
            U8(st, f + 0x0E) = 2;
        } else if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, cur(st));
        }
        break;
    }
    case 0x12: {
        U8(st, f + 0x65) = 0;
        int16_t p = S16(st, f + 0x68);
        int ready = 0;
        if (p != 0) {
            int16_t t = S16(st, BUILDING_GRID + 2u * U32(st, FIGURE_ADDR(p) + 0x28));
            if (t != 0 && (int8_t) GAME_CALL(st, 0x004EEA10u, t) != 0) {
                ready = 1;
            }
        }
        if (!ready) {
            int16_t m = S16(st, fig(st) + 0x4E);
            int32_t r = GAME_CALL(st, 0x004F29E0u, m, 1);
            if (r != 0) {
                int32_t c = cur(st);
                S16(st, FIGURE_ADDR(c) + 0x68) = (int16_t) r;
                S16(st, FIGURE_ADDR(r) + 0x68) = (int16_t) c;
            } else {
                if (S16(st, BUILDING_ADDR(m) + 0xC8) != 0) {
                    break;
                }
                f = fig(st);
                U8(st, f + 0x38) = 8;
                S16(st, f + 0x120) = 0;
            }
        }
        GAME_CALL(st, 0x004F04D0u, cur(st), 2, 8, 0x13);
        f = fig(st);
        if (U8(st, f + 0x38) == 8) {
            S16(st, f + 0x120) = 0;
        }
        break;
    }
    case 0x13: {
        U8(st, f + 0x65) = 0;
        int16_t p = S16(st, f + 0x68);
        int16_t t = S16(st, BUILDING_GRID + 2u * U32(st, FIGURE_ADDR(p) + 0x20));
        if (S16(st, BUILDING_ADDR(t) + 0x10) == TYPE_PYRAMID) {
            GAME_CALL(st, 0x004F02F0u, id, t, S16(st, f + 0x50), 0);
            U8(st, fig(st) + 0x38) = 0x14;
        } else {
            U8(st, f + 0x38) = 8;
            S16(st, f + 0x120) = 0;
        }
        break;
    }
    case 0x14: {
        U8(st, f + 0x65) = 0;
        if ((int8_t) GAME_CALL(st, 0x004F1D10u, id, 1) != 0) {
            int16_t p = S16(st, fig(st) + 0x68);
            uint32_t pf = FIGURE_ADDR(p);
            int notify = 0;
            if (U8(st, pf + 0x0A) == 0x51 && U8(st, pf + 0x38) == 0x0A) {
                U8(st, pf + 0x38) = 0x0B;
                notify = 1;
            } else if (U8(st, pf + 0x0A) == 0x50 && U8(st, pf + 0x38) == 0x0B) {
                U8(st, pf + 0x38) = 0x0C;
                notify = 1;
            }
            if (notify) {
                S16(st, pf + 0x36) = 0;
                GAME_CALL(st, 0x004F2960u, p);
            }
            f = fig(st);
            U8(st, f + 0x38) = 0x1D;
            S16(st, f + 0x36) = 0;
            break;
        }
        moving_else(st, S8(st, fig(st) + 0x11));
        break;
    }
    case 0x15:
        U8(st, f + 0x0C) = 1;
        U8(st, f + 0x65) = 0;
        if (GAME_CALL(st, 0x004B3FF0u, id, 1) == 1) {
            f = fig(st);
            U8(st, f + 0x38) = 0x16;
            S16(st, f + 0x36) = 0;
        }
        break;
    case 0x16:
        if ((int8_t) GAME_CALL(st, 0x004F4440u, id) != 0) {
            break;
        }
        if (labor_wanted_anywhere(st, LABOR_OBJECT) != 0) {
            U8(st, fig(st) + 0x38) = 8;
            break;
        }
        f = fig(st);
        U8(st, f + 0x154) += 1;
        if (U8(st, f + 0x154) >= 4) {
            U8(st, f + 0x38) = 8;
            break;
        }
        if ((int8_t) GAME_CALL(st, 0x004F4320u, cur(st)) == 0) {
            U8(st, fig(st) + 0x38) = 8;
        } else {
            U8(st, fig(st) + 0x38) = 0x15;
        }
        break;
    case 0x17:
        state_move_to_monument(st, id, speed, 0, 0);
        break;
    case 0x18:
        state_move_then(st, id, speed, 0x19, 0);
        break;
    case 0x19:
        state_work(st, 0, 0x00541DE0u, 0x00542A80u);
        break;
    case 0x1A:
        state_move_to_monument(st, id, speed, 0, 1);
        break;
    case 0x1B:
        state_move_then(st, id, speed, 0x1C, 0);
        break;
    case 0x1C:
        state_work(st, 0, 0x004E0050u, 0x004E0C00u);
        break;
    case 0x1D:
        S16(st, f + 0x36) += 1;
        if (S16(st, f + 0x36) > 0x2A) {
            U8(st, f + 0x38) = 8;
            S16(st, f + 0x120) = 0;
            S16(st, f + 0x36) = 0;
            U8(st, f + 0x65) = 1;
        }
        break;
    case 0x1F:
        state_move_to_monument(st, id, speed, 0x006FD5E0u, 0);
        break;
    case 0x20:
        state_move_then(st, id, speed, 0x21, 1);
        break;
    case 0x21:
        state_work(st, 0x006FD5D8u, 0x0040F0D0u, 0x0040F070u);
        break;
    case 0x22:
        state_move_to_monument(st, id, speed, 0x006FD370u, 0);
        break;
    case 0x23:
        state_move_to_monument(st, id, speed, 0x006DE550u, 0);
        break;
    case 0x24:
    {
        U8(st, f + 0x72) = 7;
        GAME_CALL(st, 0x004A5DA0u, id, speed);
        f = fig(st);
        /* @bin 0x004A9E51 sprintf(buffer, "Laborer at x,y: %d, %d\n", x, y) into this frame, then
         * OutputDebugStringA(buffer) (0x0056F0F8): debugger output only (0: a buffer outside the
         * state, see crt_sprintf) */
        GAME_CALL(st, 0x00561A36u, 0, 0x005DF6E8, S16(st, f + 0x14), S16(st, f + 0x16));
        f = fig(st);
        int8_t dir = S8(st, f + 0x11);
        if (dir == 8) {
            int16_t site = S16(st, f + 0x5E);
            uint32_t sb = BUILDING_ADDR(site);
            uint32_t o = monument_object(S16(st, sb + 0x10));
            if (o != 0) {
                /* @bin 0x004A9EB1 the stone delivered to the main building (+0x12 back to it) */
                U8(st, o + 0x4C) = 0;
                S32(st, BUILDING_ADDR(site - S16(st, sb + 0x12)) + 0x98) += 0x190;
                GAME_CALLT(st, o, 0x00547B60u, site);
            }
            U8(st, fig(st) + 0x0E) = 2;
        } else if (dir == 9) {
            GAME_CALL(st, 0x004B3250u, cur(st));
            U8(st, fig(st) + 0x0E) = 2;
            monument_flag_clear(st, site_type(st));
        } else if (dir == 0x0A) {
            U8(st, f + 0x0E) = 2;
            monument_flag_clear(st, site_type(st));
        }
        break;
    }
    default:
        U8(st, f + 0x38) = 8;
        break;
    }
    /* @bin 0x004AB240 */
    f = fig(st);
    if (U8(st, f + 0x0E) == 2) {
        U8(st, BUILDING_ADDR(S16(st, f + 0x5A)) + 0xA6) -= 1;
    }
    int32_t d = sprite_direction(st, S8(st, f + 0x11));
    uint32_t base;
    int32_t frame = U8(st, f + 0x01);
    /* @bin 0x004AB2AF jump tables 0x004AB958 / 0x004AB93C */
    switch (U8(st, f + 0x38)) {
    case 4:
        S32(st, f + 0x04) = (int32_t) (dying_frame(st, f) + U32(st, 0x00F56094u));
        return;
    case 7:
        base = U8(st, f + 0x10) == 0x11 ? 0x00F560B4u : 0x00F5608Cu;
        break;
    case 0x11: case 0x1E:
        base = S16(st, f + 0x68) != 0 ? 0x00F560B4u : 0x00F5608Cu;
        break;
    case 0x13: case 0x14: case 0x17: case 0x1A: case 0x1F: case 0x22: case 0x23: case 0x24:
        base = 0x00F560B4u;
        break;
    case 0x16: case 0x19: case 0x1C: case 0x21:
        base = 0x00F5609Cu;
        frame >>= 1;
        break;
    default:
        base = 0x00F5608Cu;
        break;
    }
    S32(st, f + 0x04) = S32(st, base) + 8 * frame + d;
}
