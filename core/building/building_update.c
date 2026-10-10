/* Imhotep - the daily building update: every building spawns its walkers (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32), figure record: chunk 21.
 *
 * 0x00462180 is one loop over the buildings with a switch on the type (types 0x18 .. 0xE9); each
 * case is a function here. Most cases follow one pattern: no workers -> +0xC3 := 2; the walker of
 * slot +0x2A still out -> nothing; a road next to the building (0x00480180, its tile in
 * 0x00D3B4E4 / 0x00D3B4E0); few workers -> 0x00467220; the delay from the worker ratio
 * (0x004CDE60); the counter +0x32 past the delay -> a new figure (0x004B4800) at the road tile. */
#include "building/building_update.h"

#include <stdint.h>

#include "building/market.h"
#include "random/random.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define SKIP INT32_MIN

/* the original's stack frame: locals that live across the buildings of one call */
typedef struct {
    game_state *st;
    int32_t id;
    uint32_t b;
    uint8_t l49, l48, l47, l46, l45;
    int32_t l40, l3c, l38, l34, l30, l2c, l28, l24, l20, l1c, l18, l14;
    int32_t l10[4];
} upd;

static uint32_t fig_addr(int32_t f)
{
    return FIGURE_ADDR(f);
}

/* @bin 0x004623D1 no workers: +0xC3 := 2 */
static void flag_no_workers(upd *u)
{
    if (S16(u->st, u->b + BLD_SERVICE) <= 0) {
        U8(u->st, u->b + BLD_PROBLEM_OVERLAY) = 2;
    }
}

/* @bin 0x004623E1 the figure in `slot` (i16) is alive (+0xE != 0), of type `type` or 0, and belongs
 * to the building: 1; otherwise a positive slot is cleared */
static int fig_kept(upd *u, uint32_t slot, uint8_t type)
{
    game_state *st = u->st;
    int16_t f = S16(st, slot);
    if (f <= 0) {
        return 0;
    }
    uint32_t a = fig_addr(f);
    if (U8(st, a + FIG_STATE) != 0 && (U8(st, a + FIG_TYPE) == type || U8(st, a + FIG_TYPE) == 0) &&
        S16(st, a + FIG_HOME) == u->id) {
        return 1;
    }
    S16(st, slot) = 0;
    return 0;
}

/* @bin 0x00466446 a non-zero `slot` whose figure is not alive (+0xE == 1) with type `type` is cleared */
static void fig_slot_check(upd *u, uint32_t slot, uint8_t type)
{
    game_state *st = u->st;
    int16_t f = S16(st, slot);
    if (f == 0) {
        return;
    }
    uint32_t a = fig_addr(f);
    if (U8(st, a + FIG_STATE) != 1 || U8(st, a + FIG_TYPE) != type) {
        S16(st, slot) = 0;
    }
}

static int32_t road(upd *u)
{
    game_state *st = u->st;
    return GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, u->b + BLD_X), S16(st, u->b + BLD_Y), U8(st, u->b + BLD_SIZE));
}

static int32_t road3(upd *u)
{
    game_state *st = u->st;
    return GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, u->b + BLD_X), S16(st, u->b + BLD_Y), 3);
}

/* @bin 0x00462453 few workers (+0x18 at most `limit`) */
static void few_workers(upd *u, int16_t limit)
{
    if (S16(u->st, u->b + BLD_SERVICE) <= limit) {
        GAME_CALL(u->st, FN_BUILDING_SPAWN_TYPE5, u->id);
    }
}

/* @bin 0x00462466 the ratio of workers (+0x40) to the type's need */
static int32_t worker_ratio_of(game_state *st, uint32_t b)
{
    return GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, b + BLD_WORKERS),
                     S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) S16(st, b + BLD_TYPE)));
}

static int32_t worker_ratio(upd *u)
{
    return worker_ratio_of(u->st, u->b);
}

/* @bin 0x00462488 the value for a ratio: >= 100, >= 75, >= 50, >= 25, >= 1; SKIP below 1 */
static int32_t by_ratio(int32_t r, int32_t v100, int32_t v75, int32_t v50, int32_t v25, int32_t v1)
{
    return r >= 100 ? v100 : r >= 75 ? v75 : r >= 50 ? v50 : r >= 25 ? v25 : r >= 1 ? v1 : SKIP;
}

/* @bin 0x004624C6 the counter +0x32 goes up; past the delay */
static int counter_due(upd *u, int32_t delay)
{
    uint8_t c = (uint8_t) (U8(u->st, u->b + BLD_SPAWN_DELAY) + 1);
    U8(u->st, u->b + BLD_SPAWN_DELAY) = c;
    return (int32_t) c > delay;
}

/* @bin 0x004B4800 a new figure of `type` at (x, y) */
static int32_t create_at(upd *u, int32_t type, int32_t x, int32_t y, int32_t dir)
{
    return GAME_CALL(u->st, FN_FIGURE_CREATE, U8(u->st, u->b + BLD_OWNER), type, x, y, dir);
}

/* a new figure at the road tile found last */
static int32_t create(upd *u, int32_t type, int32_t dir)
{
    return create_at(u, type, S32(u->st, G_ROAD_ACCESS_X), S32(u->st, G_ROAD_ACCESS_Y), dir);
}

/* @bin 0x0046463B the figure belongs to the building; 0x004B1DF0 */
static void tail_link(upd *u, int32_t f)
{
    S16(u->st, fig_addr(f) + FIG_HOME) = (int16_t) u->id;
    GAME_CALL(u->st, FN_FIGURE_WALKER_START, f);
}

/* @bin 0x00463BFC slot +0x2A, action 8 */
static void tail_8(upd *u, int32_t f)
{
    S16(u->st, u->b + BLD_WALKER) = (int16_t) f;
    U8(u->st, fig_addr(f) + FIG_ACTION) = 8;
    S16(u->st, fig_addr(f) + FIG_HOME) = (int16_t) u->id;
}

/* @bin 0x00466271 slot +0x2A, action `action`, then 0x0046463B */
static void tail_action_link(upd *u, int32_t f, uint8_t action)
{
    S16(u->st, u->b + BLD_WALKER) = (int16_t) f;
    U8(u->st, fig_addr(f) + FIG_ACTION) = action;
    tail_link(u, f);
}

/* @bin 0x00466E15 slot +0x2A, action 10 */
static void tail_10(upd *u, int32_t f)
{
    S16(u->st, u->b + BLD_WALKER) = (int16_t) f;
    U8(u->st, fig_addr(f) + FIG_ACTION) = 10;
    S16(u->st, fig_addr(f) + FIG_HOME) = (int16_t) u->id;
}

enum { TAIL_8, TAIL_1 };

/* The common case: `check` (0: none) is the type of the slot +0x2A walker; `limit` the workers
 * threshold of 0x00467220; delays by worker ratio; a walker of type `type`, direction 0. */
static void service(upd *u, uint8_t check, int16_t limit, const int32_t d[5], int32_t type, int tail)
{
    flag_no_workers(u);
    if (check && fig_kept(u, u->b + BLD_WALKER, check)) {
        return;
    }
    if (!road(u)) {
        return;
    }
    few_workers(u, limit);
    int32_t delay = by_ratio(worker_ratio(u), d[0], d[1], d[2], d[3], d[4]);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    U8(u->st, u->b + BLD_SPAWN_DELAY) = 0;
    int32_t f = create(u, type, 0);
    if (tail == TAIL_8) {
        tail_8(u, f);
    } else {
        tail_action_link(u, f, 1);
    }
}

/* @bin 0x004626CF types 0x18 .. 0x1D: one figure 0x28 a day at most (0x0078D584) */
static void upd_0x18(upd *u)
{
    game_state *st = u->st;
    if (!road(u)) {
        return;
    }
    uint8_t c = (uint8_t) (U8(st, u->b + BLD_SPAWN_DELAY) + 1);
    U8(st, u->b + BLD_SPAWN_DELAY) = c;
    if (c <= 0x28 || S32(st, G_FIGURE_28_SENT) != 0) {
        return;
    }
    U8(st, u->b + BLD_SPAWN_DELAY) = 0;
    S32(st, G_FIGURE_28_SENT) = 1;
    int32_t f = create(u, 0x28, 4);
    U8(st, fig_addr(f) + FIG_ACTION) = 1;
    tail_link(u, f);
}

