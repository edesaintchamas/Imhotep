/* Imhotep - storage and delivery destinations under the cart and porter figure actions
 * (package figx, part b). Names describe what the binary does; the game meaning of the city
 * fields involved (trade status 1..4, ...) is not established here.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_HELPERS_XB_H
#define IMHOTEP_FIGURE_HELPERS_XB_H

#include <stdint.h>

#include "state/state.h"

/* goods checks against an empire city (city data +0x2826 status, route limits) */
int32_t trade_good_open_24(game_state *st, int32_t player, int32_t city, int32_t good); /* @bin 0x004434E0 */
int32_t trade_good_open_13(game_state *st, int32_t player, int32_t city, int32_t good); /* @bin 0x004435F0 */

/* storage buildings (types 0x48 / 0xC1) */
int32_t storage_over_limit(game_state *st, int32_t bid, int32_t amount, int32_t good);  /* @bin 0x0045B230 */
int32_t storage_add(game_state *st, int32_t player, int32_t bid, int32_t good, int32_t amount); /* @bin 0x0045B2A0 */
int32_t storage_room_loads(game_state *st, int32_t bid, int32_t good, int32_t amount);  /* @bin 0x004454D0 */
int32_t storage_trades_24(game_state *st, int32_t fid, int32_t player, int32_t bid, int32_t city); /* @bin 0x0045CF50 */
int32_t storage_trades_13(game_state *st, int32_t fid, int32_t player, int32_t bid, int32_t city); /* @bin 0x0045D180 */
int32_t storage_next_good_13(game_state *st, int32_t player, int32_t bid, int32_t city);  /* @bin 0x0045D370 */
void city_good_value_move(game_state *st, int32_t player, int32_t unused, int32_t good);  /* @bin 0x0045D4D0 */
int32_t storage_find_for_trader(game_state *st, int32_t fid, int32_t x, int32_t y, int32_t city, int32_t a4); /* @bin 0x004450C0 */
int32_t storage_find_13(game_state *st, int32_t x, int32_t y, int32_t city, int32_t a3, int32_t player); /* @bin 0x00445670 */
int32_t storage_find_24(game_state *st, int32_t x, int32_t y, int32_t city, int32_t a3, int32_t player); /* @bin 0x00445AC0 */
int32_t storage_take_from_trader(game_state *st, int32_t bid, int32_t good, int32_t city, int32_t fid); /* @bin 0x00445980 */
void figure_slot_count_good(game_state *st, int32_t fid, int32_t good);                  /* @bin 0x00444B80 */
void figure_slot_count_good_b(game_state *st, int32_t fid, int32_t good);                /* @bin 0x00444BD0 */
int32_t storage_sell_next(game_state *st, int32_t player, int32_t bid, int32_t city);     /* @bin 0x0045D010 */

/* other delivery destinations */
int32_t building_type_0x459ef0(game_state *st, int32_t bid);                              /* @bin 0x00459EF0 */
int32_t building_47_add(game_state *st, int32_t bid, int32_t good, int32_t count, int32_t amount); /* @bin 0x0045E220 */
int32_t building_add_good(game_state *st, int32_t bid, int32_t amount, int32_t good);     /* @bin 0x0045FE20 */
int32_t cart_city_building(game_state *st, int32_t x, int32_t y, int32_t good, int32_t terrain); /* @bin 0x0045F7B0 */
void figure_mark_building_0x20(game_state *st, int32_t fid);                              /* @bin 0x00468530 */

/* map scans */
int32_t map_free_tile_in_ring(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r); /* @bin 0x004800B0 */
int32_t map_free_tile_near(game_state *st, int32_t x, int32_t y, int32_t size, int32_t max_r); /* @bin 0x00480060 */

#endif
