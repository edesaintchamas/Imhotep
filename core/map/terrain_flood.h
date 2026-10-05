/* Imhotep - the floodplain sub-pass of the terrain images over any rectangle (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_FLOOD_H
#define IMHOTEP_MAP_TERRAIN_FLOOD_H

#include <stdint.h>

#include "state/state.h"

void floodplain_pass(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1); /* @bin 0x004777C0 */
void floodplain_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag);       /* @bin 0x00479E70 */
int32_t pattern_match_by_water(game_state *st, uint32_t records, int32_t per_block);    /* @bin 0x00484D70 */

#endif
