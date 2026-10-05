/* Imhotep - daily passes over the buildings (day dispatcher 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY_BUILDINGS_H
#define IMHOTEP_CITY_DAY_BUILDINGS_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004CDE60 value * 100 / total (0 when total is 0) */
int32_t calc_ratio_percent(int32_t value, int32_t total);
/* @bin 0x0045A0A0 amount shared by the weights of a three-group table (x87) */
int32_t share_by_weight(game_state *st, int32_t amount, int32_t group, uint32_t table);
/* @bin 0x0045A140 a building's share of a city total, by type */
int32_t building_service_share(game_state *st, int32_t id);
/* @bin 0x004684E0 day step 0x22 */
void day_service_shares(game_state *st);
/* @bin 0x004558B0 day step 0xC */
void day_decay_0x18(game_state *st);
/* @bin 0x004806A0 building_road_access for size 4 */
int32_t building_road_access_4(game_state *st, int32_t x, int32_t y);
/* @bin 0x0045D630 per-city totals of building types 0x47 / 0xBF */
void day_totals_45d630(game_state *st, int32_t player);
/* @bin 0x0045D5D0 day step 0x11 */
void day_step_0x11(game_state *st);
/* @bin 0x0045AA30 day step 0xA: highest used building ids */
void buildings_highest_update(game_state *st);
/* @bin 0x00459020 */
void spawn_figure_3(game_state *st, int32_t player, int32_t x, int32_t y, int32_t amount);
/* @bin 0x00459070 day step 0x18 */
void day_list_45d83c(game_state *st);

/* @bin 0x00469E40 */
void building_spawn_469e40(game_state *st, int32_t id, int32_t flag);
/* @bin 0x00469B90 day step 0x2D */
void day_spawn_469b90(game_state *st);
/* @bin 0x004E8610 */
void city1_clear_4e8610(game_state *st);
/* @bin 0x004583F0 */
void buildings_list_active(game_state *st);
/* @bin 0x00458440 day step 0x16 */
void day_step_0x16(game_state *st);
/* @bin 0x00455430 day step 0x1C */
void day_step_0x1c(game_state *st);

/* @bin 0x004539E0 day step 0x26 */
void buildings_water_level(game_state *st);
/* @bin 0x0045E040 day step 8 */
void day_stock_totals_45e040(game_state *st);
/* @bin 0x00459100 day step 0x19 */
void labour_categories_need(game_state *st);
/* @bin 0x004689E0 flood fill of one road network */
void road_network_fill(game_state *st, int32_t tile, int32_t id);
/* @bin 0x00468860 day step 7: road networks */
void road_networks_update(game_state *st);

/* @bin 0x00488630 (thiscall on 0x00BCA350) */
void farms_season_488630(game_state *st, uint32_t self);
/* @bin 0x00488350 (thiscall on 0x00BCA350) */
int32_t farm_grow_488350(game_state *st, uint32_t self, int32_t id);
/* @bin 0x00455900 day step 0x14 */
void day_production_455900(game_state *st);

/* @bin 0x004677C0 day step 0x21: buildings by type */
void buildings_count_4677c0(game_state *st);

/* @bin 0x00488E60 (thiscall on 0x00BCA350) */
void farm_redraw_488e60(game_state *st, uint32_t self, int32_t id, int32_t flag);
/* @bin 0x00488140 (thiscall on 0x00BCA350) */
int32_t farm_yield_488140(game_state *st, uint32_t self, int32_t id, int32_t full);

/* @bin 0x00489050 (thiscall on 0x00BCA350) */
int32_t area_fertility_489050(game_state *st, int32_t x, int32_t y, int32_t width, int32_t rows);
/* @bin 0x004890F0 (thiscall) */
int32_t building_road_flag_4890f0(game_state *st, int32_t id);
/* @bin 0x00489110 (thiscall on 0x00BCA350) */
int32_t farm_fertility_489110(game_state *st, int32_t id);
/* @bin 0x004C5340 */
int32_t trade_tax_4c5340(game_state *st, int32_t player);

/* @bin 0x00488C20 (thiscall on 0x00BCA350) */
void farm_draw_488c20(game_state *st, uint32_t self, int32_t id, int32_t x, int32_t y, int32_t flag);

#endif
