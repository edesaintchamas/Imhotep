/* Imhotep - the overlay view's per-tile drawing (package cityview): the buildings an overlay does not
 * show (0x00432D30), the desirability-style ground 0x00433760, the overlays 0x2D (0x00431CE0,
 * 0x004321A0, 0x00432320) and 0x33 (0x00432620, 0x00432820).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Every function draws at (x, y), the walked tile 0x0070E488's screen position; footprints are drawn
 * from their corner tile (the size in the low 4 bits of 0x00B285CC). A building the overlay hides
 * is drawn as flat ground tiles: the images 0x00F56BDC + the per-size tables 0x005D267C.. at the
 * offsets 0x005D0050.. (one entry per tile of the footprint). */
#include "city.h"

#include "frame/view.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

/* @bin 0x00432F2F the per-size footprint tables: tile offsets (x, y pairs) and image offsets */
static const struct {
    uint32_t offsets, images;
    int32_t count;
} footprints[6] = {
    {0, 0, 1},
    {G_FOOTPRINT_POS2, G_FOOTPRINT_IMG2, 4},
    {G_FOOTPRINT_POS3, G_FOOTPRINT_IMG3, 9},
    {G_FOOTPRINT_POS4, G_FOOTPRINT_IMG4, 0x10},
    {G_FOOTPRINT_POS5, G_FOOTPRINT_IMG5, 0x19},
    {G_FOOTPRINT_POS6, G_FOOTPRINT_IMG6, 0x24},
};

static int32_t grid_image(game_state *st)
{
    return S32(st, G_GRID_IMAGE + 4u * (uint32_t) S32(st, G_VIEW_TILE));
}

static uint32_t walked_corner(game_state *st)
{
    return U8(st, G_GRID_BITFIELDS + (uint32_t) S32(st, G_VIEW_TILE)) & 0xF;
}

/* the walked tile's image (DRAW_IMAGE) from its corner; past5: drawn at (x, y) past size 5 (and at
 * corner 0), else not drawn past 5 */
static void corner_draw(game_state *st, uint32_t draw, int32_t x, int32_t y, int past5)
{
    int32_t img = grid_image(st);
    S32(st, G_GLYPH_LAST) = img;
    int32_t px, py;
    if (cv_overlay_corner_pos(st, walked_corner(st), x, y, &px, &py) < 0) {
        if (!past5) {
            return;
        }
        px = x;
        py = y;
    }
    GAME_CALLT(st, G_GRAPHICS_GFX, draw, img, px, py);
}

/* ---- 0x00432D30 ---- */

static int32_t farm_part(int32_t o, int32_t i, int32_t j, int32_t rows, int32_t cols)
{
    /* @bin 0x00432FDD the part of a 0x41..0x45 field drawn at (i, j) by orientation: 0 the corner,
     * 1 and 2 the edges, 3 the inside */
    switch (o) {
    case 0:
        if (i == 0) {
            if (j == 0) {
                return 0;
            }
            return j > 0 ? 1 : 3;
        }
        return (i > 0 && j == 0) ? 2 : 3;
    case 2:
        if (i == rows - 1) {
            if (j == 0) {
                return 0;
            }
            if (j > 0) {
                return 2;
            }
        }
        return (i > 0 && j == 0) ? 1 : 3;
    case 4:
        if (i == rows - 1) {
            if (j == cols - 1) {
                return 0;
            }
            if (j >= 0) {
                return 1;
            }
        }
        return (i >= 0 && j == cols - 1) ? 2 : 3;
    default:
        if (i == 0) {
            if (j == cols - 1) {
                return 0;
            }
            if (j >= 0) {
                return 2;
            }
            return j == cols - 1 ? 1 : 3;
        }
        if (i < 0) {
            return 3;
        }
        return j == cols - 1 ? 1 : 3;
    }
}

static void flat_footprint(game_state *st, int size, uint32_t r, int32_t x, int32_t y, int32_t extra)
{
    /* @bin 0x00433222 (and the 4 other sizes) one flat tile per footprint tile; +4 for a building in
     * use (the record r: +5) */
    for (int32_t k = 0; k < footprints[size].count; k++) {
        int32_t img = S32(st, G_FLAT_IMAGE) + S32(st, footprints[size].images + 4u * (uint32_t) k) + extra;
        S32(st, G_GLYPH_LAST) = img;
        if (U8(st, r + BLD_HOUSE_SIZE) != 0) {
            img += 4;
            S32(st, G_GLYPH_LAST) = img;
        }
        cv_image_draw(st, img, x + S32(st, footprints[size].offsets + 8u * (uint32_t) k),
                      y + S32(st, footprints[size].offsets + 8u * (uint32_t) k + 4));
    }
}

