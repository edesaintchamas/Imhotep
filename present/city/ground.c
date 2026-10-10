/* Imhotep - the city view's ground pass 0x00430EF0 (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Terrain and building footprints, row by row over the view-to-tile table 0x0070E494, with the
 * state writes of docs/presentation.md 1.4: the water animation into the image grid (chunk 7) when
 * the 60 ms flag 0x0070E3C4 is up, building 0's type := 0x27 before the ambient count 0x0041D410,
 * and the ambient counters 0x00F55B50.. of the tiles near the centre. */
#include "city.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

static void image_draw(game_state *st, int32_t image, int32_t x, int32_t y)
{
    cv_image_draw(st, image, x, y);
}

static void count_ambient(game_state *st, uint32_t terrain)
{
    /* @bin 0x004312A5 the terrain kinds near the centre (no building) */
    uint32_t a;
    if ((terrain & 4) != 0 && (terrain & 0x80000) == 0) {
        a = G_AMBIENT_COUNTERS + 4u * 10u;
    } else if (terrain & 2) {
        a = G_AMBIENT_COUNTERS + 4u * 9u;
    } else if (terrain & 0x800) {
        a = G_AMBIENT_COUNTERS + 4u * 7u;
    } else if (terrain & 0x50000) {
        a = G_AMBIENT_COUNTERS + 4u * 8u;
    } else if (terrain & 0x11) {
        a = G_AMBIENT_COUNTERS + 4u * 11u;
    } else if (terrain & 0x80) {
        a = G_AMBIENT_COUNTERS + 4u * 7u;
    } else {
        a = G_AMBIENT_COUNTERS + 4u * 6u;
    }
    S32(st, a) = S32(st, a) + 1;
}

