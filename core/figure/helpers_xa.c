/* Imhotep - figure helpers under the figure actions (package figx, part a).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21); building record: chunk 32. */
#include "figure/helpers_xa.h"

#include <string.h>

#include "building/market.h"
#include "city/events.h"
#include "city/month_events.h"
#include "city/month_message.h"
#include "figure/formation.h"
#include "figure/actions_a.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define MAP_START 0x00BCA2D8u
#define CURRENT_FIGURE 0x00D056F8u
#define DIST_X 0x00D3B4FCu /* i32: the last |dx| */
#define DIST_Y 0x00D3B4F8u /* i32: the last |dy| */
#define BEST_DIST 0x00BCD5ECu

#define COUNTS 0x0096E644u        /* 27 i32: candidates per good index */
#define CAND_TILE 0x0089B744u     /* 27 x 4000 i32: each candidate's road tile */
#define CAND_BUILDING 0x00904EC4u /* 27 x 4000 i32: each candidate's building */
#define TARGETS 0x0089B6D8u       /* 27 i32: the chosen source per good index */
#define CAND_ROW 4000
#define NGOODS 27
#define RECORDS 0x007900BCu /* records of 0xC4 bytes: +0 i16 building, +4 u8 in use */
#define RECORD_SIZE 0xC4u
#define RECORDS_HIGHEST 0x0078D788u
#define RECORD_KIND 0x007900C2u /* u8 per good of the record selected by building +0xC1 */

/* |a - b| as the original computes it (sub after a signed compare) */
static int32_t absdiff(int32_t a, int32_t b)
{
    if (a > b) {
        return (int32_t) ((uint32_t) a - (uint32_t) b);
    }
    if (a < b) {
        return (int32_t) ((uint32_t) b - (uint32_t) a);
    }
    return 0;
}

static void put16(uint8_t *b, uint32_t off, int32_t v)
{
    b[off] = (uint8_t) v;
    b[off + 1] = (uint8_t) ((uint32_t) v >> 8);
}

/* a good the city may not trade (0x00EA42F8, i32 per good of the current city) */
static int good_blocked(game_state *st, int32_t good)
{
    return S32(st, 0x00EA42F8u + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) good)) != 0;
}

static int16_t stock(game_state *st, int32_t b, int32_t idx)
{
    return S16(st, BUILDING_ADDR(b) + 0x56u + 2u * (uint32_t) idx);
}

/* @bin 0x004569CA / 0x00456ACB / 0x00456D15 candidate building `sid` for good index `idx` */
static void add_candidate(game_state *st, int32_t idx, int32_t sid)
{
    uint32_t s = BUILDING_ADDR(sid);
    int32_t n = S32(st, COUNTS + 4u * (uint32_t) idx);
    uint32_t slot = (uint32_t) (idx * CAND_ROW + n);
    S32(st, CAND_BUILDING + 4u * slot) = sid;
    S32(st, CAND_TILE + 4u * slot) = (int32_t) (U32(st, MAP_START) + (uint32_t) (int32_t) S16(st, s + 0x26) +
                                                (uint32_t) ((int32_t) S16(st, s + 0x28) * GRID_SIZE));
    S32(st, COUNTS + 4u * (uint32_t) idx) = n + 1;
}

/* @bin 0x00456861 / 0x00456BDF the larger of the distances between the buildings (kept in
 * 0x00D3B4FC, 0x00D3B4F8) is below 40 */
static int near(game_state *st, uint32_t p, uint32_t s)
{
    int32_t dx = absdiff(S16(st, p + 6), S16(st, s + 6));
    S32(st, DIST_X) = dx;
    int32_t dy = absdiff(S16(st, p + 8), S16(st, s + 8));
    S32(st, DIST_Y) = dy;
    return (dx < dy ? dy : dx) < 0x28;
}