void city_overlay_hidden_building(game_state *st, int32_t b, int32_t x, int32_t y, int32_t extra)
{
    /* @bin 0x00432D30 (building, x, y, image offset) */
    if (b == 0) {
        return;
    }
    if (S32(st, G_OVERLAY) == 0x1F) {
        GAME_CALL(st, FN_CITY_BUILDING_PROBLEMS, b);
    }
    int32_t main = GAME_CALL(st, FN_BUILDING_MAIN_PART, b);
    int problem = S32(st, BUILDING_ADDR(main) + BLD_PROBLEMS) != 0 && S32(st, G_OVERLAY) == 0x1F;
    uint32_t r = BUILDING_ADDR(b);
    int16_t type;
    if ((uint8_t) GAME_CALL(st, FN_CITY_OVERLAY_SHOWS, main) != 0) {
        goto shown;
    }
    type = S16(st, r + BLD_TYPE);
    if (type == 0xD1 || problem) {
        goto shown;
    }
    int32_t tile = S32(st, G_VIEW_TILE);
    uint32_t corner = U8(st, G_GRID_BITFIELDS + (uint32_t) tile) & 0xF;
    if ((type == 0x1F || type == 0x1E || type == 0x21) && (U8(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 0x60) != 0) {
        /* @bin 0x00432DDB */
        int32_t img = S32(st, G_GRID_IMAGE + 4u * (uint32_t) tile);
        S32(st, G_GLYPH_LAST) = img;
        int32_t px, py;
        if (cv_overlay_corner_pos(st, corner, x, y, &px, &py) <= 0) {
            px = x;
            py = y;
        }
        cv_image_draw(st, img, px, py);
        return;
    }
    if (type == 0x48) {
        /* @bin 0x004335C3 by orientation, the 3x3 tables */
        int32_t o = S32(st, G_VIEW_ORIENTATION);
        if (o == 0) {
            x -= 0x3C;
            y += 0x1E;
        } else if (o == 4) {
            x -= 0x3C;
            y -= 0x1E;
        } else if (o == 6) {
            x -= 0x78;
        }
        for (int32_t k = 0; k < 9; k++) {
            int32_t img = S32(st, G_FLAT_IMAGE) + S32(st, G_FOOTPRINT_IMG3 + 4u * (uint32_t) k);
            S32(st, G_GLYPH_LAST) = img;
            if (U8(st, r + BLD_HOUSE_SIZE) != 0) {
                img += 4;
                S32(st, G_GLYPH_LAST) = img;
            }
            cv_image_draw(st, img, x + S32(st, G_FOOTPRINT_POS3 + 8u * (uint32_t) k), y + S32(st, G_FOOTPRINT_POS3 + 4u + 8u * (uint32_t) k));
        }
        return;
    }
    if (type == 0x49) {
        return;
    }
    if (type >= 0x41 && type <= 0x45) {
        /* @bin 0x00432F0B the field: its tiles at their screen positions (0x004F0BA0) from +6 / +8
         * moved by 0x005D1FF8[+0xAA >> 1], by +0xAA: 13 x 7 / 7 x 13 tiles, step 1 or 5 */
        if (S16(st, r + BLD_PREV_PART) != 0) {
            return;
        }
        uint32_t o = U8(st, r + BLD_ORIENTATION);
        uint32_t k8 = (o >> 1) * 8u;
        int32_t tx = S16(st, r + BLD_X) + S32(st, G_COMPLEX_ORIGIN_SHIFT + k8);
        int32_t ty = S16(st, r + BLD_Y) + S32(st, G_COMPLEX_ORIGIN_SHIFT + 4u + k8);
        int32_t rows, cols, step;
        switch (o) {
        case 0:
            rows = 0xD, cols = 7, step = 1;
            break;
        case 2:
            rows = 7, cols = 0xD, step = 5;
            break;
        case 4:
            rows = 0xD, cols = 7, step = 5;
            break;
        case 6:
            rows = 7, cols = 0xD, step = 1;
            break;
        default:
            return;
        }
        int32_t sx = 0, sy = 0; /* (the original's stack outputs; always written for orientations 0..6) */
        for (int32_t i = 0; i < rows; i++) {
            for (int32_t j = 0; j < cols; j++) {
                int32_t part = farm_part(S32(st, G_VIEW_ORIENTATION), i, j, rows, cols);
                view_tile_to_screen(st, i * step + tx, j * step + ty, &sx, &sy);
                int32_t img = S32(st, G_FLAT_IMAGE) + part;
                S32(st, G_GLYPH_LAST) = img;
                cv_image_draw(st, img, sx, sy);
            }
        }
        return;
    }
    if (corner == 0) {
        /* @bin 0x0043311B */
        int32_t img = S32(st, G_FLAT_IMAGE);
        S32(st, G_GLYPH_LAST) = img;
        if (U8(st, r + BLD_HOUSE_SIZE) != 0) {
            if (S32(st, G_OVERLAY) == 0x1F && U8(st, r + BLD_PROBLEM_OVERLAY) != 0) {
                img = S32(st, G_GRID_IMAGE + 4u * (uint32_t) tile);
            } else {
                img += 4;
            }
            img += extra;
        } else if (S32(st, G_OVERLAY) == 0x1F) {
            int32_t m = GAME_CALL(st, FN_BUILDING_MAIN_PART, b);
            if (S32(st, BUILDING_ADDR(m) + BLD_PROBLEMS) != 0) {
                img = grid_image(st);
            } else {
                img = S32(st, G_GLYPH_LAST) + extra;
            }
        } else {
            img += extra;
        }
        S32(st, G_GLYPH_LAST) = img;
        cv_image_draw(st, img, x, y);
        return;
    }
    if (corner > 5) {
        return;
    }
    if (S32(st, G_OVERLAY) == 0x1F) {
        if (corner <= 2) {
            /* @bin 0x004331BD sizes 1, 2: the main building's +0x100; its record for the in-use test */
            int32_t m = GAME_CALL(st, FN_BUILDING_MAIN_PART, b);
            r = BUILDING_ADDR(m);
            if (S32(st, r + BLD_PROBLEMS) != 0) {
                corner_draw(st, FN_IMAGE_DRAW, x, y, 0);
                return;
            }
        } else if (S32(st, r + BLD_PROBLEMS) != 0) {
            /* @bin 0x00433362 sizes 3..5: the building's own +0x100 */
            GAME_CALL(st, FN_BUILDING_MAIN_PART, b);
            corner_draw(st, FN_IMAGE_DRAW, x, y, 0);
            return;
        }
    }
    flat_footprint(st, (int) corner, r, x, y, extra);
    return;
shown:
    /* @bin 0x0043366B the building's own image (past size 5: at the tile) */
    corner_draw(st, FN_IMAGE_DRAW, x, y, 1);
}

/* ---- 0x00433760 ---- */

void city_overlay_desirability_tile(game_state *st, int32_t x, int32_t y, int top)
{
    /* @bin 0x00433760 (x, y, top: 0x0053F250 instead of 0x0053E590) */
    uint32_t draw = top ? FN_IMAGE_DRAW_TOP : FN_IMAGE_DRAW;
    int32_t tile = S32(st, G_VIEW_TILE);
    uint32_t t = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    int32_t d = S8(st, G_GRID_DESIRABILITY + (uint32_t) tile);
    if ((t & 0x26341777u) != 0 && (t & 8) == 0) {
        if ((U8(st, G_GRID_EDGE + (uint32_t) tile) & 0x40) == 0) {
            return;
        }
        corner_draw(st, draw, x, y, 0);
        return;
    }
    if (t & 0x4000) {
        /* @bin 0x004338B8 the variant 0x00ACF85C & 7 of 0x00F56B8C */
        int32_t img = (U8(st, G_GRID_RANDOM + (uint32_t) tile) & 7) + S32(st, G_IMAGE_GROUP(507));
        S32(st, G_GLYPH_LAST) = img;
        GAME_CALLT(st, G_GRAPHICS_GFX, draw, img, x, y);
        return;
    }
    if ((t & 0x28) == 0 && d == 0) {
        int32_t img = S32(st, G_GRID_IMAGE + 4u * (uint32_t) tile);
        S32(st, G_GLYPH_LAST) = img;
        GAME_CALLT(st, G_GRAPHICS_GFX, draw, img, x, y);
        return;
    }
    /* @bin 0x0043390E the value's image: group 0x22B (a building in use) or 0x22A, + 0 (< -10),
     * 9 (>= 0x46), else value / 10 + 2 */
    int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
    int32_t group = (b != 0 && U8(st, BUILDING_ADDR(b) + BLD_HOUSE_SIZE) != 0) ? 0x22B : 0x22A;
    int32_t k;
    if (d < -10) {
        k = 0;
    } else if (d >= 0x46) {
        k = 9;
    } else {
        k = d / 10 + 2;
    }
    GAME_CALLT(st, G_GRAPHICS_GFX, draw, cv_group_base(st, group) + k, x, y);
}

/* ---- overlay 0x2D ---- */

static void ground_footprint(game_state *st, uint32_t draw, int32_t x, int32_t y, uint32_t t)
{
    /* @bin 0x004321EF (and 0x00431ECE, 0x0043251E) the footprint's ground tiles (past size 5: the tile
     * alone): 0x00F56D14 (0x00F56D1C for a building in use) + 2 (t & 0x2000, or in use with +0x77),
     * + 1 (t == 0x80: the terrain word, masked with 0x2080 by 0x004321A0's and 0x00432320's callers) */
    int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) S32(st, G_VIEW_TILE));
    uint32_t r = BUILDING_ADDR(b);
    int in_use = U8(st, r + BLD_HOUSE_SIZE) != 0;
    uint32_t corner = walked_corner(st);
    int size = corner > 5 ? 0 : (int) corner;
    int32_t img = in_use ? S32(st, G_IMAGE_GROUP(557)) : S32(st, G_WATER_IMAGE);
    if ((t & 0x2000) != 0 || (in_use && U8(st, r + BLD_ACCESS_WATER) != 0)) {
        img += 2;
    } else if (t == 0x80) {
        img += 1;
    }
    for (int32_t k = 0; k < footprints[size].count; k++) {
        int32_t ox = size ? S32(st, footprints[size].offsets + 8u * (uint32_t) k) : 0;
        int32_t oy = size ? S32(st, footprints[size].offsets + 8u * (uint32_t) k + 4) : 0;
        GAME_CALLT(st, G_GRAPHICS_GFX, draw, img, x + ox, y + oy);
    }
}

