/* Imhotep - road access of buildings, as checked by the daily passes.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/day_access.h"

#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


int32_t building_road_access(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x00480180 */
    int32_t best_rank = 0xC;
    int32_t base = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    int32_t best = base; /* the original's initial value, never used when nothing is found */
    uint32_t offsets = G_RING_OFFSETS + 0x60u * (uint32_t) size;
    for (int32_t i = 0; i < 0x18; i++) {
        int32_t d = S32(st, offsets + 4u * (uint32_t) i);
        if (d == 0) {
            break;
        }
        int32_t tile = base + d;
        uint32_t terrain = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
        if (terrain & 8) {
            /* @bin 0x004801F0 a building tile of type 0x8A or 0x88 is not a road */
            int16_t type = S16(st, BUILDING_ADDR(S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile)) + BLD_TYPE);
            if (type == 0x8A || type == 0x88) {
                continue;
            }
        }
        if (!(terrain & 0x40) || (terrain & 4)) {
            continue;
        }
        /* @bin 0x00480210 the rank of the tile's network in the city's list (10 entries of 8
         * bytes at +0x46AC), 0xB when absent */
        uint32_t network = U8(st, G_GRID_ROAD_NETWORK + (uint32_t) tile);
        uint32_t list = CURRENT_CITY(st) + CITY_ROAD_NETWORKS;
        int32_t rank = 0xB;
        for (int32_t k = 0; k < 10; k++) {
            if (U32(st, list + 8u * (uint32_t) k) == network) {
                rank = k;
                break;
            }
        }
        if (rank < best_rank) {
            best_rank = rank;
            best = i;
        }
    }
    if (best_rank >= 0xC) {
        return 0;
    }
    uint32_t c = G_RING_COORDS + 8u * ((uint32_t) best + 0x18u * (uint32_t) size);
    S32(st, G_ROAD_ACCESS_X) = S32(st, c) + x;
    S32(st, G_ROAD_ACCESS_Y) = S32(st, c + 4) + y;
    return 1;
}

int32_t building_main_part(game_state *st, int32_t id)
{
    /* @bin 0x00475B50 */
    for (int32_t n = 0; n < 0x20D; n++) {
        int16_t up = S16(st, BUILDING_ADDR(id) + BLD_PREV_PART);
        if (up <= 0) {
            return id;
        }
        id = up;
    }
    return 0;
}

int32_t building_type_0x487f80(int32_t type)
{
    /* @bin 0x00487F80 (thiscall, `this` unused) types 0x64..0x69, 0xC4 and 0xE0 */
    switch (type) {
    case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0xC4: case 0xE0: return 1;
    default: return 0;
    }
}

int32_t building_is_type_0x487f80(game_state *st, int32_t id)
{
    /* @bin 0x00488030 (thiscall on 0x00BCA350) */
    return building_type_0x487f80(S16(st, BUILDING_ADDR(id) + BLD_TYPE));
}

int32_t map_area_test(game_state *st, int32_t x, int32_t y, int32_t width, int32_t rows, uint32_t mask)
{
    /* @bin 0x004893C0 the tiles of offset table 0x005CC8C0 (rows of 0x30 bytes, entries of 8)
     * around (x, y): a tile with terrain & mask, or (mask 0x10000) an object in grid 0x00D05720
     * whose byte +0x18 is 0 */
    int32_t base = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    S32(st, G_TILE_CURSOR) = base;
    for (int32_t i = 0; i < rows; i++) {
        uint32_t row = G_FOOTPRINT_OFFSETS + 0x30u * (uint32_t) i;
        for (int32_t j = 0; j < width; j++) {
            int32_t tile = base + S32(st, row + 8u * (uint32_t) j);
            if (mask & 0x10000) {
                uint32_t obj = U32(st, G_FLOODPLAIN_MANAGER + 4u * (uint32_t) tile);
                if (obj != 0 && P8(st, obj + 0x18) == 0) {
                    return 1;
                }
            }
            if (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & mask) {
                return 1;
            }
        }
    }
    return 0;
}

int32_t building_area_test_3x3(game_state *st, int32_t id)
{
    /* @bin 0x004894A0 (thiscall on 0x00BCA350) -> 0x00489470(x, y, 3, 3) (stdcall) ->
     * 0x004893C0(x, y, 3, 3, 0x10000) */
    uint32_t b = BUILDING_ADDR(id);
    return map_area_test(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), 3, 3, 0x10000);
}

int32_t building_needs_area_check(game_state *st, int32_t id)
{
    /* @bin 0x00488050 (thiscall on 0x00BCA350) */
    return building_is_type_0x487f80(st, id) && building_area_test_3x3(st, id);
}

