/* Imhotep - workers given to the buildings, by labour category.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/labour.h"

#include "sim/call.h"
#include "sim/math.h"
#include "state/ported.h"
#include "state/records.h"

#define BUILDING_HIGHEST 0x0078D790u
#define BUILDING_MANAGER 0x00BCA350u     /* `this` of 0x00488030, 0x00488050 */
#define TYPE_CATEGORY 0x005D06ECu        /* i32 labour category per building type, stride 0x14 */
#define TYPE_WORKERS 0x00705EB8u         /* i32 workers needed per building type, stride 0x28 */
#define CATEGORIES 10

static int32_t category(game_state *st, int16_t type)
{
    return S32(st, TYPE_CATEGORY + 0x14u * (uint32_t) (int32_t) type);
}

static int32_t workers_needed(game_state *st, int16_t type)
{
    return S32(st, TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) type);
}

int32_t building_resource_in_stock(game_state *st, int32_t id)
{
    /* @bin 0x00459240 the type tests before it always end with "not excluded" (its range test
     * t >= 0x6A || t <= 0x72 holds for every type); the call is kept */
    GAME_CALLT(st, BUILDING_MANAGER, 0x00488030u, id);
    uint32_t i = U8(st, BUILDING_ADDR(id) + 0x43) + 0x24ECu * U32(st, CURRENT_PLAYER_ADDR);
    return S16(st, 0x00EA4126u + 2 * i) != 0;
}

void labour_allocate(game_state *st)
{
    /* @bin 0x00459670 per category (city +0x2B30 + 0x14 * c: workers available, +0x2B2C: needed):
     * pool := 1 when fewer are available than needed */
    uint32_t cats = CURRENT_CITY(st) + 0x2B30u;
    int32_t pool[CATEGORIES];
    for (int c = 0; c < CATEGORIES; c++) {
        pool[c] = S32(st, cats + 0x14u * (uint32_t) c) < S32(st, cats + 0x14u * (uint32_t) c - 4);
    }
    /* @bin 0x004596C6 first pass: each building's share, in proportion to its +0x1A (percent) */
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1) {
            continue;
        }
        int16_t type = S16(st, b + 0x10);
        int32_t c = category(st, type);
        if ((c == 1 || c == 0) && (int8_t) GAME_CALLT(st, BUILDING_MANAGER, 0x00488050u, id) != 0) {
            continue;
        }
        if (c < 0 || (c == 3 && S16(st, b + 0x38) != 0)) {
            continue;
        }
        S16(st, b + 0x40) = 0;
        if (c == 1 || c == 0) {
            if (building_resource_in_stock(st, id)) {
                continue;
            }
        } else if (c == 2) {
            if ((type == 0x1F || type == 0x1E || type == 0x21) && S16(st, b + 0x38) != 0) {
                continue;
            }
        } else if (c == 8 && (type == 0x70 || type == 0xCD)) {
            if (building_resource_in_stock(st, id)) {
                continue;
            }
        }
        int16_t percent = S16(st, b + 0x1A);
        if (percent <= 0) {
            continue;
        }
        if (pool[c] == 0) {
            S16(st, b + 0x40) = (int16_t) workers_needed(st, type);
        } else {
            int32_t share = calc_percentage(S32(st, cats + 0x14u * (uint32_t) c), percent) / 100;
            if (share >= workers_needed(st, type)) {
                share = workers_needed(st, type);
            }
            pool[c] += share;
            S16(st, b + 0x40) = (int16_t) share;
        }
    }
    /* @bin 0x00459817 what is left of each category */
    for (int c = 0; c < CATEGORIES; c++) {
        if (pool[c] != 0) {
            pool[c] -= 1;
        }
    }
    for (int c = 0; c < CATEGORIES; c++) {
        int32_t avail = S32(st, cats + 0x14u * (uint32_t) c);
        pool[c] = pool[c] < avail ? avail - pool[c] : 0;
    }
    /* @bin 0x00459875 second pass: the rest tops buildings up to their need */
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1) {
            continue;
        }
        int16_t type = S16(st, b + 0x10);
        int32_t c = category(st, type);
        if (c == 0) {
            if ((int8_t) GAME_CALLT(st, BUILDING_MANAGER, 0x00488050u, id) != 0) {
                continue;
            }
            if (building_resource_in_stock(st, id)) {
                continue;
            }
        } else if (c < 0 || c == 8) {
            continue;
        } else if (c == 1) {
            if (building_resource_in_stock(st, id)) {
                continue;
            }
        } else if (c == 2 && S16(st, b + 0x38) != 0) {
            continue;
        }
        if (S16(st, b + 0x1A) <= 0 || pool[c] == 0) {
            continue;
        }
        int16_t have = S16(st, b + 0x40);
        int32_t need = workers_needed(st, type);
        if (have >= need) {
            continue;
        }
        need -= have;
        if (need > pool[c]) {
            S16(st, b + 0x40) = (int16_t) (have + pool[c]);
            pool[c] = 0;
        } else {
            S16(st, b + 0x40) = (int16_t) (have + need);
            pool[c] -= need;
        }
    }
    /* @bin 0x00459934 type-0x88 buildings (as in 0x00516F90) */
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1 || S16(st, b + 0x10) != 0x88) {
            continue;
        }
        if (S16(st, b + 0x40) == 0 || S16(st, BUILDING_ADDR(U32(st, b + 0xC4)) + 0x40) <= 0) {
            GAME_CALL(st, 0x0043F060u, id, 1, 3);
            GAME_CALL(st, 0x004A36B0u, id);
        } else {
            GAME_CALL(st, 0x0043F060u, id, 1, 1);
        }
    }
}

static int32_t run_allocate(game_state *st, const int32_t *a)
{
    (void) a;
    labour_allocate(st);
    return 0;
}

static int32_t run_in_stock(game_state *st, const int32_t *a)
{
    return building_resource_in_stock(st, a[0]);
}

static int32_t run_percentage(game_state *st, const int32_t *a)
{
    (void) st;
    return calc_percentage(a[0], a[1]);
}

const ported_function labour_ports[] = {
    {0x00459670u, "labour_allocate", 0, 0, 0, run_allocate, 0},
    {0x00459240u, "building_resource_in_stock", 1, 32, 0, run_in_stock, 0},
    {0x004CDE40u, "calc_percentage", 2, 32, 0, run_percentage, 0},
};
const int labour_ports_count = sizeof(labour_ports) / sizeof(labour_ports[0]);
