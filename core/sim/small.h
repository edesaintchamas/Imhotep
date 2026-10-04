/* Imhotep - small functions of the tick (Pharaoh.exe).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_SIM_SMALL_H
#define IMHOTEP_SIM_SMALL_H

#include "state/state.h"

void anim_counters(game_state *st);       /* @bin 0x004CDC70 */
void clear_0x0078d784(game_state *st);    /* @bin 0x00452150 */
void countdown_0x00ea89dc(game_state *st); /* @bin 0x004E7E40 */

#endif
