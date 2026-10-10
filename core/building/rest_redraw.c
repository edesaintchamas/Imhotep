/* Imhotep - building footprints drawn again by type (package rest): the callees of
 * buildings_redraw 0x0046D840 (core/building/day2_redraw.c) and the mastaba-like monuments'
 * part images (0x004DED50, 0x004DF360, 0x004DFD90).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/rest_redraw.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static void tiles(game_state *st, int32_t b, int32_t x, int32_t y, int32_t sx, int32_t sy, int32_t img, int32_t flags)
{
    GAME_CALL(st, FN_MAP_BUILDING_TILES, b, x, y, sx, sy, img, flags);
}

int32_t god_index_4c53d0(game_state *st, int32_t v)
{
    /* @bin 0x004C53D0 the index of v among the five words at 0x007848E2, or -1 */
    for (int32_t i = 0; i < 5; i++) {
        if (S16(st, G_GOD_MODE + 2u * (uint32_t) i) == v) {
            return i;
        }
    }
    return -1;
}

void building_redraw_4bfb30(game_state *st, int32_t b)
{
    /* @bin 0x004BFB30 the four parts of table 0x005DF768 (part entries 0x005DF80C, 16 bytes:
     * +4 flag 0x40, +8 size, +0xC, +0x10 offsets by orientation), image group 0x1AE replaced by
     * the one of table 0x005DF778 for 0x004C53D0(2), else (1), else 0 */
    uint32_t r = BUILDING_ADDR(b);
    int32_t x = S16(st, r + BLD_X), y = S16(st, r + BLD_Y);
    int32_t v = god_index_4c53d0(st, 2);
    if (v == -1) {
        v = god_index_4c53d0(st, 1);
        if (v == -1) {
            v = 0;
        }
    }
    int32_t sel = S32(st, G_GATE_GOD_GROUPS + 4u * (uint32_t) v);
    int32_t k = (int32_t) U8(st, r + BLD_ORIENTATION) + S32(st, G_VIEW_ORIENTATION);
    if (k >= 8) {
        k -= 8;
    }
    /* @bin 0x004BFCAC jump table: views 0, 3, 4, 7 use the group's second image */
    int32_t second = (uint32_t) k <= 7 && (k == 0 || k == 3 || k == 4 || k == 7);
    for (uint32_t p = G_GATE_GROUPS; p < G_GATE_GROUPS + 4 * 4; p += 4) {
        int32_t part = S32(st, p + 0x24u + 16u * (uint32_t) k);
        uint32_t e = G_GATE_PARTS - 4 + 16u * (uint32_t) part;
        int32_t px, py;
        switch (U8(st, r + BLD_ORIENTATION)) {
        case 0:
            px = S32(st, e + 0x10) + x;
            py = S32(st, e + 0xC) + y;
            break;
        case 1:
            px = S32(st, e + 0xC) + x;
            py = S32(st, e + 0x10) + y;
            break;
        default:
            px = x;
            py = y;
            break;
        }
        int32_t g = S32(st, p);
        if (g == 0x1AE) {
            g = sel;
        }
        int32_t img = S32(st, G_IMAGE_GROUPS + 8u * (uint32_t) g) + second;
        int32_t size = S32(st, e + 8);
        tiles(st, b, px, py, size, size, img, U8(st, e + 4) != 0 ? 0x8048 : 0x8008);
    }
    walls_rect_47afd0(st, x, y, 5);
}

