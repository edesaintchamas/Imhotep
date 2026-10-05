/* Imhotep - breadth-first distance floods over the routing grids (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_ROUTE_FLOOD_H
#define IMHOTEP_MAP_ROUTE_FLOOD_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00519A30 distances (0x00EED760, i32 per tile, 1 at the start) from tile (x, y) over the
 * tiles of grid 0x00EE0C50 whose value is 0..2; counts the search in 0x00EADFD4 */
void route_flood_ee0c50(game_state *st, int32_t x, int32_t y);

/* @bin 0x0051B040 distances from (x, y) over the tiles of grid 0x00EC7630 with a value >= 0, until
 * (tx, ty) is taken from the queue; is (tx, ty) reached */
int32_t route_flood_ec7630(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty);

#endif
