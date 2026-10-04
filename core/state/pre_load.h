/* Imhotep - what the game does before reading a saved game (Pharaoh.exe 0x004D5100 prologue).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_STATE_PRE_LOAD_H
#define IMHOTEP_STATE_PRE_LOAD_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004BD330 floodplain settings (this = 0x00D38360) := defaults */
void floodplain_settings_reset(game_state *st);

/* @bin 0x0046FA40 grids cleared, floodplain manager reset, random grid re-rolled, image grid
 * filled from it (uses the map globals of the previous map) */
void pre_load_clear(game_state *st);

/* @bin 0x0046F930 1 if (x, y) is inside the diamond-shaped map */
int map_tile_inside(const game_state *st, int32_t x, int32_t y);

#endif
