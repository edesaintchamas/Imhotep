/* Imhotep - the city view's upper pass 0x00436B60 (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Row by row over the view (4 extra rows, 0x20 / 0x50 when a 0xB7 / 0xE6 building is seen), four
 * walks per row: building tops and special buildings; the figures on tiles that are not plain;
 * animated building parts (0x0040F2C0: the sprite grid, state), storage contents (0x00442280:
 * city goods flags, state) and the per-tile overlays 0x00436310; the figures on plain tiles. */
#include "city.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define DRAW_COUNTER 0x00775688u /* u16, +1 per upper pass */

static int32_t row_x0(game_state *st, int flag)
{
    /* @bin 0x00436BC8 the first column's x: rows alternate, the odd ones half a tile left */
    int32_t x = S32(st, VIEW_X0);
    if (flag) {
        x = x - S32(st, 0x0070E45Cu) - 1;
    }
    return x + (-2 - S32(st, TILE_HALF_W)) * 4;
}

/* the tile at the walk's column, or -1 when the column is outside the table */
static int32_t walk_tile(game_state *st)
{
    int32_t vx = S32(st, VIEW_COL);
    if (vx < 0 || vx >= 0xE7) {
        return -1;
    }
    return S32(st, VIEW_TABLE + 4u * (uint32_t) (S32(st, VIEW_ROW) * 0xE7 + vx));
}

static int neighbours_cover(game_state *st, int flag)
{
    /* @bin 0x00436D3D the tiles up-left and up-right of the walked one (parity 0: -0xE7, -0xE6;
     * else -0xE8, -0xE7 in the view table) */
    int32_t idx = S32(st, VIEW_ROW) * 0xE7 + S32(st, VIEW_COL);
    int32_t t1, t2;
    if (flag == 0) {
        t1 = S32(st, 0x0070E0F8u + 4u * (uint32_t) idx);
        t2 = S32(st, 0x0070E0FCu + 4u * (uint32_t) idx);
    } else {
        t1 = S32(st, 0x0070E0F4u + 4u * (uint32_t) idx);
        t2 = S32(st, 0x0070E0F8u + 4u * (uint32_t) idx);
    }
    return (t1 < 0xFFF0000 && cv_tile_covers(st, t1)) || (t2 < 0xFFF0000 && cv_tile_covers(st, t2));
}

/* @bin 0x00436E98 a footprint's top drawn from its corner tile (size 1..5), as the ground pass
 * places it; 0: not a corner */
static int corner_pos(game_state *st, uint32_t corner, int32_t x, int32_t y, int32_t *px, int32_t *py)
{
    switch (corner) {
    case 1:
        *px = S32(st, 0x0070E45Cu) + x + 1;
        *py = y - S32(st, TILE_H);
        return 1;
    case 2:
        *px = S32(st, TILE_HALF_W) + x + 2;
        *py = y - S32(st, 0x0070E448u);
        return 1;
    case 3:
        *px = S32(st, 0x0070E454u) + x + 3;
        *py = y - S32(st, 0x0070E444u);
        return 1;
    case 4:
        *px = S32(st, 0x0070E450u) + x + 4;
        *py = y - S32(st, 0x0070E440u);
        return 1;
    case 5:
        *px = S32(st, 0x0070E450u) + S32(st, 0x0070E45Cu) + x + 5;
        *py = y - S32(st, 0x0070E440u) - S32(st, TILE_H);
        return 1;
    default:
        return 0;
    }
}

