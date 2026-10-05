/* Imhotep - the rock, dune and flag 0x20 sub-passes of the terrain images (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_ROCKS_H
#define IMHOTEP_MAP_TERRAIN_ROCKS_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0047CAE0 (x, y, w, h, flag, need) a w x h block whose tiles all have terrain & 0xEEFED77F
 * == flag (and image need - 1 when need != 0); cursor 0x00BC9768 := the first tile */
int32_t terrain_block_free(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t flag, int32_t need);

/* @bin 0x00482650 (x, y, n, d, mask) scan the n x n square grown by d for terrain & mask; the tile
 * it stopped at is left in 0x00D3B4E0 (y), 0x00D3B4E4 (x) */
int32_t terrain_scan_mask(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, uint32_t mask);

/* @bin 0x00475FD0 (tile) a cliff tile's image index, -1 if none */
int32_t terrain_cliff_index(game_state *st, int32_t tile);

void special_rocks_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag); /* @bin 0x004798E0 */
void special_rocks_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1); /* @bin 0x004770F0 */
void rocks_all(game_state *st);                                                         /* @bin 0x00475DC0 */
void dunes_all(game_state *st);                                                         /* @bin 0x00477210 */
void flag_0x20_all(game_state *st);                                                     /* @bin 0x00476530 */

#endif
