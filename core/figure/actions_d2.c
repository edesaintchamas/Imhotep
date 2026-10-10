/* Imhotep - figure actions, group D: type 0x55 (the laborer that leads a stone sledge to a
 * monument, 0x004A98B0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). The action works on the current figure
 * (0x00D056F8) and re-reads it after each call, as the original does. */
#include "figure/actions_d.h"

#include "figure/actions_d_local.h"
#include "state/layout.h"
#include "state/ported.h"

#define TYPE_PYRAMID 0xB7        /* @bin building types of the monuments the laborer serves */

/* @bin 0x004A99CB, 0x004A9DD5, 0x004A9FF3 (jump tables 0x004AB830, 0x004AB850, 0x004AB8B0): for
 * the monument types 0xE5, 0xEA, 0xEB, 0xEC a byte of the monument's object is cleared */
static uint32_t monument_object(int16_t type)
{
    switch (type) {
    case 0xE5: return G_MONUMENT_E5;
    case 0xEA: return G_MONUMENT_EA;
    case 0xEB: return G_MONUMENT_EB;
    case 0xEC: return G_MONUMENT_EC;
    default: return 0;
    }
}

static void monument_flag_clear(game_state *st, int16_t type)
{
    uint32_t o = monument_object(type);
    if (o != 0) {
        U8(st, o + MONOBJ_U4C) = 0;
    }
}

/* the type of the building the laborer works for (+0x5E) */
static int16_t site_type(game_state *st)
{
    return S16(st, BUILDING_ADDR(S16(st, fig(st) + FIG_DEST_BUILDING)) + BLD_TYPE);
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
        int32_t n = GAME_CALL(st, FN_FIGURE_CREATE, U8(st, tb + BLD_OWNER), 0x60, S16(st, f + FIG_X), S16(st, f + FIG_Y), 0);
        uint32_t nf = FIGURE_ADDR(n);
        U8(st, nf + FIG_ACTION) = 8;
        S16(st, nf + FIG_LEADER) = (int16_t) prev;
        S16(st, nf + FIG_HOME) = 0;
        S16(st, nf + FIG_WAIT_TICKS) = 0;
        S16(st, nf + FIG_ROAM_LENGTH) = 0;
        U8(st, nf + FIG_RESOURCE) = (uint8_t) v;
        v = 1 - v;
        prev = n;
    }
    uint32_t f = fig(st);
    int32_t n = GAME_CALL(st, FN_FIGURE_CREATE, U8(st, tb + BLD_OWNER), 0x56, S16(st, f + FIG_X), S16(st, f + FIG_Y), 0);
    uint32_t nf = FIGURE_ADDR(n);
    U8(st, nf + FIG_ACTION) = 8;
    U8(st, nf + FIG_RESOURCE) = U8(st, fig(st) + FIG_COLLECTING);
    S16(st, nf + FIG_LEADER) = (int16_t) prev;
    S16(st, nf + FIG_HOME) = 0;
    S16(st, nf + FIG_WAIT_TICKS) = 0;
    S16(st, nf + FIG_ROAM_LENGTH) = 0;
}

/* @bin 0x004AA07C / 0x004AA3D6: take 0x190 of the resource +0x7A from the storage +0x5E of the
 * player (0x0045B570; its result is what it could not take: given back with 0x0045B2A0). +0x74:
 * the amount carried (0x190 when all of it was there). */
static void take_stone(game_state *st, int32_t site)
{
    uint32_t f = fig(st);
    S16(st, f + FIG_AMOUNT) = 0;
    int32_t r = GAME_CALL(st, FN_STORAGE_TAKE, S32(st, CURRENT_PLAYER_ADDR), site, U8(st, f + FIG_COLLECTING), 0x190);
    if (r == 0x190) {
        S16(st, fig(st) + FIG_AMOUNT) = 0;
    } else if (r == 0) {
        S16(st, fig(st) + FIG_AMOUNT) = 0x190;
    } else {
        GAME_CALL(st, FN_STORAGE_ADD, S32(st, CURRENT_PLAYER_ADDR), S16(st, fig(st) + FIG_DEST_BUILDING), U8(st, fig(st) + FIG_COLLECTING), 0x190 - r);
        S16(st, fig(st) + FIG_AMOUNT) = 0;
    }
}

/* the same with the site read once (0x004AA3D6 passes ebp to both calls) */
static void take_stone_at(game_state *st, int32_t site)
{
    uint32_t f = fig(st);
    S16(st, f + FIG_AMOUNT) = 0;
    int32_t r = GAME_CALL(st, FN_STORAGE_TAKE, S32(st, CURRENT_PLAYER_ADDR), site, U8(st, f + FIG_COLLECTING), 0x190);
    if (r == 0x190) {
        S16(st, fig(st) + FIG_AMOUNT) = 0;
    } else if (r == 0) {
        S16(st, fig(st) + FIG_AMOUNT) = 0x190;
    } else {
        GAME_CALL(st, FN_STORAGE_ADD, S32(st, CURRENT_PLAYER_ADDR), site, U8(st, fig(st) + FIG_COLLECTING), 0x190 - r);
        S16(st, fig(st) + FIG_AMOUNT) = 0;
    }
}

/* @bin 0x004AA129 nothing taken: back to state 8 */
static void nothing_taken(game_state *st)
{
    uint32_t f = fig(st);
    U8(st, f + FIG_RESOURCE) = 0;
    U8(st, f + FIG_ACTION) = 8;
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
    U8(st, fig(st) + FIG_ANIM_FRAME) = 0;
}

/* @bin 0x004AA2AF: is the building next to the sledge (+0x68's destination) still accepting?
 * (types 0xB7, 0xD5, 0xDE, 0xE1, 0xE3, 0xE4: jump tables 0x004AB90C / 0x004AB8F0) */
static int site_accepts(game_state *st, int16_t b)
{
    int8_t r;
    switch (S16(st, BUILDING_ADDR(b) + BLD_TYPE)) {
    case TYPE_PYRAMID: r = (int8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, b); break;
    case 0xD5: r = (int8_t) GAME_CALL(st, FN_MONUMENT_D5_WORKER_BUSY, b); break;
    case 0xDE: r = (int8_t) GAME_CALL(st, FN_MASTABA_WORKER_BUSY, b); break;
    case 0xE1: r = (int8_t) GAME_CALLT(st, G_MONUMENT_E1, FN_OBELISK_BUSY, b); break;
    case 0xE3: r = (int8_t) GAME_CALLT(st, G_MONUMENT_E3, FN_MONUMENT_0XE3_BUSY, b); break;
    case 0xE4: r = (int8_t) GAME_CALLT(st, G_MONUMENT_E4, FN_MONUMENT_0XE4_BUSY, b); break;
    default: return 1;
    }
    return r != 0;
}

