/* Imhotep - the terrain checks of the construction preview (package ui).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_CONSTRUCTION_CHECKS_H
#define IMHOTEP_COMMAND_CONSTRUCTION_CHECKS_H

#include <stdint.h>

#include "state/state.h"

int32_t terrain_scan_predicate(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d,
                               uint32_t predicate);                                     /* @bin 0x00482720 */
int32_t terrain_all_mask(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, uint32_t mask); /* @bin 0x004828E0 */
int32_t terrain_count_mask(game_state *st, int32_t x, int32_t y, int32_t n, int32_t d, uint32_t mask,
                           int32_t count);                                              /* @bin 0x004829B0 */
void ghost_mark(game_state *st);                                                        /* @bin 0x0047D4D0 */
void ghost_footprint(game_state *st, int32_t n, int32_t x, int32_t y, uint32_t mask, int32_t no_anchor,
                     int32_t floodable);                                                /* @bin 0x0047D360 */
int32_t footprint_fits(game_state *st, int32_t n, int32_t x, int32_t y, uint32_t mask,
                       int32_t floodable);                                              /* @bin 0x0047D530 */
void gatehouse_ghost(game_state *st, int32_t x, int32_t y, int32_t rotation);           /* @bin 0x004BFA50 */
int32_t ferry_landing_at(game_state *st, int32_t x, int32_t y);                         /* @bin 0x00486BF0 */
int32_t ferry_landing_at_offset(game_state *st, int32_t offset);                        /* @bin 0x00486CF0 */

#endif
