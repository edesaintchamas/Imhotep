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
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


#define CAND_ROW 4000
#define NGOODS 27
#define RECORD_SIZE 0xC4u

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
    return S32(st, CITY_BASE + CITY_U2A88 + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) good)) != 0;
}

static int16_t stock(game_state *st, int32_t b, int32_t idx)
{
    return S16(st, BUILDING_ADDR(b) + BLD_INVENTORY + 2u * (uint32_t) idx);
}

/* @bin 0x004569CA / 0x00456ACB / 0x00456D15 candidate building `sid` for good index `idx` */
static void add_candidate(game_state *st, int32_t idx, int32_t sid)
{
    uint32_t s = BUILDING_ADDR(sid);
    int32_t n = S32(st, G_COUNTS + 4u * (uint32_t) idx);
    uint32_t slot = (uint32_t) (idx * CAND_ROW + n);
    S32(st, G_CAND_BUILDING + 4u * slot) = sid;
    S32(st, G_CAND_TILE + 4u * slot) = (int32_t) (U32(st, G_MAP_START) + (uint32_t) (int32_t) S16(st, s + BLD_ROAD_ACCESS_X) +
                                                (uint32_t) ((int32_t) S16(st, s + BLD_ROAD_ACCESS_Y) * GRID_SIZE));
    S32(st, G_COUNTS + 4u * (uint32_t) idx) = n + 1;
}

/* @bin 0x00456861 / 0x00456BDF the larger of the distances between the buildings (kept in
 * 0x00D3B4FC, 0x00D3B4F8) is below 40 */
static int near(game_state *st, uint32_t p, uint32_t s)
{
    int32_t dx = absdiff(S16(st, p + 6), S16(st, s + 6));
    S32(st, G_DIST_X) = dx;
    int32_t dy = absdiff(S16(st, p + 8), S16(st, s + 8));
    S32(st, G_DIST_Y) = dy;
    return (dx < dy ? dy : dx) < 0x28;
}

