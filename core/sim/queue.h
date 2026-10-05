/* Imhotep - small container classes of the game (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_SIM_QUEUE_H
#define IMHOTEP_SIM_QUEUE_H

#include <stdint.h>

#include "state/state.h"

/* fixed ring (+4 head, +8 tail, +0x0C size, +0x14 slots), thiscall on q */
void queue_clear(game_state *st, uint32_t q);                         /* @bin 0x004F6560 */
int32_t queue_contains(game_state *st, uint32_t q, int32_t value);    /* @bin 0x004F6570 */
int32_t queue_next(game_state *st, uint32_t q, uint32_t cursor);      /* @bin 0x004F65F0 */
int32_t queue_pop(game_state *st, uint32_t q);                        /* @bin 0x004F6620 */
/* growable ring (+4 head, +0x0C count, +0x10 capacity, +0x14 slot pointer), thiscall on r */
int32_t ring_wrap(game_state *st, uint32_t r, uint32_t index);        /* @bin 0x004F6A10 */
int32_t ring_pop(game_state *st, uint32_t r);                         /* @bin 0x004F68B0 */

#endif
