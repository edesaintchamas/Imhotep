/* Imhotep - the figures' remaining route searches and route builders (package figx).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "map/route_x.h"

#include "figure/actions_a.h"
#include "map/route_army.h"
#include "map/route_path.h"
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

static void set_dist(game_state *st, int32_t t, int32_t d)
{
    S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) t) = d;
}

static int32_t queue_at(game_state *st, int32_t i)
{
    return S32(st, G_ROUTE_QUEUE + 4u * (uint32_t) i);
}

static void push(game_state *st, int32_t tile)
{
    int32_t tail = S32(st, G_ROUTE_QUEUE_TAIL);
    S32(st, G_ROUTE_QUEUE + 4u * (uint32_t) tail) = tile;
    tail++;
    S32(st, G_ROUTE_QUEUE_TAIL) = tail > QUEUE_LAST ? 0 : tail;
}

/* the index + 1 of `tile` in the n offsets at `list` (searched from the last), else 0 */
static int32_t in_list(game_state *st, uint32_t list, int32_t n, int32_t tile)
{
    for (int32_t i = n - 1; i >= 0; i--) {
        if (S32(st, list + 4u * (uint32_t) i) == tile) {
            return i + 1;
        }
    }
    return 0;
}

/* @bin 0x0051AB73 mode 0: neighbour `to` costs d + 0, 10 (value 1) or 40 (value 3) */
static void weighted_visit(game_state *st, int32_t to, int32_t d)
{
    if (dist(st, to) != 0 || !route_tile_not_held(st, to)) {
        return;
    }
    int8_t v = S8(st, G_EC7630 + (uint32_t) to);
    if (v < 0 || v >= 5) {
        return;
    }
    set_dist(st, to, (v == 3 ? 40 : v == 1 ? 10 : 0) + d);
    push(st, to);
}

/* @bin 0x0051ADA7 other modes: values 0 and 2, or 1 on the building `mode` */
static void building_visit(game_state *st, int32_t to, int32_t d, int32_t mode)
{
    if (dist(st, to) != 0 || !route_tile_not_held(st, to)) {
        return;
    }
    int8_t v = S8(st, G_EC7630 + (uint32_t) to);
    if (v != 0 && v != 2 && !(v == 1 && S16(st, G_GRID_BUILDING + 2u * (uint32_t) to) == mode)) {
        return;
    }
    set_dist(st, to, d);
    push(st, to);
}

