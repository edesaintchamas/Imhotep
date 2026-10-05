/* Imhotep - the gods' blessings and wrath (package day2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY2_GODS_H
#define IMHOTEP_CITY_DAY2_GODS_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004C47A0 god 0..4 acts on the city when its blessing / wrath counter is full (force
 * -1), or as forced: 1 blessing, 2 small blessing, 3 wrath, 4 small wrath. `kind` is unused. */
void god_act(game_state *st, int32_t god, int32_t kind, int32_t force);

#endif
