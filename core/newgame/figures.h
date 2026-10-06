/* Imhotep - the figures placed when a map starts (package initfig, docs/newgame.md §3.3, §6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_NEWGAME_FIGURES_H
#define IMHOTEP_NEWGAME_FIGURES_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004BB3F0 the herds: a formation and its animals at each herd point of the scenario */
void newgame_herds_create(game_state *st);

/* @bin 0x004BBC10 a figure of type 0x5F at each of the 8 points 0x00784B74 / 0x00784B84 */
void newgame_points_5f_create(game_state *st);

/* @bin 0x004BBD40 a figure of type 0x41 at each of the 8 points (walking a line from its tile),
 * then, with `flag`, 20 figures of type 0x25 at (x, y) */
void newgame_points_41_create(game_state *st, int32_t x, int32_t y, int32_t flag);

#endif
