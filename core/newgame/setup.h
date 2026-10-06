/* Imhotep - the scenario setup of a custom map start (docs/newgame.md 3.3): 0x004DE820, the map
 * load 0x004DE600, the empire and trade setup.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_NEWGAME_SETUP_H
#define IMHOTEP_NEWGAME_SETUP_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004DE820 the scenario setup: the map load, the initial figures, entry and exit points,
 * treasury, year and deadline, the empire, the camera */
void scenario_setup(game_state *st);

/* @bin 0x004DE600 the map load: maps\<0x00E37078>.map read (0x004D5820), map size and points,
 * the grids, terrain rebuild, buildings of flagged tiles, routing grids */
void scenario_map_load(game_state *st);

/* @bin 0x00444A80 empire objects: default trade amounts, cleared for goods not traded */
void empire_trade_amounts_setup(game_state *st);
/* @bin 0x00444720 the empire cities (0x0077DE00) from the empire objects, the route limits */
void empire_cities_setup(game_state *st);
/* @bin 0x00444B10 0x0077AF00 (0x2260 bytes) and 0x0077AEFC := 0 */
void empire_0x77af00_clear(game_state *st);
/* @bin 0x00442800 the empire map scroll position clamped */
void empire_scroll_clamp(game_state *st);

/* @bin 0x00444950 route `route` (0..49) is of kind 2 (+0x140) */
int32_t empire_route_is_kind2(game_state *st, int32_t route);
/* @bin 0x00444A00 trade amount of good `good` of empire object `obj` (+0x3E + good, 0..40), 0
 * when the object is not a city (kind 1, +0x18 != 0) or the value is above 40 */
int32_t empire_object_trade_amount(game_state *st, int32_t obj, int32_t good);
/* @bin 0x00444A40 its setter (values >= 0) */
void empire_object_trade_amount_set(game_state *st, int32_t obj, int32_t good, int32_t value);

#endif
