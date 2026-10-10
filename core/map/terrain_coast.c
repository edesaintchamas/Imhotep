/* Imhotep - the shore marks (terrain 0x4000000) of the water tiles (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004877F0 (x0, y0, x1, y1) marks the water tiles (terrain 4 without 0x80000) whose 3x3 square
 * is all water and has no floodplain within 1 tile, then removes the marks that do not hold: a
 * marked tile needs 2 marked neighbours in its row and in its column, and its 8 neighbours (the
 * scratch of 0x0047DB00) a run of 3 marked ones between unmarked ones; each mark removed
 * re-checks its neighbours (0x00487B70, recursive). Called by the water pass with the map reveal
 * (0x00475CC0) and at load (core/building/day_state.c). */
#include "map/terrain_coast.h"

#include "building/day_access.h"
#include "map/terrain_land.h"
#include "state/layout.h"
#include "state/ported.h"

#define MARK 0x4000000u
#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))

int32_t map_area_lacks(game_state *st, int32_t x, int32_t y, int32_t r, uint32_t mask)
{
    /* @bin 0x00481770 (x, y, r, mask) a tile on the map within r - 1 of (x, y) without terrain
     * & mask: 1; the row offset advances by xs - xe + 227 after each row */
    int32_t xs = x + 1 - r, xe = x + r - 1, ys = y + 1 - r, ye = y + r - 1;
    int32_t step = xs - xe + 0xE3;
    int32_t o = S32(st, G_MAP_START) + ys * 228 + xs;
    for (int32_t yy = ys; yy <= ye; yy++) {
        for (int32_t xx = xs; xx <= xe; xx++, o++) {
            if (xx >= 0 && yy >= 0 && xx < S32(st, G_MAP_WIDTH) && yy < S32(st, G_MAP_HEIGHT) && (TER(o) & mask) == 0) {
                return 1;
            }
        }
        o += step;
    }
    return 0;
}

int32_t row_pair_with(game_state *st, int32_t x, int32_t y, uint32_t mask)
{
    /* @bin 0x00487730 (x, y, mask) at least 2 of (x - 1 .. x + 1, y) on the map (columns only)
     * have terrain & mask */
    int32_t n = 0;
    for (int32_t xx = x - 1; xx < x + 2; xx++) {
        if (xx >= 0 && xx < S32(st, G_MAP_WIDTH) && (TER(S32(st, G_MAP_START) + y * 228 + xx) & mask)) {
            n++;
        }
    }
    return n > 1;
}

int32_t column_pair_with(game_state *st, int32_t x, int32_t y, uint32_t mask)
{
    /* @bin 0x00487790 (x, y, mask) at least 2 of (x, y - 1 .. y + 1) on the map (rows only) have
     * terrain & mask */
    int32_t n = 0, h = S32(st, G_MAP_HEIGHT);
    int32_t o = S32(st, G_MAP_START) + (y - 1) * 228 + x;
    for (int32_t yy = y - 1; yy < y + 2; yy++, o += 228) {
        if (yy >= 0 && yy < h && (TER(o) & mask)) {
            n++;
        }
    }
    return n > 1;
}

static int mark_holds(game_state *st, int32_t o)
{
    /* @bin 0x00487C61 the scratch (0x0047DB00 (0x4000000) at the cursor := o), read around from
     * neighbour 0: after the first unmarked one, a run of exactly 2 marked ones closed by an
     * unmarked one holds the mark (scan of at most 10 more entries, stops when a run reaches 8) */
    S32(st, G_TILE_CURSOR) = o;
    land_neighbour_flags(st, MARK);
    int32_t i = 0, end = 0x10, run = -1, holds = 0;
    while (run < 8) {
        if (S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) (i & 7)) != 0) {
            if (run >= 0) {
                run++;
            }
        } else {
            if (run < 0) {
                end = i + 10;
            } else if (run == 2) {
                holds = 1;
                i = end;
            }
            run = 0;
        }
        i++;
        if (i >= end) {
            break;
        }
    }
    return holds;
}

