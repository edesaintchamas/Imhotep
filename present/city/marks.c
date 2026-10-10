/* Imhotep - the city view's last passes: figures drawn on top 0x00439740 and the debug grid values
 * 0x0043E300 (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

void city_figures_on_top(game_state *st, int32_t param)
{
    /* @bin 0x00439740 (param: as 0x00438E50's, 9999 in the overlay view): the figures flagged +0x77
     * drawn again over everything, row by row; a second walk leaves the view tile 0x0070E488 of
     * each column (in the normal view only) */
    if (U8(st, G_U605B3D) == 1) {
        S32(st, G_TOP_COUNTER) = S32(st, G_TOP_COUNTER) + 1;
        if (S32(st, G_TOP_COUNTER) >= 5) {
            S32(st, G_TOP_COUNTER) = 0;
        }
    }
    int32_t y = S32(st, G_VIEW_Y) - 9 * S32(st, G_TILE_H);
    S32(st, G_VIEW_ROW) = S32(st, G_VIEW_FIRST_ROW) - 8;
    if (S32(st, G_VIEW_ROWS) + 0xE <= 0) {
        return;
    }
    int flag = 0;
    int32_t row = 0;
    do {
        int32_t vy = S32(st, G_VIEW_ROW);
        if (vy >= 0 && vy < 0x1C9) {
            int32_t x0 = S32(st, G_VIEW_X);
            if (flag) {
                x0 = x0 - S32(st, G_TILE_W) - 1;
            }
            x0 += (-2 - S32(st, G_U70E458)) * 4;
            int32_t x = x0;
            S32(st, G_VIEW_COLUMN) = S32(st, G_VIEW_FIRST_COL) - 4;
            for (int32_t col = 0; col <= S32(st, G_VIEW_COLS) + 6; col++) {
                int32_t vx = S32(st, G_VIEW_COLUMN);
                if (vx >= 0 && vx < 0xE7) {
                    int32_t tile = S32(st, G_VIEW_TABLE + 4u * (uint32_t) (S32(st, G_VIEW_ROW) * 0xE7 + vx));
                    S32(st, G_VIEW_TILE) = tile;
                    if (tile < 0xFFF0000) {
                        for (int32_t f = S16(st, G_FIGURE_GRID + 2u * (uint32_t) tile); f > 0;
                             f = S16(st, FIGURE_ADDR(f) + FIG_NEXT_ON_TILE)) {
                            if (U8(st, FIGURE_ADDR(f) + FIG_HEIGHT_TICKS) != 0) {
                                GAME_CALL(st, FN_FIGURE_DRAW, f, x, y, param);
                            }
                        }
                    }
                }
                x += S32(st, G_U70E458) + 2;
                S32(st, G_VIEW_COLUMN) = S32(st, G_VIEW_COLUMN) + 1;
            }
            /* @bin 0x0043989B */
            int32_t vx = S32(st, G_VIEW_FIRST_COL) - 4;
            S32(st, G_VIEW_COLUMN) = vx;
            int32_t n = S32(st, G_VIEW_COLS) + 6;
            if (n >= 0) {
                int32_t vrow = S32(st, G_VIEW_ROW);
                for (int32_t k = n + 1; k != 0; k--) {
                    if (vx >= 0 && vx < 0xE7 &&
                        (S32(st, G_OVERLAY) == 0 || S32(st, G_OVERLAY) == 0x33 || U8(st, G_NO_GROUND) != 0)) {
                        S32(st, G_VIEW_TILE) = S32(st, G_VIEW_TABLE + 4u * (uint32_t) (vrow * 0xE7 + vx));
                    }
                    vx++;
                }
                S32(st, G_VIEW_COLUMN) = vx;
            }
        }
        flag = !flag;
        y += S32(st, G_TILE_H);
        row++;
        S32(st, G_VIEW_ROW) = S32(st, G_VIEW_ROW) + 1;
    } while (row < S32(st, G_VIEW_ROWS) + 0xE);
}

int32_t tile_figure_count(game_state *st, int32_t tile)
{
    /* @bin 0x004B54F0 the figures on a tile (its chain, to the first 0) */
    int32_t n = 0;
    for (int32_t f = S16(st, G_FIGURE_GRID + 2u * (uint32_t) tile); f != 0; f = S16(st, FIGURE_ADDR(f) + FIG_NEXT_ON_TILE)) {
        n++;
    }
    return n;
}

