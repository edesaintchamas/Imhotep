/* Imhotep - the month's mood of the houses and of the city (0x004E81D0 and its helpers).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_MONTH_MOOD_H
#define IMHOTEP_CITY_MONTH_MOOD_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004EB800 search square rings around (x0, y0) for a figure of one of `ntypes` types (u32
 * list at the state address `types`, other than `except`: returns its id) or a tile whose terrain
 * has a bit of `mask` (returns ntypes + 1); *x, *y: the tile found. 0: none within `radius` */
int32_t map_search_rings(game_state *st, int32_t x0, int32_t y0, int32_t ntypes, uint32_t types, uint32_t mask,
                         int32_t *x, int32_t *y, int32_t radius, int32_t except);
/* @bin 0x004E7F00 one house's month: its +0xFC and +0xE8 */
void house_mood_month(game_state *st, int32_t id, int32_t recompute);
/* @bin 0x004E81D0 the month's city mood (+0x14, +0x18) from the houses */
void city_mood_month(game_state *st);
/* @bin 0x004E9350 n people leave (ages by the RNG pool, then from age 10) */
void people_remove(game_state *st, int32_t player, int32_t n);
/* @bin 0x004E9400 n people arrive */
void people_add(game_state *st, int32_t player, int32_t n);
/* @bin 0x004E90F0 the population changes by delta; city +0x24 recounted */
void people_change(game_state *st, int32_t player, int32_t delta);
/* @bin 0x0042FB30 a house's people leave */
void house_empty(game_state *st, int32_t id);
void house_leave_0x430040(game_state *st, int32_t id);
int32_t house_leave_0x430060(game_state *st, int32_t id);
void houses_leave_countdown(game_state *st);
void houses_leave_messages(game_state *st);
int32_t city_riot_roll(game_state *st);
int32_t house_most_0xfc(game_state *st);
int32_t house_riot_start(game_state *st, int32_t id);
int32_t city_riot(game_state *st, int32_t any);

#endif
