/* Imhotep - water supply of the day (day step 0x1B).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/day_water.h"

#include "building/day_access.h"
#include "random/random.h"
#include "sim/math.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


void grid_and(game_state *st, uint32_t p, int32_t n, uint32_t mask)
{
    /* @bin 0x00455540 */
    for (int32_t i = 0; i < n; i++) {
        P32(st, p + 4u * (uint32_t) i) &= mask;
    }
}

int32_t building_road_ahead(game_state *st, int32_t id)
{
    /* @bin 0x00487E60 the offsets 0x005CD6C8 / 0x005CD6D8 by +0xAA from the building's tile */
    uint32_t b = BUILDING_ADDR(id);
    uint32_t dir = U8(st, b + BLD_ORIENTATION);
    int32_t tile = S32(st, b + BLD_GRID_OFFSET);
    if (U8(st, G_GRID_TERRAIN + 4u * (uint32_t) (tile + S32(st, G_AHEAD_OFFSETS_A + 4 * dir))) & 4) {
        return 1;
    }
    return (U8(st, G_GRID_TERRAIN + 4u * (uint32_t) (tile + S32(st, G_AHEAD_OFFSETS_B + 4 * dir))) & 4) != 0;
}

void map_terrain_or_around(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r, uint32_t mask)
{
    /* @bin 0x004824B0 clipped to the map; G_ROAD_ACCESS_X / Y and 0x00BC9768 end as the original's
     * loop counters */
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
    int32_t tile = S32(st, G_MAP_START) + y0 * GRID_SIZE + x0;
    S32(st, G_ROAD_ACCESS_Y) = y0;
    S32(st, G_TILE_CURSOR) = tile;
    if (y0 > y1) {
        return;
    }
    S32(st, G_ROAD_ACCESS_Y) = y1 + 1;
    for (int32_t ty = y0; ty <= y1; ty++) {
        S32(st, G_ROAD_ACCESS_X) = x0;
        if (x0 <= x1) {
            S32(st, G_ROAD_ACCESS_X) = x1 + 1;
            for (int32_t tx = x0; tx <= x1; tx++, tile++) {
                U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) |= mask;
            }
        }
        tile += x0 - x1 + 0xE3;
    }
    S32(st, G_TILE_CURSOR) = tile;
}

void map_grid_468790(game_state *st)
{
    /* @bin 0x00468790 tiles with terrain 0x100 and no road (4), empty or of a type-0x8A building:
     * grid 0x00AE8E7C := 0; for empty ones without bit 0x80 in grid 0x00B285CC, the i32 grid
     * 0x00B7482C grows by 0x30 up to 0x00F56B84 + 0x30 */
    int32_t limit = S32(st, G_CANAL_BASE) + 0x30;
    int32_t tile = S32(st, G_MAP_START);
    for (int32_t y = S32(st, G_MAP_HEIGHT); y > 0; y--) {
        for (int32_t x = S32(st, G_MAP_WIDTH); x > 0; x--, tile++) {
            uint32_t t = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
            if ((t & 4) || !(t & 0x100)) {
                continue;
            }
            int16_t bid = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
            if (bid != 0 && S16(st, BUILDING_ADDR(bid) + BLD_TYPE) != 0x8A) {
                continue;
            }
            uint8_t flags = U8(st, G_GRID_BITFIELDS + (uint32_t) tile);
            U8(st, G_CANAL + (uint32_t) tile) = 0;
            if ((flags & 0x80) || bid != 0) {
                continue;
            }
            int32_t v = S32(st, G_GRID_IMAGE + 4u * (uint32_t) tile);
            if (v < limit) {
                S32(st, G_GRID_IMAGE + 4u * (uint32_t) tile) = v + 0x30;
            }
        }
        tile += S32(st, G_MAP_STRIDE_EXTRA);
    }
}


