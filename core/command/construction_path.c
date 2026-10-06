/* Imhotep - the drag path distances of roads, canals and walls (package ui).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * At the construction start (0x00405C20) roads, canals and water lifts fill the distance grid
 * 0x00EED760 from the start tile (0x0051B680), walls over the wall tiles (0x0051C790): a
 * breadth-first fill with the queue 0x00F203A0 (head 0x00EAE00C, tail 0x00EAE008, 0xCB10 entries)
 * over the routing grid 0x00EE0C50 (signed bytes) and the terrain. The drag then follows the
 * distances. Bridges (0x0051C390, 0x0051C510) and canal / road crossings (0x0051C630) let a path
 * through only across their direction. */
#include "command/construction_path.h"

#include <string.h>

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define MAP_START 0x00BCA2D8u
#define VIEW_ORIENTATION 0x00BCA2E0u
#define TER(o) U32(st, 0x00AF598Cu + 4u * (uint32_t) (o))
#define IMG(o) S32(st, 0x00B7482Cu + 4u * (uint32_t) (o))
#define ROUTE(o) S8(st, 0x00EE0C50u + (uint32_t) (o))
#define DIST(o) S32(st, 0x00EED760u + 4u * (uint32_t) (o))
#define BGRID(o) S16(st, 0x00B4E6FCu + 2u * (uint32_t) (o))
#define BITFIELDS 0x00B285CCu
#define OBJECT_GRID 0x00D05720u
#define QUEUE 0x00F203A0u
#define HEAD 0x00EAE00Cu
#define TAIL 0x00EAE008u
#define START 0x00EAE004u
#define QUEUE_LAST 0xCB0F
#define GRID_W 228

static int crosswise(game_state *st, int along_x)
{
    /* @bin 0x0051C3C7 view orientations 2 and 6 swap the axes */
    int32_t o = S32(st, VIEW_ORIENTATION);
    return (o == 2 || o == 6) ? along_x ^ 1 : along_x;
}

int32_t terrain_0x10000_or_desirable(game_state *st, int32_t o)
{
    /* @bin 0x0047B8E0 0 on terrain 0x10000; 1 when the signed byte 0x00AC2D4C is above 4; 0 when
     * it is not above 0; else terrain bit 13 */
    uint32_t t = TER(o);
    if (t & 0x10000u) {
        return 0;
    }
    int8_t c = S8(st, 0x00AC2D4Cu + (uint32_t) o);
    if (c > 4) {
        return 1;
    }
    if (c <= 0) {
        return 0;
    }
    return (int32_t) ((t >> 13) & 1);
}

int32_t water_both_axes(game_state *st, int32_t o)
{
    /* @bin 0x0047E090 a neighbour with terrain 0x8000040 on the y axis (north or south) and one on
     * the x axis (east or west) */
    const uint32_t m = 0x8000040u;
    if ((TER(o - GRID_W) & m) == 0 && (TER(o + GRID_W) & m) == 0) {
        return 0;
    }
    if ((TER(o + 1) & m) || (TER(o - 1) & m)) {
        return 1;
    }
    return 0;
}

int32_t water_neighbours(game_state *st, int32_t o)
{
    /* @bin 0x0047E0D0 the four neighbours (north, east, south, west: 0x00BC97C0, C8, D0, D8)
     * with terrain 0x8000040, or terrain 8 and a building of type 0x8A or 0x88; their count */
    static const int32_t step[4] = {-GRID_W, 1, GRID_W, -1};
    static const uint32_t flag[4] = {0x00BC97C0u, 0x00BC97C8u, 0x00BC97D0u, 0x00BC97D8u};
    uint32_t v[4];
    for (int i = 0; i < 4; i++) {
        v[i] = TER(o + step[i]) & 0x8000040u;
        S32(st, flag[i]) = (int32_t) v[i];
    }
    for (int i = 0; i < 4; i++) {
        if (TER(o + step[i]) & 8u) {
            int16_t t = S16(st, BUILDING_ADDR(BGRID(o + step[i])) + 0x10);
            if (t == 0x8A || t == 0x88) {
                v[i] = 1;
                S32(st, flag[i]) = 1;
            }
        }
    }
    int32_t n = 0;
    for (int i = 0; i < 4; i++) {
        n += v[i] != 0;
    }
    return n;
}

/* @bin 0x0051C428 (byte table of the jump at 0x0051C3BB) image classes 0..47: 1 along x, 0 along
 * y, -1 none; the same classes for 0x0051C460 */
