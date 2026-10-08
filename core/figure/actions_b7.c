/* Imhotep - figure actions, group b: helpers reached from the group's actions.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21); buildings: chunk 32. */
#include "figure/actions_b.h"

#include "city/month_mood.h"
#include "random/crt_rand.h"
#include "random/random.h"
#include "sim/call.h"
#include "sim/platform.h"

void figb_follow(game_state *st, int32_t id, int32_t leader, int32_t steps)
{
    /* @bin 0x004B22E0 move up to `steps` sub-steps towards the leader's previous tile */
    uint32_t f = FIGURE_ADDR(id);
    if (S16(st, f + 0x14) == S16(st, f + 0x2C) && S16(st, f + 0x16) == S16(st, f + 0x2E)) {
        U8(st, f + 0x65) = 1;
    }
    uint32_t lf = FIGURE_ADDR(leader);
    while (steps > 0) {
        steps--;
        int8_t p = (int8_t) (U8(st, f + 0x39) + 1);
        U8(st, f + 0x39) = (uint8_t) p;
        if (p >= 0x0F) {
            U8(st, f + 0x39) = 0x0F;
            int8_t d = (int8_t) GAME_CALL(st, 0x0051CBA0u, S16(st, f + 0x14), S16(st, f + 0x16), S16(st, lf + 0x18),
                                          S16(st, lf + 0x1A));
            U8(st, f + 0x11) = (uint8_t) d;
            if (d >= 8) {
                return;
            }
            U8(st, f + 0x12) = (uint8_t) d;
            U8(st, f + 0x39) = 0;
        }
        GAME_CALL(st, 0x004B3F00u, id);
        if (U8(st, f + 0x39) == 8) {
            GAME_CALL(st, 0x004B3C60u, id);
        }
    }
}

/* building types with this stock (0x46, 0xBE) */
static int stocked(game_state *st, int32_t b)
{
    int16_t t = S16(st, BUILDING_ADDR(b) + 0x10);
    return t == 0x46 || t == 0xBE;
}

int32_t figb_stock_max(game_state *st, int32_t b)
{
    /* @bin 0x004578B0 the largest stock (+0x56 words) among the goods listed at 0x00F53DDC */
    static const int8_t slot[19] = {0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, 9, -1, 8, -1, 10, -1, 11}; /* @bin 0x00457980 */
    int32_t best = 0;
    if (b <= 0) {
        return 0;
    }
    if (!stocked(st, b)) {
        return 0;
    }
    int32_t n = S32(st, 0x00F53DD8u);
    for (int32_t i = 0; i < n; i++) {
        uint32_t g = U32(st, 0x00F53DDCu + 4u * (uint32_t) i);
        if (g == 0) {
            continue;
        }
        int32_t k = g - 1 < 19 ? slot[g - 1] : -1;
        int32_t v = S16(st, BUILDING_ADDR(b) + 0x56 + 2u * (uint32_t) k);
        if (v > best) {
            best = v;
        }
    }
    return best;
}

int32_t figb_stock_max2(game_state *st, int32_t b)
{
    /* @bin 0x004579D0 the largest of the four words at +0x66 (at least 0) */
    if (b <= 0) {
        return 0;
    }
    if (!stocked(st, b)) {
        return 0;
    }
    uint32_t ba = BUILDING_ADDR(b);
    int32_t best = 0;
    for (uint32_t i = 0; i < 4; i++) {
        int32_t v = S16(st, ba + 0x66 + 2u * i);
        if (v > best) {
            best = v;
        }
    }
    return best;
}

int32_t figb_look_for_target(game_state *st, int32_t id)
{
    /* @bin 0x00497CF0 every 40 ticks, a target in range (0x00499B50, property +0x18) starts an
     * attack (action 10) where the figure stands */
    if (S32(st, 0x00D0563Cu) <= 0) {
        return 0;
    }
    uint32_t f = FIGURE_ADDR(id);
    uint8_t a = U8(st, f + 0x38);
    if (a == 5 || a == 4) {
        return 0;
    }
    if (U8(st, f + 0x40) != 0) {
        return 0;
    }
    uint8_t c = (uint8_t) (U8(st, f + 0x80) + 1);
    U8(st, f + 0x80) = c;
    if (c < 0x28) {
        return 0;
    }
    U8(st, f + 0x80) = 0;
    if (GAME_CALL(st, 0x00499B50u, id, S32(st, FIGB_PROPS + 0x18u + 0x38u * U8(st, f + 0x0A))) <= 0) {
        return 0;
    }
    if (S32(st, 0x00BCD5ECu) > 10) {
        return 0;
    }
    U8(st, f + 0x38) = 10;
    U16(st, f + 0x24) = U16(st, f + 0x14);
    U16(st, f + 0x26) = U16(st, f + 0x16);
    return 1;
}

