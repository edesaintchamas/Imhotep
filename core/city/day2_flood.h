/* Imhotep - the floodplain tiles under the Nile flood (package day2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY2_FLOOD_H
#define IMHOTEP_CITY_DAY2_FLOOD_H

#include <stdint.h>

#include "state/state.h"

void flood_tile_step(game_state *st, uint32_t self, int32_t flood);                         /* @bin 0x004BD950 */
void flood_level_step(game_state *st, uint32_t self, int32_t flood, int32_t pct, int32_t all); /* @bin 0x004BD850 */
void flood_level_more(game_state *st, uint32_t self);                                       /* @bin 0x004BE0D0 */
void flood_quality_roll(game_state *st, uint32_t self);                                     /* @bin 0x004BD380 */
void flood_season_start(game_state *st, uint32_t self);                                     /* @bin 0x004BE490 */
void flood_season_end(game_state *st, uint32_t self);                                       /* @bin 0x004BE580 */

#endif
