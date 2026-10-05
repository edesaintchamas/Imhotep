/* Imhotep - what the destruction of a building releases or redraws (package rest): the callees of
 * 0x0046AC50 (building_release) and 0x0046D100 (building_collapse) in core/building/day2_destroy.c.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/rest_destroy.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define TERRAIN 0x00AF598Cu       /* u32 per tile */
#define BITS 0x00B285CCu          /* u8 */
#define BUILDING_GRID 0x00B4E6FCu /* i16 */
#define EDGE 0x00B67D1Cu          /* u8 */
#define IMAGE 0x00B7482Cu         /* u32 */
#define RANDOM 0x00ACF85Cu        /* u8 */
#define MAP_START 0x00BCA2D8u
#define ORIENT 0x00BCA2E0u

void building_0x0046a2e0(game_state *st, int32_t b)
{
    /* @bin 0x0046A2E0 the slot +0xCB (1..10) of table 0x009783C0 (0x1C bytes each) is freed:
     * its byte +0 and dword +0x18 cleared */
    int32_t k = (int32_t) U8(st, BUILDING_ADDR(b) + 0xCB) - 1;
    if (k < 0 || k >= 10) {
        return;
    }
    uint32_t e = 0x009783C0u + 0x1Cu * (uint32_t) k;
    U8(st, e) = 0;
    U32(st, e + 0x18) = 0;
}

void city_slot_clear_0x004f4a80(game_state *st, int32_t b, int32_t i)
{
    /* @bin 0x004F4A80 the current player's dword 0x00EA5FAC + 4 * i cleared (b unused) */
    (void) b;
    U32(st, 0x00EA5FACu + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) i)) = 0;
}

int32_t monument_slot_type_4f52b0(game_state *st, int32_t i)
{
    /* @bin 0x004F52B0 the monument kind of slot i (word 0x00784DE0 into table 0x005D19C8, 16 bytes):
     * 0xCF or 0xD0 for the 2 x 2 kinds (by its +4), else 0 */
    uint32_t e = 0x005D19C8u + 16u * (uint32_t) (int32_t) S16(st, 0x00784DE0u + 2u * (uint32_t) i);
    if (S16(st, e) == 2 && S16(st, e + 2) == 2) {
        return 0xCF + (S16(st, e + 4) != 0);
    }
    return 0;
}

void monument_slot_images_4e5950(game_state *st, int32_t i)
{
    /* @bin 0x004E5950 the slot's graphics (names 0x005E4588 / 0x005E4598) loaded into image slot
     * i + 5 (0x0053DD90 on 0x00F55BB0) */
    int32_t t = monument_slot_type_4f52b0(st, i);
    if (t == 0xCF) {
        GAME_CALLT(st, 0x00F55BB0u, 0x0053DD90u, S32(st, 0x005E4588u), i + 5);
    } else if (t == 0xD0) {
        GAME_CALLT(st, 0x00F55BB0u, 0x0053DD90u, S32(st, 0x005E4598u), i + 5);
    }
}

void storage_take_45b700(game_state *st, int32_t b, int32_t amount)
{
    /* @bin 0x0045B700 a storage building (0x48, 0xC1) gives up to `amount` from its parts (+0x3A
     * chain, 8 steps; +0x3C quantity of good +0x12): the city's stock 0x00EA60D0 by good grows
     * and 0x00EA6160 shrinks; each part's image set again (0x00F566BC / 0x00F57204 by good,
     * 0x0047D350, 0x0045B490; empty 0x00F566B4) */
    int16_t type = S16(st, BUILDING_ADDR(b) + 0x10);
    if (type != 0x48 && type != 0xC1) {
        return;
    }
    int32_t cur = b;
    for (int32_t k = 0; k < 8; k++) {
        if (amount <= 0) {
            return;
        }
        int32_t p = S16(st, BUILDING_ADDR(cur) + 0x3A);
        cur = p;
        if (p <= 0) {
            continue;
        }
        uint32_t r = BUILDING_ADDR(p);
        int16_t qty = S16(st, r + 0x3C);
        if (qty <= 0) {
            continue;
        }
        int32_t g = S16(st, r + 0x12);
        uint32_t c = 0x00EA60D0u + 4u * (U32(st, CURRENT_PLAYER_ADDR) * 0x1276u + (uint32_t) g);
        if (qty <= amount) {
            amount -= qty;
            S32(st, c) += qty;
            S32(st, c + 0x90) -= qty;
            S16(st, r + 0x3C) = 0;
            S16(st, r + 0x12) = 0;
        } else {
            S16(st, r + 0x3C) = (int16_t) (qty - amount);
            S32(st, c) += amount;
            S32(st, c + 0x90) -= amount;
            amount = 0;
        }
        int32_t t = S32(st, r + 0xC);
        int32_t img;
        if (S16(st, r + 0x3C) > 0) {
            img = g < 0x1F ? S32(st, 0x00F566BCu) + g * 4 - 4 : S32(st, 0x00F57204u) + g * 4 - 0x7C;
            img += GAME_CALL(st, 0x0047D350u, g, 0);
            img += GAME_CALL(st, 0x0045B490u, S16(st, r + 0x3C));
        } else {
            img = S32(st, 0x00F566B4u);
        }
        U32(st, IMAGE + 4u * (uint32_t) t) = (uint32_t) img;
    }
}

