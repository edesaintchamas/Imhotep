/* Imhotep - per-building records rebuilt after a load (Pharaoh.exe 0x0046B320, 0x0046F030,
 * 0x0046A320) and their constructors.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_STATE_POST_LOAD_H
#define IMHOTEP_STATE_POST_LOAD_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x0046B100 new storage record (granaries, storage yards: types 0x46-0x48, 0xBE, 0xBF,
 * 0xC1) for a building; its id, or 0 when all 199 are used */
int32_t storage_create(game_state *st, int32_t building_id);
/* @bin 0x0046B320 storage records <-> buildings (+0xC1) */
void load_rebuild_storage_links(game_state *st);

/* @bin 0x0046EFD0 new record for a type-0x88 building; its id or 0 */
int32_t rec88_create(game_state *st, int32_t building_id);
/* @bin 0x0046F030 type-0x88 records <-> buildings (+0xCA) */
void load_rebuild_0x88_links(game_state *st);

/* @bin 0x0046A280 new record for a type-0x4A building (sets the building's +0xCB) */
void rec4a_create(game_state *st, int32_t building_id);
/* @bin 0x0046A320 type-0x4A records <-> buildings (+0xCB) */
void load_rebuild_0x4a_links(game_state *st);

#endif