void buyer_find_sources(game_state *st, int32_t fid, int32_t b)
{
    /* @bin 0x00456760 for building `b`: the buildings of types 0x47 / 0xBF (when 0x00784BBC is 0,
     * same +0x14) and 0x48 / 0xC1 of the records 0x007900BC (in use, +0x44 set, +0x20 > 0, within
     * 40 tiles) are candidates for each good they have; the nearest by walking route from b's
     * road tile (0x00519E30) gives the source of each good index (its main part 0x00475B50) */
    uint32_t p = BUILDING_ADDR(b);
    uint32_t kinds = RECORD_KIND + RECORD_SIZE * U8(st, p + 0xC1);
    memset(state_ptr(st, COUNTS, 4 * NGOODS), 0, 4 * NGOODS);
    memset(state_ptr(st, CAND_TILE, 4 * 0x1A5E0), 0, 4 * 0x1A5E0);
    memset(state_ptr(st, CAND_BUILDING, 4 * 0x1A5E0), 0, 4 * 0x1A5E0);
    memset(state_ptr(st, TARGETS, 4 * NGOODS), 0, 4 * NGOODS);
    for (int32_t i = 1; i <= S32(st, RECORDS_HIGHEST); i++) {
        uint32_t r = RECORDS + RECORD_SIZE * (uint32_t) i;
        if (U8(st, r + 4) == 0) {
            continue;
        }
        int32_t sid = S16(st, r);
        if (sid == 0) {
            continue;
        }
        uint32_t s = BUILDING_ADDR(sid);
        if (U8(st, s) != 1) {
            continue;
        }
        int16_t type = S16(st, s + 0x10);
        if (type == 0x47 || type == 0xBF) {
            /* @bin 0x00456BA2 */
            if (S32(st, 0x00784BBCu) != 0 || U8(st, s + 0x44) == 0 || S16(st, s + 0x20) <= 0 ||
                U8(st, s + 0x14) != U8(st, p + 0x14) || !near(st, p, s)) {
                continue;
            }
            /* @bin 0x00456C3F goods 1..8 (index good - 1) the building has (+0x56 by good) */
            for (int32_t good = 1; good <= 8; good++) {
                int32_t idx = good - 1;
                if (S16(st, s + 0x56u + 2u * (uint32_t) good) == 0 || U8(st, kinds + (uint32_t) good) == 1 ||
                    stock(st, b, idx) >= 20000) {
                    continue;
                }
                add_candidate(st, idx, sid);
            }
        } else if (type == 0x48 || type == 0xC1) {
            /* @bin 0x00456845 */
            if (U8(st, s + 0x44) == 0 || S16(st, s + 0x20) <= 0 || !near(st, p, s)) {
                continue;
            }
            for (int32_t good = 1; good <= 8; good++) {
                int32_t idx = good - 1;
                if (good_blocked(st, good) || GAME_CALL(st, 0x0045D510u, sid, good) <= 0 ||
                    U8(st, kinds + (uint32_t) good) == 1 || stock(st, b, idx) >= 20000) {
                    continue;
                }
                /* @bin 0x004569A5 the city's setting for the good (0x00EA4096, i16) is 1 or 3 */
                int16_t v = S16(st, 0x00EA4096u + 2u * (U32(st, CURRENT_PLAYER_ADDR) * 0x24ECu + (uint32_t) good));
                if (v != 1 && v != 3) {
                    continue;
                }
                add_candidate(st, idx, sid);
            }
            /* @bin 0x00456A26 goods 0xD, 0x13, 0x11, 0xF (indexes 9, 11, 10, 8; tables 0x00456F0C,
             * 0x00456F1C), without that setting */
            static const int32_t pairs[4][2] = {{9, 0x0D}, {11, 0x13}, {10, 0x11}, {8, 0x0F}};
            for (int k = 0; k < 4; k++) {
                int32_t idx = pairs[k][0], good = pairs[k][1];
                if (good_blocked(st, good) || GAME_CALL(st, 0x0045D510u, sid, good) <= 0 ||
                    U8(st, kinds + (uint32_t) good) == 1 || stock(st, b, idx) >= 20000) {
                    continue;
                }
                add_candidate(st, idx, sid);
            }
        }
    }
    /* @bin 0x00456D87 */
    for (int32_t idx = 0; idx < NGOODS; idx++) {
        int32_t n = S32(st, COUNTS + 4u * (uint32_t) idx);
        if (n == 0) {
            continue;
        }
        int32_t r = route_walk_to_any(st, S16(st, p + 0x26), S16(st, p + 0x28),
                                      CAND_TILE + 4u * (uint32_t) (idx * CAND_ROW), n);
        if (r != 0) {
            int32_t sid = S32(st, CAND_BUILDING + 4u * (uint32_t) (idx * CAND_ROW + r - 1));
            S32(st, TARGETS + 4u * (uint32_t) idx) = GAME_CALL(st, 0x00475B50u, sid);
        }
    }
    /* @bin 0x00456DEB four delays (+0x9E..+0xA4) of the figure's building (+0x5A) count down;
     * while one is 0 its good index (9, 11, 10, 8; tables 0x00456F98, 0x00456FA8) has no source */
    static const int32_t delayed[4] = {9, 11, 10, 8};
    uint32_t fb = BUILDING_ADDR(S16(st, FIGURE_ADDR(fid) + 0x5A));
    for (int k = 0; k < 4; k++) {
        uint32_t a = fb + 0x9Eu + 2u * (uint32_t) k;
        int16_t v = S16(st, a);
        if (v > 0) {
            S16(st, a) = (int16_t) (v - 1);
        } else {
            S32(st, COUNTS + 4u * (uint32_t) delayed[k]) = 0;
            S32(st, TARGETS + 4u * (uint32_t) delayed[k]) = 0;
        }
    }
}