void walls_rect_47afd0(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x0047AFD0 the wall tiles (terrain 0x4000 without 8) of the size x size area from
     * (x - 1, y - 1), clipped to the map: their images by the wall patterns (0x0047DB00,
     * 0x00484F00, 0x00484BE0 on table 0x005CB9D8; next to buildings 0x0047E000, 0x0047E750, table
     * 0x005CBCD8), group 0x00F5686C + 0x00BC9740 + 0x00BC9748; then 0x004746D0 on each (as
     * 0x0047B1A0 on the whole map). Cursor 0x00BC9768, walk 0x00BC9764 / 0x00BC9760 */
    int32_t x0 = x - 1, y0 = y - 1, w = size, h = size;
    if (x0 < 0) {
        w = x0 + size;
        x0 = 0;
    } else if (x0 + size > S32(st, G_MAP_WIDTH)) {
        w = S32(st, G_MAP_WIDTH) - x0;
    }
    if (y0 < 0) {
        h = y0 + size;
        y0 = 0;
    } else if (y0 + size > S32(st, G_MAP_HEIGHT)) {
        h = S32(st, G_MAP_HEIGHT) - y0;
    }
    int32_t yend = h + y0;
    S32(st, G_WALK_Y) = y0;
    S32(st, G_TILE_CURSOR) = S32(st, G_MAP_START) + y0 * 228 + x0;
    if (y0 >= yend) {
        return;
    }
    do {
        int32_t xend = w + x0;
        S32(st, G_WALK_X) = x0;
        if (x0 < xend) {
            do {
                uint32_t t = (uint32_t) S32(st, G_TILE_CURSOR);
                uint32_t v = U32(st, G_GRID_TERRAIN + 4u * t);
                if ((v & 0x4000) != 0 && (v & 8) == 0) {
                    GAME_CALL(st, FN_NEIGHBOURS_WITH_FLAGS, 0x4000);
                    GAME_CALL0(st, FN_SCRATCH_INVERT);
                    GAME_CALL(st, FN_LAND_PATTERN_MATCH, (int32_t) G_WALL_PATTERNS, 0x30, -1, 0);
                    t = (uint32_t) S32(st, G_TILE_CURSOR);
                    U32(st, G_GRID_IMAGE + 4u * t) =
                        (uint32_t) (S32(st, G_IMAGE_GROUPS + 8 * 407) + S32(st, G_MATCH_VARIANT) + S32(st, G_MATCH_OFFSET));
                    U8(st, G_GRID_EDGE + t) |= 0x40;
                    U8(st, G_GRID_BITFIELDS + t) &= 0xF0;
                    if (GAME_CALL0(st, FN_SCRATCH_WALLS_BUILDINGS) != 0) {
                        GAME_CALL0(st, FN_SCRATCH_ADD_WALLS);
                        if (GAME_CALL(st, FN_LAND_PATTERN_MATCH, (int32_t) G_WALL_PATTERNS_BUILDINGS, 0xF, -1, 0) != 0) {
                            U32(st, G_GRID_IMAGE + 4u * (uint32_t) S32(st, G_TILE_CURSOR)) =
                                (uint32_t) (S32(st, G_MATCH_VARIANT) + S32(st, G_IMAGE_GROUPS + 8 * 407) + S32(st, G_MATCH_OFFSET));
                        } else {
                            GAME_CALL(st, FN_NOTHING, 1);
                        }
                    } else {
                        GAME_CALL(st, FN_NOTHING, 0);
                    }
                    GAME_CALL(st, FN_TERRAIN_LAND_AROUND, S32(st, G_WALK_X), S32(st, G_WALK_Y), 1);
                }
                S32(st, G_WALK_X) += 1;
                S32(st, G_TILE_CURSOR) += 1;
            } while (S32(st, G_WALK_X) < xend);
        }
        S32(st, G_WALK_Y) += 1;
        S32(st, G_TILE_CURSOR) += 0xE4 - w;
    } while (S32(st, G_WALK_Y) < yend);
}

void building_redraw_47d1f0(game_state *st, int32_t b, int32_t x, int32_t y, int32_t img)
{
    /* @bin 0x0047D1F0 a 3 x 3 footprint from (x, y) set by hand (offsets 0x005CC8C0, edge bytes
     * of the table), the corner of the view (0x00E94E04) marked 0x40 */
    int32_t o = S32(st, G_CITY_ORIENTATION);
    int32_t cx, cy;
    if (o == 0) {
        cx = 0, cy = 2;
    } else if (o == 2) {
        cx = 0, cy = 0;
    } else if (o == 4) {
        cx = 2, cy = 0;
    } else {
        cx = 2, cy = 2;
    }
    if (x < 0 || y < 0 || x + 2 >= S32(st, G_MAP_WIDTH) || y + 2 >= S32(st, G_MAP_HEIGHT)) {
        S32(st, G_OUT_OF_MAP) = 1;
        return;
    }
    int32_t base = S32(st, G_MAP_START) + y * 228 + x;
    S32(st, G_TILE_CURSOR) = base;
    for (int32_t i = 0; i < 3; i++) {
        uint32_t e = G_FOOTPRINT_OFFSETS + 4 + 0x30u * (uint32_t) i;
        for (int32_t j = 0; j < 3; j++, e += 8) {
            uint32_t t = (uint32_t) (S32(st, e - 4) + base);
            uint32_t ter = U32(st, G_GRID_TERRAIN + 4u * t) | 8;
            U32(st, G_GRID_TERRAIN + 4u * t) = ter;
            if ((ter & 4) == 0) {
                U32(st, G_GRID_TERRAIN + 4u * t) = (ter & 0x03052E80u) | 8;
            }
            S16(st, G_GRID_BUILDING + 2u * t) = (int16_t) b;
            U8(st, G_GRID_BITFIELDS + t) = (uint8_t) ((U8(st, G_GRID_BITFIELDS + t) & 0xEF) | 2);
            U32(st, G_GRID_IMAGE + 4u * t) = (uint32_t) img;
            uint8_t edge = U8(st, e);
            U8(st, G_GRID_EDGE + t) = edge;
            if (j == cx && i == cy) {
                U8(st, G_GRID_EDGE + t) = edge | 0x40;
            }
        }
    }
}

