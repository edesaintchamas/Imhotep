/* Imhotep - actions of the scenario events (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_EVENTS_ACTIONS_H
#define IMHOTEP_CITY_EVENTS_ACTIONS_H

#include <stdint.h>

#include "state/state.h"

void city_mood_add(game_state *st, int32_t delta);                 /* @bin 0x004E85D0 */
int32_t flood_month_before(game_state *st, uint32_t self);         /* @bin 0x004BECF0 */
void flood_quality_add(game_state *st, uint32_t q, int32_t delta); /* @bin 0x004BD5E0 */
void flood_worse(game_state *st, uint32_t self);                   /* @bin 0x004BED00 */
void flood_better(game_state *st, uint32_t self);                  /* @bin 0x004BED40 */
void plague_frogs_end(game_state *st);                             /* @bin 0x004E6EB0 */

void plague_hail(game_state *st);                                  /* @bin 0x004E6ED0 */
void plague_frogs(game_state *st, int32_t months);                 /* @bin 0x004E70B0 */
void plague_flies(game_state *st, int32_t months);                 /* @bin 0x004E7200 */
void plague_fire(game_state *st, int32_t times);                   /* @bin 0x00469C60 */
int32_t building_monument_reachable(game_state *st, int32_t b);    /* @bin 0x004E7C90 */
void plague_tomb_robbers(game_state *st, int32_t arg);             /* @bin 0x004E7AC0 */
int32_t building_destroy_first_of_type(game_state *st, int32_t t); /* @bin 0x00469F80 */

void ramp_walk(game_state *st, int32_t x, int32_t y, int32_t mode, int32_t arg4); /* @bin 0x004ED360 (mode 1: state_unported) */

#endif