static int bridge_class(int32_t v)
{
    static const signed char cls[48] = {
        1, 0, -1, -1, -1, -1, 0, 1, 0, 1, -1, -1, -1, -1, -1, 1, 0, 1, 0, 1, 0, 1, 0, -1,
        -1, -1, -1, 0, 1, 0, 1, -1, -1, -1, -1, -1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
    };
    if ((uint32_t) v > 0x2F) {
        return -1;
    }
    return cls[v];
}

int32_t bridge_passable(game_state *st, int32_t o)
{
    /* @bin 0x0051C390 a bridge tile (image - 0x00F56B84, modulo 0x30) lets a path through when
     * the tiles beside it across its direction have no terrain 0x40 */
    int32_t v = IMG(o) - S32(st, 0x00F56B84u);
    if (v >= 0x30) {
        v -= 0x30;
    }
    int c = bridge_class(v);
    if (c < 0) {
        return 0;
    }
    if (crosswise(st, c)) {
        if (U8(st, 0x00AF5990u + 4u * (uint32_t) o) & 0x40) { /* east */
            return 0;
        }
        if (U8(st, 0x00AF5988u + 4u * (uint32_t) o) & 0x40) { /* west */
            return 0;
        }
        return 1;
    }
    if (U8(st, 0x00AF55FCu + 4u * (uint32_t) o) & 0x40) { /* north */
        return 0;
    }
    if (U8(st, 0x00AF5D1Cu + 4u * (uint32_t) o) & 0x40) { /* south */
        return 0;
    }
    return 1;
}

int32_t ramp_passable(game_state *st, int32_t o)
{
    /* @bin 0x0051C510 terrain 0x100: the bridge test; else a tile whose object (0x00D05720) is
     * present with +0x18 clear passes only as a ramp (image - 0x00F56C34: 0x30 / 0x34 along y,
     * 0x32 / 0x36 along x) with no terrain 0x40 and no distance yet on both sides */
    if (TER(o) & 0x100u) {
        return bridge_passable(st, o);
    }
    uint32_t obj = U32(st, OBJECT_GRID + 4u * (uint32_t) o);
    if (obj == 0 || P8(st, obj + 0x18) != 0) {
        return 1;
    }
    int32_t v = IMG(o) - S32(st, 0x00F56C34u);
    int along;
    if (v == 0x34 || v == 0x30) {
        along = 0;
    } else if (v == 0x36 || v == 0x32) {
        along = 1;
    } else {
        return 0;
    }
    if (crosswise(st, along)) {
        if ((U8(st, 0x00AF55FCu + 4u * (uint32_t) o) & 0x40) || DIST(o - GRID_W) > 0) {
            return 0;
        }
        if ((U8(st, 0x00AF5D1Cu + 4u * (uint32_t) o) & 0x40) || DIST(o + GRID_W) > 0) {
            return 0;
        }
        return 1;
    }
    if ((U8(st, 0x00AF5990u + 4u * (uint32_t) o) & 0x40) || DIST(o + 1) > 0) {
        return 0;
    }
    if ((U8(st, 0x00AF5988u + 4u * (uint32_t) o) & 0x40) || DIST(o - 1) > 0) {
        return 0;
    }
    return 1;
}

int32_t crossing_passable(game_state *st, int32_t o)
{
    /* @bin 0x0051C630 a road / canal crossing tile: a building of type 0x8A passes; images of
     * the floodplain group (0x00F56CD4), the canal groups (0x00F56C44 when 0x0047B8E0, else
     * 0x00F56C94) at 0..11 give the direction (0, 2, 8, 10 along x; 1, 3, 9, 11 along y; 4..7
     * none); bitfield 0x80 tiles (image - 0x00F5695C below 3) pass with at most 2 water
     * neighbours on one axis only */
    int32_t b = BGRID(o);
    if (b != 0 && S16(st, BUILDING_ADDR(b) + 0x10) == 0x8A) {
        return 1;
    }
    int32_t img, base;
    if (TER(o) & 0x10000u) {
        img = IMG(o);
        base = S32(st, 0x00F56CD4u);
    } else if (U8(st, BITFIELDS + (uint32_t) o) & 0x80) {
        if (IMG(o) - S32(st, 0x00F5695Cu) >= 3) {
            return 0;
        }
        if (water_neighbours(st, o) > 2) {
            return 0;
        }
        return (uint8_t) water_both_axes(st, o) == 0;
    } else {
        int32_t r = terrain_0x10000_or_desirable(st, o);
        base = S32(st, 0x00F56C44u);
        img = IMG(o);
        if (r == 0) {
            base = S32(st, 0x00F56C94u);
        }
    }
    uint32_t v = (uint32_t) (img - base);
    if (v > 0xB) {
        return 0;
    }
    /* @bin 0x0051C758 jump table */
    int along;
    if (v == 0 || v == 2 || v == 8 || v == 10) {
        along = 1;
    } else if (v == 1 || v == 3 || v == 9 || v == 11) {
        along = 0;
    } else {
        return 0;
    }
    if (crosswise(st, along) == 1) {
        if (TER(o - GRID_W) & 0x100u) {
            return 0;
        }
        if (TER(o + GRID_W) & 0x100u) {
            return 0;
        }
        return 1;
    }
    if (TER(o + 1) & 0x100u) {
        return 0;
    }
    if (TER(o - 1) & 0x100u) {
        return 0;
    }
    return 1;
}

