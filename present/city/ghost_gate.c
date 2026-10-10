/* Imhotep - the construction ghosts of gatehouses and temple complexes (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Called by the ghost 0x0043B630 (present/city/ghost.c): gatehouses 0xAB / 0xCA (0x004BFCD0, its
 * red parts 0x004C0040), temple complexes 0x41..0x45 (0x004C0F30) and their additions 0xD3 / 0xD4
 * (0x004C2F50). */
#include "ghost.h"

#include "frame/frame.h"
#include "state/ported.h"

#define GATE_PARTS 0x005DF810u /* 16 bytes per part (4): +0 u8 flag, +4 size, +8 a, +0xC b */

void ghost_4c0040(game_state *st, int32_t x, int32_t y, int32_t rot)
{
    /* @bin 0x004C0040 (x, y, rotation) the gatehouse's parts as footprints, tile by tile: green
     * where land_block_fits 0x0047C880 allows it */
    for (uint32_t p = GATE_PARTS + 8; p < 0x005DF858u; p += 0x10) {
        int32_t a = S32(st, p), b = S32(st, p + 4);
        int32_t tx, ty;
        if (rot == 0) {
            tx = b + x;
            ty = a + y;
        } else if (rot == 1) {
            tx = a + x;
            ty = b + y;
        } else {
            tx = x;
            ty = y;
        }
        int32_t n = S32(st, p - 4);
        int32_t sx = 0, sy = 0;
        view_tile_to_screen(st, tx, ty, &sx, &sy);
        for (int32_t i = 0; i < n; i++, tx++) {
            int32_t yy = ty;
            for (int32_t j = 0; j < n; j++, yy++) {
                view_tile_to_screen(st, tx, yy, &sx, &sy);
                int32_t colour;
                if (U8(st, p - 8) != 0 && GAME_CALL(st, 0x0047C880u, tx, yy, 1, 1, 0x40, 0, 0) == 0) {
                    colour = GREEN;
                } else {
                    colour = GAME_CALL(st, 0x0047C880u, tx, yy, 1, 1, -1, 0, 0) != 0 ? GREEN : RED;
                }
                g_foot(st, S32(st, IMG_FOOTPRINT), sx, sy, colour);
            }
        }
    }
}

void ghost_4bfcd0(game_state *st)
{
    /* @bin 0x004BFCD0 a gatehouse (0xAB, 0xCA): the R key flips 0x00BC9714 (0 / 1); its four parts
     * (groups 0x005DF768, 0x1AE: the city god's 0x005DF778[0x004C53D0]) in the order of
     * 0x005DF78C by the view and the flip */
    int32_t k = GAME_CALL(st, 0x004C53D0u, 2);
    if (k == -1) {
        k = GAME_CALL(st, 0x004C53D0u, 1);
        if (k == -1) {
            k = 0;
        }
    }
    int32_t god_group = S32(st, 0x005DF778u + 4u * (uint32_t) k);
    int32_t hx = S32(st, G_HOVER_X), hy = S32(st, G_HOVER_Y);
    if ((uint16_t) frame_key_state(st, 0x52) == 0x8001) {
        S32(st, G_BUILD_ORIENT) = S32(st, G_BUILD_ORIENT) != 1;
    }
    int32_t rot = S32(st, G_BUILD_ORIENT);
    if ((GAME_CALL(st, 0x004BFF60u, hx, hy, rot) & 0xFF) == 0) {
        S32(st, 0x00605994u) = 0;
        ghost_4c0040(st, hx, hy, S32(st, G_BUILD_ORIENT));
        return;
    }
    int32_t v = S32(st, G_ORIENT) + S32(st, G_BUILD_ORIENT);
    if (v >= 8) {
        v -= 8;
    }
    for (int32_t j = 0; j < 4; j++) {
        int32_t p = S32(st, 0x005DF78Cu + 16u * (uint32_t) v + 4u * (uint32_t) j);
        /* @bin 0x004BFDB6 jump table 0x004BFEC0: the second image for v = 0, 3, 4, 7 */
        int32_t frame = (v == 0 || v == 3 || v == 4 || v == 7) ? 1 : 0;
        uint32_t r = GATE_PARTS + 16u * (uint32_t) p;
        int32_t tx, ty;
        int32_t flip = S32(st, G_BUILD_ORIENT);
        if (flip == 0) {
            tx = S32(st, r + 0xC) + hx;
            ty = S32(st, r + 8) + hy;
        } else if (flip == 1) {
            tx = S32(st, r + 8) + hx;
            ty = S32(st, r + 0xC) + hy;
        } else {
            tx = hx;
            ty = hy;
        }
        int32_t o = S32(st, G_ORIENT);
        int32_t n1 = S32(st, r + 4) - 1;
        if (o == 2) {
            tx += n1;
        } else if (o == 4) {
            tx += n1;
            ty += n1;
        } else if (o != 0) {
            ty += n1;
        }
        int32_t g = S32(st, 0x005DF768u + 4u * (uint32_t) j);
        if (g == 0x1AE) {
            g = god_group;
        }
        int32_t image = g_group(st, g) + frame;
        int32_t sx = 0, sy = 0;
        view_tile_to_screen(st, tx, ty, &sx, &sy);
        U16(st, G_TINT) = GREEN;
        g_draw(st, image, sx, sy);
        g_top(st, image, sx, sy);
        U16(st, G_TINT) = 0;
    }
}