void groundwater_spread(game_state *st, int32_t tile)
{
    /* @bin 0x00468590 from a groundwater tile (terrain 0x100), over the connected groundwater
     * tiles (breadth first, ring queue): each one's i32 grid 0x00B7482C (empty or type-0x8A
     * tiles, bit 0x80 of 0x00B285CC clear) loses 0x30 down to 0x00F56B84 + 0x30; type-0x5A
     * neighbours with +0x35 == 0 turn to 2 (redrawn, 0x00487EC0); each new neighbour marks
     * terrain 0x1000000 on the 5 x 5 tiles around it. A road tile stops the walk (the original
     * spins its step counter out there). */
    if (!(U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 0x100)) {
        return;
    }
    int32_t limit = S32(st, G_CANAL_BASE) + 0x30;
    for (uint32_t k = 0; k < 0x3E8; k++) {
        S32(st, G_TILE_QUEUE + 4 * k) = 0;
    }
    S32(st, G_TILE_QUEUE_HEAD) = 0;
    S32(st, G_TILE_QUEUE_TAIL) = 0;
    int32_t t = tile;
    for (int32_t steps = 1;; steps++) {
        if (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t) & 4) {
            return;
        }
        int16_t bid = S16(st, G_GRID_BUILDING + 2u * (uint32_t) t);
        if (bid == 0 || S16(st, BUILDING_ADDR(bid) + BLD_TYPE) == 0x8A) {
            int32_t v = S32(st, G_GRID_IMAGE + 4u * (uint32_t) t);
            if (v >= limit && !(U8(st, G_GRID_BITFIELDS + (uint32_t) t) & 0x80)) {
                S32(st, G_GRID_IMAGE + 4u * (uint32_t) t) = v - 0x30;
            }
        }
        U8(st, G_CANAL + (uint32_t) t) = 1;
        int32_t next = -1;
        for (uint32_t d = 0; d < 4; d++) {
            int32_t n = t + S32(st, G_NEIGHBOUR4_OFFSETS + 4 * d);
            int16_t nb = S16(st, G_GRID_BUILDING + 2u * (uint32_t) n);
            if (nb != 0 && S16(st, BUILDING_ADDR(nb) + BLD_TYPE) == 0x5A) {
                if (U8(st, BUILDING_ADDR(nb) + BLD_WATER_SUPPLY) == 0) {
                    U8(st, BUILDING_ADDR(nb) + BLD_WATER_SUPPLY) = 2;
                    water_lift_redraw(st, nb);
                }
                continue;
            }
            if (!(U32(st, G_GRID_TERRAIN + 4u * (uint32_t) n) & 0x100) || U8(st, G_CANAL + (uint32_t) n) != 0) {
                continue;
            }
            int32_t a = n - 0x1CA;
            for (int r = 0; r < 5; r++, a += GRID_SIZE) {
                for (int c = 0; c < 5; c++) {
                    U8(st, G_GRID_TERRAIN + 4u * (uint32_t) (a + c) + 3) |= 1;
                }
            }
            S32(st, G_TILE_CURSOR) = a;
            if (next == -1) {
                next = n;
            } else {
                int32_t tail = S32(st, G_TILE_QUEUE_TAIL);
                S32(st, G_TILE_QUEUE + 4u * (uint32_t) tail) = n;
                S32(st, G_TILE_QUEUE_TAIL) = tail + 1 < 0x3E8 ? tail + 1 : 0;
            }
        }
        if (next == -1) {
            int32_t head = S32(st, G_TILE_QUEUE_HEAD);
            if (head == S32(st, G_TILE_QUEUE_TAIL)) {
                return;
            }
            next = S32(st, G_TILE_QUEUE + 4u * (uint32_t) head);
            S32(st, G_TILE_QUEUE_HEAD) = head + 1 < 0x3E8 ? head + 1 : 0;
        }
        t = next;
        if (next <= -1 || steps >= 0xCB10) {
            return;
        }
    }
}

static void groundwater_from_object(game_state *st, uint32_t obj)
{
    /* @bin 0x004BECC0 (thiscall on a floodplain object) its tile (+8), when groundwater not yet
     * reached */
    int32_t t = (int32_t) P32(st, obj + 8);
    if ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t) & 0x100) && U8(st, G_CANAL + (uint32_t) t) == 0) {
        groundwater_spread(st, t);
    }
}

