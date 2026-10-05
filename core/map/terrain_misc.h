/* Imhotep - the signpost, plaza and wall sub-passes of the terrain images (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_MISC_H
#define IMHOTEP_MAP_TERRAIN_MISC_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00482810 (x, y, n, d, exclude) the first tile of terrain 0 (other than offset exclude) in
 * the n x n square grown by d: 1, position in 0x00D3B4E0 (y) / 0x00D3B4E4 (x) */
int32_t terrain_find_empty(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, int32_t exclude);

void signposts_update(game_state *st, int32_t clear);                                     /* @bin 0x0047D870 */
int32_t plaza_block_free(game_state *st, int32_t tile);                                    /* @bin 0x00475670 */
void plaza_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t clear); /* @bin 0x00475720 */
void walls_all(game_state *st);                                                            /* @bin 0x0047B1A0 */

#endif
