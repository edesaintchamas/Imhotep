/* Imhotep - the road sub-pass of the terrain images over any rectangle (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_ROADS_H
#define IMHOTEP_MAP_TERRAIN_ROADS_H

#include <stdint.h>

#include "state/state.h"

void roads_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1); /* @bin 0x0047B590 */
void roads_all(game_state *st);                                                  /* @bin 0x0047B570 */

void canals_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t dry); /* @bin 0x0047BAE0 */

#endif
