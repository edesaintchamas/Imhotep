/* Imhotep - the buyers of buildings of types 0x46 / 0xBE (package bigb): which goods they want and
 * which building they fetch them from (called by the building update 0x00462180).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32). */
#include "building/market.h"

#include <string.h>

#include "map/route_flood.h"
#include "map/route_search.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define GOODS_COUNT 0x00F53DD8u /* i32: entries of the goods list */
#define GOODS_LIST 0x00F53DDCu  /* i32 per entry: a good (1..0x13), 0: none */
#define COUNTS 0x0096E644u      /* 27 i32: candidate sources per good index */
#define CAND_TILE 0x0089B744u   /* 27 x 4000 i32: grid offset of each candidate's road tile */
#define CAND_BUILDING 0x00904EC4u /* 27 x 4000 i32: each candidate's building id */
#define TARGETS 0x0089B6D8u     /* 27 i32: the nearest candidate (after 0x00475B50) per good index */
#define CAND_ROW 4000
#define NGOODS 27
#define RECORDS 0x007900BCu     /* records of 0xC4 bytes: +0 i16 building, +4 u8 in use */
#define RECORD_SIZE 0xC4u
#define RECORDS_HIGHEST 0x0078D788u
#define RECORD_KIND 0x007900C2u /* u8 per good of the record selected by building +0xC1 */
#define MAP_START 0x00BCA2D8u

/* @bin 0x00455E41 (and the same table in 0x00456FC0, 0x004561E0, 0x00462180) a good -> its index
 * in the 27-entry arrays; -1 for the goods without one */
int32_t market_good_index(int32_t good)
{
    static const int8_t idx[0x13] = {0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, 9, -1, 8, -1, 10, -1, 11};
    uint32_t i = (uint32_t) good - 1u;
    return i <= 0x12u ? idx[i] : -1;
}

/* building +0x56: i16 per good index (index -1 reads +0x54) */
static int16_t stock(game_state *st, int32_t id, int32_t idx)
{
    return S16(st, BUILDING_ADDR(id) + 0x56u + 2u * (uint32_t) idx);
}

static int32_t count_of(game_state *st, uint32_t counts, int32_t idx)
{
    return S32(st, counts + 4u * (uint32_t) idx);
}

int market_listed(int32_t value, const int32_t *list)
{
    /* @bin 0x00455CA0 */
    for (int i = 0; i < 4; i++) {
        if (list[i] == value) {
            return 1;
        }
    }
    return 0;
}

void market_wanted_goods(game_state *st, int32_t id, uint32_t counts, int32_t *out)
{
    /* @bin 0x00455E00 */
    int n = 0;
    out[0] = out[1] = out[2] = out[3] = -1;
    for (int32_t i = 0; i < S32(st, GOODS_COUNT); i++) {
        int32_t good = S32(st, GOODS_LIST + 4u * (uint32_t) i);
        if (good == 0) {
            continue;
        }
        int32_t idx = market_good_index(good);
        if (stock(st, id, idx) != 0 || count_of(st, counts, idx) == 0) {
            continue;
        }
        out[n++] = idx;
        if (n >= 4) {
            return;
        }
    }
    for (int32_t i = 0; i < S32(st, GOODS_COUNT); i++) {
        int32_t good = S32(st, GOODS_LIST + 4u * (uint32_t) i);
        if (good == 0) {
            continue;
        }
        int32_t idx = market_good_index(good);
        /* @bin 0x00455F89 below 600 (signed) */
        if (count_of(st, counts, idx) == 0 || stock(st, id, idx) >= 600 || market_listed(idx, out)) {
            continue;
        }
        out[n++] = idx;
        if (n >= 4) {
            return;
        }
    }
}