static void specials(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x004372AB the buildings with extra parts by type (+0x10), part (+0x12) and orientation */
    uint32_t r = BUILDING_ADDR(b);
    int32_t orient = S32(st, ORIENTATION);
    if (S16(st, r + 0x10) == 0xD2) {
        int16_t part;
        int draw = 0;
        if (orient == 4 || orient == 2) {
            part = S16(st, r + 0x12);
            draw = part == 0 || part == 3;
        } else if (orient == 0 || orient == 6) {
            part = S16(st, r + 0x12);
            draw = part == 2 || part == 5;
        }
        if (draw) {
            GAME_CALL(st, 0x0041FFF0u, x, y);
        }
    }
    if (S16(st, r + 0x10) == 0xCF || S16(st, r + 0x10) == 0xD0) {
        GAME_CALL(st, 0x004E5E90u, b, x, y);
    }
    if (S16(st, r + 0x10) == 0xD5 && S16(st, r + 0x12) == 0) {
        GAME_CALL(st, 0x00542E10u, b, x, y);
        GAME_CALL(st, 0x00420050u, x, y);
    }
    if (S16(st, r + 0x10) == 0xE1) {
        int draw = 0;
        if (orient == 0) {
            draw = S16(st, r + 0x12) == 6;
        } else if (orient == 2) {
            draw = S16(st, r + 0x12) == 5;
        } else if (orient == 4) {
            draw = S16(st, r + 0x12) == 7;
        } else if (orient == 6) {
            draw = S16(st, r + 0x12) == 8;
        }
        if (draw) {
            GAME_CALL(st, 0x004206F0u, x, y);
        }
    }
    if (S16(st, r + 0x10) == 0xE3 && S16(st, r + 0x12) == 0) {
        GAME_CALL(st, 0x00420D60u, x, y);
    }
    int16_t t = S16(st, r + 0x10);
    if ((t == 0xE5 || t == 0xEA || t == 0xEC || t == 0xEB) && S32(st, r + 0x64) != 0 &&
        U8(st, FIGURE_ADDR(S32(st, r + 0x5C)) + 0x11) == 8) {
        GAME_CALL(st, 0x00421920u, x, y, b);
    }
    if (S16(st, r + 0x10) == 0xE4 && S16(st, r + 0x12) == 0) {
        GAME_CALL(st, 0x00421060u, x, y);
    }
}

static void walk1_tile(game_state *st, int32_t tile, int32_t x, int32_t y, int flag, int32_t *extra)
{
    /* @bin 0x00436C62 tops: tiles of the normal view (in overlay 0x33 / 0x006DE515 not the ones with
     * terrain 0x20000002), drawn ones (0x40) */
    if ((S32(st, OVERLAY) == 0x33 || U8(st, NO_GROUND) != 0) &&
        (U32(st, TERRAIN + 4u * (uint32_t) tile) & 0x20000002u) == 0x20000002u) {
        return;
    }
    if ((U8(st, TILE_DRAW + (uint32_t) tile) & 0x40) == 0) {
        return;
    }
    int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) tile);
    uint32_t r = BUILDING_ADDR(b);
    if (b != 0 && U8(st, r + 0xBF) != 0) {
        U16(st, TINT) = 0xF863;
    }
    uint32_t corner = U8(st, EDGE_GRID + (uint32_t) tile) & 0xF;
    int32_t img = S32(st, IMAGE_GRID + 4u * (uint32_t) tile);
    S32(st, DRAW_IMAGE) = img;
    int16_t type = S16(st, r + 0x10);
    int32_t px, py;
    if (type == 0xB7) {
        *extra = 0x20;
        GAME_CALL(st, 0x004EF550u, img, x, y, b);
    } else if (type == 0xE6) {
        /* @bin 0x00436D11 */
        *extra = 0x50;
        if (corner == 0) {
            if ((U32(st, TERRAIN + 4u * (uint32_t) tile) & TERRAIN_PLAIN_MASK) != 0 || !neighbours_cover(st, flag)) {
                cv_image_draw_top(st, img, x, y);
            }
        } else if (corner_pos(st, corner, x, y, &px, &py)) {
            cv_image_draw_top(st, img, px, py);
        }
        /* @bin 0x00436F43 by orientation and part +0x12 */
        int32_t orient = S32(st, ORIENTATION);
        if (((orient == 0 || orient == 4) && S16(st, r + 0x12) == 0) ||
            ((orient == 2 || orient == 6) && S16(st, r + 0x12) == 1)) {
            GAME_CALLT(st, 0x006DE650u, 0x00401F70u, b, x, y);
            GAME_CALL(st, 0x00420A30u, x, y);
        }
    } else {
        int32_t skip = S32(st, 0x00F572ECu);
        if (img >= skip + 0x30 && img < S32(st, 0x00F572ECu) + 0x48) {
            /* @bin 0x00436FBB images 0x00F572EC + 0x30 .. 0x47 (left out by the ground pass): 150 up */
            cv_image_draw(st, img, x, y - 0x96);
            cv_image_draw_top(st, S32(st, DRAW_IMAGE), x, y - 0x96);
        } else {
            if (corner == 0) {
                if ((U32(st, TERRAIN + 4u * (uint32_t) tile) & TERRAIN_PLAIN_MASK) != 0 || !neighbours_cover(st, flag)) {
                    cv_image_draw_top(st, img, x, y);
                }
            } else if (corner == 2) {
                cv_image_draw_top(st, img, S32(st, TILE_HALF_W) + x + 2, y - S32(st, 0x0070E448u));
                /* @bin 0x004371A4 */
                if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488030u, b) != 0) {
                    GAME_CALLT(st, 0x00BCA350u, 0x00488C20u, b, S32(st, TILE_HALF_W) + x + 2, y - S32(st, 0x0070E448u), 1);
                }
            } else if (corner_pos(st, corner, x, y, &px, &py)) {
                cv_image_draw_top(st, img, px, py);
            }
            /* @bin 0x0043724B the building's +0x05 / +0x87 marker */
            if (U8(st, r + 0x05) != 0) {
                if (U8(st, r + 0x87) == 1) {
                    cv_image_draw(st, S32(st, 0x00F56724u), x + 0x12, y - 0x20);
                }
                if (U8(st, r + 0x05) != 0 && U8(st, r + 0x87) == 2) {
                    cv_image_draw(st, S32(st, 0x00F572E4u), x + 0x12, y - 0x20);
                }
            }
        }
    }
    specials(st, b, x, y);
    U16(st, TINT) = 0;
}

