/* Imhotep - daily desirability update (Pharaoh.exe 0x0046A0C0 + 0x0046A140).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/desirability.h"

#include <string.h>

#include "city/ring_tables.h"
#include "state/layout.h"

#define SIDE DESIRABILITY_GRID_SIDE
#define GRID (SIDE * SIDE)

/* @bin 0x0046F8D0 1x1 rings 1..6: the border of the square -k..k, rows (dy) outer, dx inner */
static int ring1(int k, int8_t *dxdy)
{
    int n = 0;
    for (int dy = -k; dy <= k; dy++) {
        for (int dx = -k; dx <= k; dx++) {
            if (dx == -k || dx == k || dy == -k || dy == k) {
                dxdy[2 * n] = (int8_t) dx;
                dxdy[2 * n + 1] = (int8_t) dy;
                n++;
            }
        }
    }
    return n;
}

static ring_table ring(int size, int r, int8_t *scratch)
{
    if (size == 1) {
        ring_table t = {scratch, ring1(r, scratch)};
        return t;
    }
    return ring_tables[size][r];
}

static void add(int8_t *g, int o, int8_t v, int clamp)
{
    if (o >= 0 && o < GRID) {
        g[o] = (int8_t) (g[o] + v); /* byte arithmetic: wraps */
    }
    if (clamp >= 0 && clamp < GRID) {
        if (g[clamp] < -100) {
            g[clamp] = -100;
        }
        if (g[clamp] > 100) {
            g[clamp] = 100;
        }
    }
}

/* @bin 0x004818A0 add v to every tile of ring r around the size x size footprint at (x, y) */
static void add_ring(game_state *st, int x, int y, int size, int r, int8_t v)
{
    if (size < 1 || size > 6) {
        return; /* switch default */
    }
    int8_t s1[2 * 4 * 12 + 2], s2[2 * 4 * 12 + 2];
    ring_table t = ring(size, r, s1);
    int base = S32(st, G_MAP_START) + y * SIDE + x;
    S32(st, G_TILE_CURSOR) = base;
    int8_t *g = (int8_t *) state_ptr(st, G_DESIRABILITY_GRID, GRID);
    int width = S32(st, G_MAP_WIDTH), height = S32(st, G_MAP_HEIGHT);
    if (y + size - 1 + r > height || x + size - 1 + r > width || y - r < -1 || x - r < -1) {
        /* near the edge: each tile is tested by 0x00481AA0, which for 6x6 footprints reads entry i
         * of ring r + 1 (ring 6 for r = 6) instead of ring r; the clamp then applies to the
         * footprint's own tile, not to the tile just changed */
        ring_table e = size == 6 ? ring(6, r < 6 ? r + 1 : 6, s2) : t;
        for (int i = 0; i < t.count; i++) {
            int ex = x + e.dxdy[2 * i], ey = y + e.dxdy[2 * i + 1];
            if (ex < -1 || ey < -1 || width < ex || height < ey) {
                continue;
            }
            add(g, base + t.dxdy[2 * i] + t.dxdy[2 * i + 1] * SIDE, v, base);
        }
        return;
    }
    for (int i = 0; i < t.count; i++) {
        int o = base + t.dxdy[2 * i] + t.dxdy[2 * i + 1] * SIDE;
        add(g, o, v, o);
    }
}

/* @bin 0x00481830 rings 1..range (at most 6); the value changes by step_size every `step` rings */
static void spread(game_state *st, int x, int y, int size, int32_t value, int32_t step, int32_t step_size,
                   int32_t range)
{
    if (size <= 0) {
        return;
    }
    if (range > 6) {
        range = 6;
    }
    for (int r = 1, k = 0; r <= range; r++) {
        add_ring(st, x, y, size, r, (int8_t) value);
        if (step <= ++k) {
            value += step_size;
            k = 0;
        }
    }
}

/* model record fields b..e (value, step, step size, range) of building type `type` */
static void spread_model(game_state *st, int x, int y, int size, int type)
{
    uint32_t m = G_MODEL_BUILDING + G_TYPE_TABLE_STRIDE * (uint32_t) type;
    spread(st, x, y, size, S32(st, m + 4), S32(st, m + 8), S32(st, m + 12), S32(st, m + 16));
}

void desirability_buildings_upto(game_state *st, int32_t last_id)
{
    /* @bin 0x0046A0C0 clear, then buildings 1 .. highest id in use (state 1), model fields b-e */
    memset(state_ptr(st, G_DESIRABILITY_GRID, GRID), 0, GRID);
    for (int32_t id = 1; id <= last_id; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        /* the type indexes the model table unchecked (types are < 237 in every save seen) */
        spread_model(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), S16(st, b + BLD_TYPE));
    }
}

void desirability_buildings(game_state *st)
{
    desirability_buildings_upto(st, S32(st, G_BUILDING_HIGHEST));
}

void desirability_tiles(game_state *st)
{
    /* @bin 0x0046A140 map tiles, rows then columns, from MAP_START with MAP_ROW_SKIP per row */
    int o = S32(st, G_MAP_START);
    for (int y = 0; y < S32(st, G_MAP_HEIGHT); y++) {
        for (int x = 0; x < S32(st, G_MAP_WIDTH); x++, o++) {
            uint32_t t = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) o);
            uint8_t *bits = &U8(st, G_DAY2_DESTROY_BITS + (uint32_t) o);
            if (!(*bits & 0x80)) {
                if (t & 0x20) {
                    spread_model(st, x, y, 1, 39); /* gardens (0x007064C0 = record 39 + 4) */
                } else if (t & 0x1000) {
                    spread(st, x, y, 1, -2, 1, 1, 2); /* rubble */
                }
            } else if (t & 0x40) {
                spread_model(st, x, y, 1, 0x26); /* plaza */
            } else if (t & 2) {
                spread_model(st, x, y, 1, 10);
            } else {
                *bits &= 0x7F;
            }
        }
        o += S32(st, G_MAP_STRIDE_EXTRA);
    }
}

void desirability_update(game_state *st)
{
    /* @bin 0x00451C50 step 0x25: 0x0046A0C0 then 0x0046A140 */
    desirability_buildings(st);
    desirability_tiles(st);
}
