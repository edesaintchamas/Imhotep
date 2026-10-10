/* Imhotep - the plague of frogs: one building's frog (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004E70B0 (core/city/events_actions.c) calls 0x004E7300 on buildings of the hit type: on one
 * in two game random draws, a frog figure (type 0x6A) appears on the nearest tile of terrain
 * 0x40000 or 4 (water) within the map's size. */
#include <limits.h>

#include "city/month_mood.h"
#include "random/random.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"

#define BREC(id) BUILDING_ADDR((int32_t) (id))

static int32_t isqrt_ftol(int32_t d)
{
    /* @bin 0x004E7622 fild, fsqrt, __ftol (0x0056133C): the square root of an integer below 2^31
     * is an integer or farther than 2^-33 from one, so the truncated root is floor(sqrt(d)) */
    if (d < 0) {
        return INT32_MIN;
    }
    uint32_t r = 0;
    for (uint32_t bit = 1u << 30; bit != 0; bit >>= 2) {
        if ((uint32_t) d >= r + bit) {
            d -= (int32_t) (r + bit);
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
    }
    return (int32_t) r;
}

int32_t frog_tile_4e75a0(game_state *st, int32_t id, int32_t *ox, int32_t *oy)
{
    /* @bin 0x004E75A0 the nearest tile of terrain 0x40000 and of terrain 4 around the building
     * (0x004EB800 within the larger map side, 0x00BCA2D0 / 0x00BCA2D4); their distances (the
     * truncated square root, or the radius when none); the strictly nearer one, if nearer than the
     * radius, into *ox, *oy: 1. Else 0 */
    int32_t radius = S32(st, G_MAP_WIDTH);
    if (S32(st, G_MAP_HEIGHT) > radius) {
        radius = S32(st, G_MAP_HEIGHT);
    }
    uint32_t b = BREC(id);
    int32_t x1 = 0, y1 = 0, x2 = 0, y2 = 0, d1, d2;
    if (map_search_rings(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), 0, 0, 0x40000u, &x1, &y1, radius, 0) != 0) {
        int32_t dy = S16(st, b + BLD_Y) - y1, dx = S16(st, b + BLD_X) - x1;
        d1 = isqrt_ftol(dy * dy + dx * dx);
    } else {
        d1 = radius;
    }
    if (map_search_rings(st, S16(st, b + BLD_X), S16(st, b + BLD_Y), 0, 0, 4u, &x2, &y2, radius, 0) != 0) {
        int32_t dy = S16(st, b + BLD_Y) - y2, dx = S16(st, b + BLD_X) - x2;
        d2 = isqrt_ftol(dy * dy + dx * dx);
    } else {
        d2 = radius;
    }
    if (d1 > d2) {
        if (d2 < radius) {
            *ox = x2;
            *oy = y2;
            return 1;
        }
        return 0;
    }
    if (d1 < d2 && d1 < radius) {
        *ox = x1;
        *oy = y1;
        return 1;
    }
    return 0;
}

void frog_create_4e7410(game_state *st, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x004E7410 a figure of type 0x6A at (x, y) (0x004B4800 with 0x00EAAC28, flag 0):
     * (offsets from the figure's +8:) action +0x30 := 8, building +0x54 := id, the building's +0x2E := the figure; one game random
     * draw; +0x2E := the 15-bit draw % 150, +0x5C := 0, +0x6A := 6. */
    int32_t f = GAME_CALL(st, FN_FIGURE_CREATE, S32(st, G_PLAYER), 0x6A, x, y, 0);
    uint32_t a = FIGURE_ADDR(f);
    U8(st, a + FIG_ACTION) = 8;
    S16(st, a + FIG_TARGET_HOUSE) = (int16_t) id;
    S16(st, BREC(id) + BLD_IMMIGRANT) = (int16_t) f;
    random_next_state(st);
    S16(st, a + FIG_WAIT_TICKS) = (int16_t) (S32(st, G_R_1_15) % 0x96);
    U8(st, a + FIG_MIGRANT_PEOPLE) = 0;
    U8(st, a + FIG_TERRAIN_USAGE) = 6;
    /* @bin 0x004E7573 sprintf(buffer, 0x005E4958, figure, x, y, id) into the original's frame, then
     * OutputDebugStringA(buffer): debugger output only (0: a buffer outside the state) */
    GAME_CALL(st, FN_CRT_SPRINTF, 0, G_U5E4958, f, x, y, id);
}

void frog_maybe_4e7300(game_state *st, int32_t id)
{
    /* @bin 0x004E7300 one game random draw; when its 15-bit value % 100 is below 50, the frog's
     * tile (0x004E75A0) and, when there is one, the frog (0x004E7410) */
    random_next_state(st);
    if (S32(st, G_R_1_15) % 100 >= 0x32) {
        return;
    }
    int32_t x = 0, y = id;
    if (frog_tile_4e75a0(st, id, &x, &y) != 0) {
        frog_create_4e7410(st, id, x, y);
    }
}

static int32_t run_4e7300(game_state *st, const int32_t *a)
{
    frog_maybe_4e7300(st, a[0]);
    return 0;
}

static int32_t run_4e7410(game_state *st, const int32_t *a)
{
    frog_create_4e7410(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_4e75a0(game_state *st, const int32_t *a)
{
    if (!state_ptr(st, (uint32_t) a[1], 4) || !state_ptr(st, (uint32_t) a[2], 4)) {
        state_unported(st, FN_FROG_TILE_4E75A0); /* outputs in the caller's frame */
        return 0;
    }
    int32_t x = S32(st, (uint32_t) a[1]), y = S32(st, (uint32_t) a[2]);
    int32_t r = frog_tile_4e75a0(st, a[0], &x, &y);
    P32(st, (uint32_t) a[1]) = (uint32_t) x;
    P32(st, (uint32_t) a[2]) = (uint32_t) y;
    return r;
}

const ported_function frog_ports[] = {
    {0x004E7300u, "frog_maybe_4e7300", 1, 0, 0, run_4e7300, 0},
    {0x004E7410u, "frog_create_4e7410", 3, 0, 0, run_4e7410, 0},
    {0x004E75A0u, "frog_tile_4e75a0", 3, 32, 0, run_4e75a0, 0},
};
const int frog_ports_count = sizeof(frog_ports) / sizeof(frog_ports[0]);
