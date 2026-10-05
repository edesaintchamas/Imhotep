/* Imhotep - daily passes over the buildings (dispatcher steps of 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_DAILY_H
#define IMHOTEP_BUILDING_DAILY_H

#include "state/state.h"

/* @bin 0x00457A40 road distances from the map entry point; buildings of types 0x4B and 0xC0 note
 * whether a road next to them is reached (+0x35) (day step 0x13) */
void buildings_entry_road_check(game_state *st);

#endif
