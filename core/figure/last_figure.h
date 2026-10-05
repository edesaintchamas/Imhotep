/* Imhotep - figure helpers left over by the earlier packages (package last).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_FIGURE_LAST_FIGURE_H
#define IMHOTEP_FIGURE_LAST_FIGURE_H

#include <stdint.h>

#include "state/state.h"

int32_t figure_unload_to_works(game_state *st, int32_t building, int32_t good, int32_t route, int32_t figure); /* @bin 0x00445E50 */
int32_t building_nearest_0x458c30(game_state *st, int32_t x, int32_t y);  /* @bin 0x00458C30 */
void figure_hit_target(game_state *st, int32_t attacker, int32_t target); /* @bin 0x004BC4A0 */
int32_t city_list_index_0x480860(game_state *st, int32_t value);          /* @bin 0x00480860 */

#endif