/* @bin 0x004644FB type 0x1E: walkers 0xF (slot +0x9E, if +0xA7) or else 0x10 (slot +0xA0, if +0xA8) */
static void upd_0x1e(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    flag_no_workers(u);
    if (GAME_CALL(st, FN_BUILDING_MAIN_PART, u->id) != u->id || !road(u)) {
        return;
    }
    few_workers(u, 0x32);
    if (S16(st, b + BLD_PERFORMERS) != 0 && S16(st, b + BLD_PERFORMERS + 2) != 0) {
        return;
    }
    int32_t delay = by_ratio(worker_ratio(u), 3, 7, 15, 29, 44);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    if (U8(st, b + BLD_SHOWS + 1) != 0 && S16(st, b + BLD_PERFORMERS) == 0) {
        U8(st, b + BLD_SPAWN_DELAY) = 0;
        int32_t f = create(u, 0xF, 0);
        S16(st, b + BLD_PERFORMERS) = (int16_t) f;
        U8(st, fig_addr(f) + FIG_ACTION) = 0xC;
        tail_link(u, f);
    } else if (U8(st, b + BLD_SHOWS + 2) != 0 && S16(st, b + BLD_PERFORMERS + 2) == 0) {
        U8(st, b + BLD_SPAWN_DELAY) = 0;
        int32_t f = create(u, 0x10, 0);
        S16(st, b + BLD_PERFORMERS + 2) = (int16_t) f;
        U8(st, fig_addr(f) + FIG_ACTION) = 0xC;
        tail_link(u, f);
    }
}

/* @bin 0x004646AC type 0x1F */
static void upd_0x1f(upd *u)
{
    game_state *st = u->st;
    flag_no_workers(u);
    if (GAME_CALL(st, FN_BUILDING_MAIN_PART, u->id) != u->id || fig_kept(u, u->b + BLD_WALKER, 0xF) || !road(u)) {
        return;
    }
    few_workers(u, 0x32);
    int32_t delay = by_ratio(worker_ratio(u), 3, 7, 15, 29, 44);
    if (delay == SKIP || !counter_due(u, delay) || U8(st, u->b + BLD_SHOWS + 1) == 0) {
        return;
    }
    U8(st, u->b + BLD_SPAWN_DELAY) = 0;
    tail_action_link(u, create(u, 0xF, 0), 0xC);
}

/* @bin 0x004623D1 type 0x20: walker 0x12, paid from the stock +0x3C (0x007063C4 a walker) */
static void upd_0x20(upd *u)
{
    game_state *st = u->st;
    flag_no_workers(u);
    if (fig_kept(u, u->b + BLD_WALKER, 0x12) || !road(u)) {
        return;
    }
    few_workers(u, 0x32);
    int32_t delay = by_ratio(worker_ratio(u), 7, 15, 30, 50, 80);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    int32_t cost = S32(st, G_TYPE_WALKER_COST + 0x28 * 0x20);
    int16_t s = S16(st, u->b + BLD_STORED);
    if (s < cost) {
        return;
    }
    S16(st, u->b + BLD_STORED) = (int16_t) (s - cost);
    U8(st, u->b + BLD_SPAWN_DELAY) = 0;
    tail_action_link(u, create(u, 0x12, 0), 0xC);
}

/* @bin 0x00464800 type 0x21: walkers 0xF, 0x10, 0x11 (slots +0x9E, +0xA0, +0xA2 if +0xA7..+0xA9) */
static void upd_0x21(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    flag_no_workers(u);
    if (GAME_CALL(st, FN_BUILDING_MAIN_PART, u->id) != u->id || !road(u)) {
        return;
    }
    few_workers(u, 0x32);
    if (S16(st, b + BLD_PERFORMERS) != 0 && S16(st, b + BLD_PERFORMERS + 2) != 0 && S16(st, b + BLD_PERFORMERS + 4) != 0) {
        return;
    }
    int32_t delay = by_ratio(worker_ratio(u), 6, 12, 20, 40, 70);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    for (uint32_t k = 0; k < 3; k++) {
        if (U8(st, b + BLD_SHOWS + 1 + k) != 0 && S16(st, b + BLD_PERFORMERS + 2 * k) == 0) {
            U8(st, b + BLD_SPAWN_DELAY) = 0;
            int32_t f = create(u, (int32_t) (0xF + k), 0);
            S16(st, b + BLD_PERFORMERS + 2 * k) = (int16_t) f;
            U8(st, fig_addr(f) + FIG_ACTION) = 0xC;
            tail_link(u, f);
        }
    }
}

/* @bin 0x004659FB types 0x33, 0x35, 0x2F: a walker paid from the stock, the cost (0x007066BC,
 * 0x0070670C, 0x0070661C) reduced by a percentage under a condition of the city */
static void upd_paid(upd *u, uint8_t type, int32_t cost_addr, int32_t mode, int32_t mode_arg, uint32_t pct_addr)
{
    game_state *st = u->st;
    flag_no_workers(u);
    if (fig_kept(u, u->b + BLD_WALKER, type) || !road(u)) {
        return;
    }
    few_workers(u, 0x32);
    int32_t delay = type == 0x22 ? by_ratio(worker_ratio(u), 3, 7, 15, 29, 44)
                                 : by_ratio(worker_ratio(u), 5, 10, 25, 50, 100);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    int32_t cost = S32(st, (uint32_t) cost_addr);
    uint32_t c = CURRENT_CITY(st);
    if (S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == mode &&
        (uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_U472C), mode_arg) != 0) {
        cost = calc_percentage(cost, S32(st, pct_addr)); /* 0x004CDE40 */
    }
    int16_t s = S16(st, u->b + BLD_STORED);
    if (s < cost) {
        return;
    }
    S16(st, u->b + BLD_STORED) = (int16_t) (s - cost);
    U8(st, u->b + BLD_SPAWN_DELAY) = 0;
    if (type == 0x1D && S32(st, G_CUSTOM) == 0 && S8(st, G_MISSION) == S32(st, G_MISSION_LISTS + 0x150) &&
        U8(st, G_MISSION_LIST_FLAGS + 0xC) == 0 && U8(st, G_MISSION_LIST_FLAGS + 0xB) != 0) {
        /* @bin 0x00465B86 */
        S32(st, G_U78D828) = 0;
        GAME_CALL(st, FN_TUTORIAL_STEP, 0xC);
    }
    tail_action_link(u, create(u, type, 0), 1);
}

/* @bin 0x004637AB types 0x3B, 0xAC, 0xAD */
static void upd_0x3b(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    flag_no_workers(u);
    if (!road(u)) {
        return;
    }
    few_workers(u, 0x32);
    if (S16(st, b + BLD_WORKERS) <= 0) {
        return;
    }
    if (S16(st, b + BLD_TOWER_WALKER) == 0 && S16(st, b + BLD_WALKER) != 0) {
        int32_t f = create_at(u, 0x3E, S16(st, b + BLD_X), S16(st, b + BLD_Y), 0);
        S16(st, b + BLD_TOWER_WALKER) = (int16_t) f;
        S16(st, fig_addr(f) + FIG_HOME) = (int16_t) u->id;
        U8(st, fig_addr(f) + FIG_ACTION) = 8;
    }
    fig_kept(u, b + BLD_WALKER, 0x2A);
    if (S16(st, b + BLD_WALKER) <= 0) {
        S32(st, G_TOWER_SENTRY_REQUEST) = 2;
    }
}

/* @bin 0x004656EE types 0x3C .. 0x45, 0x97, 0x9C: walker 0x1B, numbered per kind (0x004C5120) */
static void upd_0x3c(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    if (S16(st, b + BLD_PREV_PART) != 0) {
        return;
    }
    flag_no_workers(u);
    if (fig_kept(u, b + BLD_WALKER, 0x1B)) {
        return;
    }
    int32_t type = S16(st, b + BLD_TYPE);
    int32_t r = type >= 0x41 && type <= 0x45
                    ? GAME_CALL(st, FN_BUILDING_FIND_EDGE_TILE, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_ORIENTATION))
                    : road(u);
    if (!r) {
        return;
    }
    few_workers(u, 0x32);
    int32_t ratio = worker_ratio(u);
    int32_t delay = S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) S16(st, b + BLD_TYPE)) <= 0
                        ? 7 /* @bin 0x004657F8 no workers needed */
                        : by_ratio(ratio, 3, 7, 10, 15, 20);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    U8(st, b + BLD_SPAWN_DELAY) = 0;
    int32_t f = create(u, 0x1B, 0);
    uint32_t a = fig_addr(f);
    S16(st, b + BLD_WALKER) = (int16_t) f;
    U8(st, a + FIG_ACTION) = 1;
    S16(st, a + FIG_HOME) = (int16_t) u->id;
    uint32_t kind = (uint32_t) GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, (int32_t) (int16_t) u->id);
    if (kind <= 4) {
        /* @bin 0x004658C5 a counter per kind (0x00D056B0 down to 0x00D056A0), 0..15, and a value */
        static const uint32_t counters[5] = {G_NAME_COUNTER_10A, G_NAME_COUNTER_10A - 4, G_NAME_COUNTER_10A - 8, G_NAME_COUNTER_10A - 0xC, G_NAME_COUNTER_10A - 0x10};
        int32_t n = S32(st, counters[kind]);
        S16(st, a + FIG_NAME_INDEX) = (int16_t) n;
        n++;
        S32(st, a + FIG_NAME_GROUP) = 0x10A + (int32_t) kind;
        S32(st, counters[kind]) = n >= 16 ? 0 : n;
    }
    GAME_CALL(st, FN_FIGURE_WALKER_START, f);
}