static void groundwater_list(game_state *st, uint32_t node)
{
    /* the list's objects (+0xC of each node, next at +0) until one is missing */
    while (node != 0) {
        uint32_t obj = P32(st, node + 0xC);
        node = P32(st, node);
        if (obj == 0) {
            return;
        }
        groundwater_from_object(st, obj);
    }
}

void flood_groundwater(game_state *st, uint32_t self)
{
    /* @bin 0x004BEB90 (thiscall on the floodplain manager 0x00D05720) the groundwater spreads
     * from the floodplain tiles of the current flood level (+0x32FAC, records of 0x1C bytes from
     * +0x32C64, their list at +0x10; at level 30 the list +0x32FA0 when +0x32F90 is set) and of
     * the two levels above */
    int32_t level = S32(st, self + 0x32FAC);
    if (level < 0x1E) {
        uint32_t rec = self + 0x32C64u + 0x1Cu * (uint32_t) level;
        if (P32(st, rec) != 0) {
            groundwater_list(st, P32(st, rec + 0x10));
        }
    } else if (P32(st, self + 0x32F90) != 0) {
        groundwater_list(st, P32(st, self + 0x32FA0));
    }
    for (int32_t up = 1; up <= 2; up++) {
        level = S32(st, self + 0x32FAC);
        if (level + up >= 0x1E) {
            continue;
        }
        uint32_t rec = self + 0x32C64u + 0x1Cu * (uint32_t) (level + up);
        if (P32(st, rec) != 0) {
            groundwater_list(st, P32(st, rec + 0x10));
        }
    }
}

int32_t map_flood_object_near(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00487DC0 a floodplain object (grid 0x00D05720) with +0x18 clear on the 2 x 2 tiles
     * at (x, y) */
    int32_t tile = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    static const int32_t around[4] = {0, 1, GRID_SIZE, GRID_SIZE + 1};
    for (int k = 0; k < 4; k++) {
        uint32_t obj = U32(st, G_FLOODPLAIN_MANAGER + 4u * (uint32_t) (tile + around[k]));
        if (obj != 0 && P8(st, obj + 0x18) == 0) {
            return 1;
        }
    }
    return 0;
}

int32_t building_flood_object_near(game_state *st, int32_t id)
{
    /* @bin 0x00487E30 */
    uint32_t b = BUILDING_ADDR(id);
    return map_flood_object_near(st, S16(st, b + BLD_X), S16(st, b + BLD_Y));
}

int32_t water_lift_image(game_state *st, int32_t dir, int32_t edge)
{
    /* @bin 0x00487D30 the image of a type-0x5A building by its direction, the map edge flag and
     * the flags 0x00BCA32C (+ 4, + 8 with 0x00BCA32D) */
    int32_t img = U8(st, G_WATER_LIFT_FLOOD) == 0 ? 0 : U8(st, G_WATER_LIFT_WATER) != 0 ? 8 : 4;
    static const uint32_t edge_img[4] = {G_IMAGE_GROUPS + 8 * 730, G_IMAGE_GROUPS + 8 * 731, G_IMAGE_GROUPS + 8 * 732, G_IMAGE_GROUPS + 8 * 733};
    static const uint32_t plain_img[4] = {G_IMAGE_GROUPS + 8 * 547, G_IMAGE_GROUPS + 8 * 537, G_IMAGE_GROUPS + 8 * 536, G_IMAGE_GROUPS + 8 * 535};
    int k = dir == 0 ? 0 : dir == 1 ? 1 : dir == 2 ? 2 : 3;
    return img + S32(st, (uint8_t) edge != 0 ? edge_img[k] : plain_img[k]);
}