/* state 0x10 (0x004AA062): load the sledge */
static void state_load(game_state *st, int32_t id, uint32_t home)
{
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 3;
    if (U8(st, f + FIG_COLLECTING) == 0x22) {
        /* @bin 0x004AA07C resource 0x22: a sledge without ropes, going to +0x5C */
        take_stone(st, S16(st, f + FIG_DEST_BUILDING));
        if (S16(st, fig(st) + FIG_AMOUNT) == 0) {
            nothing_taken(st);
            return;
        }
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
        id = cur(st);
        f = FIGURE_ADDR(id);
        int16_t dest = S16(st, f + FIG_TARGET_HOUSE);
        uint32_t db = BUILDING_ADDR(dest);
        U8(st, f + FIG_ACTION) = 0x24;
        S16(st, f + FIG_DEST_BUILDING) = dest;
        S16(st, f + FIG_DEST_X) = S16(st, db + BLD_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, db + BLD_ROAD_ACCESS_Y);
        S32(st, f + FIG_U28) = S32(st, db + BLD_GRID_OFFSET);
        S16(st, f + FIG_LEADER) = 0;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        S16(st, f + FIG_ROAM_LENGTH) = 0;
        int32_t n = GAME_CALL(st, FN_FIGURE_CREATE, U8(st, home + BLD_OWNER), 0x56, S16(st, f + FIG_X), S16(st, f + FIG_Y), 0);
        uint32_t nf = FIGURE_ADDR(n);
        id = cur(st);
        U8(st, nf + FIG_ACTION) = 8;
        U8(st, nf + FIG_RESOURCE) = U8(st, FIGURE_ADDR(id) + FIG_COLLECTING);
        S16(st, nf + FIG_LEADER) = (int16_t) id;
        S16(st, nf + FIG_HOME) = 0;
        S16(st, nf + FIG_WAIT_TICKS) = 0;
        S16(st, nf + FIG_ROAM_LENGTH) = 0;
        S16(st, nf + FIG_SOURCE_X) = 0;
        S16(st, nf + FIG_SOURCE_Y) = 0;
        U8(st, FIGURE_ADDR(id) + FIG_ANIM_FRAME) = 0;
        return;
    }
    int16_t prev = S16(st, f + FIG_LEADER);
    int16_t b = 0;
    if (prev != 0 && S16(st, FIGURE_ADDR(prev) + FIG_LEADER) == id) {
        b = S16(st, G_GRID_BUILDING + 2u * U32(st, FIGURE_ADDR(prev) + FIG_U28));
    }
    if (b == 0) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        U8(st, fig(st) + FIG_STATE) = 2;
        return;
    }
    if (!site_accepts(st, b)) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
        U8(st, fig(st) + FIG_STATE) = 2;
        return;
    }
    f = fig(st);
    int16_t site = S16(st, f + FIG_DEST_BUILDING);
    S16(st, f + FIG_WAIT_TICKS) += 1;
    if (S16(st, f + FIG_WAIT_TICKS) <= 4) {
        U8(st, f + FIG_ANIM_FRAME) = 0;
        return;
    }
    take_stone_at(st, site);
    if (S16(st, fig(st) + FIG_AMOUNT) == 0) {
        nothing_taken(st);
        return;
    }
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
    id = cur(st);
    f = FIGURE_ADDR(id);
    /* @bin 0x004AA48E the building at the sledge's tile (+0x68's +0x20) */
    int16_t t = S16(st, G_GRID_BUILDING + 2u * U32(st, FIGURE_ADDR(S16(st, f + FIG_LEADER)) + FIG_GRID_OFFSET));
    uint32_t tb = BUILDING_ADDR(t);
    int16_t type = S16(st, tb + BLD_TYPE);
    if (type == TYPE_PYRAMID) {
        U8(st, f + FIG_ACTION) = 0x1E;
        S16(st, f + FIG_WAIT_TICKS) = 0;
        S16(st, f + FIG_DEST_BUILDING) = t;
        GAME_CALL(st, FN_PYRAMID_SEND_FIGURE, id, t);
        f = fig(st);
        int at_dest = S16(st, f + FIG_X) == S16(st, f + FIG_DEST_X) && S16(st, f + FIG_Y) == S16(st, f + FIG_DEST_Y);
        if (!at_dest) {
            GAME_CALL(st, FN_FIGURE_FIND_ROUTE, cur(st));
            if (S16(st, fig(st) + FIG_PATH_ID) <= 0) {
                GAME_CALL(st, FN_STORAGE_ADD, S32(st, CURRENT_PLAYER_ADDR), site, U8(st, fig(st) + FIG_COLLECTING), 0x190);
                U8(st, fig(st) + FIG_ACTION) = 8;
                return;
            }
        }
        sledge_create(st, tb);
        if (at_dest) {
            U8(st, fig(st) + FIG_ACTION) = 0x12;
            GAME_CALL(st, FN_PYRAMID_BUILDER_PAIR, cur(st));
            GAME_CALL(st, FN_WORKER_START_JOB, cur(st));
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
            U8(st, f + FIG_ACTION) = 8;
        } else {
            U8(st, f + FIG_ACTION) = next;
            S16(st, f + FIG_DEST_X) = S16(st, tb + BLD_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, tb + BLD_ROAD_ACCESS_Y);
            S32(st, f + FIG_U28) = S32(st, tb + BLD_GRID_OFFSET);
            S16(st, f + FIG_ROAM_LENGTH) = 0;
            sledge_create(st, tb);
        }
    }
    f = fig(st);
    S16(st, f + FIG_WAIT_TICKS) = 0;
    U8(st, f + FIG_ANIM_FRAME) = 0;
}

/* the laborer arrived next to its sledge's helper (+0x68 of type 0x51 in state 0x0A): that one
 * goes on (0x0B); the laborer waits (0x1D) */
