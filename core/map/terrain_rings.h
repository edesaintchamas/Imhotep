/* Imhotep - the tile ring offset tables the game builds at start-up (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_RINGS_H
#define IMHOTEP_MAP_TERRAIN_RINGS_H

#include "state/state.h"

/* @bin 0x0046F8D0 the 1x1 rings 1..12 as (dx, dy) byte pairs, at the pointers 0x005DBBD8 */
void rings_1x1_build(game_state *st);

/* @bin 0x00481F20 0x0046F8D0, then the ring tile offsets (dx + 228 dy, words) of every footprint
 * size: 1x1 rings 1..12 at 0x0070C8EC, 2x2 .. 6x6 rings 1..6 at 0x0070C76C, 0x0070C5BC,
 * 0x0070C3DC, 0x0070C1CC, 0x0070BF8C */
void ring_offsets_build(game_state *st);

#endif