void map_place_2x2(game_state *st, int32_t id, int32_t x, int32_t y, int32_t img)
{
    /* @bin 0x0047CF00 a 2 x 2 building footprint at (x, y) (offset table 0x005CC8C0): terrain
     * bit 8 (other bits but the mask 0x03052E80 cleared off roads), the building grid, grid
     * 0x00B285CC (& 0xE1 | 1), the image grid 0x00B7482C and the part grid 0x00B67D1C (bit 0x40
     * on the part chosen by the view mode 0x00E94E04); outside the map 0x00BC9734 := 1 */
    int32_t mode = S32(st, G_CITY_ORIENTATION);
    int32_t pj = mode == 0 || mode == 2 ? 0 : 1;
    int32_t pi = mode == 2 || mode == 4 ? 0 : 1;
    if (x < 0 || y < 0 || x + 1 >= S32(st, G_MAP_WIDTH) || y + 1 >= S32(st, G_MAP_HEIGHT)) {
        S32(st, G_OUT_OF_MAP) = 1;
        return;
    }
    int32_t tile = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    S32(st, G_TILE_CURSOR) = tile;
    for (int32_t i = 0; i < 2; i++) {
        for (int32_t j = 0; j < 2; j++) {
            uint32_t e = G_FOOTPRINT_OFFSETS + 0x30u * (uint32_t) i + 8u * (uint32_t) j;
            int32_t t = tile + S32(st, e);
            uint32_t a = G_GRID_TERRAIN + 4u * (uint32_t) t;
            uint32_t v = U32(st, a) | 8;
            U32(st, a) = v;
            if (!(v & 4)) {
                U32(st, a) = (v & 0x03052E80u) | 8;
            }
            S16(st, G_GRID_BUILDING + 2u * (uint32_t) t) = (int16_t) id;
            U8(st, G_GRID_BITFIELDS + (uint32_t) t) = (uint8_t) ((U8(st, G_GRID_BITFIELDS + (uint32_t) t) & 0xE1) | 1);
            S32(st, G_GRID_IMAGE + 4u * (uint32_t) t) = img;
            uint8_t part = U8(st, e + 4);
            U8(st, G_GRID_EDGE + (uint32_t) t) = part;
            if (j == pj && i == pi) {
                U8(st, G_GRID_EDGE + (uint32_t) t) = part | 0x40;
            }
        }
    }
}

void water_lift_redraw(game_state *st, int32_t id)
{
    /* @bin 0x00487EC0 */
    uint32_t b = BUILDING_ADDR(id);
    U8(st, G_WATER_LIFT_FLOOD) = (uint8_t) building_flood_object_near(st, id);
    U8(st, G_WATER_LIFT_WATER) = U8(st, b + BLD_WATER_SUPPLY) != 0;
    int32_t dir = U8(st, b + BLD_ORIENTATION) - S32(st, G_VIEW_ORIENTATION) / 2;
    if (dir < 0) {
        dir += 4;
    }
    uint8_t edge = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, S16(st, b + BLD_X), S16(st, b + BLD_Y));
    int32_t img = water_lift_image(st, dir, edge);
    map_place_2x2(st, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), img);
}