static int gate_images_4405c0(game_state *st, int32_t v, int32_t *first, int32_t *second)
{
    /* @bin 0x004405C0 the two images of a gate by the view (0x00BCA2E0) and its direction v;
     * returns 0 when it writes nothing (the original leaves its caller's stack as it was) */
    int32_t g = S32(st, G_IMAGE_GROUPS + 8 * 361);
    int32_t a, c; /* *second, *first */
    switch (S32(st, G_VIEW_ORIENTATION)) {
    case 0:
        if (v == 0) {
            a = g, c = g + 1;
        } else if (v == 1) {
            a = g + 3, c = g + 2;
        } else {
            return 0;
        }
        break;
    case 2:
        if (v == 0) {
            a = g + 3, c = g + 2;
        } else if (v == 1) {
            a = g + 1, c = g;
        } else {
            return 0;
        }
        break;
    case 4:
        if (v == 0) {
            a = g + 1, c = g;
        } else if (v == 1) {
            a = g + 2, c = g + 3;
        } else {
            return 0;
        }
        break;
    case 6:
        if (v == 0) {
            a = g + 2, c = g + 3;
        } else if (v == 1) {
            a = g, c = g + 1;
        } else {
            return 0;
        }
        break;
    default:
        return 0;
    }
    *second = a;
    *first = c;
    return 1;
}

void building_redraw_440540(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x00440540 a two-tile gate: b at (x, y) and its other part +0x3A next to it (x + 1
     * or y + 1 by +0xD4), images of 0x004405C0 */
    uint32_t r = BUILDING_ADDR(b);
    int32_t v = S32(st, r + BLD_GATE_DIRECTION);
    int32_t other = S16(st, r + BLD_NEXT_PART);
    int32_t first, second;
    if (!gate_images_4405c0(st, v, &first, &second)) {
        state_unported(st, FN_GATE_IMAGES_4405C0); /* the original's images are its caller's stack */
        return;
    }
    int32_t ox, oy;
    if (v == 0) {
        ox = x, oy = y + 1;
    } else {
        ox = x + 1, oy = y;
    }
    tiles(st, b, x, y, 1, 1, first, 8);
    tiles(st, other, ox, oy, 1, 1, second, 8);
}

void building_area_redraw_440060(game_state *st, int32_t off, int32_t type, int32_t b)
{
    /* @bin 0x00440060 the area of a temple complex (n x n from tile off: 2 for 0x1F, 3 for 0x1E,
     * 4 for 0x21, 5 for 0xD1, else 1): tiles given to b, their images by the view from tables
     * 0x005D2078.. (grass 0x00F565F4 where the terrain has no 0x40) */
    int32_t n = type == 0x1F ? 2 : type == 0x1E ? 3 : type == 0x21 ? 4 : type == 0xD1 ? 5 : 1;
    int32_t o = S32(st, G_VIEW_ORIENTATION);
    for (int32_t row = 0; row < n; row++) {
        for (int32_t col = 0; col < n; col++) {
            uint32_t t = (uint32_t) (off + row * 228 + col);
            if (type != 0xD1) {
                int32_t cur = S32(st, G_GRID_IMAGE + 4u * t);
                int32_t g = S32(st, G_IMAGE_GROUPS + 8 * 361);
                if (cur == S32(st, G_IMAGE_GROUPS + 8 * 383) || cur == g || cur == g + 1 || cur == g + 2 || cur == g + 3 ||
                    cur == S32(st, G_IMAGE_GROUPS + 8 * 317)) {
                    continue;
                }
            }
            S16(st, G_GRID_BUILDING + 2u * t) = (int16_t) b;
            uint32_t ter = (U32(st, G_GRID_TERRAIN + 4u * t) & 0x03052EC0u) | 8;
            U8(st, G_GRID_BITFIELDS + t) &= 0x60;
            U32(st, G_GRID_TERRAIN + 4u * t) = ter;
            if ((ter & 0x40) == 0 && type != 0xD1) {
                U32(st, G_GRID_TERRAIN + 4u * t) = ter | 0x20;
                U32(st, G_GRID_IMAGE + 4u * t) = (uint32_t) ((U8(st, G_GRID_RANDOM + t) & 3) + S32(st, G_IMAGE_GROUPS + 8 * 328));
            } else {
                /* @bin 0x0044048C byte table, 0x00440478 jump table */
                int32_t img;
                uint32_t sel = o == 0 ? 0 : o == 2 ? 1 : o == 4 ? 2 : 3;
                switch (type) {
                case 0x1F:
                    img = S32(st, G_IMAGE_GROUPS + 8 * 381) + S32(st, G_AREA_IMAGES_1F + 0x10u * sel + 4u * (uint32_t) (col + row * 2));
                    break;
                case 0x1E:
                    img = S32(st, G_IMAGE_GROUPS + 8 * 327) + S32(st, G_AREA_IMAGES_1E + 0x24u * sel + 4u * (uint32_t) (row * 3 + col));
                    break;
                case 0x21:
                    img = S32(st, G_IMAGE_GROUPS + 8 * 319) + S32(st, G_AREA_IMAGES_21 + 0x40u * sel + 4u * (uint32_t) (col + row * 4));
                    break;
                case 0xD1:
                    img = S32(st, G_IMAGE_GROUPS + 8 * 318) + S32(st, G_AREA_IMAGES_D1 + 0x64u * sel + 4u * (uint32_t) (row * 5 + col));
                    break;
                default:
                    img = S32(st, G_IMAGE_GROUPS + 8 * 437);
                    break;
                }
                U32(st, G_GRID_IMAGE + 4u * t) = (uint32_t) img;
                U8(st, G_GRID_ROUTING + t) = 0;
            }
            U8(st, G_GRID_EDGE + t) |= 0x40;
        }
    }
}

