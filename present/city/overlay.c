/* Imhotep - the city view's overlay passes (package cityview): the overlay ground 0x004316F0 and
 * the overlay tops 0x00439F10.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The overlay view (0x00E94E0C != 0, docs/presentation.md 3.1) draws the ground with the overlay's
 * per-tile functions instead of the ground pass 0x00430EF0, then 0x00439F10: the figures (one
 * walk), the buildings' tops or the overlay's columns (0x004354B0.. by overlay), and the animated
 * parts of the buildings the overlay keeps (0x0040F2C0: the sprite grid, state). */
#include "city.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define CORNER_H1 0x0070E44Cu /* the corner offsets of footprints of size 1..5 (as in 0x00430EF0) */
#define CORNER_H2 0x0070E448u
#define CORNER_H3 0x0070E444u
#define CORNER_H4 0x0070E440u
#define CORNER_W1 0x0070E45Cu
#define CORNER_W2 0x0070E458u
#define CORNER_W3 0x0070E454u
#define CORNER_W4 0x0070E450u
#define HOVER_MARK 0x006059D0u
#define HOVER_MARK_X 0x0070E3ECu
#define HOVER_MARK_Y 0x0070E3E8u
#define IMAGE_FLAT_TOP 0x00F572ECu /* group base: images + 0x30 .. + 0x47 are drawn 0x96 higher */

/* @bin 0x00431812 the building types drawn as themselves on every overlay: 0xE5, 0xEA, 0xEB, 0xEC,
 * 0xE6 (16-bit compares) */
static int overlay_kept(int16_t type)
{
    return type == 0xE5 || type == 0xEA || type == 0xEB || type == 0xEC || type == 0xE6;
}

static int16_t building_type(game_state *st, int32_t b)
{
    return S16(st, BUILDING_ADDR(b) + 0x10);
}

/* a footprint drawn from its corner tile (corner 1..5: its size) as 0x00430EF0 places it; 0 for
 * corner 0 (the tile's own position), -1 past 5 */
int cv_overlay_corner_pos(game_state *st, uint32_t corner, int32_t x, int32_t y, int32_t *px, int32_t *py)
{
    switch (corner) {
    case 0:
        *px = x;
        *py = y;
        return 0;
    case 1:
        *px = S32(st, CORNER_W1) + x + 1;
        *py = y - S32(st, CORNER_H1);
        return 1;
    case 2:
        *px = S32(st, CORNER_W2) + x + 2;
        *py = y - S32(st, CORNER_H2);
        return 1;
    case 3:
        *px = S32(st, CORNER_W3) + x + 3;
        *py = y - S32(st, CORNER_H3);
        return 1;
    case 4:
        *px = S32(st, CORNER_W4) + x + 4;
        *py = y - S32(st, CORNER_H4);
        return 1;
    case 5:
        *px = S32(st, CORNER_W4) + S32(st, CORNER_W1) + x + 5;
        *py = y - S32(st, CORNER_H4) - S32(st, CORNER_H1);
        return 1;
    default:
        return -1;
    }
}

/* @bin 0x00431884 (and 0x0043A1F7 in the tops) the overlays 0x21 .. 0x2A: one function 0x00434AA0
 * with two parameters each; 0 when the overlay is not one of them */
static int risk_params(int32_t overlay, int32_t *p, int32_t *q)
{
    static const int32_t params[10][2] = {{1, 0}, {3, 2}, {6, 5}, {2, 1}, {4, 3},
                                          {5, 4}, {0xD, 9}, {0x13, 0xB}, {0x11, 0xA}, {0xF, 8}};
    if (overlay < 0x21 || overlay > 0x2A) {
        return 0;
    }
    *p = params[overlay - 0x21][0];
    *q = params[overlay - 0x21][1];
    return 1;
}

/* ---- 0x004316F0 the overlay ground ---- */

static void ground_image(game_state *st, int32_t img, int32_t x, int32_t y)
{
    S32(st, DRAW_IMAGE) = img;
    cv_image_draw(st, img, x, y);
}