static void debug_value(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0043E420 (x, y) the value of the grid 0x00E38E94 at the view tile, as a number (red
     * 0xF800 when negative); 0 not drawn */
    int32_t t = S32(st, G_VIEW_TILE);
    int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) t);
    int32_t v = b;
    switch (S32(st, G_DEBUG_GRID)) {
    case 0: break;
    case 1: v = S32(st, G_GRID_TERRAIN + 4u * (uint32_t) t); break;
    case 2: v = U8(st, G_GRID_BITFIELDS + (uint32_t) t); break;
    case 3: v = S32(st, G_GRID_IMAGE + 4u * (uint32_t) t); break;
    case 4: v = U8(st, G_GRID_EDGE + (uint32_t) t); break;
    case 5: v = U8(st, G_CANAL + (uint32_t) t); break;
    case 6: v = U8(st, G_GRID_RANDOM + (uint32_t) t); break;
    case 7: v = S16(st, G_FIGURE_GRID + 2u * (uint32_t) t); break;
    case 8: v = U8(st, G_SPRITE + (uint32_t) t); break;
    case 9: v = S8(st, G_GRID_ROUTING + (uint32_t) t); break;
    case 0xA: v = S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) t); break;
    case 0xB: v = U8(st, BUILDING_ADDR(b) + BLD_WATER_SUPPLY); break;
    case 0xC: v = S16(st, BUILDING_ADDR(b) + BLD_SERVICE); break;
    case 0xD: v = S16(st, BUILDING_ADDR(b) + BLD_DISTANCE); break;
    case 0xE: v = S16(st, BUILDING_ADDR(b) + BLD_COLLAPSE_RISK); break;
    case 0xF: v = S16(st, BUILDING_ADDR(b) + BLD_POPULATION); break;
    case 0x10: v = S8(st, G_GRID_DESIRABILITY + (uint32_t) t); break;
    case 0x11: v = U8(st, G_ELEVATION_GRID + (uint32_t) t); break;
    case 0x12: v = S8(st, G_GRID_ROUTING_WATER + (uint32_t) t); break;
    case 0x13: v = S8(st, G_EC7630 + (uint32_t) t); break;
    case 0x14: v = S16(st, G_GRID_DAMAGE + 2u * (uint32_t) t); break;
    case 0x15: v = tile_figure_count(st, t); break;
    case 0x16: v = U8(st, G_GRID_RUBBLE + (uint32_t) t); break;
    case 0x17: v = U8(st, G_THREAT + (uint32_t) t); break;
    case 0x18: v = S8(st, G_EBAB20 + (uint32_t) t); break;
    case 0x19: v = U8(st, G_GRID_ROAD_NETWORK + (uint32_t) t); break;
    case 0x1A: v = U8(st, G_NETWORK2 + (uint32_t) t); break;
    case 0x1B: v = GAME_CALL(st, FN_ROAD_WEAR, 1, t); break;
    default: return;
    }
    if (v == 0) {
        return;
    }
    int32_t colour = 0xFFFF;
    if (v < 0) {
        v = (int32_t) (0u - (uint32_t) v);
        colour = 0xF800;
    }
    GAME_CALL(st, FN_NUMBER_DRAW, v, 0x20, G_NUMBER_SUFFIX, x + 0x18, y + 0xA, S32(st, G_FONT_SMALL), colour);
}

void city_debug_values(game_state *st)
{
    /* @bin 0x0043E300 when one of the debug switches 0x0078D7D8 / 0x0078D7DC / 0x0078D7D4 is on:
     * every view tile's value (0x0043E420), over the view's rows and columns from the camera (no
     * margins, no range check of the column) */
    if (S32(st, G_DEBUG_TILE) == 0 && S32(st, G_DEBUG_EVOLVE) == 0 && S32(st, G_DEBUG_FIGURE) == 0) {
        return;
    }
    int32_t y = S32(st, G_VIEW_Y) - S32(st, G_TILE_H);
    S32(st, G_VIEW_ROW) = S32(st, G_VIEW_FIRST_ROW);
    if (S32(st, G_VIEW_ROWS) <= 0) {
        return;
    }
    int flag = 0;
    int32_t row = 0;
    do {
        int32_t x = S32(st, G_VIEW_X);
        if (!flag) {
            flag = 1;
        } else {
            x = x - S32(st, G_TILE_W) - 1;
            flag = 0;
        }
        S32(st, G_VIEW_COLUMN) = S32(st, G_VIEW_FIRST_COL);
        for (int32_t col = 0; col <= S32(st, G_VIEW_COLS); col++) {
            int32_t tile = S32(st, G_VIEW_TABLE + 4u * (uint32_t) (S32(st, G_VIEW_ROW) * 0xE7 + S32(st, G_VIEW_COLUMN)));
            S32(st, G_VIEW_TILE) = tile;
            if (tile < 0xFFF0000) {
                debug_value(st, x, y);
            }
            S32(st, G_VIEW_COLUMN) = S32(st, G_VIEW_COLUMN) + 1;
            x += S32(st, G_U70E458) + 2;
        }
        y += S32(st, G_TILE_H);
        row++;
        S32(st, G_VIEW_ROW) = S32(st, G_VIEW_ROW) + 1;
    } while (row < S32(st, G_VIEW_ROWS));
}

static int32_t run_on_top(game_state *st, const int32_t *a)
{
    city_figures_on_top(st, a[0]);
    return 0;
}
static int32_t run_count(game_state *st, const int32_t *a) { return tile_figure_count(st, a[0]); }
static int32_t run_value(game_state *st, const int32_t *a)
{
    debug_value(st, a[0], a[1]);
    return 0;
}
static int32_t run_debug(game_state *st, const int32_t *a)
{
    (void) a;
    city_debug_values(st);
    return 0;
}

const ported_function cityview_marks_ports[] = {
    {0x00439740u, "city_figures_on_top", 1, 0, 0, run_on_top, 0},
    {0x004B54F0u, "tile_figure_count", 1, 32, 0, run_count, 0},
    {0x0043E420u, "city_debug_value", 2, 0, 0, run_value, 0},
    {0x0043E300u, "city_debug_values", 0, 0, 0, run_debug, 0},
};
const int cityview_marks_ports_count = sizeof(cityview_marks_ports) / sizeof(cityview_marks_ports[0]);
