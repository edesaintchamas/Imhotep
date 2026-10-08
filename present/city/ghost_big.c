/* Imhotep - the construction ghosts of forts, entertainment venues and the like (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Called by the ghost 0x0043B630 (present/city/ghost.c) for the types whose ghost has several parts:
 * forts 0x28 / 0x2C / 0x2D (0x0043D720), the venues 0x1E / 0x1F / 0x21 / 0xD1 (0x0043F5B0),
 * 0xD2 (0x00441490), 0x29..0x2B (0x00441F80). */
#include "ghost.h"

#include "command/place_ent.h"
#include "frame/frame.h"
#include "state/ported.h"

#define FORT_IMAGE 0x00F5662Cu  /* i32 the fort's first image */

/* the tiles of a footprint free for a fort (0x0043D7CD): no terrain of 0xEEFFD77F, not a wet
 * flood plain, no figure */
static int fort_tile_blocked(game_state *st, int32_t off)
{
    return (g_terrain(st, off) & 0xEEFFD77Fu) != 0 || g_flooded(st, off) ||
           S16(st, G_FIGURES + 2u * (uint32_t) off) != 0;
}

void ghost_43d720(game_state *st)
{
    /* @bin 0x0043D720 a fort (0x28, 0x2C, 0x2D): the fort (model 0x39) and its yard (model 0x36,
     * at the offset 0x005CDF78 by the orientation, drawn at 0x005D27E4 / 0x005D2804 / 0x005D2824) */
    int32_t sx = 0, sy = 0;
    int32_t bad = 0, red_all = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    int32_t y1 = sy + S32(st, G_TILE_H);
    if (S32(st, 0x00BCB9B4u) >= 6) {
        red_all = bad = 1; /* @bin 0x0043D761 six forts already */
    }
    if (g_treasury_low(st)) {
        red_all = bad = 1;
    }
    int32_t count = S32(st, G_FOOT_COUNT + 4u * U32(st, 0x005D0B50u));
    int32_t e = S32(st, G_ORIENT) / 2;
    uint32_t row = G_FOOT_TILES + 0x90u * (uint32_t) e;
    for (int32_t i = 0; i < count; i++) {
        if (fort_tile_blocked(st, S32(st, row + 4u * (uint32_t) i) + S32(st, G_HOVER))) {
            bad = 1;
        }
    }
    int32_t yard = S32(st, 0x005CDF78u + 4u * (uint32_t) e) + S32(st, G_HOVER);
    int32_t count2 = S32(st, G_FOOT_COUNT + 4u * U32(st, 0x005D0B14u));
    for (int32_t i = 0; i < count2; i++) {
        if (fort_tile_blocked(st, S32(st, row + 4u * (uint32_t) i) + yard)) {
            bad = 1;
        }
    }
    if (bad) {
        /* @bin 0x0043D88B the footprints */
        int32_t hov = S32(st, G_HOVER);
        for (int32_t i = 0; i < count; i++) {
            uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
            int32_t x = S32(st, d) + sx;
            int32_t y = S32(st, d + 4) - S32(st, G_TILE_H) + y1;
            int32_t off = S32(st, row + 4u * (uint32_t) i) + hov;
            int32_t colour;
            if ((g_terrain(st, off) & 0xEEFFD77Fu) != 0 || g_flooded(st, off)) {
                colour = RED;
            } else {
                colour = red_all ? RED : GREEN;
            }
            g_foot(st, S32(st, IMG_FOOTPRINT), x, y, colour);
        }
        e = S32(st, G_ORIENT) / 2;
        int32_t yard_off = S32(st, 0x005CDF78u + 4u * (uint32_t) e) + S32(st, G_HOVER);
        int32_t bx = S32(st, 0x005D2804u + 8u * (uint32_t) e) * S32(st, G_TILE_W) +
                     S32(st, 0x005D27E4u + 8u * (uint32_t) e) * S32(st, 0x0070E458u) +
                     S32(st, 0x005D2824u + 4u * (uint32_t) e) + sx;
        int32_t by = S32(st, 0x005D27E8u + 8u * (uint32_t) e) * S32(st, G_TILE_TOP) + y1 +
                     S32(st, 0x005D2808u + 8u * (uint32_t) e) * S32(st, G_TILE_H);
        int32_t n2 = S32(st, G_FOOT_COUNT + 4u * U32(st, 0x005D0B14u));
        uint32_t row2 = G_FOOT_TILES + 0x90u * (uint32_t) e;
        for (int32_t i = 0; i < n2; i++) {
            uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
            int32_t x = S32(st, d) + bx;
            int32_t y = by - S32(st, G_TILE_H) + S32(st, d + 4);
            int32_t off = S32(st, row2 + 4u * (uint32_t) i) + yard_off;
            int32_t colour = (fort_tile_blocked(st, off) || red_all) ? RED : GREEN;
            g_foot(st, S32(st, IMG_FOOTPRINT), x, y, colour);
        }
        return;
    }
    int32_t type;
    int32_t o = S32(st, G_ORIENT);
    if (o != 0 && o != 6) {
        /* @bin 0x0043DAB0 the yard first (behind the fort) */
        int32_t bx = S32(st, 0x005D2804u + 8u * (uint32_t) e) * S32(st, G_TILE_W) +
                     S32(st, 0x005D27E4u + 8u * (uint32_t) e) * S32(st, 0x0070E458u) +
                     S32(st, 0x005D2824u + 4u * (uint32_t) e) + sx;
        int32_t by = S32(st, 0x005D27E8u + 8u * (uint32_t) e) * S32(st, G_TILE_TOP) +
                     (S32(st, 0x005D2808u + 8u * (uint32_t) e) - 1) * S32(st, G_TILE_H) + y1;
        U16(st, G_TINT) = GREEN;
        g_draw(st, S32(st, FORT_IMAGE) + 1, bx, by);
        g_top(st, S32(st, FORT_IMAGE) + 1, bx, by);
        int32_t y = y1 - S32(st, G_TILE_H);
        g_draw(st, S32(st, FORT_IMAGE), sx, y);
        g_top(st, S32(st, FORT_IMAGE), sx, y);
        type = S32(st, G_TYPE);
        int32_t k = type == 0x28 ? 3 : type == 0x2D ? 4 : type == 0x2C ? 2 : -1;
        if (k >= 0) {
            g_draw(st, S32(st, FORT_IMAGE) + k, S32(st, G_TILE_W) + sx + 4, S32(st, G_TILE_H) + y - 5);
        }
        U16(st, G_TINT) = 0;
        return;
    }
    /* @bin 0x0043DC25 the fort first, then the yard */
    int32_t y = y1 - S32(st, G_TILE_H);
    U16(st, G_TINT) = GREEN;
    g_draw(st, S32(st, FORT_IMAGE), sx, y);
    g_top(st, S32(st, FORT_IMAGE), sx, y);
    type = S32(st, G_TYPE);
    int32_t k = type == 0x28 ? 3 : type == 0x2D ? 4 : type == 0x2C ? 2 : -1;
    if (k >= 0) {
        g_draw(st, S32(st, FORT_IMAGE) + k, S32(st, G_TILE_W) + sx + 4, S32(st, G_TILE_H) + y - 5);
    }
    e = S32(st, G_ORIENT) / 2;
    int32_t bx = S32(st, 0x005D2804u + 8u * (uint32_t) e) * S32(st, G_TILE_W) +
                 S32(st, 0x005D27E4u + 8u * (uint32_t) e) * S32(st, 0x0070E458u) +
                 S32(st, 0x005D2824u + 4u * (uint32_t) e) + sx;
    int32_t by = S32(st, 0x005D27E8u + 8u * (uint32_t) e) * S32(st, G_TILE_TOP) +
                 (S32(st, 0x005D2808u + 8u * (uint32_t) e) - 1) * S32(st, G_TILE_H) + y1;
    g_draw(st, S32(st, FORT_IMAGE) + 1, bx, by);
    g_top(st, S32(st, FORT_IMAGE) + 1, bx, by);
    U16(st, G_TINT) = 0;
}