int32_t route_army_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n, int32_t mode,
                          int32_t limit)
{
    /* @bin 0x0051A9D0 */
    GAME_CALL(st, FN_FILL_BYTES, (int32_t) G_GRID_ROAD_DISTANCE, 0x32C40, 0);
    int32_t start = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x);
    S32(st, G_ROUTE_START) = start;
    S32(st, G_ROUTE_SEARCHES) += 1;
    S32(st, G_ROUTE_SEARCHES_ENEMY) += 1;
    set_dist(st, start, 1);
    S32(st, G_ROUTE_QUEUE) = S32(st, G_ROUTE_START);
    S32(st, G_ROUTE_QUEUE_HEAD) = 0;
    S32(st, G_ROUTE_QUEUE_TAIL) = 1;
    int32_t r;
    if (mode != 0) {
        do {
            int32_t cur = queue_at(st, S32(st, G_ROUTE_QUEUE_HEAD));
            if ((r = in_list(st, list, n, cur)) != 0) {
                return r;
            }
            int32_t d = dist(st, cur) + 1;
            building_visit(st, cur - 228, d, mode);
            building_visit(st, cur + 1, d, mode);
            building_visit(st, cur + 228, d, mode);
            building_visit(st, cur - 1, d, mode);
            int32_t head = S32(st, G_ROUTE_QUEUE_HEAD) + 1;
            S32(st, G_ROUTE_QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
        } while (S32(st, G_ROUTE_QUEUE_HEAD) != S32(st, G_ROUTE_QUEUE_TAIL));
        return 0;
    }
    int32_t lowest = 0, taken = 0;
    do {
        /* @bin 0x0051AA5F the queued tile with the smallest distance (stops at one not above the
         * last smallest), swapped with the head */
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
        if ((r = in_list(st, list, n, cur)) != 0) {
            return r;
        }
        taken++;
        if (taken > limit) {
            return 0;
        }
        int32_t d = dist(st, cur) + 1;
        weighted_visit(st, cur - 228, d);
        weighted_visit(st, cur + 1, d);
        weighted_visit(st, cur + 228, d);
        weighted_visit(st, cur - 1, d);
        head = S32(st, G_ROUTE_QUEUE_HEAD) + 1;
        S32(st, G_ROUTE_QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
    } while (S32(st, G_ROUTE_QUEUE_HEAD) != S32(st, G_ROUTE_QUEUE_TAIL));
    return 0;
}

static void wide_visit(game_state *st, int32_t to, int32_t d)
{
    if (dist(st, to) == 0 && S8(st, G_GRID_ROUTING_WATER + (uint32_t) to) != -1) {
        set_dist(st, to, d);
        push(st, to);
    }
}

int32_t route_wide_distances(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0051D160 */
    GAME_CALL(st, FN_FILL_BYTES, (int32_t) G_GRID_ROAD_DISTANCE, 0x32C40, 0);
    int32_t start = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x);
    S32(st, G_ROUTE_START) = start;
    if (S8(st, G_GRID_ROUTING_WATER + (uint32_t) start) == -1) {
        return 0;
    }
    set_dist(st, start, 1);
    S32(st, G_ROUTE_QUEUE) = S32(st, G_ROUTE_START);
    S32(st, G_ROUTE_QUEUE_HEAD) = 0;
    S32(st, G_ROUTE_QUEUE_TAIL) = 1;
    int32_t taken = 0;
    do {
        /* @bin 0x0051D1D3 at most 50000 tiles */
        if (++taken > 50000) {
            return 1;
        }
        int32_t cur = queue_at(st, S32(st, G_ROUTE_QUEUE_HEAD));
        int32_t d = dist(st, cur) + 1;
        wide_visit(st, cur - 228, d);
        wide_visit(st, cur + 1, d);
        wide_visit(st, cur + 228, d);
        wide_visit(st, cur - 1, d);
        wide_visit(st, cur - 227, d);
        wide_visit(st, cur + 229, d);
        wide_visit(st, cur + 227, d);
        wide_visit(st, cur - 229, d);
        int32_t head = S32(st, G_ROUTE_QUEUE_HEAD) + 1;
        S32(st, G_ROUTE_QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
    } while (S32(st, G_ROUTE_QUEUE_HEAD) != S32(st, G_ROUTE_QUEUE_TAIL));
    return 1;
}

/* @bin 0x00518880 the walking test of 0x005190A0 (as route_a.c's): a free tile (0x0051AFF0) of
 * walking value 0 .. 4 but 3, or -3, or -1 with terrain flag 0x1000 */
static int open_walk_free(game_state *st, int32_t to)
{
    int8_t v = S8(st, G_GRID_ROUTING + (uint32_t) to);
    if (!route_free_tile(st, to)) {
        return 0;
    }
    if (!(v >= 0 || v == -3 || (v == -1 && (U32(st, G_GRID_TERRAIN + 4u * (uint32_t) to) & 0x1000u)))) {
        return 0;
    }
    return v <= 4 && v != 3;
}

/* one neighbour of 0x005190A0: the index + 1 of the marked tile found, else 0 */
static int32_t marked_visit(game_state *st, int32_t to, int32_t d, int west)
{
    if (dist(st, to) > 0 || !open_walk_free(st, to)) {
        return 0;
    }
    int32_t r = -1 - dist(st, to);
    if (!west) {
        set_dist(st, to, d);
    }
    if (r >= 0) {
        return r + 1;
    }
    if (west) {
        set_dist(st, to, d);
    }
    push(st, to);
    return 0;
}

int32_t route_walk_free_to_marked_list(game_state *st, int32_t x, int32_t y, const int32_t *list, int32_t n)
{
    /* @bin 0x005190A0 route_walk_free_to_marked (figure/route_a.c) with the tiles in C memory (the
     * callers' stack arrays) */
    GAME_CALL(st, FN_FILL_BYTES, (int32_t) G_GRID_ROAD_DISTANCE, 0x32C40, 0);
    if (n <= 0) {
        return 0;
    }
    int32_t mark = -1;
    for (int32_t i = 0; i < n; i++) {
        set_dist(st, list[i], mark--);
    }
    int32_t start = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x);
    S32(st, G_ROUTE_START) = start;
    S32(st, G_ROUTE_SEARCHES) += 1;
    set_dist(st, start, 1);
    S32(st, G_ROUTE_QUEUE) = S32(st, G_ROUTE_START);
    S32(st, G_ROUTE_QUEUE_HEAD) = 0;
    S32(st, G_ROUTE_QUEUE_TAIL) = 1;
    do {
        int32_t tile = queue_at(st, S32(st, G_ROUTE_QUEUE_HEAD));
        int32_t d = dist(st, tile) + 1;
        int32_t r;
        if ((r = marked_visit(st, tile - 228, d, 0)) != 0 || (r = marked_visit(st, tile + 1, d, 0)) != 0 ||
            (r = marked_visit(st, tile + 228, d, 0)) != 0 || (r = marked_visit(st, tile - 1, d, 1)) != 0) {
            return r;
        }
        int32_t head = S32(st, G_ROUTE_QUEUE_HEAD) + 1;
        S32(st, G_ROUTE_QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
    } while (S32(st, G_ROUTE_QUEUE_HEAD) != S32(st, G_ROUTE_QUEUE_TAIL));
    return 0;
}

static const int32_t step_x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int32_t step_y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const int32_t step_off[8] = {-228, -227, 1, 229, 228, 227, -1, -229};

static int32_t path_copy(game_state *st, int32_t slot, int32_t n)
{
    for (int32_t i = 0; i < n; i++) {
        U8(st, G_FIGURE_PATHS + 500u * (uint32_t) slot + (uint32_t) i) = U8(st, G_PATH_BUFFER - 1u + (uint32_t) (n - i));
    }
    return n;
}

int32_t route_path_straight(game_state *st, int32_t slot, int32_t x0, int32_t y0, int32_t x, int32_t y,
                            int32_t straight)
{
    /* @bin 0x0051D980 */
    int32_t cur = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x);
    S32(st, G_ROUTE_START) = cur;
    int32_t best = dist(st, cur);
    if (best <= 0) {
        return 0;
    }
    int32_t n = 0, back = -1, cx = x, cy = y;
    while (best > 1) {
        best = dist(st, cur);
        int32_t dir = -1;
        /* @bin 0x0051D9FE the first `straight` steps prefer the direction from the target */
        int32_t prefer = n <= straight ? route_direction(x, y, x0, y0) : route_direction(cx, cy, x0, y0);
        prefer &= 0xFF;
        for (int32_t k = 0; k < 8; k++) {
            if (k == back) {
                continue;
            }
            int32_t v = dist(st, S32(st, G_DIRECTION_OFFSETS + 4u * (uint32_t) k) + cur);
            if (v == 0) {
                continue;
            }
            if (n <= straight) {
                if (v <= best && k == prefer) {
                    best = v;
                    dir = k;
                }
            } else if (v < best || (v == best && (k == prefer || dir == -1))) {
                best = v;
                dir = k;
            }
        }
        if (dir == -1) {
            return 0;
        }
        cur += step_off[dir];
        cx += step_x[dir];
        cy += step_y[dir];
        back = dir + 4;
        if (back >= 8) {
            back -= 8;
        }
        U8(st, G_PATH_BUFFER + (uint32_t) n) = (uint8_t) back;
        n++;
        if (n >= 500) {
            return 0;
        }
    }
    return path_copy(st, slot, n);
}

