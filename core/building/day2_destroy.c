/* Imhotep - buildings destroyed: their tiles cleared or turned to rubble (package day2).
 * 0x0047C480 (clear the footprint), 0x0047C2B0 (rubble), 0x0047C7B0 (the footprint's corner),
 * 0x004695D0 (the linked parts), 0x0046D100 (a building collapses into rubble), 0x0046AC50 (what
 * points at a building is released).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/day2_destroy.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define TERRAIN 0x00AF598Cu      /* u32 per tile */
#define BITS 0x00B285CCu         /* u8 */
#define BUILDING_GRID 0x00B4E6FCu /* i16 */
#define EDGE 0x00B67D1Cu         /* u8 */
#define IMAGE 0x00B7482Cu        /* u32 */
#define RANDOM 0x00ACF85Cu       /* u8 */
#define SIZE_X 0x00BC973Cu
#define SIZE_Y 0x00BC9738u
#define OUT_OF_MAP 0x00BC9734u
#define MAP_W 0x00BCA2D0u
#define MAP_H 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u

static uint32_t ter(int32_t t)
{
    return TERRAIN + 4u * (uint32_t) t;
}

/* @bin 0x005CC8C0 the footprint offsets: rows of 0x30 bytes, 8 bytes a tile */
static int32_t footprint_offset(game_state *st, int32_t row, int32_t col)
{
    return S32(st, 0x005CC8C0u + 0x30u * (uint32_t) row + 8u * (uint32_t) col);
}

int32_t footprint_corner(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0047C7B0 the size from the tile's bits (low 4: 1..5 -> 2..6, else 1) into
     * 0x00BC973C / 0x00BC9738, then back along the edge bits (0x00B67D1C & 7 left, & 0x38 up)
     * to the footprint's corner tile */
    int32_t t = S32(st, MAP_START) + y * 0xE4 + x;
    uint8_t k = U8(st, BITS + (uint32_t) t) & 0xF;
    int32_t n = k >= 1 && k <= 5 ? k + 1 : 1;
    S32(st, SIZE_X) = n;
    S32(st, SIZE_Y) = n;
    for (int32_t i = 0; i < n && (U8(st, EDGE + (uint32_t) t) & 7); i++) {
        t--;
    }
    for (int32_t i = 0; i < n && (U8(st, EDGE + (uint32_t) t) & 0x38); i++) {
        t -= 0xE4;
    }
    return t;
}

int32_t building_part_size(game_state *st, int32_t b)
{
    /* @bin 0x00541AB0 table 0x00601DE0 (0x34-byte entries) by the part index +0x12 */
    return S32(st, 0x00601DE0u + 0x34u * (uint32_t) (int32_t) S16(st, BUILDING_ADDR(b) + 0x12));
}

static int on_map(game_state *st, int32_t x, int32_t y, int32_t x1, int32_t y1)
{
    if (x < 0 || y < 0 || x1 >= S32(st, MAP_W) || y1 >= S32(st, MAP_H)) {
        S32(st, OUT_OF_MAP) = 1;
        return 0;
    }
    return 1;
}