int32_t figb_dock_load(game_state *st, int32_t b)
{
    /* @bin 0x00495840 the figure waiting at building b (+0xB4, action 10, +0x74 > 0, its record at
     * 0x0077AF04 below 12) gives the current figure a destination (0x00445670) to go to (action 9) */
    uint32_t ba = BUILDING_ADDR(b);
    int32_t w = S16(st, ba + 0xB4);
    if (w == 0) {
        return 0;
    }
    uint32_t wf = FIGURE_ADDR(w);
    if (U8(st, wf + 0x38) != 10) {
        return 0;
    }
    if (S16(st, wf + 0x74) <= 0) {
        return 0;
    }
    if (S32(st, 0x0077AF04u + 0x58u * U8(st, wf + 0x7F)) >= 0x0C) {
        return 0;
    }
    uint32_t f = FIGB_F(st);
    int32_t r = GAME_CALL(st, 0x00445670u, S16(st, f + 0x14), S16(st, f + 0x16), U8(st, wf + 0x6E), S16(st, ba + 0x20),
                          U8(st, ba + 0x14));
    if (r == 0) {
        return 0;
    }
    f = FIGB_F(st);
    S16(st, f + 0x5E) = (int16_t) r;
    S16(st, f + 0x36) = 0;
    S16(st, wf + 0x74) -= 4;
    U8(st, f + 0x38) = 9;
    U16(st, f + 0x24) = U16(st, 0x00D3B4E4u);
    U16(st, f + 0x26) = U16(st, 0x00D3B4E0u);
    U8(st, f + 0x0B) = U8(st, CURRENT_CITY(st) + 0x4490); /* 0x00EA5D00 */
    return 1;
}

int32_t figb_dock_unload(game_state *st, int32_t b)
{
    /* @bin 0x00495950 the same with 0x00445AC0 (action 12) while the waiting figure's +0x6F is
     * below 12 */
    uint32_t ba = BUILDING_ADDR(b);
    int32_t w = S16(st, ba + 0xB4);
    if (w == 0) {
        return 0;
    }
    uint32_t wf = FIGURE_ADDR(w);
    if (U8(st, wf + 0x38) != 10) {
        return 0;
    }
    if (U8(st, wf + 0x6F) >= 0x0C) {
        return 0;
    }
    uint32_t f = FIGB_F(st);
    int32_t r = GAME_CALL(st, 0x00445AC0u, S16(st, f + 0x14), S16(st, f + 0x16), U8(st, wf + 0x6E), S16(st, ba + 0x20),
                          U8(st, ba + 0x14));
    if (r == 0) {
        return 0;
    }
    f = FIGB_F(st);
    S16(st, f + 0x5E) = (int16_t) r;
    U8(st, f + 0x38) = 0x0C;
    S16(st, f + 0x36) = 0;
    U16(st, f + 0x24) = U16(st, 0x00D3B4E4u);
    U16(st, f + 0x26) = U16(st, 0x00D3B4E0u);
    U8(st, f + 0x0B) = U8(st, CURRENT_CITY(st) + 0x4494); /* 0x00EA5D04 */
    return 1;
}

/* a type 0x4B / 0xC0 building, alive, with +0x40 > 0 */
static int dock_working(game_state *st, uint32_t ba)
{
    if (U8(st, ba) != 1) {
        return 0;
    }
    int16_t t = S16(st, ba + 0x10);
    if (t != 0x4B && t != 0xC0) {
        return 0;
    }
    return S16(st, ba + 0x40) > 0;
}

int32_t figb_dock_free(game_state *st, int32_t id)
{
    /* @bin 0x00495A40 1 unless the destination building (+0x5E) is a working dock where one of its
     * three figures (+0xAE) is busy (not action 8), or where this figure has waited less than 10
     * ticks (+0x7B) */
    uint32_t f = FIGURE_ADDR(id);
    uint32_t ba = BUILDING_ADDR(S16(st, f + 0x5E));
    if (!dock_working(st, ba)) {
        return 1;
    }
    for (uint32_t i = 0; i < 3; i++) {
        int16_t o = S16(st, ba + 0xAE + 2u * i);
        if (o != 0) {
            uint32_t of = FIGURE_ADDR(o);
            if (U8(st, of + 0x0E) == 1 && U8(st, of + 0x38) != 8) {
                return 0;
            }
        }
    }
    uint8_t c = (uint8_t) (U8(st, f + 0x7B) + 1);
    U8(st, f + 0x7B) = c;
    if (c < 10) {
        return 0;
    }
    U8(st, f + 0x7B) = 0x0B;
    return 1;
}