static void ground_footprint(game_state *st, int32_t tile, int32_t x, int32_t y)
{
    /* @bin 0x00431B34 the tile's image from its footprint corner (sizes 1..5; past 5: nothing) */
    int32_t img = S32(st, IMAGE_GRID + 4u * (uint32_t) tile);
    S32(st, DRAW_IMAGE) = img;
    int32_t px, py;
    if (cv_overlay_corner_pos(st, U8(st, EDGE_GRID + (uint32_t) tile) & 0xF, x, y, &px, &py) >= 0) {
        cv_image_draw(st, img, px, py);
    }
}

static void overlay_ground_tile(game_state *st, int32_t tile, int32_t x, int32_t y)
{
    int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) tile);
    int kept = overlay_kept(building_type(st, b));
    int32_t overlay = S32(st, OVERLAY);
    int32_t p, q;
    if (overlay == 0x1B) {
        /* @bin 0x0043183B */
        if (!kept) {
            GAME_CALL(st, 0x00433760u, x, y, 0);
            return;
        }
    } else if (overlay == 0x1C) {
        if (!kept) {
            GAME_CALL(st, 0x00434E10u, x, y);
            return;
        }
    } else if (risk_params(overlay, &p, &q)) {
        if (!kept) {
            GAME_CALL(st, 0x00434AA0u, x, y, p, q);
            return;
        }
    }
    /* @bin 0x004319D5 */
    if ((U8(st, TILE_DRAW + (uint32_t) tile) & 0x40) == 0) {
        return;
    }
    if (overlay_kept(building_type(st, b))) {
        ground_footprint(st, tile, x, y);
        return;
    }
    overlay = S32(st, OVERLAY);
    if (overlay == 0x2D) {
        GAME_CALL(st, 0x00431CE0u, x, y);
        return;
    }
    if (overlay == 0x1E) {
        GAME_CALL(st, 0x0051F780u, x, y);
        return;
    }
    if (overlay == 0x33 || U8(st, NO_GROUND) != 0) {
        GAME_CALL(st, 0x00432620u, x, y);
        return;
    }
    uint32_t corner = U8(st, EDGE_GRID + (uint32_t) tile) & 0xF;
    uint32_t terrain = U32(st, TERRAIN + 4u * (uint32_t) tile);
    if (terrain & 0x100) {
        /* @bin 0x00431A7D only the corner-0 tiles */
        S32(st, DRAW_IMAGE) = S32(st, IMAGE_GRID + 4u * (uint32_t) tile);
        if (corner == 0) {
            cv_image_draw(st, S32(st, DRAW_IMAGE), x, y);
        }
        return;
    }
    if ((terrain & 0x40) != 0 && (terrain & 8) == 0) {
        /* @bin 0x00431A9D sizes 0 and 1 only */
        int32_t img = S32(st, IMAGE_GRID + 4u * (uint32_t) tile);
        S32(st, DRAW_IMAGE) = img;
        if (corner == 0) {
            cv_image_draw(st, img, x, y);
        } else if (corner == 1) {
            cv_image_draw(st, img, S32(st, CORNER_W1) + x + 1, y - S32(st, CORNER_H1));
        }
        return;
    }
    if (terrain & 0x40000000) {
        GAME_CALL(st, 0x00432D30u, b, x, y, 0);
        return;
    }
    if (S32(st, OVERLAY) != 0x31 && (terrain & 0xC04000) != 0) {
        /* @bin 0x00431B04 the plain ground image 0x00F56BDC */
        ground_image(st, S32(st, 0x00F56BDCu), x, y);
        return;
    }
    if (terrain & 8) {
        GAME_CALL(st, 0x00432D30u, b, x, y, 0);
        return;
    }
    ground_footprint(st, tile, x, y);
}