/* @bin 0x00464A17 types 0x46, 0xBE: walker 0x1A, buyers 0x27 (slots +0x2C, +0xB6), walker 0x1A
 * (slot +0xB4) */
static void buyer(upd *u, uint32_t slot, int kind)
{
    game_state *st = u->st;
    if (S16(st, slot) != 0) {
        uint32_t a = fig_addr(S16(st, slot));
        if (U8(st, a + FIG_STATE) != 1 || (U8(st, a + FIG_TYPE) != 0x27 && U8(st, a + FIG_TYPE) != 5)) {
            S16(st, slot) = 0;
        }
        return;
    }
    /* @bin 0x00464C8E every 8 days, by the building's rank among these buildings */
    if (S32(st, G_TICK_DAY) % 8 != u->l1c % 8) {
        return;
    }
    road(u);
    int32_t dest = market_find_source(st, u->id, kind, u->l10, &u->l46);
    if (dest <= 0) {
        return;
    }
    int32_t f = create(u, 0x27, 0);
    uint32_t a = fig_addr(f);
    S16(st, slot) = (int16_t) f;
    U8(st, a + FIG_ACTION) = 8;
    S16(st, a + FIG_HOME) = (int16_t) u->id;
    S16(st, a + FIG_DEST_BUILDING) = (int16_t) dest;
    for (uint32_t i = 0; i < 4; i++) {
        S32(st, a + FIG_BUYERS_1 + 4 * i) = u->l10[i];
        S32(st, a + FIG_BUYERS_2 + 4 * i) = 0;
        S32(st, a + FIG_BUYERS_3 + 4 * i) = 0;
    }
    uint32_t d = BUILDING_ADDR(dest);
    if (GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, d + BLD_X), S16(st, d + BLD_Y), U8(st, d + BLD_SIZE)) ||
        GAME_CALL(st, FN_BUILDING_ROAD_ACCESS, S16(st, d + BLD_X), S16(st, d + BLD_Y), 3)) {
        S16(st, a + FIG_DEST_X) = S16(st, G_ROAD_ACCESS_X);
        S16(st, a + FIG_DEST_Y) = S16(st, G_ROAD_ACCESS_Y);
    } else {
        U8(st, a + FIG_ACTION) = 9;
        S16(st, a + FIG_DEST_X) = S16(st, a + FIG_X);
        S16(st, a + FIG_DEST_Y) = S16(st, a + FIG_Y);
    }
}

static void upd_0x46(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    u->l1c++;
    u->l10[0] = u->l10[1] = u->l10[2] = u->l10[3] = -1;
    u->l45 = (uint8_t) GAME_CALL(st, FN_BUILDING_MARK_TILES_BY_LEVEL, u->id);
    flag_no_workers(u);
    if (!road(u)) {
        return;
    }
    few_workers(u, 0x32);
    int32_t delay = by_ratio(worker_ratio(u), 2, 3, 4, 5, 10);
    if (delay == SKIP) {
        return;
    }
    int32_t sum = 0;
    int32_t n = S32(st, G_GOODS_COUNT);
    for (int32_t i = 0; i < n; i++) {
        int32_t good = S32(st, G_GOODS_LIST + 4u * (uint32_t) i);
        if (good != 0) {
            sum += S16(st, b + BLD_INVENTORY + 2u * (uint32_t) market_good_index(good));
        }
    }
    u->l38 = S16(st, b + BLD_INVENTORY + 2 * 9) + sum + S16(st, b + BLD_INVENTORY + 2 * 8) + S16(st, b + BLD_INVENTORY + 2 * 10) + S16(st, b + BLD_INVENTORY + 2 * 11);
    if (fig_kept(u, b + BLD_WALKER, 0x1A) || u->l38 == 0) {
        GAME_CALL(st, FN_BUILDING_HOUSES_NEARBY, u->id, 10);
    } else {
        if (!counter_due(u, delay)) {
            return;
        }
        U8(st, b + BLD_SPAWN_DELAY) = 0;
        tail_action_link(u, create(u, 0x1A, 0), 1);
    }
    u->l46 = 0;
    buyer(u, b + BLD_WALKER2, 0);
    buyer(u, b + BLD_SECOND_CART, 1);
    if (u->l45 == 0) {
        return;
    }
    if (S16(st, b + BLD_SECOND_WALKER) == 0 && u->l38 != 0) {
        int32_t f = create(u, 0x1A, 0);
        S16(st, fig_addr(f) + FIG_HOME) = (int16_t) u->id;
        S16(st, b + BLD_SECOND_WALKER) = (int16_t) f;
        U8(st, fig_addr(f) + FIG_ACTION) = 1;
        GAME_CALL(st, FN_FIGURE_WALKER_START, f);
        return;
    }
    uint32_t a = fig_addr(S16(st, b + BLD_SECOND_WALKER));
    if (U8(st, a + FIG_STATE) == 1 && (U8(st, a + FIG_TYPE) == 0x1A || U8(st, a + FIG_TYPE) == 5)) {
        return;
    }
    S16(st, b + BLD_SECOND_WALKER) = 0;
}

/* @bin 0x0046325F / 0x004635DF the most a cart takes: 400 for the goods of kind 3 (0x007900C2)
 * other than 0x18, 0x19, 0x1A, 0x1E, 0x23, 0xC, 0x1C, 0xA; else 100 */
static int32_t cart_limit(uint8_t kind, int32_t good)
{
    if (kind != 3) {
        return 100;
    }
    switch (good) {
    case 0x18: case 0x19: case 0x1A: case 0x1E: case 0x23: case 0x0C: case 0x1C: case 0x0A:
        return 100;
    default:
        return 400;
    }
}

/* @bin 0x00462FCF types 0x48, 0xC1 (and @bin 0x004633A0 types 0x47, 0xBF with is48 == 0): cart
 * pushers 9 (slots +0x2A, +0xB6) */