void day_water_4555d0(game_state *st)
{
    /* @bin 0x004555D0 terrain bits 0x1000000 and 0x2000 cleared; the type-0x5A buildings listed
     * (0x0078E178) with +0x35 := 2 when a road is ahead (0x00487E60, not while the map is being
     * revealed), redrawn when it changed; then the list is walked until no building turns from 2
     * to 1 (with workers: 0x0047CC70 and 0x00468590 on both sides); the working ones mark
     * terrain 0x1000000 around; types 0x5C / 0xB4 are redrawn (0x0047BF50) */
    grid_and(st, G_GRID_TERRAIN, 0xCB10, 0xFEFFDFFFu);
    map_grid_468790(st);
    for (uint32_t k = 0; k < 0x1F40; k++) {
        U8(st, G_HOUSE_LIST + k) = 0;
    }
    S32(st, G_HOUSE_LIST_INDEX) = 0;
    S32(st, G_HOUSE_LIST_COUNT) = 0;
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_TYPE) != 0x5A) {
            continue;
        }
        int32_t n = S32(st, G_HOUSE_LIST_COUNT);
        S16(st, G_HOUSE_LIST + 2u * (uint32_t) n) = (int16_t) id;
        S32(st, G_HOUSE_LIST_COUNT) = n + 1;
        uint8_t before = U8(st, b + BLD_WATER_SUPPLY);
        if (S32(st, G_BLOOD_RIVER_LEFT) == 0 && building_road_ahead(st, id)) {
            U8(st, b + BLD_WATER_SUPPLY) = 2;
        } else {
            U8(st, b + BLD_WATER_SUPPLY) = 0;
        }
        if (U8(st, b + BLD_WATER_SUPPLY) != before) {
            water_lift_redraw(st, id);
        }
    }
    flood_groundwater(st, G_FLOODPLAIN_MANAGER);
    int32_t count = S32(st, G_HOUSE_LIST_COUNT);
    int changed;
    do {
        changed = 0;
        for (int32_t i = 0; i < count;) {
            int32_t id = S16(st, G_HOUSE_LIST + 2u * (uint32_t) i);
            uint32_t b = BUILDING_ADDR(id);
            S32(st, G_HOUSE_LIST_INDEX) = ++i;
            if (U8(st, b + BLD_WATER_SUPPLY) != 2 || S16(st, b + BLD_WORKERS) <= 0) {
                continue;
            }
            U8(st, b + BLD_WATER_SUPPLY) = 1;
            changed = 1;
            GAME_CALL(st, FN_SHORE_FIT, S16(st, b + BLD_X), S16(st, b + BLD_Y), 0, 1);
            S32(st, G_BUILD_TURN_VIEW) = U8(st, b + BLD_ORIENTATION);
            groundwater_spread(st, S32(st, b + BLD_GRID_OFFSET) + S32(st, G_AHEAD_OFFSETS_A + 4u * (U32(st, G_BUILD_TURN_VIEW) ^ 2)));
            groundwater_spread(st, S32(st, b + BLD_GRID_OFFSET) + S32(st, G_AHEAD_OFFSETS_B + 4u * (U32(st, G_BUILD_TURN_VIEW) ^ 2)));
            i = S32(st, G_HOUSE_LIST_INDEX);
            count = S32(st, G_HOUSE_LIST_COUNT);
        }
    } while (changed);
    S32(st, G_HOUSE_LIST_INDEX) = 0;
    for (int32_t i = 0; i < count;) {
        int32_t id = S16(st, G_HOUSE_LIST + 2u * (uint32_t) i);
        uint32_t b = BUILDING_ADDR(id);
        S32(st, G_HOUSE_LIST_INDEX) = ++i;
        if (S16(st, b + BLD_WORKERS) > 0 && U8(st, b + BLD_WATER_SUPPLY) != 0) {
            map_terrain_or_around(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), 2, 2, 0x1000000);
            i = S32(st, G_HOUSE_LIST_INDEX);
            count = S32(st, G_HOUSE_LIST_COUNT);
        }
    }
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        int16_t type = S16(st, b + BLD_TYPE);
        if (U8(st, b + BLD_STATE) != 1 || (type != 0x5C && type != 0xB4)) {
            continue;
        }
        int32_t low = S8(st, G_GRID_DESIRABILITY + (uint32_t) S32(st, b + BLD_GRID_OFFSET)) < 0x1E;
        int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
        if (type == 0x5C) {
            GAME_CALL(st, FN_MAP_BUILDING_TILES, id, x, y, 1, 1, (low ? 0 : 2) + S32(st, G_IMAGE_GROUPS + 8 * 292), 8);
            map_terrain_or_around(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), 1, 2, 0x2000);
        } else {
            GAME_CALL(st, FN_MAP_BUILDING_TILES, id, x, y, 2, 2, (low ? 0 : 2) + S32(st, G_IMAGE_GROUPS + 8 * 338), 8);
        }
        U8(st, b + BLD_WATER_SUPPLY) = 1;
    }
}