static void arrived_wait(game_state *st, uint32_t counter)
{
    uint32_t f = fig(st);
    uint32_t p = FIGURE_ADDR(S16(st, f + FIG_LEADER));
    if (U8(st, p + FIG_TYPE) == 0x51 && U8(st, p + FIG_ACTION) == 0x0A) {
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
            U8(st, p + FIG_ACTION) = 0x0B;
            S16(st, p + FIG_WAIT_TICKS) = 0;
        }
    }
    U8(st, f + FIG_ACTION) = 0x1D;
    S16(st, f + FIG_WAIT_TICKS) = 0;
}

/* @bin 0x004AB157 / 0x004AAFF9: not arrived: route again (9) or give up (0x0A) */
static void moving_else(game_state *st, int8_t dir)
{
    if (dir == 9) {
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
    } else if (dir == 0x0A) {
        U8(st, fig(st) + FIG_ACTION) = 8;
    }
}

/* @bin 0x004AB010, 0x004AB04B, 0x004AB0D3, 0x004AB2E1, 0x004AB505: move to the monument */
static void state_move_to_monument(game_state *st, int32_t id, int32_t speed, uint32_t counter, int route_44)
{
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 7;
    GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
    int8_t dir = S8(st, fig(st) + FIG_DIRECTION);
    if (dir == 8) {
        arrived_wait(st, counter);
        return;
    }
    if (route_44 && dir == 9) {
        /* @bin 0x004AB57C */
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
        S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        return;
    }
    moving_else(st, dir);
}

/* @bin 0x004AB16A, 0x004AB5A8, 0x004AB369: move, then state `next` */
static void state_move_then(game_state *st, int32_t id, int32_t speed, uint8_t next, int reroute)
{
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_INVISIBLE) = 0;
    U8(st, f + FIG_TERRAIN_USAGE) = 7;
    GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
    f = fig(st);
    int8_t dir = S8(st, f + FIG_DIRECTION);
    if (dir == 8) {
        U8(st, f + FIG_ACTION) = next;
        S16(st, f + FIG_WAIT_TICKS) = 0;
    } else if (reroute) {
        moving_else(st, dir);
    } else if (dir == 9 || dir == 0x0A) {
        U8(st, f + FIG_ACTION) = 8;
    }
}

/* @bin 0x004AB1C6, 0x004AB3B2, 0x004AB5F1: work at the building on the destination tile; every
 * 4th update while working, an effect at the figure's tile */
