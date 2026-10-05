/* Imhotep - the canal sub-pass of the terrain images over a rectangle (package last).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_LAST_TERRAIN_H
#define IMHOTEP_MAP_LAST_TERRAIN_H

#include <stdint.h>

#include "state/state.h"

void canal_tile(game_state *st, int32_t dry);                                            /* @bin 0x0047BE30 */
void canals_rect_redraw(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1); /* @bin 0x0047BD50 */

#endif
