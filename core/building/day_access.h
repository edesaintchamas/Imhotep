/* Imhotep - road access of buildings, as checked by the daily passes.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_DAY_ACCESS_H
#define IMHOTEP_BUILDING_DAY_ACCESS_H

#include <stdint.h>

#include "state/state.h"

#define ROAD_ACCESS_X 0x00D3B4E4u /* i32: the road tile found by building_road_access */
#define ROAD_ACCESS_Y 0x00D3B4E0u

/* @bin 0x00480180 a road tile around a building of `size` at (x, y), among the size's offsets
 * (table 0x005CC9E0), preferring the road network ranked first in the city's list (+0x46AC);
 * returns 1 and stores it in ROAD_ACCESS_X / Y, else 0 */
int32_t building_road_access(game_state *st, int32_t x, int32_t y, int32_t size);

/* @bin 0x00475B50 the main building of a multi-part building (follows +0x38 up to 0x20D times) */
int32_t building_main_part(game_state *st, int32_t id);

/* @bin 0x00487F80 types 0x64..0x69, 0xC4, 0xE0 (thiscall, this unused) */
int32_t building_type_0x487f80(int32_t type);
/* @bin 0x00488030 (thiscall on 0x00BCA350) building_type_0x487f80 of a building */
int32_t building_is_type_0x487f80(game_state *st, int32_t id);
/* @bin 0x004893C0 a tile around (x, y) with terrain & mask (offset table 0x005CC8C0) */
int32_t map_area_test(game_state *st, int32_t x, int32_t y, int32_t width, int32_t rows, uint32_t mask);
/* @bin 0x004894A0 (thiscall on 0x00BCA350) */
int32_t building_area_test_3x3(game_state *st, int32_t id);
/* @bin 0x00488050 (thiscall on 0x00BCA350) */
int32_t building_needs_area_check(game_state *st, int32_t id);
/* @bin 0x004816B0 a tile with terrain & mask within size - 1 of (x, y) */
int32_t map_area_has_terrain(game_state *st, int32_t x, int32_t y, int32_t size, uint32_t mask);
/* @bin 0x0047FC40 mark (+0x3F) the buildings within r of a building */
void map_mark_buildings_around(game_state *st, int32_t id, int32_t r);
/* @bin 0x0047FEB0 the best road tile at most r from a building of `size` at (x, y) */
int32_t building_road_in_ring(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r, int32_t no_road);
/* @bin 0x0047FE60 */
int32_t building_road_within(game_state *st, int32_t x, int32_t y, int32_t size, int32_t radius);

/* @bin 0x00481060 */
int32_t building_reached_road(game_state *st, int32_t x, int32_t y, int32_t size);
/* @bin 0x004802C0 */
int32_t road_next_to_4802c0(game_state *st, int32_t x, int32_t y);

#endif
