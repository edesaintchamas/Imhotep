/* Imhotep - the water lift drag (package ui): 0x0047EDA0, called by the drag preview 0x0046FEF0.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A water lift (type 7) is dragged from one end to the other: each end is an existing lift
 * (type 0x5A, 0x0047F190) or a free 3 x 3 block (0x0047C880); the canal between them is the
 * cheapest of the 4 x 4 end offsets 0x005DC188 tried with 0x0051C930 (package place). */
#include <string.h>

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#include "command/construction_checks.h"
#include "command/construction_path.h"
#include "state/layout.h"

#define GRID_W 228

static int32_t water_lift_at(game_state *st, int32_t o)
{
    /* @bin 0x0047F190 the tile is the corner of a whole type-0x5A building (2 x 2) */
    int32_t b = S16(st, G_GRID_BUILDING + 2u * (uint32_t) o);
    if (b == 0 || S16(st, BUILDING_ADDR(b) + BLD_TYPE) != 0x5A) {
        return 0;
    }
    for (uint32_t r = 0; r < 2; r++) {
        for (uint32_t i = 0; i < 2; i++) {
            int32_t u = o + S32(st, G_FOOTPRINT_OFFSETS + 0x30u * r + 8u * i);
            if (b != S16(st, G_GRID_BUILDING + 2u * (uint32_t) u)) {
                return 0;
            }
        }
    }
    return 1;
}

static void distances_clear(game_state *st, int32_t keep, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x0047CA50 (keep, x, y, w, h) the distances 0x00EED760 of the block := 0 unless keep;
     * the cursor 0x00BC9768 := its corner */
    if (x < 0 || y < 0 || x + w - 1 >= S32(st, G_MAP_WIDTH) || y + h - 1 >= S32(st, G_MAP_HEIGHT)) {
        return;
    }
    int32_t c = S32(st, G_MAP_START) + y * GRID_W + x;
    S32(st, G_TILE_CURSOR) = c;
    for (int32_t r = 0; r < h; r++) {
        for (int32_t i = 0; i < w; i++) {
            if (keep == 0) {
                int32_t t = S32(st, G_FOOTPRINT_OFFSETS + 8u * (uint32_t) (6 * r + i)) + c;
                S32(st, G_GRID_ROAD_DISTANCE + 4u * (uint32_t) t) = 0;
            }
        }
    }
}

static int32_t lift_end(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0047EE67 2 on an existing lift, 1 where a 3 x 3 block fits, else -1 */
    if (water_lift_at(st, S32(st, G_MAP_START) + y * GRID_W + x - 0xE5) != 0) {
        return 2;
    }
    return GAME_CALL(st, FN_LAND_BLOCK_FITS, x - 1, y - 1, 3, 3, 0xFFFF, 0, 1) != 0 ? 1 : -1;
}