void building_0x004f4ae0(game_state *st, int32_t b)
{
    /* @bin 0x004F4AE0 a main building (+0x38 == 0) listed in the current city's three slots
     * +0x473C: the slot's +0x4730 entry cleared; types 0xCF, 0xD0 call 0x004E5950 with the slot,
     * then 0x004F4A80 clears the slot */
    uint32_t r = BUILDING_ADDR(b);
    if (S16(st, r + 0x38) != 0) {
        return;
    }
    uint32_t c = U32(st, CURRENT_PLAYER_ADDR);
    int32_t i;
    for (i = 0; i < 3; i++) {
        if (S32(st, 0x00EA5FACu + c * 0x49D8u + 4u * (uint32_t) i) == b) {
            break;
        }
    }
    if (i >= 3) {
        return;
    }
    U32(st, 0x00EA5FA0u + 4u * (c * 0x1276u + (uint32_t) i)) = 0;
    int16_t type = S16(st, r + 0x10);
    if (type == 0xCF || type == 0xD0) {
        monument_slot_images_4e5950(st, i);
    }
    city_slot_clear_0x004f4a80(st, b, i);
}

void formation_clear_0x004b82e0(game_state *st, int32_t f)
{
    /* @bin 0x004B82E0 formation f (0x90 bytes at 0x00BCB9C8) in use: its figure +0x40 deleted
     * (0x004B4980), the record zeroed (0x0040F8F0), the formations counted again (0x004B77D0) */
    if (f <= 0) {
        return;
    }
    uint32_t r = 0x00BCB9C8u + 0x90u * (uint32_t) f;
    if (U8(st, r) == 0) {
        return;
    }
    int32_t fig = S16(st, r + 0x40);
    if (fig != 0) {
        GAME_CALL(st, 0x004B4980u, fig);
    }
    GAME_CALL(st, 0x0040F8F0u, (int32_t) r, 0x90, 0);
    GAME_CALL0(st, 0x004B77D0u);
}