int32_t map_area_has_terrain(game_state *st, int32_t x, int32_t y, int32_t size, uint32_t mask)
{
    /* @bin 0x004816B0 a tile with terrain & mask in the square (x - size + 1 .. x + size - 1)^2
     * (y likewise), inside the map */
    int32_t x0 = x + 1 - size, x1 = x + size - 1;
    int32_t y0 = y + 1 - size, y1 = y + size - 1;
    for (int32_t ty = y0; ty <= y1; ty++) {
        for (int32_t tx = x0; tx <= x1; tx++) {
            if (tx < 0 || ty < 0 || tx >= S32(st, G_MAP_WIDTH) || ty >= S32(st, G_MAP_HEIGHT)) {
                continue;
            }
            int32_t tile = S32(st, G_MAP_START) + ty * GRID_SIZE + tx;
            if (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & mask) {
                return 1;
            }
        }
    }
    return 0;
}

void map_mark_buildings_around(game_state *st, int32_t id, int32_t r)
{
    /* @bin 0x0047FC40 +0x3F := 1 for every building on the tiles within r of the building's
     * corner, clipped to the map; G_ROAD_ACCESS_X / Y end as the loop counters do */
    uint32_t b = BUILDING_ADDR(id);
    int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
    int32_t x0 = x - r, x1 = x + r, y0 = y - r, y1 = y + r;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= S32(st, G_MAP_WIDTH)) {
        x1 = S32(st, G_MAP_WIDTH) - 1;
    }
    if (y1 >= S32(st, G_MAP_HEIGHT)) {
        y1 = S32(st, G_MAP_HEIGHT) - 1;
    }
    S32(st, G_ROAD_ACCESS_Y) = y0;
    if (y0 > y1) {
        return;
    }
    S32(st, G_ROAD_ACCESS_Y) = y1 + 1;
    int32_t tile = S32(st, G_MAP_START) + y0 * GRID_SIZE + x0;
    for (int32_t ty = y0; ty <= y1; ty++) {
        S32(st, G_ROAD_ACCESS_X) = x0;
        if (x0 <= x1) {
            S32(st, G_ROAD_ACCESS_X) = x1 + 1;
            for (int32_t tx = x0; tx <= x1; tx++, tile++) {
                int16_t bid = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
                if (bid != 0) {
                    U8(st, BUILDING_ADDR(bid) + BLD_WELL_ACCESS) = 1;
                }
            }
            tile += x0 - x1 + 0xE3; /* with the x1 - x0 + 1 tiles: the next row */
        } else {
            tile += x0 - x1 + 0xE3;
        }
    }
}

int32_t building_road_in_ring(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r, int32_t no_road)
{
    /* @bin 0x0047FEB0 the best road tile (terrain & 0x44 == 0x40, network ranked among the city's
     * first five) in the square around a building of `size` at (x, y), r tiles wide, clipped to
     * the map; none when no_road is set. Found: 1, with G_ROAD_ACCESS_X / Y and 0x00BC9768 (its
     * grid offset). The globals otherwise end as the original's loop counters. */
    int32_t x0 = x - r, x1 = x + size + r - 1;
    int32_t y0 = y - r, y1 = y + size + r - 1;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= S32(st, G_MAP_WIDTH)) {
        x1 = S32(st, G_MAP_WIDTH) - 1;
    }
    if (y1 >= S32(st, G_MAP_HEIGHT)) {
        y1 = S32(st, G_MAP_HEIGHT) - 1;
    }
    int32_t best = 0xC, best_x = x0, best_y = y0;
    int32_t tile = S32(st, G_MAP_START) + y0 * GRID_SIZE + x0;
    int32_t best_tile = tile;
    int32_t stride = x0 - x1 + 0xE3;
    S32(st, G_ROAD_ACCESS_Y) = y0;
    S32(st, G_TILE_CURSOR) = tile;
    if (y0 > y1) {
        return 0;
    }
    int32_t ty = y0;
    for (; ty <= y1; ty++) {
        S32(st, G_ROAD_ACCESS_X) = x0;
        if (x0 <= x1) {
            int32_t tx = x0;
            for (; tx <= x1; tx++, tile++) {
                if ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 0x44) != 0x40 || no_road != 0) {
                    continue;
                }
                uint32_t network = U8(st, G_GRID_ROAD_NETWORK + (uint32_t) tile);
                uint32_t list = CURRENT_CITY(st) + CITY_ROAD_NETWORKS;
                int32_t rank = 0xB;
                for (int32_t k = 0; k < 5; k++) {
                    if (U32(st, list + 8u * (uint32_t) k) == network) {
                        rank = k;
                        break;
                    }
                }
                if (rank < best) {
                    best = rank;
                    best_tile = tile;
                    best_x = tx;
                    best_y = ty;
                }
            }
            S32(st, G_ROAD_ACCESS_X) = tx;
        }
        tile += stride;
    }
    S32(st, G_TILE_CURSOR) = tile;
    S32(st, G_ROAD_ACCESS_Y) = ty;
    if (best >= 0xC) {
        return 0;
    }
    S32(st, G_TILE_CURSOR) = best_tile;
    S32(st, G_ROAD_ACCESS_X) = best_x;
    S32(st, G_ROAD_ACCESS_Y) = best_y;
    return 1;
}