void city_overlay_ground_draw(game_state *st)
{
    /* @bin 0x004316F0 the walk of 0x00430EF0: rows 0 .. 0x0070E460 + 0xE, columns 0 .. 0x0070E464 + 6 */
    int32_t y = S32(st, VIEW_Y0) - 9 * S32(st, TILE_H);
    int odd = 0;
    S32(st, VIEW_ROW) = S32(st, CV_CAMERA_Y) - 8;
    for (int32_t row = 0; row < S32(st, VIEW_ROWS) + 0xE; row++) {
        int32_t x = S32(st, VIEW_X0);
        if (!odd) {
            odd = 1;
        } else {
            odd = 0;
            x = x - S32(st, CORNER_W1) - 1;
        }
        x += (-2 - S32(st, TILE_HALF_W)) * 4;
        S32(st, VIEW_COL) = S32(st, CV_CAMERA_X) - 4;
        for (int32_t col = 0; col <= S32(st, VIEW_COLS) + 6; col++) {
            int32_t vx = S32(st, VIEW_COL), vy = S32(st, VIEW_ROW);
            if (vx >= 0 && vx < 0xE7 && vy >= 0 && vy < 0x1C9) {
                int32_t tile = S32(st, VIEW_TABLE + 4u * (uint32_t) (vy * 0xE7 + vx));
                S32(st, VIEW_TILE) = tile;
                if (tile == S32(st, HOVER_MARK)) {
                    S32(st, HOVER_MARK_X) = x;
                    S32(st, HOVER_MARK_Y) = y;
                }
                if (tile >= 0xFFF0000) {
                    /* @bin 0x004317E3 off the map: the border image */
                    ground_image(st, tile - 0xFFF0000, x, y);
                } else {
                    overlay_ground_tile(st, tile, x, y);
                }
            }
            S32(st, VIEW_COL) = S32(st, VIEW_COL) + 1;
            x += S32(st, TILE_HALF_W) + 2;
        }
        y += S32(st, TILE_H);
        S32(st, VIEW_ROW) = S32(st, VIEW_ROW) + 1;
    }
}

/* ---- 0x00439F10 the overlay tops ---- */

static void top_footprint(game_state *st, int32_t img, int32_t x, int32_t y, int past5)
{
    /* the top 0x0053F250 from the footprint corner; past 5: at the tile (past5) or nothing */
    int32_t px, py;
    int k = cv_overlay_corner_pos(st, U8(st, EDGE_GRID + (uint32_t) S32(st, VIEW_TILE)) & 0xF, x, y, &px, &py);
    if (k < 0) {
        if (!past5) {
            return;
        }
        px = x;
        py = y;
    }
    cv_image_draw_top(st, img, px, py);
}

static int flat_top(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0043A353 images 0x00F572EC + 0x30 .. + 0x47: drawn and topped 0x96 higher */
    int32_t img = S32(st, IMAGE_GRID + 4u * (uint32_t) S32(st, VIEW_TILE));
    S32(st, DRAW_IMAGE) = img;
    int32_t base = S32(st, IMAGE_FLAT_TOP);
    if (img < base + 0x30 || img >= base + 0x48) {
        return 0;
    }
    cv_image_draw(st, img, x, y - 0x96);
    cv_image_draw_top(st, S32(st, DRAW_IMAGE), x, y - 0x96);
    return 1;
}

