/* Imhotep - helpers of the daily building update 0x00462180 (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32), figure record: chunk 21. */
#include "building/building_spawn.h"

#include "figure/figure_create.h"
#include "map/map_building.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define ROAD_X 0x00D3B4E4u
#define ROAD_Y 0x00D3B4E0u
#define HIGHEST 0x0078D790u

void building_spawn_type5(game_state *st, int32_t id)
{
    /* @bin 0x00467220 */
    if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488050u, id) != 0) {
        return;
    }
    if (S32(st, CURRENT_CITY(st) + 0x24) <= 0) {
        return;
    }
    uint32_t b = BUILDING_ADDR(id);
    int16_t f = S16(st, b + 0x2C);
    if (f != 0) {
        uint32_t a = FIGURE_ADDR(f);
        if (U8(st, a + 0x0E) == 0) {
            S16(st, b + 0x2C) = 0;
        }
        if (U8(st, a + 0x0A) != 5) {
            S16(st, b + 0x2C) = 0;
        }
        if (S16(st, a + 0x5A) != id) {
            S16(st, b + 0x2C) = 0;
        }
        return;
    }
    int32_t x = S32(st, ROAD_X), y = S32(st, ROAD_Y);
    int32_t n = figure_create(st, U8(st, b + 1), 5, x, y, 0);
    uint32_t a = FIGURE_ADDR(n);
    S16(st, b + 0x2C) = (int16_t) n;
    U8(st, a + 0x38) = 1;
    S16(st, a + 0x5A) = (int16_t) id;
    figure_walker_start(st, n);
    S32(st, ROAD_X) = x;
    S32(st, ROAD_Y) = y;
}

int32_t building_mark_tiles_by_level(game_state *st, int32_t id)
{
    /* @bin 0x004673C0 the building's tiles (0x0047BF50) with the value 0x00F56584 above level 0x1E
     * of grid 0x00AC2D4C at its tile, else 0x00F564CC; 1 in the first case */
    uint32_t b = BUILDING_ADDR(id);
    if (U8(st, b) != 1) {
        return 0;
    }
    int above = S8(st, 0x00AC2D4Cu + (uint32_t) S32(st, b + 0x0C)) > 0x1E;
    map_building_tiles(st, id, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3), U8(st, b + 3),
                       S32(st, above ? 0x00F56584u : 0x00F564CCu), 8);
    return above;
}

void building_update_0xb4_tiles(game_state *st, int32_t id)
{
    /* @bin 0x00467310 */
    uint32_t b = BUILDING_ADDR(id);
    if (U8(st, b) != 1) {
        return;
    }
    int32_t g = S32(st, b + 0x0C);
    U8(st, b + 0x35) = GAME_CALL(st, 0x004816B0u, S16(st, b + 6), S16(st, b + 8), 2, 0x80) != 0 ? 1 : 0;
    if (S16(st, b + 0x40) != 0 && U8(st, b + 0x35) != 0) {
        /* @bin 0x0046737B two more from 0x1E on */
        int32_t image = (S8(st, 0x00AC2D4Cu + (uint32_t) g) < 0x1E ? 0 : 2) + S32(st, 0x00F56644u);
        map_building_tiles(st, id, S16(st, b + 6), S16(st, b + 8), 2, 2, image, 8);
    }
}

int32_t building_count_figures(game_state *st, int32_t id, int32_t type)
{
    /* @bin 0x00462140 alive figures of `type` belonging to building `id` */
    int32_t n = 0;
    for (int32_t f = 1; f < FIGURE_COUNT; f++) {
        uint32_t a = FIGURE_ADDR(f);
        if (U8(st, a + 0x0E) == 1 && (int32_t) U8(st, a + 0x0A) == type && S16(st, a + 0x5A) == id) {
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
        id = S16(st, BUILDING_ADDR(id) + 0x3A);
        if (id <= 0) {
            return 0;
        }
        uint32_t p = BUILDING_ADDR(id);
        if (S16(st, p + 0x12) != 0 && S16(st, p + 0x12) == good) {
            sum += S16(st, p + 0x3C);
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
    return S16(st, b + 0x54) >= (S16(st, b + 0x12) != 0 ? 400 : 200);
}

void building_production_take(game_state *st, int32_t id)
{
    /* @bin 0x00455C30 */
    uint32_t b = BUILDING_ADDR(id);
    S16(st, b + 0x54) = 0;
    if (S16(st, b + 0x12) != 0 && S16(st, b + 0x3C) != 0) {
        S16(st, b + 0x3C) = (int16_t) (S16(st, b + 0x3C) - 100);
    }
    if (S16(st, b + 0xCE) != 0 && S16(st, b + 0xCC) != 0) {
        /* @bin 0x00455C7D 25 for type 0xCC, else 100 */
        S16(st, b + 0xCC) = (int16_t) (S16(st, b + 0xCC) - (S16(st, b + 0x10) == 0xCC ? 0x19 : 100));
    }
}

void building_houses_nearby(game_state *st, int32_t id, int32_t dist)
{
    /* @bin 0x00455CC0 houses (+0x05 size, +0x1C) within `dist` tiles: per their level (+0x12), four
     * values of table 0x0070CDF8 (0x58 bytes per level, times 4 for houses larger than 1); each
     * non-zero one sets a delay of building `id` (+0x9E, +0xA2, +0xA0, +0xA4) to 10 */
    int32_t n = S32(st, HIGHEST);
    uint32_t p = BUILDING_ADDR(id);
    for (int32_t k = 1; k <= n; k++) {
        uint32_t h = BUILDING_ADDR(k);
        if (U8(st, h) != 1 || U8(st, h + 5) == 0 || S16(st, h + 0x1C) == 0) {
            continue;
        }
        int32_t hx = S16(st, h + 6), hy = S16(st, h + 8), px = S16(st, p + 6), py = S16(st, p + 8);
        int32_t dx = hx > px ? hx - px : hx < px ? px - hx : 0;
        int32_t dy = hy > py ? hy - py : hy < py ? py - hy : 0;
        S32(st, 0x00D3B4FCu) = dx;
        S32(st, 0x00D3B4F8u) = dy;
        if ((dx < dy ? dy : dx) > dist) {
            continue;
        }
        int32_t level = S16(st, h + 0x12);
        if (level < 0x13) {
            level++;
        }
        uint32_t rec = 0x0070CDF8u + 0x58u * (uint32_t) level;
        int big = U8(st, h + 3) > 1;
        static const uint8_t fields[4] = {0xA4, 0x9E, 0xA2, 0xA0};
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
    uint32_t kinds = 0x007900C2u + 0xC4u * U8(st, b + 0xC1);
    int32_t r = GAME_CALL(st, 0x004CDE60u, S16(st, b + 0x40), S32(st, 0x00705EB8u + 0x28u * (uint32_t) (int32_t) S16(st, b + 0x10)));
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
        if (U8(st, kinds + (uint32_t) g) == 3 && S16(st, b + 0x56u + 2u * (uint32_t) g) != 0) {
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