static void state_work(game_state *st, uint32_t this_ptr, uint32_t work, uint32_t effect)
{
    uint32_t f = fig(st);
    U8(st, f + FIG_INVISIBLE) = 0;
    int16_t t = S16(st, G_GRID_BUILDING + 2u * U32(st, f + FIG_U28));
    if (t == 0) {
        U8(st, f + FIG_ACTION) = 8;
        return;
    }
    if (this_ptr) {
        GAME_CALLT(st, this_ptr, work, t, cur(st), 8);
    } else {
        GAME_CALL(st, work, t, cur(st), 8);
    }
    f = fig(st);
    if (U8(st, f + FIG_ACTION) != 8) {
        return;
    }
    U8(st, f + FIG_U154) += 1;
    if (U8(st, f + FIG_U154) >= 4) {
        U8(st, f + FIG_U154) = 0;
        return;
    }
    if (this_ptr) {
        GAME_CALLT(st, this_ptr, effect, S16(st, f + FIG_X), S16(st, f + FIG_Y), cur(st));
    } else {
        GAME_CALL(st, effect, S16(st, f + FIG_X), S16(st, f + FIG_Y), cur(st));
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
    int32_t x0 = S16(st, bb + BLD_X), y0 = S16(st, bb + BLD_Y);
    uint32_t kind = U8(st, bb + BLD_MONUMENT_KIND);
    int32_t c1 = b + S32(st, G_U5F6B94 + 4u * kind);
    int32_t x1 = S16(st, BUILDING_ADDR(c1) + BLD_X), y1 = S16(st, BUILDING_ADDR(c1) + BLD_Y);
    int32_t limit = S32(st, G_PYRAMID_KIND) != 5 ? 10 : 5;
    int8_t busy = 0;
    for (int32_t i = b;; i++) {
        int32_t end = b + S32(st, (S32(st, G_PYRAMID_KIND) == 5 ? G_PYRAMID_PARTS_KIND5 : G_PYRAMID_PARTS) + 4u * kind);
        if (i >= end) {
            break;
        }
        uint32_t ib = BUILDING_ADDR(i);
        if (check && S16(st, ib + BLD_U64) > 0 && (int8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, i) != 0) {
            busy = 1;
            continue;
        }
        int32_t level = U8(st, ib + BLD_U55);
        if (level == 3) {
            level = 4;
        }
        int32_t mask = U8(st, ib + BLD_U5B);
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
            int32_t sx = S16(st, ib + BLD_X) + S32(st, G_U5F6BBC + 4u * (uint32_t) c);
            int32_t sy = S16(st, ib + BLD_Y) + S32(st, G_U5F6BCC + 4u * (uint32_t) c);
            int32_t d;
            switch (S32(st, G_U5F6B68 + 4u * (uint32_t) level)) {
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
    if (busy || S32(st, G_PYRAMID_KIND) == 5 || U8(st, bb + BLD_U55) != 0x0A || U8(st, bb + BLD_U5B) != 0x0F) {
        return 0;
    }
    if (check) {
        for (int32_t j = 0; j < 4; j++) {
            if (S16(st, BUILDING_ADDR(b + j) + BLD_U64) > 0 && (int8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, b + j) != 0) {
                return 0;
            }
        }
    }
    *px = S16(st, bb + BLD_X) + 1;
    *py = S16(st, bb + BLD_Y) + 1;
    return 1;
}

/* ---- the laborers' work assignment (the object at 0x00D05720, thiscall methods) ---- */


static int8_t building_test(game_state *st, uint32_t f, int32_t b)
{
    return (int8_t) GAME_CALLT(st, G_BUILDING_MANAGER, f, b);
}

/* |a - b| as the original computes it (also stored in the globals) */
static int32_t tile_distance(game_state *st, uint32_t f, uint32_t b)
{
    int32_t fx = S16(st, f + FIG_X), bx = S16(st, b + BLD_X);
    int32_t dx = fx > bx ? fx - bx : (fx < bx ? bx - fx : 0);
    S32(st, G_DIST_X) = dx;
    int32_t fy = S16(st, f + FIG_Y), by = S16(st, b + BLD_Y);
    int32_t dy = fy > by ? fy - by : (fy < by ? by - fy : 0);
    S32(st, G_DIST_Y) = dy;
    return dx >= dy ? dx : dy;
}

/* a building that needs laborers: 0x00488030 and 0x004894A0 true, no laborer assigned (+0xB4)
 * and 0x00459240 0 */
static int labor_wanted(game_state *st, int32_t i)
{
    uint32_t b = BUILDING_ADDR(i);
    return U8(st, b + BLD_STATE) == 1 && building_test(st, FN_BUILDING_IS_TYPE_0X487F80, i) != 0 && building_test(st, FN_BUILDING_AREA_TEST_3X3, i) != 0
           && S16(st, b + BLD_SECOND_WALKER) == 0 && GAME_CALL(st, FN_BUILDING_RESOURCE_IN_STOCK, i) == 0;
}

/* the monument work tests: (building, figure, commit) -> a byte */
static int8_t monument_work(game_state *st, int16_t type, int32_t b, int32_t fid, int32_t commit)
{
    switch (type) {
    case 0xD5: return (int8_t) GAME_CALL(st, FN_SUN_TEMPLE_TAKE, b, fid, commit);
    case 0xE3: return (int8_t) GAME_CALLT(st, G_MONUMENT_E3, FN_MONUMENT_0XE3_TAKE, b, fid, commit);
    case 0xE4: return (int8_t) GAME_CALLT(st, G_MONUMENT_E4, FN_MONUMENT_0XE4_TAKE, b, fid, commit);
    case 0xE1: return (int8_t) GAME_CALLT(st, G_MONUMENT_E1, FN_OBELISK_TAKE, b, fid, commit);
    case 0xDE: return (int8_t) GAME_CALL(st, FN_MASTABA_TAKE, b, fid, commit);
    case 0xE5: return (int8_t) GAME_CALLT(st, G_MONUMENT_E5, FN_TOMB_METHOD_2C, b, fid, commit);
    case 0xEA: return (int8_t) GAME_CALLT(st, G_MONUMENT_EA, FN_TOMB_METHOD_2C, b, fid, commit);
    case 0xEB: return (int8_t) GAME_CALLT(st, G_MONUMENT_EB, FN_TOMB_METHOD_2C, b, fid, commit);
    case 0xEC: return (int8_t) GAME_CALLT(st, G_MONUMENT_EC, FN_TOMB_METHOD_2C, b, fid, commit);
    default: return 0;
    }
}

static void work_add(game_state *st, int32_t *values, int32_t b, int32_t v)
{
    int32_t n = S32(st, G_WORK_COUNT);
    if (n < 500) {
        S16(st, G_WORK_LIST + 2u * (uint32_t) n) = (int16_t) b;
        values[n] = v;
        S32(st, G_WORK_COUNT) = n + 1;
    }
}

void labor_assign(game_state *st, uint32_t self, int32_t fid)
{
    /* @bin 0x004BEEB0 (thiscall) find work for the laborer fid: a building that wants laborers
     * nearby (mode 0 of +0x32FB4), else a monument; the nearest one it can reach */
    uint32_t f = FIGURE_ADDR(fid);
    int32_t values[500];
    for (;;) {
        S32(st, G_WORK_COUNT) = 0;
        int found_unreachable = 0;
        int32_t best_value = 0;
        int32_t mode = S32(st, self + 0x32FB4u);
        if (mode != 1 && mode != 2 && mode != 3 && S32(st, G_BUILDING_HIGHEST) >= 1) {
            for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
                uint32_t b = BUILDING_ADDR(i);
                if (!labor_wanted(st, i) || S16(st, b + BLD_DISTANCE) == 0
                    || GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE)) == 0) {
                    continue;
                }
                /* @bin 0x004BEFF0 within 40 tiles, 0x0078D79C <= 0, +0xA7 below 0x30 */
                if (tile_distance(st, f, b) > 0x28 || S32(st, G_LOCUST_TIMER) > 0 || U8(st, b + BLD_UA7) >= 0x30) {
                    continue;
                }
                int32_t n = S32(st, G_WORK_COUNT);
                S16(st, G_WORK_LIST + 2u * (uint32_t) n) = (int16_t) i;
                n++;
                S32(st, G_WORK_COUNT) = n >= 500 ? 499 : n;
            }
        }
        if (S32(st, G_WORK_COUNT) == 0) {
            if (S32(st, G_BUILDING_HIGHEST) < 1) {
                U8(st, f + FIG_STATE) = 2;
                break;
            }
            for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
                uint32_t b = BUILDING_ADDR(i);
                if (U8(st, b) != 1) {
                    continue;
                }
                int16_t type = S16(st, b + BLD_TYPE);
                if (type == TYPE_PYRAMID) {
                    if (U8(st, b + BLD_U55) > 1) {
                        int32_t x, y;
                        if (pyramid_block_slot(st, i, &x, &y, 1) != 0) {
                            work_add(st, values, i, 0);
                        }
                    } else {
                        int32_t r = GAME_CALL(st, FN_PYRAMID_WAITING_WORKER, i, 0);
                        if (r != 0) {
                            work_add(st, values, i, r);
                        }
                    }
                    /* @bin 0x004BF118 skip the pyramid's other parts */
                    uint32_t t = S32(st, G_PYRAMID_KIND) == 5 ? G_PYRAMID_PARTS_KIND5 : G_PYRAMID_PARTS;
                    i += S32(st, t + 4u * U8(st, b + BLD_MONUMENT_KIND)) - 1;
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
        if (S32(st, G_WORK_COUNT) <= 0) {
            U8(st, f + FIG_STATE) = 2;
            break;
        }
        /* @bin 0x004BF3E6 the nearest candidate (+0xA7 added for the buildings of 0x00488030) */
        int32_t best = 0, best_d = 10000;
        S32(st, G_WORK_INDEX) = 0;
        for (int32_t k = 0; k < S32(st, G_WORK_COUNT);) {
            int32_t b = S16(st, G_WORK_LIST + 2u * (uint32_t) k);
            int32_t v = values[k];
            k++;
            S32(st, G_WORK_INDEX) = k;
            uint32_t bb = BUILDING_ADDR(b);
            int32_t d = tile_distance(st, f, bb);
            if (building_test(st, FN_BUILDING_IS_TYPE_0X487F80, b) != 0) {
                d += U8(st, bb + BLD_UA7);
            }
            if (d < best_d) {
                if (building_test(st, FN_BUILDING_IS_TYPE_0X487F80, b) != 0
                    && GAME_CALL(st, FN_ROUTE_WALK_TO, S16(st, f + FIG_X), S16(st, f + FIG_Y), S16(st, bb + BLD_ROAD_ACCESS_X),
                                 S16(st, bb + BLD_ROAD_ACCESS_Y)) == 0) {
                    S16(st, bb + BLD_SECOND_WALKER) = -1; /* unreachable: excluded, and the search runs again */
                    found_unreachable = 1;
                } else {
                    best_d = d;
                    best = b;
                    best_value = v;
                }
            }
            k = S32(st, G_WORK_INDEX);
        }
        if (best == 0) {
            if (found_unreachable) {
                continue;
            }
            U8(st, f + FIG_STATE) = 2;
            break;
        }
        uint32_t bb = BUILDING_ADDR(best);
        if (building_test(st, FN_BUILDING_IS_TYPE_0X487F80, best) != 0) {
            if (road_near(st, bb) == 0) {
                U8(st, f + FIG_STATE) = 2;
                break;
            }
            S16(st, bb + BLD_SECOND_WALKER) = (int16_t) fid;
            S16(st, f + FIG_DEST_BUILDING) = (int16_t) best;
            U8(st, f + FIG_ACTION) = 0x0A;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            /* @bin 0x004BF5E8 the current figure's +0x26, as in the original */
            S16(st, fig(st) + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
            S16(st, f + FIG_ROAM_LENGTH) = 0;
            break;
        }
        int16_t type = S16(st, bb + BLD_TYPE);
        if (type == TYPE_PYRAMID) {
            if (U8(st, bb + BLD_U55) > 1) {
                S16(st, f + FIG_DEST_BUILDING) = (int16_t) best;
                S16(st, f + FIG_LEADER) = 0;
                U8(st, f + FIG_ACTION) = 0x11;
                S16(st, f + FIG_WAIT_TICKS) = 0;
                GAME_CALL(st, FN_PYRAMID_SEND_FIGURE, fid, best);
                break;
            }
            /* @bin 0x004BF662 the stone for the next level, by the difficulty-like 0x005E4EF4 */
            int8_t r = (int8_t) GAME_CALL(st, FN_PYRAMID_PART_UNFINISHED,
                                          S16(st, G_GRID_BUILDING + 2u * U32(st, FIGURE_ADDR(best_value) + FIG_GRID_OFFSET)));
            int32_t g = S32(st, G_PYRAMID_KIND);
            uint8_t resource;
            if (r != 0) {
                resource = (g == 2 || g == 5) ? 0x0C : 0x18; /* @bin jump table 0x004BF900 */
            } else {
                resource = g == 3 ? 0x18 : (g == 5 ? 0x0C : 0x19);
            }
            int32_t n = GAME_CALL(st, FN_BUILDING_FIND_SUPPLIER, best, resource, 1);
            if (n == 0) {
                U8(st, f + FIG_STATE) = 2;
                break;
            }
            U8(st, f + FIG_COLLECTING) = resource;
            S16(st, f + FIG_AMOUNT) = 0;
            S16(st, f + FIG_DEST_BUILDING) = (int16_t) n;
            S16(st, f + FIG_LEADER) = (int16_t) best_value;
            U8(st, f + FIG_ACTION) = 0x0F;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
            S16(st, f + FIG_ROAM_LENGTH) = 0;
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, fid);
            S16(st, FIGURE_ADDR(best_value) + FIG_LEADER) = (int16_t) fid;
            break;
        }
        if (type == 0xD5 || type == 0xE3 || type == 0xE4 || type == 0xE1 || type == 0xDE || type == 0xE5
            || type == 0xEA || type == 0xEB || type == 0xEC) {
            if (monument_work(st, type, best, fid, 1) == 0) {
                U8(st, f + FIG_STATE) = 2;
            }
        }
        break;
    }
    /* @bin 0x004BF8AB the exclusions (+0xB4 = -1) are lifted */
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b) == 1 && building_test(st, FN_BUILDING_IS_TYPE_0X487F80, i) != 0 && S16(st, b + BLD_SECOND_WALKER) < 0) {
            S16(st, b + BLD_SECOND_WALKER) = 0;
        }
    }
}