int32_t figb_dock_lost(game_state *st, int32_t id)
{
    /* @bin 0x00495AF0 the destination (+0x5E) is not a working dock holding this figure at +0xB4 */
    uint32_t ba = BUILDING_ADDR(S16(st, FIGURE_ADDR(id) + 0x5E));
    if (!dock_working(st, ba)) {
        return 1;
    }
    return S16(st, ba + 0xB4) != id;
}

/* @bin 0x00499E04 target priority by figure type (0: not a target) */
static int32_t target_priority(game_state *st, uint32_t cf, uint8_t t)
{
    switch (t) {
    case 0x2B: case 0x2C: case 0x2D: case 0x36: case 0x37: case 0x38: case 0x63:
        return 0x32;
    case 0x52: case 0x53: case 0x54: case 0x66: case 0x67: case 0x68: case 0x6D:
        return 10;
    case 0x5C: case 0x65:
        return U8(st, cf + 0x130) != 0 ? 0x28 : 0x14;
    case 0x5D: case 0x64:
        return 0x1E;
    default:
        return 0;
    }
}

int32_t figb_find_target(game_state *st, int32_t id, int32_t range)
{
    /* @bin 0x00499B50 the live figure of the highest priority nearest within range (Chebyshev
     * distance, in 0x00BCD5EC) that the figure can reach (0x004B4420, unless it is in action 0x15);
     * its tile goes to 0x00D3B4E4 / 0x00D3B4E0 */
    uint32_t f = FIGURE_ADDR(id);
    int32_t x = S16(st, f + 0x14), y = S16(st, f + 0x16);
    int32_t form = S16(st, f + 0x60);
    int32_t best = 0, best_prio = 0;
    S32(st, 0x00BCD5ECu) = range;
    for (int32_t c = 1; c < FIGURE_COUNT; c++) {
        uint32_t cf = FIGURE_ADDR(c);
        if (U8(st, cf + 0x0E) != 1 || U8(st, cf + 0x38) == 4) {
            continue;
        }
        uint8_t t = U8(st, cf + 0x0A);
        if (t != 0x5C && t != 0x65 && t != 0x5D && t != 0x64) {
            /* @bin 0x00499BD4 a formation with a target (+0x8C of its record) only takes that one */
            int32_t want = S32(st, 0x00BCBA54u + 0x90u * (uint32_t) form);
            if (want > 0 && S16(st, cf + 0x60) != want) {
                continue;
            }
        }
        int32_t prio = target_priority(st, cf, t);
        if (prio == 0) {
            continue;
        }
        int32_t cx = S16(st, cf + 0x14), cy = S16(st, cf + 0x16);
        int32_t dx = x > cx ? x - cx : (x < cx ? cx - x : 0);
        S32(st, 0x00D3B4FCu) = dx;
        int32_t dy = y > cy ? y - cy : (y < cy ? cy - y : 0);
        S32(st, 0x00D3B4F8u) = dy;
        int32_t dist = dx < dy ? dy : dx;
        if (prio > best_prio && dist < S32(st, 0x00BCD5ECu)) {
            if (U8(st, f + 0x38) == 0x15 || GAME_CALL(st, 0x004B4420u, x, y, cx, cy) != 0) {
                best = c;
                S32(st, 0x00BCD5ECu) = dist;
                best_prio = prio;
            }
        }
    }
    if (best == 0) {
        return 0;
    }
    uint32_t bf = FIGURE_ADDR(best);
    S32(st, 0x00D3B4E0u) = S16(st, bf + 0x16);
    S32(st, 0x00D3B4E4u) = S16(st, bf + 0x14);
    return best;
}

void figb_remove(game_state *st, int32_t id)
{
    /* @bin 0x004BD250 the figure is removed (state 2) */
    U8(st, FIGURE_ADDR(id) + 0x0E) = 2;
}

/* @bin 0x00497A6A the point at `dist` tiles from the building in its direction +0x34 (0, 2, 4, 6:
 * north, east, south, west), on the map */
static void point_ahead(game_state *st, uint32_t ba, uint8_t dir, int32_t dist, int32_t *x, int32_t *y)
{
    *x = S16(st, ba + 0x06);
    *y = S16(st, ba + 0x08);
    if (dir == 0) {
        *y -= dist;
    } else if (dir == 2) {
        *x += dist;
    } else if (dir == 4) {
        *y += dist;
    } else if (dir == 6) {
        *x -= dist;
    }
    if (*x < 0) {
        *x = 0;
    }
    if (*y < 0) {
        *y = 0;
    }
    if (*x >= S32(st, 0x00BCA2D0u)) {
        *x = S32(st, 0x00BCA2D0u) - 1;
    }
    if (*y >= S32(st, 0x00BCA2D4u)) {
        *y = S32(st, 0x00BCA2D4u) - 1;
    }
}