static void fields_supply(game_state *st, int32_t low, int32_t high)
{
    /* @bin 0x0048643D type 0x6C (floodplain tiles, list from the start) and 0xC3 (the others,
     * from the end) working buildings with 0x005190A0, once each */
    for (int32_t id = 1; id <= S32(st, G_BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WORKERS) == 0) {
            continue;
        }
        int16_t type = S16(st, b + BLD_TYPE);
        if (type == 0x6C && U8(st, G_LIST_DONE_6C) == 0) {
            if (GAME_CALL(st, FN_ROUTE_WALK_FREE_TO_MARKED, S16(st, b + BLD_ROAD_ACCESS_X), S16(st, b + BLD_ROAD_ACCESS_Y), (int32_t) G_FULL_LIST, low) != 0) {
                U8(st, G_LIST_DONE_6C) = 1;
            }
        } else if (type == 0xC3 && U8(st, G_LIST_DONE_C3) == 0) {
            if (GAME_CALL(st, FN_ROUTE_WALK_FREE_TO_MARKED, S16(st, b + BLD_ROAD_ACCESS_X), S16(st, b + BLD_ROAD_ACCESS_Y), (int32_t) (G_FULL_LIST + 4u * (uint32_t) high),
                          0xF9F - high) != 0) {
                U8(st, G_LIST_DONE_C3) = 1;
            }
        }
    }
}

static void fertility_grow(game_state *st, int32_t tile, int32_t pct, uint32_t a, uint32_t b)
{
    /* @bin 0x004861BA one RNG step (inline in the original); + (rand15 % 10 + two globals) *
     * pct %, in 8 bits (it wraps: the original's clamp at 255 compares the wrapped byte) */
    random_next_state(st);
    int32_t v = S32(st, G_RANDOM_R1_15BIT) % 10 + S32(st, a) + S32(st, b);
    U8(st, G_GRID_VEGETATION + (uint32_t) tile) += (uint8_t) calc_percentage(v, pct);
    GAME_CALL(st, FN_TREE_IMAGE_UPDATE, tile);
}

void day_fertility_4860d0(game_state *st)
{
    /* @bin 0x004860D0 day step 0x14: fertility of irrigated (terrain 1) and floodplain
     * (0x40000) tiles, row by row over the map's rows (0x00BC9804 / 0x00BC9F24 / 0x00BC9B94);
     * at 255 they are listed for the buildings of 0x0048643D, by the 125 % bonus of the kingdom
     * building (0x004C5120 == 0 and 0x004C52D0(.., 2)) */
    uint32_t c = CURRENT_CITY(st);
    U8(st, G_LIST_DONE_6C) = 0;
    U8(st, G_IRRIGATED_SEEN) = 0;
    U8(st, G_LIST_DONE_C3) = 0;
    U8(st, G_FLOODPLAIN_SEEN) = 0;
    int32_t pct = 100, low = 0, high = 0xF9F;
    if (S16(st, c + CITY_TEMPLE_COMPLEX) != 0 && GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_TEMPLE_COMPLEX_ID)) == 0 &&
        (int8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_TEMPLE_COMPLEX_ID), 2) != 0) {
        pct = 125;
    }
    for (int32_t row = 0; row < S32(st, G_MAP_HEIGHT); row++) {
        int32_t from = S32(st, G_ROW_FIRST + 4u * (uint32_t) row);
        int32_t to = S32(st, G_ROW_END + 4u * (uint32_t) row);
        if (from >= to) {
            continue;
        }
        int32_t tile = S32(st, G_ROW_OFFSET + 4u * (uint32_t) row);
        for (int32_t n = to - from; n > 0; n--, tile++) {
            uint32_t t = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
            if (t & 1) {
                if (U8(st, G_GRID_VEGETATION + (uint32_t) tile) < 0xFF) {
                    fertility_grow(st, tile, pct, G_IRRIGATION_GROWTH_A, G_IRRIGATION_GROWTH_B);
                } else if (U8(st, G_LIST_DONE_6C) == 0) {
                    S32(st, G_FULL_LIST + 4u * (uint32_t) low) = tile;
                    low++;
                }
                U8(st, G_IRRIGATED_SEEN) = 1;
            } else if (t & 0x40000) {
                if (U8(st, G_GRID_VEGETATION + (uint32_t) tile) < 0xFF) {
                    fertility_grow(st, tile, pct, G_FLOODPLAIN_GROWTH_A, G_VEGETATION);
                } else if (U8(st, G_LIST_DONE_C3) == 0) {
                    S32(st, G_FULL_LIST + 4u * (uint32_t) high) = tile;
                    high--;
                }
                U8(st, G_FLOODPLAIN_SEEN) = 1;
            }
            if (low == high) {
                fields_supply(st, low, high);
                low = 0;
                high = 0xF9F;
            }
        }
    }
    if (low != 0 || high < 0xF9F) {
        fields_supply(st, low, high);
    }
}

