/* Imhotep - the ferry ghost 0x0043D300 and its route draw 0x0043D030 (package cityview).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The whole of 0x0043D300: its rule part (the landing validity 0x00BCA314) is also
 * core/command/hover.c ferry_hover, which leaves the drawing out; this file draws too, with the
 * same calls (ferry_reach 0x0047F3E0, shore_fit 0x0047CC70, 0x00486CF0, 0x00486A30, 0x0047CBD0)
 * in the original's order. */
#include "ghost.h"

#include "city/day2_misc.h"
#include "command/hover.h"
#include "state/ported.h"

#define FERRY_STATE 0x00BCA310u    /* i32 1: the first landing is placed */
#define FERRY_VALID 0x00BCA314u    /* u8 */
#define FERRY_BUILDING 0x00BC9800u /* i32 the first landing */
#define REACH_DIR 0x00BC96E0u
#define REACH_LEN 0x00BC96E4u
#define FERRY_MODEL 0x005D117Cu    /* the model record of type 0x88 (0x005D06DC + 0x88 * 0x14) */

static int32_t ferry_image(game_state *st)
{
    return g_group(st, S32(st, FERRY_MODEL + 8)) + S32(st, FERRY_MODEL + 0xC);
}

int32_t ghost_43d010(game_state *st, int32_t dir)
{
    /* @bin 0x0043D010 (direction) the direction in the view: (d - orientation) mod 8, / 2 */
    int32_t v = dir - S32(st, G_ORIENT);
    if (v < 0) {
        v += 8;
    }
    return v / 2;
}

/* @bin 0x0043D030 (d, x, y, screen x, screen y, reach) the routes from a landing: over a fan of
 * tiles (rows 0 .. 2 * reach, half width up to 5) in direction d from (x, y), another landing not
 * linked yet is drawn green (the ferry image turned to face the other way); a tile where a
 * landing facing back would join (0x00486A30) gets a green 2x2 footprint. Returns 1 when one was
 * found (al) */
static int ferry_routes(game_state *st, int32_t d, int32_t x, int32_t y, int32_t sx, int32_t sy, int32_t reach)
{
    uint8_t found = 0;
    int32_t rows = reach * 2;
    if (rows < 0) {
        return 0;
    }
    int32_t c = 0;
    for (int32_t i = 0; i <= rows; i++) {
        for (int32_t j = -c; j <= c; j++) {
            int32_t opp = (d + 4) % 8; /* @bin 0x0043D0A4 signed remainder */
            int32_t dx, dy;
            if (d == 0) {
                dx = j;
                dy = -i;
            } else if (d == 2) {
                dx = i;
                dy = -j;
            } else if (d == 4) {
                dx = -j;
                dy = i;
            } else {
                dx = -i;
                dy = j;
            }
            int32_t tx = x + dx, ty = y + dy;
            /* @bin 0x0043D11E the tile's screen position (over the copies of sx, sy) */
            int32_t px = sx, py = sy;
            view_tile_to_screen(st, tx, ty, &px, &py);
            int32_t origin = S32(st, G_MAP_START) + x + y * 0xE4;
            int32_t off = S32(st, G_MAP_START) + ty * 0xE4 + dx + x;
            int32_t fit = shore_fit(st, tx, ty, 1, 0);
            int32_t b = S16(st, G_BUILDINGS + 2u * (uint32_t) off);
            if (GAME_CALL(st, 0x00486CF0u, off) != 0 && S32(st, BUILDING_ADDR(b) + 0xC4) == -1 && off != origin &&
                S16(st, G_BUILDINGS + 2u * (uint32_t) off) != S16(st, G_BUILDINGS + 2u * (uint32_t) origin)) {
                /* @bin 0x0043D1A1 the other landing, turned */
                int32_t lx = 0, ly = 0;
                view_tile_to_screen(st, tx, ty, &lx, &ly);
                U16(st, G_TINT) = GREEN;
                int32_t image = ferry_image(st) + ghost_43d010(st, opp);
                g_draw(st, image, lx, ly);
                g_top(st, image, lx, ly);
                U16(st, G_TINT) = 0;
                found = 1;
            }
            int32_t k = tile_bit4_pattern(st, tx, ty) * 2; /* @bin 0x0043D221 */
            if (k == opp && fit == 0 && GAME_CALL(st, 0x00486A30u, tx, ty, x, y, 2, k, opp, 0, 0) != 0) {
                /* @bin 0x0043D25A the 2x2 footprint, green (no tile-height shift) */
                for (uint32_t p = G_FOOT_OFFS; p < 0x005CFD10u; p += 8) {
                    g_foot(st, S32(st, IMG_FOOTPRINT), S32(st, p) + px, S32(st, p + 4) + py, GREEN);
                }
                found = 1;
            }
        }
        if (++c > 5) {
            c = 5;
        }
    }
    return found;
}

static void red_footprint(game_state *st, int32_t sx, int32_t sy)
{
    for (uint32_t p = G_FOOT_OFFS; p < 0x005CFD10u; p += 8) {
        g_foot(st, S32(st, IMG_FOOTPRINT), S32(st, p) + sx, S32(st, p + 4) - S32(st, G_TILE_H) + sy, RED);
    }
}

