/* Imhotep - small functions of the tick (Pharaoh.exe).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "sim/small.h"

#include "state/layout.h"

void anim_counters(game_state *st)
{
    /* @bin 0x004CDC70 ten cycling counters (period 4, 8, 16, 24, 32, 64, 128, 256, 384, 768) */
    static const struct { uint32_t addr; int32_t max; } C[] = {
        {G_ANIM_COUNTER_4, 3}, {G_ANIM_COUNTER_8, 7}, {G_ANIM_COUNTER_16, 0xF}, {G_ANIM_COUNTER_24, 0x17},
        {G_ANIM_COUNTER_32, 0x1F}, {G_ANIM_COUNTER_64, 0x3F}, {G_ANIM_COUNTER_128, 0x7F}, {G_ANIM_COUNTER_256, 0xFF},
        {G_ANIM_COUNTER_384, 0x17F}, {G_ANIM_COUNTER_768, 0x2FF},
    };
    for (unsigned i = 0; i < sizeof(C) / sizeof(C[0]); i++) {
        if (++S32(st, C[i].addr) > C[i].max) {
            S32(st, C[i].addr) = 0;
        }
    }
}

void clear_0x0078d784(game_state *st)
{
    S32(st, G_U78D784) = 0; /* @bin 0x00452150 */
}

void countdown_0x00ea89dc(game_state *st)
{
    /* @bin 0x004E7E40 i16 0x00EA89DC decremented while > 0 */
    if (S16(st, CITY_ADDR(1) + CITY_U2794) > 0) {
        S16(st, CITY_ADDR(1) + CITY_U2794) -= 1;
    }
}