/* @bin 0x004965AC / 0x004964BC a stock -> 0..8 loads (one per 100 from 100, up to 800) */
static int32_t loads_8(int32_t v)
{
    for (int32_t k = 8; k >= 1; k--) {
        if (v >= 100 * k) {
            return k;
        }
    }
    return v > 0;
}

/* @bin 0x00496404 the same up to 4 */
static int32_t loads_4(int32_t v)
{
    for (int32_t k = 4; k >= 1; k--) {
        if (v >= 100 * k) {
            return k;
        }
    }
    return v > 0;
}

static int32_t clamp_loads(int32_t v, int32_t room)
{
    /* @bin 0x00496626 the smaller of the two, 0 when not positive */
    if (v > room) {
        v = room;
    }
    return v > 0 ? v : 0;
}

/* @bin 0x00496550 / 0x00496460 room left at building b for good index idx (800, or 600 when
 * idx != 0, minus its stock and the amount carried) in loads (rounded up) */
static int32_t room_loads(game_state *st, int32_t b, int32_t idx, int32_t amount, int32_t cap)
{
    int32_t c = (int32_t) ((uint32_t) cap - (uint32_t) (int32_t) stock(st, b, idx) - (uint32_t) amount);
    return c <= 0 ? 0 : c / 100 + 1;
}

int32_t deliver_count_storage(game_state *st, int32_t fid, int32_t good, int32_t b, int32_t dest,
                              int32_t amount, int32_t idx)
{
    /* @bin 0x00496550 loads to take from a storage `dest` (+0x56 by good) */
    (void) fid;
    int32_t room = room_loads(st, b, idx, amount, idx != 0 ? 0x258 : 0x320);
    if (room == 0) {
        return 0;
    }
    return clamp_loads(loads_8(S16(st, BUILDING_ADDR(dest) + 0x56u + 2u * (uint32_t) good)), room);
}

int32_t deliver_count_parts(game_state *st, int32_t fid, int32_t good, int32_t b, int32_t dest,
                            int32_t amount, int32_t idx)
{
    /* @bin 0x00496460 the same from the stock of dest's parts (0x0045D510, called first) */
    (void) fid;
    int32_t have = GAME_CALL(st, 0x0045D510u, dest, good);
    int32_t room = room_loads(st, b, idx, amount, idx != 0 ? 0x258 : 0x320);
    if (room == 0) {
        return 0;
    }
    return clamp_loads(loads_8(have), room);
}

int32_t deliver_count_8_11(game_state *st, int32_t good, int32_t fid, int32_t b, int32_t dest,
                           int32_t amount, int32_t idx)
{
    /* @bin 0x004963B0 goods 8..11: room up to 400, loads up to 4 from dest's parts (0x0045D510,
     * called once there is room) */
    (void) fid;
    int32_t room = room_loads(st, b, idx, amount, 0x190);
    if (room == 0) {
        return 0;
    }
    return clamp_loads(loads_4(GAME_CALL(st, 0x0045D510u, dest, good)), room);
}

int32_t figure_chain_free(game_state *st, int32_t fid)
{
    /* @bin 0x00496640 fid, when its +0x68 is 0; the original spins forever otherwise */
    if (S16(st, FIGURE_ADDR(fid) + 0x68) != 0) {
        state_unported(st, 0x00496640u);
    }
    return fid;
}

/* @bin 0x00496964 / 0x004966EB n figures of type 0x42 (0x004B4800, at fid's tile), at most `max`,
 * each following the previous one (+0x68, the first follows `lead`), carrying good index idx
 * (+0x7A), of fid's building (+0x5A) */
