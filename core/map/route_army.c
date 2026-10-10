/* Imhotep - the searches of the soldiers and invaders over the routing grids (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "map/route_army.h"

#include <string.h>

#include "map/route_search.h"
#include "map/routing.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define QUEUE_LAST 0xCB0F

static int32_t dist(game_state *st, int32_t t)
{
    return S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) t);
}

static void push(game_state *st, int32_t tile)
{
    int32_t tail = S32(st, G_ROUTE_QUEUE_TAIL);
    S32(st, G_ROUTE_QUEUE + 4u * (uint32_t) tail) = tile;
    tail++;
    S32(st, G_ROUTE_QUEUE_TAIL) = tail > QUEUE_LAST ? 0 : tail;
}

static int32_t queue_at(game_state *st, int32_t i)
{
    return S32(st, G_ROUTE_QUEUE + 4u * (uint32_t) i);
}

int32_t route_tile_not_held(game_state *st, int32_t tile)
{
    /* @bin 0x0051AFA0 */
    int32_t f = S16(st, G_FIGURE_GRID + 2u * (uint32_t) tile);
    if (f <= 0) {
        return 1;
    }
    do {
        uint32_t a = FIGURE_ADDR(f);
        if (U8(st, a + FIG_U0D) == 0 && U8(st, a + FIG_ACTION) == 5) {
            return 0;
        }
        f = S16(st, a + FIG_NEXT_ON_TILE);
    } while (f != 0);
    return 1;
}

static void start_search(game_state *st, int32_t x, int32_t y)
{
    memset(state_ptr(st, G_GRID_ROAD_DISTANCE, 4 * GRID_TILES), 0, 4 * GRID_TILES); /* 0x0040F8F0 */
    int32_t start = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    S32(st, G_ROUTE_START) = start;
    S32(st, G_ROUTE_SEARCHES) += 1;
}

/* @bin 0x0051A5D9 mode 0: neighbour `to` of `cur` (grid value at `to`), cost d + 0, 10 or 40 */
static void weighted_visit(game_state *st, int32_t to, int32_t d)
{
    if (dist(st, to) != 0 || !route_tile_not_held(st, to)) {
        return;
    }
    int8_t v = S8(st, G_EC7630 + (uint32_t) to);
    if (v < 0 || v >= 5) {
        return;
    }
    int32_t cost = v == 3 ? 40 : v == 1 ? 10 : 0;
    S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) to) = cost + d;
    push(st, to);
}

/* @bin 0x0051A7E6 other modes: values 0 and 2, or 1 on the building `mode` */
static void building_visit(game_state *st, int32_t to, int32_t d, int32_t mode)
{
    if (dist(st, to) != 0 || !route_tile_not_held(st, to)) {
        return;
    }
    int8_t v = S8(st, G_EC7630 + (uint32_t) to);
    if (v != 0 && v != 2 && !(v == 1 && S16(st, G_GRID_BUILDING + 2u * (uint32_t) to) == mode)) {
        return;
    }
    S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) to) = d;
    push(st, to);
}