static void turn(game_state *st, uint32_t ba)
{
    uint8_t d = (uint8_t) (U8(st, ba + 0x34) + 2);
    U8(st, ba + 0x34) = d;
    if (d > 6) {
        U8(st, ba + 0x34) = 0;
    }
}

int32_t figb_water_point(game_state *st, int32_t id)
{
    /* @bin 0x00497A30 a water tile (0x00480060) 8 tiles ahead of the building, else 3 tiles ahead
     * turning a quarter each try; the building's direction turns */
    uint32_t ba = BUILDING_ADDR(S16(st, FIGURE_ADDR(id) + 0x5A));
    int32_t x, y;
    point_ahead(st, ba, U8(st, ba + 0x34), 8, &x, &y);
    if (GAME_CALL(st, 0x00480060u, x, y, 1, 6) != 0) {
        turn(st, ba);
        return 1;
    }
    for (int i = 0; i < 4; i++) {
        uint8_t d = U8(st, ba + 0x34);
        turn(st, ba);
        point_ahead(st, ba, d, 3, &x, &y);
        if (GAME_CALL(st, 0x00480060u, x, y, 1, 6) != 0) {
            return 1;
        }
    }
    return 0;
}

#define MAP_W 0x00BCA2D0u
#define MAP_H 0x00BCA2D4u
#define MAP_ORIGIN 0x00BCA2D8u
#define GRID_CURSOR 0x00BC9768u /* the grid offset the ring searches are at */
#define GRID_FIGURE 0x00B350DCu /* u16 per tile: the tile's first figure */
#define GRID_TERRAIN 0x00AF598Cu

/* @bin 0x004EBD1A a figure of one of the types, not `except`, in action `action`, on the tile */
static int32_t ring_figure(game_state *st, int32_t ntypes, uint32_t types, int32_t except, uint32_t action)
{
    for (int32_t k = 0; k < ntypes; k++) {
        int32_t f = S16(st, GRID_FIGURE + 2u * U32(st, GRID_CURSOR));
        uint32_t want = U32(st, types + 4u * (uint32_t) k);
        while (f != 0) {
            uint32_t fa = FIGURE_ADDR(f);
            if ((uint32_t) U8(st, fa + 0x0A) == want && except != f && (uint32_t) U8(st, fa + 0x38) == action) {
                return f;
            }
            f = S16(st, fa + 0x08);
        }
    }
    return 0;
}

static int inside(game_state *st, int32_t x, int32_t y)
{
    return x >= 0 && y >= 0 && x < S32(st, MAP_W) && y < S32(st, MAP_H);
}

int32_t figb_ring_search(game_state *st, int32_t x0, int32_t y0, int32_t ntypes, uint32_t types, uint32_t mask,
                         int32_t *x, int32_t *y, int32_t radius, int32_t except, uint32_t action)
{
    /* @bin 0x004EBC90 as 0x004EB800 (square rings, edges +x, +y, -x, -y), for figures in a given
     * action; the terrain mask is only tested on the +x edges. After the +y edge the original
     * decrements the global 0x00D3B4E0 instead of *y, so *y drifts by one per ring. */
    uint32_t base = U32(st, MAP_ORIGIN) + 0xE4u * (uint32_t) y0 + (uint32_t) x0;
    int32_t side = 1, f;
    *x = x0;
    *y = y0;
    for (int32_t ring = 0;;) {
        U32(st, GRID_CURSOR) = base;
        for (int32_t i = 0; i < side; i++) { /* +x */
            if (inside(st, *x, *y)) {
                if ((f = ring_figure(st, ntypes, types, except, action)) != 0) {
                    return f;
                }
                uint32_t t = U32(st, GRID_TERRAIN + 4u * U32(st, GRID_CURSOR));
                if (!(t & 0x80000u) && (mask & t)) {
                    return ntypes + 1;
                }
            }
            U32(st, GRID_CURSOR) += 1;
            *x += 1;
        }
        U32(st, GRID_CURSOR) -= 1;
        *x -= 1;
        for (int32_t i = 0; i < side; i++) { /* +y */
            if (inside(st, *x, *y) && (f = ring_figure(st, ntypes, types, except, action)) != 0) {
                return f;
            }
            U32(st, GRID_CURSOR) += 0xE4;
            *y += 1;
        }
        U32(st, GRID_CURSOR) -= 0xE4;
        S32(st, 0x00D3B4E0u) -= 1; /* @bin 0x004EBEB2 (sic) */
        for (int32_t i = 0; i < side; i++) { /* -x */
            if (inside(st, *x, *y) && (f = ring_figure(st, ntypes, types, except, action)) != 0) {
                return f;
            }
            U32(st, GRID_CURSOR) -= 1;
            *x -= 1;
        }
        U32(st, GRID_CURSOR) += 1;
        *x += 1;
        for (int32_t i = 0; i < side; i++) { /* -y */
            if (inside(st, *x, *y) && (f = ring_figure(st, ntypes, types, except, action)) != 0) {
                return f;
            }
            U32(st, GRID_CURSOR) -= 0xE4;
            *y -= 1;
        }
        U32(st, GRID_CURSOR) += 0xE4;
        *y += 1;
        if (radius == 0) {
            return 0;
        }
        ring++;
        side += 2;
        base -= 0xE5u;
        *x -= 1;
        *y -= 1;
        radius--;
        if (ring >= 0xE4) {
            return 0;
        }
    }
}