/* ---- temple complexes (0x004C0F30) ---- */

#define COMPLEX_TABLE 0x005DF9B0u /* 7 rows x 13 records of 0x14 bytes: +0 group, +4..+0x10 image by turn */

/* the tile of record (r, c) of the 7 x 13 table from the corner (cx, cy), four layouts (dis
 * 0x004C1116 / 0x004C1260: C (cx + c, cy + r), A (cx - r, cy - c), B (cx - c, cy - r),
 * D (cx + r, cy + c)) chosen by the view orientation and the turn */
enum { LAY_C, LAY_A, LAY_B, LAY_D };

static int complex_layout(int32_t orient, int32_t turn2)
{
    static const int8_t lay[4][4] = {
        {LAY_C, LAY_A, LAY_B, LAY_D}, /* orientation 0, turn 0, 2, 4, 6 */
        {LAY_D, LAY_C, LAY_A, LAY_B}, /* 2 */
        {LAY_B, LAY_D, LAY_C, LAY_A}, /* 4 */
        {LAY_A, LAY_B, LAY_D, LAY_C}, /* other */
    };
    int o = orient == 0 ? 0 : orient == 2 ? 1 : orient == 4 ? 2 : 3;
    int t = turn2 == 0 ? 0 : turn2 == 2 ? 1 : turn2 == 4 ? 2 : 3;
    return lay[o][t];
}

static void complex_tile(int lay, int32_t cx, int32_t cy, int32_t r, int32_t c, int32_t *tx, int32_t *ty)
{
    switch (lay) {
    case LAY_C: *tx = cx + c; *ty = cy + r; break;
    case LAY_A: *tx = cx - r; *ty = cy - c; break;
    case LAY_B: *tx = cx - c; *ty = cy - r; break;
    default: *tx = cx + r; *ty = cy + c; break;
    }
}

/* one draw pass over table rows: first row, row step, row count, columns forward, the image field */
typedef struct {
    int8_t r0, dr, n, cols_up, field;
} complex_pass;

/* @bin 0x004C141C (before the temple, jump tables 0x004C2E38 / 0x004C2E54..) and 0x004C2270 (after
 * it, 0x004C2EC4 / 0x004C2EE0..): by the view orientation 0, 2, 4, 6 and the turn 0, 1, 2, 3 */