void tile_redraw_4c0860(game_state *st, int32_t x, int32_t y, int32_t group, int32_t add)
{
    /* @bin 0x004C0860 a tile on the map (0x0046F930 with the map's origin 0x00BCA2E8, 0x00BCA2EC)
     * given to the city's building +0x472C, with image group `group` + add */
    if ((uint8_t) GAME_CALL(st, FN_MAP_TILE_INSIDE, S32(st, G_VIEW_DX) + x, S32(st, G_VIEW_DY) + y) == 0) {
        return;
    }
    int32_t t = S32(st, G_MAP_START) + y * 228 + x;
    S32(st, G_TILE_CURSOR) = t;
    U32(st, G_GRID_IMAGE + 4u * (uint32_t) t) = (uint32_t) (S32(st, G_IMAGE_GROUPS + 8u * (uint32_t) group) + add);
    U8(st, G_GRID_EDGE + (uint32_t) t) |= 0x40;
    U8(st, G_CANAL + (uint32_t) t) = 0;
    U8(st, G_GRID_BITFIELDS + (uint32_t) t) &= 0xF0;
    S16(st, G_GRID_BUILDING + 2u * (uint32_t) t) = S16(st, CURRENT_CITY(st) + CITY_TEMPLE_COMPLEX_ID);
    U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t) |= 8;
}

void building_area_redraw_4c0be0(game_state *st, int32_t b)
{
    /* @bin 0x004C0BE0 the 13 x 7 area of table 0x005DF9B0 (0x14 bytes a tile: +0 group, +4..+0x10
     * the image by view 0x005E014C) in front of b, by its orientation +0xAA (as 0x004C0920) */
    uint32_t r = BUILDING_ADDR(b);
    int32_t dir = U8(st, r + BLD_ORIENTATION);
    int32_t h = dir / 2;
    int32_t x0 = S16(st, r + BLD_X) + S32(st, G_COMPLEX_ORIGIN + 8u * (uint32_t) h);
    int32_t y0 = S16(st, r + BLD_Y) + S32(st, G_COMPLEX_ORIGIN + 4 + 8u * (uint32_t) h);
    int32_t sel = S32(st, G_COMPLEX_VIEW + 4u * (uint32_t) (h + (S32(st, G_VIEW_ORIENTATION) / 2) * 4));
    int32_t ox = x0, oy = y0;
    for (uint32_t e = G_COMPLEX_TABLE + 0xC; e < G_COMPLEX_TABLE + 0xC + 0x14 * 13 * 7;) {
        int32_t d1 = x0 - y0, d2 = y0 - x0;
        int32_t a = x0, c = y0;
        for (int32_t n = 13; n != 0; n--, e += 0x14, c--, a++) {
            int32_t tx, ty;
            switch (dir) {
            case 0:
                ty = oy;
                tx = a;
                break;
            case 2:
                tx = ox;
                ty = c;
                break;
            case 4:
                tx = c + d1;
                ty = ox + d2;
                break;
            default:
                ty = d2 + a;
                tx = oy + d1;
                break;
            }
            int32_t group = S32(st, e - 0xC);
            if (group == 0) {
                continue;
            }
            int32_t add;
            switch (sel) {
            case 0:
                add = S32(st, e - 8);
                break;
            case 2:
                add = S32(st, e - 4);
                break;
            case 4:
                add = S32(st, e);
                break;
            default:
                add = S32(st, e + 4);
                break;
            }
            tile_redraw_4c0860(st, tx, ty, group, add);
            GAME_CALL(st, FN_TERRAIN_LAND_AROUND, tx, ty, 1);
        }
        ox--;
        oy++;
    }
}

