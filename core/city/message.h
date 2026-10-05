/* Imhotep - the city message list (chunk of 1000 records of 0x30 bytes at 0x00E95228).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#ifndef IMHOTEP_CITY_MESSAGE_H
#define IMHOTEP_CITY_MESSAGE_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004E1640 sort the messages (1000 bubble passes), count them, keep the current one */
void message_sort(game_state *st);

/* @bin 0x0040F820 the game's memcpy(src, dst, n): 8 bytes at a time as two dwords, then bytes */
void game_copy(game_state *st, uint32_t src, uint32_t dst, int32_t n);

#endif