void city_overlay_water_ground(game_state *st, int32_t x, int32_t y, uint32_t t2080, int top)
{
    /* @bin 0x004321A0 (x, y, terrain & 0x2080, top) a building in use with +0x77: 0x00432D30 */
    int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) S32(st, G_VIEW_TILE));
    uint32_t r = BUILDING_ADDR(b);
    if (U8(st, r + BLD_HOUSE_SIZE) != 0 && U8(st, r + BLD_ACCESS_WATER) != 0) {
        city_overlay_hidden_building(st, b, x, y, 0);
        return;
    }
    ground_footprint(st, top ? FN_IMAGE_DRAW_TOP : FN_IMAGE_DRAW, x, y, t2080);
}

void city_overlay_water_tile(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00431CE0 (overlay 0x2D, ground) */
    int32_t tile = S32(st, G_VIEW_TILE);
    uint32_t t = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    if ((t & 0x26341777u) != 0) {
        if (t & 8) {
            S32(st, G_GLYPH_LAST) = S32(st, G_FLAT_IMAGE);
            city_overlay_hidden_building(st, S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile), x, y, 0);
            return;
        }
        corner_draw(st, FN_IMAGE_DRAW, x, y, 0);
        return;
    }
    if (t & 0x40000000) {
        /* @bin 0x00431E80 */
        int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_HOUSE_SIZE) != 0 && U8(st, r + BLD_ACCESS_WATER) != 0) {
            city_overlay_hidden_building(st, b, x, y, 0);
            return;
        }
        ground_footprint(st, FN_IMAGE_DRAW, x, y, t);
        return;
    }
    if (t & 0x4000) {
        int32_t img = (U8(st, G_GRID_RANDOM + (uint32_t) tile) & 7) + S32(st, G_IMAGE_GROUP(507));
        S32(st, G_GLYPH_LAST) = img;
        cv_image_draw(st, img, x, y);
        return;
    }
    if ((t & 8) == 0) {
        /* @bin 0x00431FEE */
        int32_t img = S32(st, G_WATER_IMAGE);
        S32(st, G_GLYPH_LAST) = img;
        if (t & 0x2000) {
            img += 2;
            S32(st, G_GLYPH_LAST) = img;
        } else if ((t & 0x2080) == 0x80) {
            img += 1;
            S32(st, G_GLYPH_LAST) = img;
        }
        cv_image_draw(st, img, x, y);
        return;
    }
    if ((uint8_t) GAME_CALL(st, FN_CITY_OVERLAY_SHOWS, (int32_t) S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile)) != 0) {
        corner_draw(st, FN_IMAGE_DRAW, x, y, 1);
        return;
    }
    city_overlay_water_ground(st, x, y, t & 0x2080, 0);
}