static void upd_cart(upd *u, int is48)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    int32_t id = u->id;
    u->l3c = 0;
    u->l28 = 100;
    u->l2c = 100;
    u->l34 = 0;
    if (!is48) {
        u->l30 = 0;
        u->l20 = 0;
    }
    int f1 = fig_kept(u, b + BLD_WALKER, 9);
    int f2 = fig_kept(u, b + BLD_SECOND_CART, 9);
    u->l40 = f1;
    u->l38 = f2;
    flag_no_workers(u);
    int32_t r;
    if (is48) {
        /* @bin 0x004630CE the flag goes to the 8 linked parts (+0x3A) */
        int32_t cur = id;
        for (int i = 0; i < 8; i++) {
            cur = S16(st, BUILDING_ADDR(cur) + BLD_NEXT_PART);
            if (cur != 0) {
                U8(st, BUILDING_ADDR(cur) + BLD_PROBLEM_OVERLAY) = U8(st, b + BLD_PROBLEM_OVERLAY);
            }
        }
        r = road(u) || road3(u);
    } else {
        r = GAME_CALL(st, FN_BUILDING_ROAD_ACCESS_4, S16(st, b + BLD_X), S16(st, b + BLD_Y));
    }
    if (!r) {
        return;
    }
    few_workers(u, 0x64);
    if (f1 && f2) {
        return;
    }
    int32_t rx = S32(st, G_ROAD_ACCESS_X), ry = S32(st, G_ROAD_ACCESS_Y);
    building_lowest_goods(st, id, &u->l3c, &u->l34, &u->l28, &u->l2c, &u->l30, &u->l20);
    int32_t good = GAME_CALL(st, is48 ? FN_STORAGE_CART_GOOD : FN_BUILDING_CART_GOOD, id, u->l3c);
    S32(st, G_ROAD_ACCESS_X) = rx;
    S32(st, G_ROAD_ACCESS_Y) = ry;
    if (good <= -1) {
        return;
    }
    if (good != 0) {
        if (u->l40 != 0) {
            return;
        }
        int32_t f = create_at(u, 9, S16(st, b + BLD_ROAD_ACCESS_X), S16(st, b + BLD_ROAD_ACCESS_Y), 4);
        uint32_t a = fig_addr(f);
        uint8_t kind = U8(st, G_STORAGE_ORDERS + 0xC4u * U8(st, b + BLD_STORAGE_ID) + (uint32_t) good);
        U8(st, a + FIG_ACTION) = 8;
        U8(st, a + FIG_RESOURCE) = (uint8_t) good;
        int32_t amount;
        if (is48) {
            u->l38 = cart_limit(kind, good);
            int32_t have = GAME_CALL(st, FN_BUILDING_PARTS_STOCK, id, good);
            amount = u->l38 < have ? u->l38 : GAME_CALL(st, FN_BUILDING_PARTS_STOCK, id, good);
        } else {
            int32_t limit = cart_limit(kind, good);
            int32_t have = S16(st, b + BLD_INVENTORY + 2u * (uint32_t) good);
            amount = limit < have ? limit : have;
        }
        S16(st, a + FIG_AMOUNT) = (int16_t) amount;
        S16(st, a + FIG_HOME) = (int16_t) id;
        S16(st, b + BLD_WALKER) = (int16_t) f;
        GAME_CALL(st, is48 ? FN_STORAGE_TAKE : FN_STORAGE_TAKE_STOCK, S32(st, CURRENT_PLAYER_ADDR), id, U8(st, a + FIG_RESOURCE),
                  S16(st, a + FIG_AMOUNT));
        return;
    }
    int32_t n = GAME_CALL(st, FN_BUILDING_COUNT_NONZERO, u->l3c, u->l34);
    if (n == 0) {
        return;
    }
    building_ratio_flags(u->l28, u->l2c, n, &u->l48, &u->l49, &u->l24);
    if (u->l24 == 0) {
        return;
    }
    int second;
    if (u->l48 == 0) {
        if (u->l40 != 0) {
            return;
        }
        second = 0;
    } else {
        second = u->l40 != 0;
    }
    if (!second) {
        /* @bin 0x00463332 */
        if (u->l30 == 0) {
            return;
        }
        int32_t f = create_at(u, 9, S16(st, b + BLD_ROAD_ACCESS_X), S16(st, b + BLD_ROAD_ACCESS_Y), 4);
        uint32_t a = fig_addr(f);
        S16(st, b + BLD_WALKER) = (int16_t) f;
        U8(st, a + FIG_ACTION) = 8;
        U8(st, a + FIG_RESOURCE) = 0;
        U8(st, a + FIG_COLLECTING) = (uint8_t) u->l3c;
        S16(st, a + FIG_AMOUNT) = 0;
        S16(st, a + FIG_HOME) = (int16_t) id;
        return;
    }
    /* @bin 0x0046370A */
    if (u->l38 != 0) {
        return;
    }
    if (u->l49 != 0 ? u->l30 == 0 : u->l20 == 0) {
        return;
    }
    int32_t f = create_at(u, 9, S16(st, b + BLD_ROAD_ACCESS_X), S16(st, b + BLD_ROAD_ACCESS_Y), 4);
    uint32_t a = fig_addr(f);
    U8(st, a + FIG_ACTION) = 8;
    U8(st, a + FIG_RESOURCE) = 0;
    U8(st, a + FIG_COLLECTING) = (uint8_t) (u->l49 != 0 ? u->l3c : u->l34);
    S16(st, a + FIG_AMOUNT) = 0;
    S16(st, b + BLD_SECOND_CART) = (int16_t) f;
    S16(st, a + FIG_HOME) = (int16_t) id;
}

/* @bin 0x004666CE type 0x4A */
static void upd_0x4a(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    flag_no_workers(u);
    if (!road(u)) {
        return;
    }
    few_workers(u, 0x32);
    uint16_t req = U16(st, b + BLD_REQUEST);
    uint32_t mode = req & 0xF000u;
    if (mode == 0) {
        /* @bin 0x00466A00 a new request */
        if (building_pending_request(st, u->id, &u->l18)) {
            U16(st, b + BLD_REQUEST) = (uint16_t) (u->l18 | 0x2000);
            return;
        }
        if (!building_missing_worker(st, &u->l18)) {
            return;
        }
        if ((u->l18 == 0x4E || u->l18 == 0x4D) && S16(st, b + BLD_STORED) < 100) {
            return;
        }
        U16(st, b + BLD_REQUEST) = (uint16_t) (u->l18 | 0x1000);
        return;
    }
    if (mode == 0x1000) {
        u->l45 = 1;
    } else if (mode == 0x2000) {
        u->l45 = 0;
    } else {
        S16(st, b + BLD_REQUEST) = 0;
        return;
    }
    int32_t r = worker_ratio(u);
    /* @bin 0x00466774 */
    int32_t step = r >= 100 ? 10 : r >= 75 ? 8 : r >= 50 ? 6 : r >= 25 ? 4 : r <= 0 ? 0 : 2;
    if (u->l45 == 0) {
        /* @bin 0x00466945 */
        uint32_t a = fig_addr(U16(st, b + BLD_REQUEST) & 0xFFF);
        if (U8(st, a + FIG_STATE) == 2) {
            S16(st, b + BLD_REQUEST) = 0;
            return;
        }
        int16_t s = S16(st, b + BLD_STORED);
        int32_t free_mode = S32(st, G_FREE_BUILD);
        if (s < 1 && free_mode == 0) {
            return;
        }
        int32_t v = S32(st, a + FIG_HIT_POINTS) + step;
        S32(st, a + FIG_HIT_POINTS) = v;
        if (free_mode == 0) {
            S16(st, b + BLD_STORED) = (int16_t) (s - 1);
        }
        if (v < S32(st, a + FIG_HIT_POINTS_MAX)) {
            return;
        }
        S32(st, a + FIG_HIT_POINTS) = S32(st, a + FIG_HIT_POINTS_MAX);
        S16(st, b + BLD_REQUEST) = 0;
        U8(st, a + FIG_ACTION) = 8;
        return;
    }
    int32_t type = U16(st, b + BLD_REQUEST) & 0xFFF;
    u->l38 = GAME_CALL(st, FN_BUILDING_FIND_MISSING_WORKER, type);
    if (u->l38 == 0) {
        S16(st, b + BLD_REQUEST) = 0;
        S16(st, b + BLD_U54) = 0;
        return;
    }
    if (S16(st, b + BLD_STORED) < 100 && S32(st, G_FREE_BUILD) == 0 && type != 0x19) {
        step = 0;
        if (building_missing_worker(st, &u->l14) && u->l14 == 0x19) {
            S16(st, b + BLD_REQUEST) = 0;
            S16(st, b + BLD_U54) = 0;
        }
    }
    S16(st, b + BLD_U54) = (int16_t) (S16(st, b + BLD_U54) + step);
    if (S16(st, b + BLD_U54) < 0xA0) {
        return;
    }
    S16(st, b + BLD_U54) = 0;
    if (!GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE))) {
        return;
    }
    int32_t f = create(u, type, 0);
    uint32_t a = fig_addr(f);
    if (type == 0x19) {
        U8(st, a + FIG_ACTION) = 8;
        S32(st, a + FIG_U124) = 0;
        S32(st, a + FIG_HIT_POINTS) = S32(st, G_FIGURE_TYPES + 0x38 * 0x19);
        S32(st, a + FIG_HIT_POINTS_MAX) = S32(st, G_FIGURE_TYPES + 0x38 * 0x19);
    } else {
        S16(st, b + BLD_STORED) = (int16_t) (S16(st, b + BLD_STORED) - 100);
        U8(st, a + FIG_ACTION) = 8;
        GAME_CALL(st, type == 0x4D ? FN_FIGURE_INIT_TYPE_VALUES : FN_FIGURE_INIT_TYPE_VALUES_FULL, f);
    }
    S16(st, a + FIG_HOME) = (int16_t) u->l38;
    S16(st, BUILDING_ADDR(u->l38) + BLD_SECOND_CART) = (int16_t) f;
    S16(st, b + BLD_REQUEST) = 0;
}

/* @bin 0x00466294 types 0x4B, 0xC0: up to 3 walkers 0x26 (slots +0xAE..+0xB2) by worker ratio */
static void upd_0x4b(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    flag_no_workers(u);
    if (!road(u)) {
        return;
    }
    few_workers(u, 0x32);
    int32_t r = worker_ratio(u);
    u->l38 = r <= 0 ? 0 : r < 50 ? 1 : 2 + (r >= 75);
    int32_t n = 0;
    for (uint32_t k = 0; k < 3; k++) {
        int16_t f = S16(st, b + BLD_DOCKERS + 2 * k);
        if (f == 0) {
            continue;
        }
        uint32_t a = fig_addr(f);
        if (U8(st, a + FIG_TYPE) == 0x26 && S16(st, a + FIG_HOME) == u->id) {
            n++;
        } else {
            S16(st, b + BLD_DOCKERS + 2 * k) = 0;
        }
    }
    if (n > u->l38) {
        for (int k = 2; k >= 0; k--) {
            int16_t f = S16(st, b + BLD_DOCKERS + 2u * (uint32_t) k);
            if (f != 0) {
                U8(st, fig_addr(f) + FIG_STATE) = 2;
                break;
            }
        }
        return;
    }
    if (n >= u->l38) {
        return;
    }
    int32_t f = create(u, 0x26, 4);
    U8(st, fig_addr(f) + FIG_ACTION) = 8;
    S16(st, fig_addr(f) + FIG_HOME) = (int16_t) u->id;
    for (uint32_t k = 0; k < 3; k++) {
        if (S16(st, b + BLD_DOCKERS + 2 * k) == 0) {
            S16(st, b + BLD_DOCKERS + 2 * k) = (int16_t) f;
            break;
        }
    }
}

