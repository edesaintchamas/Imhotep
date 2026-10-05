/* Imhotep - water supply of the day (day step 0x1B).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_DAY_WATER_H
#define IMHOTEP_BUILDING_DAY_WATER_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00455540 n u32 at p &= mask */
void grid_and(game_state *st, uint32_t p, int32_t n, uint32_t mask);
/* @bin 0x00487E60 a road (terrain 4) on either side of the building's +0xAA direction */
int32_t building_road_ahead(game_state *st, int32_t id);
/* @bin 0x004824B0 terrain |= mask on the square r tiles around a building of `size` at (x, y) */
void map_terrain_or_around(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r, uint32_t mask);
/* @bin 0x00468790 */
void map_grid_468790(game_state *st);
/* @bin 0x004555D0 day step 0x1B */
void day_water_4555d0(game_state *st);

/* @bin 0x004860D0 day step 0x14: fertility */
void day_fertility_4860d0(game_state *st);

/* @bin 0x00468590 groundwater spreading from a tile */
void groundwater_spread(game_state *st, int32_t tile);

/* @bin 0x004BEB90 (thiscall on 0x00D05720) */
void flood_groundwater(game_state *st, uint32_t self);
/* @bin 0x00487DC0 */
int32_t map_flood_object_near(game_state *st, int32_t x, int32_t y);
/* @bin 0x00487E30 */
int32_t building_flood_object_near(game_state *st, int32_t id);
/* @bin 0x00487D30 */
int32_t water_lift_image(game_state *st, int32_t dir, int32_t edge);
/* @bin 0x00487EC0 */
void water_lift_redraw(game_state *st, int32_t id);

/* @bin 0x0047CF00 */
void map_place_2x2(game_state *st, int32_t id, int32_t x, int32_t y, int32_t img);

#endif