static void check_around(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00487B70 body (and 0x00487979 in 0x004877F0): the 3x3 square around (x, y), x and y
     * from max(v - 1, 0) to min(v + 1, size) inclusive; a marked tile whose row or column pair
     * test fails, or whose mark does not hold, loses it and its square is checked again */
    int32_t w = S32(st, G_MAP_WIDTH);
    int32_t xs = x - 1 < 0 ? 0 : x - 1, xe = w < x + 1 ? w : x + 1;
    int32_t ys = y - 1 < 0 ? 0 : y - 1, ye = S32(st, G_MAP_HEIGHT) < y + 1 ? S32(st, G_MAP_HEIGHT) : y + 1;
    int32_t step = S32(st, G_MAP_STRIDE_EXTRA) - xe + w + xs - 1;
    int32_t o = S32(st, G_MAP_START) + ys * 228 + xs;
    for (int32_t yy = ys; yy <= ye; yy++) {
        for (int32_t xx = xs; xx <= xe; xx++, o++) {
            if (!(TER(o) & MARK)) {
                continue;
            }
            if (column_pair_with(st, xx, yy, MARK) && row_pair_with(st, xx, yy, MARK) && !mark_holds(st, o)) {
                continue;
            }
            TER(o) &= ~MARK;
            coast_recheck(st, xx, yy);
        }
        o += step;
    }
}

void coast_recheck(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00487B70 (x, y) */
    check_around(st, x, y);
}

void coast_marks(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x004877F0 (x0, y0, x1, y1) clamped to the map; the cursor is restored at the end */
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
    if (y1 >= S32(st, G_MAP_HEIGHT)) {
        y1 = S32(st, G_MAP_HEIGHT) - 1;
    }
    int32_t step = w - x1 + x0 - 1;
    int32_t saved = S32(st, G_TILE_CURSOR);
    int32_t o = S32(st, G_MAP_START) + y0 * 228 + x0;
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++, o++) {
            if ((TER(o) & 0x80004u) != 4) {
                continue;
            }
            if (map_area_lacks(st, x, y, 2, 4) == 0 && map_area_has_terrain(st, x, y, 2, 0x10000) == 0) {
                TER(o) |= MARK;
            } else {
                TER(o) &= ~MARK;
            }
        }
        o += S32(st, G_MAP_STRIDE_EXTRA) + step;
    }
    o = S32(st, G_MAP_START) + y0 * 228 + x0;
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++, o++) {
            if (TER(o) & MARK) {
                check_around(st, x, y);
            }
        }
        o += S32(st, G_MAP_STRIDE_EXTRA) + step;
    }
    S32(st, G_TILE_CURSOR) = saved;
}

int32_t neighbours_with_without(game_state *st, uint32_t mask, uint32_t exclude, int32_t tile)
{
    /* @bin 0x004775F0 (mask, exclude, tile) how many of the 8 neighbours have terrain & mask and
     * not terrain & exclude */
    static const int32_t N[8] = {-228, -227, 1, 229, 228, 227, -1, -229};
    int32_t n = 0;
    for (int i = 0; i < 8; i++) {
        uint32_t v = TER(tile + N[i]);
        n += (v & mask) && !(v & exclude);
    }
    return n;
}

/* ---- registration -------------------------------------------------------------------------- */

static int32_t run_lacks(game_state *st, const int32_t *a) { return map_area_lacks(st, a[0], a[1], a[2], (uint32_t) a[3]); }
static int32_t run_row(game_state *st, const int32_t *a) { return row_pair_with(st, a[0], a[1], (uint32_t) a[2]); }
static int32_t run_column(game_state *st, const int32_t *a) { return column_pair_with(st, a[0], a[1], (uint32_t) a[2]); }
static int32_t run_recheck(game_state *st, const int32_t *a) { coast_recheck(st, a[0], a[1]); return 0; }
static int32_t run_nww(game_state *st, const int32_t *a) { return neighbours_with_without(st, (uint32_t) a[0], (uint32_t) a[1], a[2]); }
static int32_t run_marks(game_state *st, const int32_t *a) { coast_marks(st, a[0], a[1], a[2], a[3]); return 0; }

const ported_function terr_coast_ports[] = {
    {0x00481770u, "map_area_lacks", 4, 32, 0, run_lacks, 0},
    {0x00487730u, "row_pair_with", 3, 8, 0, run_row, 0},
    {0x00487790u, "column_pair_with", 3, 8, 0, run_column, 0},
    {0x00487B70u, "coast_recheck", 2, 0, 0, run_recheck, 0},
    {0x004877F0u, "coast_marks", 4, 0, 0, run_marks, 0},
    {0x004775F0u, "neighbours_with_without", 3, 32, 0, run_nww, 0},
};
const int terr_coast_ports_count = sizeof(terr_coast_ports) / sizeof(terr_coast_ports[0]);
