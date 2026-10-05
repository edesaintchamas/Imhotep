/* Imhotep - helpers left on the tick's frontier (package rest).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_REST_MISC_H
#define IMHOTEP_CITY_REST_MISC_H

#include "state/state.h"

int32_t nearest_point_480c60(game_state *st, int32_t f);    /* @bin 0x00480C60 */
int32_t figure_area_free_480d60(game_state *st, int32_t f); /* @bin 0x00480D60 */

#endif
