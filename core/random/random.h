/* Imhotep - the game's random number generator (Pharaoh.exe 0x004D7090).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Format of the saved state: docs/formats/map.md §4 (chunks 10, 11), save.md §3 (records 41, 42). */
#ifndef IMHOTEP_RANDOM_RANDOM_H
#define IMHOTEP_RANDOM_RANDOM_H

#include <stdint.h>

#include "state/state.h"

#define RANDOM_POOL_SIZE 100 /* @bin 0x004D7090 index wraps at 0x64 */

typedef struct {
    uint32_t iv1;          /* 0x00D3B384, map chunk 10, save record 41 */
    uint32_t iv2;          /* 0x00D3B380, map chunk 11, save record 42 */
    uint32_t r1_15bit;     /* 0x00D3B370 */
    uint32_t r1_7bit;      /* 0x00D3B374 */
    uint32_t r1_3bit;      /* 0x00D3B378 */
    uint32_t r2_15bit;     /* 0x00D3B364 */
    uint32_t r2_7bit;      /* 0x00D3B368 */
    uint32_t r2_3bit;      /* 0x00D3B36C */
    int32_t pool_index;    /* 0x00D3B1C4 */
    uint32_t pool[RANDOM_POOL_SIZE]; /* 0x00D3B1C8, u32 entries */
} random_state;

/* @bin 0x004D7090 one step, done at the start of every game tick (0x004D6B60). */
void random_next(random_state *r);

/* The generator's variables in the game state (addresses above), as a random_state. */
void random_from_state(const game_state *st, random_state *r);
void random_to_state(game_state *st, const random_state *r);
/* One step on the game state (the tick's step, also run by 0x0046FC80). */
void random_next_state(game_state *st);

#endif