static const complex_pass PASS_BEFORE[4][4] = {
    {{0, 1, 5, 1, 4}, {5, 1, 2, 0, 0x10}, {2, 1, 5, 0, 0xC}, {1, -1, 2, 1, 8}},
    {{2, 1, 5, 1, 0xC}, {0, 1, 5, 0, 8}, {0, 1, 5, 0, 4}, {2, 1, 5, 1, 0x10}},
    {{1, -1, 2, 1, 4}, {2, 1, 5, 0, 0x10}, {5, 1, 2, 0, 0xC}, {0, 1, 5, 1, 8}},
    {{5, 1, 2, 1, 0xC}, {1, -1, 2, 0, 8}, {1, -1, 2, 0, 4}, {5, 1, 2, 1, 0x10}},
};
static const complex_pass PASS_AFTER[4][4] = {
    {{5, 1, 2, 1, 4}, {0, 1, 5, 0, 0x10}, {1, -1, 2, 0, 0xC}, {2, 1, 5, 1, 8}},
    {{1, -1, 2, 1, 0xC}, {5, 1, 2, 0, 8}, {5, 1, 2, 0, 4}, {1, -1, 2, 1, 0x10}},
    {{2, 1, 5, 1, 4}, {1, -1, 2, 0, 0x10}, {4, -1, 5, 0, 0xC}, {5, 1, 2, 1, 8}},
    {{4, -1, 5, 1, 0xC}, {2, 1, 5, 0, 8}, {2, 1, 5, 0, 4}, {4, -1, 5, 1, 0x10}},
};

static void complex_pass_draw(game_state *st, const complex_pass (*table)[4], int32_t cx, int32_t cy, int32_t *sx,
                              int32_t *sy)
{
    int32_t orient = S32(st, G_ORIENT);
    if ((uint32_t) orient > 6 || (orient & 1)) {
        return;
    }
    int32_t turn2 = S32(st, G_BUILD_ORIENT) * 2;
    if ((uint32_t) turn2 > 6) {
        return;
    }
    const complex_pass *p = &table[orient / 2][turn2 / 2];
    int lay = complex_layout(orient, turn2);
    for (int32_t k = 0, r = p->r0; k < p->n; k++, r += p->dr) {
        for (int32_t m = 0; m < 13; m++) {
            int32_t c = p->cols_up ? m : 12 - m;
            uint32_t rec = COMPLEX_TABLE + 0x104u * (uint32_t) r + 0x14u * (uint32_t) c;
            int32_t g = S32(st, rec);
            if (g == 0) {
                continue;
            }
            int32_t tx, ty;
            complex_tile(lay, cx, cy, r, c, &tx, &ty);
            view_tile_to_screen(st, tx, ty, sx, sy);
            int32_t image = g_group(st, g) + S32(st, rec + (uint32_t) p->field);
            g_draw(st, image, *sx, *sy);
            g_top(st, image, *sx, *sy);
        }
    }
}

static void d_two(game_state *st, int32_t image, int32_t x, int32_t y)
{
    g_draw(st, image, x, y);
    g_top(st, image, x, y);
}

