/* Imhotep - saved games <-> game state.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/state_save.h"

#include "io/save_schema.h"

/* @bin 0x004D0E10 chunk i is read into the variable at save_schema[i].address */
void state_load_chunks(game_state *st, const save_file *s)
{
    for (int i = 0; i < SAVE_SCHEMA_RECORDS; i++) {
        const save_schema_record *t = &save_schema[i];
        state_load_section(st, t->address, s->chunk[i], (size_t) t->elem_size * t->elem_count);
    }
}
