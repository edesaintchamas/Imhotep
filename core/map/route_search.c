/* Imhotep - searches over the routing grids.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "map/route_search.h"

#include <string.h>

#include "map/routing.h"
#include "state/ported.h"
#include "state/records.h"

#define MAP_START 0x00BCA2D8u /* grid offset of the map's first tile */
#define QUEUE 0x00F203A0u     /* i32 ring of 0xCB10 tile offsets */
#define QUEUE_SIZE 0xCB10
#define QUEUE_HEAD 0x00EAE00Cu
#define QUEUE_TAIL 0x00EAE008u
#define SEARCH_START 0x00EAE004u
#define VISITS 0x00EAE010u /* u8 per tile: times a -2 tile was taken from the queue */

static void push(game_state *st, int32_t tile)
{
    int32_t tail = S32(st, QUEUE_TAIL);
    S32(st, QUEUE + 4u * (uint32_t) tail) = tile;
    tail++;
    S32(st, QUEUE_TAIL) = tail > QUEUE_SIZE - 1 ? 0 : tail;
}

/* @bin 0x0051CF8C one neighbour: not reached yet and not blocked (-1, -3) -> distance + 1, or + 5
 * through a -2 tile, and queued */
static void visit(game_state *st, int32_t from, int32_t to)
{
    int32_t d = S32(st, ROUTE_DISTANCE + 4u * (uint32_t) to);
    int8_t g = S8(st, ROUTING_ED4140 + (uint32_t) to);
    if (d != 0 || g == -1 || g == -3) {
        return;
    }
    int32_t next = S32(st, ROUTE_DISTANCE + 4u * (uint32_t) from) + 1;
    S32(st, ROUTE_DISTANCE + 4u * (uint32_t) to) = g == -2 ? next + 4 : next;
    push(st, to);
}

int32_t route_distances_ed4140(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0051CE90 */
    memset(state_ptr(st, ROUTE_DISTANCE, 4 * GRID_TILES), 0, 4 * GRID_TILES);
    int32_t start = S32(st, MAP_START) + y * GRID_SIZE + x;
    S32(st, SEARCH_START) = start;
    if (S8(st, ROUTING_ED4140 + (uint32_t) start) == -4) {
        return 0;
    }
    memset(state_ptr(st, VISITS, GRID_TILES), 0, GRID_TILES);
    S32(st, ROUTE_DISTANCE + 4u * (uint32_t) start) = 1;
    S32(st, QUEUE) = start;
    S32(st, QUEUE_HEAD) = 0;
    S32(st, QUEUE_TAIL) = 1;
    for (int32_t steps = 1;; steps++) {
        if (steps > 50000) {
            return 1;
        }
        int32_t head = S32(st, QUEUE_HEAD);
        int32_t t = S32(st, QUEUE + 4u * (uint32_t) head);
        int requeued = 0;
        if (S8(st, ROUTING_ED4140 + (uint32_t) t) == -2) {
            /* @bin 0x0051CF33 a -2 tile goes back to the queue up to four times before expanding */
            int8_t n = S8(st, VISITS + (uint32_t) t);
            U8(st, VISITS + (uint32_t) t) = (uint8_t) (n + 1);
            if (n < 4) {
                push(st, t);
                requeued = 1;
            }
        }
        if (!requeued) {
            visit(st, t, t - GRID_SIZE);
            visit(st, t, t + 1);
            visit(st, t, t + GRID_SIZE);
            visit(st, t, t - 1);
        }
        head = S32(st, QUEUE_HEAD) + 1;
        S32(st, QUEUE_HEAD) = head > QUEUE_SIZE - 1 ? 0 : head;
        if (S32(st, QUEUE_HEAD) == S32(st, QUEUE_TAIL)) {
            return 1;
        }
    }
}

int32_t route_road_reached(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x00481420 */
    int32_t base = S32(st, MAP_START) + y * GRID_SIZE + x;
    uint32_t table = 0x005CC9E0u + 0x60u * (uint32_t) size;
    for (int i = 0; i < 24; i++) {
        int32_t o = S32(st, table + 4u * (uint32_t) i);
        if (o == 0) {
            return 0;
        }
        uint32_t t = (uint32_t) (o + base);
        if ((U32(st, TERRAIN_GRID + 4 * t) & 4) && S32(st, ROUTE_DISTANCE + 4 * t) != 0) {
            return 1;
        }
    }
    return 0;
}

static int32_t run_distances(game_state *st, const int32_t *a)
{
    return route_distances_ed4140(st, a[0], a[1]);
}

static int32_t run_road_reached(game_state *st, const int32_t *a)
{
    return route_road_reached(st, a[0], a[1], a[2]);
}

const ported_function route_search_ports[] = {
    {0x0051CE90u, "route_distances_ed4140", 2, 32, 0, run_distances, 0},
    {0x00481420u, "route_road_reached", 3, 32, 0, run_road_reached, 0},
};
const int route_search_ports_count = sizeof(route_search_ports) / sizeof(route_search_ports[0]);
