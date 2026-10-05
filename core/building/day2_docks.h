/* Imhotep - docks and storage helpers (package day2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_DAY2_DOCKS_H
#define IMHOTEP_BUILDING_DAY2_DOCKS_H

#include <stdint.h>

#include "state/state.h"

int32_t dock_reserve(game_state *st, int32_t f);              /* @bin 0x004808A0 */
int32_t dock_free_moorings(game_state *st, int32_t f);        /* @bin 0x004809B0 */
int32_t building_fill(game_state *st, int32_t b, int32_t amount, int32_t kind); /* @bin 0x00460890 */

#endif