void buyer_find_sources(game_state *st, int32_t fid, int32_t b)
{
    /* @bin 0x00456760 for building `b`: the buildings of types 0x47 / 0xBF (when 0x00784BBC is 0,
     * same +0x14) and 0x48 / 0xC1 of the records 0x007900BC (in use, +0x44 set, +0x20 > 0, within
     * 40 tiles) are candidates for each good they have; the nearest by walking route from b's
     * road tile (0x00519E30) gives the source of each good index (its main part 0x00475B50) */
    uint32_t p = BUILDING_ADDR(b);
    uint32_t kinds = G_STORAGE_ORDERS + RECORD_SIZE * U8(st, p + BLD_STORAGE_ID);
    memset(state_ptr(st, G_COUNTS, 4 * NGOODS), 0, 4 * NGOODS);
    memset(state_ptr(st, G_CAND_TILE, 4 * 0x1A5E0), 0, 4 * 0x1A5E0);
    memset(state_ptr(st, G_CAND_BUILDING, 4 * 0x1A5E0), 0, 4 * 0x1A5E0);
    memset(state_ptr(st, G_TARGETS, 4 * NGOODS), 0, 4 * NGOODS);
    for (int32_t i = 1; i <= S32(st, G_STORAGE_HIGHEST); i++) {
        uint32_t r = G_MARKET_RECORDS + RECORD_SIZE * (uint32_t) i;
        if (U8(st, r + 4) == 0) {
            continue;
        }
        int32_t sid = S16(st, r);
        if (sid == 0) {
            continue;
        }
        uint32_t s = BUILDING_ADDR(sid);
        if (U8(st, s + BLD_STATE) != 1) {
            continue;
        }
        int16_t type = S16(st, s + BLD_TYPE);
        if (type == 0x47 || type == 0xBF) {
            /* @bin 0x00456BA2 */
            if (S32(st, G_U784BBC) != 0 || U8(st, s + BLD_U44) == 0 || S16(st, s + BLD_DISTANCE) <= 0 ||
                U8(st, s + BLD_ROAD_NETWORK) != U8(st, p + BLD_ROAD_NETWORK) || !near(st, p, s)) {
                continue;
            }
            /* @bin 0x00456C3F goods 1..8 (index good - 1) the building has (+0x56 by good) */
            for (int32_t good = 1; good <= 8; good++) {
                int32_t idx = good - 1;
                if (S16(st, s + BLD_INVENTORY + 2u * (uint32_t) good) == 0 || U8(st, kinds + (uint32_t) good) == 1 ||
                    stock(st, b, idx) >= 20000) {
                    continue;
                }
                add_candidate(st, idx, sid);
            }
        } else if (type == 0x48 || type == 0xC1) {
            /* @bin 0x00456845 */
            if (U8(st, s + BLD_U44) == 0 || S16(st, s + BLD_DISTANCE) <= 0 || !near(st, p, s)) {
                continue;
            }
            for (int32_t good = 1; good <= 8; good++) {
                int32_t idx = good - 1;
                if (good_blocked(st, good) || GAME_CALL(st, FN_BUILDING_PARTS_STOCK, sid, good) <= 0 ||
                    U8(st, kinds + (uint32_t) good) == 1 || stock(st, b, idx) >= 20000) {
                    continue;
                }
                /* @bin 0x004569A5 the city's setting for the good (0x00EA4096, i16) is 1 or 3 */
                int16_t v = S16(st, CITY_BASE + CITY_GOOD_SETTING + 2u * (U32(st, CURRENT_PLAYER_ADDR) * 0x24ECu + (uint32_t) good));
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
                if (good_blocked(st, good) || GAME_CALL(st, FN_BUILDING_PARTS_STOCK, sid, good) <= 0 ||
                    U8(st, kinds + (uint32_t) good) == 1 || stock(st, b, idx) >= 20000) {
                    continue;
                }
                add_candidate(st, idx, sid);
            }
        }
    }
    /* @bin 0x00456D87 */
    for (int32_t idx = 0; idx < NGOODS; idx++) {
        int32_t n = S32(st, G_COUNTS + 4u * (uint32_t) idx);
        if (n == 0) {
            continue;
        }
        int32_t r = route_walk_to_any(st, S16(st, p + BLD_ROAD_ACCESS_X), S16(st, p + BLD_ROAD_ACCESS_Y),
                                      G_CAND_TILE + 4u * (uint32_t) (idx * CAND_ROW), n);
        if (r != 0) {
            int32_t sid = S32(st, G_CAND_BUILDING + 4u * (uint32_t) (idx * CAND_ROW + r - 1));
            S32(st, G_TARGETS + 4u * (uint32_t) idx) = GAME_CALL(st, FN_BUILDING_MAIN_PART, sid);
        }
    }
    /* @bin 0x00456DEB four delays (+0x9E..+0xA4) of the figure's building (+0x5A) count down;
     * while one is 0 its good index (9, 11, 10, 8; tables 0x00456F98, 0x00456FA8) has no source */
    static const int32_t delayed[4] = {9, 11, 10, 8};
    uint32_t fb = BUILDING_ADDR(S16(st, FIGURE_ADDR(fid) + FIG_HOME));
    for (int k = 0; k < 4; k++) {
        uint32_t a = fb + BLD_U9E + 2u * (uint32_t) k;
        int16_t v = S16(st, a);
        if (v > 0) {
            S16(st, a) = (int16_t) (v - 1);
        } else {
            S32(st, G_COUNTS + 4u * (uint32_t) delayed[k]) = 0;
            S32(st, G_TARGETS + 4u * (uint32_t) delayed[k]) = 0;
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
    return clamp_loads(loads_8(S16(st, BUILDING_ADDR(dest) + BLD_INVENTORY + 2u * (uint32_t) good)), room);
}

int32_t deliver_count_parts(game_state *st, int32_t fid, int32_t good, int32_t b, int32_t dest,
                            int32_t amount, int32_t idx)
{
    /* @bin 0x00496460 the same from the stock of dest's parts (0x0045D510, called first) */
    (void) fid;
    int32_t have = GAME_CALL(st, FN_BUILDING_PARTS_STOCK, dest, good);
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
    return clamp_loads(loads_4(GAME_CALL(st, FN_BUILDING_PARTS_STOCK, dest, good)), room);
}

int32_t figure_chain_free(game_state *st, int32_t fid)
{
    /* @bin 0x00496640 fid, when its +0x68 is 0; the original spins forever otherwise */
    if (S16(st, FIGURE_ADDR(fid) + FIG_LEADER) != 0) {
        state_unported(st, FN_FIGURE_CHAIN_FREE);
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
        int32_t id = GAME_CALL(st, FN_FIGURE_CREATE, S32(st, CURRENT_PLAYER_ADDR), 0x42, S16(st, f + FIG_X),
                               S16(st, f + FIG_Y), 0);
        uint32_t c = FIGURE_ADDR(id);
        U16(st, c + FIG_LEADER) = (uint16_t) prev;
        U8(st, c + FIG_COLLECTING) = (uint8_t) idx;
        U16(st, c + FIG_HOME) = U16(st, f + FIG_HOME);
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
    int16_t type = S16(st, BUILDING_ADDR(dest) + BLD_TYPE);
    if (type == 0x47 || type == 0xBF) {
        GAME_CALL(st, FN_STORAGE_TAKE_STOCK, S32(st, CURRENT_PLAYER_ADDR), dest, good, (int32_t) ((uint32_t) n * 100u));
    } else {
        GAME_CALL(st, FN_STORAGE_TAKE, S32(st, CURRENT_PLAYER_ADDR), dest, good, (int32_t) ((uint32_t) n * 100u));
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
    GAME_CALL(st, FN_STORAGE_TAKE, S32(st, CURRENT_PLAYER_ADDR), dest, good, (int32_t) ((uint32_t) n * 100u));
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
    uint32_t a = FIGURE_ADDR(fid) + FIG_BUYERS_3 + 4u * (uint32_t) slot;
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
    int16_t type = S16(st, BUILDING_ADDR(dest) + BLD_TYPE);
    int32_t n;
    if (type == 0x47 || type == 0xBF) {
        n = deliver_count_storage(st, fid, good, b, dest, amount, idx);
    } else {
        n = deliver_count_parts(st, fid, good, b, dest, amount, idx);
    }
    if (n <= 0) {
        return 0;
    }
    int32_t r = GAME_CALL(st, FN_FIGURE_CHAIN_FREE, fid); /* partial port: through GAME_CALL */
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
    int32_t r = GAME_CALL(st, FN_FIGURE_CHAIN_FREE, fid); /* partial port: through GAME_CALL */
    deliver_take_8_11(st, good, n, fid, b, dest, r);
    delivered(st, fid, slot, n);
    return 1;
}

int32_t deliver_slot_0_7(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t slot)
{
    /* @bin 0x00495CF0 the good index (+0x9C) and amount (+0xBC) of slot `slot` of figure fid */
    uint32_t f = FIGURE_ADDR(fid) + 4u * (uint32_t) slot;
    return deliver_good_0_7(st, fid, b, dest, S32(st, f + FIG_BUYERS_1), S32(st, f + FIG_BUYERS_3), slot);
}

int32_t deliver_slot_8_11(game_state *st, int32_t fid, int32_t b, int32_t dest, int32_t slot)
{
    /* @bin 0x00496010 the same, the slot read from the current figure (0x00D056F8) */
    uint32_t f = FIGURE_ADDR(S32(st, G_CURRENT_FIGURE)) + 4u * (uint32_t) slot;
    return deliver_good_8_11(st, fid, b, dest, S32(st, f + FIG_BUYERS_1), S32(st, f + FIG_BUYERS_3), slot);
}

int32_t soldier_nearest_0b_0d(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004998F0 the figures 1..1999 in use, not in action 4, +0x84 0, of types 0x0B..0x0D:
     * the nearest to (x, y) (largest of |dx|, |dy|, below 10000, the first on equality; kept in
     * 0x00BCD5EC); none: the first such figure whatever its +0x84, else 0 */
    int32_t best = 0;
    S32(st, G_BEST_DISTANCE) = 10000;
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        uint8_t type = U8(st, f + FIG_TYPE);
        if (U8(st, f + FIG_STATE) != 1 || U8(st, f + FIG_ACTION) == 4 || S16(st, f + FIG_TARGETED_BY) != 0 || type < 0x0B ||
            type > 0x0D) {
            continue;
        }
        int32_t dx = absdiff(x, S16(st, f + FIG_X));
        S32(st, G_DIST_X) = dx;
        int32_t dy = absdiff(y, S16(st, f + FIG_Y));
        S32(st, G_DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (d < S32(st, G_BEST_DISTANCE)) {
            S32(st, G_BEST_DISTANCE) = d;
            best = id;
        }
    }
    if (best != 0) {
        return best;
    }
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        uint8_t type = U8(st, f + FIG_TYPE);
        if (U8(st, f + FIG_STATE) == 1 && U8(st, f + FIG_ACTION) != 4 && type >= 0x0B && type <= 0x0D) {
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
    S32(st, G_BEST_DISTANCE) = range;
    /* @bin 0x00499E62 the city's +0x4678 below 4: other figures are targets too */
    int all = S32(st, CURRENT_CITY(st) + CITY_U4678) < 4;
    uint32_t self = FIGURE_ADDR(fid);
    int32_t sy = S16(st, self + FIG_Y), sx = S16(st, self + FIG_X);
    int32_t best = 0, best_prio = 0;
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + FIG_STATE) != 1 || U8(st, f + FIG_ACTION) == 4) {
            continue;
        }
        uint8_t type = U8(st, f + FIG_TYPE);
        int k = type <= 0x6D ? kind[type] : 4;
        if (k == 0 || (k == 4 && !all)) {
            continue;
        }
        int32_t prio;
        if (k == 1) {
            prio = 0x32;
        } else if (k == 2) {
            prio = U8(st, f + FIG_U130) != 0 ? 0x28 : 0x14; /* @bin 0x00499F34 */
        } else if (k == 3) {
            prio = 0x1E;
        } else {
            prio = 10;
        }
        int32_t fx = S16(st, f + FIG_X), fy = S16(st, f + FIG_Y);
        int32_t dx = absdiff(sx, fx);
        S32(st, G_DIST_X) = dx;
        int32_t dy = absdiff(sy, fy);
        S32(st, G_DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (k == 4) {
            d += 5; /* @bin 0x0049A029 */
        }
        if (prio > best_prio && d < S32(st, G_BEST_DISTANCE) && GAME_CALL(st, FN_LINE_OF_FIRE_CLEAR, sx, sy, fx, fy) != 0) {
            best = id;
            S32(st, G_BEST_DISTANCE) = d;
            best_prio = prio;
        }
    }
    if (best == 0) {
        return 0;
    }
    S32(st, G_ROAD_ACCESS_Y) = S16(st, FIGURE_ADDR(best) + FIG_Y);
    S32(st, G_ROAD_ACCESS_X) = S16(st, FIGURE_ADDR(best) + FIG_X);
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
        if (U8(st, f + FM_IN_USE) != 0) {
            continue;
        }
        if (U8(st, f + FM_HALTED) != 0) {
            U8(st, f + FM_IN_USE) = 1;
            continue;
        }
        U16(st, f + FM_U36) = (uint16_t) x;
        U16(st, f + FM_U38) = (uint16_t) y;
        U8(st, f + FM_IN_USE) = 1;
        U8(st, f + FM_PLAYER) = 0;
        U8(st, f + FM_OWNER) = 0;
        U16(st, f + FM_TYPE) = (uint16_t) a;
        U8(st, f + FM_LEGION) = (uint8_t) (id - 10);
        if (kind != 10) {
            U16(st, f + FM_LAYOUT) = (uint16_t) kind;
        } else if (sub != 0 && sub != 4) {
            U16(st, f + FM_LAYOUT) = 2;
        } else {
            U16(st, f + FM_LAYOUT) = 1;
        }
        U16(st, f + FM_MORALE) = 100;
        return id;
    }
    return 0;
}

void event_message_0x4463c0(game_state *st, int32_t n, int32_t arg)
{
    /* @bin 0x004463C0 event n (0 .. 0x00785D70 - 1) of type 2 (+2): its message block
     * (0x0044E6F0) with +2 / +0x10 by its subtype +4 (tables 0x004464D0, 0x004464E0; 2 and 3 by
     * 0x0044EF60), then message 0x85 (block +0: 0x128, 0x127, -, 0x129) for 0x00775900 */
    if (n < 0 || n >= S32(st, G_EVENTS)) {
        return;
    }
    uint32_t e = G_KINGDOM_EVENTS + 0x7Cu * (uint32_t) n;
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
    S32(st, G_MESSAGE_POPUP) = 1;
    message_post(st, 0x85, S16(st, G_INVASION_SEQUENCE), arg, blk, 0);
}

void building_destroy_at_0x469140(game_state *st, int32_t x, int32_t y, int32_t tile)
{
    /* @bin 0x00469140 the building on grid offset `tile` (0x00B4E6FC; its main part 0x00475B50)
     * is removed; none: 0x004B7510 when the tile has bit 0x4000 (0x00AF598C), 0x0047C2B0 */
    static const int32_t share[5] = {0, 0x0F, 0x1E, 0x3C, 100}; /* @bin 0x00469154 by 0x00E39080 */
    int32_t id = S16(st, G_BUILDING_GRID + 2u * (uint32_t) tile);
    if (id <= 0) {
        if (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 0x4000u) {
            GAME_CALL(st, FN_FIGURES_2A_NEAR, x, y);
        }
        GAME_CALL(st, FN_BUILDING_RUBBLE_TILES, 0, x, y, 1, 1);
    } else {
        int32_t b = GAME_CALL(st, FN_BUILDING_MAIN_PART, id);
        uint32_t ba = BUILDING_ADDR(b);
        if ((int8_t) GAME_CALL(st, FN_CLEAR_IS_MONUMENT_TYPE, S16(st, ba + BLD_TYPE)) != 0) {
            return;
        }
        int16_t type = S16(st, ba + BLD_TYPE);
        if (type == 0xCA) {
            GAME_CALL(st, FN_BUILDING_RUBBLE_4C03D0, b);
        } else if (type == 0x1F || type == 0x1E || type == 0x21 || type == 0xD1) {
            int32_t r = GAME_CALL(st, FN_BUILDING_MAIN_PART_46D0F0, b);
            if (r > 0) {
                S32(st, G_U77567C) = 1;
                GAME_CALL(st, FN_BUILDING_CLEAR_AREA, r, S16(st, BUILDING_ADDR(r) + BLD_TYPE));
            }
        } else {
            GAME_CALL(st, FN_BUILDING_RUBBLE_TILES, b, S16(st, ba + BLD_X), S16(st, ba + BLD_Y), U8(st, ba + BLD_SIZE), U8(st, ba + BLD_SIZE));
        }
        if (U8(st, ba + BLD_STATE) == 1) {
            type = S16(st, ba + BLD_TYPE);
            /* @bin 0x00469273 types without the city counters */
            int skip = type == 0x0A || type == 0x0B || type == 0x37 || type == 0x51 || type == 0x5C ||
                       type == 0x39 || type == 0x36 || type == 0x3A || type == 0xAA || type == 0xAB ||
                       type == 0xC8 || type == 0xC9 || type == 0xCA || type == 0x3B || type == 0xAC ||
                       type == 0xAD;
            if (!skip) {
                if (GAME_CALL(st, FN_BUILDING_TYPE_4D_4F, b) != 0) {
                    uint32_t c = CURRENT_CITY(st);
                    S32(st, c + CITY_U4658) += 1;
                    int32_t lvl = S32(st, G_DIFFICULTY);
                    if ((uint32_t) lvl > 4u) {
                        state_unported(st, FN_BUILDING_DESTROY_AT_0X469140); /* the original reads its stack beyond the table */
                        return;
                    }
                    int32_t amt = GAME_CALL(st, FN_CALC_PERCENTAGE, S32(st, c + CITY_U4450), share[lvl]);
                    if (amt != 0) {
                        c = CURRENT_CITY(st);
                        S32(st, c + CITY_U4450) -= amt;
                        S32(st, c + CITY_U4784) += amt;
                        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xC7, 0, 0);
                    }
                } else if (GAME_CALL(st, FN_BUILDING_TYPE_0X459EF0, b) != 0) {
                    uint32_t c = CURRENT_CITY(st);
                    S32(st, c + CITY_U4658) += 1;
                    int32_t lvl = S32(st, G_DIFFICULTY);
                    if ((uint32_t) lvl > 4u) {
                        state_unported(st, FN_BUILDING_DESTROY_AT_0X469140); /* the original reads its stack beyond the table */
                        return;
                    }
                    int32_t amt = GAME_CALL(st, FN_CALC_PERCENTAGE, S32(st, ba + BLD_UB8), share[lvl]);
                    if (amt != 0) {
                        GAME_CALL0(st, FN_DAY_SERVICE_SHARES);
                        S32(st, G_MESSAGE_POPUP) = 1;
                        c = CURRENT_CITY(st);
                        S16(st, c + CITY_U448C) += 1;
                        S32(st, c + CITY_U2C84) += amt;
                        S32(st, c + CITY_U4764) += amt;
                        S32(st, c + CITY_TREASURY) -= amt;
                        /* @bin 0x0046943A (tables 0x00469548, 0x00469558) */
                        switch (S16(st, ba + BLD_TYPE)) {
                        case 0x54: case 0x55: case 0x56: case 0x57:
                            GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xC9, 0, 0);
                            break;
                        case 0xB8:
                            GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xCA, 0, 0);
                            break;
                        case 0xBB: case 0xBC: case 0xBD:
                            GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xC8, 0, 0);
                            break;
                        default:
                            break;
                        }
                    }
                } else {
                    S32(st, CURRENT_CITY(st) + CITY_U4658) += 1;
                }
            }
            uint32_t c = CURRENT_CITY(st);
            if (S32(st, c + CITY_U4658) >= 0x0C) {
                S32(st, c + CITY_U4658) = 0x0C;
            }
            U8(st, ba + BLD_STATE) = 4;
            GAME_CALL(st, FN_FIGURE_SPAWN_RING, S16(st, ba + BLD_X), S16(st, ba + BLD_Y), U8(st, ba + BLD_SIZE), 1);
            GAME_CALL(st, FN_BUILDING_DESTROY_LINKED, b, 0);
        }
    }
    GAME_CALL0(st, FN_FIGURES_0X2A_ROTATE);
    GAME_CALL(st, FN_WALLS_RECT_47AFD0, x, y, 3);
    GAME_CALL(st, FN_CANALS_RECT, x - 2, y - 2, x + 2, y + 2, 0);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EE0C50);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EC7630);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EBAB20);
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