void building_rubble_parts_0x004c04a0(game_state *st, int32_t b)
{
    /* @bin 0x004C04A0 a building of type 0xCA collapsing: released (0x0046AC50), its tiles
     * (0x004BFEE0), turned into type 0x63, then new 0x63 buildings (0x0046A400) on the tiles of
     * table 0x005CCD10 for each of the four parts of table 0x005DF78C (by orientation) */
    uint32_t r = BUILDING_ADDR(b);
    U32(st, 0x00BC9694u) = 0;
    S16(st, r + 0x48) = 0;
    S16(st, r + 0x46) = 0;
    U8(st, r + 0x43) = 0;
    S16(st, r + 0x20) = 0;
    GAME_CALL(st, 0x0046AC50u, b);
    GAME_CALL(st, 0x004BFEE0u, b);
    S16(st, r + 0x10) = 0x63;
    S16(st, r + 0x30) = 0;
    S16(st, r + 0x4A) = (int16_t) ((U8(st, r + 0x4D) & 7) + 1);
    U8(st, r + 0x4C) = 1;
    U8(st, r + 3) = 1;
    int32_t k = (int32_t) U8(st, r + 0xAA) + S32(st, ORIENT);
    if (k >= 8) {
        k -= 8;
    }
    for (int32_t j = 0; j < 4; j++) {
        uint32_t part = U32(st, 0x005DF78Cu + 16u * (uint32_t) k + 4u * (uint32_t) j);
        uint32_t e = 0x005DF814u + 16u * part; /* +0 count, +4, +8 offsets */
        int32_t x, y;
        if (k == 0) {
            x = S16(st, r + 6) + S32(st, e + 8);
            y = S16(st, r + 8) + S32(st, e + 4);
        } else if (k == 1) {
            x = S16(st, r + 6) + S32(st, e + 4);
            y = S16(st, r + 8) + S32(st, e + 8);
        } else {
            y = S16(st, r + 8);
            x = S16(st, r + 6);
        }
        for (int32_t n = 0; n < S32(st, e); n++) {
            uint32_t t = 0x005CCD10u + 8u * (uint32_t) n;
            int32_t nb = GAME_CALL(st, 0x0046A400u, S32(st, CURRENT_PLAYER_ADDR), 0x63, S32(st, t) + x,
                                   y + S32(st, t + 4), 0, 0);
            uint32_t nr = BUILDING_ADDR(nb);
            int32_t img = S32(st, 0x00F56C5Cu);
            switch (U8(st, RANDOM + U32(st, nr + 0xC)) & 3) {
            case 0:
                break;
            case 1:
                img += 9;
                break;
            case 2:
                img += 0x12;
                break;
            default:
                img += 0x1B;
                break;
            }
            GAME_CALL(st, 0x0047BF50u, nb, S16(st, nr + 6), S16(st, nr + 8), 1, 1, img, 8);
            S16(st, nr + 0x4A) = (int16_t) ((U8(st, nr + 0x4D) & 7) + 1);
            S16(st, nr + 0x30) = 0;
            U8(st, nr + 0x4C) = 1;
        }
    }
}

void building_area_clear_0x004c0920(game_state *st, int32_t b)
{
    /* @bin 0x004C0920 the 13 x 7 area of table 0x005DF9B0 (0x14 bytes a tile) in front of
     * building b by its orientation +0xAA (origin offsets 0x005E00CC): tiles of b (or of no
     * building when b is 0) are cleared; then the area's passes */
    uint32_t r = BUILDING_ADDR(b);
    int32_t dir = U8(st, r + 0xAA);
    int32_t h = dir / 2;
    int32_t x0 = S16(st, r + 6) + S32(st, 0x005E00CCu + 8u * (uint32_t) h);
    int32_t y0 = S16(st, r + 8) + S32(st, 0x005E00D0u + 8u * (uint32_t) h);
    int32_t oy = y0, ox = x0; /* the outer counters ([esp+0x24], [esp+0x20]) */
    int32_t ex = 0, ey = 0;
    uint32_t tab = 0x005DF9B0u;
    while (tab < 0x005E00CCu) {
        int32_t a = x0, c = y0;
        int32_t d1 = x0 - y0, d2 = y0 - x0;
        for (int32_t n = 13; n != 0; n--) {
            int32_t tx, ty;
            switch (dir) {
            case 0:
                ex = x0 + 13;
                ey = y0 + 7;
                tx = a;
                ty = oy;
                break;
            case 2:
                tx = ox;
                ty = c;
                ex = x0 - 7;
                ey = y0 - 13;
                break;
            case 4:
                tx = d1 + c;
                ty = ox + d2;
                ex = x0 - 13;
                ey = y0 - 7;
                break;
            default:
                tx = oy + d1;
                ty = a + d2;
                ex = x0 + 7;
                ey = y0 + 13;
                break;
            }
            int32_t t = S32(st, MAP_START) + ty * 228 + tx;
            if (U32(st, tab) != 0) {
                int skip = 0;
                if (b != 0) {
                    int32_t g = S16(st, BUILDING_GRID + 2u * (uint32_t) t);
                    if (g != b && GAME_CALL(st, 0x00475B50u, g) != b) {
                        skip = 1;
                    }
                    if (!skip && S16(st, r + 0x10) != 0x63) {
                        U8(st, 0x00A9010Cu + (uint32_t) t) = U8(st, r + 0x10);
                    }
                }
                if (!skip) {
                    uint32_t ta = TERRAIN + 4u * (uint32_t) t;
                    U8(st, BITS + (uint32_t) t) &= 0xE0;
                    S16(st, BUILDING_GRID + 2u * (uint32_t) t) = 0;
                    S16(st, 0x00A9CC1Cu + 2u * (uint32_t) t) = 0;
                    U8(st, EDGE + (uint32_t) t) = (uint8_t) ((U8(st, EDGE + (uint32_t) t) & 0xC0) | 0x40);
                    uint32_t ter = U32(st, ta) & 0xFFFFFFF7u;
                    U8(st, 0x00ADC36Cu + (uint32_t) t) = 0;
                    U32(st, ta) = ter;
                    U8(st, 0x00AE8E7Cu + (uint32_t) t) = 0;
                    if ((ter & 4) != 0 && (ter & 0x10000) == 0) {
                        U32(st, ta) = 4;
                        GAME_CALL(st, 0x0047A220u, tx, ty, 1);
                    } else {
                        U32(st, IMAGE + 4u * (uint32_t) t) = (U8(st, RANDOM + (uint32_t) t) & 7) + U32(st, 0x00F56B54u);
                        U32(st, ta) = ter & 0x03052E84u;
                    }
                }
            }
            tab += 0x14;
            c--;
            a++;
        }
        ox--;
        oy++;
    }
    GAME_CALL(st, 0x0047B590u, x0, y0, ex, ey);
    GAME_CALL(st, 0x004767D0u, x0, y0, ex, ey, 0, 0);
    GAME_CALL(st, 0x00477500u, x0, y0, ex, ey);
    GAME_CALL(st, 0x004777C0u, x0, y0, ex, ey);
    GAME_CALL(st, 0x00475CC0u, x0, y0, ex, ey, 1);
    GAME_CALL(st, 0x004776F0u, x0, y0, ex, ey);
    GAME_CALL(st, 0x00477890u, x0, y0, ex, ey);
    U8(st, 0x00E38DCAu) = 1;
}