void ghost_4c0f30(game_state *st, int32_t type)
{
    /* @bin 0x004C0F30 (type) a temple complex 0x41..0x45: a 7 x 13 block from the corner
     * 0x005E00CC (by the turn 0x00BC9714 and the view); the R key turns it while the city has no
     * complex yet (city +0x4478); the temple (0x00F56D7C / 84 / 8C) between two passes over the
     * table 0x005DF9B0 */
    int32_t sx0 = 0, sy0 = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx0, &sy0);
    int32_t y1 = sy0 + S32(st, G_TILE_H);
    int32_t bad = 0, red_all = 0;
    int32_t orient, turn, k3c;
    if (S16(st, CURRENT_CITY(st) + 0x4478) == 0) {
        int16_t key = frame_key_state(st, 0x52);
        orient = S32(st, G_ORIENT);
        if ((uint16_t) key == 0x8001) {
            int32_t e = orient / 2;
            turn = S32(st, G_BUILD_ORIENT) == e ? e + 3 : e;
            S32(st, G_BUILD_ORIENT) = turn;
            if (turn >= 4) {
                turn -= 4;
                S32(st, G_BUILD_ORIENT) = turn;
            }
        } else {
            turn = S32(st, G_BUILD_ORIENT);
        }
        /* @bin 0x004C0FCF the turns allowed in this view */
        int32_t a, b, dflt;
        if (orient == 0) {
            a = 0, b = 3, dflt = 0;
        } else if (orient == 2) {
            a = 0, b = 1, dflt = 1;
        } else if (orient == 4) {
            a = 2, b = 1, dflt = 2;
        } else {
            a = 2, b = 3, dflt = 3;
        }
        if (turn != a && turn != b) {
            turn = dflt;
            S32(st, G_BUILD_ORIENT) = turn;
        }
        k3c = turn;
    } else {
        turn = S32(st, G_BUILD_ORIENT);
        orient = S32(st, G_ORIENT);
        k3c = S32(st, G_ORIENT) / 2;
    }
    int32_t dx0 = 0, dy0 = 0;
    if (orient == 2) {
        dx0 = -2;
    } else if (orient == 4) {
        dx0 = dy0 = -2;
    } else if (orient == 6) {
        dy0 = -2;
    }
    int32_t turn2 = turn * 2;
    if (turn2 >= 8) {
        turn2 -= 8;
    }
    uint32_t corner = 0x005E00CCu + 8u * (uint32_t) (turn + (orient / 2) * 4);
    int32_t cx = S32(st, corner) + S32(st, G_HOVER_X);
    int32_t cy = S32(st, corner + 4) + S32(st, G_HOVER_Y);
    /* @bin 0x004C10F2 the block's tiles: terrain 0xEEFFD77F */
    int lay = complex_layout(orient, turn2);
    for (int32_t r = 0; r < 7; r++) {
        for (int32_t c = 0; c < 13; c++) {
            int32_t tx, ty;
            complex_tile(lay, cx, cy, r, c, &tx, &ty);
            if (g_terrain(st, S32(st, G_MAP_START) + ty * 0xE4 + tx) & 0xEEFFD77Fu) {
                bad = 1;
            }
        }
    }
    if (S16(st, CURRENT_CITY(st) + 0x4478) != 0) {
        red_all = 1;
    } else if (bad == 0) {
        goto good;
    }
    {
        /* @bin 0x004C121E the footprints (the layout by the turn as now) */
        /* the turn read again, not reduced mod 8 (a turn of 4.. takes each view's last layout) */
        int lay2 = complex_layout(S32(st, G_ORIENT), S32(st, G_BUILD_ORIENT) * 2);
        for (int32_t r = 0; r < 7; r++) {
            for (int32_t c = 0; c < 13; c++) {
                int32_t tx, ty, x = 0, y = 0;
                complex_tile(lay2, cx, cy, r, c, &tx, &ty);
                int32_t off = S32(st, G_MAP_START) + ty * 0xE4 + tx;
                view_tile_to_screen(st, tx, ty, &x, &y);
                int32_t colour = (g_terrain(st, off) & 0xEEFFD77Fu) || red_all ? RED : GREEN;
                g_foot(st, S32(st, IMG_FOOTPRINT), x, y, colour);
            }
        }
        return;
    }
