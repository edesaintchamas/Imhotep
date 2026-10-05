/* Imhotep - the tree, meadow, flag 0x40000 and rubble sub-passes of the terrain images over any
 * rectangle (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_TERRAIN_VEG_H
#define IMHOTEP_MAP_TERRAIN_VEG_H

#include <stdint.h>

#include "state/state.h"

void trees_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag);       /* @bin 0x00479770 */
void trees_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1);  /* @bin 0x00475BF0 */
void meadow_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag);      /* @bin 0x00479B70 */
void meadow_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1); /* @bin 0x00477500 */
void flag_0x40000_tile_redraw(game_state *st, int32_t x, int32_t y, int32_t flag);      /* @bin 0x0047AB10 */
void flag_0x40000_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1); /* @bin 0x004776F0 */
void rubble_tile_redraw(game_state *st, int32_t x, int32_t y);                    /* @bin 0x0047A120 */
void rubble_rect(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1); /* @bin 0x00477890 */

#endif
