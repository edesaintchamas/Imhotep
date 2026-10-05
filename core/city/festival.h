/* Imhotep - the festival's performers (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_FESTIVAL_H
#define IMHOTEP_CITY_FESTIVAL_H

#include <stdint.h>

#include "state/state.h"

void festival_performers(game_state *st, int32_t amount); /* @bin 0x00461950 */
void festival_performer(game_state *st, int32_t building); /* @bin 0x00461B20 */

#endif
