/* Imhotep - per-tile routing grids (Pharaoh.exe 0x00517370 .. 0x00517A20 and their fillers).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_MAP_ROUTING_H
#define IMHOTEP_MAP_ROUTING_H

#include <stdint.h>

#include "state/state.h"

/* u8 per tile, 228 x 228 (not saved; rebuilt at scenario start and during play) */
#define ROUTING_EBAB20 0x00EBAB20u
#define ROUTING_EC7630 0x00EC7630u
#define ROUTING_ED4140 0x00ED4140u
#define ROUTING_EE0C50 0x00EE0C50u

void routing_clear_ee0c50(game_state *st);    /* @bin 0x00517A20 */
void routing_rebuild_ed4140(game_state *st);  /* @bin 0x005178B0 */
void routing_rebuild_ebab20(game_state *st);  /* @bin 0x005178E0 */
void routing_rebuild_ec7630(game_state *st);  /* @bin 0x00517660 */
void routing_rebuild_ee0c50(game_state *st);  /* @bin 0x00517370 */

/* the rectangle fillers (map coordinates, clamped to the map) */
void routing_fill_ed4140(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1);           /* @bin 0x00517690 */
void routing_fill_ec7630(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1);           /* @bin 0x005173A0 */
void routing_fill_ee0c50(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int flag); /* @bin 0x00516F90 */

#endif