good:;
    /* @bin 0x004C13C1 the complex's graphics slot (0x0053DD90), then the passes */
    int32_t part = ghost_4c0ed0(type);
    GAME_CALLT(st, GFX, 0x0053DD90u, S32(st, 0x005E018Cu + 4u * (uint32_t) part), 0xA);
    cx += dx0;
    cy += dy0;
    U16(st, G_TINT) = GREEN;
    int32_t sx = 0, sy = 0;
    complex_pass_draw(st, PASS_BEFORE, cx, cy, &sx, &sy);
    /* @bin 0x004C1F0A the temple in three parts, by k3c */
    if (k3c == 0) {
        d_two(st, S32(st, 0x00F56D7Cu), sx0, y1 - S32(st, G_TILE_H));
        d_two(st, S32(st, 0x00F56D84u), S32(st, 0x005DF850u) * S32(st, 0x0070E458u) + S32(st, 0x005DF890u) + S32(st, G_TILE_W) + sx0,
              S32(st, 0x005DF854u) * S32(st, G_TILE_TOP) + y1);
        d_two(st, S32(st, 0x00F56D8Cu), S32(st, 0x005DF870u) * S32(st, 0x0070E458u) + S32(st, 0x005DF8A0u) + sx0,
              S32(st, 0x005DF874u) * S32(st, G_TILE_TOP) - S32(st, G_TILE_H) + y1);
    } else if (k3c == 2) {
        d_two(st, S32(st, 0x00F56D7Cu), S32(st, 0x005DF880u) * S32(st, 0x0070E458u) + S32(st, 0x005DF8A8u) + sx0,
              S32(st, 0x005DF884u) * S32(st, G_TILE_TOP) - S32(st, G_TILE_H) + y1);
        d_two(st, S32(st, 0x00F56D84u), S32(st, 0x005DF860u) * S32(st, 0x0070E458u) + S32(st, 0x005DF898u) + S32(st, G_TILE_W) + sx0,
              S32(st, 0x005DF864u) * S32(st, G_TILE_TOP) + y1);
        d_two(st, S32(st, 0x00F56D8Cu), sx0, y1 - S32(st, G_TILE_H));
    } else if (k3c == 3) {
        d_two(st, S32(st, 0x00F56D7Cu) + 3, sx0, y1 - S32(st, G_TILE_H));
        d_two(st, S32(st, 0x00F56D84u) + 3, S32(st, 0x005DF868u) * S32(st, 0x0070E458u) + S32(st, 0x005DF89Cu) + S32(st, G_TILE_W) + sx0,
              S32(st, 0x005DF86Cu) * S32(st, G_TILE_TOP) + y1);
        d_two(st, S32(st, 0x00F56D8Cu) + 3, S32(st, 0x005DF888u) * S32(st, 0x0070E458u) + S32(st, 0x005DF8ACu) + sx0,
              S32(st, 0x005DF88Cu) * S32(st, G_TILE_TOP) - S32(st, G_TILE_H) + y1);
    } else {
        uint32_t k = (uint32_t) k3c;
        d_two(st, S32(st, 0x00F56D7Cu) + 3, S32(st, 0x005DF870u + 8u * k) * S32(st, 0x0070E458u) + S32(st, 0x005DF8A0u + 4u * k) + sx0,
              S32(st, 0x005DF874u + 8u * k) * S32(st, G_TILE_TOP) - S32(st, G_TILE_H) + y1);
        d_two(st, S32(st, 0x00F56D84u) + 3,
              S32(st, 0x005DF850u + 8u * k) * S32(st, 0x0070E458u) + S32(st, 0x005DF890u + 4u * k) + S32(st, G_TILE_W) + sx0,
              S32(st, 0x005DF854u + 8u * k) * S32(st, G_TILE_TOP) + y1);
        d_two(st, S32(st, 0x00F56D8Cu) + 3, sx0, y1 - S32(st, G_TILE_H));
    }
    complex_pass_draw(st, PASS_AFTER, cx, cy, &sx, &sy);
    U16(st, G_TINT) = 0;
}

