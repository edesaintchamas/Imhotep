/* Imhotep - searches over the routing grids (distances from a tile, reachability).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_ROUTE_SEARCH_H
#define IMHOTEP_MAP_ROUTE_SEARCH_H

#include <stdint.h>

#include "state/layout.h"
#include "state/state.h"

/* the names users outside core/map still read (docs/naming.md: kept until they are rewritten) */
#define ROUTE_DISTANCE G_GRID_ROAD_DISTANCE
#define TERRAIN_GRID G_GRID_TERRAIN

/* @bin 0x0051CE90 breadth-first distances over grid 0x00ED4140 from tile (x, y); returns 0 when
 * the start tile is blocked (-4), else 1 */
int32_t route_distances_ed4140(game_state *st, int32_t x, int32_t y);

/* @bin 0x00481420 does a road tile (terrain & 4) around a building of `size` at (x, y), listed in
 * the size's offset table 0x005CC9E0, have a distance from the last search */
int32_t route_road_reached(game_state *st, int32_t x, int32_t y, int32_t size);

#endif