void building_redraw_4c0d50(game_state *st, int32_t b, int32_t add)
{
    /* @bin 0x004C0D50 a main building (+0x38 == 0) and its parts (+0x3A): 3 x 3 images by view
     * and direction (table 0x005DF8B0); groups 0x23B / 0x23A by the city's building +0x472C
     * (0x004C52D0); then its area (0x004C0BE0) */
    uint32_t r = BUILDING_ADDR(b);
    if (S16(st, r + BLD_PREV_PART) != 0) {
        return;
    }
    int32_t dir = U8(st, r + BLD_ORIENTATION);
    if (b != 0) {
        int32_t h4 = (dir / 2) * 4;
        int32_t part = 0;
        int32_t cur = b;
        do {
            int32_t o = S32(st, G_VIEW_ORIENTATION);
            int32_t turn;
            if (o == 0 || o == 4) {
                turn = dir == 2 || dir == 6 ? 3 : 0;
            } else {
                turn = dir == 2 || dir == 6 ? 0 : 3;
            }
            int32_t idx = o / 2 + h4;
            int32_t g = S32(st, G_COMPLEX_PART_GROUPS + 4u * (uint32_t) (idx * 3 + part));
            int32_t img;
            if (g == 0x23B) {
                if ((uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_TEMPLE_COMPLEX_ID), 1) != 0) {
                    img = S32(st, G_IMAGE_GROUPS + 8 * 575) + 2 + (turn != 0);
                } else {
                    img = S32(st, G_IMAGE_GROUPS + 8 * 571) + turn + add;
                }
            } else if (g == 0x23A) {
                if ((uint8_t) GAME_CALL(st, FN_BUILDING_0XA9_IS, S32(st, CURRENT_CITY(st) + CITY_TEMPLE_COMPLEX_ID), 2) != 0) {
                    img = S32(st, G_IMAGE_GROUPS + 8 * 575) + (turn != 0);
                } else {
                    img = S32(st, G_IMAGE_GROUPS + 8 * 570) + turn + add;
                }
            } else {
                img = S32(st, G_IMAGE_GROUPS + 8u * (uint32_t) g) + turn + add;
            }
            uint32_t cr = BUILDING_ADDR(cur);
            tiles(st, cur, S16(st, cr + BLD_X), S16(st, cr + BLD_Y), 3, 3, img, 8);
            cur = S16(st, cr + BLD_NEXT_PART);
            part++;
        } while (cur != 0);
    }
    building_area_redraw_4c0be0(st, b);
}

void mastaba_redraw_4ee4a0(game_state *st, int32_t b)
{
    /* @bin 0x004EE4A0 each part of a mastaba-like monument (count by its size +0x54, tables
     * 0x0056F7EC / 0x0056F7D8 by mission kind 0x005E4EF4) through 0x004EDF50; then the ramp
     * 0x004ED360 from the corner of table 0x005F5D08 when 0x004ECEA0 accepts */
    uint32_t r = BUILDING_ADDR(b);
    int32_t size = U8(st, r + BLD_MONUMENT_KIND);
    int32_t h = S32(st, G_VIEW_ORIENTATION) / 2;
    for (int32_t p = b;; p++) {
        int32_t n = S32(st, G_PYRAMID_KIND) == 5 ? S32(st, G_PYRAMID_PARTS_KIND5 + 4u * (uint32_t) size)
                                               : S32(st, G_PYRAMID_PARTS + 4u * (uint32_t) size);
        if (p >= n + b) {
            break;
        }
        GAME_CALL(st, FN_PYRAMID_PART_REDRAW, p, h, size);
    }
    if ((uint8_t) GAME_CALL(st, FN_PYRAMID_KIND_OK, size) == 0) {
        return;
    }
    int32_t c = S32(st, G_PYRAMID_RAMP_START + 4u * (uint32_t) size) + S32(st, r + BLD_GRID_OFFSET) - S32(st, G_MAP_START);
    GAME_CALL(st, FN_RAMP_WALK, c % 228, c / 228, 4, 0);
}

void mastabas_redraw_4ee560(game_state *st)
{
    /* @bin 0x004EE560 every building of type 0xB7 (0x004EE4A0), skipping its parts */
    for (int32_t b = 1; b < 4000; b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) == 0 || S16(st, r + BLD_TYPE) != 0xB7) {
            continue;
        }
        mastaba_redraw_4ee4a0(st, b);
        int32_t size = U8(st, r + BLD_MONUMENT_KIND);
        int32_t n = S32(st, G_PYRAMID_KIND) == 5 ? S32(st, G_PYRAMID_PARTS_KIND5 + 4u * (uint32_t) size)
                                               : S32(st, G_PYRAMID_PARTS + 4u * (uint32_t) size);
        b += n - 1;
    }
}

/* ---- the mastaba-like monuments (0x004DFD90) ----------------------------------------------- */

