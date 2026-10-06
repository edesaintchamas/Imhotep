/* Imhotep - the construction place command (package place): 0x004728A0 and its helpers.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_PLACE_H
#define IMHOTEP_COMMAND_PLACE_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004728A0 (player, orientation, x1, y1, x2, y2, type) place a building or drag a type */
void place_construction(game_state *st, int32_t player, int32_t orientation, int32_t x1, int32_t y1, int32_t x2,
                        int32_t y2, int32_t type);

/* @bin 0x0040F920 (dst, n, value) AND every byte of dst[0..n) with value */
void place_and_bytes(game_state *st, uint32_t dst, int32_t n, uint8_t value);

/* @bin 0x0046FDE0 (type) 1 for the types whose land check accepts roads */
uint8_t place_type_on_roads(int32_t type);

/* @bin 0x00470900 (x1, y1, x2, y2) 1 when the rectangle covers a scenario special point */
uint8_t place_special_point_in(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

/* @bin 0x004B5830 (x1, y1, x2, y2) 1 when a figure of types 0x2B-0x39 or 0x63 is within 12 tiles of
 * either corner */
int32_t place_figures_near(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

/* @bin 0x00485200 the undo record of the last construction's cost */
void place_undo_cost(game_state *st);

/* @bin 0x004E9680 (player, amount) construction spending */
void place_spend(game_state *st, int32_t player, int32_t amount);

/* @bin 0x004F61E0 the current city's three monument slots rebuilt */
void place_monument_slots(game_state *st);

/* @bin 0x004B9EE0 (x, y) formations near the tile turn towards it */
void place_formations_alert(game_state *st, int32_t x, int32_t y);

/* @bin 0x004ECEC0 (offset, variant, reason) the pyramid fits at the grid offset; reason bits */
uint8_t place_pyramid_fits(game_state *st, int32_t offset, int32_t variant, int32_t *reason);

/* @bin 0x0045B4D0 (player, resource, amount) take goods from the storage yards; returns what was
 * taken */
int32_t place_take_goods(game_state *st, int32_t player, int32_t resource, int32_t amount);

/* @bin 0x00457AC0 (x, y) the tile reaches the entry point by road (distance grid from the entry) */
int32_t place_road_to_entry(game_state *st, int32_t x, int32_t y);

/* @bin 0x00459FA0 a building of state 1 passes 0x00459EF0 */
uint8_t place_any_building_459ef0(game_state *st);

#endif
