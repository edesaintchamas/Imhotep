/* Imhotep - the game's growable array of i32 (Pharaoh.exe 0x004F6700 family).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Object in the state: +0x04 = 1 when reset, +0x08, +0x0C (meaning per use), +0x10 capacity,
 * +0x14 pointer to the elements (heap arena). */
#ifndef IMHOTEP_STATE_VEC_H
#define IMHOTEP_STATE_VEC_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004F6760 release the elements (+0x14 := 0) and reset the fields */
void vec_reset(game_state *st, uint32_t vec);
/* @bin 0x004F6700 make room for n elements: when n > capacity, reset, then a zeroed block of n
 * elements; returns 1 */
int32_t vec_reserve(game_state *st, uint32_t vec, int32_t n);

#endif
