/* Imhotep - daily passes over the houses (dispatcher steps of 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_BUILDING_HOUSE_H
#define IMHOTEP_BUILDING_HOUSE_H

#include "state/state.h"

/* @bin 0x00454E20 service access counters of every house go down by one (day step 0x23) */
void house_service_decay(game_state *st);
/* @bin 0x00454FA0 house access ratings from the counters (day step 0x24) */
void house_access_ratings(game_state *st);

#endif
