/* Imhotep - the water sub-pass of the terrain images over any rectangle (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_WATER_H
#define IMHOTEP_MAP_TERRAIN_WATER_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00475CC0 (x0, y0, x1, y1, reveal) every water tile of the rectangle */
void water_pass(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t reveal);

/* @bin 0x0047A220 (x, y, flag) the water tiles of the 3x3 square around (x, y) */
void water_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag);

/* @bin 0x0047A1F0 (x, y) the map reveal is at column 0xE4 - x */
int32_t water_reveal_at(game_state *st, int32_t x);

#endif
