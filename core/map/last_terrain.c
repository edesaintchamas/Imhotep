/* Imhotep - the canal sub-pass of the terrain images over a rectangle (package last).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x0047BD50 (x0, y0, x1, y1) redraws every canal tile of a rectangle with 0x0047BE30, walking
 * the cursor 0x00BC9768 and the globals 0x00BC9764 (x), 0x00BC9760 (y), like the road pass
 * 0x0047B590 (core/map/terrain_roads.c). */
#include "map/last_terrain.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"

#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))
#define IMG(o) S32(st, G_GRID_IMAGE + 4u * (uint32_t) (o))
#define BGRID(o) S16(st, G_GRID_BUILDING + 2u * (uint32_t) (o))
#define BITS(o) U8(st, G_GRID_BITFIELDS + (uint32_t) (o))

void canal_tile(game_state *st, int32_t dry)
{
    /* @bin 0x0047BE30 (dry) the cursor's canal image, unless a building stands on it: the canal
     * pattern of its neighbour flags (0x0047E870, 0x00484BE0 on the records 0x005CC678, 16); a
     * road across (terrain 0x40): bitfield bit 0x80 cleared, the offset (when the pattern gave no
     * extra: 1 when the tile has no road 0x8000040, else 0) + 0x2A paved / + 0xF (0x0047B920);
     * image = canal base + variant + offset + (dry ? 0x30 : 0), floodplain (0x10000) through
     * 0x0051C620; bitfield low nibble cleared, aqueduct grid := the extra, 0x00B67D1C |= 0x40 */
    int32_t c = S32(st, G_TILE_CURSOR);
    if (BGRID(c) != 0) {
        return;
    }
    /* @bin 0x0047BE51 the original's test (difference to the canal base negative and signed
     * >= 0x30, jns / jl) can never hold: the shift is 0x30 only when `dry` */
    int32_t shift = dry != 0 ? 0x30 : 0;
    GAME_CALL0(st, FN_CANAL_FLAGS);
    GAME_CALL(st, FN_LAND_PATTERN_MATCH, (int32_t) G_PATTERN_CANALS, 0x10, -1, 0);
    c = S32(st, G_TILE_CURSOR);
    if (TER(c) & 0x40) {
        BITS(c) &= 0x7F;
        if (S32(st, G_MATCH_EXTRA) == 0) {
            S32(st, G_MATCH_OFFSET) = (U32(st, G_GRID_TERRAIN - 4u * GRID_SIZE + 4u * (uint32_t) c) & 0x8000040u) == 0;
        }
        int32_t paved = GAME_CALL0(st, FN_ROAD_PAVED_AT);
        S32(st, G_MATCH_OFFSET) = S32(st, G_MATCH_OFFSET) + (paved != 0 ? 0x2A : 0xF);
        c = S32(st, G_TILE_CURSOR);
    }
    int32_t img = (int32_t) ((uint32_t) S32(st, G_MATCH_VARIANT) + (uint32_t) S32(st, G_CANAL_BASE) +
                             (uint32_t) S32(st, G_MATCH_OFFSET) + (uint32_t) shift);
    if (TER(c) & 0x10000) {
        img = GAME_CALL(st, FN_FLOODPLAIN_CANAL_IMAGE, img);
        c = S32(st, G_TILE_CURSOR);
    }
    IMG(c) = img;
    BITS(c) &= 0xF0;
    U8(st, G_CANAL + (uint32_t) c) = (uint8_t) S32(st, G_MATCH_EXTRA);
    U8(st, G_GRID_EDGE + (uint32_t) c) |= 0x40;
}

void canals_rect_redraw(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x0047BD50 (x0, y0, x1, y1) clamped to the map; every canal tile (0x100 without 0x0C)
     * whose bitfield 0x00B285CC has no 0x80: neighbour flags (0x0047E870), then 0x0047BE30(0) */
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    int32_t w = S32(st, G_MAP_WIDTH);
    if (x1 >= w) {
        x1 = w - 1;
    }
    int32_t h = S32(st, G_MAP_HEIGHT);
    if (y1 >= h) {
        y1 = h - 1;
    }
    int32_t step = w - x1 + x0 - 1;
    S32(st, G_WALK_Y) = y0;
    S32(st, G_TILE_CURSOR) = S32(st, G_MAP_START) + y0 * 228 + x0;
    while (S32(st, G_WALK_Y) <= y1) {
        for (S32(st, G_WALK_X) = x0; S32(st, G_WALK_X) <= x1; S32(st, G_WALK_X) += 1) {
            int32_t c = S32(st, G_TILE_CURSOR);
            uint32_t v = TER(c);
            if ((v & 0x100) && !(v & 0xC) && !(BITS(c) & 0x80)) {
                GAME_CALL0(st, FN_CANAL_FLAGS);
                canal_tile(st, 0);
            }
            S32(st, G_TILE_CURSOR) += 1;
        }
        S32(st, G_TILE_CURSOR) += S32(st, G_MAP_STRIDE_EXTRA) + step;
        S32(st, G_WALK_Y) += 1;
    }
}

static int32_t run_canals_redraw(game_state *st, const int32_t *a)
{
    canals_rect_redraw(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_canal_tile(game_state *st, const int32_t *a)
{
    canal_tile(st, a[0]);
    return 0;
}

const ported_function last_terrain_ports[] = {
    {0x0047BD50u, "canals_rect_redraw", 4, 0, 0, run_canals_redraw, 0},
    {0x0047BE30u, "canal_tile", 1, 0, 0, run_canal_tile, 0},
};
const int last_terrain_ports_count = sizeof(last_terrain_ports) / sizeof(last_terrain_ports[0]);