static void walk_figures(game_state *st, int32_t x0, int32_t y, int plain, int32_t param)
{
    /* @bin 0x0043749C / 0x00437E40 the figures (+0x65 = 0, +0x120 = 0) on the tiles that are
     * not plain (terrain & 0xEEFED77F) / plain, in the tile's list order (+8 next) */
    int32_t x = x0;
    S32(st, VIEW_COL) = S32(st, CV_CAMERA_X) - 4;
    for (int32_t col = 0; col <= S32(st, VIEW_COLS) + 6; col++) {
        int32_t vx = S32(st, VIEW_COL);
        if (vx >= 0 && vx < 0xE7) {
            int32_t tile = S32(st, VIEW_TABLE + 4u * (uint32_t) (S32(st, VIEW_ROW) * 0xE7 + vx));
            S32(st, VIEW_TILE) = tile;
            if (tile < 0xFFF0000 && ((U32(st, TERRAIN + 4u * (uint32_t) tile) & TERRAIN_PLAIN_MASK) == 0) == plain) {
                for (int32_t f = S16(st, FIGURE_GRID + 2u * (uint32_t) tile); f > 0;
                     f = S16(st, FIGURE_ADDR(f) + 8)) {
                    if (U8(st, FIGURE_ADDR(f) + 0x65) == 0 && S16(st, FIGURE_ADDR(f) + 0x120) == 0) {
                        GAME_CALL(st, 0x00438E50u, f, x, y, param);
                    }
                }
            }
        }
        x += S32(st, TILE_HALF_W) + 2;
        S32(st, VIEW_COL) = S32(st, VIEW_COL) + 1;
    }
}

static int32_t entry_frames(game_state *st, int32_t img)
{
    /* the image entry's +0x1E: its animation frames (ids past the count: entry 0) */
    int32_t k = img < S32(st, IMAGE_COUNT) ? img : 0;
    return S16(st, IMAGE_ENTRIES + ((uint32_t) k << 6) + 0x1E);
}