int32_t water_lift_drag(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x0047EDA0 */
    int32_t unit = S32(st, G_MODEL_BUILDING + 0x28u * 8u); /* @bin 0x0047EDA3 the model cost of type 8 (canal, 0x00705EA4 + 8 * 0x28) */
    S32(st, G_PREVIEW_COST) = 0;
    S32(st, G_END1) = 0;
    S32(st, G_END2) = 0;
    /* @bin 0x0047EDD3 terrain and canals restored from their backups (0x0040F860), images 0x00485830(0) */
    memcpy(state_ptr(st, G_GRID_TERRAIN, 0x32C40), state_ptr(st, G_TERRAIN_BACKUP, 0x32C40), 0x32C40);
    memcpy(state_ptr(st, G_CANAL, 0xCB10), state_ptr(st, G_CANAL_BACKUP, 0xCB10), 0xCB10);
    GAME_CALL(st, FN_UNDO_RESTORE_IMAGES, 0);
    int32_t dx = x1 > x2 ? x1 - x2 : (x1 < x2 ? x2 - x1 : 0);
    S32(st, G_DIST_X) = dx;
    int32_t dy = y1 > y2 ? y1 - y2 : (y1 < y2 ? y2 - y1 : 0);
    S32(st, G_DIST_Y) = dy;
    int32_t n = dx >= dy ? dx : dy;
    if (preview != 0 && S32(st, G_CONSTRUCTION_DRAG) == 0) {
        n = 0;
    } else if (n > 0) {
        S32(st, G_END1) = lift_end(st, x1, y1);
    }
    S32(st, G_END2) = lift_end(st, x2, y2);
    int32_t e1 = S32(st, G_END1), e2 = S32(st, G_END2);
    if (e1 == -1 || e2 == -1) {
        return 0;
    }
    if (e1 == 1 && e2 == 1 && n < 3) {
        return 0;
    }
    if (n == 0) {
        S32(st, G_PREVIEW_COST) = S32(st, G_MODEL_BUILDING + 0x28u * 0x5Au); /* @bin 0x0047EF4C one lift */
        return 1;
    }
    if (drag_path_fill(st, preview, x1, y1, 1) == 0) {
        return 0;
    }
    if (S32(st, G_END1) != 0) {
        distances_clear(st, 0, x1 - 1, y1 - 1, 3, 3);
        ghost_footprint(st, 3, x1 - 1, y1 - 1, 0xFFFFFFFFu, 1, 1);
    }
    if (S32(st, G_END2) != 0) {
        distances_clear(st, 0, x2 - 1, y2 - 1, 3, 3);
        ghost_footprint(st, 3, x2 - 1, y2 - 1, 0xFFFFFFFFu, 1, 1);
    }
    /* @bin 0x0047EFDA the cheapest pair of end offsets (0x00BC971C tiles, not 0) */
    int32_t best = 10000, bi = 0, bj = 0;
    for (int32_t i = 0; i < 4; i++) {
        int32_t ix = S32(st, G_ENDS + 8u * (uint32_t) i), iy = S32(st, G_ENDS + 8u * (uint32_t) i + 4);
        for (int32_t j = 0; j < 4; j++) {
            int32_t jx = S32(st, G_ENDS + 8u * (uint32_t) j), jy = S32(st, G_ENDS + 8u * (uint32_t) j + 4);
            int32_t r = GAME_CALL(st, FN_PLACE_PATH_WALK, preview != 0 ? 1 : 2, ix + x1, iy + y1, jx + x2, jy + y2, 2);
            int32_t c = S32(st, G_CONSTRUCTION_COUNT);
            if (r != 0 && c != 0 && c < best) {
                best = c;
                bj = j;
                bi = i;
            }
        }
    }
    if (best == 10000) {
        return 0;
    }
    GAME_CALL(st, FN_PLACE_PATH_WALK, preview != 0, S32(st, G_ENDS + 8u * (uint32_t) bi) + x1,
              S32(st, G_ENDS + 8u * (uint32_t) bi + 4) + y1, S32(st, G_ENDS + 8u * (uint32_t) bj) + x2,
              S32(st, G_ENDS + 8u * (uint32_t) bj + 4) + y2, 1);
    int32_t lift = S32(st, G_MODEL_BUILDING + 0x28u * 0x5Au); /* @bin 0x0047F148 the model cost of type 0x5A */
    if (S32(st, G_END1) == 1) {
        S32(st, G_PREVIEW_COST) += lift;
    }
    if (S32(st, G_END2) == 1) {
        S32(st, G_PREVIEW_COST) += lift;
    }
    if (best != 0) {
        S32(st, G_PREVIEW_COST) += best * unit;
    }
    return 1;
}

static int32_t run_water_lift_at(game_state *st, const int32_t *a)
{
    return water_lift_at(st, a[0]);
}

static int32_t run_distances_clear(game_state *st, const int32_t *a)
{
    distances_clear(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}

static int32_t run_water_lift_drag(game_state *st, const int32_t *a)
{
    return water_lift_drag(st, a[0], a[1], a[2], a[3], a[4]);
}

const ported_function ui_lift_ports[] = {
    {0x0047F190u, "water_lift_at", 1, 32, 0, run_water_lift_at, 0},
    {0x0047CA50u, "distances_clear", 5, 0, 0, run_distances_clear, 0},
    {0x0047EDA0u, "water_lift_drag", 5, 32, 0, run_water_lift_drag, 0},
};
const int ui_lift_ports_count = sizeof(ui_lift_ports) / sizeof(ui_lift_ports[0]);
