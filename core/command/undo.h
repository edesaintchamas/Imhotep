/* Imhotep - undo of the last construction or clearing (player command, docs/commands.md 2.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_COMMAND_UNDO_H
#define IMHOTEP_COMMAND_UNDO_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00485080 forget the buildings kept for undo (state 6 back to 1, +0xBF := 0), empty the list */
void undo_forget(game_state *st);

/* @bin 0x00485830 the image grid restored from its backup on tiles without a building (arg unused) */
void undo_restore_images(game_state *st, int32_t unused);

/* @bin 0x00485890 every house (types 0x0A..0x1D) with +0x1C = 0 refreshed (0x0046CF40) */
void undo_houses_refresh(game_state *st);

/* @bin 0x004E0C80 building 0xDE: its 0x74 parts redrawn (0x004DFBB0) */
void undo_monument_de_redraw(game_state *st, int32_t id);

/* @bin 0x004858E0 a building put back by the undo of a clearing: on the map again, its links
 * taken again, the city's unique-building records set */
void undo_building_readd(game_state *st, int32_t id);

/* @bin 0x0045AFE0 goods (res, amount) given to storage yards and granaries; dry != 0: only counted.
 * Returns the amount placed. */
int32_t goods_to_storage(game_state *st, int32_t res, int32_t amount, int32_t dry);

/* @bin 0x00485230 undo the last construction / clearing */
void undo_restore(game_state *st);

#endif
