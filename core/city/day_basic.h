/* Imhotep - small daily steps of the city (day dispatcher 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY_BASIC_H
#define IMHOTEP_CITY_DAY_BASIC_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0046A060 clears four counters (day step 0x2F) */
void day_clear_counters(game_state *st);
/* @bin 0x004599A0 every building's byte +0x4E counts down to 0 (day step 0x30) */
void day_building_countdown(game_state *st);
/* @bin 0x0045AAA0 day step 0x10: 0x0045AAE0(1) when 0x00EA624A is set */
void day_step_0x10(game_state *st);
/* @bin 0x0045AAE0 per-city totals of building types 0x48 / 0xC1 / 0x49 */
void day_totals_45aae0(game_state *st, int32_t player);
/* @bin 0x0045E6C0 day step 0x12: 0x0045E6E0(1) when 0x00EA624A is set */
void day_step_0x12(game_state *st);
/* @bin 0x0045E6E0 per-city totals by the buildings' +0x12 / +0xCE slots */
void day_totals_45e6e0(game_state *st, int32_t player);
/* @bin 0x0055F561 (thiscall on the sound object) stop stream `channel` */
void sound_stop_stream(game_state *st, uint32_t self, int32_t channel);

/* @bin 0x0047E290 day step 0x2D: every 20 days the 3-bit fields of grid 0x00978D9C decay */
void map_grid_decay_978d9c(game_state *st);
/* @bin 0x004B52F0 add a figure to its tile's figure list */
void figure_tile_list_add(game_state *st, int32_t id);

/* @bin 0x0044BCA0 (thiscall on 0x00785D70) every tick: the running events */
void events_tick_44bca0(game_state *st, uint32_t self);
/* @bin 0x00485DC0 every tick: whether the undo stays available */
void undo_check_485dc0(game_state *st);

#endif
