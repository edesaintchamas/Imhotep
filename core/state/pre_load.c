/* Imhotep - what the game does before reading a saved game (Pharaoh.exe 0x004D5100 prologue).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/pre_load.h"

#include <string.h>

#include "map/floodplain.h"
#include "random/random.h"

#define MAP_WIDTH 0x00BCA2D0u
#define MAP_HEIGHT 0x00BCA2D4u

void floodplain_settings_reset(game_state *st)
{
    /* @bin 0x004BD330 (this = 0x00D38360, set at 0x004D5100) eight dwords */
    static const int32_t v[8] = {0xB4, 0x46, 100, 0xB4, 0x46, 100, 0, 100};
    for (uint32_t i = 0; i < 8; i++) {
        S32(st, 0x00D38360u + 4 * i) = v[i];
    }
}

/* @bin 0x0046F930 is (x, y) inside the diamond-shaped map (map row globals of 0x0046F7B0) */
int map_tile_inside(const game_state *st, int32_t x, int32_t y)
{
    game_state *s = (game_state *) st;
    int32_t cx = S32(s, 0x00BCA2F8u), cy = S32(s, 0x00BCA2FCu);
    if (cx < x) {
        if (y <= cy) {
            return x - y <= S32(s, 0x00BCA2F0u) - cy;
        }
        return x + y <= S32(s, 0x00BCA2F0u) + 1 + cy;
    }
    if (y <= cy) {
        return S32(s, 0x00BCA2E8u) + cy <= x + y;
    }
    return cx - S32(s, 0x00BCA2F4u) <= x - y;
}

/* @bin 0x00484EE0 byte +0x0E := 0 in n 16-byte entries */
static void clear_entries(game_state *st, uint32_t table, int n)
{
    for (int i = 0; i < n; i++) {
        U8(st, table + 0x0E + 16u * (uint32_t) i) = 0;
    }
}

void pre_load_clear(game_state *st)
{
    /* @bin 0x0046FA40 grids cleared (address, bytes) */
    static const uint32_t grids[][2] = {
        {0x00B7482C, 0x32C40}, {0x00B67D1C, 0xCB10}, {0x00B4E6FC, 0x19620}, {0x00AF598C, 0x32C40},
        {0x00A5D4CC, 0xCB10}, {0x00AE8E7C, 0xCB10}, {0x00B350DC, 0x19620}, {0x00B285CC, 0xCB10},
        {0x00ADC36C, 0xCB10}, {0x00ACF85C, 0xCB10}, {0x00AC2D4C, 0xCB10}, {0x00AB623C, 0xCB10},
        {0x00A9CC1C, 0x19620}, {0x00A9010C, 0xCB10}, {0x00A835FC, 0xCB10}, {0x00A76AEC, 0xCB10},
        {0x00A69FDC, 0xCB10}, {0x00A509BC, 0xCB10},
    };
    for (size_t i = 0; i < sizeof(grids) / sizeof(grids[0]); i++) {
        memset(state_ptr(st, grids[i][0], grids[i][1]), 0, grids[i][1]);
    }
    /* @bin 0x0046FA40 0x00A43EAC: 0x32C4 dwords := -1 */
    memset(state_ptr(st, 0x00A43EACu, 0x32C4u * 4), 0xFF, 0x32C4u * 4);
    floodplain_manager_reset(st);
    /* @bin 0x0046FA40 tables whose entries' byte +0x0E is cleared, with their counts */
    static const uint32_t tables[][2] = {
        {0x005CA658, 0x30}, {0x005CA958, 0x30}, {0x005CAC58, 0x18}, {0x005CB9D8, 0x30},
        {0x005CBCD8, 0x0F}, {0x005CBDC8, 0x0E}, {0x005CBEA8, 0x11}, {0x005CC158, 0x11},
        {0x005CC268, 0x11}, {0x005CC378, 0x30}, {0x005CC678, 0x10},
    };
    for (size_t i = 0; i < sizeof(tables) / sizeof(tables[0]); i++) {
        clear_entries(st, tables[i][0], (int) tables[i][1]);
    }
    /* @bin 0x0046FC30 terrain := 0x80005 on every tile outside the map (current map globals) */
    for (int32_t y = 0; y < 0xE4; y++) {
        for (int32_t x = 0; x < 0xE4; x++) {
            if (!map_tile_inside(st, x, y)) {
                U32(st, 0x00AF598Cu + 4u * (uint32_t) (y * 0xE4 + x)) = 0x80005u;
            }
        }
    }
    /* @bin 0x0046FC80 random grid (chunk 15): one RNG step per tile, low byte of the 15-bit value */
    for (uint32_t o = 0; o < 0xE4u * 0xE4u; o++) {
        random_next_state(st);
        U8(st, 0x00ACF85Cu + o) = (uint8_t) U32(st, 0x00D3B370u);
    }
    /* @bin 0x00475B80 image grid := random & 7 + base image 0x00F56B54 on the map's rows, and
     * bitfields |= 0x20 where the random value is odd */
    for (int32_t r = 0; r < S32(st, MAP_HEIGHT); r++) {
        int32_t lo = S32(st, 0x00BC9F24u + 4u * (uint32_t) r), hi = S32(st, 0x00BC9B94u + 4u * (uint32_t) r);
        int32_t o = S32(st, 0x00BC9804u + 4u * (uint32_t) r);
        for (int32_t k = lo; k < hi; k++, o++) {
            uint8_t v = U8(st, 0x00ACF85Cu + (uint32_t) o);
            S32(st, 0x00B7482Cu + 4u * (uint32_t) o) = (v & 7) + S32(st, 0x00F56B54u);
            if (v & 1) {
                U8(st, 0x00B285CCu + (uint32_t) o) |= 0x20;
            }
        }
    }
}
