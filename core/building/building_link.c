/* Imhotep - the link of a building of type 0x88 to its partner (+0xC4) (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32). */
#include "building/building_spawn.h"
#include "map/route_path.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


/* @bin 0x0043F18D marks (on) or clears a tile of the path: terrain bit 0x40, grid 0x00EE0C50 */
static void mark(game_state *st, int32_t t, int on, int32_t kind)
{
    if (on) {
        U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t) |= 0x40u;
        U8(st, G_GRID_ROUTING + (uint32_t) t) = kind == 1 ? 1 : 3;
    } else {
        U8(st, G_GRID_ROUTING + (uint32_t) t) = 0xFF;
        U32(st, G_GRID_TERRAIN + 4u * (uint32_t) t) &= 0xFFFFFFBFu;
    }
}

void building_link_path(game_state *st, int32_t id, int32_t on, int32_t kind)
{
    /* @bin 0x0043F060 the path between the road tiles (0x004806C0) of the building and of its
     * partner +0xC4, through wide passages (0x0051DE70, 0x0051E720), marked (`on`, low byte) or
     * cleared */
    uint32_t b = BUILDING_ADDR(id);
    int32_t other = S32(st, b + BLD_FERRY_PARTNER);
    if (other <= 0) {
        return;
    }
    int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y);
    GAME_CALL(st, FN_TILE_BIT4_PATTERN, x, y, 0);
    int32_t path[1000];
    for (int i = 0; i < 250; i++) {
        path[i] = -1;
    }
    uint32_t o = BUILDING_ADDR(other);
    int32_t ox = S16(st, o + BLD_X), oy = S16(st, o + BLD_Y);
    int32_t size = U8(st, b + BLD_SIZE);
    int32_t keep_x = S32(st, G_ROAD_ACCESS_X), keep_y = S32(st, G_ROAD_ACCESS_Y);
    building_find_road_8(st, x, y, size);
    int32_t fx = S32(st, G_ROAD_ACCESS_X), fy = S32(st, G_ROAD_ACCESS_Y);
    building_find_road_8(st, ox, oy, size); /* @bin 0x0043F116 with the first building's size */
    int32_t tx = S32(st, G_ROAD_ACCESS_X), ty = S32(st, G_ROAD_ACCESS_Y);
    route_distances_wide(st, fx, fy, tx, ty);
    int32_t n = route_path(st, 0x3E9, fx, fy, tx, ty, 0, path);
    S32(st, G_ROAD_ACCESS_X) = keep_x;
    S32(st, G_ROAD_ACCESS_Y) = keep_y;
    if (n == 0) {
        return;
    }
    int on8 = (uint8_t) on != 0;
    int32_t row = S32(st, G_MAP_START) + fy * GRID_SIZE;
    int32_t col = fx;
    mark(st, row + col, on8, kind);
    for (int32_t i = 0; i < n; i++) {
        switch (path[i]) {
        case 0: row -= GRID_SIZE; break;
        case 2: col++; break;
        case 4: row += GRID_SIZE; break;
        default: col--; break;
        }
        mark(st, row + col, on8, kind);
    }
}

void building_release_figures(game_state *st, int32_t id)
{
    /* @bin 0x004A3630 the figures of the building's list (0x00EAAF68 + 0xE0 * +0xCA, methods
     * 0x004F6630 count, 0x004F6620 next, 0x004F6560 clear) go back to their previous action (+0x10) */
    uint32_t list = G_REC88 + 0xE0u * U8(st, BUILDING_ADDR(id) + BLD_REC88_SLOT);
    if (GAME_CALLT0(st, list, FN_QUEUE_LENGTH) == 0) {
        return;
    }
    for (int32_t i = 0; i < GAME_CALLT0(st, list, FN_QUEUE_LENGTH); i++) {
        int32_t f = GAME_CALLT0(st, list, FN_QUEUE_POP);
        uint32_t a = FIGURE_ADDR(f);
        U8(st, a + FIG_ACTION) = U8(st, a + FIG_ACTION_BEFORE_ATTACK);
        GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, f);
    }
    GAME_CALLT0(st, list, FN_QUEUE_CLEAR);
}

void building_release_figures_pair(game_state *st, int32_t id)
{
    /* @bin 0x004A36B0 the building and its partner +0xC4 */
    int32_t other = S32(st, BUILDING_ADDR(id) + BLD_FERRY_PARTNER);
    building_release_figures(st, id);
    if (other > 0) {
        building_release_figures(st, other);
    }
}

static int32_t run_link_path(game_state *st, const int32_t *a)
{
    building_link_path(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_release(game_state *st, const int32_t *a)
{
    building_release_figures(st, a[0]);
    return 0;
}

static int32_t run_release_pair(game_state *st, const int32_t *a)
{
    building_release_figures_pair(st, a[0]);
    return 0;
}

const ported_function bigb_link_ports[] = {
    {0x0043F060u, "building_link_path", 3, 0, 0, run_link_path, 0},
    {0x004A3630u, "building_release_figures", 1, 0, 0, run_release, 0},
    {0x004A36B0u, "building_release_figures_pair", 1, 0, 0, run_release_pair, 0},
};
const int bigb_link_ports_count = sizeof(bigb_link_ports) / sizeof(bigb_link_ports[0]);