void labor_arrived(game_state *st, int32_t fid)
{
    /* @bin 0x004BF920 (thiscall, `this` unused) the laborer reached the building it works for */
    uint32_t f = FIGURE_ADDR(fid);
    int16_t target = S16(st, f + FIG_DEST_BUILDING);
    int16_t home = S16(st, f + FIG_HOME);
    if (building_test(st, FN_BUILDING_IS_TYPE_0X487F80, target) == 0) {
        U8(st, BUILDING_ADDR(home) + BLD_UA8) = 0x20;
        return;
    }
    U8(st, f + FIG_STATE) = 2;
    if (S32(st, G_LOCUST_TIMER) != 0) {
        return;
    }
    U8(st, BUILDING_ADDR(home) + BLD_UA7) = 0x20;
    uint32_t t = BUILDING_ADDR(target);
    U8(st, t + BLD_UA7) = 0x60;
    U8(st, t + BLD_SHOWS) += 1;
    S16(st, t + BLD_SECOND_WALKER) = 0;
    /* @bin 0x004BF9A3 per building type, 40 bytes each at 0x00705EB8 */
    S16(st, t + BLD_WORKERS) = S16(st, G_TYPE_WORKERS + 40u * (uint32_t) S16(st, t + BLD_TYPE));
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
    int16_t p = S16(st, f + FIG_LEADER);
    if (p == 0) {
        U8(st, f + FIG_ACTION) = 8;
        return 0;
    }
    int16_t b = S16(st, G_GRID_BUILDING + 2u * U32(st, FIGURE_ADDR(p) + FIG_U28));
    uint32_t bb = BUILDING_ADDR(b);
    int8_t r;
    switch (S16(st, bb + BLD_TYPE)) {
    case TYPE_PYRAMID: r = (int8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, b); break;
    case 0xD5: r = (int8_t) GAME_CALL(st, FN_MONUMENT_D5_WORKER_BUSY, b); break;
    case 0xDE:
        if ((int8_t) GAME_CALL(st, FN_MASTABA_WORKER_BUSY, b) != 0) {
            return 1;
        }
        U8(st, f + FIG_ACTION) = 8;
        return 0;
    case 0xE1: r = (int8_t) GAME_CALLT(st, G_MONUMENT_E1, FN_OBELISK_BUSY, b); break;
    case 0xE3: r = (int8_t) GAME_CALLT(st, G_MONUMENT_E3, FN_MONUMENT_0XE3_BUSY, b); break;
    case 0xE4: r = (int8_t) GAME_CALLT(st, G_MONUMENT_E4, FN_MONUMENT_0XE4_BUSY, b); break;
    default:
        U8(st, f + FIG_ACTION) = 8;
        return 0;
    }
    if (r != 0) {
        return 1;
    }
    int32_t n = GAME_CALL(st, FN_PYRAMID_WAITING_WORKER, b, 1);
    if (n != 0) {
        S16(st, f + FIG_LEADER) = (int16_t) n;
        S16(st, FIGURE_ADDR(n) + FIG_LEADER) = (int16_t) id;
        return 1;
    }
    if (S16(st, bb + BLD_UC8) == 0) {
        U8(st, f + FIG_ACTION) = 8;
    }
    return 0;
}