static void enqueue(game_state *st, int32_t o)
{
    int32_t t = S32(st, TAIL);
    S32(st, QUEUE + 4u * (uint32_t) t) = o;
    t++;
    S32(st, TAIL) = t;
    if (t > QUEUE_LAST) {
        S32(st, TAIL) = 0;
    }
}

/* @bin 0x0051B7F3 one neighbour n of the tile at distance d - 1 (route byte r, "against" route
 * value -6 for the north neighbour, -4 for the others) */
static void path_visit(game_state *st, int32_t n, int32_t r_off, int32_t against, int32_t canal, int32_t d)
{
    if (DIST(n) != 0) {
        return;
    }
    int blocked = 0;
    int8_t r;
    if ((TER(n) & 0x100u) && bridge_passable(st, n) != 0) {
        if (canal != 0) {
            goto flags;
        }
        if (bridge_passable(st, n) != 0) {
            goto flags;
        }
        DIST(n) = -1;
        blocked = 1;
        goto flags;
    }
    r = S8(st, 0x00EE0C50u + (uint32_t) r_off);
    if (r == -5) {
        if (canal != 0) {
            blocked = 1;
        } else if (ramp_passable(st, n) == 0) {
            DIST(n) = -1;
            blocked = 1;
        }
    } else if (r == 2 || r == -1) {
        blocked = 1;
    } else if (TER(n) & 0x12040011u) {
        blocked = 1;
    } else if ((TER(n) & 8u) && !(r == against && canal == 1)) {
        blocked = 1;
    }
flags:
    if ((U8(st, 0x00AF598Cu + 4u * (uint32_t) n) & 0x40) && canal != 0 && crossing_passable(st, n) == 0) {
        DIST(n) = -1;
        return;
    }
    if (blocked) {
        return;
    }
    DIST(n) = d;
    if (S8(st, 0x00EE0C50u + (uint32_t) r_off) == -2) {
        DIST(n) = d + 4;
    }
    enqueue(st, n);
}

int32_t drag_path_fill(game_state *st, int32_t unused, int32_t x, int32_t y, int32_t canal)
{
    /* @bin 0x0051B680 (_, x, y, canal) the distances from the start tile over passable tiles
     * (canal: canals and water lifts, else roads); 0 when the start tile itself is blocked */
    (void) unused;
    memset(state_ptr(st, 0x00EED760u, 0x32C40), 0, 0x32C40); /* 0x0040F8F0 */
    int32_t o = S32(st, MAP_START) + y * GRID_W + x;
    S32(st, START) = o;
    uint32_t ter = TER(o);
    uint32_t bridge = ter & 0x100u;
    /* @bin 0x0051B6CB the start tile */
    if ((ter & 0x12040011u) || ROUTE(o) == -1 || (bridge && bridge_passable(st, o) == 0)) {
        if (canal == 0) {
            return 0;
        }
        if (!bridge) {
            if (!(ter & 8u)) {
                return 0;
            }
            int16_t t = S16(st, BUILDING_ADDR(BGRID(o)) + 0x10);
            if (t != 0x5A && t != 0x8A) {
                return 0;
            }
        }
    }
    if (ROUTE(o) == 2) {
        return 0;
    }
    if (bridge && canal == 0 && bridge_passable(st, o) == 0) {
        return 0;
    }
    if (ROUTE(o) == -5) {
        if (canal != 0 || ramp_passable(st, o) == 0) {
            return 0;
        }
    }
    if ((ter & 0x40u) && canal != 0 && crossing_passable(st, o) == 0) {
        return 0;
    }
    S32(st, 0x00EADFD4u) += 1;
    memset(state_ptr(st, 0x00EAE010u, 0xCB10), 0, 0xCB10); /* 0x0040F8F0 */
    DIST(S32(st, START)) = 1;
    S32(st, QUEUE) = S32(st, START);
    S32(st, HEAD) = 0;
    S32(st, TAIL) = 1;
    do {
        int32_t c = S32(st, QUEUE + 4u * (uint32_t) S32(st, HEAD));
        int32_t d = DIST(c) + 1;
        path_visit(st, c - GRID_W, c - GRID_W, -6, canal, d);
        path_visit(st, c + 1, c + 1, -4, canal, d);
        path_visit(st, c + GRID_W, c + GRID_W, -4, canal, d);
        path_visit(st, c - 1, c - 1, -4, canal, d);
        int32_t h = S32(st, HEAD) + 1;
        S32(st, HEAD) = h;
        if (h > QUEUE_LAST) {
            S32(st, HEAD) = 0;
        }
    } while (S32(st, HEAD) != S32(st, TAIL));
    return 1;
}

