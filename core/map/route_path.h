/* Imhotep - paths between two tiles over the routing grid 0x00ED4140 (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_ROUTE_PATH_H
#define IMHOTEP_MAP_ROUTE_PATH_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0051CBA0 the direction (0..7, 8 when equal) from (x0, y0) towards (x1, y1) */
int32_t route_direction(int32_t x0, int32_t y0, int32_t x1, int32_t y1);

/* @bin 0x0051DE70 distances (0x00EED760) from (x0, y0) over grid 0x00ED4140, through tiles whose 8
 * neighbours are passable too, (x1, y1) and the start always allowed; 0 when the start is blocked */
int32_t route_distances_wide(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1);

/* @bin 0x0051E720 the path back from (x, y) along decreasing distances: its length (0 when none);
 * the directions stored for figure `id` (0x00C8AD38, 500 bytes each) when id < 1000, else in out[]
 * (when out is not NULL). mode 1: ties broken by grid 0x00ACF85C, else by the random 0x00D3B374 */
int32_t route_path(game_state *st, int32_t id, int32_t from_x, int32_t from_y, int32_t x, int32_t y, int32_t mode,
                   int32_t *out);

#endif
