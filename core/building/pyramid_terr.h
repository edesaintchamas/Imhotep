/* Imhotep - pyramid parts: building up, redraw, the step pyramid's chapel (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_PYRAMID_TERR_H
#define IMHOTEP_BUILDING_PYRAMID_TERR_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0040F860 (src, dst, n) the game's own memcpy: 64-byte blocks of dwords, then bytes */
void copy_bytes(game_state *st, uint32_t src, uint32_t dst, int32_t n);

/* @bin 0x00485100 (id) building id's record is kept in the list of removed buildings (0x00BA746C
 * ids, 0x00BA7888 records) unless already there; 1 when added */
int32_t building_keep_removed(game_state *st, int32_t id);

void pyramid_chapel_mark(game_state *st, int32_t id, int32_t remove); /* @bin 0x004F5E70 */
void pyramid_part_work_done(game_state *st, int32_t part);           /* @bin 0x004F31F0 */
void pyramid_part_redraw(game_state *st, int32_t part, int32_t half, int32_t kind); /* @bin 0x004EDF50 */
void pyramid_part_redraw_stage(game_state *st, int32_t part, int32_t half, int32_t kind); /* @bin 0x004F4160 */
void pyramid_reset_redraw(game_state *st, int32_t id);               /* @bin 0x004EE5D0 */
void pyramid_check_level_done(game_state *st, int32_t id);           /* @bin 0x004EE630 */
int32_t pyramid_block_laid(game_state *st, int32_t x, int32_t y);    /* @bin 0x004F3690 */
int32_t pyramid_lay_next_block(game_state *st, int32_t id);          /* @bin 0x004F3850 */

#endif
