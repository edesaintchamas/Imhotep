/* Imhotep - saved games <-> game state.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_STATE_STATE_SAVE_H
#define IMHOTEP_STATE_STATE_SAVE_H

#include "io/save_file.h"
#include "state/state.h"

/* @bin 0x004D0E10 each chunk of a save (version-160 layout, after save_file_load) is the bytes of
 * the variable at save_schema[i].address: copy them there. */
void state_load_chunks(game_state *st, const save_file *s);

#endif