void city_overlay_water_top(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00432320 (overlay 0x2D, tops) */
    int32_t tile = S32(st, G_VIEW_TILE);
    uint32_t t = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    if ((t & 0x26341777u) != 0) {
        if (t & 8) {
            return;
        }
        corner_draw(st, FN_IMAGE_DRAW_TOP, x, y, 0);
        return;
    }
    if ((t & 8) == 0) {
        /* @bin 0x0043248E */
        int32_t img = S32(st, G_WATER_IMAGE);
        S32(st, G_GLYPH_LAST) = img;
        if (t & 0x2000) {
            img += 2;
            S32(st, G_GLYPH_LAST) = img;
        } else if ((t & 0x2080) == 0x80) {
            img += 1;
            S32(st, G_GLYPH_LAST) = img;
        }
        cv_image_draw_top(st, img, x, y);
        return;
    }
    int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
    uint32_t r = BUILDING_ADDR(b);
    if (U8(st, r + BLD_HOUSE_SIZE) != 0 && U8(st, r + BLD_ACCESS_WATER) != 0) {
        city_overlay_hidden_building(st, b, x, y, 0);
        return;
    }
    ground_footprint(st, FN_IMAGE_DRAW_TOP, x, y, t & 0x2080);
}

/* ---- overlay 0x33 ---- */