int32_t route_path_ties(game_state *st, int32_t slot, int32_t x0, int32_t y0, int32_t x, int32_t y,
                        int32_t mode)
{
    /* @bin 0x0051DC10 */
    uint32_t tick = U32(st, G_R_1_7) & 3u;
    int32_t cur = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) y + (uint32_t) x);
    S32(st, G_ROUTE_START) = cur;
    int32_t best = dist(st, cur);
    if (best < 1) {
        return 0;
    }
    int32_t n = 0, back = -1;
    while (best > 1) {
        best = dist(st, cur);
        uint8_t flag = (uint8_t) ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) cur) >> 16) & 1u);
        /* @bin 0x0051DCAB mode 1: ties by the tile's value 0x00ACF85C & 3, else always taken */
        uint32_t local = mode == 1 ? (U8(st, G_GRID_RANDOM + (uint32_t) cur) & 3u) : tick;
        route_direction(x, y, x0, y0); /* @bin 0x0051DCDA its result is not used */
        int32_t dir = -1;
        for (int32_t k = 0; k < 8; k++) {
            if (k == back) {
                continue;
            }
            int32_t to = S32(st, G_DIRECTION_OFFSETS + 4u * (uint32_t) k) + cur;
            int32_t v = dist(st, to);
            uint8_t tflag = (uint8_t) ((U32(st, G_GRID_TERRAIN + 4u * (uint32_t) to) >> 16) & 1u);
            if (v == 0) {
                continue;
            }
            if (v < best || (v == best && ((flag != 0 && tflag == 0) || tick == local))) {
                best = v;
                dir = k;
                flag = tflag;
            }
        }
        if (dir == -1) {
            return 0;
        }
        cur += step_off[dir];
        back = dir + 4;
        if (back >= 8) {
            back -= 8;
        }
        U8(st, G_PATH_BUFFER + (uint32_t) n) = (uint8_t) back;
        n++;
        if (n >= 500) {
            return 0;
        }
    }
    return path_copy(st, slot, n);
}

static int32_t run_army_to_any(game_state *st, const int32_t *a)
{
    return route_army_to_any(st, a[0], a[1], (uint32_t) a[2], a[3], a[4], a[5]);
}

static int32_t run_wide_distances(game_state *st, const int32_t *a)
{
    return route_wide_distances(st, a[0], a[1]);
}

static int32_t run_path_straight(game_state *st, const int32_t *a)
{
    return route_path_straight(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_path_ties(game_state *st, const int32_t *a)
{
    return route_path_ties(st, a[0], a[1], a[2], a[3], a[4], a[5]);
}

const ported_function figx_route_ports[] = {
    {0x0051A9D0u, "route_army_to_any", 6, 32, 0, run_army_to_any, 0},
    {0x0051D160u, "route_wide_distances", 2, 32, 0, run_wide_distances, 0},
    {0x0051D980u, "route_path_straight", 6, 32, 0, run_path_straight, 0},
    {0x0051DC10u, "route_path_ties", 6, 32, 0, run_path_ties, 0},
};
const int figx_route_ports_count = sizeof(figx_route_ports) / sizeof(figx_route_ports[0]);
