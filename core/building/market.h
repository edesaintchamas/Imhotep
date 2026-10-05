/* Imhotep - the buyers of buildings of types 0x46 / 0xBE (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_MARKET_H
#define IMHOTEP_BUILDING_MARKET_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00455E41 a good (1..0x13) -> its index in the 27-entry per-good arrays, -1 if none */
int32_t market_good_index(int32_t good);

/* @bin 0x00455CA0 is `value` one of the four entries of `list` */
int market_listed(int32_t value, const int32_t *list);

/* @bin 0x00455E00 the (up to) four goods of list 0x00F53DDC that building `id` should buy: first
 * those it has none of, then those below 600; `counts` is the game address of 27 counts (only
 * goods with a non-zero count qualify); out[4] (-1: none) */
void market_wanted_goods(game_state *st, int32_t id, uint32_t counts, int32_t *out);

/* @bin 0x00456070 the same for goods 8..11: those it has none of, then those below 100 */
void market_wanted_goods_8_11(game_state *st, int32_t id, uint32_t counts, int32_t *out);

/* @bin 0x004561E0 the good to fetch (stored at +0xAC) among list 0x00F53DDC, and its source
 * building from `targets` (game address, 27 dwords); 0 when none */
int32_t market_pick_good(game_state *st, int32_t id, uint32_t counts, uint32_t targets);

/* @bin 0x004565F0 the same for goods 8..11 */
int32_t market_pick_good_8_11(game_state *st, int32_t id, uint32_t counts, uint32_t targets);

/* @bin 0x00456FC0 the building a buyer of building `id` goes to (0: none); kind 0: goods of list
 * 0x00F53DDC, else goods 8..11. goods[4]: the goods to buy; *flood: set once the distances from the
 * building were computed (0x00519A30) */
int32_t market_find_source(game_state *st, int32_t id, int kind, int32_t *goods, uint8_t *flood);

#endif
