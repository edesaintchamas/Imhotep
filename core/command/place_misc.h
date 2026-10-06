/* Imhotep - per-type checks and work of the construction place command (package place).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_PLACE_MISC_H
#define IMHOTEP_COMMAND_PLACE_MISC_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0047F530 */
void place_bridge(game_state *st, int32_t x, int32_t y, int32_t kind);
/* @bin 0x004BFF60 */
uint8_t place_gatehouse_fits(game_state *st, int32_t x, int32_t y, int32_t rot);
/* @bin 0x004C3730 */
int32_t place_temple_complex_fits(game_state *st, int32_t x, int32_t y);
/* @bin 0x004DEF00 */
uint8_t place_monument_de_fits(game_state *st, int32_t offset);
/* @bin 0x00541AE0 */
uint8_t place_monument_d5_fits(game_state *st, int32_t offset);
/* @bin 0x00542EE0 */
uint8_t place_monument_d5_done(game_state *st);
/* @bin 0x004E5330 */
uint8_t place_monument_fits(game_state *st, uint32_t obj, int32_t offset, uint32_t parts, int32_t n);
/* @bin 0x004E5790 */
uint8_t place_monument_exists(game_state *st, uint32_t obj, int32_t type, int32_t n);
/* @bin 0x005463E0 */
uint8_t place_monument_fits_5463e0(game_state *st, uint32_t obj, int32_t offset, uint32_t parts, int32_t n);
/* @bin 0x00443900 */
int32_t place_trade_sells_443900(game_state *st, int32_t res);
/* @bin 0x00480770 */
int32_t place_tower_wall_near(game_state *st, int32_t x, int32_t y, int32_t size);
/* @bin 0x00447530 */
int32_t place_food_ratio_447530(game_state *st);
/* @bin 0x00489880 */
uint8_t place_type_farm_489880(int32_t type);

#endif