/* @bin 0x004664FC type 0x4C */
static void upd_0x4c(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    flag_no_workers(u);
    fig_slot_check(u, b + BLD_DOCK_BOAT, 0x19);
    if (!road(u)) {
        return;
    }
    few_workers(u, 0x32);
    if (fig_kept(u, b + BLD_WALKER, 4) || U8(st, b + BLD_SPAWN_DELAY) == 0) {
        return;
    }
    int32_t x = S32(st, G_ROAD_ACCESS_X);
    U8(st, b + BLD_SPAWN_DELAY) = 0;
    S16(st, b + BLD_U64) = 0;
    S32(st, CURRENT_CITY(st) + CITY_U4780) += 100;
    U8(st, b + BLD_OUTPUT_RESOURCE) = 7;
    int32_t f = create_at(u, 4, x, S32(st, G_ROAD_ACCESS_Y), 4);
    uint32_t a = fig_addr(f);
    uint32_t c = CURRENT_CITY(st);
    U8(st, a + FIG_ACTION) = 8;
    S16(st, a + FIG_AMOUNT) = 100;
    if (S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == 0 &&
        (uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_U472C), 2) != 0) {
        S16(st, a + FIG_AMOUNT) = (int16_t) (S16(st, a + FIG_AMOUNT) << 1);
    }
    U8(st, a + FIG_RESOURCE) = 7;
    S16(st, b + BLD_WALKER) = (int16_t) f;
    S16(st, a + FIG_HOME) = (int16_t) u->id;
    S16(st, a + FIG_WAIT_TICKS) = 0x1E;
}

/* @bin 0x004661A6 type 0x50 */
static void upd_0x50(upd *u)
{
    game_state *st = u->st;
    if (fig_kept(u, u->b + BLD_WALKER, 0x40) || !road(u) || S32(st, CURRENT_CITY(st) + CITY_POPULATION) <= 0) {
        return;
    }
    S32(st, CURRENT_CITY(st) + CITY_U459C) = 1;
    uint8_t c = (uint8_t) (U8(st, u->b + BLD_SPAWN_DELAY) + 1);
    U8(st, u->b + BLD_SPAWN_DELAY) = c;
    if (c <= 1) {
        return;
    }
    U8(st, u->b + BLD_SPAWN_DELAY) = 0;
    tail_action_link(u, create(u, 0x40, 0), 1);
}

/* @bin 0x00463FB7 types 0x54 .. 0x57, 0xBB .. 0xBD: walker 7 (types 0x56, 0x57 only) while the
 * building of city +0x2790 has workers */
static void upd_0x54(upd *u)
{
    game_state *st = u->st;
    flag_no_workers(u);
    if (fig_kept(u, u->b + BLD_WALKER, 7) || !road(u)) {
        return;
    }
    few_workers(u, 0x32);
    int32_t delay = by_ratio(worker_ratio(u), 0, 1, 3, 7, 15);
    if (delay == SKIP) {
        return;
    }
    int32_t other = worker_ratio_of(st, BUILDING_ADDR(S32(st, CURRENT_CITY(st) + CITY_U2790)));
    if (!counter_due(u, delay)) {
        return;
    }
    U8(st, u->b + BLD_SPAWN_DELAY) = 0;
    int16_t type = S16(st, u->b + BLD_TYPE);
    if ((type == 0x56 || type == 0x57) && other > 0) {
        tail_8(u, create(u, 7, 0));
    }
}

/* @bin 0x00466C51 type 0x58 */
static void upd_0x58(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    int32_t g = S32(st, b + BLD_GRID_OFFSET);
    S32(st, G_GRID_IMAGE + 4u * (uint32_t) g) = (U8(st, G_GRID_RANDOM + (uint32_t) g) & 1) + S32(st, G_IMAGE_GROUPS + 8 * 452);
    if (fig_kept(u, b + BLD_WALKER, 0x29) || S16(st, b + BLD_SUBTYPE) <= 0) {
        return;
    }
    if (!GAME_CALL(st, FN_BUILDING_FIND_OPEN_TILE, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE))) {
        return;
    }
    uint8_t c = (uint8_t) (U8(st, b + BLD_SPAWN_DELAY) + 1);
    U8(st, b + BLD_SPAWN_DELAY) = c;
    if (c <= 4) {
        return;
    }
    U8(st, b + BLD_SPAWN_DELAY) = 0;
    tail_10(u, create(u, 0x29, 0));
}

/* @bin 0x00466D2C type 0x59 */
static void upd_0x59(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    GAME_CALL(st, FN_MAP_BUILDING_TILES, u->id, S16(st, b + BLD_X), S16(st, b + BLD_Y), 2, 2, S32(st, G_IMAGE_GROUPS + 8 * 452) + 2, 8);
    if (S32(st, CURRENT_CITY(st) + CITY_U459C) <= 0 || fig_kept(u, b + BLD_WALKER, 0x3A)) {
        return;
    }
    if (!GAME_CALL(st, FN_BUILDING_FIND_OPEN_TILE, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE))) {
        return;
    }
    uint8_t c = (uint8_t) (U8(st, b + BLD_SPAWN_DELAY) + 1);
    U8(st, b + BLD_SPAWN_DELAY) = c;
    if (c <= 8) {
        return;
    }
    U8(st, b + BLD_SPAWN_DELAY) = 0;
    tail_10(u, create(u, 0x3A, 0));
}

/* @bin 0x00466E5E type 0x5F */
static void upd_0x5f(upd *u)
{
    game_state *st = u->st;
    flag_no_workers(u);
    if (!road(u)) {
        return;
    }
    few_workers(u, 0x64);
    int32_t delay = by_ratio(worker_ratio(u), 8, 12, 16, 32, 48);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    U8(st, u->b + BLD_SPAWN_DELAY) = 0;
    road(u);
    if (GAME_CALL(st, FN_BUILDING_SEND_0X2A, u->id) == 0) {
        GAME_CALL(st, FN_BUILDING_RECRUIT, u->id);
    }
}

/* @bin 0x00462763 types 0x64 .. 0x72 (but 0x6C), 0xA1, 0xA2, ...: a cart 4 of the production */
static void upd_0x64(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    flag_no_workers(u);
    if (!road(u)) {
        return;
    }
    few_workers(u, 0x32);
    if (fig_kept(u, b + BLD_WALKER, 4)) {
        return;
    }
    int32_t amount;
    if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_IS_TYPE_0X487F80, u->id) != 0) {
        amount = GAME_CALLT(st, G_BUILDING_MANAGER, FN_FARM_HARVEST, u->id);
        if (amount == 0) {
            return;
        }
        u->l47 = 1; /* @bin 0x0046281C kept for the next buildings */
    } else {
        if (GAME_CALL(st, FN_BUILDING_PRODUCTION_READY, u->id) == 0) {
            return;
        }
        amount = 100;
    }
    GAME_CALL(st, FN_BUILDING_PRODUCTION_TAKE, u->id);
    int32_t f = create(u, 4, 4);
    uint32_t a = fig_addr(f);
    S16(st, b + BLD_WALKER) = (int16_t) f;
    U8(st, a + FIG_ACTION) = 8;
    S16(st, a + FIG_AMOUNT) = (int16_t) amount;
    U8(st, a + FIG_RESOURCE) = U8(st, b + BLD_OUTPUT_RESOURCE);
    S16(st, a + FIG_HOME) = (int16_t) u->id;
    S16(st, a + FIG_WAIT_TICKS) = 0x1E;
    if (u->l47 == 0 || S16(st, b + BLD_TYPE) != 0x66 || S16(st, CURRENT_CITY(st) + CITY_U28C8) != 0) {
        return;
    }
    f = create(u, 4, 4);
    a = fig_addr(f);
    S16(st, b + BLD_SECOND_CART) = (int16_t) f;
    U8(st, a + FIG_ACTION) = 8;
    S16(st, a + FIG_AMOUNT) = 100;
    U8(st, a + FIG_RESOURCE) = 9;
    S16(st, a + FIG_HOME) = (int16_t) u->id;
    S16(st, a + FIG_WAIT_TICKS) = 0x1E;
}

/* the game's random step, inline in 0x00462180 (0x00462B68): the 15-bit value of the first
 * generator after it */
static uint32_t random_step(game_state *st)
{
    random_next_state(st);
    return U32(st, G_R_1_15);
}

/* @bin 0x00462932 type 0x6C (good 0x14, workers 0x4B), @bin 0x00462C67 type 0xC3 (good 0x16,
 * workers 0x5A), @bin 0x004650C9 type 0x73 (good 8, workers 0x49): a cart 4 of the stock, and
 * workers up to a number from the worker ratio and the room left in the stock */