void building_clear_tiles(game_state *st, int32_t b, int32_t x, int32_t y)
{
    /* @bin 0x0047C480 the tiles of building b (all of the footprint when b is 0) lose it: grids
     * cleared, water kept (0x0047A220) or the land image back (0x00F56B54); then the terrain
     * passes over the area */
    if (!on_map(st, x, y, x, y)) {
        return;
    }
    uint32_t r = BUILDING_ADDR(b);
    int32_t t0;
    if (U16(st, r + 0x10) == 0xB7) {
        t0 = S32(st, r + 0xC);
        S32(st, SIZE_X) = 2;
        S32(st, SIZE_Y) = 2;
    } else {
        t0 = GAME_CALL(st, 0x0047C7B0u, x, y);
    }
    if (S32(st, ter(t0)) == 2) {
        return;
    }
    if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488030u, b) != 0) {
        S32(st, SIZE_X) = 3;
        S32(st, SIZE_Y) = 3;
    } else if (U16(st, r + 0x10) == 0xD5) {
        int32_t n = GAME_CALL(st, 0x00541AB0u, b);
        S32(st, SIZE_Y) = n;
        S32(st, SIZE_X) = n;
    }
    uint32_t keep = 0x3052E84u;
    if (U16(st, r + 0x10) == 0x8A) {
        keep |= U32(st, ter(t0)) & 0x140;
    }
    for (int32_t row = 0; row < S32(st, SIZE_Y); row++) {
        for (int32_t col = 0; col < S32(st, SIZE_X); col++) {
            int32_t t = t0 + footprint_offset(st, row, col);
            if (b != 0) {
                if (S16(st, BUILDING_GRID + 2u * (uint32_t) t) != b) {
                    continue;
                }
                if (U16(st, r + 0x10) != 0x63) {
                    U8(st, 0x00A9010Cu + (uint32_t) t) = U8(st, r + 0x10);
                }
            }
            U8(st, BITS + (uint32_t) t) &= 0xE0;
            S16(st, BUILDING_GRID + 2u * (uint32_t) t) = 0;
            S16(st, 0x00A9CC1Cu + 2u * (uint32_t) t) = 0;
            U8(st, 0x00AE8E7Cu + (uint32_t) t) = 0;
            U8(st, EDGE + (uint32_t) t) = (uint8_t) ((U8(st, EDGE + (uint32_t) t) & 0xC0) | 0x40);
            uint32_t v = U32(st, ter(t));
            U8(st, 0x00ADC36Cu + (uint32_t) t) = 0;
            if ((v & 4) && !(v & 0x10000)) {
                U32(st, ter(t)) = 4;
                GAME_CALL(st, 0x0047A220u, col + x, row + y, 1);
            } else {
                U32(st, IMAGE + 4u * (uint32_t) t) = (U8(st, RANDOM + (uint32_t) t) & 7) + U32(st, 0x00F56B54u);
                U32(st, ter(t)) = v & keep;
            }
        }
    }
    int32_t w = S32(st, SIZE_X), h = S32(st, SIZE_Y);
    GAME_CALL(st, 0x004767D0u, x, y, w + x, h + y, 0, 0);
    GAME_CALL(st, 0x004746D0u, x, y, S32(st, SIZE_X));
    GAME_CALL(st, 0x00477500u, x, y, S32(st, SIZE_X) + x, S32(st, SIZE_Y) + y);
    GAME_CALL(st, 0x004777C0u, x, y, S32(st, SIZE_X) + x, S32(st, SIZE_Y) + y);
    GAME_CALL(st, 0x00475CC0u, x, y, S32(st, SIZE_X) + x, S32(st, SIZE_Y) + y, 0);
    GAME_CALL(st, 0x004776F0u, x, y, S32(st, SIZE_X) + x, S32(st, SIZE_Y) + y);
    GAME_CALL(st, 0x00477890u, x, y, S32(st, SIZE_X) + x, S32(st, SIZE_Y) + y);
}

void building_rubble_tiles(game_state *st, int32_t b, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x0047C2B0 the w x h tiles at (x, y) of building b become rubble (terrain 0x1000,
     * image 0x00F56C4C + random), or keep their water */
    if (!on_map(st, x, y, x - 1 + w, y - 1 + h)) {
        return;
    }
    int32_t base = S32(st, MAP_START) + y * 0xE4 + x;
    for (int32_t row = 0; row < h; row++) {
        for (int32_t col = 0; col < w; col++) {
            int32_t t = footprint_offset(st, row, col) + base;
            if (S16(st, BUILDING_GRID + 2u * (uint32_t) t) != b) {
                continue;
            }
            if (b != 0 && U16(st, BUILDING_ADDR(b) + 0x10) != 0x63) {
                U8(st, 0x00A9010Cu + (uint32_t) t) = U8(st, BUILDING_ADDR(b) + 0x10);
            }
            U8(st, 0x00AE8E7Cu + (uint32_t) t) = 0;
            U8(st, BITS + (uint32_t) t) &= 0xE0;
            uint32_t v = U32(st, ter(t));
            S16(st, BUILDING_GRID + 2u * (uint32_t) t) = 0;
            S16(st, 0x00A9CC1Cu + 2u * (uint32_t) t) = 0;
            U8(st, 0x00ADC36Cu + (uint32_t) t) = 0;
            U8(st, EDGE + (uint32_t) t) = 0x40;
            if (v & 4) {
                U32(st, ter(t)) = 4;
                GAME_CALL(st, 0x0047A220u, col + x, row + y, 1);
            } else {
                U32(st, ter(t)) = (v & 0x3052E80u) | 0x1000;
                U32(st, IMAGE + 4u * (uint32_t) t) = (U8(st, RANDOM + (uint32_t) t) & 7) + U32(st, 0x00F56C4Cu);
            }
        }
    }
}

