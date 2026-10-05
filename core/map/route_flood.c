/* Imhotep - breadth-first distance floods over the routing grids (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "map/route_flood.h"

#include <string.h>

#include "map/route_search.h"
#include "map/routing.h"
#include "state/ported.h"
#include "state/records.h"

#define MAP_START 0x00BCA2D8u    /* grid offset of the map's first tile */
#define QUEUE 0x00F203A0u        /* i32 ring of tile offsets */
#define QUEUE_LAST 0xCB0F        /* @bin 0x00519AD7 an index above this wraps to 0 */
#define QUEUE_HEAD 0x00EAE00Cu
#define QUEUE_TAIL 0x00EAE008u
#define SEARCH_START 0x00EAE004u
#define SEARCH_COUNT 0x00EADFD4u

/* @bin 0x00519AAA one neighbour `to` of `from`: not reached yet and its grid value in 0..2 (signed
 * byte) -> distance of `from` + 1, queued */
static void visit(game_state *st, int32_t to, int32_t dist)
{
    if (S32(st, ROUTE_DISTANCE + 4u * (uint32_t) to) != 0) {
        return;
    }
    int8_t g = S8(st, ROUTING_EE0C50 + (uint32_t) to);
    if (g < 0 || g > 2) {
        return;
    }
    S32(st, ROUTE_DISTANCE + 4u * (uint32_t) to) = dist;
    int32_t tail = S32(st, QUEUE_TAIL);
    S32(st, QUEUE + 4u * (uint32_t) tail) = to;
    tail++;
    S32(st, QUEUE_TAIL) = tail > QUEUE_LAST ? 0 : tail;
}

void route_flood_ee0c50(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00519A30 */
    memset(state_ptr(st, ROUTE_DISTANCE, 4 * GRID_TILES), 0, 4 * GRID_TILES); /* 0x0040F8F0 */
    int32_t start = S32(st, MAP_START) + y * GRID_SIZE + x;
    S32(st, SEARCH_START) = start;
    S32(st, SEARCH_COUNT) += 1;
    S32(st, ROUTE_DISTANCE + 4u * (uint32_t) start) = 1;
    S32(st, QUEUE) = start;
    S32(st, QUEUE_HEAD) = 0;
    S32(st, QUEUE_TAIL) = 1;
    do {
        int32_t t = S32(st, QUEUE + 4u * (uint32_t) S32(st, QUEUE_HEAD));
        int32_t d = S32(st, ROUTE_DISTANCE + 4u * (uint32_t) t) + 1;
        /* @bin 0x00519AAA up, right, down, left */
        visit(st, t - GRID_SIZE, d);
        visit(st, t + 1, d);
        visit(st, t + GRID_SIZE, d);
        visit(st, t - 1, d);
        int32_t head = S32(st, QUEUE_HEAD) + 1;
        S32(st, QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
    } while (S32(st, QUEUE_HEAD) != S32(st, QUEUE_TAIL));
}

/* @bin 0x0051B0F1 one neighbour over grid 0x00EC7630: not reached yet and its value >= 0 */
static void visit_ec7630(game_state *st, int32_t to, int32_t dist)
{
    if (S32(st, ROUTE_DISTANCE + 4u * (uint32_t) to) != 0 || S8(st, ROUTING_EC7630 + (uint32_t) to) < 0) {
        return;
    }
    S32(st, ROUTE_DISTANCE + 4u * (uint32_t) to) = dist;
    int32_t tail = S32(st, QUEUE_TAIL);
    S32(st, QUEUE + 4u * (uint32_t) tail) = to;
    tail++;
    S32(st, QUEUE_TAIL) = tail > QUEUE_LAST ? 0 : tail;
}

int32_t route_flood_ec7630(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty)
{
    /* @bin 0x0051B040 */
    memset(state_ptr(st, ROUTE_DISTANCE, 4 * GRID_TILES), 0, 4 * GRID_TILES); /* 0x0040F8F0 */
    int32_t start = S32(st, MAP_START) + y * GRID_SIZE + x;
    S32(st, SEARCH_START) = start;
    S32(st, SEARCH_COUNT) += 1;
    S32(st, ROUTE_DISTANCE + 4u * (uint32_t) start) = 1;
    S32(st, QUEUE) = S32(st, SEARCH_START);
    S32(st, QUEUE_HEAD) = 0;
    S32(st, QUEUE_TAIL) = 1;
    int32_t target = S32(st, MAP_START) + ty * GRID_SIZE + tx;
    do {
        int32_t t = S32(st, QUEUE + 4u * (uint32_t) S32(st, QUEUE_HEAD));
        if (t == target) {
            break;
        }
        int32_t d = S32(st, ROUTE_DISTANCE + 4u * (uint32_t) t) + 1;
        visit_ec7630(st, t - GRID_SIZE, d);
        visit_ec7630(st, t + 1, d);
        visit_ec7630(st, t + GRID_SIZE, d);
        visit_ec7630(st, t - 1, d);
        int32_t head = S32(st, QUEUE_HEAD) + 1;
        S32(st, QUEUE_HEAD) = head > QUEUE_LAST ? 0 : head;
    } while (S32(st, QUEUE_HEAD) != S32(st, QUEUE_TAIL));
    return S32(st, ROUTE_DISTANCE + 4u * (uint32_t) target) != 0;
}

static int32_t run_flood_ec7630(game_state *st, const int32_t *a)
{
    return route_flood_ec7630(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_flood_ee0c50(game_state *st, const int32_t *a)
{
    route_flood_ee0c50(st, a[0], a[1]);
    return 0;
}

const ported_function bigb_route_ports[] = {
    {0x00519A30u, "route_flood_ee0c50", 2, 0, 0, run_flood_ee0c50, 0},
    {0x0051B040u, "route_flood_ec7630", 4, 8, 0, run_flood_ec7630, 0},
};
const int bigb_route_ports_count = sizeof(bigb_route_ports) / sizeof(bigb_route_ports[0]);
