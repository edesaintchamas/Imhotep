/* Imhotep - the canal sub-pass of the terrain images over a rectangle (package last).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x0047BD50 (x0, y0, x1, y1) redraws every canal tile of a rectangle with 0x0047BE30, walking
 * the cursor 0x00BC9768 and the globals 0x00BC9764 (x), 0x00BC9760 (y), like the road pass
 * 0x0047B590 (core/map/terrain_roads.c). */
#include "map/last_terrain.h"

#include "sim/call.h"
#include "state/ported.h"

#define CURSOR 0x00BC9768u
#define CUR_X 0x00BC9764u
#define CUR_Y 0x00BC9760u
#define M_VARIANT 0x00BC9740u
#define M_EXTRA 0x00BC9744u
#define M_OFFSET 0x00BC9748u
#define CANAL_BASE 0x00F56B84u
#define TER(o) U32(st, 0x00AF598Cu + 4u * (uint32_t) (o))
#define IMG(o) S32(st, 0x00B7482Cu + 4u * (uint32_t) (o))
#define BGRID(o) S16(st, 0x00B4E6FCu + 2u * (uint32_t) (o))
#define BITS(o) U8(st, 0x00B285CCu + (uint32_t) (o))

void canal_tile(game_state *st, int32_t dry)
{
    /* @bin 0x0047BE30 (dry) the cursor's canal image, unless a building stands on it: the canal
     * pattern of its neighbour flags (0x0047E870, 0x00484BE0 on the records 0x005CC678, 16); a
     * road across (terrain 0x40): bitfield bit 0x80 cleared, the offset (when the pattern gave no
     * extra: 1 when the tile has no road 0x8000040, else 0) + 0x2A paved / + 0xF (0x0047B920);
     * image = canal base + variant + offset + (dry ? 0x30 : 0), floodplain (0x10000) through
     * 0x0051C620; bitfield low nibble cleared, aqueduct grid := the extra, 0x00B67D1C |= 0x40 */
    int32_t c = S32(st, CURSOR);
    if (BGRID(c) != 0) {
        return;
    }
    /* @bin 0x0047BE51 the original's test (difference to the canal base negative and signed
     * >= 0x30, jns / jl) can never hold: the shift is 0x30 only when `dry` */
    int32_t shift = dry != 0 ? 0x30 : 0;
    GAME_CALL0(st, 0x0047E870u);
    GAME_CALL(st, 0x00484BE0u, (int32_t) 0x005CC678, 0x10, -1, 0);
    c = S32(st, CURSOR);
    if (TER(c) & 0x40) {
        BITS(c) &= 0x7F;
        if (S32(st, M_EXTRA) == 0) {
            S32(st, M_OFFSET) = (U32(st, 0x00AF55FCu + 4u * (uint32_t) c) & 0x8000040u) == 0;
        }
        int32_t paved = GAME_CALL0(st, 0x0047B920u);
        S32(st, M_OFFSET) = S32(st, M_OFFSET) + (paved != 0 ? 0x2A : 0xF);
        c = S32(st, CURSOR);
    }
    int32_t img = (int32_t) ((uint32_t) S32(st, M_VARIANT) + (uint32_t) S32(st, CANAL_BASE) +
                             (uint32_t) S32(st, M_OFFSET) + (uint32_t) shift);
    if (TER(c) & 0x10000) {
        img = GAME_CALL(st, 0x0051C620u, img);
        c = S32(st, CURSOR);
    }
    IMG(c) = img;
    BITS(c) &= 0xF0;
    U8(st, 0x00AE8E7Cu + (uint32_t) c) = (uint8_t) S32(st, M_EXTRA);
    U8(st, 0x00B67D1Cu + (uint32_t) c) |= 0x40;
}

void canals_rect_redraw(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    /* @bin 0x0047BD50 (x0, y0, x1, y1) clamped to the map; every canal tile (0x100 without 0x0C)
     * whose bitfield 0x00B285CC has no 0x80: neighbour flags (0x0047E870), then 0x0047BE30(0) */
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    int32_t w = S32(st, 0x00BCA2D0u);
    if (x1 >= w) {
        x1 = w - 1;
    }
    int32_t h = S32(st, 0x00BCA2D4u);
    if (y1 >= h) {
        y1 = h - 1;
    }
    int32_t step = w - x1 + x0 - 1;
    S32(st, CUR_Y) = y0;
    S32(st, CURSOR) = S32(st, 0x00BCA2D8u) + y0 * 228 + x0;
    while (S32(st, CUR_Y) <= y1) {
        for (S32(st, CUR_X) = x0; S32(st, CUR_X) <= x1; S32(st, CUR_X) += 1) {
            int32_t c = S32(st, CURSOR);
            uint32_t v = TER(c);
            if ((v & 0x100) && !(v & 0xC) && !(BITS(c) & 0x80)) {
                GAME_CALL0(st, 0x0047E870u);
                canal_tile(st, 0);
            }
            S32(st, CURSOR) += 1;
        }
        S32(st, CURSOR) += S32(st, 0x00BCA2DCu) + step;
        S32(st, CUR_Y) += 1;
    }
}

static int32_t run_canals_redraw(game_state *st, const int32_t *a)
{
    canals_rect_redraw(st, a[0], a[1], a[2], a[3]);
    return 0;
}

static int32_t run_canal_tile(game_state *st, const int32_t *a)
{
    canal_tile(st, a[0]);
    return 0;
}

const ported_function last_terrain_ports[] = {
    {0x0047BD50u, "canals_rect_redraw", 4, 0, 0, run_canals_redraw, 0},
    {0x0047BE30u, "canal_tile", 1, 0, 0, run_canal_tile, 0},
};
const int last_terrain_ports_count = sizeof(last_terrain_ports) / sizeof(last_terrain_ports[0]);
