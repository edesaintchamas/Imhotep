/* Imhotep - write a saved game from the game state (Pharaoh.exe 0x004D4E20).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Format: docs/formats/save.md. Check: tests/differential/oracle_save_check.py (the file equals
 * the one the original's saver writes from the same state). */
#ifndef IMHOTEP_STATE_SAVE_GAME_H
#define IMHOTEP_STATE_SAVE_GAME_H

#include <stddef.h>
#include <stdint.h>

#include "state/state.h"

/* @bin 0x004D4E20 save: the state is prepared as the game does it before writing (shipwright
 * queues packed, version, player name, terrain flag 0x10, route buffers released, schema), then
 * the 181 chunks are written in schema order. *out is malloc'd. Returns 0, -1 when out of memory.
 * The steps after writing (route rebuild, the "game saved" notice) are not done here. */
int game_save(game_state *st, uint8_t **out, size_t *out_len);

/* @bin 0x004771C0 terrain flag 0x10 becomes 1 on the map's rows */
void terrain_flag10_to_1(game_state *st);

#endif
