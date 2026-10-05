/* Imhotep - the scenario's win and loss conditions, every tick (0x004EA2C0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY_GOALS_H
#define IMHOTEP_CITY_DAY_GOALS_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004EA930 every tested counter of 0x00784E7C at least its goal at 0x00784DEC */
int32_t goals_counters_met(game_state *st);
/* @bin 0x004F52F0 */
int32_t goal_kind_4f52f0(game_state *st, int32_t i);
/* @bin 0x004EA280 houses (used, +5, inhabited) of level (+0x10 - 10) at least `level` */
int32_t houses_at_least(game_state *st, int32_t level);
/* @bin 0x004EA2C0 every tick */
void goals_check(game_state *st);

#endif
