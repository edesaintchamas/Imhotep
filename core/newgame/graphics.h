/* Imhotep - the graphics slots chosen when a scenario starts (package initfig, docs/newgame.md §6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_NEWGAME_GRAPHICS_H
#define IMHOTEP_NEWGAME_GRAPHICS_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004F4520 the monument slots 0x11..0x1D by the scenario's monument mode 0x00784CBC */
void newgame_monument_slots(game_state *st);

/* @bin 0x0053D8E0 the second value of the first of the 2 scenario entries 0x00784DE0 whose record
 * 0x005D19C8 has kind 2 and a value below 5; 1 when none */
int32_t newgame_monument_kind2(game_state *st);

/* @bin 0x0053D970 the extra slot 9 (sphinx, obelisk, sun temple or mausoleum graphics) */
void newgame_extra_slot(game_state *st);

#endif
