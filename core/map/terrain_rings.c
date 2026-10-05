/* Imhotep - the tile ring offset tables the game builds at start-up (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x00540B50 (game start-up) calls 0x00481F20, which fills the ring tables read by the ring
 * walks (0x00481490 trees, 0x00481AA0, ...). A state built from the exe image alone
 * (tests/differential/terrain_check.c) has them empty: core/map/terrain.c builds them when it
 * loads a map. */
#include "map/terrain_rings.h"

#include "state/ported.h"

void rings_1x1_build(game_state *st)
{
    /* @bin 0x0046F8D0 ring k (1..12): the border of the square -k..k, rows (dy) outer, dx inner,
     * written at the k-th pointer of 0x005DBBD8 */
    for (int32_t k = 1; k <= 12; k++) {
        uint32_t p = U32(st, 0x005DBBD8u + 4u * (uint32_t) (k - 1));
        for (int32_t dy = -k; dy <= k; dy++) {
            for (int32_t dx = -k; dx <= k; dx++) {
                if (dx == -k || dx == k || dy == -k || dy == k) {
                    U8(st, p) = (uint8_t) dx;
                    U8(st, p + 1) = (uint8_t) dy;
                    p += 2;
                }
            }
        }
    }
}

static void offsets(game_state *st, uint32_t counts, int rings, uint32_t dest, const uint32_t *sources)
{
    /* @bin 0x00481F20 ring r: counts[r] (word) offsets (int8 dx) + (int8 dy) * 228, packed */
    uint32_t out = dest;
    for (int r = 0; r < rings; r++) {
        int32_t n = S16(st, counts + 2u * (uint32_t) r);
        for (int32_t i = 0; i < n; i++) {
            int16_t dx = (int8_t) U8(st, sources[r] + 2u * (uint32_t) i);
            int16_t dy = (int8_t) U8(st, sources[r] + 2u * (uint32_t) i + 1);
            S16(st, out) = (int16_t) (dx + dy * 0xE4);
            out += 2;
        }
    }
}

void ring_offsets_build(game_state *st)
{
    /* @bin 0x00481F20 the source tables are the ones its code names (1x1: the rings of
     * 0x0046F8D0; larger footprints: static data) */
    static const uint32_t S1[12] = {0x0070BF7Cu, 0x0070BF5Cu, 0x0070BF2Cu, 0x0070BEECu, 0x0070BE9Cu, 0x0070BE3Cu,
                                    0x0070BDCCu, 0x0070BD4Cu, 0x0070BCBCu, 0x0070BC1Cu, 0x0070BB6Cu, 0x0070BAACu};
    static const uint32_t S2[6] = {0x005C9BA8u, 0x005C9BC0u, 0x005C9BE8u, 0x005C9C20u, 0x005C9C68u, 0x005C9CC0u};
    static const uint32_t S3[6] = {0x005C9D28u, 0x005C9D48u, 0x005C9D78u, 0x005C9DB8u, 0x005C9E08u, 0x005C9E68u};
    static const uint32_t S4[6] = {0x005C9ED8u, 0x005C9F00u, 0x005C9F38u, 0x005C9F80u, 0x005C9FD8u, 0x005CA040u};
    static const uint32_t S5[6] = {0x005CA0B8u, 0x005CA0E8u, 0x005CA128u, 0x005CA178u, 0x005CA1D8u, 0x005CA248u};
    static const uint32_t S6[6] = {0x005CA2C8u, 0x005CA300u, 0x005CA348u, 0x005CA3A0u, 0x005CA408u, 0x005CA480u};
    rings_1x1_build(st);
    offsets(st, 0x005C9B00u, 12, 0x0070C8ECu, S1);
    offsets(st, 0x005C9B18u, 6, 0x0070C76Cu, S2);
    offsets(st, 0x005C9B24u, 6, 0x0070C5BCu, S3);
    offsets(st, 0x005C9B30u, 6, 0x0070C3DCu, S4);
    offsets(st, 0x005C9B3Cu, 6, 0x0070C1CCu, S5);
    offsets(st, 0x005C9B48u, 6, 0x0070BF8Cu, S6);
}

static int32_t run_rings_1x1(game_state *st, const int32_t *a)
{
    (void) a;
    rings_1x1_build(st);
    return 0;
}

static int32_t run_ring_offsets(game_state *st, const int32_t *a)
{
    (void) a;
    ring_offsets_build(st);
    return 0;
}

const ported_function terr_rings_ports[] = {
    {0x0046F8D0u, "rings_1x1_build", 0, 0, 0, run_rings_1x1, 0},
    {0x00481F20u, "ring_offsets_build", 0, 0, 0, run_ring_offsets, 0},
};
const int terr_rings_ports_count = sizeof(terr_rings_ports) / sizeof(terr_rings_ports[0]);
