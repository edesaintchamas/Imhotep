/* Imhotep - the building constructor (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_BUILDING_CREATE_H
#define IMHOTEP_BUILDING_BUILDING_CREATE_H

#include <stdint.h>

#include "state/state.h"

int32_t building_removed_listed(game_state *st, int32_t id);                           /* @bin 0x004851B0 */
int32_t footprint_touches_water(game_state *st, int32_t x, int32_t y, int32_t size);   /* @bin 0x00481000 */
/* @bin 0x0046A400 (city, type, x, y, n, id) the new building's id, 0 if none */
int32_t building_create(game_state *st, int32_t city, int32_t type, int32_t x, int32_t y, int32_t n, int32_t id);

#endif