static void overlay_column(game_state *st, int32_t overlay, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x0043A64D the overlay's column for a building: one function per overlay */
    uint32_t f;
    switch (overlay) {
    case 8: f = 0x00435290u; break;
    case 9: f = 0x004353A0u; break;
    case 0xA: f = 0x00435570u; break;
    case 0x2B: f = 0x004354B0u; break;
    case 0x30: f = 0x00435510u; break;
    case 0xB: f = 0x004355D0u; break;
    case 0x11: f = 0x00435630u; break;
    case 0xC: f = 0x004356A0u; break;
    case 0xD: f = 0x00435700u; break;
    case 0xE: f = 0x00435760u; break;
    case 0xF: f = 0x004357C0u; break;
    case 0x10: f = 0x00435820u; break;
    case 0x1A: f = 0x00435880u; break;
    case 4: f = 0x00435A70u; break;
    case 0x12: f = 0x00435AC0u; break;
    case 0x13: f = 0x00435B20u; break;
    case 0x14: f = 0x0051F780u; break;
    case 0x15: f = 0x00435B80u; break;
    case 0x2D: f = 0x00435A10u; break;
    case 0x17: f = 0x00435C40u; break;
    case 0x16: f = 0x00435BE0u; break;
    case 0x18: f = 0x00435CA0u; break;
    case 0x19: f = 0x00435D60u; break;
    case 0x1D: f = 0x00436140u; break;
    case 0x1F: f = 0x00435E70u; break;
    case 0x2C: f = 0x004360A0u; break;
    case 0x32: f = 0x00435D00u; break;
    default: return;
    }
    GAME_CALL(st, f, b, x, y);
}

static void tops_tile(game_state *st, int32_t tile, int32_t x, int32_t y)
{
    int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) tile);
    int kept = overlay_kept(building_type(st, b));
    if ((U32(st, TERRAIN + 4u * (uint32_t) tile) & 0x44) == 0x44 && U8(st, SPRITE_GRID + (uint32_t) tile) != 0) {
        /* @bin 0x0043A127 the sprite on a 0x44 tile (group 0x00F56D34) */
        cv_image_draw(st, U8(st, SPRITE_GRID + (uint32_t) tile) + S32(st, 0x00F56D34u) - 1, x,
                      y - S32(st, CORNER_H2));
        return;
    }
    int32_t overlay = S32(st, OVERLAY);
    int32_t p, q;
    if (overlay == 0x1B) {
        if (!kept) {
            GAME_CALL(st, 0x004339C0u, x, y);
            flat_top(st, x, y);
            return;
        }
    } else if (overlay == 0x1C) {
        if (!kept) {
            GAME_CALL(st, 0x00435050u, x, y);
            return;
        }
    } else if (risk_params(overlay, &p, &q)) {
        if (!kept) {
            GAME_CALL(st, 0x00434AA0u, x, y, p, q);
            return;
        }
    }
    /* @bin 0x0043A327 */
    if ((U8(st, TILE_DRAW + (uint32_t) tile) & 0x40) == 0) {
        return;
    }
    if (S32(st, OVERLAY) == 0x2D && !kept) {
        GAME_CALL(st, 0x00432320u, x, y);
    }
    if (flat_top(st, x, y)) {
        return;
    }
    overlay = S32(st, OVERLAY);
    if (overlay == 0x33 || U8(st, NO_GROUND) != 0) {
        GAME_CALL(st, 0x00432820u, x, y);
        return;
    }
    if (overlay == 0x1E) {
        GAME_CALL(st, 0x0051F780u, x, y);
        return;
    }
    tile = S32(st, VIEW_TILE);
    b = S16(st, BUILDING_GRID + 2u * (uint32_t) tile);
    uint32_t terrain = U32(st, TERRAIN + 4u * (uint32_t) tile);
    int32_t img = S32(st, DRAW_IMAGE); /* the image grid's, read by flat_top */
    if (overlay_kept(building_type(st, b))) {
        /* @bin 0x0043A9D3 */
        S32(st, DRAW_IMAGE) = img;
        top_footprint(st, img, x, y, 0);
        return;
    }
    if ((terrain & 8) == 0) {
        /* @bin 0x0043A88D */
        if (terrain & 0x40000000) {
            return;
        }
        if ((terrain & 0x80) != 0 && S32(st, OVERLAY) == 0x2D) {
            return;
        }
        if ((terrain & 0xC04000) != 0 && S32(st, OVERLAY) != 0x31) {
            return;
        }
        S32(st, DRAW_IMAGE) = img;
        top_footprint(st, img, x, y, 0);
        return;
    }
    if (b == 0) {
        return;
    }
    if ((uint8_t) GAME_CALL(st, 0x00431690u, b) != 0) {
        /* @bin 0x0043A450 a building the overlay shows: its top (past 5: at the tile) */
        img = S32(st, IMAGE_GRID + 4u * (uint32_t) S32(st, VIEW_TILE));
        S32(st, DRAW_IMAGE) = img;
        top_footprint(st, img, x, y, 1);
        return;
    }
    int16_t type = building_type(st, b);
    if ((type == 0x21 || type == 0x1E || type == 0x1F) &&
        (U8(st, TERRAIN + 4u * (uint32_t) S32(st, VIEW_TILE)) & 0x60) != 0) {
        /* @bin 0x0043A536 */
        img = S32(st, IMAGE_GRID + 4u * (uint32_t) S32(st, VIEW_TILE));
        S32(st, DRAW_IMAGE) = img;
        top_footprint(st, img, x, y, 1);
        return;
    }
    if (type == 0xD1) {
        return;
    }
    overlay_column(st, S32(st, OVERLAY), b, x, y);
}

