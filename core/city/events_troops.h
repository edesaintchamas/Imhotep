/* Imhotep - the troops of an invasion or a military request event (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_EVENTS_TROOPS_H
#define IMHOTEP_CITY_EVENTS_TROOPS_H

#include <stdint.h>

#include "state/state.h"

void figure_init_military(game_state *st, int32_t f); /* @bin 0x00446650 */
/* @bin 0x00446A30 the troops of event `ev`; the entry tile or -1 */
int32_t event_troops(game_state *st, int32_t kind, int32_t amount, int32_t where, int32_t arg4, int32_t ev);

#endif