int32_t route_army_distances(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t mode, int32_t limit)
{
    /* @bin 0x0051A420 */
    start_search(st, x, y);
    S32(st, G_ROUTE_SEARCHES_ENEMY) += 1;
    S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) S32(st, G_ROUTE_START)) = 1;
    S32(st, G_ROUTE_QUEUE) = S32(st, G_ROUTE_START);
    S32(st, G_ROUTE_QUEUE_HEAD) = 0;
    S32(st, G_ROUTE_QUEUE_TAIL) = 1;
    int32_t target = S32(st, G_MAP_START) + ty * GRID_SIZE + tx;
    if (mode == 0) {
        int32_t lowest = 0, taken = 0;
        do {
            /* @bin 0x0051A4D1 the queued tile with the smallest distance (the head first; stops at
             * one not above the last smallest), swapped with the head */
            int32_t head = S32(st, G_ROUTE_QUEUE_HEAD), tail = S32(st, G_ROUTE_QUEUE_TAIL);
            int32_t sel = head;
            int32_t best = dist(st, queue_at(st, head));
            if (best > lowest) {
                if (tail > head) {
                    for (int32_t i = head + 1; i < tail; i++) {
                        int32_t v = dist(st, queue_at(st, i));
                        if (v < best) {
                            sel = i;
                            best = v;
                            if (v <= lowest) {
                                break;
                            }
                        }
                    }
                } else {
                    int found = 0;
                    for (int32_t i = head + 1; i <= QUEUE_LAST; i++) {
                        int32_t v = dist(st, queue_at(st, i));
                        if (v < best) {
                            sel = i;
                            best = v;
                            if (v <= lowest) {
                                found = 1;
                                break;
                            }
                        }
                    }
                    if (!found && best > lowest) {
                        for (int32_t i = 0; i < tail; i++) {
                            int32_t v = dist(st, queue_at(st, i));
                            if (v < best) {
                                sel = i;
                                best = v;
                            }
                        }
                    }
                }
                lowest = best;
            }
            int32_t cur = queue_at(st, sel);
            if (sel != S32(st, G_ROUTE_QUEUE_HEAD)) {
                S32(st, G_ROUTE_QUEUE + 4u * (uint32_t) sel) = queue_at(st, S32(st, G_ROUTE_QUEUE_HEAD));
            }
            if (cur == target) {
                break;
            }
            taken++;
            if (taken > limit) {
                break;
            }
            int32_t d = dist(st, cur) + 1;
            weighted_visit(st, cur - GRID_SIZE, d);
            weighted_visit(st, cur + 1, d);
            weighted_visit(st, cur + GRID_SIZE, d);
            weighted_visit(st, cur - 1, d);
            head = S32(st, G_ROUTE_QUEUE_HEAD) + 1;
            S32(st, G_ROUTE_QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
        } while (S32(st, G_ROUTE_QUEUE_HEAD) != S32(st, G_ROUTE_QUEUE_TAIL));
    } else {
        do {
            int32_t cur = queue_at(st, S32(st, G_ROUTE_QUEUE_HEAD));
            if (cur == target) {
                break;
            }
            int32_t d = dist(st, cur) + 1;
            building_visit(st, cur - GRID_SIZE, d, mode);
            building_visit(st, cur + 1, d, mode);
            building_visit(st, cur + GRID_SIZE, d, mode);
            building_visit(st, cur - 1, d, mode);
            int32_t head = S32(st, G_ROUTE_QUEUE_HEAD) + 1;
            S32(st, G_ROUTE_QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
        } while (S32(st, G_ROUTE_QUEUE_HEAD) != S32(st, G_ROUTE_QUEUE_TAIL));
    }
    return dist(st, target) != 0;
}

/* @bin 0x00518276 values >= 0 or -3, but neither 1 nor 3 */
static int walk_ok(int8_t v)
{
    return (v >= 0 || v == -3) && v != 1 && v != 3;
}

int32_t route_walk_distances(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    /* @bin 0x005181C0 */
    start_search(st, x, y);
    S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) S32(st, G_ROUTE_START)) = 1;
    S32(st, G_ROUTE_QUEUE) = S32(st, G_ROUTE_START);
    S32(st, G_ROUTE_QUEUE_HEAD) = 0;
    S32(st, G_ROUTE_QUEUE_TAIL) = 1;
    int32_t target = S32(st, G_MAP_START) + ty * GRID_SIZE + tx;
    do {
        int32_t cur = queue_at(st, S32(st, G_ROUTE_QUEUE_HEAD));
        if (cur == target) {
            break;
        }
        int32_t d = dist(st, cur) + 1;
        static const int32_t step[3] = {-GRID_SIZE, 1, GRID_SIZE};
        for (int k = 0; k < 3; k++) {
            int32_t n = cur + step[k];
            if (dist(st, n) == 0 && walk_ok(S8(st, G_GRID_ROUTING + (uint32_t) n))) {
                S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) n) = d;
                push(st, n);
            }
        }
        /* @bin 0x00518367 to the left, 0x0051AFF0 too */
        int32_t n = cur - 1;
        if (dist(st, n) == 0 && GAME_CALL(st, FN_ROUTE_FREE_TILE, n) != 0 && walk_ok(S8(st, G_GRID_ROUTING + (uint32_t) n))) {
            S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) n) = d;
            push(st, n);
        }
        int32_t head = S32(st, G_ROUTE_QUEUE_HEAD) + 1;
        S32(st, G_ROUTE_QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
    } while (S32(st, G_ROUTE_QUEUE_HEAD) != S32(st, G_ROUTE_QUEUE_TAIL));
    return dist(st, target) != 0;
}

static int32_t run_not_held(game_state *st, const int32_t *a)
{
    return route_tile_not_held(st, a[0]);
}

static int32_t run_army(game_state *st, const int32_t *a)
{
    return route_army_distances(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_walk(game_state *st, const int32_t *a)
{
    return route_walk_distances(st, a[0], a[1], a[2], a[3]);
}

const ported_function bigb_army_route_ports[] = {
    {0x0051AFA0u, "route_tile_not_held", 1, 32, 0, run_not_held, 0},
    {0x0051A420u, "route_army_distances", 6, 8, 0, run_army, 0},
    {0x005181C0u, "route_walk_distances", 4, 8, 0, run_walk, 0},
};
const int bigb_army_route_ports_count = sizeof(bigb_army_route_ports) / sizeof(bigb_army_route_ports[0]);
