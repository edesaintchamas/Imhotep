/* Imhotep - a building's tiles on the map grids (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "map/map_building.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

void map_building_tiles(game_state *st, int32_t id, int32_t x, int32_t y, int32_t sx, int32_t sy, int32_t image,
                        uint32_t flags)
{
    /* @bin 0x0047BF50 the tile that gets edge bit 0x40, by view orientation */
    int32_t cx, cy;
    switch (S32(st, G_UE94E04)) {
    case 0: cx = 0; cy = sy - 1; break;
    case 2: cx = 0; cy = 0; break;
    case 4: cx = sx - 1; cy = 0; break;
    default: cx = sx - 1; cy = sy - 1; break;
    }
    if (x < 0 || y < 0 || sx + x - 1 >= S32(st, G_MAP_WIDTH) || y + sy - 1 >= S32(st, G_MAP_HEIGHT)) {
        S32(st, G_OUT_OF_MAP) = 1;
        return;
    }
    int32_t base = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    S32(st, G_TILE_CURSOR) = base;
    for (int32_t r = 0; r < sy; r++) {
        for (int32_t c = 0; c < sx; c++) {
            uint32_t e = G_FOOTPRINT_OFFSETS + 0x30u * (uint32_t) r + 8u * (uint32_t) c;
            uint32_t t = (uint32_t) (S32(st, e) + base);
            U32(st, G_GRID_TERRAIN + 4 * t) = (U32(st, G_GRID_TERRAIN + 4 * t) & 0x03052E80u) | flags;
            U16(st, G_GRID_BUILDING + 2 * t) = (uint16_t) id;
            uint8_t s = U8(st, G_GRID_BITFIELDS + t) & 0xE0;
            U8(st, G_GRID_BITFIELDS + t) = s;
            if (sx >= 2 && sx <= 6) {
                U8(st, G_GRID_BITFIELDS + t) = (uint8_t) (s | (sx - 1)); /* @bin 0x0047C08D */
            }
            S32(st, G_GRID_IMAGE + 4 * t) = image;
            uint8_t edge = U8(st, e + 4);
            U8(st, G_GRID_EDGE + t) = edge;
            if (c == cx && r == cy) {
                U8(st, G_GRID_EDGE + t) = edge | 0x40;
            }
        }
    }
    GAME_CALL(st, FN_TERRAIN_LAND_AROUND, x, y, sx);
}

static int32_t run_tiles(game_state *st, const int32_t *a)
{
    map_building_tiles(st, a[0], a[1], a[2], a[3], a[4], a[5], (uint32_t) a[6]);
    return 0;
}

const ported_function bigb_map_ports[] = {
    {0x0047BF50u, "map_building_tiles", 7, 0, 0, run_tiles, 0},
};
const int bigb_map_ports_count = sizeof(bigb_map_ports) / sizeof(bigb_map_ports[0]);
