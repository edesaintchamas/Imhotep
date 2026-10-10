/* Imhotep - paths between two tiles over the routing grid 0x00ED4140 (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "map/route_path.h"

#include <string.h>

#include "map/route_search.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define QUEUE_LAST 0xCB0F

int32_t route_direction(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x0051CBA0 */
    if (x0 > x1) {
        return y0 > y1 ? 7 : y0 == y1 ? 6 : 5;
    }
    if (x0 == x1) {
        return y0 > y1 ? 0 : y0 < y1 ? 4 : 8;
    }
    return y0 > y1 ? 1 : y0 == y1 ? 2 : 3;
}

/* @bin 0x0051E0A7 the 8 neighbours of a tile (the first 8 of the 24-entry ring the original builds) */
static const int32_t around[8] = {-GRID_SIZE, -GRID_SIZE + 1, 1, GRID_SIZE + 1, GRID_SIZE, GRID_SIZE - 1, -1, -GRID_SIZE - 1};

static void push(game_state *st, int32_t tile)
{
    int32_t tail = S32(st, G_ROUTE_QUEUE_TAIL);
    S32(st, G_ROUTE_QUEUE + 4u * (uint32_t) tail) = tile;
    tail++;
    S32(st, G_ROUTE_QUEUE_TAIL) = tail > QUEUE_LAST ? 0 : tail;
}

static void visit(game_state *st, int32_t c, int32_t d, int32_t end)
{
    if (S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) c) != 0) {
        return;
    }
    int32_t start = S32(st, G_ROUTE_START);
    if (c != start && c != end) {
        uint32_t v = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) c);
        if (S8(st, G_GRID_ROUTING_WATER + (uint32_t) c) == -1 || (v & 0x10000) || (v & 0x40)) {
            return;
        }
        for (int i = 0; i < 8; i++) {
            int32_t n = c + around[i];
            uint32_t w = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) n);
            if (w & 0x40) {
                return;
            }
            if (n == S32(st, G_ROUTE_START) || n == end) {
                break;
            }
            if (S8(st, G_GRID_ROUTING_WATER + (uint32_t) n) == -1 || (w & 0x10000)) {
                return;
            }
        }
    }
    S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) c) = d;
    push(st, c);
}

int32_t route_distances_wide(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x0051DE70 */
    memset(state_ptr(st, G_GRID_ROAD_DISTANCE, 4 * GRID_TILES), 0, 4 * GRID_TILES); /* 0x0040F8F0 */
    int32_t start = S32(st, G_MAP_START) + y0 * GRID_SIZE + x0;
    S32(st, G_ROUTE_START) = start;
    int32_t end = S32(st, G_MAP_START) + y1 * GRID_SIZE + x1;
    if (S8(st, G_GRID_ROUTING_WATER + (uint32_t) start) == -1) {
        return 0;
    }
    S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) start) = 1;
    S32(st, G_ROUTE_QUEUE) = S32(st, G_ROUTE_START);
    S32(st, G_ROUTE_QUEUE_HEAD) = 0;
    S32(st, G_ROUTE_QUEUE_TAIL) = 1;
    for (int32_t n = 1;; n++) {
        if (n > 50000) {
            return 1;
        }
        int32_t cur = S32(st, G_ROUTE_QUEUE + 4u * (uint32_t) S32(st, G_ROUTE_QUEUE_HEAD));
        int32_t d = S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) cur) + 1;
        visit(st, cur - GRID_SIZE, d, end);
        visit(st, cur + 1, d, end);
        visit(st, cur + GRID_SIZE, d, end);
        visit(st, cur - 1, d, end);
        int32_t head = S32(st, G_ROUTE_QUEUE_HEAD) + 1;
        S32(st, G_ROUTE_QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
        if (S32(st, G_ROUTE_QUEUE_HEAD) == S32(st, G_ROUTE_QUEUE_TAIL)) {
            return 1;
        }
    }
}

int32_t route_path(game_state *st, int32_t id, int32_t from_x, int32_t from_y, int32_t x, int32_t y, int32_t mode,
                   int32_t *out)
{
    /* @bin 0x0051E720 */
    uint32_t rnd = U32(st, G_RANDOM_R1_7BIT) & 3;
    int32_t t = S32(st, G_MAP_START) + y * GRID_SIZE + x;
    S32(st, G_ROUTE_START) = t;
    int32_t d = S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) t);
    if (d <= 0) {
        return 0;
    }
    int32_t n = 0, last = -1;
    while (d > 1) {
        d = S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) t);
        uint32_t tie = mode == 1 ? (U8(st, G_GRID_RANDOM + (uint32_t) t) & 3u) : rnd;
        route_direction(x, y, from_x, from_y); /* 0x0051CBA0, its result unused */
        int32_t best = -1;
        for (int32_t dir = 0; dir < 8; dir += 2) {
            if (dir == last) {
                continue;
            }
            int32_t v = S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) (t + S32(st, G_DIRECTION_OFFSETS + 4u * (uint32_t) dir)));
            /* @bin 0x0051E7F1 smaller, or equal when the tie value matches */
            if (v != 0 && (v < d || (v == d && rnd == tie))) {
                d = v;
                best = dir;
            }
        }
        if (best == -1) {
            return 0;
        }
        switch (best) {
        case 0: y--; t -= GRID_SIZE; break;
        case 2: x++; t++; break;
        case 4: y++; t += GRID_SIZE; break;
        case 6: x--; t--; break;
        default: break;
        }
        last = best + 4 >= 8 ? best - 4 : best + 4;
        U8(st, G_PATH_BUFFER + (uint32_t) n) = (uint8_t) last;
        n++;
        if (n >= 500) {
            return 0;
        }
    }
    if (id < 1000) {
        for (int32_t i = 0; i < n; i++) {
            U8(st, G_FIGURE_PATHS + 500u * (uint32_t) id + (uint32_t) i) = U8(st, G_PATH_BUFFER + (uint32_t) (n - 1 - i));
        }
    } else if (out != NULL) {
        for (int32_t i = 0; i < n; i++) {
            out[i] = U8(st, G_PATH_BUFFER + (uint32_t) (n - 1 - i));
        }
    }
    return n;
}

static int32_t run_direction(game_state *st, const int32_t *a)
{
    (void) st;
    return route_direction(a[0], a[1], a[2], a[3]);
}

static int32_t run_distances_wide(game_state *st, const int32_t *a)
{
    return route_distances_wide(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_path(game_state *st, const int32_t *a)
{
    /* the output array is a game address: in the state, or outside it (a caller's stack: not
     * portable) */
    if (a[0] >= 1000 && a[6] != 0) {
        int32_t buf[500];
        int32_t n = route_path(st, a[0], a[1], a[2], a[3], a[4], a[5], buf);
        if (n > 0) {
            uint8_t *p = state_ptr(st, (uint32_t) a[6], 4u * (uint32_t) n);
            if (!p) {
                state_unported(st, FN_ROUTE_PATH);
                return n;
            }
            memcpy(p, buf, 4u * (uint32_t) n); /* little-endian, as the game */
        }
        return n;
    }
    return route_path(st, a[0], a[1], a[2], a[3], a[4], a[5], NULL);
}

const ported_function bigb_path_ports[] = {
    {0x0051CBA0u, "route_direction", 4, 8, 0, run_direction, 0},
    {0x0051DE70u, "route_distances_wide", 4, 32, 0, run_distances_wide, 0},
    {0x0051E720u, "route_path", 7, 32, 0, run_path, 0},
};
const int bigb_path_ports_count = sizeof(bigb_path_ports) / sizeof(bigb_path_ports[0]);