/* @bin 0x0043AC54 the animated part of a building the overlay shows (as in the upper pass) */
static void tops_animation(game_state *st, int32_t b, int32_t tile, int32_t img, int32_t x, int32_t y)
{
    uint32_t r = BUILDING_ADDR(b);
    int32_t frame = GAME_CALL(st, 0x0040F2C0u, img, tile);
    if (frame <= 0) {
        return;
    }
    int32_t frames = S16(st, cv_image_entry(st, img) + 0x1E);
    if (frame > frames) {
        frame = frames;
    }
    uint32_t corner = U8(st, EDGE_GRID + (uint32_t) S32(st, VIEW_TILE)) & 0xF;
    if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488050u, b) != 0) {
        GAME_CALLT(st, 0x00BCA350u, 0x00489500u, b, frame, x, y);
        return;
    }
    switch (S16(st, r + 0x10)) {
    case 0x1E:
        /* @bin 0x0043B1AA */
        if (img == S32(st, 0x00F566FCu)) {
            cv_image_draw_offset(st, cv_group_base(st, 0x24A) + frame - 1, x, y, 0x34, -3);
        } else {
            cv_image_draw_offset(st, cv_group_base(st, 0x24B) + frame - 1, x, y, 0x14, 0);
        }
        break;
    case 0x1F:
        cv_image_draw_offset(st, S32(st, 0x00F56DF4u) + frame - 1, x, y, 0x1D, 0xE);
        break;
    case 0x21:
        cv_image_draw_offset(st, S32(st, 0x00F56DECu) + frame - 1, x, y, 0x40, 0);
        break;
    case 0x22:
        cv_image_draw_offset(st, S32(st, 0x00F56E0Cu) + frame - 1, x, y, 0x52, 0xE);
        break;
    case 0x23:
        cv_image_draw_offset(st, S32(st, 0x00F56DECu) + frame - 1, x, y, 0x6C, 1);
        break;
    case 0x47:
    case 0xBF: {
        /* @bin 0x0043AD17 (not reached: these types take 0x00442280 before) */
        int32_t id = S32(st, 0x00F56F34u) + frame - 1;
        int32_t dx = 0x71 - S16(st, cv_image_entry(st, id) + 0x22) + x;
        int32_t dy = 3 - S16(st, cv_image_entry(st, id) + 0x24) + y;
        cv_image_draw(st, id, dx, dy);
        break;
    }
    case 0x48:
    case 0xC1: {
        /* @bin 0x0043AD62 */
        int32_t id = S32(st, 0x00F56F54u) + frame - 1;
        int32_t dx = 0x16 - S16(st, cv_image_entry(st, id) + 0x22) + x;
        int32_t dy = 0x18 - S16(st, cv_image_entry(st, id) + 0x24) + y;
        cv_image_draw(st, id, dx, dy);
        cv_image_draw(st, cv_group_base(st, 0x15F) + 0x11, x - 4, y - 0x2A);
        break;
    }
    case 0x4A: {
        /* @bin 0x0043AF63 by building +0xB4 (u16): 0x1000 | kind 0x19 / 0x4D, or 0x2000 | figure (in
         * state 1 not doing 4: type 0x4D); offsets by image - 0x00F56C0C (0..3) */
        static const int32_t offsets[8] = {0x6E, 0x14, 0x55, 0x14, 0x55, 0x14, 0x64, 0xA};
        uint32_t v = U16(st, r + 0xB4);
        if (v == 0) {
            break;
        }
        uint32_t kind = v & 0xFFF;
        int32_t group;
        if ((v & 0xF000) == 0x1000) {
            group = kind == 0x19 ? 0x277 : kind == 0x4D ? 0x275 : 0x26E;
        } else if ((v & 0xF000) == 0x2000) {
            uint32_t f = FIGURE_ADDR(kind);
            if (U8(st, f + 0x0E) != 1 || U8(st, f + 0x38) == 4) {
                break;
            }
            group = U8(st, f + 0x0A) == 0x4D ? 0x275 : 0x26E;
        } else {
            group = 0x26E;
        }
        int32_t s = img - S32(st, 0x00F56C0Cu);
        cv_image_draw_offset(st, cv_group_base(st, group) + frame - 1, x, y, offsets[s * 2], offsets[s * 2 + 1]);
        break;
    }
    case 0x4B:
    case 0xC0: {
        /* @bin 0x0043AE5B by direction (image - 0x00F56CC4) & 3; a figure +0xB4 in action 0xA: group
         * 0x279, the second offsets, frames capped at 0x14 */
        static const int32_t offsets[16] = {0x84, -7, 0x73, 0x19, 0x3C, 0x14, 0x31, -4,
                                            0x98, -13, 0x7B, 0x10, 0x41, 0xE, 0x26, -12};
        int32_t f = S16(st, r + 0xB4);
        int32_t s = (img - S32(st, 0x00F56CC4u)) & 3;
        int32_t prev = (s - 1) & 3;
        int32_t group = 0x278;
        if (f != 0 && U8(st, FIGURE_ADDR(f) + 0x38) == 0xA) {
            s += 4;
            group = 0x279;
            if (frame > 0x14) {
                frame = 0x14;
            }
        }
        cv_image_draw_offset(st, cv_group_base(st, group) + frame * 4 + prev - 4, x, y, offsets[s * 2],
                             offsets[s * 2 + 1]);
        break;
    }
    case 0x4C: {
        /* @bin 0x0043B044 by image - 0x00F56BCC (0..3); a figure +0xB4 in action 0xC: group 0x26F */
        static const int32_t offsets[16] = {0x4B, 10, 0x4E, 7, 0x3E, 0x12, 0x30, 8,
                                            0x4B, 10, 0x4E, 7, 0x3E, 0x12, 0x30, 8};
        int32_t s = img - S32(st, 0x00F56BCCu);
        int32_t f = S16(st, r + 0xB4);
        int32_t prev = (s - 1) & 3;
        int32_t group = 0x278;
        if (f != 0 && U8(st, FIGURE_ADDR(f) + 0x38) == 0xC) {
            group = 0x26F;
            s += 4;
        }
        cv_image_draw_offset(st, cv_group_base(st, group) + frame * 4 + prev - 4, x, y, offsets[s * 2],
                             offsets[s * 2 + 1]);
        break;
    }
    case 0x5A: {
        /* @bin 0x0043ADD2 by direction (image - 0x00F56CCC) & 3 */
        static const int32_t groups[4] = {0x242, 0x243, 0x244, 0x245};
        static const int32_t dxs[4] = {4, 3, 0xC, 0xF};
        static const int32_t dys[4] = {0, -2, 0, 6};
        int32_t s = (img - S32(st, 0x00F56CCCu)) & 3;
        cv_image_draw_offset(st, cv_group_base(st, groups[s]) + frame - 1, x, y,
                             dxs[s] + S32(st, CORNER_W1) * 2 - 8, dys[s] + S32(st, CORNER_H1));
        break;
    }
    case 0x6A:
    case 0x6B:
    case 0xD8:
    case 0xDD:
        cv_image_draw_offset(st, S32(st, 0x00F56F44u) + frame - 1, x, y, 0x5A, 0x11);
        cv_image_draw_offset(st, S32(st, 0x00F56F4Cu) + frame - 1, x, y, 0x28, 0x14);
        break;
    case 0xA1:
    case 0xA2:
    case 0xD9:
        cv_image_draw_offset(st, S32(st, 0x00F56F3Cu) + frame - 1, x, y, 0x36, 0xF);
        break;
    default: {
        /* @bin 0x0043B277 image + frame at the entry's offsets (+0x22 x, +0x24 - +0x16 y), from the
         * footprint corner (past 5: nothing) */
        static const int32_t base_dy[6] = {0x1E, 0x3C, 0x5A, 0x78, 0x96, 0xB4};
        if (corner > 5) {
            break;
        }
        uint32_t e = cv_image_entry(st, img);
        int32_t dy = S16(st, e + 0x24) - S16(st, e + 0x16);
        switch (corner) {
        case 1: dy -= S32(st, CORNER_H1); break;
        case 2: dy -= S32(st, CORNER_H2); break;
        case 3: dy -= S32(st, CORNER_H3); break;
        case 4: dy -= S32(st, CORNER_H4); break;
        case 5: dy = dy - S32(st, CORNER_H4) - S32(st, CORNER_H1); break;
        default: break;
        }
        dy += y + base_dy[corner];
        cv_image_draw(st, frame + img, S16(st, e + 0x22) + x, dy);
        break;
    }
    }
}