int32_t wall_path_fill(game_state *st, int32_t unused, int32_t x, int32_t y)
{
    /* @bin 0x0051C790 (_, x, y) the distances from the start tile over tiles whose routing byte
     * is 4 (walls) */
    (void) unused;
    memset(state_ptr(st, 0x00EED760u, 0x32C40), 0, 0x32C40); /* 0x0040F8F0 */
    int32_t o = S32(st, MAP_START) + y * GRID_W + x;
    S32(st, START) = o;
    DIST(o) = 1;
    S32(st, QUEUE) = S32(st, START);
    S32(st, HEAD) = 0;
    S32(st, TAIL) = 1;
    static const int32_t step[4] = {-GRID_W, 1, GRID_W, -1};
    do {
        int32_t c = S32(st, QUEUE + 4u * (uint32_t) S32(st, HEAD));
        int32_t d = DIST(c) + 1;
        for (int i = 0; i < 4; i++) {
            int32_t n = c + step[i];
            if (DIST(n) == 0 && ROUTE(n) == 4) {
                DIST(n) = d;
                enqueue(st, n);
            }
        }
        int32_t h = S32(st, HEAD) + 1;
        S32(st, HEAD) = h;
        if (h > QUEUE_LAST) {
            S32(st, HEAD) = 0;
        }
    } while (S32(st, HEAD) != S32(st, TAIL));
    return 1;
}

static int32_t run_0047b8e0(game_state *st, const int32_t *a)
{
    return terrain_0x10000_or_desirable(st, a[0]);
}

static int32_t run_0047e090(game_state *st, const int32_t *a)
{
    return water_both_axes(st, a[0]);
}

static int32_t run_0047e0d0(game_state *st, const int32_t *a)
{
    return water_neighbours(st, a[0]);
}

static int32_t run_0051c390(game_state *st, const int32_t *a)
{
    return bridge_passable(st, a[0]);
}

static int32_t run_0051c510(game_state *st, const int32_t *a)
{
    return ramp_passable(st, a[0]);
}

static int32_t run_0051c630(game_state *st, const int32_t *a)
{
    return crossing_passable(st, a[0]);
}

static int32_t run_0051b680(game_state *st, const int32_t *a)
{
    return drag_path_fill(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_0051c790(game_state *st, const int32_t *a)
{
    return wall_path_fill(st, a[0], a[1], a[2]);
}

const ported_function ui_path_ports[] = {
    {0x0047B8E0u, "terrain_0x10000_or_desirable", 1, 32, 0, run_0047b8e0, 0},
    {0x0047E090u, "water_both_axes", 1, 8, 0, run_0047e090, 0},
    {0x0047E0D0u, "water_neighbours", 1, 32, 0, run_0047e0d0, 0},
    {0x0051C390u, "bridge_passable", 1, 32, 0, run_0051c390, 0},
    {0x0051C510u, "ramp_passable", 1, 32, 0, run_0051c510, 0},
    {0x0051C630u, "crossing_passable", 1, 32, 0, run_0051c630, 0},
    {0x0051B680u, "drag_path_fill", 4, 32, 0, run_0051b680, 0},
    {0x0051C790u, "wall_path_fill", 3, 32, 0, run_0051c790, 0},
};
const int ui_path_ports_count = sizeof(ui_path_ports) / sizeof(ui_path_ports[0]);