static void upd_gatherers(upd *u, int which)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    flag_no_workers(u);
    if (!road(u)) {
        return;
    }
    few_workers(u, which == 0x73 ? 0x64 : 0x32);
    if (which != 0x73 && GAME_CALL(st, FN_BUILDING_RESOURCE_IN_STOCK, u->id) != 0) {
        return;
    }
    int32_t r = worker_ratio(u);
    int32_t n = which == 0x6C   ? by_ratio(r, 3, 2, 2, 1, 1)
                : which == 0xC3 ? by_ratio(r, 5, 4, 2, 1, 1)
                                : by_ratio(r, 3, 2, 1, 1, 0);
    if (n == SKIP) {
        return;
    }
    u->l38 = n;
    if (!fig_kept(u, b + BLD_WALKER, 4)) {
        int16_t s = S16(st, b + BLD_STORED);
        if (which == 0x6C ? s >= 100 : which == 0xC3 ? s > 0x32 : s > 0) {
            int32_t f = create(u, 4, 4);
            uint32_t a = fig_addr(f);
            if (which == 0x73) {
                /* @bin 0x004651F0 */
                uint32_t c = CURRENT_CITY(st);
                S32(st, c + CITY_U4774) += 100;
                S16(st, b + BLD_WALKER) = (int16_t) f;
                U8(st, a + FIG_ACTION) = 8;
                U8(st, a + FIG_RESOURCE) = 8;
                S16(st, a + FIG_HOME) = (int16_t) u->id;
                S16(st, a + FIG_WAIT_TICKS) = 0x1E;
                S16(st, a + FIG_AMOUNT) = 100;
                if (S16(st, c + CITY_U4478) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) == 0 &&
                    (uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_U472C), 2) != 0) {
                    S16(st, a + FIG_AMOUNT) = (int16_t) (S16(st, a + FIG_AMOUNT) << 1);
                }
                S16(st, b + BLD_STORED) = (int16_t) (S16(st, b + BLD_STORED) - 100);
                U8(st, b + BLD_OUTPUT_RESOURCE) = 8;
            } else {
                S16(st, b + BLD_WALKER) = (int16_t) f;
                S16(st, b + BLD_STORED) = (int16_t) (S16(st, b + BLD_STORED) - 100);
                if (which == 0x6C) {
                    U8(st, b + BLD_OUTPUT_RESOURCE) = 0x14;
                }
                U8(st, a + FIG_ACTION) = 8;
                U8(st, a + FIG_RESOURCE) = which == 0x6C ? 0x14 : 0x16;
                S16(st, a + FIG_HOME) = (int16_t) u->id;
                S16(st, a + FIG_WAIT_TICKS) = 0x1E;
                S16(st, a + FIG_AMOUNT) = 100;
            }
        }
    }
    /* @bin 0x00462AA5 at most the room left below 500, in units of 25 / 50 / 100 */
    int32_t unit = which == 0x6C ? 25 : which == 0xC3 ? 50 : 100;
    int32_t room = 500 - S16(st, b + BLD_STORED);
    if (n * unit > room) {
        n = room / unit;
        u->l38 = n;
    }
    int32_t wtype = which == 0x6C ? 0x4B : which == 0xC3 ? 0x5A : 0x49;
    if (U8(st, b + BLD_SHOWS) > 5) {
        U8(st, b + BLD_SHOWS) = (uint8_t) GAME_CALL(st, FN_BUILDING_COUNT_FIGURES, u->id, wtype);
    }
    if (which == 0xC3) {
        U8(st, b + BLD_SPAWN_DELAY) = 0;
        if (U8(st, G_LIST_DONE_C3) == 0) {
            return;
        }
    } else {
        if ((int32_t) U8(st, b + BLD_SHOWS) >= n) {
            return;
        }
        if (which == 0x6C && U8(st, G_LIST_DONE_6C) == 0) {
            return;
        }
        U8(st, b + BLD_SPAWN_DELAY) = 0;
    }
    while ((int32_t) U8(st, b + BLD_SHOWS) < n) {
        int32_t f = create(u, wtype, 0);
        uint32_t a = fig_addr(f);
        U8(st, a + FIG_ACTION) = which == 0x73 ? 0xE : 8;
        S16(st, a + FIG_HOME) = (int16_t) u->id;
        uint32_t v = random_step(st);
        /* @bin 0x00462C23 a wait of rand % 30, @bin 0x00465421 rand & 0x1F */
        S16(st, a + FIG_WAIT_TICKS) = (int16_t) (which == 0x73 ? (v & 0x1F) : (uint32_t) ((int32_t) v % 30));
        U8(st, b + BLD_SHOWS) = (uint8_t) (U8(st, b + BLD_SHOWS) + 1);
    }
}

/* @bin 0x00466A65 type 0x88 */
static void upd_0x88(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    if (!road(u)) {
        return;
    }
    few_workers(u, 0x32);
    int32_t r = worker_ratio(u);
    int32_t other = S32(st, b + BLD_FERRY_PARTNER);
    worker_ratio_of(st, BUILDING_ADDR(other));
    if (other < 0 || r == 0) {
        return;
    }
    int32_t delay = by_ratio(r, 3, 7, 15, 29, -1);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    U8(st, b + BLD_SPAWN_DELAY) = 0;
    if (fig_kept(u, b + BLD_WALKER, 0x4C)) {
        return;
    }
    if (!GAME_CALL(st, FN_BUILDING_FIND_ROAD_8, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE))) {
        return;
    }
    int32_t f = create(u, 0x4C, 0);
    int32_t slot = GAME_CALL(st, FN_FIGURE_GROUP_NEW, f);
    uint32_t a = fig_addr(f);
    S16(st, b + BLD_WALKER) = (int16_t) f;
    S32(st, a + FIG_U11C) = slot;
    U8(st, a + FIG_ACTION) = 8;
    for (uint32_t i = 0; i < 10; i++) {
        S32(st, G_FERRY_SLOTS + 4 + 44u * (uint32_t) slot + 4 * i) = -1;
    }
    S16(st, a + FIG_HOME) = (int16_t) u->id;
}

/* @bin 0x00465462 type 0xB4 */
static void upd_0xb4(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    GAME_CALL(st, FN_BUILDING_UPDATE_0XB4_TILES, u->id);
    flag_no_workers(u);
    if (fig_kept(u, b + BLD_WALKER, 0x57) || !road(u) || U8(st, b + BLD_WATER_SUPPLY) == 0) {
        return;
    }
    few_workers(u, 0x32);
    int32_t r = worker_ratio(u);
    if (S8(st, G_GRID_DESIRABILITY + (uint32_t) S32(st, b + BLD_GRID_OFFSET)) < 0x1E) {
        r /= 2;
    }
    int32_t delay = by_ratio(r, 1, 3, 7, 15, 29);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    U8(st, b + BLD_SPAWN_DELAY) = 0;
    tail_action_link(u, create(u, 0x57, 0), 1);
}

/* @bin 0x00466436 types 0xB5, 0xB6 */
static void upd_0xb5(upd *u, uint8_t type)
{
    flag_no_workers(u);
    fig_slot_check(u, u->b + BLD_DOCK_BOAT, type);
    if (road(u) && S16(u->st, u->b + BLD_SERVICE) <= 0x32) {
        GAME_CALL(u->st, FN_BUILDING_SPAWN_TYPE5, u->id);
    }
}

/* @bin 0x00462545 type 0xE2: walker 0x69, paid from +0x3C and +0xCC (0x00708214 each) */
static void upd_0xe2(upd *u)
{
    game_state *st = u->st;
    uint32_t b = u->b;
    flag_no_workers(u);
    if (fig_kept(u, b + BLD_WALKER, 0x69) || !road(u)) {
        return;
    }
    few_workers(u, 0x32);
    int32_t delay = by_ratio(worker_ratio(u), 7, 15, 30, 50, 80);
    if (delay == SKIP || !counter_due(u, delay)) {
        return;
    }
    int32_t cost = S32(st, G_TYPE_WALKER_COST + 0x28 * 0xE2);
    int16_t s = S16(st, b + BLD_STORED);
    if (s < cost || S16(st, b + BLD_STOCK2) < cost) {
        return;
    }
    S16(st, b + BLD_STORED) = (int16_t) (s - cost);
    S16(st, b + BLD_STOCK2) = (int16_t) (S16(st, b + BLD_STOCK2) - (int16_t) cost);
    U8(st, b + BLD_SPAWN_DELAY) = 0;
    tail_action_link(u, create(u, 0x69, 0), 0xC);
}

/* @bin 0x00466F62 types 0x5A, 0x5E, 0xB9, 0xBA */
static void upd_0x5a(upd *u)
{
    flag_no_workers(u);
    if (road(u) && S16(u->st, u->b + BLD_SERVICE) <= 0x64) {
        GAME_CALL(u->st, FN_BUILDING_SPAWN_TYPE5, u->id);
    }
}