static void tops_animated_tile(game_state *st, int32_t tile, int32_t x, int32_t y)
{
    /* @bin 0x0043AB9D the buildings the overlay shows (0x00431690) whose image animates */
    int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) tile);
    int shown = 0;
    if (b != 0) {
        shown = (uint8_t) GAME_CALL(st, 0x00431690u, b) != 0;
        tile = S32(st, VIEW_TILE);
    }
    int32_t img = S32(st, IMAGE_GRID + 4u * (uint32_t) tile);
    if (U16(st, cv_image_entry(st, img) + 0x1E) == 0 || !shown) {
        return;
    }
    uint8_t draw = U8(st, TILE_DRAW + (uint32_t) tile);
    int16_t type = building_type(st, b);
    if (type == 0x48 || type == 0xC1) {
        cv_image_draw(st, S32(st, 0x00F566ACu) + 0x11, x - 4, y - 0x2A);
        tile = S32(st, VIEW_TILE);
    }
    if ((draw & 0x40) == 0) {
        return;
    }
    type = building_type(st, b);
    if (type == 0x47 || type == 0xBF) {
        GAME_CALL(st, 0x00442280u, b, tile, x, y);
    } else {
        tops_animation(st, b, tile, img, x, y);
    }
    GAME_CALL(st, 0x00436310u, b, x, y);
}