void building_destroy_linked(game_state *st, int32_t b, int32_t collapse)
{
    /* @bin 0x004695D0 the parts linked before (+0x38) and after (+0x3A) b, up to 525 each way:
     * collapsed (0x0046D100) or turned to rubble with state 4 */
    for (int pass = 0; pass < 2; pass++) {
        int32_t p = b;
        for (int32_t n = 0; n < 0x20D; n++) {
            p = S16(st, BUILDING_ADDR(p) + (pass == 0 ? 0x38 : 0x3A));
            if (p < 1) {
                break;
            }
            if (collapse != 0) {
                GAME_CALL(st, 0x0046D100u, p, 0);
            } else {
                uint32_t r = BUILDING_ADDR(p);
                int32_t sz = U8(st, r + 3);
                GAME_CALL(st, 0x0047C2B0u, p, S16(st, r + 6), S16(st, r + 8), sz, sz);
                U8(st, r) = 4;
            }
        }
    }
}

int32_t building_main_part_46d0f0(game_state *st, int32_t b)
{
    /* @bin 0x0046D0F0 = 0x00475B50 */
    return GAME_CALL(st, 0x00475B50u, b);
}

static int32_t rubble_image(game_state *st, int32_t b, int32_t plain)
{
    /* @bin 0x0046D2E3 0x00F56C54, or 0x00F56C5C + 9 x (random & 3) of the building's tile */
    if (plain != 0) {
        return S32(st, 0x00F56C54u);
    }
    return S32(st, 0x00F56C5Cu) + 9 * (U8(st, RANDOM + U32(st, BUILDING_ADDR(b) + 0xC)) & 3);
}

static void rubble_fields(game_state *st, uint32_t r, uint8_t flag)
{
    S16(st, r + 0x4A) = (int16_t) ((U8(st, r + 0x4D) & 7) + 1);
    S16(st, r + 0x30) = 0;
    U8(st, r + 0x4C) = 1;
    U8(st, r + 0xBD) = flag;
}

