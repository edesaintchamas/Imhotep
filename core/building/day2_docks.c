/* Imhotep - docks and storage helpers (package day2): the dock a boat figure is sent to
 * (0x004808A0, 0x004809B0, called by 0x00491A90) and the filling of a building's stock
 * (0x00460890, called by 0x0048B620).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/day2_docks.h"

#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static uint32_t dock_slot(game_state *st, uint32_t k)
{
    /* the city's 10 docks: i16 at city +0x4460 (0x00EA5CD0) */
    return CITY_BASE + CITY_U4460 + 2u * ((uint32_t) S32(st, CURRENT_PLAYER_ADDR) * 0x24ECu + k);
}

int32_t dock_reserve(game_state *st, int32_t f)
{
    /* @bin 0x004808A0 from the figure's dock index (+0x7A) on, the first dock free (+0xB4 0) or
     * reserved by f; when none is, the last one read is taken anyway when it exists (the loop
     * leaves it in eax). The mooring tile by the dock's orientation (+0xAA) into
     * 0x00D3B4E4 / 0x00D3B4E0, and the dock is reserved (+0xB4 := f) */
    if (S32(st, G_DOCK_COUNT) <= 0) {
        return 0;
    }
    uint32_t k = U8(st, FIGURE_ADDR(f) + FIG_COLLECTING);
    if (k >= 10) {
        return 0;
    }
    int32_t d = 0;
    for (; k < 10; k++) {
        d = S16(st, dock_slot(st, k));
        if (d > 0) {
            int16_t r = S16(st, BUILDING_ADDR(d) + BLD_DOCK_BOAT);
            if (r == 0 || r == f) {
                break;
            }
        }
    }
    if (d <= 0) {
        return 0;
    }
    uint32_t b = BUILDING_ADDR(d);
    int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
    switch (U8(st, b + BLD_ORIENTATION)) {
    case 0: x += 1; y -= 1; break;
    case 1: x += 3; y += 1; break;
    case 2: x += 1; y += 3; break;
    default: x -= 1; y += 1; break;
    }
    S32(st, G_ROAD_ACCESS_X) = x;
    S16(st, b + BLD_DOCK_BOAT) = (int16_t) f;
    S32(st, G_ROAD_ACCESS_Y) = y;
    return d;
}

static int32_t mooring_free(game_state *st, int32_t d, int32_t far)
{
    /* the tile `far` (1 or 2) steps out from the dock, by its orientation; free of figures? */
    uint32_t b = BUILDING_ADDR(d);
    int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
    switch (U8(st, b + BLD_ORIENTATION)) {
    case 0: x += 1; y -= 1 + far; break;
    case 1: x += 3 + far; y += 1; break;
    case 2: x += 1; y += 3 + far; break;
    default: x -= 1 + far; y += 1; break;
    }
    S32(st, G_ROAD_ACCESS_X) = x;
    S32(st, G_ROAD_ACCESS_Y) = y;
    return S16(st, G_FIGURE_GRID + 2u * (uint32_t) (S32(st, G_MAP_START) + y * 0xE4 + x)) == 0;
}

int32_t dock_free_moorings(game_state *st, int32_t f)
{
    /* @bin 0x004809B0 from the figure's dock index on, the first dock whose tile 2 steps out is
     * free of figures, else 3 steps out; the tile in 0x00D3B4E4 / 0x00D3B4E0 */
    if (S32(st, G_DOCK_COUNT) <= 0) {
        return 0;
    }
    uint32_t k0 = U8(st, FIGURE_ADDR(f) + FIG_COLLECTING);
    for (int far = 1; far <= 2; far++) {
        for (uint32_t k = k0; k < 10; k++) {
            int32_t d = S16(st, dock_slot(st, k));
            if (d > 0 && mooring_free(st, d, far)) {
                return d;
            }
        }
    }
    return 0;
}

static int32_t capacity(int16_t type)
{
    /* @bin 0x00460A78 (the same table six times) */
    switch (type) {
    case 0x20: case 0x33: return 300;
    case 0x2F: case 0x35: return 500;
    case 0x54: case 0x55: case 0x56: case 0x57: case 0xB8: case 0xBB: case 0xBC: case 0xBD: return 100000;
    default: return 200;
    }
}

int32_t building_fill(game_state *st, int32_t b, int32_t amount, int32_t kind)
{
    /* @bin 0x00460890 adds up to the type's capacity to the stock (+0x3C for kind 10, else
     * +0xCC; i16) and counts one delivery down (+0x104 / +0x106); returns what was added, 0 when
     * full, 1 for no building */
    if (b <= 0) {
        return 1;
    }
    uint32_t r = BUILDING_ADDR(b);
    uint32_t stock = kind == 10 ? r + BLD_STORED : r + BLD_STOCK2;
    uint32_t count = kind == 10 ? r + BLD_INCOMING_CARTS : r + BLD_INCOMING_AMOUNT;
    int16_t type = S16(st, r + BLD_TYPE);
    int16_t have = S16(st, stock);
    if (have >= capacity(type)) {
        return 0;
    }
    if (have + amount > capacity(type)) {
        amount += have - capacity(type);
    }
    S16(st, stock) = (int16_t) (have + amount);
    S16(st, count) = (int16_t) (S16(st, count) - 1);
    return amount;
}

static int32_t run_reserve(game_state *st, const int32_t *a)
{
    return dock_reserve(st, a[0]);
}

static int32_t run_free(game_state *st, const int32_t *a)
{
    return dock_free_moorings(st, a[0]);
}

static int32_t run_fill(game_state *st, const int32_t *a)
{
    return building_fill(st, a[0], a[1], a[2]);
}

const ported_function day2_docks_ports[] = {
    {0x004808A0u, "dock_reserve", 1, 32, 0, run_reserve, 0},
    {0x004809B0u, "dock_free_moorings", 1, 32, 0, run_free, 0},
    {0x00460890u, "building_fill", 3, 32, 0, run_fill, 0},
};
const int day2_docks_ports_count = sizeof(day2_docks_ports) / sizeof(day2_docks_ports[0]);