static int32_t row_x0(game_state *st, int odd)
{
    int32_t x = S32(st, VIEW_X0);
    if (odd) {
        x = x - S32(st, CORNER_W1) - 1;
    }
    return x + (-2 - S32(st, TILE_HALF_W)) * 4;
}

void city_overlay_tops_draw(game_state *st)
{
    /* @bin 0x00439F10 rows 0 .. 0x0070E460 + 0x12 (inside the table), three walks per row */
    int32_t y = S32(st, VIEW_Y0) - 9 * S32(st, TILE_H);
    int odd = 0;
    S32(st, VIEW_ROW) = S32(st, CV_CAMERA_Y) - 8;
    for (int32_t row = 0; row < S32(st, VIEW_ROWS) + 0x12; row++) {
        if (S32(st, VIEW_ROW) >= 0 && S32(st, VIEW_ROW) < 0x1C9) {
            /* @bin 0x00439F83 the figures not hidden (+0x65) and not +0x120 */
            int32_t x = row_x0(st, odd);
            S32(st, VIEW_COL) = S32(st, CV_CAMERA_X) - 4;
            for (int32_t col = 0; col <= S32(st, VIEW_COLS) + 6; col++) {
                int32_t vx = S32(st, VIEW_COL);
                if (vx >= 0 && vx < 0xE7) {
                    int32_t tile = S32(st, VIEW_TABLE + 4u * (uint32_t) (S32(st, VIEW_ROW) * 0xE7 + vx));
                    S32(st, VIEW_TILE) = tile;
                    if (tile < 0xFFF0000) {
                        int32_t f = S16(st, FIGURE_GRID + 2u * (uint32_t) tile);
                        while (f > 0) {
                            uint32_t a = FIGURE_ADDR(f);
                            if (U8(st, a + 0x65) == 0 && S16(st, a + 0x120) == 0) {
                                GAME_CALL(st, 0x00438E50u, f, x, y, 0x270F);
                            }
                            f = S16(st, a + 8);
                        }
                    }
                }
                S32(st, VIEW_COL) = S32(st, VIEW_COL) + 1;
                x += S32(st, TILE_HALF_W) + 2;
            }
            /* @bin 0x0043A060 the tops and the overlay's columns */
            x = row_x0(st, odd);
            S32(st, VIEW_COL) = S32(st, CV_CAMERA_X) - 4;
            for (int32_t col = 0; col <= S32(st, VIEW_COLS) + 6; col++) {
                int32_t vx = S32(st, VIEW_COL);
                if (vx >= 0 && vx < 0xE7) {
                    int32_t tile = S32(st, VIEW_TABLE + 4u * (uint32_t) (S32(st, VIEW_ROW) * 0xE7 + vx));
                    S32(st, VIEW_TILE) = tile;
                    if (tile < 0xFFF0000) {
                        tops_tile(st, tile, x, y);
                    }
                }
                x += S32(st, TILE_HALF_W) + 2;
                S32(st, VIEW_COL) = S32(st, VIEW_COL) + 1;
            }
            /* @bin 0x0043AB13 the animated parts */
            x = row_x0(st, odd);
            S32(st, VIEW_COL) = S32(st, CV_CAMERA_X) - 4;
            for (int32_t col = 0; col <= S32(st, VIEW_COLS) + 6; col++) {
                int32_t vx = S32(st, VIEW_COL);
                if (vx >= 0 && vx < 0xE7) {
                    int32_t tile = S32(st, VIEW_TABLE + 4u * (uint32_t) (S32(st, VIEW_ROW) * 0xE7 + vx));
                    S32(st, VIEW_TILE) = tile;
                    if (tile < 0xFFF0000) {
                        tops_animated_tile(st, tile, x, y);
                    }
                }
                x += S32(st, TILE_HALF_W) + 2;
                S32(st, VIEW_COL) = S32(st, VIEW_COL) + 1;
            }
        }
        odd = !odd;
        y += S32(st, TILE_H);
        S32(st, VIEW_ROW) = S32(st, VIEW_ROW) + 1;
    }
}

static int32_t run_overlay_ground(game_state *st, const int32_t *a)
{
    (void) a;
    city_overlay_ground_draw(st);
    return 0;
}

static int32_t run_overlay_tops(game_state *st, const int32_t *a)
{
    (void) a;
    city_overlay_tops_draw(st);
    return 0;
}

const ported_function cityview_overlay_ports[] = {
    {0x004316F0u, "city_overlay_ground_draw", 0, 0, 0, run_overlay_ground, 0},
    {0x00439F10u, "city_overlay_tops_draw", 0, 0, 0, run_overlay_tops, 0},
};
const int cityview_overlay_ports_count = sizeof(cityview_overlay_ports) / sizeof(cityview_overlay_ports[0]);