int32_t building_road_within(game_state *st, int32_t x, int32_t y, int32_t size, int32_t radius)
{
    /* @bin 0x0047FE60 building_road_in_ring for r = 1 .. radius */
    for (int32_t r = 1; r <= radius; r++) {
        if (building_road_in_ring(st, x, y, size, r, 0)) {
            return 1;
        }
    }
    return 0;
}


static int32_t network_rank(game_state *st, int32_t tile, int32_t entries)
{
    uint32_t network = U8(st, G_GRID_ROAD_NETWORK + (uint32_t) tile);
    uint32_t list = CURRENT_CITY(st) + CITY_ROAD_NETWORKS;
    for (int32_t k = 0; k < entries; k++) {
        if (U32(st, list + 8u * (uint32_t) k) == network) {
            return k;
        }
    }
    return 0xB;
}

int32_t building_reached_road(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x00481060 like building_road_access, among the road tiles (terrain 0x40 without 4)
     * reached by the last distance search; failing that the nearest reached tile of any kind.
     * Returns its grid offset + 1 (G_ROAD_ACCESS_X / Y), or 0 */
    int32_t base = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    uint32_t offsets = G_RING_OFFSETS + 0x60u * (uint32_t) size;
    int32_t best_rank = 0xC, best = base;
    for (int32_t i = 0; i < 0x18; i++) {
        int32_t d = S32(st, offsets + 4u * (uint32_t) i);
        if (d == 0) {
            break;
        }
        int32_t t = base + d;
        uint32_t terrain = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t);
        if (!(terrain & 0x40) || (terrain & 4) || S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) t) <= 0) {
            continue;
        }
        int32_t rank = network_rank(st, t, 10);
        if (rank < best_rank) {
            best_rank = rank;
            best = i;
        }
    }
    if (best_rank < 0xC) {
        uint32_t c = G_RING_COORDS + 8u * ((uint32_t) best + 0x18u * (uint32_t) size);
        S32(st, G_ROAD_ACCESS_X) = S32(st, c) + x;
        S32(st, G_ROAD_ACCESS_Y) = S32(st, c + 4) + y;
        return S32(st, offsets + 4u * (uint32_t) best) + base + 1;
    }
    /* @bin 0x0048116E the nearest reached tile */
    int32_t found = -1, nearest = 100000, fx = 0, fy = 0;
    for (int32_t i = 0; i < 0x18; i++) {
        int32_t d = S32(st, offsets + 4u * (uint32_t) i);
        if (d == 0) {
            break;
        }
        int32_t t = base + d;
        int32_t v = S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) t);
        if (v > 0 && v < nearest) {
            uint32_t c = G_RING_COORDS + 8u * ((uint32_t) i + 0x18u * (uint32_t) size);
            fx = S32(st, c) + x;
            fy = S32(st, c + 4) + y;
            found = t;
            nearest = v;
        }
    }
    if (found < 0) {
        return 0;
    }
    S32(st, G_ROAD_ACCESS_X) = fx;
    S32(st, G_ROAD_ACCESS_Y) = fy;
    return found + 1;
}

