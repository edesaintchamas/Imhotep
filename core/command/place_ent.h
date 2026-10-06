/* Imhotep - entertainment venues of the construction place command (package place).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_PLACE_ENT_H
#define IMHOTEP_COMMAND_PLACE_ENT_H

#include <stdint.h>

#include "state/state.h"

void ent_roads_4413b0(game_state *st, int32_t x, int32_t y, int32_t type); /* @bin 0x004413B0 */
int32_t ent_fits_43f410(game_state *st, int32_t type);                    /* @bin 0x0043F410 */
/* @bin 0x00441130 (the original's outputs are its caller's stack) */
int32_t ent_spot_441130(game_state *st, int32_t x, int32_t y, int32_t *ox, int32_t *oy, int32_t type, int32_t part);
int32_t place_booth(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2);     /* @bin 0x00440700 */
int32_t place_bandstand(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2); /* @bin 0x00440850 */
int32_t place_pavilion(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2, int32_t y2);  /* @bin 0x00440B10 */
int32_t place_festival_square(game_state *st, int32_t preview, int32_t x1, int32_t y1, int32_t x2,
                              int32_t y2); /* @bin 0x00440ED0 */

#endif
