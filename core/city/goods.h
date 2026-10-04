/* Imhotep - which goods the city can obtain (empire trade, production chains, events).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Ported from Pharaoh.exe; each function names the original. Records: docs/formats/save.md §5
 * (empire cities, chunk 50; trade-route limits, chunk 114; city data, chunk 28). */
#ifndef IMHOTEP_CITY_GOODS_H
#define IMHOTEP_CITY_GOODS_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004C5190 (0 < a) - 2 (0 < b) - (0 < c) from three i16 of the current player's city data */
int32_t trade_city_mood(game_state *st);
/* @bin 0x004C5240 a trade-route limit adjusted by trade_city_mood */
int32_t trade_limit_adjusted(game_state *st, int32_t limit);
/* @bin 0x00447790 the adjusted limit of `resource` on empire city `city`'s route is > 0 */
int32_t empire_city_trades(game_state *st, int32_t city, int32_t resource);
/* @bin 0x004439C0 some trading city (kind 1, 3, 5) has flag +0x2A+resource and trades it */
int32_t goods_trade_city(game_state *st, int32_t resource);
/* @bin 0x00443950 same, for cities whose byte +5 is set */
int32_t goods_trade_city_flag5(game_state *st, int32_t resource);
/* @bin 0x00443B90 production-chain test (recursive, with goods made from others) */
int32_t goods_chain_b90(game_state *st, int32_t resource);
/* @bin 0x00443D90 production-chain test, second variant */
int32_t goods_chain_d90(game_state *st, int32_t resource);
/* @bin 0x00559480 goods named by kind-0x17 entries of the list at 0x00785D70, and their chains */
int32_t goods_event_list(game_state *st, int32_t resource);
/* @bin 0x005596D0 lists of available goods (0x00F53E70, 1..35; 0x00F53DDC, 1..8) and the city's
 * flags for them */
void goods_available_update(game_state *st);

#endif
