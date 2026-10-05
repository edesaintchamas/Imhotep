/* Imhotep - the monument rating (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_MONUMENT_RATING_H
#define IMHOTEP_CITY_MONUMENT_RATING_H

#include <stdint.h>

#include "city/day_x87.h"
#include "state/state.h"

int32_t monuments_planned(game_state *st); /* @bin 0x004F4FF0 */
/* @bin 0x004F78A0 the rating value (an x87 double): 1, or -1 for the NaN of a negative sum, or 0
 * when it is left to the original */
int monument_rating_value(game_state *st, int32_t planned, x87_val *out);
void pyramid_status(game_state *st, int32_t b, int32_t *out); /* @bin 0x004F1310 out: 0x1C i32 */
void monument_rating_update(game_state *st); /* @bin 0x0046EF50 */

#endif