int32_t road_next_to_4802c0(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x004802C0 the best road tile (as building_road_access) among the eight at distance
     * 1 and 2 in line (a table on the original's stack: offsets -228, 1, 228, -1, -456, 2, 456,
     * -2 with the coordinates (0, -1), (1, 0), (-1, 0), (0, 1), (0, -2), (2, 0), (-2, 0), (0, 2),
     * as written there) */
    static const int32_t offsets[8] = {-228, 1, 228, -1, -456, 2, 456, -2};
    static const int32_t dxy[8][2] = {{0, -1}, {1, 0}, {-1, 0}, {0, 1}, {0, -2}, {2, 0}, {-2, 0}, {0, 2}};
    int32_t base = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    int32_t best_rank = 0xC, best = base;
    for (int32_t i = 0; i < 8; i++) {
        int32_t t = base + offsets[i];
        uint32_t terrain = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t);
        if (terrain & 8) {
            int16_t type = S16(st, BUILDING_ADDR(S16(st, G_GRID_BUILDING + 2u * (uint32_t) t)) + BLD_TYPE);
            if (type == 0x8A || type == 0x88) {
                continue;
            }
        }
        if (!(terrain & 0x40)) {
            continue;
        }
        int32_t rank = network_rank(st, t, 10);
        if (rank < best_rank) {
            best_rank = rank;
            best = i;
        }
    }
    if (best_rank >= 0xC) {
        return 0;
    }
    S32(st, G_ROAD_ACCESS_X) = dxy[best][0] + x;
    S32(st, G_ROAD_ACCESS_Y) = dxy[best][1] + y;
    return 1;
}

static int32_t run_road_access(game_state *st, const int32_t *a)
{
    return building_road_access(st, a[0], a[1], a[2]);
}

static int32_t run_main_part(game_state *st, const int32_t *a)
{
    return building_main_part(st, a[0]);
}

static int32_t run_type_487f80(game_state *st, const int32_t *a)
{
    (void) st;
    return building_type_0x487f80(a[0]);
}

static int32_t run_is_type_487f80(game_state *st, const int32_t *a)
{
    return building_is_type_0x487f80(st, a[0]);
}

static int32_t run_area_test(game_state *st, const int32_t *a)
{
    return map_area_test(st, a[0], a[1], a[2], a[3], (uint32_t) a[4]);
}

static int32_t run_area_test_489470(game_state *st, const int32_t *a)
{
    return map_area_test(st, a[0], a[1], a[2], a[3], 0x10000);
}

static int32_t run_area_test_3x3(game_state *st, const int32_t *a)
{
    return building_area_test_3x3(st, a[0]);
}

static int32_t run_needs_area_check(game_state *st, const int32_t *a)
{
    return building_needs_area_check(st, a[0]);
}

static int32_t run_area_has_terrain(game_state *st, const int32_t *a)
{
    return map_area_has_terrain(st, a[0], a[1], a[2], (uint32_t) a[3]);
}

static int32_t run_mark_buildings_around(game_state *st, const int32_t *a)
{
    map_mark_buildings_around(st, a[0], a[1]);
    return 0;
}

static int32_t run_road_in_ring(game_state *st, const int32_t *a)
{
    return building_road_in_ring(st, a[0], a[1], a[2], a[3], a[4]);
}

static int32_t run_road_within(game_state *st, const int32_t *a)
{
    return building_road_within(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_reached_road(game_state *st, const int32_t *a)
{
    return building_reached_road(st, a[0], a[1], a[2]);
}

static int32_t run_next_to(game_state *st, const int32_t *a)
{
    return road_next_to_4802c0(st, a[0], a[1]);
}

const ported_function day_access_ports[] = {
    {0x00481060u, "building_reached_road", 3, 32, 0, run_reached_road, 0},
    {0x004802C0u, "road_next_to_4802c0", 2, 32, 0, run_next_to, 0},
    {0x00487F80u, "building_type_0x487f80", 1, 8, G_BUILDING_MANAGER, run_type_487f80, 0},
    {0x00488030u, "building_is_type_0x487f80", 1, 8, G_BUILDING_MANAGER, run_is_type_487f80, 0},
    {0x004893C0u, "map_area_test", 5, 8, 0, run_area_test, 0},
    {0x00489470u, "map_area_test_489470", 4, 8, 0, run_area_test_489470, 0},
    {0x004894A0u, "building_area_test_3x3", 1, 8, G_BUILDING_MANAGER, run_area_test_3x3, 0},
    {0x00488050u, "building_needs_area_check", 1, 32, G_BUILDING_MANAGER, run_needs_area_check, 0},
    {0x004816B0u, "map_area_has_terrain", 4, 32, 0, run_area_has_terrain, 0},
    {0x0047FC40u, "map_mark_buildings_around", 2, 0, 0, run_mark_buildings_around, 0},
    {0x0047FEB0u, "building_road_in_ring", 5, 32, 0, run_road_in_ring, 0},
    {0x0047FE60u, "building_road_within", 4, 32, 0, run_road_within, 0},
    {0x00480180u, "building_road_access", 3, 32, 0, run_road_access, 0},
    {0x00475B50u, "building_main_part", 1, 32, 0, run_main_part, 0},
};
const int day_access_ports_count = sizeof(day_access_ports) / sizeof(day_access_ports[0]);
