/* Imhotep - the daily step of the moving scenario events (package day2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY2_EVENTS_H
#define IMHOTEP_CITY_DAY2_EVENTS_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0044BD20 terrain bits 0x206 clear at (x, y) */
int32_t event_tile_free(game_state *st, int32_t x, int32_t y);
/* @bin 0x0044BE30 (thiscall, record e of type 3) start, or move one of its four points */
int32_t event_move_step(game_state *st, uint32_t e);

/* @bin 0x0044BD50 the tile (x, y) is wiped by the event */
void event_tile_hit(game_state *st, int32_t x, int32_t y);

#endif