static void ground_tile(game_state *st, int32_t x, int32_t y, int32_t col, int32_t row, int32_t half_cols,
                        int32_t half_rows, int odd, int8_t near_edge)
{
    int32_t tile = S32(st, G_VIEW_TILE);
    int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
    int32_t img = S32(st, G_GRID_IMAGE + 4u * (uint32_t) tile);
    uint32_t terrain = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    uint32_t edge = U8(st, G_GRID_BITFIELDS + (uint32_t) tile);
    S32(st, G_GLYPH_LAST) = img;
    int near = col > half_cols - 3 && col < half_cols + 3 && row > half_rows - 3 && row < half_rows + 3;
    if (b != 0) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_UBF) != 0) { /* @bin 0x00431101 */
            U16(st, G_TINT) = 0xF863;
        }
        if (near) {
            /* @bin 0x00431150 the ambient kind of the building type 0x0053B830: 1, 2 or 4 */
            int32_t type = S16(st, r + BLD_TYPE);
            if (GAME_CALL(st, FN_AMBIENT_CLASS, type) == 1) {
                S32(st, G_AMBIENT_COUNTERS + 4u * 1u) = S32(st, G_AMBIENT_COUNTERS + 4u * 1u) + 1;
            } else if (GAME_CALL(st, FN_AMBIENT_CLASS, (int32_t) S16(st, r + BLD_TYPE)) == 2) {
                S32(st, G_AMBIENT_COUNTERS + 4u * 2u) = S32(st, G_AMBIENT_COUNTERS + 4u * 2u) + 1;
            } else if (GAME_CALL(st, FN_AMBIENT_CLASS, (int32_t) S16(st, r + BLD_TYPE)) == 4) {
                S32(st, G_AMBIENT_COUNTERS + 4u * 4u) = S32(st, G_AMBIENT_COUNTERS + 4u * 4u) + 1;
            }
            tile = S32(st, G_VIEW_TILE);
            img = S32(st, G_GLYPH_LAST);
        }
    } else if (near) {
        count_ambient(st, terrain);
    }
    S32(st, G_AMBIENT_TOTAL) = S32(st, G_AMBIENT_TOTAL) + 1;
    if (terrain & 0x20) {
        /* @bin 0x00431191 building 0's type := 0x27, then its ambient count (0, 2) */
        S16(st, BUILDING_ADDR(0) + BLD_TYPE) = 0x27;
        GAME_CALL(st, FN_AMBIENT_BUILDING, 0, 2);
        tile = S32(st, G_VIEW_TILE);
        img = S32(st, G_GLYPH_LAST);
    }
    if (edge & 0x10) { /* @bin 0x004311B6 */
        img = S32(st, G_FLAT_IMAGE);
        S32(st, G_GLYPH_LAST) = img;
    }
    uint32_t corner = edge & 0xF;
    if (corner != 0) {
        /* @bin 0x00431538 the footprint drawn from its corner tile (1..5: the size), offset by the
         * size's half extents 0x0070E440..0x0070E45C */
        int32_t dx, dy;
        switch (corner) {
        case 1:
            dy = y - S32(st, G_TILE_H);
            dx = S32(st, G_TILE_W) + x + 1;
            break;
        case 2:
            dy = y - S32(st, G_U70E448);
            dx = S32(st, G_U70E458) + x + 2;
            break;
        case 3:
            dy = y - S32(st, G_CORNER_H3);
            dx = S32(st, G_CORNER_W3) + x + 3;
            break;
        case 4:
            dy = y - S32(st, G_CORNER_H4);
            dx = S32(st, G_CORNER_W4) + x + 4;
            break;
        case 5:
            dy = y - S32(st, G_CORNER_H4) - S32(st, G_TILE_H);
            dx = S32(st, G_CORNER_W4) + S32(st, G_TILE_W) + x + 5;
            break;
        default:
            goto done;
        }
        image_draw(st, img, dx, dy);
        goto done;
    }
    if ((terrain & 4) != 0 && S32(st, G_CV_ANIM_FLAG) != 0) {
        /* @bin 0x004311E6 the water animation: the image steps through its group (deep water
         * 0x4000000: 0x00F57224 / 0x00F56D24 by the map-edge test 0x0047A1C0, 6 frames of 15; else
         * 0x00F56BD4, 6 frames of 1), written back into the image grid */
        int32_t base, step, wrap;
        if (terrain & 0x4000000) {
            base = near_edge != 0 ? S32(st, G_IMAGE_GROUP(718)) : S32(st, G_IMAGE_GROUP(558));
            step = 0xF;
            wrap = 0x5A;
        } else {
            base = S32(st, G_IMAGE_GROUP(516));
            step = 1;
            wrap = 6;
        }
        int32_t last = base + step * 6 - 1;
        if (img <= last && img >= base) {
            img += step;
            S32(st, G_GLYPH_LAST) = img;
            if (img > last) {
                img -= wrap;
                S32(st, G_GLYPH_LAST) = img;
            }
            S32(st, G_GRID_IMAGE + 4u * (uint32_t) tile) = img;
        }
    }
    {
        /* @bin 0x00431355 images 0x00F572EC + 0x30 .. + 0x47 are not drawn here */
        int32_t skip = S32(st, G_IMAGE_FLAT_TOP);
        if (img < skip + 0x30 || img >= skip + 0x48) {
            image_draw(st, img, x, y);
            tile = S32(st, G_VIEW_TILE);
            img = S32(st, G_GLYPH_LAST);
        }
    }
    if ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 0xEEFED77Fu) == 0) {
        int32_t idx = S32(st, G_VIEW_ROW) * 0xE7 + S32(st, G_VIEW_COLUMN);
        int32_t t1, t2;
        if (odd) {
            t1 = S32(st, G_VIEW_TABLE - 4u * 0xE8u + 4u * (uint32_t) idx);
            t2 = S32(st, G_VIEW_TABLE - 4u * 0xE7u + 4u * (uint32_t) idx);
        } else {
            t1 = S32(st, G_VIEW_TABLE - 4u * 0xE7u + 4u * (uint32_t) idx);
            t2 = S32(st, G_VIEW_TABLE - 4u * 0xE6u + 4u * (uint32_t) idx);
        }
        if ((t1 < 0xFFF0000 && cv_tile_covers(st, t1)) || (t2 < 0xFFF0000 && cv_tile_covers(st, t2))) {
            cv_image_draw_top(st, img, x, y);
        }
    }