static const int32_t D_3_44[5] = {3, 7, 15, 29, 44};
static const int32_t D_5_60[5] = {5, 10, 20, 35, 60};
static const int32_t D_7_90[5] = {7, 15, 30, 60, 90};
static const int32_t D_0_15[5] = {0, 1, 3, 7, 15};

static void update_one(upd *u)
{
    game_state *st = u->st;
    int32_t id = u->id;
    switch (S16(st, u->b + BLD_TYPE)) {
    case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D:
        upd_0x18(u);
        break;
    case 0x1E:
        upd_0x1e(u);
        break;
    case 0x1F:
        upd_0x1f(u);
        break;
    case 0x20:
        upd_0x20(u);
        break;
    case 0x21:
        upd_0x21(u);
        break;
    case 0x22: /* @bin 0x00464234 */
        service(u, 0, 0x32, D_3_44, 0x10, TAIL_8);
        break;
    case 0x23: /* @bin 0x00464321 */
        service(u, 0, 0x32, D_5_60, 0x11, TAIL_8);
        break;
    case 0x24: /* @bin 0x00464147 */
        service(u, 0, 0x32, D_3_44, 0xF, TAIL_8);
        break;
    case 0x25: /* @bin 0x0046440E */
        service(u, 0, 0x32, D_7_90, 0x12, TAIL_8);
        break;
    case 0x2E: /* @bin 0x00465EC0 */
        service(u, 0x21, 0x32, D_3_44, 0x21, TAIL_1);
        break;
    case 0x2F: /* @bin 0x00465FF6 */
        upd_paid(u, 0x22, (int32_t) (G_TYPE_WALKER_COST + 0x28 * 0x2F), 3, 1, G_TYPE_WALKER_COST + 0x28 * 0x44);
        break;
    case 0x31: /* @bin 0x00465D94 */
        service(u, 0x1F, 0x32, D_3_44, 0x1F, TAIL_1);
        break;
    case 0x33: /* @bin 0x004659FB */
        upd_paid(u, 0x1D, (int32_t) (G_TYPE_WALKER_COST + 0x28 * 0x33), 2, 2, G_TYPE_WALKER_COST + 0x28 * 0x43);
        break;
    case 0x35: /* @bin 0x00465BE5 */
        upd_paid(u, 0x1E, (int32_t) (G_TYPE_WALKER_COST + 0x28 * 0x35), 2, 2, G_TYPE_WALKER_COST + 0x28 * 0x43);
        break;
    case 0x37: /* @bin 0x00463D5B */
        service(u, 0x58, 0x64, D_0_15, 0x58, TAIL_8);
        break;
    case 0x39: /* @bin 0x00466E50 */
        GAME_CALL(st, FN_BUILDING_FORMATION_SIZE, id);
        break;
    case 0x3B: case 0xAC: case 0xAD:
        upd_0x3b(u);
        break;
    case 0x3C: case 0x3D: case 0x3E: case 0x3F: case 0x40: case 0x41: case 0x42: case 0x43: case 0x44:
    case 0x45: case 0x97: case 0x9C:
        upd_0x3c(u);
        break;
    case 0x46: case 0xBE:
        upd_0x46(u);
        break;
    case 0x47: case 0xBF:
        upd_cart(u, 0);
        break;
    case 0x48: case 0xC1:
        upd_cart(u, 1);
        break;
    case 0x4A:
        upd_0x4a(u);
        break;
    case 0x4B: case 0xC0:
        upd_0x4b(u);
        break;
    case 0x4C:
        upd_0x4c(u);
        break;
    case 0x50:
        upd_0x50(u);
        break;
    case 0x51: /* @bin 0x00463ACF */
        service(u, 8, 0x64, D_0_15, 8, TAIL_8);
        break;
    case 0x54: case 0x55: case 0x56: case 0x57: case 0xBB: case 0xBC: case 0xBD:
        upd_0x54(u);
        break;
    case 0x58:
        upd_0x58(u);
        break;
    case 0x59:
        upd_0x59(u);
        break;
    case 0x5A: case 0x5E: case 0xB9: case 0xBA:
        upd_0x5a(u);
        break;
    case 0x5D: /* @bin 0x00466E42 */
        GAME_CALL(st, FN_BUILDING_CYCLE_IMAGE, id);
        break;
    case 0x5F:
        upd_0x5f(u);
        break;
    case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6D:
    case 0x6E: case 0x6F: case 0x70: case 0x71: case 0x72: case 0xA1: case 0xA2: case 0xC2: case 0xC4:
    case 0xCB: case 0xCC: case 0xCD: case 0xD8: case 0xD9: case 0xDD: case 0xE0: case 0xE8: case 0xE9:
        upd_0x64(u);
        break;
    case 0x6C:
        upd_gatherers(u, 0x6C);
        break;
    case 0x73:
        upd_gatherers(u, 0x73);
        break;
    case 0x88:
        upd_0x88(u);
        break;
    case 0xA7: /* @bin 0x00463E8E */
        service(u, 0xA, 0x64, D_0_15, 0xA, TAIL_8);
        break;
    case 0xB1: /* @bin 0x00462F87 */
        GAME_CALL(st, FN_BUILDING_UPDATE_0XB1, id);
        break;
    case 0xB2: /* @bin 0x00462FA3 */
        GAME_CALL(st, FN_BUILDING_UPDATE_0XB2, id);
        break;
    case 0xB3: /* @bin 0x00462F95 */
        GAME_CALL(st, FN_BUILDING_UPDATE_0XB3, id);
        break;
    case 0xB4:
        upd_0xb4(u);
        break;
    case 0xB5:
        upd_0xb5(u, 0x4D);
        break;
    case 0xB6:
        upd_0xb5(u, 0x4E);
        break;
    case 0xB8: /* @bin 0x00463C29 */
        service(u, 0x59, 0x64, D_0_15, 0x59, TAIL_8);
        break;
    case 0xC3:
        upd_gatherers(u, 0xC3);
        break;
    case 0xC7: /* @bin 0x00462FBF */
        GAME_CALLT(st, G_UD05720, FN_BUILDING_UPDATE_0XC7, id);
        break;
    case 0xCE: /* @bin 0x004655C2 */
        service(u, 0x20, 0x32, D_3_44, 0x20, TAIL_1);
        break;
    case 0xE2:
        upd_0xe2(u);
        break;
    case 0xE7: /* @bin 0x00462FB1 */
        GAME_CALL(st, FN_BUILDING_UPDATE_0XE7, id);
        break;
    default:
        break;
    }
}

void building_update_all(game_state *st)
{
    /* @bin 0x00462180 (timeGetTime calls for profiling are left out: their results are unused) */
    upd u = {0};
    u.st = st;
    u.l28 = 100;
    u.l2c = 100;
    /* l30 and l20 are not initialised by the original (stack contents) */
    S32(st, G_FIGURE_28_SENT) = 0;
    U8(st, G_HARVEST_DOUBLED) = 0;
    if (S32(st, G_TOWER_SENTRY_REQUEST) > 0) {
        S32(st, G_TOWER_SENTRY_REQUEST) -= 1;
    }
    int32_t highest = S32(st, G_BUILDING_HIGHEST);
    /* @bin 0x004621EE buildings of types 0xB1, 0xB2, 0xB3, 0xE7: +0xA6 := 0 */
    for (int32_t id = 1; id <= highest; id++) {
        uint32_t b = BUILDING_ADDR(id);
        int16_t t = S16(st, b + BLD_TYPE);
        if (t == 0xB3 || t == 0xB2 || t == 0xB1 || t == 0xE7) {
            U8(st, b + BLD_SHOWS) = 0;
        }
    }
    /* @bin 0x0046221D their figures (types 0x51, 0x50, 0x4F, 0x6C) are counted in +0xA6; past 4,
     * the figure of a building of the matching type is sent back */
    for (int32_t fid = 1; fid < FIGURE_COUNT; fid++) {
        uint32_t f = FIGURE_ADDR(fid);
        if (U8(st, f + FIG_STATE) == 0) {
            continue;
        }
        uint8_t ft = U8(st, f + FIG_TYPE);
        int16_t match = ft == 0x51 ? 0xB3 : ft == 0x50 ? 0xB2 : ft == 0x4F ? 0xB1 : ft == 0x6C ? 0xE7 : 0;
        if (match == 0) {
            continue;
        }
        int32_t bid = S16(st, f + FIG_HOME);
        if (bid <= 0 || bid > highest) {
            continue;
        }
        uint32_t b = BUILDING_ADDR(bid);
        uint8_t n = (uint8_t) (U8(st, b + BLD_SHOWS) + 1);
        U8(st, b + BLD_SHOWS) = n;
        if (S16(st, b + BLD_TYPE) == match && n > 4) {
            U8(st, f + FIG_ACTION) = 4;
            S16(st, f + FIG_FOLLOW) = 0;
            U8(st, f + FIG_INVISIBLE) = 1;
        }
    }
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        if (S16(st, b + BLD_TYPE) != 0x49) {
            U8(st, b + BLD_PROBLEM_OVERLAY) = 0;
        }
        u.id = id;
        u.b = b;
        update_one(&u);
    }
    if (U8(st, G_HARVEST_DOUBLED) != 0) {
        S32(st, CURRENT_CITY(st) + CITY_U4994) = 0; /* @bin 0x00466FE0 ebp, 0 on every path */
    }
}

