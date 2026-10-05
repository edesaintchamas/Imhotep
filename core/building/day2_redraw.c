/* Imhotep - every building's footprint image set again (package day2): 0x0046D840, run by the
 * daily dispatcher 0x00451C50, the view rotation 0x0047D6E0 and the scenario start 0x004D52A0.
 * The images go through 0x0047BF50, which also sets the footprint's terrain bits, building grid
 * and edge grid (simulation state).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/day2_redraw.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define ORIENT 0x00BCA2E0u
#define GROUPS 0x00F55BB4u /* image group starts, 8 bytes each */

static int32_t orient_half(game_state *st)
{
    return S32(st, ORIENT) / 2;
}

static void footprint(game_state *st, int32_t b, int32_t img, int32_t flags)
{
    uint32_t r = BUILDING_ADDR(b);
    int32_t s = U8(st, r + 3);
    GAME_CALL(st, 0x0047BF50u, b, S16(st, r + 6), S16(st, r + 8), s, s, img, flags);
}

/* @bin 0x0046DB52 the building's own orientation (+0xAA) in the view's */
static int32_t view_dir(game_state *st, uint32_t r)
{
    int32_t k = U8(st, r + 0xAA) - orient_half(st);
    return k < 0 ? k + 4 : k;
}

/* the gatehouse-like buildings: a different image group near the map's edge (0x0047A1C0) */
static void edge_choice(game_state *st, int32_t b, uint32_t fn, uint32_t near, uint32_t far)
{
    uint32_t r = BUILDING_ADDR(b);
    int32_t k = view_dir(st, r);
    int near_edge = (uint8_t) GAME_CALL(st, 0x0047A1C0u, S16(st, r + 6), S16(st, r + 8)) != 0;
    GAME_CALL(st, fn, b, S16(st, r + 6), S16(st, r + 8), S32(st, near_edge ? near : far) + k);
}

static int32_t monument_part_image(game_state *st, uint32_t r)
{
    /* @bin 0x0046D9C6 type 0xD2 by orientation and part (+0x12): images 0x00F56D54, 0x00F56D5C,
     * 0x00F56D64 (+1 for orientations 0 and 4) */
    int32_t a = S32(st, 0x00F56D54u), c = S32(st, 0x00F56D5Cu), e = S32(st, 0x00F56D64u);
    int16_t p = S16(st, r + 0x12);
    switch (S32(st, ORIENT)) {
    case 0:
        return (p == 0 || p == 3 ? a : p == 1 || p == 4 ? c : e) + 1;
    case 4:
        return (p == 0 || p == 3 ? e : p == 1 || p == 4 ? c : a) + 1;
    case 6:
        return p == 0 || p == 3 ? a : p == 1 || p == 4 ? c : e;
    default:
        return p == 0 || p == 3 ? e : p == 1 || p == 4 ? c : a;
    }
}

static void big_temple(game_state *st, int32_t b, int32_t type)
{
    /* @bin 0x0046DE99 types 0x1E, 0x1F, 0x21, 0xD1: the main part (0x00475B50) with +0xD8 set
     * redraws its area (0x00440060 with its tile +0xD0) */
    uint32_t r = BUILDING_ADDR(b);
    if (GAME_CALL(st, 0x00475B50u, b) == b && S32(st, r + 0xD8) != 0) {
        GAME_CALL(st, 0x00440060u, S32(st, r + 0xD0), type, b);
    }
}