void city_overlay_flood_tile(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00432620 (overlay 0x33, ground) flood plain water (0x20000002): 0x00F56D14 on the
     * footprint (sizes 1, 2; else the tile alone) */
    int32_t tile = S32(st, G_VIEW_TILE);
    if ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile) & 0x20000002u) == 0x20000002u) {
        uint32_t corner = U8(st, G_GRID_BITFIELDS + (uint32_t) tile) & 0xF;
        S32(st, G_GLYPH_LAST) = S32(st, G_WATER_IMAGE);
        int size = corner == 1 ? 1 : corner == 2 ? 2 : 0;
        for (int32_t k = 0; k < footprints[size].count; k++) {
            int32_t ox = size ? S32(st, footprints[size].offsets + 8u * (uint32_t) k) : 0;
            int32_t oy = size ? S32(st, footprints[size].offsets + 8u * (uint32_t) k + 4) : 0;
            cv_image_draw(st, S32(st, G_GLYPH_LAST), x + ox, y + oy);
        }
        return;
    }
    corner_draw(st, FN_IMAGE_DRAW, x, y, 0);
}

void city_overlay_flood_top(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00432820 (overlay 0x33, tops) */
    if ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) S32(st, G_VIEW_TILE)) & 0x20000002u) == 0x20000002u) {
        int32_t img = S32(st, G_WATER_IMAGE);
        S32(st, G_GLYPH_LAST) = img;
        cv_image_draw_top(st, img, x, y);
        return;
    }
    corner_draw(st, FN_IMAGE_DRAW_TOP, x, y, 1);
}

/* ---- overlay 0x1C ---- */