/* @bin 0x004405C0 (v, &first, &second) the two images of a gate (0x00F566FC + 0..3) by the view
 * orientation and v (0 or 1); nothing written otherwise (also core/building/rest_redraw.c) */
static void gate_images(game_state *st, int32_t v, int32_t *first, int32_t *second)
{
    int32_t g = S32(st, 0x00F566FCu);
    static const int8_t tab[4][2][2] = {
        /* {second, first} by orientation / 2, v */
        {{0, 1}, {3, 2}}, {{3, 2}, {1, 0}}, {{1, 0}, {2, 3}}, {{2, 3}, {0, 1}}};
    int32_t o = S32(st, G_ORIENT);
    if ((uint32_t) o > 6 || (o & 1) || (v != 0 && v != 1)) {
        return;
    }
    *second = g + tab[o / 2][v][0];
    *first = g + tab[o / 2][v][1];
}

void ghost_43f5b0(game_state *st, int32_t type)
{
    /* @bin 0x0043F5B0 (type) an entertainment venue 0x1E / 0x1F / 0x21 / 0xD1 (n x n, n = 3, 2, 4,
     * 5): the fit 0x0043F410 sets the tint (green, else 0xF863) and 0x00605994; the stage parts
     * found by 0x00441130 (0x21: 0x21, 0x1E, 0x1F; 0x1E: 0x1E, 0x1F; 0x1F: 0x1F) drawn at their
     * tiles, the other tiles from the image tables 0x005D2078.. by the orientation */
    int32_t hx = S32(st, G_HOVER_X), hy = S32(st, G_HOVER_Y);
    int32_t n = 1;
    if (type == 0x1F) {
        n = 2;
    } else if (type == 0x1E) {
        n = 3;
    } else if (type == 0x21) {
        n = 4;
    } else if (type == 0xD1) {
        n = 5;
    }
    uint8_t bad;
    if (ent_fits_43f410(st, type) != 0) {
        U16(st, G_TINT) = GREEN;
        S32(st, 0x00605994u) = 1;
        bad = 0;
    } else {
        U16(st, G_TINT) = 0xF863;
        S32(st, 0x00605994u) = 0;
        bad = 1;
    }
    ent_roads_4413b0(st, hx, hy, type);
    uint8_t has1f = 0, has1e = 0, has21 = 0;
    if (type == 0x1E) {
        has1e = has1f = 1;
    } else if (type == 0x1F) {
        has1f = 1;
    } else if (type == 0x21) {
        has21 = has1e = has1f = 1;
    }
    int32_t r21 = 0, r1e = 0;
    int32_t x21 = hx, y21 = hy, x1e = hx, y1e = hy, x1f = hx, y1f = hy;
    if (has21) {
        r21 = ent_spot_441130(st, hx, hy, &x21, &y21, type, 0x21);
        if (r21 == -1) {
            bad = 1;
        }
    }
    if (has1e) {
        r1e = ent_spot_441130(st, hx, hy, &x1e, &y1e, type, 0x1E);
        if (r1e == -1) {
            bad = 1;
        }
    }
    if (has1f) {
        r21 = ent_spot_441130(st, hx, hy, &x1f, &y1f, type, 0x1F);
    }
    if (r21 == -1) {
        bad = 1;
    }
    int32_t first = 0, second = 0; /* the gate images (0x004405C0) */
    for (int32_t r = 0; r < n; r++) {
        int32_t ty = hy + r;
        for (int32_t c = 0; c < n; c++) {
            int32_t tx = hx + c;
            int32_t x = 0, y = 0;
            view_tile_to_screen(st, tx, ty, &x, &y);
            if (bad) {
                g_foot(st, S32(st, IMG_FOOTPRINT), x, y, RED);
                continue;
            }
            if (has1f && tx == x1f && ty == y1f) {
                /* @bin 0x0043F79B the 0x1F part */
                int32_t img = S32(st, 0x00F567ACu);
                g_draw(st, img, x, y);
                g_top(st, img, x, y);
                continue;
            }
            if (has1e) {
                if (tx == x1e && ty == y1e) {
                    gate_images(st, r1e, &first, &second);
                    g_draw(st, first, x, y);
                    g_top(st, first, x, y);
                    continue;
                }
                if (r1e != 0) {
                    if (tx == x1e + 1 && ty == y1e) {
                        gate_images(st, r1e, &first, &second);
                        g_draw(st, second, x, y);
                        g_top(st, second, x, y);
                        continue;
                    }
                } else if (tx == x1e && ty == y1e + 1) {
                    gate_images(st, 0, &first, &second);
                    g_draw(st, second, x, y);
                    g_top(st, second, x, y);
                    continue;
                }
            }
            if (has21) {
                if (tx == x21 && ty == y21) {
                    /* @bin 0x0043F91B the 0x21 part: its 2x2 image from the view's front tile */
                    int32_t o = S32(st, G_ORIENT);
                    if (o == 2) {
                        view_tile_to_screen(st, tx + 1, ty, &x, &y);
                    } else if (o == 4) {
                        view_tile_to_screen(st, c + hx + 1, ty + 1, &x, &y);
                    } else if (o == 6) {
                        view_tile_to_screen(st, tx, ty + 1, &x, &y);
                    }
                    int32_t img = S32(st, 0x00F5659Cu);
                    g_draw(st, img, x, y);
                    g_top(st, img, x, y);
                    continue;
                }
                if (tx >= x21 && ty >= y21 && tx < x21 + 2 && ty < y21 + 2) {
                    continue;
                }
            }
            /* @bin 0x0043F9C4 the venue's ground (jump tables 0x0043FC44 / 0x0043FC30) */
            int32_t o = S32(st, G_ORIENT);
            int32_t k = 0, img;
            uint32_t tab = 0;
            if (type == 0x1F) {
                tab = o == 0 ? 0x005D2078u : o == 2 ? 0x005D2088u : o == 4 ? 0x005D2098u : o == 6 ? 0x005D20A8u : 0;
                k = c + r * 2;
                img = S32(st, 0x00F5679Cu);
            } else if (type == 0x1E) {
                tab = o == 0 ? 0x005D20B8u : o == 2 ? 0x005D20DCu : o == 4 ? 0x005D2100u : o == 6 ? 0x005D2124u : 0;
                k = r * 3 + c;
                img = S32(st, 0x00F565ECu);
            } else if (type == 0x21) {
                tab = o == 0 ? 0x005D2148u : o == 2 ? 0x005D2188u : o == 4 ? 0x005D21C8u : o == 6 ? 0x005D2208u : 0;
                k = c + r * 4;
                img = S32(st, 0x00F565ACu);
            } else if (type == 0xD1) {
                tab = o == 0 ? 0x005D2248u : o == 2 ? 0x005D22ACu : o == 4 ? 0x005D2310u : o == 6 ? 0x005D2374u : 0;
                k = r * 5 + c;
                img = S32(st, 0x00F565A4u);
            } else {
                img = S32(st, 0x00F56774u);
            }
            if (tab) {
                img += S32(st, tab + 4u * (uint32_t) k);
            }
            g_draw(st, img, x, y);
        }
    }
    U16(st, G_TINT) = 0;
    S32(st, 0x00605994u) = bad == 0;
}

