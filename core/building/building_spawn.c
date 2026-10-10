/* Imhotep - helpers of the daily building update 0x00462180 (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32), figure record: chunk 21. */
#include "building/building_spawn.h"

#include "figure/figure_create.h"
#include "map/map_building.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


void building_spawn_type5(game_state *st, int32_t id)
{
    /* @bin 0x00467220 */
    if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_NEEDS_AREA_CHECK, id) != 0) {
        return;
    }
    if (S32(st, CURRENT_CITY(st) + CITY_POPULATION) <= 0) {
        return;
    }
    uint32_t b = BUILDING_ADDR(id);
    int16_t f = S16(st, b + BLD_WALKER2);
    if (f != 0) {
        uint32_t a = FIGURE_ADDR(f);
        if (U8(st, a + FIG_STATE) == 0) {
            S16(st, b + BLD_WALKER2) = 0;
        }
        if (U8(st, a + FIG_TYPE) != 5) {
            S16(st, b + BLD_WALKER2) = 0;
        }
        if (S16(st, a + FIG_HOME) != id) {
            S16(st, b + BLD_WALKER2) = 0;
        }
        return;
    }
    int32_t x = S32(st, G_ROAD_ACCESS_X), y = S32(st, G_ROAD_ACCESS_Y);
    int32_t n = figure_create(st, U8(st, b + BLD_OWNER), 5, x, y, 0);
    uint32_t a = FIGURE_ADDR(n);
    S16(st, b + BLD_WALKER2) = (int16_t) n;
    U8(st, a + FIG_ACTION) = 1;
    S16(st, a + FIG_HOME) = (int16_t) id;
    figure_walker_start(st, n);
    S32(st, G_ROAD_ACCESS_X) = x;
    S32(st, G_ROAD_ACCESS_Y) = y;
}

int32_t building_mark_tiles_by_level(game_state *st, int32_t id)
{
    /* @bin 0x004673C0 the building's tiles (0x0047BF50) with the value 0x00F56584 above level 0x1E
     * of grid 0x00AC2D4C at its tile, else 0x00F564CC; 1 in the first case */
    uint32_t b = BUILDING_ADDR(id);
    if (U8(st, b + BLD_STATE) != 1) {
        return 0;
    }
    int above = S8(st, G_GRID_DESIRABILITY + (uint32_t) S32(st, b + BLD_GRID_OFFSET)) > 0x1E;
    map_building_tiles(st, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), U8(st, b + BLD_SIZE),
                       S32(st, above ? G_IMAGE_GROUPS + 8 * 314 : G_IMAGE_GROUPS + 8 * 291), 8);
    return above;
}

void building_update_0xb4_tiles(game_state *st, int32_t id)
{
    /* @bin 0x00467310 */
    uint32_t b = BUILDING_ADDR(id);
    if (U8(st, b + BLD_STATE) != 1) {
        return;
    }
    int32_t g = S32(st, b + BLD_GRID_OFFSET);
    U8(st, b + BLD_WATER_SUPPLY) = GAME_CALL(st, FN_MAP_AREA_HAS_TERRAIN, S16(st, b + BLD_X), S16(st, b + BLD_Y), 2, 0x80) != 0 ? 1 : 0;
    if (S16(st, b + BLD_WORKERS) != 0 && U8(st, b + BLD_WATER_SUPPLY) != 0) {
        /* @bin 0x0046737B two more from 0x1E on */
        int32_t image = (S8(st, G_GRID_DESIRABILITY + (uint32_t) g) < 0x1E ? 0 : 2) + S32(st, G_IMAGE_GROUPS + 8 * 338);
        map_building_tiles(st, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), 2, 2, image, 8);
    }
}

int32_t building_count_figures(game_state *st, int32_t id, int32_t type)
{
    /* @bin 0x00462140 alive figures of `type` belonging to building `id` */
    int32_t n = 0;
    for (int32_t f = 1; f < FIGURE_COUNT; f++) {
        uint32_t a = FIGURE_ADDR(f);
        if (U8(st, a + FIG_STATE) == 1 && (int32_t) U8(st, a + FIG_TYPE) == type && S16(st, a + FIG_HOME) == id) {
            n++;
        }
    }
    return n;
}