int32_t figb_flee_tile(game_state *st, int32_t x, int32_t y, int32_t fx, int32_t fy, int32_t *ox, int32_t *oy)
{
    /* @bin 0x004EABF0 a free tile next to (x, y), trying the directions from the one of 0x0051CD80
     * (from (fx, fy) to (x, y)) round: no building (0x00AC2D4C), no terrain 0x80000, passable
     * (0x004EC410), reachable (0x005181C0). The original's tables disagree on the x side of the
     * diagonals (grid offsets vs coordinates); kept as they are. */
    static const int32_t grid[8] = {-0xE4, -0xE3, 1, 0xE5, 0xE4, 0xE3, -1, -0xE5}; /* @bin 0x004EABFF */
    static const int32_t dx[8] = {0, -1, -1, -1, 0, 1, 1, 1};
    static const int32_t dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    int32_t i = (uint8_t) GAME_CALL(st, 0x0051CD80u, fx, fy, x, y);
    int32_t end = i - 1;
    if (end < 0) {
        end += 8;
    }
    uint32_t base = U32(st, MAP_ORIGIN) + 0xE4u * (uint32_t) y + (uint32_t) x;
    while (i != end) {
        uint32_t g = base + (uint32_t) grid[i];
        if (U8(st, 0x00AC2D4Cu + g) == 0) {
            uint32_t t = U32(st, GRID_TERRAIN + 4u * g);
            if (!(t & 0x80000u) && (int8_t) GAME_CALL(st, 0x004EC410u, (int32_t) t, 1) != 0) {
                *ox = dx[i] + x;
                *oy = dy[i] + y;
                if (GAME_CALL(st, 0x005181C0u, x, y, *ox, *oy) != 0) {
                    return 1;
                }
            }
        }
        i++;
        if (i >= 8) {
            i -= 8;
        }
    }
    return 0;
}

int32_t figb_animal_flee(game_state *st, int32_t id)
{
    /* @bin 0x004EB000 an animal (0x004EA9A0) looks around: a type 0x5E4DF0 figure in action 0x10
     * nearby -> it flees too (0x10, near that one's destination) or starts (0xF); hunters
     * (0x005E4DD0 types, map_search_rings) -> it flees (0x10 by 0x004EABF0) or keeps watching
     * (0xE, 0xF, back to 8 after 20 ticks). The CRT rand is reseeded with timeGetTime(), the wall
     * clock (core/sim/platform.h). */
    uint32_t f = FIGURE_ADDR(id);
    if ((int8_t) GAME_CALL(st, 0x004EA9A0u, U8(st, f + 0x0A)) == 0) {
        return 0;
    }
    if (U8(st, f + 0x0E) == 0) {
        return 0;
    }
    uint8_t a = U8(st, f + 0x38);
    if (a == 0x0C || a == 4) {
        return 0;
    }
    if (a == 0x10 || a == 0x11) {
        return 1;
    }
    crt_srand(st, platform_clock_ms(st)); /* @bin 0x004EB063 srand(timeGetTime()) */
    int32_t ox, oy;
    int32_t r = figb_ring_search(st, S16(st, f + 0x14), S16(st, f + 0x16), 3, 0x005E4DF0u, 0, &ox, &oy, 1, id, 0x10);
    if (r != 0) {
        GAME_CALL(st, 0x004B3250u, id);
        a = U8(st, f + 0x38);
        if (a != 0x0E && a != 0x0F) {
            U8(st, f + 0x38) = 0x0F;
            U8(st, f + 0x92) = 0;
            return 1;
        }
        U8(st, f + 0x38) = 0x10;
        uint32_t rf = FIGURE_ADDR(r);
        random_next_state(st);
        U16(st, f + 0x24) = (uint16_t) (U16(st, rf + 0x24) + (U32(st, 0x00D3B384u) & 3) - 2);
        random_next_state(st);
        U16(st, f + 0x26) = (uint16_t) (U16(st, rf + 0x26) + (U32(st, 0x00D3B384u) & 3) - 2);
        return 1;
    }
    if (U8(st, f + 0x38) == 0x0E) {
        r = map_search_rings(st, S16(st, f + 0x14), S16(st, f + 0x16), 4, 0x005E4DD0u, 0, &ox, &oy, 2, id);
        if (r != 0) {
            random_next_state(st);
            if ((U32(st, 0x00D3B384u) & 3) == 0) {
                uint32_t rf = FIGURE_ADDR(r);
                int32_t tx, ty;
                if ((int8_t) figb_flee_tile(st, S16(st, f + 0x14), S16(st, f + 0x16), S16(st, rf + 0x14),
                                            S16(st, rf + 0x16), &tx, &ty) != 0) {
                    U16(st, f + 0x24) = (uint16_t) tx;
                    U16(st, f + 0x26) = (uint16_t) ty;
                    U8(st, f + 0x38) = 0x10;
                    return 1;
                }
            }
        }
        a = U8(st, f + 0x38);
        if (a == 0x0E || a == 0x0F) {
            uint8_t c = (uint8_t) (U8(st, f + 0x92) + 1);
            U8(st, f + 0x92) = c;
            if (c > 0x14) {
                GAME_CALL(st, 0x004B3250u, id);
                U8(st, f + 0x38) = 8;
            }
        }
        return 0;
    }
    r = figb_ring_search(st, S16(st, f + 0x14), S16(st, f + 0x16), 3, 0x005E4DF0u, 0, &ox, &oy, 3, id, 0x0E);
    if (r != 0) {
        U8(st, f + 0x38) = 0x0F;
        U8(st, f + 0x92) = 0;
        return 1;
    }
    r = map_search_rings(st, S16(st, f + 0x14), S16(st, f + 0x16), 4, 0x005E4DD0u, 0, &ox, &oy, 6, id);
    if (r != 0) {
        U8(st, f + 0x38) = 0x0E;
        U8(st, f + 0x92) = 0;
        return 1;
    }
    return 0;
}

