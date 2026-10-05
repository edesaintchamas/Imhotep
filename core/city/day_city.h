/* Imhotep - daily city-wide steps: ratings, debt, events (day dispatcher 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY_CITY_H
#define IMHOTEP_CITY_DAY_CITY_H

#include <stdint.h>

#include "state/state.h"

#define EVENTS 0x00785D70u /* the event list: i32 count, then records of 0x7C bytes */

/* @bin 0x0044BAF0 (thiscall on the event list) the next active event record after `prev` (0:
 * the first); its +0 gets its index */
uint32_t events_next_active(game_state *st, uint32_t self, uint32_t prev);
/* @bin 0x0044D4C0 (thiscall on an event record) */
int32_t event_is_running(game_state *st, uint32_t rec);
/* @bin 0x00559C60 a value capped by kind */
int32_t value_cap(int32_t kind, int32_t v);
/* @bin 0x00443080 buildings serving the slot of `kind` */
int32_t buildings_serving(game_state *st, int32_t kind);
/* @bin 0x00443330 day step 2 */
void day_step_0x02(game_state *st);
/* @bin 0x0045A990 the current city's finance totals */
void finance_totals(game_state *st);
/* @bin 0x0046E840 */
void city_0x46e840(game_state *st);
/* @bin 0x0044EF90 (thiscall on an event record) */
void event_stop(game_state *st, uint32_t rec);
/* @bin 0x004460B0 day step 4: debt */
void day_step_0x04(game_state *st);

/* @bin 0x004550E0 day step 0x31 */
void day_city_ratings_4550e0(game_state *st);

/* @bin 0x004C06E0 */
int32_t god_building_type(int32_t god, int32_t kind);
/* @bin 0x004E94A0 */
int32_t city_people_young_4e94a0(game_state *st, int32_t player);
/* @bin 0x004E94D0 */
int32_t city_people_4e94d0(game_state *st, int32_t player);
/* @bin 0x00468170 day step 0x21 */
void day_coverage_468170(game_state *st);

/* @bin 0x00559FC0 (kinds: a 16-entry buffer of the caller) */
int32_t trade_open_routes_559fc0(game_state *st, int32_t *kinds);
/* @bin 0x00444C20 day step 0x20: trade routes */
void day_trade_444c20(game_state *st);

/* @bin 0x004E9890 day step 1: the gods */
void gods_update_4e9890(game_state *st, int32_t moods);

#endif