static void chain_create(game_state *st, int32_t n, int32_t max, int32_t fid, int32_t lead, int32_t idx)
{
    uint32_t f = FIGURE_ADDR(fid);
    int32_t prev = lead;
    for (int32_t k = 1; k <= max; k++) {
        int32_t id = GAME_CALL(st, 0x004B4800u, S32(st, CURRENT_PLAYER_ADDR), 0x42, S16(st, f + 0x14),
                               S16(st, f + 0x16), 0);
        uint32_t c = FIGURE_ADDR(id);
        U16(st, c + 0x68) = (uint16_t) prev;
        U8(st, c + 0x7A) = (uint8_t) idx;
        U16(st, c + 0x5A) = U16(st, f + 0x5A);
        if (n <= k) {
            break;
        }
        prev = id;
    }
}

int32_t deliver_take(game_state *st, int32_t good, int32_t n, int32_t fid, int32_t b, int32_t dest,
                     int32_t lead)
{
    /* @bin 0x004968A0 n loads of `good` taken from dest (0x0045E360 for types 0x47 / 0xBF, else
     * 0x0045B570; n x 100) and up to 8 followers created */
    (void) b;
    int32_t idx = market_good_index(good);
    int16_t type = S16(st, BUILDING_ADDR(dest) + 0x10);
    if (type == 0x47 || type == 0xBF) {
        GAME_CALL(st, 0x0045E360u, S32(st, CURRENT_PLAYER_ADDR), dest, good, (int32_t) ((uint32_t) n * 100u));
    } else {
        GAME_CALL(st, 0x0045B570u, S32(st, CURRENT_PLAYER_ADDR), dest, good, (int32_t) ((uint32_t) n * 100u));
    }
    chain_create(st, n, 8, fid, lead, idx);
    return 1;
}

int32_t deliver_take_8_11(game_state *st, int32_t good, int32_t n, int32_t fid, int32_t b, int32_t dest,
                          int32_t lead)
{
    /* @bin 0x00496660 the same through 0x0045B570, up to 4 followers */
    (void) b;
    int32_t idx = market_good_index(good);
    GAME_CALL(st, 0x0045B570u, S32(st, CURRENT_PLAYER_ADDR), dest, good, (int32_t) ((uint32_t) n * 100u));
    chain_create(st, n, 4, fid, lead, idx);
    return 1;
}

/* @bin 0x00496C84 (table 0x00496DA8), 0x00496DF4 (table 0x00496EE4) a good index -> its good */
static int32_t index_good(int32_t idx)
{
    static const int32_t goods[12] = {1, 2, 3, 4, 5, 6, 7, 8, 0x0F, 0x0D, 0x11, 0x13};
    return (uint32_t) idx <= 11u ? goods[idx] : -1;
}

static void delivered(game_state *st, int32_t fid, int32_t slot, int32_t n)
{
    /* @bin 0x00496D70 the figure's +0xBC slot counts the delivered amount x 100 */
    uint32_t a = FIGURE_ADDR(fid) + 0xBCu + 4u * (uint32_t) slot;
    S32(st, a) = (int32_t) ((uint32_t) S32(st, a) + (uint32_t) n * 100u);
}

int32_t deliver_good_0_7(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t idx,
                         int32_t amount, int32_t slot)
{
    /* @bin 0x00496C70 a good the city trades: given to `dest` (0x00496550 for types 0x47 / 0xBF,
     * else 0x00496460), then 0x00496640, 0x004968A0 */
    int32_t good = index_good(idx);
    if (good_blocked(st, good)) {
        return 0;
    }
    int16_t type = S16(st, BUILDING_ADDR(dest) + 0x10);
    int32_t n;
    if (type == 0x47 || type == 0xBF) {
        n = deliver_count_storage(st, fid, good, b, dest, amount, idx);
    } else {
        n = deliver_count_parts(st, fid, good, b, dest, amount, idx);
    }
    if (n <= 0) {
        return 0;
    }
    int32_t r = GAME_CALL(st, 0x00496640u, fid); /* partial port: through GAME_CALL */
    deliver_take(st, good, n, fid, b, dest, r);
    delivered(st, fid, slot, n);
    return 1;
}

int32_t deliver_good_8_11(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t idx,
                          int32_t amount, int32_t slot)
{
    /* @bin 0x00496DE0 the same through 0x004963B0 and 0x00496660 */
    int32_t good = index_good(idx);
    if (good_blocked(st, good)) {
        return 0;
    }
    int32_t n = deliver_count_8_11(st, good, fid, b, dest, amount, idx);
    if (n <= 0) {
        return 0;
    }
    int32_t r = GAME_CALL(st, 0x00496640u, fid); /* partial port: through GAME_CALL */
    deliver_take_8_11(st, good, n, fid, b, dest, r);
    delivered(st, fid, slot, n);
    return 1;
}

