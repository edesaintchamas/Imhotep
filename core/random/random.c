/* Imhotep - the game's random number generator (Pharaoh.exe 0x004D7090).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "random/random.h"

void random_next(random_state *r)
{
    /* @bin 0x004D7090 push the previous 7-bit value into the pool (dword store, index wraps at 100) */
    r->pool[r->pool_index] = r->r1_7bit;
    r->pool_index++;
    if (r->pool_index >= RANDOM_POOL_SIZE) {
        r->pool_index = 0;
    }
    /* @bin 0x004D7090 two 31-bit LFSRs, 31 steps each: logical shift right, bit 30 = bit0 ^ bit4 */
    for (int i = 0; i < 31; i++) {
        uint32_t b1 = ((r->iv1 >> 4) ^ r->iv1) & 1;
        uint32_t b2 = ((r->iv2 >> 4) ^ r->iv2) & 1;
        r->iv1 >>= 1;
        r->iv2 >>= 1;
        if (b1) {
            r->iv1 |= 0x40000000u;
        }
        if (b2) {
            r->iv2 |= 0x40000000u;
        }
    }
    /* @bin 0x004D7090 derived values */
    r->r1_15bit = r->iv1 & 0x7FFF;
    r->r1_7bit = r->iv1 & 0x7F;
    r->r1_3bit = r->iv1 & 7;
    r->r2_15bit = r->iv2 & 0x7FFF;
    r->r2_7bit = r->iv2 & 0x7F;
    r->r2_3bit = r->iv2 & 7;
}

void random_from_state(const game_state *st, random_state *r)
{
    r->iv1 = U32(st, 0x00D3B384u);
    r->iv2 = U32(st, 0x00D3B380u);
    r->r1_15bit = U32(st, 0x00D3B370u);
    r->r1_7bit = U32(st, 0x00D3B374u);
    r->r1_3bit = U32(st, 0x00D3B378u);
    r->r2_15bit = U32(st, 0x00D3B364u);
    r->r2_7bit = U32(st, 0x00D3B368u);
    r->r2_3bit = U32(st, 0x00D3B36Cu);
    r->pool_index = S32(st, 0x00D3B1C4u);
    for (int i = 0; i < RANDOM_POOL_SIZE; i++) {
        r->pool[i] = U32(st, 0x00D3B1C8u + 4u * (uint32_t) i);
    }
}

void random_to_state(game_state *st, const random_state *r)
{
    U32(st, 0x00D3B384u) = r->iv1;
    U32(st, 0x00D3B380u) = r->iv2;
    U32(st, 0x00D3B370u) = r->r1_15bit;
    U32(st, 0x00D3B374u) = r->r1_7bit;
    U32(st, 0x00D3B378u) = r->r1_3bit;
    U32(st, 0x00D3B364u) = r->r2_15bit;
    U32(st, 0x00D3B368u) = r->r2_7bit;
    U32(st, 0x00D3B36Cu) = r->r2_3bit;
    S32(st, 0x00D3B1C4u) = r->pool_index;
    for (int i = 0; i < RANDOM_POOL_SIZE; i++) {
        U32(st, 0x00D3B1C8u + 4u * (uint32_t) i) = r->pool[i];
    }
}

void random_next_state(game_state *st)
{
    random_state r;
    random_from_state(st, &r);
    random_next(&r);
    random_to_state(st, &r);
}