void buildings_redraw(game_state *st)
{
    /* @bin 0x0046D840 */
    for (int32_t b = 1; b <= S32(st, 0x0078D790u); b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r) == 0) {
            continue;
        }
        int32_t type = S16(st, r + 0x10);
        if (type >= 0x8C && type <= 0x95) {
            /* @bin 0x0046D878 table 0x005D06E4 (0x14-byte entries): the image group */
            footprint(st, b, S32(st, GROUPS + 8u * U32(st, 0x005D06E4u + 0x14u * (uint32_t) type)), 8);
            continue;
        }
        if (type >= 0x29 && type <= 0x2B) {
            /* @bin 0x0046D8D7 +0xAA: low 4 bits the direction, high 4 bits the stage */
            uint8_t v = U8(st, r + 0xAA);
            int32_t k = (v & 0xF) - orient_half(st) - 1;
            if (k < 0) {
                k += 4;
            }
            int32_t stage = v >> 4, img;
            if (stage < 2) {
                img = S32(st, GROUPS + 8u * U32(st, 0x005D06E4u + 0x14u * (uint32_t) type)) + stage * 4;
            } else if (U8(st, r + 3) == 1) {
                img = S32(st, 0x00F5730Cu) + stage * 4 - 8;
            } else if (U8(st, r + 3) == 2) {
                img = S32(st, 0x00F57304u) + stage * 4 - 8;
            } else {
                img = S32(st, 0x00F572FCu) + stage * 4 - 8;
            }
            footprint(st, b, img + k, 8);
            continue;
        }
        switch (type) {
        case 0x3A: case 0xC8: case 0xAA: case 0xC9: case 0x38:
            continue;
        case 0xAB: case 0xCA:
            GAME_CALL(st, 0x004BFB30u, b);
            continue;
        case 0xD2:
            footprint(st, b, monument_part_image(st, r), 8);
            continue;
        case 0x4A:
            edge_choice(st, b, 0x0047D1F0u, 0x00F5724Cu, 0x00F56C0Cu);
            continue;
        case 0x4C:
            edge_choice(st, b, 0x0047CF00u, 0x00F5726Cu, 0x00F56BCCu);
            continue;
        case 0xB5:
            edge_choice(st, b, 0x0047CF00u, 0x00F5725Cu, 0x00F56BC4u);
            continue;
        case 0xB6:
            edge_choice(st, b, 0x0047D1F0u, 0x00F57254u, 0x00F56C1Cu);
            continue;
        case 0x5A:
            GAME_CALL(st, 0x00487EC0u, b);
            continue;
        case 0x4B:
            edge_choice(st, b, 0x0047D1F0u, 0x00F5727Cu, 0x00F56CC4u);
            continue;
        case 0xC0:
            GAME_CALL(st, 0x0047D1F0u, b, S16(st, r + 6), S16(st, r + 8), S32(st, 0x00F56C14u) + view_dir(st, r));
            continue;
        case 0x88:
            edge_choice(st, b, 0x0047CF00u, 0x00F57274u, 0x00F56BF4u);
            continue;
        case 0x1F: case 0x21: case 0xD1:
            big_temple(st, b, type);
            continue;
        case 0x1E:
            if (U16(st, BUILDING_ADDR(S16(st, r + 0x3A)) + 0x10) == 0x1E) {
                GAME_CALL(st, 0x00440540u, b, S16(st, r + 6), S16(st, r + 8));
            }
            big_temple(st, b, type);
            continue;
        default:
            break;
        }
        if (type >= 0x41 && type <= 0x45) {
            GAME_CALL(st, 0x004C0D50u, b, 0);
            continue;
        }
        int32_t img;
        switch (type) {
        case 0xD5:
            GAME_CALL(st, 0x00541920u, b);
            break;
        case 0xD7:
            footprint(st, b, S32(st, 0x00F56B3Cu), 0x48);
            break;
        case 0xDA:
            img = (orient_half(st) & 1) + S32(st, 0x00F57374u);
            if (img != 0) {
                footprint(st, b, img, 8);
            }
            break;
        case 0xDB:
            img = (orient_half(st) & 1) + S32(st, 0x00F5737Cu);
            if (img != 0) {
                footprint(st, b, img, 8);
            }
            break;
        case 0xDC:
            img = orient_half(st) + S32(st, 0x00F57384u);
            if (img != 0) {
                footprint(st, b, img, 8);
            }
            break;
        case 0xDE:
            GAME_CALL(st, 0x004DFD90u, b);
            break;
        case 0xE1:
            GAME_CALLT(st, 0x006FD5D8u, 0x0040DCA0u, b);
            break;
        case 0xE3:
            GAME_CALLT(st, 0x006FD368u, 0x004017A0u, b);
            break;
        case 0xE6:
            GAME_CALLT(st, 0x006DE650u, 0x004017A0u, b);
            break;
        case 0xE4:
            GAME_CALLT(st, 0x006DE548u, 0x004E69C0u, b);
            break;
        case 0xE5:
            GAME_CALLT(st, 0x006F9320u, 0x005468D0u, b);
            break;
        case 0xEA:
            GAME_CALLT(st, 0x006F5648u, 0x005468D0u, b);
            break;
        case 0xEB:
            GAME_CALLT(st, 0x006DE9E0u, 0x005468D0u, b);
            break;
        case 0xEC:
            GAME_CALLT(st, 0x006E9188u, 0x005468D0u, b);
            break;
        default:
            break;
        }
    }
    GAME_CALL0(st, 0x004EE560u);
}

static int32_t run_redraw(game_state *st, const int32_t *a)
{
    (void) a;
    buildings_redraw(st);
    return 0;
}

const ported_function day2_redraw_ports[] = {
    {0x0046D840u, "buildings_redraw", 0, 0, 0, run_redraw, 0},
};
const int day2_redraw_ports_count = sizeof(day2_redraw_ports) / sizeof(day2_redraw_ports[0]);