void building_collapse(game_state *st, int32_t b, int32_t flag)
{
    /* @bin 0x0046D100 building b collapses: released (0x0046AC50), its tiles cleared
     * (0x0047C480), itself turned into rubble (type 0x63, size 1) on dry land or state 5 on
     * water, and one rubble building (0x0046A400) on each other dry tile of its footprint
     * (offsets 0x005CCD18); type 0xCA goes to 0x004C04A0 instead */
    uint32_t r = BUILDING_ADDR(b);
    int routes = 0;
    if (U16(st, r + 0x10) == 0xCA) {
        GAME_CALL(st, 0x004C04A0u, b);
        return;
    }
    S16(st, r + 0x48) = 0;
    S16(st, r + 0x46) = 0;
    S32(st, 0x00BC9694u) = 0;
    if (U8(st, r + 5) != 0 && S16(st, r + 0x1C) != 0) {
        GAME_CALL(st, 0x004E9170u, S32(st, CURRENT_PLAYER_ADDR), S16(st, r + 0x1C));
    }
    S16(st, r + 0x1C) = 0;
    U8(st, r + 5) = 0;
    U8(st, r + 0x43) = 0;
    S16(st, r + 0x20) = 0;
    GAME_CALL(st, 0x0046AC50u, b);
    uint16_t type = U16(st, r + 0x10);
    if (type == 0x1F || type == 0x1E || type == 0x21 || type == 0xD1) {
        int32_t m = GAME_CALL(st, 0x0046D0F0u, b);
        if (m > 0 && m == b) {
            GAME_CALL(st, 0x0043FD00u, m, S16(st, BUILDING_ADDR(m) + 0x10));
        }
    }
    type = U16(st, r + 0x10);
    if (type == 0x4B || type == 0x4C || type == 0x4A || type == 0xB5 || type == 0xB6 || type == 0xC0) {
        routes = 1;
    }
    /* @bin 0x0046D217 +5 is 0 by now: the plain rubble image is never chosen here */
    int32_t plain = U8(st, r + 5) != 0 && S16(st, r + 0x12) <= 1;
    int32_t n;
    switch (U8(st, r + 3)) {
    case 2: n = 4; break;
    case 3: n = 9; break;
    case 4: n = 0x10; break;
    case 5: n = 0x19; break;
    case 6: n = 0x24; break;
    default: n = 0; break;
    }
    GAME_CALL(st, 0x0047C480u, b, S16(st, r + 6), S16(st, r + 8));
    if (U8(st, TERRAIN + 4u * U32(st, r + 0xC)) & 4) {
        U8(st, r) = 5;
    } else {
        S16(st, r + 0x10) = 0x63;
        S16(st, r + 0x30) = 0;
        S16(st, r + 0x4A) = (int16_t) ((U8(st, r + 0x4D) & 7) + 1);
        U8(st, r + 0x4C) = 1;
        U8(st, r + 3) = 1;
        U8(st, r + 0xBD) = (uint8_t) flag;
        GAME_CALL(st, 0x0047BF50u, b, S16(st, r + 6), S16(st, r + 8), 1, 1, rubble_image(st, b, plain), 8);
    }
    for (int32_t k = 0; k + 1 < n; k++) {
        int32_t x = S16(st, r + 6) + S32(st, 0x005CCD18u + 8u * (uint32_t) k);
        int32_t y = S16(st, r + 8) + S32(st, 0x005CCD1Cu + 8u * (uint32_t) k);
        if (U8(st, TERRAIN + 4u * (uint32_t) (S32(st, MAP_START) + y * 0xE4 + x)) & 4) {
            continue;
        }
        int32_t nb = GAME_CALL(st, 0x0046A400u, S32(st, CURRENT_PLAYER_ADDR), 0x63, x, y, 0, 0);
        uint32_t q = BUILDING_ADDR(nb);
        GAME_CALL(st, 0x0047BF50u, nb, S16(st, q + 6), S16(st, q + 8), 1, 1, rubble_image(st, nb, plain), 8);
        rubble_fields(st, q, (uint8_t) flag);
    }
    if (routes == 1) {
        GAME_CALL0(st, 0x005178B0u);
    }
}

