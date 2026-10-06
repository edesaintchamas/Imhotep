/* Imhotep - scenario start (Pharaoh.exe 0x004D52A0): the runtime state the game derives when a
 * campaign record or a saved game is started, and the steps of it ported here.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Checks: tests/differential/oracle_function_check.py (loader entries), oracle_state_check.py. */
#ifndef IMHOTEP_STATE_SCENARIO_START_H
#define IMHOTEP_STATE_SCENARIO_START_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004D52A0 (prologue) map size and grid offsets from the scenario info (chunk 75), the
 * image grid cleared, then the map row ranges 0x0046F7B0. */
void scenario_start_map(game_state *st);

/* @bin 0x004D52A0 the scenario start: every step in the original's order; the steps of other
 * modules are called through GAME_CALL (core/sim/call.h). The model files are read by their
 * loaders through the platform (platform_file_read). */
void scenario_start(game_state *st);

/* @bin 0x0046F7B0 visible map rectangle and per-row ranges of the diamond-shaped map. */
void map_row_ranges(game_state *st);

/* steps of 0x004D52A0 (each @bin its address) */
void terrain_flag10_to_1(game_state *st);        /* 0x004771C0 */
void monument_slots_images(game_state *st);      /* 0x004E59A0 */
void buildings_map_add_all(game_state *st);      /* 0x00485F70 */
void buildings_scale_0x54(game_state *st);       /* 0x00489840 */
void figure_slots_check(game_state *st);         /* 0x004B32C0 */
void terrain_images_flag(game_state *st);        /* 0x00476750 */
void figures_count_5d_64(game_state *st);        /* 0x004BCA90 */
void empire_objects_images(game_state *st);      /* 0x00442660 */
void empire_objects_sizes(game_state *st);       /* 0x004426C0 */
int32_t buildings_count_b6(game_state *st);      /* 0x0046F190 */
void storage_yards_check(game_state *st);        /* 0x004D5AD0 */
void city_building_d1_position(game_state *st);  /* 0x004D5BC0 */
void figures_check_kind6(game_state *st);        /* 0x004A3560 */

#endif