void building_lowest_goods(game_state *st, int32_t id, int32_t *good1, int32_t *good2, int32_t *ratio1,
                           int32_t *ratio2, int32_t *dest1, int32_t *dest2)
{
    /* @bin 0x0045B880 */
    uint32_t b = BUILDING_ADDR(id);
    uint32_t rec = U8(st, b + BLD_STORAGE_ID);
    int32_t rx = S32(st, G_ROAD_ACCESS_X), ry = S32(st, G_ROAD_ACCESS_Y);
    int16_t type = S16(st, b + BLD_TYPE);
    int big = type == 0x47 || type == 0xBF;
    uint32_t list = big ? G_GOODS_LIST : G_STORAGE_LIST + 4;
    int32_t n = big ? S32(st, G_GOODS_COUNT) : S32(st, G_STORAGE_LIST);
    int32_t ratio[36]; /* the original's stack array, by good (written before it is read) */
    int32_t r = 100;
    int any = 0;
    if (n <= 0) {
        return;
    }
    for (int32_t i = 0; i < n; i++) {
        int32_t good = S32(st, list + 4u * (uint32_t) i);
        if (good < 0 || good >= 36) {
            state_unported(st, FN_BUILDING_LOWEST_GOODS); /* outside the original's stack array */
            return;
        }
        ratio[good] = 100;
        if (U8(st, G_STORAGE_ORDERS + 0xC4u * rec + (uint32_t) good) != 2) {
            continue;
        }
        any = 1;
        /* @bin 0x0045B958 the stock against the record's capacity (i16 at 0x00790134), in percent;
         * r is kept from the previous good when the good is not positive */
        int16_t cap = S16(st, G_STORAGE_GET + 0xC4u * rec + 2u * (uint32_t) good);
        if (S16(st, b + BLD_TYPE) == 0x48) {
            if (GAME_CALL(st, FN_BUILDING_PARTS_STOCK, id, good) >= cap) {
                r = 100;
            } else if (good > 0) {
                r = GAME_CALL(st, FN_BUILDING_PARTS_STOCK, id, good) * 100 / cap;
            }
        } else {
            int16_t s = S16(st, b + BLD_INVENTORY + 2u * (uint32_t) good);
            if (s >= cap) {
                r = 100;
            } else if (good > 0) {
                r = s * 100 / cap;
            }
        }
        ratio[good] = r;
    }
    if (!any) {
        return;
    }
    for (int32_t i = 0; i < n; i++) {
        int32_t good = S32(st, list + 4u * (uint32_t) i);
        int32_t dest = GAME_CALL(st, FN_BUILDING_FIND_SUPPLIER, id, good, 0);
        if (S32(st, CITY_BASE + CITY_U2A88 + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) good)) != 0 || dest == 0) {
            continue;
        }
        int32_t v = ratio[good];
        if (v < *ratio1) {
            *ratio2 = *ratio1;
            *good2 = *good1;
            *dest2 = *dest1;
            *ratio1 = v;
            *good1 = good;
            *dest1 = dest;
        } else if (v < *ratio2) {
            *ratio2 = v;
            *good2 = good;
            *dest2 = dest;
        }
    }
    S32(st, G_ROAD_ACCESS_X) = rx;
    S32(st, G_ROAD_ACCESS_Y) = ry;
}

void building_ratio_flags(int32_t ratio1, int32_t ratio2, int32_t n, uint8_t *out1, uint8_t *out2, int32_t *out3)
{
    /* @bin 0x004611D0 */
    if (n == 0) {
        *out1 = 0;
        *out2 = 0;
        *out3 = 0;
        return;
    }
    if (n == 1) {
        *out2 = 1;
        if (ratio1 < 50) {
            *out1 = 1;
            *out3 = 2;
        } else if (ratio1 < 100) {
            *out1 = 0;
            *out3 = 1;
        } else {
            *out1 = 0;
            *out3 = 0;
        }
        return;
    }
    if (ratio1 > 100 && ratio2 > 100) {
        *out3 = 0;
        *out2 = 0;
        *out1 = 0;
    }
    if (ratio1 < 50 && ratio2 > 100) {
        *out3 = 2;
        *out2 = 1;
        *out1 = 1;
    }
    if (ratio1 < 100) {
        if (ratio2 > 100) {
            *out3 = 1;
            *out2 = 1;
            *out1 = 0;
        }
        if (ratio2 < 100) {
            *out3 = 2;
            *out2 = 0;
            *out1 = 1;
        }
    }
}

int building_pending_request(game_state *st, int32_t id, int32_t *out)
{
    /* @bin 0x00460F70 the queue object of entry +0xCB - 1 (0x1C bytes each), its next item
     * (0x004F68A0, thiscall) */
    uint8_t q = U8(st, BUILDING_ADDR(id) + BLD_REC4A_SLOT);
    if (q == 0) {
        return 0;
    }
    int32_t v = GAME_CALLT0(st, G_REC4A + 0x1C + 0x1Cu * (uint32_t) (q - 1), FN_RING_POP_4F68A0);
    *out = v;
    return v != 0;
}

/* one slot of 0x00460FB0: the figure is alive, of `type` or 0, and belongs to building `id` */
static int missing_slot(game_state *st, uint32_t slot, int32_t id, uint8_t type)
{
    int16_t f = S16(st, slot);
    if (f <= 0) {
        return 0;
    }
    uint32_t a = FIGURE_ADDR(f);
    if (U8(st, a + FIG_STATE) != 0 && (U8(st, a + FIG_TYPE) == type || U8(st, a + FIG_TYPE) == 0) &&
        S16(st, a + FIG_HOME) == id) {
        return 1;
    }
    S16(st, slot) = 0;
    return 0;
}

int building_missing_worker(game_state *st, int32_t *out)
{
    /* @bin 0x00460FB0 buildings 1 .. 3999 in use with +0x40 != 0: per type 0x4C / 0xB5 / 0xB6,
     * how many, how many with their figure (+0xB6) out, and requests (+0xB4) of type 0x4A */
    int32_t n4c = 0, ok4c = 0, nb5 = 0, okb5 = 0, nb6 = 0, okb6 = 0, r19 = 0, r4d = 0, r4e = 0;
    for (int32_t id = 1; id < BUILDING_COUNT; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WORKERS) == 0) {
            continue;
        }
        int16_t t = S16(st, b + BLD_TYPE);
        if (t == 0x4C) {
            n4c++;
            ok4c += missing_slot(st, b + BLD_SECOND_CART, id, 0x19);
        }
        if (t == 0xB5) {
            nb5++;
            okb5 += missing_slot(st, b + BLD_SECOND_CART, id, 0x4D);
        }
        if (t == 0xB6) {
            nb6++;
            okb6 += missing_slot(st, b + BLD_SECOND_CART, id, 0x4E);
        }
        if (t == 0x4A) {
            int16_t r = S16(st, b + BLD_REQUEST);
            if (r == 0x1019) {
                r19++;
            }
            if (r == 0x104D) {
                r4d++;
            }
            if (r == 0x104E) {
                r4e++;
            }
        }
    }
    if (r19 + ok4c < n4c) {
        *out = 0x19;
        return 1;
    }
    if (r4d + okb5 < nb5) {
        *out = 0x4D;
        return 1;
    }
    if (r4e + okb6 < nb6) {
        *out = 0x4E;
        return 1;
    }
    return 0;
}

static int32_t run_update_all(game_state *st, const int32_t *a)
{
    (void) a;
    building_update_all(st);
    return 0;
}

const ported_function bigb_update_ports[] = {
    {0x00462180u, "building_update_all", 0, 0, 0, run_update_all, 0},
    /* they write their callers' stack variables: ported inside their callers (run NULL) */
    {0x0045B880u, "building_lowest_goods (in building_update.c)", 7, 0, 0, NULL, 0},
    {0x004611D0u, "building_ratio_flags (in building_update.c)", 6, 0, 0, NULL, 0},
    {0x00460F70u, "building_pending_request (in building_update.c)", 2, 8, 0, NULL, 0},
    {0x00460FB0u, "building_missing_worker (in building_update.c)", 1, 8, 0, NULL, 0},
};
const int bigb_update_ports_count = sizeof(bigb_update_ports) / sizeof(bigb_update_ports[0]);
