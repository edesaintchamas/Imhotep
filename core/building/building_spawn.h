/* Imhotep - helpers of the daily building update 0x00462180 (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_BUILDING_SPAWN_H
#define IMHOTEP_BUILDING_BUILDING_SPAWN_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00467220 a walker of type 5 (slot +0x2C) unless 0x00488050 (this 0x00BCA350) says no */
void building_spawn_type5(game_state *st, int32_t id);
/* @bin 0x004673C0 */
int32_t building_mark_tiles_by_level(game_state *st, int32_t id);
/* @bin 0x00467310 */
void building_update_0xb4_tiles(game_state *st, int32_t id);
/* @bin 0x00462140 */
int32_t building_count_figures(game_state *st, int32_t id, int32_t type);
/* @bin 0x0045D510 */
int32_t building_parts_stock(game_state *st, int32_t id, int32_t good);
/* @bin 0x0045BB30 */
int32_t building_count_nonzero(int32_t a, int32_t b);
/* @bin 0x00455BF0 */
int32_t building_production_ready(game_state *st, int32_t id);
/* @bin 0x00455C30 */
void building_production_take(game_state *st, int32_t id);
/* @bin 0x00455CC0 */
void building_houses_nearby(game_state *st, int32_t id, int32_t dist);
/* @bin 0x0045CE90 */
int32_t building_cart_good(game_state *st, int32_t id, int32_t pending);

/* @bin 0x004612C0, 0x00461490, 0x004615D0, 0x00461720 (building_gatherers.c) */
void building_update_0xb1(game_state *st, int32_t id);
void building_update_0xb3(game_state *st, int32_t id);
void building_update_0xb2(game_state *st, int32_t id);
void building_update_0xe7(game_state *st, int32_t id);

/* building_misc.c */
void building_formation_size(game_state *st, int32_t id);         /* @bin 0x004B76B0 */
void building_cycle_image(game_state *st, int32_t id);            /* @bin 0x0047C260 */
int32_t figure_group_new(game_state *st);                         /* @bin 0x0046F140 */
int32_t building_find_open_tile(game_state *st, int32_t x, int32_t y, int32_t size); /* @bin 0x004807D0 */
int32_t building_find_road_8(game_state *st, int32_t x, int32_t y, int32_t size);    /* @bin 0x004806C0 */
int32_t building_find_edge_tile(game_state *st, int32_t x, int32_t y, int32_t orientation); /* @bin 0x00480460 */
int32_t building_find_missing_worker(game_state *st, int32_t type); /* @bin 0x00460EA0 */
void figure_init_type_values(game_state *st, int32_t id);         /* @bin 0x004620C0 */
void figure_init_type_values_full(game_state *st, int32_t id);    /* @bin 0x00461F90 */

/* building_link.c */
void building_link_path(game_state *st, int32_t id, int32_t on, int32_t kind); /* @bin 0x0043F060 */
void building_release_figures(game_state *st, int32_t id);                     /* @bin 0x004A3630 */
void building_release_figures_pair(game_state *st, int32_t id);                /* @bin 0x004A36B0 */

int32_t building_distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t a, int32_t b, game_state *st); /* @bin 0x0045DC50 */
int32_t building_find_supplier(game_state *st, int32_t id, int32_t good, int32_t big_only); /* @bin 0x0045BB50 */

/* building_extra.c */
int32_t building_send_0x2a(game_state *st, int32_t id);   /* @bin 0x004B7400 */
void building_update_0xc7(game_state *st, int32_t id);    /* @bin 0x004BED70 */
int32_t farm_harvest(game_state *st, int32_t id);         /* @bin 0x00488400 */

#endif