int32_t figb_find_target_building(game_state *st, int32_t id)
{
    /* @bin 0x00492650 the buildings that pass 0x00459FF0 (and 0x00459EC0 only with city money at
     * +0x4450, 0x00459EF0 only with +0x0C) with +0xB8 > 0 are listed (count 0x0096E6B0, tiles from
     * 0x0096E6B4, ids from 0x00972534); the route search 0x004B28F0 picks one */
    uint32_t c = CURRENT_CITY(st);
    int has_a = S32(st, c + 0x4450) > 0;
    S32(st, 0x0096E6B0u) = 0;
    int has_b = S32(st, c + 0x0C) > 0;
    if (!has_a && !has_b) {
        return 0;
    }
    for (int32_t b = 1; b < BUILDING_COUNT; b++) {
        uint32_t ba = BUILDING_ADDR(b);
        if (U8(st, ba) == 0) {
            continue;
        }
        if (GAME_CALL(st, 0x00459FF0u, b) == 0) {
            continue;
        }
        if (GAME_CALL(st, 0x00459EC0u, b) != 0 && !has_a) {
            continue;
        }
        if (GAME_CALL(st, 0x00459EF0u, b) != 0 && !has_b) {
            continue;
        }
        if (S32(st, ba + 0xB8) <= 0) {
            continue;
        }
        int32_t n = S32(st, 0x0096E6B0u);
        S32(st, 0x00972534u + 4u * (uint32_t) n) = b;
        n++;
        S32(st, 0x0096E6B0u) = n;
        S32(st, 0x0096E6B0u + 4u * (uint32_t) n) =
            (int32_t) (U32(st, 0x00BCA2D8u) + (uint32_t) S16(st, ba + 0x26) + 228u * (uint32_t) S16(st, ba + 0x28));
    }
    int32_t n = S32(st, 0x0096E6B0u);
    if (n == 0) {
        return 0;
    }
    int32_t r = GAME_CALL(st, 0x004B28F0u, id, 0x0096E6B4, n);
    if (r <= 0) {
        return 0;
    }
    return S32(st, 0x00972530u + 4u * (uint32_t) r);
}