void figure_action_55(game_state *st)
{
    /* @bin 0x004A98B0 */
    int32_t id = cur(st);
    uint32_t f = FIGURE_ADDR(id);
    U8(st, f + FIG_TERRAIN_USAGE) = 1;
    U8(st, f + FIG_U0C) = 0;
    S16(st, f + FIG_MAX_ROAM_LENGTH) = 0x200;
    int32_t speed = S32(st, G_PROPS + FIGTYPE_SPEED + 0x38u * U8(st, f + FIG_TYPE));
    next_frame(st, f);
    U8(st, f + FIG_U6B) += 1;
    if (U8(st, f + FIG_U6B) >= 0x78) {
        U8(st, f + FIG_U6B) = 0;
    }
    uint32_t home = BUILDING_ADDR(S16(st, f + FIG_HOME));
    if (U8(st, home + BLD_STATE) != 1) {
        U8(st, f + FIG_STATE) = 2;
        return;
    }
    /* @bin 0x004A996B jump table 0x004AB7AC for states 4 .. 0x24 */
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        monument_flag_clear(st, site_type(st));
        GAME_CALL(st, FN_FIGURE_DYING, id);
        break;
    case 5:
        GAME_CALL(st, FN_FIGURE_FIGHT, id);
        f = fig(st);
        U8(st, f + FIG_ANIM_FRAME) += 1;
        if (U8(st, f + FIG_ANIM_FRAME) >= 0x20) {
            U8(st, f + FIG_ANIM_FRAME) = 0;
        }
        break;
    case 6:
        S32(st, f + FIG_U04) = 0;
        break;
    case 7:
        U8(st, f + FIG_ANIM_FRAME) = 0;
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x64) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            if ((int8_t) GAME_CALL(st, FN_FIGURE_MAY_LEAVE, id) == 0) {
                U8(st, fig(st) + FIG_ACTION) = 8;
            }
        }
        break;
    case 8:
        U8(st, f + FIG_INVISIBLE) = 1;
        U8(st, f + FIG_U154) = 0;
        U8(st, f + FIG_ANIM_FRAME) = 0;
        U8(st, f + FIG_U6B) = 0;
        S16(st, f + FIG_LEADER) = 0;
        S16(st, f + FIG_WAIT_TICKS) -= 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0) {
            break;
        }
        if (road_near(st, home) == 0) {
            U8(st, fig(st) + FIG_ACTION) = 4;
            break;
        }
        if (S32(st, G_PYRAMID_CURSE_COUNTDOWN) >= 1) { /* @bin 0x004A9A97 */
            break;
        }
        id = cur(st);
        f = FIGURE_ADDR(id);
        U8(st, f + FIG_ACTION) = 9;
        S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        move_to_dest(st, id, f);
        S16(st, fig(st) + FIG_ROAM_LENGTH) = 0;
        break;
    case 9:
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 1;
        if (GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 1) == 1) {
            labor_assign(st, G_FLOODPLAIN_MANAGER, cur(st));
        }
        U8(st, fig(st) + FIG_INVISIBLE) = 1;
        break;
    case 0x0A: {
        if (U8(st, BUILDING_ADDR(S16(st, f + FIG_DEST_BUILDING)) + BLD_STATE) != 1) {
            labor_assign(st, G_FLOODPLAIN_MANAGER, id);
            break;
        }
        U8(st, f + FIG_TERRAIN_USAGE) = 7;
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= 0xC80) {
            U8(st, f + FIG_STATE) = 2;
        }
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        id = cur(st);
        f = FIGURE_ADDR(id);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            labor_arrived(st, id);
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        } else if (dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    }
    case 0x0C:
        U8(st, f + FIG_INVISIBLE) = 0;
        S16(st, f + FIG_ROAM_LENGTH) += 1;
        if (S16(st, f + FIG_ROAM_LENGTH) >= S16(st, f + FIG_MAX_ROAM_LENGTH)) {
            if (road_near(st, home) == 0) {
                id = cur(st);
                U8(st, FIGURE_ADDR(id) + FIG_STATE) = 2;
                GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
                break;
            }
            id = cur(st);
            f = FIGURE_ADDR(id);
            U8(st, f + FIG_ACTION) = 0x0D;
            S16(st, f + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
            S16(st, f + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
        }
        GAME_CALL(st, FN_FIGURE_ROAM, id, 1);
        break;
    case 0x0D: {
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8 || dir == 9 || dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        }
        break;
    }
    case 0x0F: {
        U8(st, f + FIG_INVISIBLE) = 0;
        U8(st, f + FIG_U0C) = 0;
        U8(st, f + FIG_TERRAIN_USAGE) = 3;
        int16_t prev = S16(st, f + FIG_LEADER);
        int ok = 0;
        if (prev != 0) {
            uint32_t p = FIGURE_ADDR(prev);
            ok = S16(st, p + FIG_LEADER) == id && S16(st, G_GRID_BUILDING + 2u * U32(st, p + FIG_U28)) != 0;
        }
        if (!ok) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
            U8(st, fig(st) + FIG_STATE) = 2;
            break;
        }
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            U8(st, f + FIG_ACTION) = 0x10;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
            U8(st, fig(st) + FIG_STATE) = 2;
            monument_flag_clear(st, site_type(st));
        } else if (dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
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
        if (S16(st, f + FIG_PATH_ID) <= 0) {
            S16(st, f + FIG_WAIT_TICKS) += 1;
            int16_t v = S16(st, f + FIG_WAIT_TICKS);
            if (v % 30 == 0) { /* @bin 0x004AAC15 */
                if (v >= 0x78) {
                    U8(st, f + FIG_ACTION) = 8;
                } else {
                    GAME_CALL(st, FN_FIGURE_FIND_ROUTE, cur(st));
                }
            }
        }
        if ((int8_t) GAME_CALL(st, FN_PYRAMID_MASON_FREE, cur(st)) == 0) {
            break;
        }
        id = cur(st);
        U8(st, FIGURE_ADDR(id) + FIG_ACTION) = 0x11;
    }
    /* fall through - state 0x11 (0x004AAC66) */
    case 0x11: {
        f = FIGURE_ADDR(id);
        U8(st, f + FIG_TERRAIN_USAGE) = 7;
        U8(st, f + FIG_INVISIBLE) = 0;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x1E) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            int16_t t = S16(st, G_GRID_BUILDING + 2u * U32(st, f + FIG_U28));
            if (t != 0 && S16(st, BUILDING_ADDR(t) + BLD_TYPE) == TYPE_PYRAMID && U8(st, BUILDING_ADDR(t) + BLD_U55) > 1) {
                /* @bin 0x004AACF1 the slot itself is not used here */
                int32_t x, y;
                if (pyramid_block_slot(st, t - S16(st, BUILDING_ADDR(t) + BLD_PART), &x, &y, 1) == 0) {
                    U8(st, fig(st) + FIG_ACTION) = 8;
                }
            }
        }
        f = fig(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            int16_t t = S16(st, G_GRID_BUILDING + 2u * U32(st, f + FIG_U28));
            if (t == 0 || S16(st, BUILDING_ADDR(t) + BLD_TYPE) != TYPE_PYRAMID) {
                U8(st, f + FIG_ACTION) = 8;
            } else if (U8(st, BUILDING_ADDR(t) + BLD_U55) > 1) {
                if ((int8_t) GAME_CALL(st, FN_FIGURE_PYRAMID_BLOCK, cur(st)) == 0) {
                    U8(st, fig(st) + FIG_ACTION) = 8;
                } else {
                    U8(st, fig(st) + FIG_ACTION) = 0x15;
                }
            } else {
                U8(st, f + FIG_ACTION) = 0x12;
                GAME_CALL(st, FN_PYRAMID_BUILDER_PAIR, cur(st));
                GAME_CALL(st, FN_WORKER_START_JOB, cur(st));
            }
        } else if (dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
        }
        break;
    }
    case 0x12: {
        U8(st, f + FIG_INVISIBLE) = 0;
        int16_t p = S16(st, f + FIG_LEADER);
        int ready = 0;
        if (p != 0) {
            int16_t t = S16(st, G_GRID_BUILDING + 2u * U32(st, FIGURE_ADDR(p) + FIG_U28));
            if (t != 0 && (int8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, t) != 0) {
                ready = 1;
            }
        }
        if (!ready) {
            int16_t m = S16(st, fig(st) + FIG_CC_DEST_X);
            int32_t r = GAME_CALL(st, FN_PYRAMID_WAITING_WORKER, m, 1);
            if (r != 0) {
                int32_t c = cur(st);
                S16(st, FIGURE_ADDR(c) + FIG_LEADER) = (int16_t) r;
                S16(st, FIGURE_ADDR(r) + FIG_LEADER) = (int16_t) c;
            } else {
                if (S16(st, BUILDING_ADDR(m) + BLD_UC8) != 0) {
                    break;
                }
                f = fig(st);
                U8(st, f + FIG_ACTION) = 8;
                S16(st, f + FIG_FOLLOW) = 0;
            }
        }
        GAME_CALL(st, FN_FIGURE_PYRAMID_WORK, cur(st), 2, 8, 0x13);
        f = fig(st);
        if (U8(st, f + FIG_ACTION) == 8) {
            S16(st, f + FIG_FOLLOW) = 0;
        }
        break;
    }
    case 0x13: {
        U8(st, f + FIG_INVISIBLE) = 0;
        int16_t p = S16(st, f + FIG_LEADER);
        int16_t t = S16(st, G_GRID_BUILDING + 2u * U32(st, FIGURE_ADDR(p) + FIG_GRID_OFFSET));
        if (S16(st, BUILDING_ADDR(t) + BLD_TYPE) == TYPE_PYRAMID) {
            GAME_CALL(st, FN_FIGURE_PYRAMID_TARGET, id, t, S16(st, f + FIG_CC_DEST_Y), 0);
            U8(st, fig(st) + FIG_ACTION) = 0x14;
        } else {
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_FOLLOW) = 0;
        }
        break;
    }
    case 0x14: {
        U8(st, f + FIG_INVISIBLE) = 0;
        if ((int8_t) GAME_CALL(st, FN_FIGURE_PYRAMID_STEP, id, 1) != 0) {
            int16_t p = S16(st, fig(st) + FIG_LEADER);
            uint32_t pf = FIGURE_ADDR(p);
            int notify = 0;
            if (U8(st, pf + FIG_TYPE) == 0x51 && U8(st, pf + FIG_ACTION) == 0x0A) {
                U8(st, pf + FIG_ACTION) = 0x0B;
                notify = 1;
            } else if (U8(st, pf + FIG_TYPE) == 0x50 && U8(st, pf + FIG_ACTION) == 0x0B) {
                U8(st, pf + FIG_ACTION) = 0x0C;
                notify = 1;
            }
            if (notify) {
                S16(st, pf + FIG_WAIT_TICKS) = 0;
                GAME_CALL(st, FN_WORKER_RELEASE_4F2960, p);
            }
            f = fig(st);
            U8(st, f + FIG_ACTION) = 0x1D;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            break;
        }
        moving_else(st, S8(st, fig(st) + FIG_DIRECTION));
        break;
    }
    case 0x15:
        U8(st, f + FIG_U0C) = 1;
        U8(st, f + FIG_INVISIBLE) = 0;
        if (GAME_CALL(st, FN_FIGURE_STEP_STRAIGHT, id, 1) == 1) {
            f = fig(st);
            U8(st, f + FIG_ACTION) = 0x16;
            S16(st, f + FIG_WAIT_TICKS) = 0;
        }
        break;
    case 0x16:
        if ((int8_t) GAME_CALL(st, FN_FIGURE_PYRAMID_LAY, id) != 0) {
            break;
        }
        if (labor_wanted_anywhere(st, G_FLOODPLAIN_MANAGER) != 0) {
            U8(st, fig(st) + FIG_ACTION) = 8;
            break;
        }
        f = fig(st);
        U8(st, f + FIG_U154) += 1;
        if (U8(st, f + FIG_U154) >= 4) {
            U8(st, f + FIG_ACTION) = 8;
            break;
        }
        if ((int8_t) GAME_CALL(st, FN_FIGURE_PYRAMID_BLOCK, cur(st)) == 0) {
            U8(st, fig(st) + FIG_ACTION) = 8;
        } else {
            U8(st, fig(st) + FIG_ACTION) = 0x15;
        }
        break;
    case 0x17:
        state_move_to_monument(st, id, speed, 0, 0);
        break;
    case 0x18:
        state_move_then(st, id, speed, 0x19, 0);
        break;
    case 0x19:
        state_work(st, 0, FN_SUN_TEMPLE_WORK, FN_MONUMENT_TAKE_0XD5);
        break;
    case 0x1A:
        state_move_to_monument(st, id, speed, 0, 1);
        break;
    case 0x1B:
        state_move_then(st, id, speed, 0x1C, 0);
        break;
    case 0x1C:
        state_work(st, 0, FN_MASTABA_WORK, FN_MONUMENT_TAKE_0XDE);
        break;
    case 0x1D:
        S16(st, f + FIG_WAIT_TICKS) += 1;
        if (S16(st, f + FIG_WAIT_TICKS) > 0x2A) {
            U8(st, f + FIG_ACTION) = 8;
            S16(st, f + FIG_FOLLOW) = 0;
            S16(st, f + FIG_WAIT_TICKS) = 0;
            U8(st, f + FIG_INVISIBLE) = 1;
        }
        break;
    case 0x1F:
        state_move_to_monument(st, id, speed, G_MONUMENT_E1 + MONOBJ_ROUND_ROBIN, 0);
        break;
    case 0x20:
        state_move_then(st, id, speed, 0x21, 1);
        break;
    case 0x21:
        state_work(st, G_MONUMENT_E1, FN_OBELISK_BUILD, FN_MONUMENT_TAKE_0XE1);
        break;
    case 0x22:
        state_move_to_monument(st, id, speed, G_MONUMENT_E3 + MONOBJ_ROUND_ROBIN, 0);
        break;
    case 0x23:
        state_move_to_monument(st, id, speed, G_MONUMENT_E4 + MONOBJ_ROUND_ROBIN, 0);
        break;
    case 0x24:
    {
        U8(st, f + FIG_TERRAIN_USAGE) = 7;
        GAME_CALL(st, FN_FIGURE_MOVE, id, speed);
        f = fig(st);
        /* @bin 0x004A9E51 sprintf(buffer, "Laborer at x,y: %d, %d\n", x, y) into this frame, then
         * OutputDebugStringA(buffer) (0x0056F0F8): debugger output only (0: a buffer outside the
         * state, see crt_sprintf) */
        GAME_CALL(st, FN_CRT_SPRINTF, 0, G_STR_DEBUG_5DF6E8, S16(st, f + FIG_X), S16(st, f + FIG_Y));
        f = fig(st);
        int8_t dir = S8(st, f + FIG_DIRECTION);
        if (dir == 8) {
            int16_t site = S16(st, f + FIG_DEST_BUILDING);
            uint32_t sb = BUILDING_ADDR(site);
            uint32_t o = monument_object(S16(st, sb + BLD_TYPE));
            if (o != 0) {
                /* @bin 0x004A9EB1 the stone delivered to the main building (+0x12 back to it) */
                U8(st, o + MONOBJ_U4C) = 0;
                S32(st, BUILDING_ADDR(site - S16(st, sb + BLD_PART)) + BLD_U98) += 0x190;
                GAME_CALLT(st, o, FN_TOMB_PARTS_REDRAW_547B60, site);
            }
            U8(st, fig(st) + FIG_STATE) = 2;
        } else if (dir == 9) {
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, cur(st));
            U8(st, fig(st) + FIG_STATE) = 2;
            monument_flag_clear(st, site_type(st));
        } else if (dir == 0x0A) {
            U8(st, f + FIG_STATE) = 2;
            monument_flag_clear(st, site_type(st));
        }
        break;
    }
    default:
        U8(st, f + FIG_ACTION) = 8;
        break;
    }
    /* @bin 0x004AB240 */
    f = fig(st);
    if (U8(st, f + FIG_STATE) == 2) {
        U8(st, BUILDING_ADDR(S16(st, f + FIG_HOME)) + BLD_SHOWS) -= 1;
    }
    int32_t d = sprite_direction(st, S8(st, f + FIG_DIRECTION));
    uint32_t base;
    int32_t frame = U8(st, f + FIG_ANIM_FRAME);
    /* @bin 0x004AB2AF jump tables 0x004AB958 / 0x004AB93C */
    switch (U8(st, f + FIG_ACTION)) {
    case 4:
        S32(st, f + FIG_U04) = (int32_t) (dying_frame(st, f) + U32(st, G_IMAGE_GROUP_156));
        return;
    case 7:
        base = U8(st, f + FIG_ACTION_BEFORE_ATTACK) == 0x11 ? G_IMAGE_GROUP_160 : G_IMAGE_GROUP_155;
        break;
    case 0x11: case 0x1E:
        base = S16(st, f + FIG_LEADER) != 0 ? G_IMAGE_GROUP_160 : G_IMAGE_GROUP_155;
        break;
    case 0x13: case 0x14: case 0x17: case 0x1A: case 0x1F: case 0x22: case 0x23: case 0x24:
        base = G_IMAGE_GROUP_160;
        break;
    case 0x16: case 0x19: case 0x1C: case 0x21:
        base = G_IMAGE_GROUP_157;
        frame >>= 1;
        break;
    default:
        base = G_IMAGE_GROUP_155;
        break;
    }
    S32(st, f + FIG_U04) = S32(st, base) + 8 * frame + d;
}
