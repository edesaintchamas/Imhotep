/* Imhotep - a building's tiles on the map grids (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_MAP_BUILDING_H
#define IMHOTEP_MAP_MAP_BUILDING_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0047BF50 building `id`'s sx x sy tiles from (x, y): terrain flags (kept & 0x03052E80 |
 * flags), building grid, size bits, image `image`, edge bits (0x40 on the corner the view shows);
 * 0x00BC9734 := 1 when outside the map */
void map_building_tiles(game_state *st, int32_t id, int32_t x, int32_t y, int32_t sx, int32_t sy, int32_t image,
                        uint32_t flags);

#endif