static int32_t run_grid_and(game_state *st, const int32_t *a)
{
    grid_and(st, (uint32_t) a[0], a[1], (uint32_t) a[2]);
    return 0;
}

static int32_t run_road_ahead(game_state *st, const int32_t *a)
{
    return building_road_ahead(st, a[0]);
}

static int32_t run_or_around(game_state *st, const int32_t *a)
{
    map_terrain_or_around(st, a[0], a[1], a[2], a[3], (uint32_t) a[4]);
    return 0;
}

static int32_t run_468790(game_state *st, const int32_t *a)
{
    (void) a;
    map_grid_468790(st);
    return 0;
}

static int32_t run_water(game_state *st, const int32_t *a)
{
    (void) a;
    day_water_4555d0(st);
    return 0;
}

static int32_t run_fertility(game_state *st, const int32_t *a)
{
    (void) a;
    day_fertility_4860d0(st);
    return 0;
}

static int32_t run_groundwater(game_state *st, const int32_t *a)
{
    groundwater_spread(st, a[0]);
    return 0;
}

static int32_t run_flood_groundwater(game_state *st, const int32_t *a)
{
    (void) a;
    flood_groundwater(st, G_FLOODPLAIN_MANAGER);
    return 0;
}

static int32_t run_from_object(game_state *st, const int32_t *a)
{
    groundwater_from_object(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

static int32_t run_object_near(game_state *st, const int32_t *a)
{
    return map_flood_object_near(st, a[0], a[1]);
}

static int32_t run_building_object_near(game_state *st, const int32_t *a)
{
    return building_flood_object_near(st, a[0]);
}

static int32_t run_lift_image(game_state *st, const int32_t *a)
{
    return water_lift_image(st, a[0], a[1]);
}

static int32_t run_place_2x2(game_state *st, const int32_t *a)
{
    map_place_2x2(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_lift_redraw(game_state *st, const int32_t *a)
{
    water_lift_redraw(st, a[0]);
    return 0;
}

const ported_function day_water_ports[] = {
    {0x004BEB90u, "flood_groundwater", 0, 0, G_FLOODPLAIN_MANAGER, run_flood_groundwater, 0},
    {0x004BECC0u, "groundwater_from_object", 0, 0, 0, run_from_object, 0},
    {0x00487DC0u, "map_flood_object_near", 2, 32, 0, run_object_near, 0},
    {0x00487E30u, "building_flood_object_near", 1, 32, 0, run_building_object_near, 0},
    {0x00487D30u, "water_lift_image", 2, 32, 0, run_lift_image, 0},
    {0x0047CF00u, "map_place_2x2", 4, 0, 0, run_place_2x2, 0},
    {0x00487EC0u, "water_lift_redraw", 1, 0, 0, run_lift_redraw, 0},
    {0x00468590u, "groundwater_spread", 1, 0, 0, run_groundwater, 0},
    {0x004860D0u, "day_fertility_4860d0", 0, 0, 0, run_fertility, 0},
    {0x00455540u, "grid_and", 3, 0, 0, run_grid_and, 0},
    {0x00487E60u, "building_road_ahead", 1, 32, 0, run_road_ahead, 0},
    {0x004824B0u, "map_terrain_or_around", 5, 0, 0, run_or_around, 0},
    {0x00468790u, "map_grid_468790", 0, 0, 0, run_468790, 0},
    {0x004555D0u, "day_water_4555d0", 0, 0, 0, run_water, 0},
};
const int day_water_ports_count = sizeof(day_water_ports) / sizeof(day_water_ports[0]);
