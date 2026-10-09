/* Imhotep - the nearest place for a pyramid under the cursor (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * While building type 0xB7 is being placed, the hovered tile (0x00405F60, core/frame/frame.c)
 * moves to the nearest tile where the pyramid fits: 0x004ED330 walks square rings around the
 * mouse tile (0x004ED0D0) and stops at the first tile the callback 0x004ED2D0 accepts. */
#include "command/place.h"
#include "sim/call.h"
#include "state/ported.h"

#define MAP_START 0x00BCA2D8u
#define VARIANT 0x005E6970u

int32_t pyramid_tile_4ed2d0(game_state *st, int32_t grid, int32_t x, int32_t y, int32_t param)
{
    /* @bin 0x004ED2D0 (grid, x, y, unused) the pyramid of variant 0x005E6970 fits there
     * (0x004ECEC0; its reason goes to the argument's slot): the hovered tile 0x00BCA300 := grid,
     * 0x00BCA304 / 0x00BCA308 := its x / y from the map start; 0 (stop). Else 1 (go on) */
    (void) x;
    (void) y;
    (void) param;
    int32_t reason = 0;
    if (place_pyramid_fits(st, grid, S32(st, VARIANT), &reason) == 0) {
        return 1;
    }
    int32_t rel = grid - S32(st, MAP_START);
    S32(st, 0x00BCA300u) = grid;
    S32(st, 0x00BCA304u) = rel % 0xE4;
    S32(st, 0x00BCA308u) = rel / 0xE4;
    return 0;
}

static int32_t ring_call(game_state *st, uint32_t cb, int32_t grid, int32_t x, int32_t y, int32_t param)
{
    if (cb == 0x004ED2D0u) {
        return pyramid_tile_4ed2d0(st, grid, x, y, param);
    }
    return GAME_CALL(st, cb, grid, x, y, param);
}

void tile_rings_4ed0d0(game_state *st, int32_t tile, int32_t n, uint32_t cb, int32_t param)
{
    /* @bin 0x004ED0D0 (tile, n, callback, param) the square rings of radius 0 .. n - 1 around the
     * tile (x = tile % 228, y = tile / 228), clamped to the view's tile bounds 0x00BCA2E8 (x min),
     * 0x00BCA2EC (y min), 0x00BCA2F0 (x max), 0x00BCA2F4 (y max): the top row then the bottom row
     * (left to right), then the left and right columns between them (top to bottom), a side
     * skipped when it was clamped; callback(y * 228 + x, x, y, param) for each tile, stopping when
     * it returns 0 (al) */
    int32_t cx = tile % 0xE4, cy = tile / 0xE4;
    if (n <= 0) {
        return;
    }
    int32_t top = cy, bottom = cy;
    for (int32_t r = 0; r < n; r++, top--, bottom++) {
        int32_t x0 = cx - cy + top, x1 = cx - cy + bottom, y0 = top, y1 = bottom;
        int cl = 0, cr = 0, ct = 0, cb_ = 0;
        if (x0 < S32(st, 0x00BCA2E8u)) {
            x0 = S32(st, 0x00BCA2E8u);
            cl = 1;
        }
        if (x1 > S32(st, 0x00BCA2F0u)) {
            x1 = S32(st, 0x00BCA2F0u);
            cr = 1;
        }
        if (y0 < S32(st, 0x00BCA2ECu)) {
            y0 = S32(st, 0x00BCA2ECu);
            ct = 1;
        }
        if (y1 > S32(st, 0x00BCA2F4u)) {
            y1 = S32(st, 0x00BCA2F4u);
            cb_ = 1;
        }
        if (!(ct && cb_)) {
            for (int32_t x = x0; x <= x1; x++) {
                if (!ct && (uint8_t) ring_call(st, cb, x + y0 * 0xE4, x, y0, param) == 0) {
                    return;
                }
                if (!cb_ && y1 > y0 && (uint8_t) ring_call(st, cb, x + y1 * 0xE4, x, y1, param) == 0) {
                    return;
                }
            }
        }
        if (!(cl && cr)) {
            for (int32_t y = y0 + 1; y <= y1 - 1; y++) {
                if (!cl && (uint8_t) ring_call(st, cb, y * 0xE4 + x0, x0, y, param) == 0) {
                    return;
                }
                if (!cr && x1 > x0 && (uint8_t) ring_call(st, cb, y * 0xE4 + x1, x1, y, param) == 0) {
                    return;
                }
            }
        }
    }
}

void pyramid_nearest_4ed330(game_state *st)
{
    /* @bin 0x004ED330 the rings around the mouse tile 0x00BCA2C4, as many as 0x005F5D1C[variant],
     * with the callback 0x004ED2D0 */
    tile_rings_4ed0d0(st, S32(st, 0x00BCA2C4u), S32(st, 0x005F5D1Cu + 4u * U32(st, VARIANT)), 0x004ED2D0u, 0);
}

static int32_t run_4ed2d0(game_state *st, const int32_t *a)
{
    return pyramid_tile_4ed2d0(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_4ed0d0(game_state *st, const int32_t *a)
{
    tile_rings_4ed0d0(st, a[0], a[1], (uint32_t) a[2], a[3]);
    return 0;
}

static int32_t run_4ed330(game_state *st, const int32_t *a)
{
    (void) a;
    pyramid_nearest_4ed330(st);
    return 0;
}

const ported_function place_nearest_ports[] = {
    {0x004ED2D0u, "pyramid_tile_4ed2d0", 4, 8, 0, run_4ed2d0, 0},
    {0x004ED0D0u, "tile_rings_4ed0d0", 4, 0, 0, run_4ed0d0, 0},
    {0x004ED330u, "pyramid_nearest_4ed330", 0, 0, 0, run_4ed330, 0},
};
const int place_nearest_ports_count = sizeof(place_nearest_ports) / sizeof(place_nearest_ports[0]);
