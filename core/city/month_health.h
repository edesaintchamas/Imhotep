/* Imhotep - the city's health, twice a month (0x004E8840, days 0 and 8).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_MONTH_HEALTH_H
#define IMHOTEP_CITY_MONTH_HEALTH_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004C38F0 -2 when city +0x4478 is set and building +0x472C is of level 1 with +0xA9 in
 * (2, 3), else 0 */
int32_t city_0x4c38f0(game_state *st);
/* @bin 0x004E8EC0 a malus 0..-6 from the share 0x004CDE60(+0x463C, +0x24) */
int32_t city_0x4e8ec0(game_state *st);
/* @bin 0x004E8840 the houses' +0xC2 and +0xE4, the city's +0x10 */
void city_health_update(game_state *st);

#endif