void building_release(game_state *st, int32_t b)
{
    /* @bin 0x0046AC50 what refers to a building going away: its slots (+0xC1 in 0x007900C0,
     * +0xCA in 0x00EAAF68, +0xCB 0x0046A2E0) and, by type, the city's records of it */
    uint32_t r = BUILDING_ADDR(b);
    uint8_t k = U8(st, r + 0xC1);
    if (k != 0) {
        U8(st, r + 0xC1) = 0;
        U8(st, 0x007900C0u + 0xC4u * k) = 0;
    }
    k = U8(st, r + 0xCA);
    if (k != 0) {
        U8(st, r + 0xCA) = 0;
        S32(st, 0x00EAAF68u + 0xE0u * k) = 0;
    }
    if (U8(st, r + 0xCB) != 0) {
        GAME_CALL(st, 0x0046A2E0u, b);
        U8(st, r + 0xCB) = 0;
    }
    int32_t type = S16(st, r + 0x10);
    uint32_t c = CURRENT_CITY(st);
    switch (type) {
    case 0xD1:
        S32(st, c + 0x47A0) = -1;
        S32(st, c + 0x47A4) = -1;
        S16(st, c + 0x4814) = 0;
        S32(st, 0x00775678u) = 0;
        return;
    case 0xBB: case 0xBC: case 0xBD:
        if (S32(st, r + 0xC) == S32(st, c + 0x278C)) {
            S32(st, c + 0x278C) = 0;
            S16(st, c + 0x2788) = 0;
            S16(st, c + 0x278A) = 0;
            S16(st, c + 0x4486) = 0;
        }
        return;
    case 0x4D: case 0x4E: case 0x4F:
        S32(st, c + 0x499C) = 0;
        return;
    case 0x5F:
        if (S32(st, r + 0xC) == S32(st, c + 0x44A8)) {
            S32(st, c + 0x44A8) = 0;
            S16(st, c + 0x44A4) = 0;
            S16(st, c + 0x44A6) = 0;
            S32(st, c + 0x44B0) = 0;
        }
        return;
    case 0x5E:
        S32(st, c + 0x49D0) = 0;
        return;
    case 0x32:
        if (S32(st, r + 0xC) == S32(st, c + 0x45F8)) {
            S32(st, c + 0x45F8) = 0;
            S16(st, c + 0x45F4) = 0;
            S16(st, c + 0x45F6) = 0;
            S32(st, c + 0x4600) = 0;
        }
        return;
    case 0x88: {
        int32_t o = S32(st, r + 0xC4);
        if (o > 0) {
            S32(st, BUILDING_ADDR(o) + 0xC4) = -1;
            GAME_CALL(st, 0x004A36B0u, b);
        }
        return;
    }
    case 0x39:
        GAME_CALL(st, 0x004B82E0u, S16(st, r + 0x50));
        return;
    case 0xD2:
        S32(st, c + 0x49D4) = S16(st, c + 0x433A);
        S16(st, c + 0x433A) = 0;
        GAME_CALL(st, 0x004F4AE0u, b);
        return;
    case 0xB7: case 0xD5: case 0xDE: case 0xE1: case 0xE3:
        GAME_CALL(st, 0x004F4AE0u, b);
        return;
    case 0xE4: case 0xE5: case 0xE6: case 0xEA: case 0xEB: case 0xEC: {
        /* @bin 0x0046AE6D the last part of these monuments: the terrain image is rebuilt */
        GAME_CALL(st, 0x004F4AE0u, b);
        int16_t last = type == 0xE4 ? 1 : type == 0xE5 ? 0xA6 : type == 0xE6 ? 0xF : type == 0xEA ? 0x9D
                     : type == 0xEB ? 0x1B4 : 0x200;
        if (S16(st, r + 0x12) == last) {
            GAME_CALL0(st, 0x004DEA50u);
            U8(st, 0x00E38DCAu) = 1;
        }
        return;
    }
    case 0x4A:
        GAME_CALL(st, 0x0046A2E0u, b);
        return;
    case 0x8A: case 0xD7: {
        int32_t x = S16(st, r + 6), y = S16(st, r + 8);
        GAME_CALL(st, 0x0047B590u, x, y, x, y);
        x = S16(st, r + 6);
        y = S16(st, r + 8);
        GAME_CALL(st, 0x0047BD50u, x, y, x, y);
        return;
    }
    case 0xB6:
        S32(st, 0x00BCB9B0u) -= 1;
        return;
    default:
        break;
    }
    if (type == 0xCF || type == 0xD0) {
        GAME_CALL(st, 0x004E5830u, b);
        return;
    }
    if (type >= 0x41 && type <= 0x45) {
        S16(st, c + 0x4478) = 0;
        S16(st, c + 0x448A) = 0;
        int32_t m = GAME_CALL(st, 0x00475B50u, b);
        GAME_CALL(st, 0x004C0920u, m);
        S32(st, CURRENT_CITY(st) + 0x472C) = 0;
    }
}