int32_t deliver_slot_0_7(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t slot)
{
    /* @bin 0x00495CF0 the good index (+0x9C) and amount (+0xBC) of slot `slot` of figure fid */
    uint32_t f = FIGURE_ADDR(fid) + 4u * (uint32_t) slot;
    return deliver_good_0_7(st, fid, b, dest, S32(st, f + 0x9C), S32(st, f + 0xBC), slot);
}

int32_t deliver_slot_8_11(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t slot)
{
    /* @bin 0x00496010 the same, the slot read from the current figure (0x00D056F8) */
    uint32_t f = FIGURE_ADDR(S32(st, CURRENT_FIGURE)) + 4u * (uint32_t) slot;
    return deliver_good_8_11(st, fid, b, dest, S32(st, f + 0x9C), S32(st, f + 0xBC), slot);
}

int32_t soldier_nearest_0b_0d(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004998F0 the figures 1..1999 in use, not in action 4, +0x84 0, of types 0x0B..0x0D:
     * the nearest to (x, y) (largest of |dx|, |dy|, below 10000, the first on equality; kept in
     * 0x00BCD5EC); none: the first such figure whatever its +0x84, else 0 */
    int32_t best = 0;
    S32(st, BEST_DIST) = 10000;
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        uint8_t type = U8(st, f + 0x0A);
        if (U8(st, f + 0x0E) != 1 || U8(st, f + 0x38) == 4 || S16(st, f + 0x84) != 0 || type < 0x0B ||
            type > 0x0D) {
            continue;
        }
        int32_t dx = absdiff(x, S16(st, f + 0x14));
        S32(st, DIST_X) = dx;
        int32_t dy = absdiff(y, S16(st, f + 0x16));
        S32(st, DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (d < S32(st, BEST_DIST)) {
            S32(st, BEST_DIST) = d;
            best = id;
        }
    }
    if (best != 0) {
        return best;
    }
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        uint8_t type = U8(st, f + 0x0A);
        if (U8(st, f + 0x0E) == 1 && U8(st, f + 0x38) != 4 && type >= 0x0B && type <= 0x0D) {
            return id;
        }
    }
    return 0;
}

int32_t soldier_find_target(game_state *st, int32_t fid, int32_t range)
{
    /* @bin 0x00499E50 the figures 1..1999 in use, not in action 4, by their type (tables 0x0049A0D4,
     * 0x0049A0C0): a priority and the distance (largest of |dx|, |dy|) from soldier fid; the one of
     * highest priority (the first on equality) closer than the best so far (0x00BCD5EC, from
     * `range`) in line of fire (0x004B4420); its tile in 0x00D3B4E4 (x), 0x00D3B4E0 (y) */
    static const uint8_t kind[0x6E] = {
        0, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 1, 1, 1, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 4, 4, 4, 0, 1, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0,
        0, 4, 0, 0, 0, 0, 1, 0, 4, 0, 4, 0, 0, 4, 0, 0, 4, 4, 4, 4, 4, 2, 3, 4, 4, 4, 0, 0,
        0, 4, 0, 4, 4, 4, 4, 4, 0, 0, 4, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 0,
    };
    S32(st, BEST_DIST) = range;
    /* @bin 0x00499E62 the city's +0x4678 below 4: other figures are targets too */
    int all = S32(st, CURRENT_CITY(st) + 0x4678u) < 4;
    uint32_t self = FIGURE_ADDR(fid);
    int32_t sy = S16(st, self + 0x16), sx = S16(st, self + 0x14);
    int32_t best = 0, best_prio = 0;
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + 0x0E) != 1 || U8(st, f + 0x38) == 4) {
            continue;
        }
        uint8_t type = U8(st, f + 0x0A);
        int k = type <= 0x6D ? kind[type] : 4;
        if (k == 0 || (k == 4 && !all)) {
            continue;
        }
        int32_t prio;
        if (k == 1) {
            prio = 0x32;
        } else if (k == 2) {
            prio = U8(st, f + 0x130) != 0 ? 0x28 : 0x14; /* @bin 0x00499F34 */
        } else if (k == 3) {
            prio = 0x1E;
        } else {
            prio = 10;
        }
        int32_t fx = S16(st, f + 0x14), fy = S16(st, f + 0x16);
        int32_t dx = absdiff(sx, fx);
        S32(st, DIST_X) = dx;
        int32_t dy = absdiff(sy, fy);
        S32(st, DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (k == 4) {
            d += 5; /* @bin 0x0049A029 */
        }
        if (prio > best_prio && d < S32(st, BEST_DIST) && GAME_CALL(st, 0x004B4420u, sx, sy, fx, fy) != 0) {
            best = id;
            S32(st, BEST_DIST) = d;
            best_prio = prio;
        }
    }
    if (best == 0) {
        return 0;
    }
    S32(st, 0x00D3B4E0u) = S16(st, FIGURE_ADDR(best) + 0x16);
    S32(st, 0x00D3B4E4u) = S16(st, FIGURE_ADDR(best) + 0x14);
    return best;
}