void monument_part_layout_4ded50(game_state *st, int32_t out[13], int32_t part, int32_t orient, int32_t dir)
{
    /* @bin 0x004DED50 the part's 13 words of table 0x005E2AD8 (0x34 bytes a part): +0 x, +4 y,
     * +8 size, +0x10 the offset y * 228 + x, turned by the view and the direction */
    for (int i = 0; i < 13; i++) {
        out[i] = S32(st, G_MASTABA_LAYOUT + 0x34u * (uint32_t) part + 4u * (uint32_t) i);
    }
    if (dir != 0 && dir != 4) {
        if (orient == 4 || orient == 6) {
            out[0] = 0x16 - out[0] - out[2];
            out[1] = 8 - out[1] - out[2];
        }
        int32_t t = out[0];
        out[0] = out[1];
        out[1] = t;
        out[4] = out[0] + t * 228;
        return;
    }
    if (orient == 4 || orient == 2) {
        int32_t a = 0x16 - out[0] - out[2];
        int32_t c = 8 - out[1] - out[2];
        out[0] = a;
        out[1] = c;
        out[4] = a + c * 228;
    }
}

int32_t monument_part_image_4df360(game_state *st, int32_t b, int32_t *flags)
{
    /* @bin 0x004DF360 a mastaba-like part's image by its kind (layout word 3, low byte) and stage
     * +0x58; *flags: 1 the tiles one by one, 4, 5 */
    static const int32_t turn[16] = {0, 1, 0, 1, 3, 2, 3, 2, 0, 1, 0, 1, 3, 2, 3, 2};
    uint32_t r = BUILDING_ADDR(b);
    int32_t lay[13];
    *flags = 0;
    int32_t dir = S32(st, r + BLD_LAYOUT_DIRECTION);
    monument_part_layout_4ded50(st, lay, S16(st, r + BLD_PART), S32(st, G_VIEW_ORIENTATION), dir);
    uint32_t kindw = (uint32_t) lay[3];
    uint32_t kind = kindw & 0xFF;
    int32_t stage;
    if (kind > 0xD) {
        return 0;
    }
    if (kind <= 3) {
        /* @bin 0x004DF412.. walls: maus0_1a..1d graphics (table 0x005E4268 by the main part's
         * flags +0x64 bit 0x10 << kind, mission 0x00E94E18) */
        static const uint32_t done[4] = {G_IMAGE_GROUPS + 8 * 786, G_IMAGE_GROUPS + 8 * 787, G_IMAGE_GROUPS + 8 * 788, G_IMAGE_GROUPS + 8 * 789};
        static const uint32_t building[4] = {G_IMAGE_GROUPS + 8 * 564, G_IMAGE_GROUPS + 8 * 565, G_IMAGE_GROUPS + 8 * 566, G_IMAGE_GROUPS + 8 * 567};
        if (S16(st, r + BLD_SEQUENCE) == -1) {
            return S32(st, done[kind]) - (((S32(st, G_VIEW_ORIENTATION) + dir) / 2) & 1) + 1;
        }
        int32_t v = S32(st, r + BLD_MONUMENT_STAGE), i;
        if (v < 0xFF) {
            i = 0;
        } else if ((U8(st, BUILDING_ADDR(b - S16(st, r + BLD_PART)) + BLD_MONUMENT_FLAGS) & (0x10u << kind)) == 0) {
            i = 0;
        } else {
            i = (v >= 0xFFFF) + 1;
        }
        int32_t m = S32(st, G_MASTABA_VARIANT);
        GAME_CALLT(st, G_GRAPHICS_GFX, FN_GRAPHICS_LOAD_SLOT, S32(st, G_MASTABA_WALL_FILES + 4u * kind + 16u * (uint32_t) (i + m * 3)),
                   (int32_t) (5 + kind));
        return S32(st, building[kind]) - (((dir + S32(st, G_VIEW_ORIENTATION)) / 2) & 1) + 1;
    }
    if (kind <= 7) {
        if (S16(st, r + BLD_SEQUENCE) != -1) {
            *flags = 5;
        }
        stage = S32(st, r + BLD_MONUMENT_STAGE);
        if (stage == 2) {
            return S32(st, G_IMAGE_GROUPS + 8 * 568) + (int32_t) (kind - 4);
        }
        if (stage == 1) {
            return S32(st, G_PLAIN);
        }
        goto marked;
    }
    if (S16(st, r + BLD_SEQUENCE) != -1) {
        *flags = 1;
    }
    if (S32(st, r + BLD_MONUMENT_STAGE) == 2) {
        int32_t ti = S32(st, G_VIEW_ORIENTATION) / 2 + (dir / 2) * 4;
        if (ti < 0 || ti >= 16) {
            state_unported(st, FN_MONUMENT_PART_IMAGE_4DF360); /* the original reads its stack outside the table */
            return 0;
        }
        int32_t tv = turn[ti];
        switch (kind) {
        case 8:
            return tv + S32(st, G_IMAGE_GROUPS + 8 * 568) + 4;
        case 9:
            return (int32_t) (((uint32_t) tv - 2) & 3) + S32(st, G_IMAGE_GROUPS + 8 * 568) + 4;
        case 10:
            return S32(st, G_IMAGE_GROUPS + 8 * 568) + (int32_t) (((uint32_t) tv - 2) & 3) * 2 + 9;
        case 11:
            return S32(st, G_IMAGE_GROUPS + 8 * 568) + (int32_t) (((uint32_t) tv - 2) & 3) * 2 + 8;
        case 12:
            return S32(st, G_IMAGE_GROUPS + 8 * 568) + tv * 2 + 8;
        default:
            return S32(st, G_IMAGE_GROUPS + 8 * 568) + tv * 2 + 9;
        }
    }
    *flags |= 4;
    if (S32(st, r + BLD_MONUMENT_STAGE) == 1) {
        return S32(st, G_PLAIN);
    }
marked:
    if ((kindw & 0x10000) != 0) {
        return U8(st, G_MONUMENT_MODE) != 3 ? S32(st, G_IMAGE_GROUPS + 8 * 759) : S32(st, G_IMAGE_GROUPS + 8 * 790);
    }
    return 0;
}

