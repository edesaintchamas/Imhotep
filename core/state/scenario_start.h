/* Imhotep - scenario start (Pharaoh.exe 0x004D52A0): the runtime state the game derives when a
 * scenario or a saved game is started. Ported step by step (docs/formats/state.md).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_STATE_SCENARIO_START_H
#define IMHOTEP_STATE_SCENARIO_START_H

#include "state/load_game.h"
#include "state/state.h"

/* @bin 0x004D52A0 (prologue) map size and grid offsets from the scenario info (chunk 75), the
 * image grid cleared, then the map row ranges 0x0046F7B0. */
void scenario_start_map(game_state *st);

/* @bin 0x004D52A0 the scenario start, as far as ported (each step in the original's order; the
 * steps not yet ported are listed in docs/formats/state.md). Returns 0, or -1 when a game file
 * it reads is missing. */
int scenario_start(game_state *st, const game_env *env);

/* @bin 0x0046F7B0 visible map rectangle and per-row ranges of the diamond-shaped map. */
void map_row_ranges(game_state *st);

#endif