static void level_tile(game_state *st, int32_t x, int32_t y, uint32_t draw)
{
    /* @bin 0x00434E10 / 0x00435050 (overlay 0x1C; ground / tops) a tile with a value 0x00A5D4CC
     * (u8): 0x00F56CA4 + value / 10 + 1 (at most + 9); else the terrain 0x26341677 without 8: the
     * footprint from its corner (TILE_DRAW 0x40; past 5: nothing), else the tile's image */
    int32_t tile = S32(st, G_VIEW_TILE);
    uint32_t v = U8(st, G_FERTILITY_GRID + (uint32_t) tile);
    uint32_t t = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) tile);
    if (v != 0) {
        int32_t k = (int32_t) (v / 10 + 1);
        if (k > 9) {
            k = 9;
        }
        int32_t img = S32(st, G_IMAGE_GROUP(542)) + k;
        S32(st, G_GLYPH_LAST) = img;
        GAME_CALLT(st, G_GRAPHICS_GFX, draw, img, x, y);
        return;
    }
    if ((t & 0x26341677u) == 0 || (t & 8) != 0) {
        int32_t img = S32(st, G_GRID_IMAGE + 4u * (uint32_t) tile);
        S32(st, G_GLYPH_LAST) = img;
        GAME_CALLT(st, G_GRAPHICS_GFX, draw, img, x, y);
        return;
    }
    if ((U8(st, G_GRID_EDGE + (uint32_t) tile) & 0x40) == 0) {
        return;
    }
    corner_draw(st, draw, x, y, 0);
}

static int32_t run_level(game_state *st, const int32_t *a)
{
    level_tile(st, a[0], a[1], FN_IMAGE_DRAW);
    return 0;
}
static int32_t run_level_top(game_state *st, const int32_t *a)
{
    level_tile(st, a[0], a[1], FN_IMAGE_DRAW_TOP);
    return 0;
}
static int32_t run_hidden(game_state *st, const int32_t *a)
{
    city_overlay_hidden_building(st, a[0], a[1], a[2], a[3]);
    return 0;
}
static int32_t run_desir(game_state *st, const int32_t *a)
{
    city_overlay_desirability_tile(st, a[0], a[1], (uint8_t) a[2] != 0);
    return 0;
}
static int32_t run_desir_top(game_state *st, const int32_t *a)
{
    city_overlay_desirability_tile(st, a[0], a[1], 1);
    return 0;
}
static int32_t run_draw(game_state *st, const int32_t *a)
{
    cv_image_draw(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_draw_top(game_state *st, const int32_t *a)
{
    cv_image_draw_top(st, a[0], a[1], a[2]);
    return 0;
}
static int32_t run_water_ground(game_state *st, const int32_t *a)
{
    city_overlay_water_ground(st, a[0], a[1], (uint32_t) a[2], (uint8_t) a[3] != 0);
    return 0;
}
static int32_t run_water_tile(game_state *st, const int32_t *a)
{
    city_overlay_water_tile(st, a[0], a[1]);
    return 0;
}
static int32_t run_water_top(game_state *st, const int32_t *a)
{
    city_overlay_water_top(st, a[0], a[1]);
    return 0;
}
static int32_t run_flood_tile(game_state *st, const int32_t *a)
{
    city_overlay_flood_tile(st, a[0], a[1]);
    return 0;
}
static int32_t run_flood_top(game_state *st, const int32_t *a)
{
    city_overlay_flood_top(st, a[0], a[1]);
    return 0;
}

const ported_function cityview_overlay_tile_ports[] = {
    {0x00432D30u, "city_overlay_hidden_building", 4, 0, 0, run_hidden, 0},
    {0x00433760u, "city_overlay_desirability_tile", 3, 0, 0, run_desir, 0},
    {0x004339C0u, "city_overlay_desirability_top", 2, 0, 0, run_desir_top, 0},
    {0x00433980u, "city_overlay_draw_433980", 3, 0, 0, run_draw, 0},
    {0x004339A0u, "city_overlay_draw_top_4339a0", 3, 0, 0, run_draw_top, 0},
    {0x004321A0u, "city_overlay_water_ground", 4, 0, 0, run_water_ground, 0},
    {0x00431CE0u, "city_overlay_water_tile", 2, 0, 0, run_water_tile, 0},
    {0x00432320u, "city_overlay_water_top", 2, 0, 0, run_water_top, 0},
    {0x00432620u, "city_overlay_flood_tile", 2, 0, 0, run_flood_tile, 0},
    {0x00432820u, "city_overlay_flood_top", 2, 0, 0, run_flood_top, 0},
    {0x00434E10u, "city_overlay_level_tile", 2, 0, 0, run_level, 0},
    {0x00435050u, "city_overlay_level_top", 2, 0, 0, run_level_top, 0},
};
const int cityview_overlay_tile_ports_count =
    sizeof(cityview_overlay_tile_ports) / sizeof(cityview_overlay_tile_ports[0]);
