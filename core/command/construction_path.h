/* Imhotep - the drag path distances of roads, canals and walls (package ui).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_CONSTRUCTION_PATH_H
#define IMHOTEP_COMMAND_CONSTRUCTION_PATH_H

#include <stdint.h>

#include "state/state.h"

int32_t terrain_0x10000_or_desirable(game_state *st, int32_t o);                 /* @bin 0x0047B8E0 */
int32_t water_both_axes(game_state *st, int32_t o);                              /* @bin 0x0047E090 */
int32_t water_neighbours(game_state *st, int32_t o);                             /* @bin 0x0047E0D0 */
int32_t bridge_passable(game_state *st, int32_t o);                              /* @bin 0x0051C390 */
int32_t ramp_passable(game_state *st, int32_t o);                                /* @bin 0x0051C510 */
int32_t crossing_passable(game_state *st, int32_t o);                            /* @bin 0x0051C630 */
int32_t drag_path_fill(game_state *st, int32_t unused, int32_t x, int32_t y, int32_t canal); /* @bin 0x0051B680 */
int32_t wall_path_fill(game_state *st, int32_t unused, int32_t x, int32_t y);    /* @bin 0x0051C790 */

#endif
