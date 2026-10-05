/* Imhotep - the shore marks (terrain 0x4000000) of the water tiles (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_COAST_H
#define IMHOTEP_MAP_TERRAIN_COAST_H

#include <stdint.h>

#include "state/state.h"

int32_t map_area_lacks(game_state *st, int32_t x, int32_t y, int32_t r, uint32_t mask); /* @bin 0x00481770 */
int32_t row_pair_with(game_state *st, int32_t x, int32_t y, uint32_t mask);             /* @bin 0x00487730 */
int32_t column_pair_with(game_state *st, int32_t x, int32_t y, uint32_t mask);          /* @bin 0x00487790 */
void coast_recheck(game_state *st, int32_t x, int32_t y);                               /* @bin 0x00487B70 */
void coast_marks(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1);       /* @bin 0x004877F0 */
int32_t neighbours_with_without(game_state *st, uint32_t mask, uint32_t exclude, int32_t tile); /* @bin 0x004775F0 */

#endif