void market_wanted_goods_8_11(game_state *st, int32_t id, uint32_t counts, int32_t *out)
{
    /* @bin 0x00456070 goods 8..11: first those with no stock, then those below 100 */
    int n = 0;
    out[0] = out[1] = out[2] = out[3] = -1;
    for (int32_t g = 8; g <= 11; g++) {
        if (stock(st, id, g) == 0 && count_of(st, counts, g) != 0) {
            out[n++] = g;
        }
    }
    if (n >= 4) {
        return; /* @bin 0x00456102 checked after good 11 only (at most 4 by then) */
    }
    for (int32_t g = 8; g <= 11; g++) {
        if (count_of(st, counts, g) != 0 && stock(st, id, g) < 100 && !market_listed(g, out)) {
            out[n++] = g;
            if (n >= 4) {
                return;
            }
        }
    }
}

int32_t market_pick_good(game_state *st, int32_t id, uint32_t counts, uint32_t targets)
{
    /* @bin 0x004561E0 */
    for (int32_t i = 0; i < S32(st, GOODS_COUNT); i++) {
        int32_t good = S32(st, GOODS_LIST + 4u * (uint32_t) i);
        if (good == 0) {
            continue;
        }
        int32_t idx = market_good_index(good);
        /* @bin 0x0045628F a good with no stock and a source: taken at once, +0xAC unchanged */
        if (stock(st, id, idx) == 0 && count_of(st, counts, idx) != 0) {
            return S32(st, targets + 4u * (uint32_t) idx);
        }
    }
    int32_t best = -1;
    int32_t lowest = 50; /* @bin 0x004561F7 */
    int32_t n = S32(st, GOODS_COUNT);
    for (int32_t i = 0; i < n; i++) {
        int32_t good = S32(st, GOODS_LIST + 4u * (uint32_t) i);
        if (good == 0) {
            continue;
        }
        int32_t idx = market_good_index(good);
        if (count_of(st, counts, idx) != 0 && stock(st, id, idx) < lowest) {
            lowest = stock(st, id, idx);
            best = idx;
        }
    }
    if (best == -1) {
        n = S32(st, GOODS_COUNT);
        for (int32_t i = 0; i < n; i++) {
            int32_t good = S32(st, GOODS_LIST + 4u * (uint32_t) i);
            if (good == 0) {
                continue;
            }
            int32_t idx = market_good_index(good);
            /* @bin 0x004563EC the last one below 600 */
            if (count_of(st, counts, idx) != 0 && stock(st, id, idx) < 600) {
                best = idx;
            }
        }
    }
    for (int32_t i = 0; i < S32(st, GOODS_COUNT); i++) {
        int32_t good = S32(st, GOODS_LIST + 4u * (uint32_t) i);
        if (good == 0) {
            continue;
        }
        int32_t idx = market_good_index(good);
        if (best == idx) {
            U8(st, BUILDING_ADDR(id) + 0xACu) = (uint8_t) idx;
            return S32(st, targets + 4u * (uint32_t) idx);
        }
    }
    return 0;
}

int32_t market_pick_good_8_11(game_state *st, int32_t id, uint32_t counts, uint32_t targets)
{
    /* @bin 0x004565F0 */
    uint32_t b = BUILDING_ADDR(id);
    for (int32_t g = 8; g <= 11; g++) {
        if (stock(st, id, g) == 0 && count_of(st, counts, g) != 0) {
            U8(st, b + 0xACu) = (uint8_t) g;
            return S32(st, targets + 4u * (uint32_t) g);
        }
    }
    int32_t best = 0;
    int32_t lowest = 100;
    for (int32_t g = 8; g <= 11; g++) {
        if (count_of(st, counts, g) != 0 && stock(st, id, g) < lowest) {
            lowest = stock(st, id, g);
            best = g;
        }
    }
    if (best == 0) {
        return 0;
    }
    U8(st, b + 0xACu) = (uint8_t) best;
    return S32(st, targets + 4u * (uint32_t) best);
}

/* @bin 0x004570BB the distances between the two buildings (kept in 0x00D3B4FC, 0x00D3B4F8): is the
 * larger one below 40 */
