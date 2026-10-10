/* Imhotep - the tile ring offset tables the game builds at start-up (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x00540B50 (game start-up) calls 0x00481F20, which fills the ring tables read by the ring
 * walks (0x00481490 trees, 0x00481AA0, ...). A state built from the exe image alone
 * (tests/differential/terrain_check.c) has them empty: core/map/terrain.c builds them when it
 * loads a map. */
#include "map/terrain_rings.h"

#include "state/layout.h"
#include "state/ported.h"

void rings_1x1_build(game_state *st)
{
    /* @bin 0x0046F8D0 ring k (1..12): the border of the square -k..k, rows (dy) outer, dx inner,
     * written at the k-th pointer of 0x005DBBD8 */
    for (int32_t k = 1; k <= 12; k++) {
        uint32_t p = U32(st, G_RINGS_1X1 + 4u * (uint32_t) (k - 1));
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
    /* the rings 1..12 of 0x0046F8D0, stored from ring 12 up */
    static const uint32_t S1[12] = {G_RINGS_1X1_DATA + 0x4D0u, G_RINGS_1X1_DATA + 0x4B0u, G_RINGS_1X1_DATA + 0x480u,
                                    G_RINGS_1X1_DATA + 0x440u, G_RINGS_1X1_DATA + 0x3F0u, G_RINGS_1X1_DATA + 0x390u,
                                    G_RINGS_1X1_DATA + 0x320u, G_RINGS_1X1_DATA + 0x2A0u, G_RINGS_1X1_DATA + 0x210u,
                                    G_RINGS_1X1_DATA + 0x170u, G_RINGS_1X1_DATA + 0xC0u, G_RINGS_1X1_DATA};
    static const uint32_t S2[6] = {G_RINGS, G_RINGS + 0x18u, G_RINGS + 0x40u,
                                   G_RINGS + 0x78u, G_RINGS + 0xC0u, G_RINGS + 0x118u};
    static const uint32_t S3[6] = {G_RING_SOURCES_3, G_RING_SOURCES_3 + 0x20u, G_RING_SOURCES_3 + 0x50u,
                                   G_RING_SOURCES_3 + 0x90u, G_RING_SOURCES_3 + 0xE0u, G_RING_SOURCES_3 + 0x140u};
    static const uint32_t S4[6] = {G_RING_SOURCES_4, G_RING_SOURCES_4 + 0x28u, G_RING_SOURCES_4 + 0x60u,
                                   G_RING_SOURCES_4 + 0xA8u, G_RING_SOURCES_4 + 0x100u, G_RING_SOURCES_4 + 0x168u};
    static const uint32_t S5[6] = {G_RING_SOURCES_5, G_RING_SOURCES_5 + 0x30u, G_RING_SOURCES_5 + 0x70u,
                                   G_RING_SOURCES_5 + 0xC0u, G_RING_SOURCES_5 + 0x120u, G_RING_SOURCES_5 + 0x190u};
    static const uint32_t S6[6] = {G_RING_SOURCES_6, G_RING_SOURCES_6 + 0x38u, G_RING_SOURCES_6 + 0x80u,
                                   G_RING_SOURCES_6 + 0xD8u, G_RING_SOURCES_6 + 0x140u, G_RING_SOURCES_6 + 0x1B8u};
    rings_1x1_build(st);
    offsets(st, G_RING_COUNTS_1, 12, G_RING_OFFSETS_1, S1);
    offsets(st, G_RING_COUNTS_2, 6, G_RING_OFFSETS_2, S2);
    offsets(st, G_RING_COUNTS_3, 6, G_RING_OFFSETS_3, S3);
    offsets(st, G_RING_COUNTS_4, 6, G_RING_OFFSETS_4, S4);
    offsets(st, G_RING_COUNTS_5, 6, G_RING_OFFSETS_5, S5);
    offsets(st, G_RING_COUNTS_6, 6, G_RING_OFFSETS_6, S6);
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
