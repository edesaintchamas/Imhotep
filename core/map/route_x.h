/* Imhotep - the figures' remaining route searches and route builders (package figx).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_ROUTE_X_H
#define IMHOTEP_MAP_ROUTE_X_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0051A9D0 distances from (x, y) over grid 0x00EC7630 until one of the n grid offsets at
 * `list` is taken: its index + 1, else 0. mode 0 by cost (as 0x0051A420, at most `limit` tiles
 * taken), else breadth first over values 0, 2 and 1 on the building `mode` */
int32_t route_army_to_any(game_state *st, int32_t x, int32_t y, uint32_t list, int32_t n, int32_t mode,
                          int32_t limit);

/* @bin 0x0051D160 breadth-first distances from (x, y) over grid 0x00ED4140 (not -1), 8 directions,
 * at most 50000 tiles taken; 0 when the start is blocked, else 1 */
int32_t route_wide_distances(game_state *st, int32_t x, int32_t y);

/* @bin 0x0051D980 the path to (x, y) back along the distances into route slot `slot`; the first
 * `straight` steps only in the direction of (x, y) from (x0, y0); its length (0: none) */
int32_t route_path_straight(game_state *st, int32_t slot, int32_t x0, int32_t y0, int32_t x, int32_t y,
                            int32_t straight);

/* @bin 0x0051DC10 the path to (x, y) back along the distances into route slot `slot`, ties broken
 * towards terrain without flag 0x10000; its length (0: none) */
int32_t route_path_ties(game_state *st, int32_t slot, int32_t x0, int32_t y0, int32_t x, int32_t y,
                        int32_t mode);

/* @bin 0x005190A0 route_walk_free_to_marked with the n tiles in C memory */
int32_t route_walk_free_to_marked_list(game_state *st, int32_t x, int32_t y, const int32_t *list, int32_t n);

#endif