void monument_part_place_4dfd90(game_state *st, int32_t b)
{
    /* @bin 0x004DFD90 a mastaba-like part placed again from its main corner (+0x68, +0x6C) and
     * layout (0x004DED50); its worker figure +0x5C follows; then drawn (0x004DFBB0) */
    uint32_t r = BUILDING_ADDR(b);
    int32_t lay[13];
    monument_part_layout_4ded50(st, lay, S16(st, r + BLD_PART), S32(st, G_VIEW_ORIENTATION), S32(st, r + BLD_LAYOUT_DIRECTION));
    int32_t y = S32(st, r + BLD_LAYOUT_Y) + lay[1];
    int32_t x = S32(st, r + BLD_LAYOUT_X) + lay[0];
    S16(st, r + BLD_X) = (int16_t) x;
    S16(st, r + BLD_Y) = (int16_t) y;
    S32(st, r + BLD_GRID_OFFSET) = S32(st, G_MAP_START) + y * 228 + x;
    /* @bin 0x004E0C50 (building_entry_corner) the corner into the road access +0x26, +0x28 */
    S16(st, r + BLD_ROAD_ACCESS_X) = S16(st, r + BLD_X);
    S16(st, r + BLD_ROAD_ACCESS_Y) = S16(st, r + BLD_Y);
    int16_t ay = S16(st, r + BLD_Y); /* ax in the original: the corner's y */
    int32_t f = S32(st, r + BLD_MONUMENT_WORKER);
    if (f != 0) {
        uint32_t fr = FIGURE_ADDR(f);
        uint8_t type = U8(st, fr + FIG_TYPE);
        uint8_t act = U8(st, fr + FIG_ACTION);
        int to_route = 0, moved = 0;
        if (type == 0x4F) {
            if (act == 9) {
                to_route = 1;
            } else if (act == 0xA) {
                moved = 1;
            }
        } else if (type == 0x51) {
            if (act == 9) {
                to_route = 1;
            } else if (act == 0xB || act == 0xA) {
                GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, f);
                S16(st, fr + FIG_X) = S16(st, r + BLD_X);
                S16(st, fr + FIG_Y) = S16(st, r + BLD_Y);
                S32(st, fr + FIG_GRID_OFFSET) = S32(st, r + BLD_GRID_OFFSET);
                S32(st, fr + FIG_U28) = S32(st, r + BLD_GRID_OFFSET);
                GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, f);
                if (U8(st, fr + FIG_ACTION) == 0xA) {
                    int32_t g = S16(st, FIGURE_ADDR(S32(st, G_CURRENT_FIGURE)) + FIG_LEADER);
                    if (g != 0) {
                        uint32_t gr = FIGURE_ADDR(g);
                        if (U8(st, gr + FIG_TYPE) == 0x55 && U8(st, gr + FIG_ACTION) == 0x1A) {
                            S16(st, gr + FIG_DEST_X) = S16(st, r + BLD_ROAD_ACCESS_X);
                            S16(st, gr + FIG_DEST_Y) = S16(st, r + BLD_ROAD_ACCESS_Y);
                            S32(st, gr + FIG_U28) = S32(st, r + BLD_GRID_OFFSET);
                            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, g);
                            S16(st, gr + FIG_ROAM_LENGTH) = 0;
                        }
                    }
                }
            }
        } else if (type == 0x55) {
            if (act == 0x1B) {
                to_route = 1;
            } else if (act == 0x1C) {
                moved = 1;
            }
        }
        if (to_route) {
            S16(st, fr + FIG_DEST_X) = S16(st, r + BLD_ROAD_ACCESS_X);
            S16(st, fr + FIG_DEST_Y) = ay;
            S32(st, fr + FIG_U28) = S32(st, r + BLD_GRID_OFFSET);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, f);
            S16(st, fr + FIG_ROAM_LENGTH) = 0;
        } else if (moved) {
            GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, f);
            S16(st, fr + FIG_X) = S16(st, r + BLD_X);
            S16(st, fr + FIG_Y) = S16(st, r + BLD_Y);
            S32(st, fr + FIG_GRID_OFFSET) = S32(st, r + BLD_GRID_OFFSET);
            GAME_CALL(st, FN_FIGURE_TILE_LIST_ADD, f);
        }
    }
    GAME_CALL(st, FN_MONUMENT_PART_REDRAW, b);
}