void ghost_4c2f50(game_state *st, int32_t type)
{
    /* @bin 0x004C2F50 (type) an addition 0xD3 / 0xD4 to the temple complex under the mouse: refused
     * without a complex (city +0x4478), on another building than a complex 0x41..0x45 (main part
     * 0x00475B50), with city +0x448A < 2, or when 0x004C52D0 (city +0x472C, 1 / 2) says it is
     * there; else drawn on the complex's part whose image is 0x00F56D8C (0xD3) / 0x00F56D84 (0xD4)
     * (+3: the turned one) */
    int32_t sx = 0, sy = 0;
    int32_t count = S32(st, 0x005CFCE0u);
    int32_t e = S32(st, G_ORIENT) / 2;
    int32_t bad = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    int32_t y1 = sy + S32(st, G_TILE_H);
    uint32_t city = CURRENT_CITY(st);
    if (S16(st, city + 0x4478) == 0) {
        bad = 1;
    }
    int32_t hov = S32(st, G_HOVER);
    int32_t main = GAME_CALL(st, 0x00475B50u, S16(st, G_BUILDINGS + 2u * (uint32_t) hov));
    int32_t mt = S16(st, BUILDING_ADDR(main) + 0x10);
    if (mt < 0x41 || mt > 0x45) {
        bad = 1;
    }
    city = CURRENT_CITY(st);
    if (S16(st, city + 0x448A) < 2) {
        bad = 1;
    }
    if ((GAME_CALL(st, 0x004C52D0u, S32(st, city + 0x472C), type == 0xD3 ? 1 : 2) & 0xFF) != 0) {
        bad = 1;
    }
    if (g_treasury_low(st)) {
        bad = 1;
    } else if (bad == 0) {
        goto good;
    }
    if (count > 0) {
        uint32_t row = G_FOOT_TILES + 0x90u * (uint32_t) e;
        for (int32_t i = 0; i < count; i++) {
            uint32_t d = G_FOOT_OFFS + 8u * (uint32_t) i;
            int32_t x = S32(st, d) + sx;
            int32_t y = S32(st, d + 4) - S32(st, G_TILE_H) + y1;
            int32_t off = S32(st, row + 4u * (uint32_t) i) + hov;
            uint32_t v = (uint32_t) (int32_t) S16(st, G_FIGURES + 2u * (uint32_t) off) |
                         (g_terrain(st, off) & 0xEEFED77Fu);
            g_foot(st, S32(st, IMG_FOOTPRINT), x, y, (bad || v) ? RED : GREEN);
        }
    }
    U16(st, G_TINT) = 0;
    return;
good:;
    int32_t dx = 0, dy = 0;
    int32_t o = S32(st, G_ORIENT);
    if (o == 2) {
        dx = 2;
    } else if (o == 4) {
        dx = dy = 2;
    } else if (o == 6) {
        dy = 2;
    }
    U16(st, G_TINT) = GREEN;
    if (main == 0) {
        U16(st, G_TINT) = 0;
        return;
    }
    int32_t img = type == 0xD3 ? S32(st, 0x00F56D8Cu) : S32(st, 0x00F56D84u);
    int32_t k0 = type == 0xD3 ? 2 : 0;
    for (int32_t b = main;;) {
        uint32_t r = BUILDING_ADDR(b);
        int32_t tile = S32(st, G_MAP_START) + S16(st, r + 8) * 0xE4 + S16(st, r + 6);
        int32_t ti = S32(st, G_IMAGE + 4u * (uint32_t) tile);
        int32_t k = -1;
        if (ti == img) {
            k = k0;
        } else if (ti == img + 3) {
            k = k0 + 1;
        }
        if (k >= 0) {
            /* @bin 0x004C31F3 the addition on that part */
            int32_t x = 0, y = 0;
            view_tile_to_screen(st, S16(st, r + 6) + dx, S16(st, r + 8) + dy, &x, &y);
            g_draw(st, S32(st, 0x00F56DACu) + k, x, y);
            g_top(st, S32(st, 0x00F56DACu) + k, x, y);
            U16(st, G_TINT) = 0;
            return;
        }
        b = S16(st, r + 0x3A);
        if (b == 0) {
            U16(st, G_TINT) = 0;
            return;
        }
    }
}

static int32_t run_4bfcd0(game_state *st, const int32_t *a) { (void) a; ghost_4bfcd0(st); return 0; }
static int32_t run_4c0040(game_state *st, const int32_t *a) { ghost_4c0040(st, a[0], a[1], a[2]); return 0; }
static int32_t run_4c0f30(game_state *st, const int32_t *a) { ghost_4c0f30(st, a[0]); return 0; }
static int32_t run_4c2f50(game_state *st, const int32_t *a) { ghost_4c2f50(st, a[0]); return 0; }

const ported_function cityview_ghost_gate_ports[] = {
    {0x004BFCD0u, "ghost_gatehouse", 0, 0, 0, run_4bfcd0, 0},
    {0x004C0040u, "ghost_gatehouse_red", 3, 0, 0, run_4c0040, 0},
    {0x004C0F30u, "ghost_temple_complex", 1, 0, 0, run_4c0f30, 0},
    {0x004C2F50u, "ghost_complex_addition", 1, 0, 0, run_4c2f50, 0},
};
const int cityview_ghost_gate_ports_count = sizeof(cityview_ghost_gate_ports) / sizeof(cityview_ghost_gate_ports[0]);
