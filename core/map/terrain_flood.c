/* Imhotep - the floodplain sub-pass of the terrain images over any rectangle (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004777C0 (x0, y0, x1, y1) redraws every floodplain tile (terrain 0x10000 without 8) of a
 * rectangle with 0x00479E70, which redraws the floodplain tiles of the 3x3 square around it, for
 * any view orientation and with the 'blood river' groups of the map reveal. core/map/terrain.c
 * has the whole-map version for a new scenario; this one is general. */
#include "map/terrain_flood.h"

#include "map/terrain_land.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/pre_load.h"

#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))

/* @bin 0x0047DB00 neighbour order of the scratch */
static const int32_t NEIGHBOURS[8] = {-228, -227, 1, 229, 228, 227, -1, -229};

static void neighbour_bits(game_state *st, uint32_t mask, int32_t shift)
{
    /* @bin 0x0047DDD0 (mask, shift) scratch[i] |= (terrain & mask != 0) << shift */
    int32_t c = S32(st, G_TILE_CURSOR);
    for (uint32_t i = 0; i < 8; i++) {
        uint32_t bit = (TER(c + NEIGHBOURS[i]) & mask) != 0;
        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = (int32_t) ((uint32_t) S32(st, G_NEIGHBOUR_FLAGS + 4u * i) | (bit << shift));
    }
}

static void scratch_invert(game_state *st)
{
    /* @bin 0x00484F00 */
    for (uint32_t i = 0; i < 8; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * i) = S32(st, G_NEIGHBOUR_FLAGS + 4u * i) == 0;
    }
}

int32_t pattern_match_by_water(game_state *st, uint32_t records, int32_t per_block)
{
    /* @bin 0x00484D70 (records, per block, unused) the orthogonal water bits (bit 1) of the
     * scratch, the diagonal ones counting for both sides, pick a block of records (lookup
     * {0, 1, 3, 2, 5, 1, 4, 2, 6, 7, 1, 1, 5, 1, 1, 1}); its first record matching bit 0 gives
     * offset (byte 8 + orientation / 2), extra (12), variant (the cycling counter 14); returns the
     * rank + 1 in the table, 0 if none */
    static const uint8_t BLOCK[16] = {0, 1, 3, 2, 5, 1, 4, 2, 6, 7, 1, 1, 5, 1, 1, 1};
    uint32_t mask = 0;
    for (uint32_t i = 0; i < 4; i++) {
        if (S32(st, G_NEIGHBOUR_FLAGS + 8u * i + 4) & 2) {
            mask |= 1u << ((i + 1) & 3) | 1u << i;
        }
        if (S32(st, G_NEIGHBOUR_FLAGS + 8u * i) & 2) {
            mask |= 1u << i;
        }
    }
    int32_t block = mask ? BLOCK[mask] : 0, base = block * per_block;
    int32_t half = S32(st, G_VIEW_ORIENTATION) / 2;
    for (int32_t k = 0; k < per_block; k++) {
        uint32_t r = records + 16u * (uint32_t) (base + k);
        int ok = 1;
        for (uint32_t i = 0; i < 8 && ok; i++) {
            uint8_t b = U8(st, r + i);
            if (b != 2 && (b != 0) != ((S32(st, G_NEIGHBOUR_FLAGS + 4u * i) & 1) != 0)) {
                ok = 0;
            }
        }
        if (!ok) {
            continue;
        }
        S32(st, G_MATCH_OFFSET) = U8(st, r + PATTERN_OFFSET + (uint32_t) half);
        S32(st, G_MATCH_EXTRA) = U8(st, r + PATTERN_EXTRA);
        uint8_t c = (uint8_t) (U8(st, r + PATTERN_COUNTER) + 1);
        U8(st, r + PATTERN_COUNTER) = c;
        if (c >= U8(st, r + PATTERN_VARIANTS)) {
            U8(st, r + PATTERN_COUNTER) = 0;
        }
        S32(st, G_MATCH_VARIANT) = U8(st, r + PATTERN_COUNTER);
        return k + base + 1;
    }
    return 0;
}

/* Fertility term: x87 trunc(f * 0.01 * 8.0) equals (f * 8) / 100 for every f in 0..255 (53-bit
 * products, checked for every value). @bin 0x0047A049 FMUL [0x0056F598] = 0.01, FMUL [0x0056F590]
 * = 8.0 */
static int32_t fertility_level(int32_t f)
{
    return f * 8 / 100;
}

