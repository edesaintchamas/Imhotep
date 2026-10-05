/* Imhotep - small helpers of the daily passes (package day2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY2_MISC_H
#define IMHOTEP_CITY_DAY2_MISC_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00481AA0 entry i of ring `ring` around a size x size footprint at (x, y) lies outside
 * the map square (-1 .. width / height); 1 if outside */
int32_t ring_tile_outside(game_state *st, int32_t x, int32_t y, int32_t size, int32_t ring, int32_t i);

/* @bin 0x0047CBD0 the pattern (0..3, 999 none) of terrain bit 4 on the 2x2 block at (x, y) */
int32_t tile_bit4_pattern(game_state *st, int32_t x, int32_t y);
/* @bin 0x0051CC00 the direction (0..15) from (x1, y1) to (x2, y2) */
int32_t direction_between(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
/* @bin 0x004CF1C0 clear the screen buffer */
void screen_buffer_clear(game_state *st);

/* @bin 0x0047CC70 a 2x2 shore building fits at (x, y): see day2_misc.c */
int32_t shore_fit(game_state *st, int32_t x, int32_t y, int32_t size, int32_t floodable);

int32_t pyramid_ramp_blocked(game_state *st, int32_t b); /* @bin 0x004F6020 */

#endif