int32_t figb_find_monument(game_state *st, int32_t id)
{
    /* @bin 0x00492D20 the monuments (types 0xDE, 0xB7, 0xE5, 0xEA, 0xEB, 0xEC; the main part, +0x12
     * = 0) that are finished (+0x16 = -1), or any of them when one of the counters at 0x00784E78
     * picked by 0x00492F9C is positive, are listed with their tile; the route search 0x004B28F0
     * picks one. The tile of a monument whose helper writes nothing is the previous one (the
     * original's stack locals; 0 here for the first). */
    static const uint8_t counted[30] = {1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1,
                                        0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1}; /* @bin 0x00492F9C */
    S32(st, 0x0096E6B0u) = 0;
    GAME_CALL(st, 0x004CC110u, 0x133, S8(st, 0x00E38EA8u));
    int any = 0;
    for (int32_t k = -1; k + 1 < 0x24; k++) {
        if (k >= 0 && k <= 0x1D && counted[k] && S32(st, 0x00784E78u + 4u * (uint32_t) (k + 1)) > 0) {
            any = 1;
            break;
        }
    }
    int32_t x = 0, y = 0;
    for (int32_t b = 1; b <= S32(st, 0x0078D790u); b++) {
        uint32_t ba = BUILDING_ADDR(b);
        if (U8(st, ba) == 0) {
            continue;
        }
        int16_t t = S16(st, ba + 0x10);
        int done = S16(st, ba + 0x16) == -1;
        int main = S16(st, ba + 0x12) == 0;
        int32_t n = S32(st, 0x0096E6B0u);
        if (t == 0xDE && (done || any)) {
            S32(st, 0x00972534u + 4u * (uint32_t) n) = b;
            figb_tile_4e0cc0(st, b, &x, &y);
        } else if (t == 0xB7 && any) {
            S32(st, 0x00972534u + 4u * (uint32_t) n) = b;
            figb_tile_4f6420(st, b, &x, &y);
        } else if ((t == 0xE5 || t == 0xEA || t == 0xEB || t == 0xEC) && main && (done || any)) {
            S32(st, 0x00972534u + 4u * (uint32_t) n) = b;
            if (t == 0xE5) {
                figb_tile_545a30(st, b, &x, &y);
            } else if (t == 0xEA) {
                figb_tile_558f00(st, b, &x, &y);
            } else if (t == 0xEB) {
                figb_tile_556230(st, b, &x, &y);
            } else {
                figb_tile_54f8a0(st, b, &x, &y);
            }
        } else {
            continue;
        }
        n = S32(st, 0x0096E6B0u);
        S32(st, 0x0096E6B4u + 4u * (uint32_t) n) = (int32_t) (U32(st, 0x00BCA2D8u) + 228u * (uint32_t) y + (uint32_t) x);
        S32(st, 0x0096E6B0u) = n + 1;
    }
    int32_t n = S32(st, 0x0096E6B0u);
    if (n == 0) {
        return 0;
    }
    int32_t r = GAME_CALL(st, 0x004B28F0u, id, 0x0096E6B4, n);
    if (r <= 0) {
        return 0;
    }
    return S32(st, 0x00972530u + 4u * (uint32_t) r);
}

void figb_monument_damage(game_state *st)
{
    /* @bin 0x00492FC0 the counter at 0x00784E78 (those picked by 0x00493090) with the largest weight
     * (0x005D2C48, 8 bytes each) loses one; unless 0x004EA930, every monument of type 0xE5, 0xEA,
     * 0xEB, 0xEC is updated by its handler's 0x005468D0 */
    static const uint8_t counted[30] = {1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1,
                                        0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1}; /* @bin 0x00493090 */
    int32_t best = 0, pick = 0;
    for (int32_t k = 0; k < 0x24; k++) {
        if ((uint32_t) (k - 1) <= 0x1D && counted[k - 1] && S32(st, 0x00784E78u + 4u * (uint32_t) k) != 0) {
            int32_t w = S32(st, 0x005D2C48u + 8u * (uint32_t) k);
            if (w > best) {
                best = w;
                pick = k;
            }
        }
    }
    if (pick > 0) {
        S32(st, 0x00784E78u + 4u * (uint32_t) pick) -= 1;
    }
    if ((int8_t) GAME_CALL0(st, 0x004EA930u) != 0) {
        return;
    }
    for (int32_t b = 1; b < S32(st, 0x0078D790u); b++) {
        int16_t t = S16(st, BUILDING_ADDR(b) + 0x10);
        uint32_t obj = t == 0xE5 ? 0x006F9320u : t == 0xEA ? 0x006F5648u : t == 0xEB ? 0x006DE9E0u : t == 0xEC ? 0x006E9188u : 0;
        if (obj) {
            GAME_CALLT(st, obj, 0x005468D0u, b);
        }
    }
}

void figb_deliver(game_state *st, int32_t id, int32_t b, int32_t dest)
{
    /* @bin 0x00496060 for each of the current figure's four goods (+0x9C: 0..7 -> 0x00495CF0,
     * 8..11 -> 0x00496010), then the destination is appended to the list at +0xAC (+0x93 entries)
     * and the next leg chosen by 0x00495F00 (some goods 0..7) or 0x00495E10 */
    int any = 0;
    for (int32_t i = 0; i < 4; i++) {
        uint32_t g = U32(st, FIGB_F(st) + 0x9C + 4u * (uint32_t) i);
        if (g <= 7) {
            GAME_CALL(st, 0x00495CF0u, id, b, dest, i);
            any = 1;
        } else if (g <= 0x0B) {
            GAME_CALL(st, 0x00496010u, id, b, dest, i);
        }
    }
    uint32_t f = FIGURE_ADDR(id);
    S32(st, f + 0xAC + 4u * U8(st, f + 0x93)) = dest;
    U8(st, f + 0x93) += 1;
    if (any) {
        GAME_CALL(st, 0x00495F00u, id, b, dest);
    } else {
        GAME_CALL(st, 0x00495E10u, id, b, dest);
    }
}

