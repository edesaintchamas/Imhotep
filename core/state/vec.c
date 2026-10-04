/* Imhotep - the game's growable array of i32 (Pharaoh.exe 0x004F6700 family).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/vec.h"

void vec_reset(game_state *st, uint32_t vec)
{
    /* @bin 0x004F6760 */
    if (U32(st, vec + 0x14) != 0) {
        state_dealloc(st, U32(st, vec + 0x14));
        U32(st, vec + 0x14) = 0;
    }
    S32(st, vec + 0x10) = 0;
    S32(st, vec + 0x08) = 0;
    S32(st, vec + 0x0C) = 0;
    S32(st, vec + 0x04) = 1;
}

int32_t vec_reserve(game_state *st, uint32_t vec, int32_t n)
{
    /* @bin 0x004F6700 (signed comparison with the capacity) */
    if (n <= S32(st, vec + 0x10)) {
        return 1;
    }
    vec_reset(st, vec);
    S32(st, vec + 0x10) = n;
    U32(st, vec + 0x14) = state_alloc(st, (uint32_t) n * 4); /* new: zero-filled by the arena */
    S32(st, vec + 0x04) = 1;
    S32(st, vec + 0x08) = 0;
    S32(st, vec + 0x0C) = 0;
    return 1;
}