void ghost_441f80(game_state *st)
{
    /* @bin 0x00441F80 types 0x29..0x2B (statues): the R key turns them through 16 positions
     * (0x00BC9714: 4 images by its remainder mod 4, + its quarter) */
    int32_t sx = 0, sy = 0;
    int32_t bad = 0, red_all = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    int32_t y1 = sy + S32(st, G_TILE_H);
    int32_t size = S32(st, G_MODELS + 20u * U32(st, G_TYPE));
    int32_t hov = S32(st, G_HOVER);
    int32_t o = S32(st, G_BUILD_ORIENT);
    if ((uint16_t) frame_key_state(st, 0x52) == 0x8001) {
        o++;
        S32(st, G_BUILD_ORIENT) = o;
    }
    if (o > 0xF) {
        o = 0;
        S32(st, G_BUILD_ORIENT) = 0;
    }
    int32_t k = o % 4;
    int32_t q = (o + 4) / 4;
    if (q >= 4) {
        q = 0;
    }
    int32_t image;
    if (k < 2) {
        image = g_group(st, S32(st, G_MODELS + 20u * U32(st, G_TYPE) + 8)) + k * 4;
    } else {
        int32_t g = size == 1 ? S32(st, 0x00F5730Cu) : size == 2 ? S32(st, 0x00F57304u) : S32(st, 0x00F572FCu);
        image = g + k * 4 - 8;
    }
    image += q;
    if (S32(st, G_EDITOR) == 0) {
        int32_t hx = S32(st, G_HOVER_X), hy = S32(st, G_HOVER_Y);
        if (hx < 3 || hx + size - 1 >= S32(st, G_MAP_W) - 3 || hy < 3 || hy + size - 1 >= S32(st, G_MAP_H) - 3) {
            red_all = bad = 1;
        }
    }
    int32_t count = S32(st, G_FOOT_COUNT + 4u * (uint32_t) size);
    uint32_t row = G_FOOT_TILES + 0x90u * (uint32_t) (S32(st, G_ORIENT) / 2);
    for (int32_t i = 0; i < count; i++) {
        int32_t off = S32(st, row + 4u * (uint32_t) i) + hov;
        uint32_t t = g_terrain(st, off);
        if ((t & 0x10000) || g_flooded(st, off)) {
            bad = red_all = 1;
        }
        if (t & 0xEEFED77Fu) {
            bad = 1;
        }
        if (S16(st, G_FIGURES + 2u * (uint32_t) off) != 0) {
            bad = 1;
        }
    }
    if (g_treasury_low(st)) {
        red_all = 1;
    } else if (bad == 0) {
        int32_t y = y1 - S32(st, G_TILE_H);
        U16(st, G_TINT) = GREEN;
        g_draw(st, image, sx, y);
        g_top(st, image, sx, y);
        U16(st, G_TINT) = 0;
        return;
    }
    for (int32_t i = 0; i < count; i++) {
        uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
        int32_t x = S32(st, d) + sx;
        int32_t y = S32(st, d + 4) - S32(st, G_TILE_H) + y1;
        int32_t off = S32(st, row + 4u * (uint32_t) i) + hov;
        uint32_t t = g_terrain(st, off) & 0xEEFED77Fu;
        if (S16(st, G_FIGURES + 2u * (uint32_t) off) != 0) {
            t = 1;
        }
        g_foot(st, S32(st, IMG_FOOTPRINT), x, y, (red_all || t) ? RED : GREEN);
    }
}