static void animated(game_state *st, int32_t tile, int32_t img, int32_t x, int32_t y)
{
    /* @bin 0x0043768D a building tile whose image animates */
    int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) tile);
    uint32_t r = BUILDING_ADDR(b);
    if (b != 0 && U8(st, r + 0xBF) != 0) {
        U16(st, TINT) = 0xF863;
    }
    if (S16(st, r + 0x10) == 0x47 || S16(st, r + 0x10) == 0xBF) {
        GAME_CALL(st, 0x00442280u, b, tile, x, y);
        tile = S32(st, VIEW_TILE);
    }
    int16_t type = S16(st, r + 0x10);
    if (type == 0x4B) {
        img = S32(st, IMAGE_GRID + 4u * (uint32_t) tile);
    }
    if (type == 0x48 || type == 0xC1) {
        cv_image_draw(st, S32(st, 0x00F566ACu) + 0x11, x - 4, y - 0x2A);
        tile = S32(st, VIEW_TILE);
    }
    /* @bin 0x0043773E the animation frame (0x0040F2C0 advances the sprite grid), capped at +0x1E */
    int32_t frame = GAME_CALL(st, 0x0040F2C0u, img, tile);
    if (frame <= 0) {
        U16(st, TINT) = 0;
        return;
    }
    if (frame > entry_frames(st, img)) {
        frame = entry_frames(st, img);
    }
    uint32_t corner = U8(st, EDGE_GRID + (uint32_t) S32(st, VIEW_TILE)) & 0xF;
    if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488050u, b) != 0) {
        GAME_CALLT(st, 0x00BCA350u, 0x00489500u, b, frame, x, y);
        U16(st, TINT) = 0;
        return;
    }
    switch (S16(st, r + 0x10)) {
    case 0x1E:
        GAME_CALL(st, 0x00438590u, img, frame, x, y);
        break;
    case 0x1F:
        GAME_CALL(st, 0x004386B0u, img, frame, x, y);
        break;
    case 0x21:
        GAME_CALL(st, 0x00438710u, img, frame, x, y);
        break;
    case 0x22:
        GAME_CALL(st, 0x00438650u, img, frame, x, y);
        break;
    case 0x23:
        GAME_CALL(st, 0x00438770u, img, frame, x, y);
        break;
    case 0x47:
    case 0xBF:
        cv_image_draw_offset(st, cv_group_base(st, 0x270) + frame - 1, x, y, 0x71, 3);
        break;
    case 0x48:
    case 0xC1:
        cv_image_draw_offset(st, cv_group_base(st, 0x274) + frame - 1, x, y, 0x16, 0x18);
        cv_image_draw(st, cv_group_base(st, 0x15F) + 0x11, x - 4, y - 0x2A);
        break;
    case 0x4A:
        GAME_CALL(st, 0x00438210u, img, frame, x, y, (int32_t) U16(st, r + 0xB4));
        break;
    case 0x4B:
    case 0xC0: {
        /* @bin 0x0043792F the direction (image - group 0x222) & 3; a figure +0xB4 in action 0xA
         * (+0x38): group 0x279 and the second half of the offsets, frames capped at 0x14 */
        static const int32_t offsets[16] = {0x84, -7, 0x73, 0x19, 0x3C, 0x14, 0x31, -4,
                                            0x98, -13, 0x7B, 0x10, 0x41, 0xE, 0x26, -12};
        int32_t f = S16(st, r + 0xB4);
        int32_t dir = (img - cv_group_base(st, 0x222)) & 3;
        int32_t prev = (dir - 1) & 3;
        int32_t group = 0x278;
        if (f != 0 && U8(st, FIGURE_ADDR(f) + 0x38) == 0xA) {
            dir += 4;
            group = 0x279;
            if (frame > 0x14) {
                frame = 0x14;
            }
        }
        cv_image_draw_offset(st, cv_group_base(st, group) + prev + frame * 4 - 4, x, y, offsets[dir * 2],
                             offsets[dir * 2 + 1]);
        break;
    }
    case 0x4C:
        GAME_CALL(st, 0x00438350u, img, frame, x, y, (int32_t) S16(st, r + 0xB4));
        break;
    case 0x5A: {
        /* @bin 0x00437866 by the direction (image - group 0x223) & 3 */
        static const int32_t groups[4] = {0x242, 0x243, 0x244, 0x245};
        static const int32_t dxs[4] = {4, 3, 0xC, 0xF};
        static const int32_t dys[4] = {0, -2, 0, 6};
        int32_t dir = (img - cv_group_base(st, 0x223)) & 3;
        cv_image_draw_offset(st, cv_group_base(st, groups[dir]) + frame - 1, x, y,
                             dxs[dir] + S32(st, 0x0070E45Cu) * 2 - 8, dys[dir] + S32(st, TILE_H));
        break;
    }
    case 0x6A:
    case 0x6B:
    case 0xD8:
    case 0xDD:
        GAME_CALL(st, 0x004384D0u, img, frame, x, y);
        break;
    case 0xA1:
    case 0xA2:
    case 0xD9:
        GAME_CALL(st, 0x00438470u, img, frame, x, y);
        break;
    default: {
        /* @bin 0x00437B4D the frame image (image + frame) at the entry's offsets (+0x22 x, +0x24 -
         * +0x16 y), from the footprint corner: 0x1E, then 0x1E more per size step */
        static const int32_t base_dy[6] = {0x1E, 0x3C, 0x5A, 0x78, 0x96, 0xB4};
        if (corner > 5) {
            break;
        }
        int32_t dy = S16(st, cv_image_entry(st, img) + 0x24);
        dy -= S16(st, cv_image_entry(st, img) + 0x16);
        switch (corner) {
        case 1:
            dy -= S32(st, TILE_H);
            break;
        case 2:
            dy -= S32(st, 0x0070E448u);
            break;
        case 3:
            dy -= S32(st, 0x0070E444u);
            break;
        case 4:
            dy -= S32(st, 0x0070E440u);
            break;
        case 5:
            dy -= S32(st, 0x0070E440u);
            dy -= S32(st, TILE_H);
            break;
        default:
            break;
        }
        dy += y + base_dy[corner];
        int32_t dx = S16(st, cv_image_entry(st, img) + 0x22) + x;
        cv_image_draw(st, frame + img, dx, dy);
        break;
    }
    }
    U16(st, TINT) = 0;
}

