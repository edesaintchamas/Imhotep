/* Imhotep - building record states handled by the day (day step 0x28).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_DAY_STATE_H
#define IMHOTEP_BUILDING_DAY_STATE_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004E9170 city `player` +0x26E4 += n (0x004E9350), then the population total */
void city_people_add_4e9170(game_state *st, int32_t player, int32_t n);
/* @bin 0x004E9250 city `player` +0x26F8 += n (0x004E9350), then the population total */
void city_people_add_4e9250(game_state *st, int32_t player, int32_t n);
/* @bin 0x0046AC10 free a building record */
void building_free(game_state *st, int32_t id);
/* @bin 0x004BFEE0 0x0047C480 on the four tiles of table 0x005DF818 around a building */
void building_clear_tiles_4bfee0(game_state *st, int32_t id);
/* @bin 0x00486E70 */
void map_refresh_486e70(game_state *st);
/* @bin 0x004525F0 day step 0x28 */
void buildings_states_update(game_state *st);

/* @bin 0x004696B0 */
int32_t building_can_collapse(game_state *st, int32_t id);
/* @bin 0x00469730 day step 0x2B */
void day_fires_469730(game_state *st);

/* @bin 0x00468B70 day step 0x2C: collapse and fire risks */
void day_risks_468b70(game_state *st);

#endif
