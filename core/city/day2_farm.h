/* Imhotep - the farms' crop images (package day2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_DAY2_FARM_H
#define IMHOTEP_CITY_DAY2_FARM_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00488690 / 0x00488840 (thiscall on 0x00BCA350) */
void farm_crop_small(game_state *st, int32_t id, int32_t x, int32_t y, int32_t img, int32_t stage, int32_t flag);
void farm_crop_large(game_state *st, int32_t id, int32_t x, int32_t y, int32_t img, int32_t stage, int32_t flag);

int32_t farm_growth_pct(game_state *st, int32_t id);                                /* @bin 0x004883D0 */
void farm_fertility_cut(game_state *st, uint32_t self, int32_t id, int32_t pct);     /* @bin 0x00489180 */
void farm_flood_notice(game_state *st, uint32_t self);                               /* @bin 0x00489510 */

#endif