static int near(game_state *st, uint32_t p, uint32_t s)
{
    int32_t px = S16(st, p + 6), sx = S16(st, s + 6), py = S16(st, p + 8), sy = S16(st, s + 8);
    int32_t dx = px > sx ? px - sx : px < sx ? sx - px : 0;
    int32_t dy = py > sy ? py - sy : py < sy ? sy - py : 0;
    S32(st, 0x00D3B4FCu) = dx;
    S32(st, 0x00D3B4F8u) = dy;
    return (dx < dy ? dy : dx) < 40;
}

/* @bin 0x00457228 candidate building `sid` for good index `idx`: its road tile (+0x26, +0x28) */
static void add_candidate(game_state *st, int32_t idx, int32_t sid)
{
    uint32_t s = BUILDING_ADDR(sid);
    int32_t n = S32(st, COUNTS + 4u * (uint32_t) idx);
    uint32_t slot = (uint32_t) (idx * CAND_ROW + n);
    S32(st, CAND_BUILDING + 4u * slot) = sid;
    S32(st, CAND_TILE + 4u * slot) = S32(st, MAP_START) + S16(st, s + 0x26) + S16(st, s + 0x28) * GRID_SIZE;
    S32(st, COUNTS + 4u * (uint32_t) idx) = n + 1;
}

/* @bin 0x004571AA, 0x00457527 a good the city may not trade (0x00EA42F8, i32 per good) */
static int good_blocked(game_state *st, int32_t good)
{
    return S32(st, 0x00EA42F8u + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) good)) != 0;
}