int32_t formation_create_0x4b6ff0(game_state *st, int32_t a, int32_t kind, int32_t sub, int32_t x, int32_t y)
{
    /* @bin 0x004B6FF0 the first free formation of 10..49 (+0 0) whose +0x69 is 0; free ones with
     * +0x69 set are marked in use (+0 1) on the way. It gets +0 1, +1 0, +2 its index - 10, +4 a,
     * +0x36 x, +0x38 y, +0x42 0, +0x2A kind (10: 1, or 2 when sub is neither 0 nor 4), +0x2C 100.
     * Returns its id, or 0 */
    for (int32_t id = 10; id < 50; id++) {
        uint32_t f = FORMATION_ADDR(id);
        if (U8(st, f) != 0) {
            continue;
        }
        if (U8(st, f + 0x69) != 0) {
            U8(st, f) = 1;
            continue;
        }
        U16(st, f + 0x36) = (uint16_t) x;
        U16(st, f + 0x38) = (uint16_t) y;
        U8(st, f) = 1;
        U8(st, f + 0x42) = 0;
        U8(st, f + 1) = 0;
        U16(st, f + 4) = (uint16_t) a;
        U8(st, f + 2) = (uint8_t) (id - 10);
        if (kind != 10) {
            U16(st, f + 0x2A) = (uint16_t) kind;
        } else if (sub != 0 && sub != 4) {
            U16(st, f + 0x2A) = 2;
        } else {
            U16(st, f + 0x2A) = 1;
        }
        U16(st, f + 0x2C) = 100;
        return id;
    }
    return 0;
}

void event_message_0x4463c0(game_state *st, int32_t n, int32_t arg)
{
    /* @bin 0x004463C0 event n (0 .. 0x00785D70 - 1) of type 2 (+2): its message block
     * (0x0044E6F0) with +2 / +0x10 by its subtype +4 (tables 0x004464D0, 0x004464E0; 2 and 3 by
     * 0x0044EF60), then message 0x85 (block +0: 0x128, 0x127, -, 0x129) for 0x00775900 */
    if (n < 0 || n >= S32(st, 0x00785D70u)) {
        return;
    }
    uint32_t e = 0x00785D74u + 0x7Cu * (uint32_t) n;
    if (U8(st, e + 2) != 2) {
        return;
    }
    uint8_t blk[32] = {0}; /* the original's is uninitialised stack; +0x1E, +0x1F are not set */
    int16_t sub = S16(st, e + 4);
    switch (sub) {
    case 1:
        put16(blk, 2, 0xB6);
        put16(blk, 0x10, 0xBE);
        break;
    case 2:
        put16(blk, 2, 0xAC);
        put16(blk, 0x10, ((int8_t) event_linked_flag(st, e) != 0) + 0xB4);
        break;
    case 3:
        put16(blk, 2, 0xA2);
        put16(blk, 0x10, ((int8_t) event_linked_flag(st, e) != 0) + 0xAA);
        break;
    case 4:
        put16(blk, 2, 0xFF);
        put16(blk, 0x10, 0x107);
        break;
    default:
        return;
    }
    event_fill_block(st, e, blk);
    switch (S16(st, e + 4)) {
    case 1: put16(blk, 0, 0x128); break;
    case 2: put16(blk, 0, 0x127); break;
    case 4: put16(blk, 0, 0x129); break;
    default: return;
    }
    S32(st, 0x00E950DCu) = 1;
    message_post(st, 0x85, S16(st, 0x00775900u), arg, blk, 0);
}