void building_clear_area(game_state *st, int32_t b, int32_t type)
{
    /* @bin 0x0043FD00 the n x n tiles from b's tile +0xD0 (types 0x1F: 2, 0x1E: 3, 0x21: 4,
     * 0xD1: 5) are cleared, keeping terrain bits 0x3052EC4, the old type in 0x00A9010C; plaza
     * tiles (0x40) are redrawn by 0x0047B310, others by 0x0047A120 while 0x0077567C is set;
     * then the terrain passes over the area */
    if (U16(st, BUILDING_ADDR(b) + 0x10) == 0xD1) {
        S16(st, CURRENT_CITY(st) + 0x4814) = 0;
    }
    int32_t n = type == 0x1E ? 3 : type == 0x1F ? 2 : type == 0x21 ? 4 : type == 0xD1 ? 5 : 0;
    int32_t t = S32(st, BUILDING_ADDR(b) + 0xD0);
    int32_t x0 = (t - S32(st, MAP_START)) % 0xE4, y0 = (t - S32(st, MAP_START)) / 0xE4;
    for (int32_t row = 0; row < n; row++) {
        for (int32_t col = 0; col < n; col++, t++) {
            U8(st, 0x00ADC36Cu + (uint32_t) t) = 0;
            U8(st, 0x00AE8E7Cu + (uint32_t) t) = 0;
            U8(st, BITS + (uint32_t) t) &= 0xE0;
            S16(st, BUILDING_GRID + 2u * (uint32_t) t) = 0;
            S16(st, 0x00A9CC1Cu + 2u * (uint32_t) t) = 0;
            U8(st, EDGE + (uint32_t) t) = (uint8_t) ((U8(st, EDGE + (uint32_t) t) & 0xC0) | 0x40);
            uint32_t v = U32(st, ter(t)) & 0x3052EC4u;
            U32(st, IMAGE + 4u * (uint32_t) t) = 0;
            U32(st, ter(t)) = v;
            U8(st, 0x00A9010Cu + (uint32_t) t) = (uint8_t) type;
            if (v & 0x40) {
                GAME_CALL(st, 0x0047B310u, x0 + col, y0 + row, 1);
            } else if (S32(st, 0x0077567Cu) != 0) {
                GAME_CALL(st, 0x0047A120u, x0 + col, y0 + row);
            }
        }
        t += 0xE4 - n;
    }
    int32_t x1 = n + x0, y1 = n + y0;
    S32(st, 0x0077567Cu) = 0;
    GAME_CALL(st, 0x004767D0u, x0, y0, x1, y1, 0, 0);
    GAME_CALL(st, 0x004767D0u, x0 - 5, y0 - 5, x1 + 5, y1 + 5, 1, 0);
    GAME_CALL(st, 0x00477500u, x0, y0, x1, y1);
    GAME_CALL(st, 0x004777C0u, x0, y0, x1, y1);
    GAME_CALL(st, 0x00475CC0u, x0, y0, x1, y1, 0);
    GAME_CALL(st, 0x004776F0u, x0, y0, x1, y1);
    GAME_CALL(st, 0x00477890u, x0, y0, x1, y1);
    GAME_CALL(st, 0x00475720u, x0, y0, x1, y1, 1);
    GAME_CALL(st, 0x00516F90u, x0, y0, x1, y1, 1);
}

