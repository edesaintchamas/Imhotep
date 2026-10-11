/* Imhotep - the new-game reset of a custom scenario start (docs/newgame.md section 3.3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_NEWGAME_RESET_H
#define IMHOTEP_NEWGAME_RESET_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00540A10 the reset before a custom map: every record cleared, then the scenario setup
 * 0x004DE820(path), the player name and the treasury scaled by the difficulty. path: the address
 * of "maps\\<name>" in the state. Returns the new treasury (eax). */
int32_t newgame_reset(game_state *st, uint32_t path);

/* @bin 0x00540B50 the view, the random generator and the UI text boxes of a new city */
void newgame_view_reset(game_state *st);

/* @bin 0x004E7D10 city records cleared and initialised */
void city_records_init(game_state *st);
/* @bin 0x004E9EE0 */
void city_records_init_9ee0(game_state *st);

/* @bin 0x0041D730 the ambient sound slots (save record 92, 0x00702900) */
void ambient_slots_reset(game_state *st);

/* @bin 0x004303A0 the view geometry (eight values) */
/* @imhotep enhanced fit: the view's columns and rows for a sidebar at x `right` and a screen height h */
void view_tiles(int32_t right, int32_t h, int32_t *cols, int32_t *rows);
void view_geometry_set(game_state *st, int32_t a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4,
                       int32_t a5, int32_t a6, int32_t a7);

/* @bin 0x004CDD60 the game generator advanced 100 steps from pool index 0 */
void random_warm_up(game_state *st);

/* @bin 0x004CC560 a UI text box's parameters */
void text_box_set(game_state *st, int32_t box, int32_t text, int32_t a2, int32_t a3, int32_t a4,
                  int32_t a5);

#endif