void building_destroy_at_0x469140(game_state *st, int32_t x, int32_t y, int32_t tile)
{
    /* @bin 0x00469140 the building on grid offset `tile` (0x00B4E6FC; its main part 0x00475B50)
     * is removed; none: 0x004B7510 when the tile has bit 0x4000 (0x00AF598C), 0x0047C2B0 */
    static const int32_t share[5] = {0, 0x0F, 0x1E, 0x3C, 100}; /* @bin 0x00469154 by 0x00E39080 */
    int32_t id = S16(st, 0x00B4E6FCu + 2u * (uint32_t) tile);
    if (id <= 0) {
        if (U32(st, 0x00AF598Cu + 4u * (uint32_t) tile) & 0x4000u) {
            GAME_CALL(st, 0x004B7510u, x, y);
        }
        GAME_CALL(st, 0x0047C2B0u, 0, x, y, 1, 1);
    } else {
        int32_t b = GAME_CALL(st, 0x00475B50u, id);
        uint32_t ba = BUILDING_ADDR(b);
        if ((int8_t) GAME_CALL(st, 0x00474380u, S16(st, ba + 0x10)) != 0) {
            return;
        }
        int16_t type = S16(st, ba + 0x10);
        if (type == 0xCA) {
            GAME_CALL(st, 0x004C03D0u, b);
        } else if (type == 0x1F || type == 0x1E || type == 0x21 || type == 0xD1) {
            int32_t r = GAME_CALL(st, 0x0046D0F0u, b);
            if (r > 0) {
                S32(st, 0x0077567Cu) = 1;
                GAME_CALL(st, 0x0043FD00u, r, S16(st, BUILDING_ADDR(r) + 0x10));
            }
        } else {
            GAME_CALL(st, 0x0047C2B0u, b, S16(st, ba + 6), S16(st, ba + 8), U8(st, ba + 3), U8(st, ba + 3));
        }
        if (U8(st, ba) == 1) {
            type = S16(st, ba + 0x10);
            /* @bin 0x00469273 types without the city counters */
            int skip = type == 0x0A || type == 0x0B || type == 0x37 || type == 0x51 || type == 0x5C ||
                       type == 0x39 || type == 0x36 || type == 0x3A || type == 0xAA || type == 0xAB ||
                       type == 0xC8 || type == 0xC9 || type == 0xCA || type == 0x3B || type == 0xAC ||
                       type == 0xAD;
            if (!skip) {
                if (GAME_CALL(st, 0x00459EC0u, b) != 0) {
                    uint32_t c = CURRENT_CITY(st);
                    S32(st, c + 0x4658) += 1;
                    int32_t lvl = S32(st, 0x00E39080u);
                    if ((uint32_t) lvl > 4u) {
                        state_unported(st, 0x00469140u); /* the original reads its stack beyond the table */
                        return;
                    }
                    int32_t amt = GAME_CALL(st, 0x004CDE40u, S32(st, c + 0x4450), share[lvl]);
                    if (amt != 0) {
                        c = CURRENT_CITY(st);
                        S32(st, c + 0x4450) -= amt;
                        S32(st, c + 0x4784) += amt;
                        GAME_CALL(st, 0x004E2970u, 0xC7, 0, 0);
                    }
                } else if (GAME_CALL(st, 0x00459EF0u, b) != 0) {
                    uint32_t c = CURRENT_CITY(st);
                    S32(st, c + 0x4658) += 1;
                    int32_t lvl = S32(st, 0x00E39080u);
                    if ((uint32_t) lvl > 4u) {
                        state_unported(st, 0x00469140u); /* the original reads its stack beyond the table */
                        return;
                    }
                    int32_t amt = GAME_CALL(st, 0x004CDE40u, S32(st, ba + 0xB8), share[lvl]);
                    if (amt != 0) {
                        GAME_CALL0(st, 0x004684E0u);
                        S32(st, 0x00E950DCu) = 1;
                        c = CURRENT_CITY(st);
                        S16(st, c + 0x448C) += 1;
                        S32(st, c + 0x2C84) += amt;
                        S32(st, c + 0x4764) += amt;
                        S32(st, c + 0x0C) -= amt;
                        /* @bin 0x0046943A (tables 0x00469548, 0x00469558) */
                        switch (S16(st, ba + 0x10)) {
                        case 0x54: case 0x55: case 0x56: case 0x57:
                            GAME_CALL(st, 0x004E2970u, 0xC9, 0, 0);
                            break;
                        case 0xB8:
                            GAME_CALL(st, 0x004E2970u, 0xCA, 0, 0);
                            break;
                        case 0xBB: case 0xBC: case 0xBD:
                            GAME_CALL(st, 0x004E2970u, 0xC8, 0, 0);
                            break;
                        default:
                            break;
                        }
                    }
                } else {
                    S32(st, CURRENT_CITY(st) + 0x4658) += 1;
                }
            }
            uint32_t c = CURRENT_CITY(st);
            if (S32(st, c + 0x4658) >= 0x0C) {
                S32(st, c + 0x4658) = 0x0C;
            }
            U8(st, ba) = 4;
            GAME_CALL(st, 0x0048C0E0u, S16(st, ba + 6), S16(st, ba + 8), U8(st, ba + 3), 1);
            GAME_CALL(st, 0x004695D0u, b, 0);
        }
    }
    GAME_CALL0(st, 0x00497B90u);
    GAME_CALL(st, 0x0047AFD0u, x, y, 3);
    GAME_CALL(st, 0x0047BAE0u, x - 2, y - 2, x + 2, y + 2, 0);
    GAME_CALL0(st, 0x00517370u);
    GAME_CALL0(st, 0x00517660u);
    GAME_CALL0(st, 0x005178E0u);
}