int32_t building_parts_stock(game_state *st, int32_t id, int32_t good)
{
    /* @bin 0x0045D510 the stock (+0x3C) of `good` (+0x12) in the 8 parts linked from the building
     * (+0x3A); 0 when the chain is shorter than 8 */
    int32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        id = S16(st, BUILDING_ADDR(id) + BLD_NEXT_PART);
        if (id <= 0) {
            return 0;
        }
        uint32_t p = BUILDING_ADDR(id);
        if (S16(st, p + BLD_STORED_GOOD) != 0 && S16(st, p + BLD_STORED_GOOD) == good) {
            sum += S16(st, p + BLD_STORED);
        }
    }
    return sum;
}

int32_t building_count_nonzero(int32_t a, int32_t b)
{
    /* @bin 0x0045BB30 */
    return (a != 0) + (b != 0);
}

int32_t building_production_ready(game_state *st, int32_t id)
{
    /* @bin 0x00455BF0 progress +0x54 reached 200 (400 with a good at +0x12) */
    uint32_t b = BUILDING_ADDR(id);
    return S16(st, b + BLD_INDUSTRY_PROGRESS) >= (S16(st, b + BLD_INPUT) != 0 ? 400 : 200);
}

void building_production_take(game_state *st, int32_t id)
{
    /* @bin 0x00455C30 */
    uint32_t b = BUILDING_ADDR(id);
    S16(st, b + BLD_INDUSTRY_PROGRESS) = 0;
    if (S16(st, b + BLD_INPUT) != 0 && S16(st, b + BLD_STORED) != 0) {
        S16(st, b + BLD_STORED) = (int16_t) (S16(st, b + BLD_STORED) - 100);
    }
    if (S16(st, b + BLD_INPUT2) != 0 && S16(st, b + BLD_STOCK2) != 0) {
        /* @bin 0x00455C7D 25 for type 0xCC, else 100 */
        S16(st, b + BLD_STOCK2) = (int16_t) (S16(st, b + BLD_STOCK2) - (S16(st, b + BLD_TYPE) == 0xCC ? 0x19 : 100));
    }
}

void building_houses_nearby(game_state *st, int32_t id, int32_t dist)
{
    /* @bin 0x00455CC0 houses (+0x05 size, +0x1C) within `dist` tiles: per their level (+0x12), four
     * values of table 0x0070CDF8 (0x58 bytes per level, times 4 for houses larger than 1); each
     * non-zero one sets a delay of building `id` (+0x9E, +0xA2, +0xA0, +0xA4) to 10 */
    int32_t n = S32(st, G_BUILDING_HIGHEST);
    uint32_t p = BUILDING_ADDR(id);
    for (int32_t k = 1; k <= n; k++) {
        uint32_t h = BUILDING_ADDR(k);
        if (U8(st, h + BLD_STATE) != 1 || U8(st, h + BLD_HOUSE_SIZE) == 0 || S16(st, h + BLD_POPULATION) == 0) {
            continue;
        }
        int32_t hx = S16(st, h + BLD_X), hy = S16(st, h + BLD_Y), px = S16(st, p + BLD_X), py = S16(st, p + BLD_Y);
        int32_t dx = hx > px ? hx - px : hx < px ? px - hx : 0;
        int32_t dy = hy > py ? hy - py : hy < py ? py - hy : 0;
        S32(st, G_DIST_X) = dx;
        S32(st, G_DIST_Y) = dy;
        if ((dx < dy ? dy : dx) > dist) {
            continue;
        }
        int32_t level = S16(st, h + BLD_SUBTYPE);
        if (level < 0x13) {
            level++;
        }
        uint32_t rec = G_HOUSE_LEVEL_GOODS + 0x58u * (uint32_t) level;
        int big = U8(st, h + BLD_SIZE) > 1;
        static const uint8_t fields[4] = {BLD_BAZAAR_DEMAND + 6, BLD_BAZAAR_DEMAND, BLD_BAZAAR_DEMAND + 4,
                                          BLD_BAZAAR_DEMAND + 2};
        static const uint8_t values[4] = {0x0C, 0x00, 0x04, 0x08};
        for (int i = 0; i < 4; i++) {
            uint32_t v = U32(st, rec + values[i]);
            if (big) {
                v <<= 2;
            }
            if (v != 0) {
                S16(st, p + fields[i]) = 10;
            }
        }
    }
}

