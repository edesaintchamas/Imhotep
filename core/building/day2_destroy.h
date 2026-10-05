/* Imhotep - buildings destroyed: their tiles cleared or turned to rubble (package day2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_DAY2_DESTROY_H
#define IMHOTEP_BUILDING_DAY2_DESTROY_H

#include <stdint.h>

#include "state/state.h"

int32_t footprint_corner(game_state *st, int32_t x, int32_t y);                                    /* @bin 0x0047C7B0 */
int32_t building_part_size(game_state *st, int32_t b);                                             /* @bin 0x00541AB0 */
void building_clear_tiles(game_state *st, int32_t b, int32_t x, int32_t y);                       /* @bin 0x0047C480 */
void building_rubble_tiles(game_state *st, int32_t b, int32_t x, int32_t y, int32_t w, int32_t h); /* @bin 0x0047C2B0 */
void building_destroy_linked(game_state *st, int32_t b, int32_t collapse);                         /* @bin 0x004695D0 */

int32_t building_main_part_46d0f0(game_state *st, int32_t b);                                     /* @bin 0x0046D0F0 */
void building_collapse(game_state *st, int32_t b, int32_t flag);                                    /* @bin 0x0046D100 */

void building_release(game_state *st, int32_t b);                                                   /* @bin 0x0046AC50 */

void building_clear_area(game_state *st, int32_t b, int32_t type);                                   /* @bin 0x0043FD00 */

void house_split(game_state *st, int32_t b);                                                        /* @bin 0x0046C500 */

#endif
