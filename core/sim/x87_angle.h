/* Imhotep - the direction from one tile to another, computed by the original in x87 doubles.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_SIM_X87_ANGLE_H
#define IMHOTEP_SIM_X87_ANGLE_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0051CAE0 the direction (0..32) of the vector (x2 - x1, y1 - y2) as the original's x87
 * code computes it (asin, 53-bit precision); 0 for a null vector. Exact for |x2 - x1| and
 * |y1 - y2| below 2^26; *ok is set to 0 outside that range (and the result is meaningless). */
int32_t x87_direction(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int *ok);

/* The port (state unused): state_unported outside the exact range. */
int32_t x87_direction_port(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

#endif