/* ---- the table ---------------------------------------------------------------------------- */

static int32_t run_4c53d0(game_state *st, const int32_t *a)
{
    return god_index_4c53d0(st, a[0]);
}

static int32_t run_4bfb30(game_state *st, const int32_t *a)
{
    building_redraw_4bfb30(st, a[0]);
    return 0;
}

static int32_t run_47afd0(game_state *st, const int32_t *a)
{
    walls_rect_47afd0(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_47d1f0(game_state *st, const int32_t *a)
{
    building_redraw_47d1f0(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_440540(game_state *st, const int32_t *a)
{
    building_redraw_440540(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_440060(game_state *st, const int32_t *a)
{
    building_area_redraw_440060(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_4c0860(game_state *st, const int32_t *a)
{
    tile_redraw_4c0860(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_4c0be0(game_state *st, const int32_t *a)
{
    building_area_redraw_4c0be0(st, a[0]);
    return 0;
}

static int32_t run_4c0d50(game_state *st, const int32_t *a)
{
    building_redraw_4c0d50(st, a[0], a[1]);
    return 0;
}

static int32_t run_4ee4a0(game_state *st, const int32_t *a)
{
    mastaba_redraw_4ee4a0(st, a[0]);
    return 0;
}

static int32_t run_4ee560(game_state *st, const int32_t *a)
{
    (void) a;
    mastabas_redraw_4ee560(st);
    return 0;
}

static int32_t run_4ded50(game_state *st, const int32_t *a)
{
    int32_t out[13];
    if (state_ptr(st, (uint32_t) a[0], 13 * 4) == NULL) {
        state_unported(st, FN_MONUMENT_PART_LAYOUT_4DED50); /* output outside the state */
        return 0;
    }
    monument_part_layout_4ded50(st, out, a[1], a[2], a[3]);
    for (int i = 0; i < 13; i++) {
        U32(st, (uint32_t) a[0] + 4u * (uint32_t) i) = (uint32_t) out[i];
    }
    return a[0];
}

static int32_t run_4df360(game_state *st, const int32_t *a)
{
    int32_t f = 0;
    if (state_ptr(st, (uint32_t) a[1], 4) == NULL) {
        state_unported(st, FN_MONUMENT_PART_IMAGE_4DF360); /* output outside the state */
        return 0;
    }
    int32_t img = monument_part_image_4df360(st, a[0], &f);
    U32(st, (uint32_t) a[1]) = (uint32_t) f;
    return img;
}

static int32_t run_4dfd90(game_state *st, const int32_t *a)
{
    monument_part_place_4dfd90(st, a[0]);
    return 0;
}

const ported_function rest_redraw_ports[] = {
    {0x004C53D0u, "god_index_4c53d0", 1, 32, 0, run_4c53d0, 0},
    {0x004BFB30u, "building_redraw_4bfb30", 1, 0, 0, run_4bfb30, 0},
    {0x0047D1F0u, "building_redraw_47d1f0", 4, 0, 0, run_47d1f0, 0},
    {0x0047AFD0u, "walls_rect_47afd0", 3, 0, 0, run_47afd0, 0},
    {0x004405C0u, "gate_images_4405c0 (writes its caller's stack, in rest_redraw.c)", 3, 0, 0, NULL, 0},
    {0x00440540u, "building_redraw_440540", 3, 0, 0, run_440540, 0},
    {0x00440060u, "building_area_redraw_440060", 3, 0, 0, run_440060, 0},
    {0x004C0860u, "tile_redraw_4c0860", 4, 0, 0, run_4c0860, 0},
    {0x004C0BE0u, "building_area_redraw_4c0be0", 1, 0, 0, run_4c0be0, 0},
    {0x004C0D50u, "building_redraw_4c0d50", 2, 0, 0, run_4c0d50, 0},
    {0x004EE4A0u, "mastaba_redraw_4ee4a0", 1, 0, 0, run_4ee4a0, 0},
    {0x004EE560u, "mastabas_redraw_4ee560", 0, 0, 0, run_4ee560, 0},
    {0x004DED50u, "monument_part_layout_4ded50", 4, 32, 0, run_4ded50, 0},
    {0x004DF360u, "monument_part_image_4df360", 2, 32, 0, run_4df360, 0},
    {0x004DFD90u, "monument_part_place_4dfd90", 1, 0, 0, run_4dfd90, 0},
};
const int rest_redraw_ports_count = sizeof(rest_redraw_ports) / sizeof(rest_redraw_ports[0]);
