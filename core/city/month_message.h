/* Imhotep - posting a city message (0x004E0CF0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_MONTH_MESSAGE_H
#define IMHOTEP_CITY_MONTH_MESSAGE_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004E0CF0 post message `id` with two parameters and an optional 30-byte block (the
 * original's pointer argument: NULL or the bytes it points to); `arg5`: the original's fifth stack
 * argument (its low byte: post even when the settings filter it) */
void message_post(game_state *st, int32_t id, int32_t a, int32_t b, const uint8_t *block, uint32_t arg5);
/* @bin 0x004E0F90 does the setting 0x00E38EF0 let message `id` through (else a short notice) */
int32_t message_allowed(game_state *st, int32_t id, const uint8_t *block);
/* @bin 0x004E1750 the first free message slot, or -1 */
int32_t message_free_slot(game_state *st);
/* @bin 0x004E14B0 0x00D3A6E8 by the message id */
void message_kind(game_state *st, int32_t id);

#endif
