/* Imhotep - helpers left on the tick's frontier (package rest): 0x00480C60 and 0x00480D60, called
 * by the figure action 0x004937A0.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/rest_misc.h"

#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static int32_t absdiff(int32_t a, int32_t b)
{
    return a > b ? a - b : a < b ? b - a : 0;
}

int32_t nearest_point_480c60(game_state *st, int32_t f)
{
    /* @bin 0x00480C60 of the 8 points (x 0x00784B74, y 0x00784B84, i16; x > 0 in use) the one
     * nearest to figure f (+0x14, +0x16; the larger of the distances on each axis, first of
     * equals, below 10000) into 0x00D3B4E4 / 0x00D3B4E0 (their low bytes); 0 when none.
     * 0x00D3B4FC / 0x00D3B4F8 keep the last distances computed */
    int32_t n = 0;
    for (uint32_t i = 0; i < 8; i++) {
        if (S16(st, G_POINTS_X + 2u * i) > 0) {
            n++;
        }
    }
    if (n <= 0) {
        return 0;
    }
    uint32_t fr = FIGURE_ADDR(f);
    int32_t best = 10000, sel = 0;
    for (int32_t i = 0; i < 8; i++) {
        int32_t x = S16(st, G_POINTS_X + 2u * (uint32_t) i);
        if (x <= 0) {
            continue;
        }
        int32_t y = S16(st, G_POINTS_Y + 2u * (uint32_t) i);
        int32_t dx = absdiff(S16(st, fr + FIG_X), x);
        S32(st, G_DIST_X) = dx;
        int32_t dy = absdiff(S16(st, fr + FIG_Y), y);
        S32(st, G_DIST_Y) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (d < best) {
            best = d;
            sel = i;
        }
    }
    if (best >= 10000) {
        return 0;
    }
    S32(st, G_ROAD_ACCESS_X) = U8(st, G_POINTS_X + 2u * (uint32_t) sel);
    S32(st, G_ROAD_ACCESS_Y) = U8(st, G_POINTS_Y + 2u * (uint32_t) sel);
    return 1;
}

int32_t figure_area_free_480d60(game_state *st, int32_t f)
{
    /* @bin 0x00480D60 1 when figure f is the first on its tile (+0x20) or no tile with terrain
     * bit 4 and no figure lies within 5 tiles of it (rings 1..5, clipped to the map; cursor
     * 0x00D3B4E4 / 0x00D3B4E0 left where the scan stopped); else 0 */
    uint32_t fr = FIGURE_ADDR(f);
    if (S16(st, G_GRID_FIGURE + 2u * U32(st, fr + FIG_GRID_OFFSET)) == f) {
        return 1;
    }
    int32_t fx = S16(st, fr + FIG_X), fy = S16(st, fr + FIG_Y);
    for (int32_t r = 1; r <= 5; r++) {
        int32_t x1 = fx + r, y1 = fy + r, x0 = fx - r, y0 = fy - r;
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
        int32_t step = x0 - x1 + 0xE3;
        int32_t off = S32(st, G_MAP_START) + y0 * 228 + x0;
        for (int32_t y = y0; y <= y1;) {
            S32(st, G_ROAD_ACCESS_X) = x0;
            for (int32_t x = x0; x <= x1;) {
                if ((U8(st, G_GRID_TERRAIN + 4u * (uint32_t) off) & 4) != 0 &&
                    S16(st, G_GRID_FIGURE + 2u * (uint32_t) off) == 0) {
                    return 0;
                }
                x++;
                off++;
                S32(st, G_ROAD_ACCESS_X) = x;
            }
            y++;
            off += step;
            S32(st, G_ROAD_ACCESS_Y) = y;
        }
    }
    return 1;
}

static int32_t run_480c60(game_state *st, const int32_t *a)
{
    return nearest_point_480c60(st, a[0]);
}

static int32_t run_480d60(game_state *st, const int32_t *a)
{
    return figure_area_free_480d60(st, a[0]);
}

const ported_function rest_misc_ports[] = {
    {0x00480C60u, "nearest_point_480c60", 1, 32, 0, run_480c60, 0},
    {0x00480D60u, "figure_area_free_480d60", 1, 32, 0, run_480d60, 0},
};
const int rest_misc_ports_count = sizeof(rest_misc_ports) / sizeof(rest_misc_ports[0]);