static int32_t run_find_sources(game_state *st, const int32_t *a)
{
    buyer_find_sources(st, a[0], a[1]);
    return 0;
}

static int32_t run_slot_0_7(game_state *st, const int32_t *a)
{
    return deliver_slot_0_7(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_slot_8_11(game_state *st, const int32_t *a)
{
    return deliver_slot_8_11(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_good_0_7(game_state *st, const int32_t *a)
{
    return deliver_good_0_7(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_good_8_11(game_state *st, const int32_t *a)
{
    return deliver_good_8_11(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_nearest_0b_0d(game_state *st, const int32_t *a)
{
    return soldier_nearest_0b_0d(st, a[0], a[1]);
}

static int32_t run_find_target(game_state *st, const int32_t *a)
{
    return soldier_find_target(st, a[0], a[1]);
}

static int32_t run_count_storage(game_state *st, const int32_t *a)
{
    return deliver_count_storage(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_count_parts(game_state *st, const int32_t *a)
{
    return deliver_count_parts(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_count_8_11(game_state *st, const int32_t *a)
{
    return deliver_count_8_11(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_chain_free(game_state *st, const int32_t *a)
{
    return figure_chain_free(st, a[0]);
}

static int32_t run_take(game_state *st, const int32_t *a)
{
    return deliver_take(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_take_8_11(game_state *st, const int32_t *a)
{
    return deliver_take_8_11(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_formation_create(game_state *st, const int32_t *a)
{
    return formation_create_0x4b6ff0(st, a[0], a[1], a[2], a[3], a[4]);
}

static int32_t run_event_message(game_state *st, const int32_t *a)
{
    event_message_0x4463c0(st, a[0], a[1]);
    return 0;
}

static int32_t run_destroy_at(game_state *st, const int32_t *a)
{
    building_destroy_at_0x469140(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function figx_a_ports[] = {
    {0x004B6FF0u, "formation_create_0x4b6ff0", 5, 32, 0, run_formation_create, 0},
    {0x004463C0u, "event_message_0x4463c0", 2, 0, 0, run_event_message, 0},
    {0x00469140u, "building_destroy_at_0x469140", 3, 0, 0, run_destroy_at, 0},
    {0x00496550u, "deliver_count_storage", 6, 32, 0, run_count_storage, 0},
    {0x00496460u, "deliver_count_parts", 6, 32, 0, run_count_parts, 0},
    {0x004963B0u, "deliver_count_8_11", 6, 32, 0, run_count_8_11, 0},
    {0x00496640u, "figure_chain_free", 1, 32, 0, run_chain_free, 1},
    {0x004968A0u, "deliver_take", 6, 32, 0, run_take, 0},
    {0x00496660u, "deliver_take_8_11", 6, 32, 0, run_take_8_11, 0},
    {0x00456760u, "buyer_find_sources", 2, 0, 0, run_find_sources, 0},
    {0x00495CF0u, "deliver_slot_0_7", 4, 32, 0, run_slot_0_7, 0},
    {0x00496010u, "deliver_slot_8_11", 4, 32, 0, run_slot_8_11, 0},
    {0x00496C70u, "deliver_good_0_7", 6, 32, 0, run_good_0_7, 0},
    {0x00496DE0u, "deliver_good_8_11", 6, 32, 0, run_good_8_11, 0},
    {0x004998F0u, "soldier_nearest_0b_0d", 2, 32, 0, run_nearest_0b_0d, 0},
    {0x00499E50u, "soldier_find_target", 2, 32, 0, run_find_target, 0},
};
const int figx_a_ports_count = sizeof(figx_a_ports) / sizeof(figx_a_ports[0]);