static void still(game_state *st, int32_t tile, int32_t x, int32_t y)
{
    /* @bin 0x00437CF3 a tile whose image does not animate: the 0x39 parts, 0xCA, storage */
    int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) tile);
    uint32_t r = BUILDING_ADDR(b);
    uint8_t draw = U8(st, TILE_DRAW + (uint32_t) tile);
    int16_t type = S16(st, r + 0x10);
    if (type == 0x39) {
        if (draw & 0x40) {
            int16_t part = S16(st, r + 0x12);
            if (part == 0xD) {
                cv_image_draw(st, S32(st, 0x00F5662Cu) + 3, x + 0x5D, y - 0x14);
            } else if (part == 0xC) {
                cv_image_draw(st, S32(st, 0x00F5662Cu) + 4, x + 0x5D, y - 0x14);
            } else if (part == 0xB) {
                cv_image_draw(st, S32(st, 0x00F5662Cu) + 2, x + 0x5D, y - 0x14);
            }
        }
    } else if (type == 0x3A || type == 0xAA || type == 0xAB || type == 0xC8 || type == 0xC9) {
        /* nothing */
    } else if (type == 0xCA) {
        if (draw & 0x40) {
            GAME_CALL(st, 0x004C0160u, b, x, y);
        }
    } else if (type == 0x47 || type == 0xBF) {
        GAME_CALL(st, 0x00442280u, b, tile, x, y);
    }
}