done:
    U16(st, G_TINT) = 0;
}

void city_ground_draw(game_state *st)
{
    /* @bin 0x00430EF0 */
    GAME_CALL0(st, FN_AMBIENT_COUNTERS_CLEAR); /* the ambient counters := 0 */
    int32_t y = S32(st, G_VIEW_Y) - 9 * S32(st, G_TILE_H);
    int32_t half_cols = (S32(st, G_VIEW_COLS) + 6) / 2;
    int32_t half_rows = (S32(st, G_VIEW_ROWS) + 0xE) / 2;
    int odd = 0;
    S32(st, G_VIEW_ROW) = S32(st, G_VIEW_FIRST_ROW) - 8;
    for (int32_t row = 0; row < S32(st, G_VIEW_ROWS) + 0xE; row++) {
        /* @bin 0x00430F5C rows alternate their start: 0x0070E490, then one tile left (0x0070E45C + 1) */
        int32_t x = S32(st, G_VIEW_X);
        if (!odd) {
            odd = 1;
        } else {
            odd = 0;
            x = x - S32(st, G_TILE_W) - 1;
        }
        x += (-2 - S32(st, G_U70E458)) * 4;
        S32(st, G_VIEW_COLUMN) = S32(st, G_VIEW_FIRST_COL) - 4;
        for (int32_t col = 0; col <= S32(st, G_VIEW_COLS) + 6; col++) {
            int32_t vx = S32(st, G_VIEW_COLUMN), vy = S32(st, G_VIEW_ROW);
            if (vx >= 0 && vx < 0xE7 && vy >= 0 && vy < 0x1C9) {
                S32(st, G_VIEW_TILE) = S32(st, G_VIEW_TABLE + 4u * (uint32_t) (vy * 0xE7 + vx));
                /* @bin 0x00430FF6 (view x, view y): 0x0047A1C0: its byte picks the deep water group */
                int8_t near_edge = (int8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, vx, vy);
                int32_t tile = S32(st, G_VIEW_TILE);
                if (tile == S32(st, G_HOVER_MARK)) {
                    S32(st, G_HOVER_MARK_X) = x;
                    S32(st, G_HOVER_MARK_Y) = y;
                }
                if (tile >= 0xFFF0000) {
                    /* @bin 0x00431029 off the map: the border image (0 in 0x00E38E54 mode 1) */
                    int32_t img = S32(st, G_UE38E54) == 1 ? 0 : tile - 0xFFF0000;
                    S32(st, G_GLYPH_LAST) = img;
                    image_draw(st, img, x, y);
                } else if (U8(st, G_GRID_EDGE + (uint32_t) tile) & 0x40) {
                    ground_tile(st, x, y, col, row, half_cols, half_rows, odd, near_edge);
                }
            }
            S32(st, G_VIEW_COLUMN) = S32(st, G_VIEW_COLUMN) + 1;
            x += S32(st, G_U70E458) + 2;
        }
        y += S32(st, G_TILE_H);
        S32(st, G_VIEW_ROW) = S32(st, G_VIEW_ROW) + 1;
    }
}

static int32_t run_ground(game_state *st, const int32_t *a)
{
    (void) a;
    city_ground_draw(st);
    return 0;
}

const ported_function cityview_ground_ports[] = {
    {0x00430EF0u, "city_ground_draw", 0, 0, 0, run_ground, 0},
};
const int cityview_ground_ports_count = sizeof(cityview_ground_ports) / sizeof(cityview_ground_ports[0]);
