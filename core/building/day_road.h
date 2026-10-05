/* Imhotep - the buildings' road access by day (day step 0x15).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_DAY_ROAD_H
#define IMHOTEP_BUILDING_DAY_ROAD_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00487D00 the tile (x + 9, y + 6), also stored in ROAD_ACCESS_X / Y */
int32_t map_tile_487d00(game_state *st, int32_t x, int32_t y);
/* @bin 0x004F30B0 the entry tile of a monument part (writes *tile, *x, *y; returns 1) */
int32_t monument_entry_tile(game_state *st, int32_t id, int32_t *tile, int32_t *x, int32_t *y);
/* @bin 0x00457AF0 day step 0x15 */
void day_road_access_457af0(game_state *st);

/* @bin 0x0046F200 day step 0x20: finished monuments (the message path calls state_unported) */
void day_monuments_46f200(game_state *st);

/* @bin 0x00517A40 distances over the routing grid from (sx, sy) until (ex, ey) */
int32_t route_distances_to(game_state *st, int32_t sx, int32_t sy, int32_t ex, int32_t ey);

#endif