static void walk3(game_state *st, int32_t x0, int32_t y)
{
    /* @bin 0x0043758D sprites, animations, storage, then the tile's overlays 0x00436310 */
    int32_t x = x0;
    S32(st, VIEW_COL) = S32(st, CV_CAMERA_X) - 4;
    for (int32_t col = 0; col <= S32(st, VIEW_COLS) + 6; col++) {
        int32_t tile = walk_tile(st);
        if (S32(st, VIEW_COL) >= 0 && S32(st, VIEW_COL) < 0xE7) {
            S32(st, VIEW_TILE) = tile;
            if (tile < 0xFFF0000) {
                uint32_t terrain = U32(st, TERRAIN + 4u * (uint32_t) tile);
                uint8_t sprite = U8(st, SPRITE_GRID + (uint32_t) tile);
                if ((terrain & 0x44) == 0x44 && sprite != 0) {
                    /* @bin 0x0043763C terrain 0x44 with a sprite: image 0x00F56D34 + sprite - 1 */
                    cv_image_draw(st, sprite + S32(st, 0x00F56D34u) - 1, x, y - S32(st, 0x0070E448u));
                } else {
                    int32_t img = S32(st, IMAGE_GRID + 4u * (uint32_t) tile);
                    int over = 1;
                    if (entry_frames(st, img) == 0) {
                        still(st, tile, x, y);
                    } else if (U8(st, TILE_DRAW + (uint32_t) tile) & 0x40) {
                        animated(st, tile, img, x, y);
                    } else {
                        over = 0;
                    }
                    if (over) {
                        tile = S32(st, VIEW_TILE);
                        if (U8(st, TILE_DRAW + (uint32_t) tile) & 0x40) {
                            GAME_CALL(st, 0x00436310u, (int32_t) S16(st, BUILDING_GRID + 2u * (uint32_t) tile), x, y);
                        }
                    }
                }
            }
        }
        S32(st, VIEW_COL) = S32(st, VIEW_COL) + 1;
        x += S32(st, TILE_HALF_W) + 2;
    }
}

void city_upper_draw(game_state *st, int32_t param)
{
    /* @bin 0x00436B60 (param: passed to the figure draw 0x00438E50) */
    U16(st, DRAW_COUNTER) = (uint16_t) (U16(st, DRAW_COUNTER) + 1);
    int32_t y = S32(st, VIEW_Y0) - 9 * S32(st, TILE_H);
    S32(st, VIEW_ROW) = S32(st, CV_CAMERA_Y) - 8;
    int32_t extra = 4;
    int flag = 0;
    if (S32(st, VIEW_ROWS) + 0x12 <= 0) {
        return;
    }
    int32_t row = 0;
    do {
        int32_t vy = S32(st, VIEW_ROW);
        if (vy >= 0 && vy < 0x1C9) {
            /* walk 1: tops */
            int32_t x = row_x0(st, flag);
            S32(st, VIEW_COL) = S32(st, CV_CAMERA_X) - 4;
            for (int32_t col = 0; col <= S32(st, VIEW_COLS) + 6; col++) {
                int32_t tile = walk_tile(st);
                if (S32(st, VIEW_COL) >= 0 && S32(st, VIEW_COL) < 0xE7) {
                    S32(st, VIEW_TILE) = tile;
                    if (tile < 0xFFF0000) {
                        walk1_tile(st, tile, x, y, flag, &extra);
                    }
                }
                S32(st, VIEW_COL) = S32(st, VIEW_COL) + 1;
                x += S32(st, TILE_HALF_W) + 2;
            }
            walk_figures(st, row_x0(st, flag), y, 0, param);
            walk3(st, row_x0(st, flag), y);
            walk_figures(st, row_x0(st, flag), y, 1, param);
        }
        flag = !flag;
        y += S32(st, TILE_H);
        row++;
        S32(st, VIEW_ROW) = S32(st, VIEW_ROW) + 1;
    } while (row < S32(st, VIEW_ROWS) + 0xE + extra);
}

static int32_t run_upper(game_state *st, const int32_t *a)
{
    city_upper_draw(st, a[0]);
    return 0;
}

const ported_function cityview_upper_ports[] = {
    {0x00436B60u, "city_upper_draw", 1, 0, 0, run_upper, 0},
};
const int cityview_upper_ports_count = sizeof(cityview_upper_ports) / sizeof(cityview_upper_ports[0]);