/* @bin 0x00495DA0 the index of the largest want / 100 above 0 (the first one on ties), else -1 */
static int32_t largest_want(const int32_t *want)
{
    int32_t best = 0, k = -1;
    for (int32_t i = 0; i < 4; i++) {
        int32_t v = want[i] / 100;
        if (v > best) {
            best = v;
            k = i;
        }
    }
    return k;
}

int32_t figb_go_home_or_die(game_state *st, int32_t id)
{
    /* @bin 0x00495DE0 with something left at +0xBC (0x00495D70) back to the source tile (0x00495D30,
     * action 9), else removed */
    uint32_t f = FIGURE_ADDR(id);
    int any = 0;
    for (uint32_t i = 0; i < 4; i++) {
        if (S32(st, f + 0xBC + 4u * i) != 0) { /* @bin 0x00495D70 */
            any = 1;
            break;
        }
    }
    if (any) {
        U8(st, f + 0x38) = 9; /* @bin 0x00495D30 */
        U16(st, f + 0x24) = U16(st, f + 0x2C);
        U16(st, f + 0x26) = U16(st, f + 0x2E);
        GAME_CALL(st, 0x004B3250u, id);
        return 1;
    }
    U8(st, f + 0x0E) = 2;
    return 0;
}

int32_t figb_go_to_building(game_state *st, int32_t id, int32_t b)
{
    /* @bin 0x00495C10 action 8 towards building b's road tile (0x00480180 with its size, then 3), or
     * back to the source tile (action 9) */
    uint32_t f = FIGURE_ADDR(id);
    uint32_t ba = BUILDING_ADDR(b);
    U8(st, f + 0x38) = 8;
    S16(st, f + 0x5E) = (int16_t) b;
    U8(st, f + 0x11) = 0;
    if (GAME_CALL(st, 0x00480180u, S16(st, ba + 0x06), S16(st, ba + 0x08), U8(st, ba + 0x03)) == 0 &&
        GAME_CALL(st, 0x00480180u, S16(st, ba + 0x06), S16(st, ba + 0x08), 3) == 0) {
        U8(st, f + 0x38) = 9;
        U16(st, f + 0x24) = U16(st, f + 0x2C);
        U16(st, f + 0x26) = U16(st, f + 0x2E);
        return 0;
    }
    U16(st, f + 0x24) = U16(st, 0x00D3B4E4u);
    U16(st, f + 0x26) = U16(st, 0x00D3B4E0u);
    return 0;
}

/* @bin 0x00495E10, 0x00495F00 the next stop: the goods (+0x9C, -1 none) wanted most at building
 * b (`base` minus its stock +0x56 and the amount +0xBC) go to the building listed for them at
 * 0x0089B6D8, if any; 0x00456760 first. At most 3 stops (+0x93). */
static void next_stop(game_state *st, int32_t id, int32_t b, int32_t dest, int split)
{
    uint32_t f = FIGURE_ADDR(id);
    if (U8(st, f + 0x93) < 3) {
        int32_t want[4];
        for (uint32_t i = 0; i < 4; i++) {
            int32_t g = S32(st, f + 0x9C + 4u * i);
            want[i] = 0;
            if (g == -1) {
                continue;
            }
            int32_t base = split ? (g == 0 ? 0x320 : 0x258) : 0x190;
            want[i] = base - S16(st, BUILDING_ADDR(b) + 0x56 + 2u * (uint32_t) g) - S32(st, f + 0xBC + 4u * i);
        }
        int32_t k = largest_want(want);
        GAME_CALL(st, 0x00456760u, id, dest);
        while (k != -1) {
            int32_t g = S32(st, f + 0x9C + 4u * (uint32_t) k);
            int32_t to = S32(st, 0x0089B6D8u + 4u * (uint32_t) g);
            if (to > 0) {
                figb_go_to_building(st, id, to);
                return;
            }
            want[k] = 0;
            k = largest_want(want);
        }
    }
    figb_go_home_or_die(st, id);
}

void figb_next_stop(game_state *st, int32_t id, int32_t b, int32_t dest)
{
    /* @bin 0x00495E10 */
    next_stop(st, id, b, dest, 0);
}

void figb_next_stop_split(game_state *st, int32_t id, int32_t b, int32_t dest)
{
    /* @bin 0x00495F00 good 0 wanted up to 800, the others up to 600 */
    next_stop(st, id, b, dest, 1);
}
