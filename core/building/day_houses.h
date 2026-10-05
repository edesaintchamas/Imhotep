/* Imhotep - the houses' daily evolution pass (day step 0x27).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_DAY_HOUSES_H
#define IMHOTEP_BUILDING_DAY_HOUSES_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00454D70 clears the current city's house demand counters */
void houses_counters_clear(game_state *st);
/* @bin 0x0046B740 a building's tile and coordinates checked against the building grid */
void building_check_position(game_state *st, int32_t id);
/* @bin 0x00454B40 a house's daily use of its four goods (+0x64..+0x6A) */
void house_consume(game_state *st, int32_t id);
/* @bin 0x004528B0 day step 0x27 */
void houses_evolve_all(game_state *st);

/* @bin 0x0046CEA0 */
void house_set_type(game_state *st, int32_t id, int32_t type);
/* @bin 0x004539A0 */
int32_t houses_redraw_only(game_state *st);
/* @bin 0x00453AF0 */
int32_t house_desirability_trend(game_state *st, int32_t id);
/* @bin 0x0046B9E0 */
int32_t house_merge_neighbours(game_state *st, int32_t id, int32_t n);
/* @bin 0x0046BDA0 */
void house_become_large(game_state *st, int32_t id, int32_t people);
/* @bin 0x0046B840 */
int32_t house_try_merge(game_state *st, int32_t id);
/* @bin 0x0046B480 */
int32_t house_find_expansion(game_state *st, int32_t id, int32_t n);
/* @bin 0x0046B940 */
void house_clear_expansion(game_state *st, int32_t id, int32_t n);
/* @bin 0x0046BAB0 0x0046BBC0 0x0046BCB0 */
void house_grow_0x14(game_state *st, int32_t id, int32_t people);
void house_grow_0x18(game_state *st, int32_t id, int32_t people);
void house_grow_0x1c(game_state *st, int32_t id, int32_t people);
/* @bin 0x0046BE90 0x0046C1C0 0x0046CB30 0x0046CCF0 */
void house_split_large(game_state *st, int32_t id);
void house_devolve_0x14(game_state *st, int32_t id);
void house_devolve_0x18(game_state *st, int32_t id);
void house_devolve_0x1c(game_state *st, int32_t id);
/* @bin 0x00453B40 */
int32_t house_can_evolve(game_state *st, int32_t id, int32_t up);

#endif
