/* Imhotep - what the game does before reading a saved game (Pharaoh.exe 0x004D5100 prologue).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/pre_load.h"

#include <string.h>

#include "map/floodplain.h"
#include "random/random.h"
#include "state/layout.h"

void floodplain_settings_reset(game_state *st)
{
    /* @bin 0x004BD330 (this = 0x00D38360, set at 0x004D5100) eight dwords */
    static const int32_t v[8] = {0xB4, 0x46, 100, 0xB4, 0x46, 100, 0, 100};
    for (uint32_t i = 0; i < 8; i++) {
        S32(st, G_FLOODPLAIN + 4 * i) = v[i];
    }
}

/* @bin 0x0046F930 is (x, y) inside the diamond-shaped map (map row globals of 0x0046F7B0) */
int map_tile_inside(const game_state *st, int32_t x, int32_t y)
{
    game_state *s = (game_state *) st;
    int32_t cx = S32(s, G_MAP_CENTER_X), cy = S32(s, G_MAP_CENTER_Y);
    if (cx < x) {
        if (y <= cy) {
            return x - y <= S32(s, G_MAP_XE) - cy;
        }
        return x + y <= S32(s, G_MAP_XE) + 1 + cy;
    }
    if (y <= cy) {
        return S32(s, G_VIEW_DX) + cy <= x + y;
    }
    return cx - S32(s, G_MAP_YE) <= x - y;
}

/* @bin 0x00484EE0 byte +0x0E := 0 in n 16-byte entries */
static void clear_entries(game_state *st, uint32_t table, int n)
{
    for (int i = 0; i < n; i++) {
        U8(st, table + PATTERN_COUNTER + 16u * (uint32_t) i) = 0;
    }
}

void pre_load_clear(game_state *st)
{
    /* @bin 0x0046FA40 grids cleared (address, bytes) */
    static const uint32_t grids[][2] = {
        {G_IMAGE, 0x32C40}, {G_EDGE, 0xCB10}, {G_BUILDING_GRID, 0x19620}, {G_GRID_TERRAIN, 0x32C40},
        {G_FERTILITY_GRID, 0xCB10}, {G_CANAL, 0xCB10}, {G_FIGURE_GRID, 0x19620}, {G_DAY2_DESTROY_BITS, 0xCB10},
        {G_SPRITE, 0xCB10}, {G_RANDOM, 0xCB10}, {G_DESIRABILITY_GRID, 0xCB10}, {G_ELEVATION_GRID, 0xCB10},
        {G_GRID_DAMAGE, 0x19620}, {G_GRID_RUBBLE, 0xCB10}, {G_THREAT, 0xCB10}, {G_GRID_ROAD_NETWORK, 0xCB10},
        {G_NETWORK2, 0xCB10}, {G_GRID_MOISTURE, 0xCB10},
    };
    for (size_t i = 0; i < sizeof(grids) / sizeof(grids[0]); i++) {
        memset(state_ptr(st, grids[i][0], grids[i][1]), 0, grids[i][1]);
    }
    /* @bin 0x0046FA40 0x00A43EAC: 0x32C4 dwords := -1 */
    memset(state_ptr(st, G_UA43EAC, 0x32C4u * 4), 0xFF, 0x32C4u * 4);
    floodplain_manager_reset(st);
    /* @bin 0x0046FA40 tables whose entries' byte +0x0E is cleared, with their counts */
    static const uint32_t tables[][2] = {
        {G_PATTERN_SHORE, 0x30}, {G_U5CA958, 0x30}, {G_PATTERN_FLOODPLAIN, 0x18}, {G_PATTERN_WALLS, 0x30},
        {G_PATTERN_WALLS_BUILDING, 0x0F}, {G_U5CBDC8, 0x0E}, {G_PATTERN_SPECIAL_ROCKS, 0x11}, {G_U5CC158, 0x11},
        {G_PATTERN_ROADS, 0x11}, {G_PATTERN_PAVED_ROADS, 0x30}, {G_PATTERN_CANALS, 0x10},
    };
    for (size_t i = 0; i < sizeof(tables) / sizeof(tables[0]); i++) {
        clear_entries(st, tables[i][0], (int) tables[i][1]);
    }
    /* @bin 0x0046FC30 terrain := 0x80005 on every tile outside the map (current map globals) */
    for (int32_t y = 0; y < 0xE4; y++) {
        for (int32_t x = 0; x < 0xE4; x++) {
            if (!map_tile_inside(st, x, y)) {
                U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (y * 0xE4 + x)) = 0x80005u;
            }
        }
    }
    /* @bin 0x0046FC80 random grid (chunk 15): one RNG step per tile, low byte of the 15-bit value */
    for (uint32_t o = 0; o < 0xE4u * 0xE4u; o++) {
        random_next_state(st);
        U8(st, G_RANDOM + o) = (uint8_t) U32(st, G_R_1_15);
    }
    /* @bin 0x00475B80 image grid := random & 7 + base image 0x00F56B54 on the map's rows, and
     * bitfields |= 0x20 where the random value is odd */
    for (int32_t r = 0; r < S32(st, G_MAP_HEIGHT); r++) {
        int32_t lo = S32(st, G_ROW_FIRST + 4u * (uint32_t) r), hi = S32(st, G_ROW_END + 4u * (uint32_t) r);
        int32_t o = S32(st, G_ROW_OFFSET + 4u * (uint32_t) r);
        for (int32_t k = lo; k < hi; k++, o++) {
            uint8_t v = U8(st, G_RANDOM + (uint32_t) o);
            S32(st, G_IMAGE + 4u * (uint32_t) o) = (v & 7) + S32(st, G_PLAIN);
            if (v & 1) {
                U8(st, G_DAY2_DESTROY_BITS + (uint32_t) o) |= 0x20;
            }
        }
    }
}
