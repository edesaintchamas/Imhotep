/* Imhotep - the figure tick: every figure's action, deletion of dead figures.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_FIGURE_H
#define IMHOTEP_FIGURE_FIGURE_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00489B70 run every live figure's action (table 0x005DC28C by type), delete the dead */
void figure_update_all(game_state *st);
/* @bin 0x004B4980 release what the figure holds (building slots, counters) and clear its record */
void figure_delete(game_state *st, int32_t id);
/* @bin 0x00489A50, 0x00489A80 profiling of one action (not read by the simulation) */
void figure_profile_start(game_state *st, int32_t type);
void figure_profile_end(game_state *st);

#endif
