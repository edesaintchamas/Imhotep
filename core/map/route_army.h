/* Imhotep - the searches of the soldiers and invaders over the routing grids (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_ROUTE_ARMY_H
#define IMHOTEP_MAP_ROUTE_ARMY_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0051AFA0 no figure on the tile (figure grid 0x00B350DC, chain +0x08) with +0x0D 0 in
 * action 5 */
int32_t route_tile_not_held(game_state *st, int32_t tile);

/* @bin 0x0051A420 distances from (x, y) over grid 0x00EC7630 until (tx, ty) is taken: mode 0 by
 * cost (the queued tile with the smallest distance first; values 1 cost 10 more, 3 cost 40 more,
 * at most `limit` tiles taken), else breadth first over values 0, 2 and 1 when the building grid
 * holds `mode`; tiles held by a fighting figure (0x0051AFA0) are skipped; is (tx, ty) reached */
int32_t route_army_distances(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty, int32_t mode, int32_t limit);

/* @bin 0x005181C0 breadth-first distances from (x, y) over grid 0x00EE0C50 (values >= 0 or -3, but
 * 1 and 3; to the left also 0x0051AFF0) until (tx, ty) is taken; is it reached */
int32_t route_walk_distances(game_state *st, int32_t x, int32_t y, int32_t tx, int32_t ty);

#endif
