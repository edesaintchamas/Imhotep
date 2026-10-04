/* Imhotep - small functions of the tick (Pharaoh.exe).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "sim/small.h"

void anim_counters(game_state *st)
{
    /* @bin 0x004CDC70 ten cycling counters (period 4, 8, 16, 24, 32, 64, 128, 256, 384, 768) */
    static const struct { uint32_t addr; int32_t max; } C[] = {
        {0x00D3B430u, 3}, {0x00D3B42Cu, 7}, {0x00D3B428u, 0xF}, {0x00D3B424u, 0x17}, {0x00D3B420u, 0x1F},
        {0x00D3B41Cu, 0x3F}, {0x00D3B418u, 0x7F}, {0x00D3B414u, 0xFF}, {0x00D3B410u, 0x17F}, {0x00D3B40Cu, 0x2FF},
    };
    for (unsigned i = 0; i < sizeof(C) / sizeof(C[0]); i++) {
        if (++S32(st, C[i].addr) > C[i].max) {
            S32(st, C[i].addr) = 0;
        }
    }
}

void clear_0x0078d784(game_state *st)
{
    S32(st, 0x0078D784u) = 0; /* @bin 0x00452150 */
}

void countdown_0x00ea89dc(game_state *st)
{
    /* @bin 0x004E7E40 i16 0x00EA89DC decremented while > 0 */
    if (S16(st, 0x00EA89DCu) > 0) {
        S16(st, 0x00EA89DCu) -= 1;
    }
}