int32_t building_cart_good(game_state *st, int32_t id, int32_t pending)
{
    /* @bin 0x0045CE90 with half its workers or more: 0 when `pending` and a good of kind 2 exists;
     * else the first good 1..8 of kind 3 in stock; -1 otherwise */
    uint32_t b = BUILDING_ADDR(id);
    uint32_t kinds = G_STORAGE_ORDERS + 0xC4u * U8(st, b + BLD_STORAGE_ID);
    int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S16(st, b + BLD_WORKERS), S32(st, G_TYPE_WORKERS + 0x28u * (uint32_t) (int32_t) S16(st, b + BLD_TYPE)));
    if (r < 50) {
        return -1;
    }
    if (pending != 0) {
        for (int32_t g = 1; g <= 8; g++) {
            if (U8(st, kinds + (uint32_t) g) == 2) {
                return 0;
            }
        }
    }
    for (int32_t g = 1; g <= 8; g++) {
        if (U8(st, kinds + (uint32_t) g) == 3 && S16(st, b + BLD_INVENTORY + 2u * (uint32_t) g) != 0) {
            return g;
        }
    }
    return -1;
}

#define INT1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { return fn(st, a[0]); }
#define INT2(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { return fn(st, a[0], a[1]); }
INT1(run_mark_tiles, building_mark_tiles_by_level)
INT1(run_production_ready, building_production_ready)
INT2(run_count_figures, building_count_figures)
INT2(run_parts_stock, building_parts_stock)
INT2(run_cart_good, building_cart_good)

static int32_t run_spawn_type5(game_state *st, const int32_t *a)
{
    building_spawn_type5(st, a[0]);
    return 0;
}

static int32_t run_0xb4_tiles(game_state *st, const int32_t *a)
{
    building_update_0xb4_tiles(st, a[0]);
    return 0;
}

static int32_t run_count_nonzero(game_state *st, const int32_t *a)
{
    (void) st;
    return building_count_nonzero(a[0], a[1]);
}

static int32_t run_production_take(game_state *st, const int32_t *a)
{
    building_production_take(st, a[0]);
    return 0;
}

static int32_t run_houses_nearby(game_state *st, const int32_t *a)
{
    building_houses_nearby(st, a[0], a[1]);
    return 0;
}

const ported_function bigb_spawn_ports[] = {
    {0x00467220u, "building_spawn_type5", 1, 0, 0, run_spawn_type5, 0},
    {0x004673C0u, "building_mark_tiles_by_level", 1, 8, 0, run_mark_tiles, 0},
    {0x00467310u, "building_update_0xb4_tiles", 1, 0, 0, run_0xb4_tiles, 0},
    {0x00462140u, "building_count_figures", 2, 32, 0, run_count_figures, 0},
    {0x0045D510u, "building_parts_stock", 2, 32, 0, run_parts_stock, 0},
    {0x0045BB30u, "building_count_nonzero", 2, 32, 0, run_count_nonzero, 0},
    {0x00455BF0u, "building_production_ready", 1, 32, 0, run_production_ready, 0},
    {0x00455C30u, "building_production_take", 1, 0, 0, run_production_take, 0},
    {0x00455CC0u, "building_houses_nearby", 2, 0, 0, run_houses_nearby, 0},
    {0x0045CE90u, "building_cart_good", 2, 32, 0, run_cart_good, 0},
};
const int bigb_spawn_ports_count = sizeof(bigb_spawn_ports) / sizeof(bigb_spawn_ports[0]);