static int32_t run_52b0(game_state *st, const int32_t *a)
{
    return monument_slot_type_4f52b0(st, a[0]);
}

static int32_t run_5950(game_state *st, const int32_t *a)
{
    monument_slot_images_4e5950(st, a[0]);
    return 0;
}

static int32_t run_b700(game_state *st, const int32_t *a)
{
    storage_take_45b700(st, a[0], a[1]);
    return 0;
}

static int32_t run_a2e0(game_state *st, const int32_t *a)
{
    building_0x0046a2e0(st, a[0]);
    return 0;
}

static int32_t run_4a80(game_state *st, const int32_t *a)
{
    city_slot_clear_0x004f4a80(st, a[0], a[1]);
    return 0;
}

static int32_t run_4ae0(game_state *st, const int32_t *a)
{
    building_0x004f4ae0(st, a[0]);
    return 0;
}

static int32_t run_5830(game_state *st, const int32_t *a)
{
    /* @bin 0x004E5830 calls 0x004F4AE0 */
    building_0x004f4ae0(st, a[0]);
    return 0;
}

static int32_t run_82e0(game_state *st, const int32_t *a)
{
    formation_clear_0x004b82e0(st, a[0]);
    return 0;
}

static int32_t run_04a0(game_state *st, const int32_t *a)
{
    building_rubble_parts_0x004c04a0(st, a[0]);
    return 0;
}

static int32_t run_0920(game_state *st, const int32_t *a)
{
    building_area_clear_0x004c0920(st, a[0]);
    return 0;
}

const ported_function rest_destroy_ports[] = {
    {0x0046A2E0u, "building_0x0046a2e0", 1, 0, 0, run_a2e0, 0},
    {0x004F52B0u, "monument_slot_type_4f52b0", 1, 32, 0, run_52b0, 0},
    {0x004E5950u, "monument_slot_images_4e5950", 1, 0, 0, run_5950, 0},
    {0x0045B700u, "storage_take_45b700", 2, 0, 0, run_b700, 0},
    {0x004F4A80u, "city_slot_clear_0x004f4a80", 2, 0, 0, run_4a80, 0},
    {0x004F4AE0u, "building_0x004f4ae0", 1, 0, 0, run_4ae0, 0},
    {0x004E5830u, "building_0x004e5830", 1, 0, 0, run_5830, 0},
    {0x004B82E0u, "formation_clear_0x004b82e0", 1, 0, 0, run_82e0, 0},
    {0x004C04A0u, "building_rubble_parts_0x004c04a0", 1, 0, 0, run_04a0, 0},
    {0x004C0920u, "building_area_clear_0x004c0920", 1, 0, 0, run_0920, 0},
};
const int rest_destroy_ports_count = sizeof(rest_destroy_ports) / sizeof(rest_destroy_ports[0]);
