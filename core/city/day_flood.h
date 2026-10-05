/* Imhotep - the Nile flood, every tick (0x004BE770).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY_FLOOD_H
#define IMHOTEP_CITY_DAY_FLOOD_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004BE770 (thiscall on the floodplain manager 0x00D05720) */
void flood_tick(game_state *st, uint32_t self);

#endif