/* the screen position of part 2 (k = 0) or 3 (k = 1) of the 0xD2 ghost, orientation e: the tile
 * steps 0x007756AC / 0x007756CC (x, y), the pixel steps 0x005D28B4 / 0x005D28D4 and the offsets
 * 0x005D28F4 / 0x005D2904 (@bin 0x0044181D) */
static int32_t d2_x(game_state *st, int k, int32_t e, int32_t sx)
{
    uint32_t steps = k ? 0x007756CCu : 0x007756ACu, pix = k ? 0x005D28D4u : 0x005D28B4u;
    uint32_t off = k ? 0x005D2904u : 0x005D28F4u;
    return S32(st, steps + 8u * (uint32_t) e) * S32(st, G_TILE_W) + S32(st, pix + 8u * (uint32_t) e) * S32(st, 0x0070E458u) +
           S32(st, off + 4u * (uint32_t) e) + sx;
}

static int32_t d2_y(game_state *st, int k, int32_t e, int32_t y1, int32_t minus)
{
    uint32_t steps = k ? 0x007756D0u : 0x007756B0u, pix = k ? 0x005D28D8u : 0x005D28B8u;
    return S32(st, pix + 8u * (uint32_t) e) * S32(st, G_TILE_TOP) + y1 +
           (S32(st, steps + 8u * (uint32_t) e) - minus) * S32(st, G_TILE_H);
}

