/* Imhotep - the C runtime's rand() as statically linked into Pharaoh.exe.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Its state is a global of the exe, not saved (docs/formats/save.md, figures: determinism note). */
#ifndef IMHOTEP_RANDOM_CRT_RAND_H
#define IMHOTEP_RANDOM_CRT_RAND_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x00562348 rand(): s = s * 214013 + 2531011; returns (s >> 16) & 0x7FFF */
int32_t crt_rand(game_state *st);
/* @bin 0x0056233E srand(seed): s = seed (the figure actions reseed it with timeGetTime()) */
void crt_srand(game_state *st, uint32_t seed);

#endif