int32_t market_find_source(game_state *st, int32_t id, int kind, int32_t *goods, uint8_t *flood)
{
    /* @bin 0x00456FC0 */
    uint32_t p = BUILDING_ADDR(id);
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
            /* @bin 0x00457403 */
            if (S32(st, 0x00784BBCu) != 0 || U8(st, s + 0x44) == 0 || S16(st, s + 0x20) <= 0 ||
                U8(st, s + 0x14) != U8(st, p + 0x14) || !near(st, p, s)) {
                continue;
            }
            for (int32_t good = 1; good <= 8; good++) {
                int32_t idx = good - 1;
                if (good_blocked(st, good) || S16(st, s + 0x56u + 2u * (uint32_t) good) == 0 ||
                    U8(st, kinds + (uint32_t) good) == 1 || stock(st, id, idx) >= 20000) {
                    continue;
                }
                add_candidate(st, idx, sid);
            }
        } else if (type == 0x48 || type == 0xC1) {
            /* @bin 0x0045709F */
            if (U8(st, s + 0x44) == 0 || S16(st, s + 0x20) <= 0 || !near(st, p, s)) {
                continue;
            }
            for (int32_t good = 1; good <= 8; good++) {
                int32_t idx = good - 1;
                if (good_blocked(st, good) || GAME_CALL(st, 0x0045D510u, sid, good) <= 0 ||
                    U8(st, kinds + (uint32_t) good) == 1 || stock(st, id, idx) >= 20000) {
                    continue;
                }
                /* @bin 0x00457210 the city's setting for the good (0x00EA4096, i16) is 1 or 3 */
                int16_t v = S16(st, 0x00EA4096u + 2u * (U32(st, CURRENT_PLAYER_ADDR) * 0x24ECu + (uint32_t) good));
                if (v != 1 && v != 3) {
                    continue;
                }
                add_candidate(st, idx, sid);
            }
            /* @bin 0x00457283 goods 0xD, 0x13, 0x11, 0xF (indexes 9, 11, 10, 8), without that setting */
            static const int32_t pairs[4][2] = {{9, 0x0D}, {11, 0x13}, {10, 0x11}, {8, 0x0F}};
            for (int k = 0; k < 4; k++) {
                int32_t idx = pairs[k][0], good = pairs[k][1];
                if (good_blocked(st, good) || GAME_CALL(st, 0x0045D510u, sid, good) <= 0 ||
                    U8(st, kinds + (uint32_t) good) == 1 || stock(st, id, idx) >= 20000) {
                    continue;
                }
                add_candidate(st, idx, sid);
            }
        }
    }
    /* @bin 0x004575FD four delays (+0x9E..+0xA4) count down; while one is 0 its good index (9, 11,
     * 10, 8) has no candidate */
    static const int32_t delayed[4] = {9, 11, 10, 8};
    for (int k = 0; k < 4; k++) {
        uint32_t a = p + 0x9Eu + 2u * (uint32_t) k;
        int16_t v = S16(st, a);
        if (v > 0) {
            S16(st, a) = (int16_t) (v - 1);
        } else {
            S32(st, COUNTS + 4u * (uint32_t) delayed[k]) = 0;
        }
    }
    if (*flood == 0) {
        for (int idx = 0; idx < NGOODS; idx++) {
            if (S32(st, COUNTS + 4u * (uint32_t) idx) != 0) {
                route_flood_ee0c50(st, S16(st, p + 0x26), S16(st, p + 0x28));
                *flood = 1;
                break;
            }
        }
    }
    /* @bin 0x004576C5 the candidate with the smallest distance > 0 (the first on equality) */
    for (int32_t idx = 0; idx < NGOODS; idx++) {
        int32_t n = S32(st, COUNTS + 4u * (uint32_t) idx);
        if (n == 0) {
            continue;
        }
        int32_t best = 0, lowest = 9999;
        for (int32_t j = 0; j < n; j++) {
            int32_t tile = S32(st, CAND_TILE + 4u * (uint32_t) (idx * CAND_ROW + j));
            int32_t d = S32(st, ROUTE_DISTANCE + 4u * (uint32_t) tile);
            if (d > 0 && d < lowest) {
                lowest = d;
                best = j + 1;
            }
        }
        if (best != 0) {
            int32_t sid = S32(st, CAND_BUILDING + 4u * (uint32_t) (idx * CAND_ROW + best - 1));
            S32(st, TARGETS + 4u * (uint32_t) idx) = GAME_CALL(st, 0x00475B50u, sid);
        } else {
            S32(st, COUNTS + 4u * (uint32_t) idx) = 0;
        }
    }
    if ((uint8_t) kind == 0) {
        market_wanted_goods(st, id, COUNTS, goods);
        return market_pick_good(st, id, COUNTS, TARGETS);
    }
    market_wanted_goods_8_11(st, id, COUNTS, goods);
    return market_pick_good_8_11(st, id, COUNTS, TARGETS);
}

static int32_t run_pick_good(game_state *st, const int32_t *a)
{
    return market_pick_good(st, a[0], (uint32_t) a[1], (uint32_t) a[2]);
}

static int32_t run_pick_good_8_11(game_state *st, const int32_t *a)
{
    return market_pick_good_8_11(st, a[0], (uint32_t) a[1], (uint32_t) a[2]);
}

/* the functions that take a caller's stack buffer are ported inside their callers (run NULL) */
const ported_function bigb_market_ports[] = {
    {0x00455CA0u, "market_listed (in market.c)", 3, 8, 0, NULL, 0},
    {0x00455E00u, "market_wanted_goods (in market.c)", 3, 0, 0, NULL, 0},
    {0x00456070u, "market_wanted_goods_8_11 (in market.c)", 3, 0, 0, NULL, 0},
    {0x004561E0u, "market_pick_good", 3, 32, 0, run_pick_good, 0},
    {0x004565F0u, "market_pick_good_8_11", 3, 32, 0, run_pick_good_8_11, 0},
    {0x00456FC0u, "market_find_source (in market.c)", 4, 32, 0, NULL, 0},
};
const int bigb_market_ports_count = sizeof(bigb_market_ports) / sizeof(bigb_market_ports[0]);