void ghost_ferry(game_state *st)
{
    /* @bin 0x0043D300 the ferry ghost */
    int32_t sx = 0, sy = 0;
    U8(st, FERRY_VALID) = 0;
    view_tile_to_screen(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), &sx, &sy);
    sy += S32(st, G_TILE_H);
    if (S32(st, FERRY_STATE) == 0) {
        /* the first landing */
        int drew = 0;
        ferry_reach(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y));
        int32_t d = S32(st, REACH_DIR);
        if (d < 0) {
            d += 8;
        }
        int32_t y0 = sy - S32(st, G_TILE_H);
        S32(st, 0x0070E13Cu) = 0;
        int32_t fit = shore_fit(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), 1, 0);
        int32_t b = S16(st, G_BUILDINGS + 2u * U32(st, G_HOVER));
        if (GAME_CALL(st, 0x00486CF0u, S32(st, G_HOVER)) != 0 && S32(st, BUILDING_ADDR(b) + 0xC4) == -1) {
            /* @bin 0x0043D3DE a landing not linked under the mouse: its routes, itself green */
            drew = 1;
            ferry_routes(st, d, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), sx, y0, S32(st, REACH_LEN));
            U16(st, G_TINT) = GREEN;
            int32_t image = ferry_image(st) + ghost_43d010(st, d);
            g_draw(st, image, sx, sy - S32(st, G_TILE_H));
            g_top(st, image, sx, sy - S32(st, G_TILE_H));
            U16(st, G_TINT) = 0;
        }
        if (fit == 0 && ferry_routes(st, d, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), sx, y0, S32(st, REACH_LEN))) {
            U8(st, FERRY_VALID) = 1; /* @bin 0x0043D498 */
            ghost_43dd70(st, 0, 0);
            U16(st, G_TINT) = 0;
            return;
        }
        if (drew) {
            return;
        }
        U8(st, FERRY_VALID) = 0;
        red_footprint(st, sx, sy);
        return;
    }
    /* @bin 0x0043D51A the second landing, joined to the first one 0x00BC9800 */
    int32_t k = tile_bit4_pattern(st, S32(st, G_HOVER_X), S32(st, G_HOVER_Y));
    uint32_t first = BUILDING_ADDR(S32(st, FERRY_BUILDING));
    int32_t size = U8(st, first + 3);
    int32_t rot = (int32_t) U8(st, first + 0xAA) * 2;
    int32_t fx = S16(st, first + 6), fy = S16(st, first + 8);
    if (GAME_CALL(st, 0x00486CF0u, S32(st, G_HOVER)) != 0) {
        int32_t b = S16(st, G_BUILDINGS + 2u * U32(st, G_HOVER));
        if (b != S32(st, FERRY_BUILDING) && S32(st, BUILDING_ADDR(b) + 0xC4) == -1) {
            /* @bin 0x0043D5BF another landing under the mouse: drawn green, turned */
            U16(st, G_TINT) = GREEN;
            int32_t image = ferry_image(st) + ghost_43d010(st, k * 2);
            g_draw(st, image, sx, sy - S32(st, G_TILE_H));
            g_top(st, image, sx, sy - S32(st, G_TILE_H));
            U16(st, G_TINT) = 0;
            return;
        }
    }
    int32_t fsx = 0, fsy = 0;
    view_tile_to_screen(st, fx, fy, &fsx, &fsy);
    ferry_routes(st, rot, fx, fy, fsx, fsy, S32(st, REACH_LEN)); /* @bin 0x0043D65E */
    int32_t opp = (rot + 4) % 8;
    /* @bin 0x0043D696 the route buffer and the length are locals of the draw: not kept */
    if (GAME_CALL(st, 0x00486A30u, S32(st, G_HOVER_X), S32(st, G_HOVER_Y), fx, fy, size, k * 2, opp, 0, 0) == 1) {
        U8(st, FERRY_VALID) = 1;
        ghost_43dd70(st, 0, 0);
        return;
    }
    U8(st, FERRY_VALID) = 0;
    red_footprint(st, sx, sy);
}

static int32_t run_43d010(game_state *st, const int32_t *a) { return ghost_43d010(st, a[0]); }
static int32_t run_routes(game_state *st, const int32_t *a) { return ferry_routes(st, a[0], a[1], a[2], a[3], a[4], a[5]); }

static int32_t run_ghost_ferry(game_state *st, const int32_t *a)
{
    (void) a;
    ghost_ferry(st);
    return 0;
}

const ported_function cityview_ghost_ferry_ports[] = {
    {0x0043D010u, "ghost_43d010", 1, 32, 0, run_43d010, 0},
    {0x0043D030u, "ghost_ferry_routes", 6, 8, 0, run_routes, 0},
    /* 0x0043D300 whole: drawing and rules (core/command/hover.c ferry_hover, the rule part alone,
     * is not registered) */
    {0x0043D300u, "ghost_ferry", 0, 0, 0, run_ghost_ferry, 0},
};
const int cityview_ghost_ferry_ports_count = sizeof(cityview_ghost_ferry_ports) / sizeof(cityview_ghost_ferry_ports[0]);
