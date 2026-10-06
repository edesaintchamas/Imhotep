/* Imhotep - map values computed for the hovered tile (package cmdrest).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The platform calls these each frame for the hovered tile (0x00BCA300 / 0x00BCA304 / 0x00BCA308)
 * while the construction type 0x006059E8 is a ferry (0x88: ferry_hover) or a bridge (0x52, 0x53:
 * bridge_measure), as the original's city-view drawing does; the place step reads their results. */
#ifndef IMHOTEP_COMMAND_HOVER_H
#define IMHOTEP_COMMAND_HOVER_H

#include <stdint.h>

#include "state/state.h"

int32_t edge_neighbours_with(game_state *st, int32_t offset, uint32_t mask);   /* @bin 0x0047EC80 */
int32_t corner_neighbours_with(game_state *st, int32_t offset, uint32_t mask); /* @bin 0x0047ECC0 */
/* @bin 0x0047F3E0 the ferry's reach over the water from (x, y): 0x00BC96E0..0x00BC96F0 */
int32_t ferry_reach(game_state *st, int32_t x, int32_t y);
/* @bin 0x0047F200 the bridge starting at (x, y): 0x00BC96F4..0x00BC9704 */
int32_t bridge_measure(game_state *st, int32_t x, int32_t y);
/* @bin 0x0043D300 (rule part) the ferry landing validity 0x00BCA314 for the hovered tile */
void ferry_hover(game_state *st);

#endif
