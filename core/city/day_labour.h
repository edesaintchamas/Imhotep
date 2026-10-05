/* Imhotep - the city's labour totals, by day (day step 0x19).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY_LABOUR_H
#define IMHOTEP_CITY_DAY_LABOUR_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00459400 workers available per labour category (city +0x2B30 + 0x14 * c), by priority */
void labour_categories_fill(game_state *st);
/* @bin 0x00459350 labour totals, the unemployment rate with its smoothing, one message */
void labour_update(game_state *st);
/* @bin 0x004595A0 each building's share of its category's workers (+0x1A, percent) */
void labour_building_shares(game_state *st);

/* @bin 0x00458880 */
void population_workforce_458880(game_state *st);
/* @bin 0x004E87A0 */
void city_population_month_4e87a0(game_state *st, int32_t player);
/* @bin 0x00458540 day step 0x17 */
void day_population_458540(game_state *st);

/* @bin 0x004E92C0 */
int32_t city_age_group(game_state *st, int32_t player, int32_t group);
/* @bin 0x004E9460 */
int32_t city_working_age(game_state *st);
/* @bin 0x004E7E60 (four output pointers into the state) */
void houses_access_ratios(game_state *st, uint32_t out0, uint32_t out1, uint32_t out2, uint32_t out3);
/* @bin 0x004E8F80 */
void city_migration_4e8f80(game_state *st, int32_t player);

/* @bin 0x00458D00 */
void immigrant_send(game_state *st, int32_t id, int32_t n);
/* @bin 0x00458950 */
void immigrants_place(game_state *st, int32_t n);
/* @bin 0x00458D80 */
void emigrant_send(game_state *st, int32_t id, int32_t n);
/* @bin 0x00458B60 */
void emigrants_leave(game_state *st, int32_t n);

/* @bin 0x00458F00 */
int32_t houses_remove_people(game_state *st, int32_t player, int32_t n);
/* @bin 0x00458E20 */
int32_t houses_add_people(game_state *st, int32_t player, int32_t n);
/* @bin 0x004E92F0 */
void city_age_group_remove(game_state *st, int32_t player, int32_t group, int32_t n);
/* @bin 0x004E8650 */
void city_deaths_4e8650(game_state *st);
/* @bin 0x004E8720 */
void city_births_4e8720(game_state *st);

#endif