void floodplain_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag)
{
    /* @bin 0x00479E70 (x, y, flag) inside the map: terrain 0x10000 set (flag 1) or cleared; the
     * floodplain tiles of the 3x3 square (0x10000 without 0x148): a tile with a floodplain object
     * of flag +0x18 == 0 matches the inverted floodplain neighbours backward, the others by their
     * water bits (0x0047DDD0 (4, 1), diagonals cleared, 0x00484D70); image = group 528 + variant
     * (+ progress of the object's list + 6 * (fertility level, low bit from the random grid of the
     * object's tile)), the blood groups while the map reveal has not reached it; aqueduct grid 0 */
    if (!map_tile_inside(st, S32(st, G_VIEW_DX) + x, S32(st, G_VIEW_DY) + y)) {
        return;
    }
    int32_t c = S32(st, G_MAP_START) + y * 228 + x;
    S32(st, G_TILE_CURSOR) = c;
    TER(c) = flag == 1 ? TER(c) | 0x10000 : TER(c) & 0xFFFEFFFFu;
    int32_t xa = x - 1, ya = y - 1, w = 3, h = 3;
    if (xa < 0) {
        w = xa + 3;
        xa = 0;
    } else if (xa + 3 > S32(st, G_MAP_WIDTH)) {
        w = S32(st, G_MAP_WIDTH) - xa;
    }
    if (ya < 0) {
        h = ya + 3;
        ya = 0;
    } else if (ya + 3 > S32(st, G_MAP_HEIGHT)) {
        h = S32(st, G_MAP_HEIGHT) - ya;
    }
    int blood = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, xa, ya) != 0;
    S32(st, G_WALK_Y) = ya;
    S32(st, G_TILE_CURSOR) = S32(st, G_MAP_START) + ya * 228 + xa;
    while (S32(st, G_WALK_Y) < ya + h) {
        for (S32(st, G_WALK_X) = xa; S32(st, G_WALK_X) < xa + w; S32(st, G_WALK_X) += 1) {
            int32_t o = S32(st, G_TILE_CURSOR);
            uint32_t v = TER(o);
            if ((v & 0x10000) && !(v & 0x148)) {
                land_neighbour_flags(st, 0x10000);
                scratch_invert(st);
                uint32_t obj = U32(st, G_FLOODPLAIN_MANAGER + 4u * (uint32_t) S32(st, G_TILE_CURSOR));
                if (obj != 0 && P8(st, obj + FLOODOBJ_FLAG) == 0) {
                    land_pattern_match(st, G_PATTERN_FLOODPLAIN, 0x18, 0, 1);
                } else {
                    neighbour_bits(st, 4, 1);
                    S32(st, G_NEIGHBOUR_FLAGS + 0x1C) = 0;
                    S32(st, G_NEIGHBOUR_FLAGS + 0x14) = 0;
                    S32(st, G_NEIGHBOUR_FLAGS + 0x0C) = 0;
                    S32(st, G_NEIGHBOUR_FLAGS + 0x04) = 0;
                    pattern_match_by_water(st, G_PATTERN_FLOODPLAIN, 0x18);
                }
                int32_t g = S32(st, G_IMAGE_GROUPS + 8u * 528), img = g + S32(st, G_MATCH_VARIANT);
                S32(st, G_MATCH_OFFSET) = 0;
                o = S32(st, G_TILE_CURSOR);
                obj = U32(st, G_FLOODPLAIN_MANAGER + 4u * (uint32_t) o);
                if (obj != 0) {
                    uint32_t t = P32(st, obj + FLOODOBJ_TILE);
                    int32_t lvl = fertility_level(U8(st, G_GRID_FERTILITY + t));
                    lvl = (lvl & ~1) | (U8(st, G_GRID_RANDOM + t) & 1);
                    img += (int32_t) P32(st, P32(st, obj) + 4) + lvl * 6;
                }
                if (blood) {
                    int32_t e = img - S32(st, G_IMAGE_GROUPS + 8u * 528);
                    if (e >= 0x3C && e <= 0x53) {
                        img = img + (S32(st, G_IMAGE_GROUPS + 8u * 720) - S32(st, G_IMAGE_GROUPS + 8u * 528)) - 0x3C;
                    }
                }
                U8(st, G_GRID_BITFIELDS + (uint32_t) o) &= 0xF0;
                S32(st, G_GRID_IMAGE + 4u * (uint32_t) o) = img;
                U8(st, G_GRID_EDGE + (uint32_t) o) |= 0x40;
                U8(st, G_CANAL + (uint32_t) o) = 0;
            }
            S32(st, G_TILE_CURSOR) += 1;
        }
        S32(st, G_WALK_Y) += 1;
        S32(st, G_TILE_CURSOR) += 0xE4 - w;
    }
}

void floodplain_pass(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x004777C0 clamped to the map; every floodplain tile (0x10000 without 8):
     * 0x00479E70 (x, y, 1) */
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
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++) {
            uint32_t v = TER(S32(st, G_MAP_START) + y * 228 + x);
            if ((v & 0x10000) && !(v & 8)) {
                floodplain_tile_redraw(st, x, y, 1);
            }
        }
    }
}

static int32_t run_pass(game_state *st, const int32_t *a)
{
    floodplain_pass(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_tile(game_state *st, const int32_t *a)
{
    floodplain_tile_redraw(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_bits(game_state *st, const int32_t *a)
{
    neighbour_bits(st, (uint32_t) a[0], a[1]);
    return 0;
}

static int32_t run_by_water(game_state *st, const int32_t *a)
{
    return pattern_match_by_water(st, (uint32_t) a[0], a[1]);
}

const ported_function bigb2_flood_ports[] = {
    {0x004777C0u, "floodplain_pass", 4, 0, 0, run_pass, 0},
    {0x00479E70u, "floodplain_tile_redraw", 3, 0, 0, run_tile, 0},
    {0x0047DDD0u, "neighbour_bits", 2, 0, 0, run_bits, 0},
    {0x00484D70u, "pattern_match_by_water", 3, 32, 0, run_by_water, 0},
};
const int bigb2_flood_ports_count = sizeof(bigb2_flood_ports) / sizeof(bigb2_flood_ports[0]);
