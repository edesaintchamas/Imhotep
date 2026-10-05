/* Imhotep - workers given to the buildings, by labour category.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_LABOUR_H
#define IMHOTEP_CITY_LABOUR_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00459670 share the city's workers among the buildings (+0x40), category by category
 * (day step 0x19); then type-0x88 buildings retrace their links */
void labour_allocate(game_state *st);
/* @bin 0x00459240 does the city hold any of the building's resource (+0x43) */
int32_t building_resource_in_stock(game_state *st, int32_t id);

#endif