static int d2_blocked(game_state *st, int32_t off)
{
    /* @bin 0x00441536 the terrain bits 0xEEFED77F, or near the map's edge (0x0046F9A0) */
    uint32_t t = g_terrain(st, off) & 0xEEFED77Fu;
    int near = (GAME_CALL(st, 0x0046F9A0u, off % 0xE4, off / 0xE4, 3) & 0xFF) != 0;
    return near || t != 0;
}

static void d2_draw(game_state *st, int32_t image, int32_t x, int32_t y)
{
    g_draw(st, image, x, y);
    g_top(st, image, x, y);
}

void ghost_441490(game_state *st)
{
    /* @bin 0x00441490 type 0xD2 in three parts (offsets 0x005CDFA8 / 0x005CDFB8 by the orientation;
     * images 0x00F56D54 / 0x00F56D5C / 0x00F56D64); refused when city +0x433A is set */
    int32_t sx = 0, sy = 0;
    int32_t bad = 0, red_all = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    uint32_t city = CURRENT_CITY(st);
    int32_t y1 = sy + S32(st, G_TILE_H);
    if (S16(st, city + 0x433A) != 0) {
        red_all = bad = 1;
    }
    if (S32(st, city + 0xC) <= -5000) {
        red_all = bad = 1;
    }
    int32_t count = S32(st, 0x005CFCECu);
    int32_t e = S32(st, G_ORIENT) / 2;
    uint32_t row = G_FOOT_TILES + 0x90u * (uint32_t) e;
    int32_t bases[3] = {S32(st, G_HOVER), S32(st, 0x005CDFA8u + 4u * (uint32_t) e) + S32(st, G_HOVER),
                        S32(st, 0x005CDFB8u + 4u * (uint32_t) e) + S32(st, G_HOVER)};
    for (int p = 0; p < 3; p++) {
        for (int32_t i = 0; i < count; i++) {
            if (d2_blocked(st, S32(st, row + 4u * (uint32_t) i) + bases[p])) {
                bad = 1;
            }
        }
    }
    if (bad) {
        /* @bin 0x004416EE the footprints of the three parts */
        int32_t ox[3] = {sx, d2_x(st, 0, e, sx), d2_x(st, 1, e, sx)};
        int32_t oy[3] = {y1, d2_y(st, 0, e, y1, 0), d2_y(st, 1, e, y1, 0)};
        for (int p = 0; p < 3; p++) {
            for (int32_t i = 0; i < count; i++) {
                uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
                int32_t x = S32(st, d) + ox[p];
                int32_t y = S32(st, d + 4) - S32(st, G_TILE_H) + oy[p];
                int32_t colour = d2_blocked(st, S32(st, row + 4u * (uint32_t) i) + bases[p]) || red_all ? RED : GREEN;
                g_foot(st, S32(st, IMG_FOOTPRINT), x, y, colour);
            }
        }
        return;
    }
    U16(st, G_TINT) = GREEN;
    int32_t y = y1 - S32(st, G_TILE_H);
    if (e == 0) {
        d2_draw(st, S32(st, 0x00F56D54u) + 1, sx, y);
        d2_draw(st, S32(st, 0x00F56D5Cu) + 1, d2_x(st, 0, 0, sx), d2_y(st, 0, 0, y1, 1));
        d2_draw(st, S32(st, 0x00F56D64u) + 1, d2_x(st, 1, 0, sx), d2_y(st, 1, 0, y1, 1));
    } else if (e == 2) {
        d2_draw(st, S32(st, 0x00F56D64u) + 1, sx, y);
        d2_draw(st, S32(st, 0x00F56D5Cu) + 1, d2_x(st, 0, 2, sx), d2_y(st, 0, 2, y1, 1));
        d2_draw(st, S32(st, 0x00F56D54u) + 1, d2_x(st, 1, 2, sx), d2_y(st, 1, 2, y1, 1));
    } else if (e == 3) {
        d2_draw(st, S32(st, 0x00F56D54u), sx, y);
        d2_draw(st, S32(st, 0x00F56D5Cu), d2_x(st, 0, 3, sx) - 1, d2_y(st, 0, 3, y1, 1));
        d2_draw(st, S32(st, 0x00F56D64u), d2_x(st, 1, 3, sx) - 2, d2_y(st, 1, 3, y1, 1));
    } else {
        d2_draw(st, S32(st, 0x00F56D54u), d2_x(st, 1, e, sx), d2_y(st, 1, e, y1, 1));
        d2_draw(st, S32(st, 0x00F56D5Cu), d2_x(st, 0, e, sx), d2_y(st, 0, e, y1, 1));
        d2_draw(st, S32(st, 0x00F56D64u), sx, y);
    }
    U16(st, G_TINT) = 0;
}

static int32_t run_43d720(game_state *st, const int32_t *a) { (void) a; ghost_43d720(st); return 0; }
static int32_t run_43f5b0(game_state *st, const int32_t *a) { ghost_43f5b0(st, a[0]); return 0; }
static int32_t run_441490(game_state *st, const int32_t *a) { (void) a; ghost_441490(st); return 0; }
static int32_t run_441f80(game_state *st, const int32_t *a) { (void) a; ghost_441f80(st); return 0; }

const ported_function cityview_ghost_big_ports[] = {
    {0x0043D720u, "ghost_fort", 0, 0, 0, run_43d720, 0},
    {0x0043F5B0u, "ghost_venue", 1, 0, 0, run_43f5b0, 0},
    {0x00441490u, "ghost_441490", 0, 0, 0, run_441490, 0},
    {0x00441F80u, "ghost_statue", 0, 0, 0, run_441f80, 0},
};
const int cityview_ghost_big_ports_count = sizeof(cityview_ghost_big_ports) / sizeof(cityview_ghost_big_ports[0]);
