/* Imhotep - searches over the routing grids (distances from a tile, reachability).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_ROUTE_SEARCH_H
#define IMHOTEP_MAP_ROUTE_SEARCH_H

#include <stdint.h>

#include "state/state.h"

#define ROUTE_DISTANCE 0x00EED760u /* i32 per tile: distance + 1 from the search start, 0 = not reached */
#define TERRAIN_GRID 0x00AF598Cu   /* u32 per tile: terrain flags */

/* @bin 0x0051CE90 breadth-first distances over grid 0x00ED4140 from tile (x, y); returns 0 when
 * the start tile is blocked (-4), else 1 */
int32_t route_distances_ed4140(game_state *st, int32_t x, int32_t y);

/* @bin 0x00481420 does a road tile (terrain & 4) around a building of `size` at (x, y), listed in
 * the size's offset table 0x005CC9E0, have a distance from the last search */
int32_t route_road_reached(game_state *st, int32_t x, int32_t y, int32_t size);

#endif