void house_split(game_state *st, int32_t b)
{
    /* @bin 0x0046C500 a 3x3 house splits into nine 1x1 houses (type 0x13, level 9): b keeps
     * pop / 9 + pop % 9 people and q + r of each of its 12 goods (+0x54, i16, q = v / 9,
     * r = v % 9); eight new houses (0x0046A400) get pop / 9 and q each */
    uint32_t r = BUILDING_ADDR(b);
    int32_t pop = S16(st, r + 0x1C);
    int32_t q[12], m[12];
    for (uint32_t i = 0; i < 12; i++) {
        int32_t v = S16(st, r + 0x54 + 2 * i);
        q[i] = v / 9;
        m[i] = v % 9;
    }
    int32_t share = pop / 9;
    GAME_CALL(st, 0x0047C480u, b, S16(st, r + 6), S16(st, r + 8));
    S16(st, r + 0x10) = 0x13;
    S16(st, r + 0x12) = 9;
    U8(st, r + 3) = 1;
    U8(st, r + 5) = 1;
    U8(st, r + 4) = 0;
    S16(st, r + 0x1C) = (int16_t) (pop % 9 + share);
    for (uint32_t i = 0; i < 12; i++) {
        S16(st, r + 0x54 + 2 * i) = (int16_t) (m[i] + q[i]);
    }
    S16(st, r + 0x20) = 0;
    int16_t level = S16(st, r + 0x12);
    int32_t img = S32(st, 0x00F55BB4u + 8u * U32(st, 0x005D03ECu + 4u * (uint32_t) level)) +
                  S32(st, 0x005D043Cu + 4u * (uint32_t) level);
    GAME_CALL(st, 0x0047BF50u, b, S16(st, r + 6), S16(st, r + 8), 1, 1,
              (U8(st, RANDOM + U32(st, r + 0xC)) & 1) + img, 8);
    int32_t x = S16(st, r + 6), y = S16(st, r + 8);
    static const int8_t at[8][2] = {{1, 0}, {2, 0}, {0, 1}, {1, 1}, {2, 1}, {0, 2}, {1, 2}, {2, 2}};
    for (int k = 0; k < 8; k++) {
        int32_t nb = GAME_CALL(st, 0x0046A400u, S32(st, CURRENT_PLAYER_ADDR), 0x13, x + at[k][0], y + at[k][1], 0, 0);
        uint32_t n = BUILDING_ADDR(nb);
        S16(st, n + 0x1C) = (int16_t) share;
        for (uint32_t i = 0; i < 12; i++) {
            S16(st, n + 0x54 + 2 * i) = (int16_t) q[i];
        }
        S16(st, n + 0x20) = 0;
        GAME_CALL(st, 0x0047BF50u, nb, S16(st, n + 6), S16(st, n + 8), 1, 1,
                  (U8(st, RANDOM + U32(st, n + 0xC)) & 1) + img, 8);
    }
}

static int32_t run_corner(game_state *st, const int32_t *a)
{
    return footprint_corner(st, a[0], a[1]);
}

static int32_t run_part_size(game_state *st, const int32_t *a)
{
    return building_part_size(st, a[0]);
}

static int32_t run_clear(game_state *st, const int32_t *a)
{
    building_clear_tiles(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_rubble(game_state *st, const int32_t *a)
{
    building_rubble_tiles(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}

static int32_t run_linked(game_state *st, const int32_t *a)
{
    building_destroy_linked(st, a[0], a[1]);
    return 0;
}

static int32_t run_main_part(game_state *st, const int32_t *a)
{
    return building_main_part_46d0f0(st, a[0]);
}

static int32_t run_collapse(game_state *st, const int32_t *a)
{
    building_collapse(st, a[0], a[1]);
    return 0;
}

static int32_t run_release(game_state *st, const int32_t *a)
{
    building_release(st, a[0]);
    return 0;
}

static int32_t run_clear_area(game_state *st, const int32_t *a)
{
    building_clear_area(st, a[0], a[1]);
    return 0;
}

static int32_t run_house_split(game_state *st, const int32_t *a)
{
    house_split(st, a[0]);
    return 0;
}

const ported_function day2_destroy_ports[] = {
    {0x0046C500u, "house_split", 1, 0, 0, run_house_split, 0},
    {0x0043FD00u, "building_clear_area", 2, 0, 0, run_clear_area, 0},
    {0x0046AC50u, "building_release", 1, 0, 0, run_release, 0},
    {0x0046D0F0u, "building_main_part_46d0f0", 1, 32, 0, run_main_part, 0},
    {0x0046D100u, "building_collapse", 2, 0, 0, run_collapse, 0},
    {0x0047C7B0u, "footprint_corner", 2, 32, 0, run_corner, 0},
    {0x00541AB0u, "building_part_size", 1, 32, 0, run_part_size, 0},
    {0x0047C480u, "building_clear_tiles", 3, 0, 0, run_clear, 0},
    {0x0047C2B0u, "building_rubble_tiles", 5, 0, 0, run_rubble, 0},
    {0x004695D0u, "building_destroy_linked", 2, 0, 0, run_linked, 0},
};
const int day2_destroy_ports_count = sizeof(day2_destroy_ports) / sizeof(day2_destroy_ports[0]);
