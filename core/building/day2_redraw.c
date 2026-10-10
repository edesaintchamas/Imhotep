/* Imhotep - every building's footprint image set again (package day2): 0x0046D840, run by the
 * daily dispatcher 0x00451C50, the view rotation 0x0047D6E0 and the scenario start 0x004D52A0.
 * The images go through 0x0047BF50, which also sets the footprint's terrain bits, building grid
 * and edge grid (simulation state).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/day2_redraw.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static int32_t orient_half(game_state *st)
{
    return S32(st, G_VIEW_ORIENTATION) / 2;
}

static void footprint(game_state *st, int32_t b, int32_t img, int32_t flags)
{
    uint32_t r = BUILDING_ADDR(b);
    int32_t s = U8(st, r + BLD_SIZE);
    GAME_CALL(st, FN_MAP_BUILDING_TILES, b, S16(st, r + BLD_X), S16(st, r + BLD_Y), s, s, img, flags);
}

/* @bin 0x0046DB52 the building's own orientation (+0xAA) in the view's */
static int32_t view_dir(game_state *st, uint32_t r)
{
    int32_t k = U8(st, r + BLD_ORIENTATION) - orient_half(st);
    return k < 0 ? k + 4 : k;
}

/* the gatehouse-like buildings: a different image group near the map's edge (0x0047A1C0) */
static void edge_choice(game_state *st, int32_t b, uint32_t fn, uint32_t near, uint32_t far)
{
    uint32_t r = BUILDING_ADDR(b);
    int32_t k = view_dir(st, r);
    int near_edge = (uint8_t) GAME_CALL(st, FN_FIGURE_NEAR_MAP_EDGE, S16(st, r + BLD_X), S16(st, r + BLD_Y)) != 0;
    GAME_CALL(st, fn, b, S16(st, r + BLD_X), S16(st, r + BLD_Y), S32(st, near_edge ? near : far) + k);
}

static int32_t monument_part_image(game_state *st, uint32_t r)
{
    /* @bin 0x0046D9C6 type 0xD2 by orientation and part (+0x12): images 0x00F56D54, 0x00F56D5C,
     * 0x00F56D64 (+1 for orientations 0 and 4) */
    int32_t a = S32(st, G_IMAGE_GROUPS + 8 * 564), c = S32(st, G_IMAGE_GROUPS + 8 * 565), e = S32(st, G_IMAGE_GROUPS + 8 * 566);
    int16_t p = S16(st, r + BLD_PART);
    switch (S32(st, G_VIEW_ORIENTATION)) {
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
    if (GAME_CALL(st, FN_BUILDING_MAIN_PART, b) == b && S32(st, r + BLD_UD8) != 0) {
        GAME_CALL(st, FN_BUILDING_AREA_REDRAW_440060, S32(st, r + BLD_AREA_TILE), type, b);
    }
}

void buildings_redraw(game_state *st)
{
    /* @bin 0x0046D840 */
    for (int32_t b = 1; b <= S32(st, G_BUILDING_HIGHEST); b++) {
        uint32_t r = BUILDING_ADDR(b);
        if (U8(st, r + BLD_STATE) == 0) {
            continue;
        }
        int32_t type = S16(st, r + BLD_TYPE);
        if (type >= 0x8C && type <= 0x95) {
            /* @bin 0x0046D878 table 0x005D06E4 (0x14-byte entries): the image group */
            footprint(st, b, S32(st, G_IMAGE_GROUPS + 8u * U32(st, G_MODELS + 8 + 0x14u * (uint32_t) type)), 8);
            continue;
        }
        if (type >= 0x29 && type <= 0x2B) {
            /* @bin 0x0046D8D7 +0xAA: low 4 bits the direction, high 4 bits the stage */
            uint8_t v = U8(st, r + BLD_ORIENTATION);
            int32_t k = (v & 0xF) - orient_half(st) - 1;
            if (k < 0) {
                k += 4;
            }
            int32_t stage = v >> 4, img;
            if (stage < 2) {
                img = S32(st, G_IMAGE_GROUPS + 8u * U32(st, G_MODELS + 8 + 0x14u * (uint32_t) type)) + stage * 4;
            } else if (U8(st, r + BLD_SIZE) == 1) {
                img = S32(st, G_IMAGE_GROUPS + 8 * 747) + stage * 4 - 8;
            } else if (U8(st, r + BLD_SIZE) == 2) {
                img = S32(st, G_IMAGE_GROUPS + 8 * 746) + stage * 4 - 8;
            } else {
                img = S32(st, G_IMAGE_GROUPS + 8 * 745) + stage * 4 - 8;
            }
            footprint(st, b, img + k, 8);
            continue;
        }
        switch (type) {
        case 0x3A: case 0xC8: case 0xAA: case 0xC9: case 0x38:
            continue;
        case 0xAB: case 0xCA:
            GAME_CALL(st, FN_BUILDING_REDRAW_4BFB30, b);
            continue;
        case 0xD2:
            footprint(st, b, monument_part_image(st, r), 8);
            continue;
        case 0x4A:
            edge_choice(st, b, FN_BUILDING_REDRAW_47D1F0, G_IMAGE_GROUPS + 8 * 723, G_IMAGE_GROUPS + 8 * 523);
            continue;
        case 0x4C:
            edge_choice(st, b, FN_MAP_PLACE_2X2, G_IMAGE_GROUPS + 8 * 727, G_IMAGE_GROUPS + 8 * 515);
            continue;
        case 0xB5:
            edge_choice(st, b, FN_MAP_PLACE_2X2, G_IMAGE_GROUPS + 8 * 725, G_IMAGE_GROUPS + 8 * 514);
            continue;
        case 0xB6:
            edge_choice(st, b, FN_BUILDING_REDRAW_47D1F0, G_IMAGE_GROUPS + 8 * 724, G_IMAGE_GROUPS + 8 * 525);
            continue;
        case 0x5A:
            GAME_CALL(st, FN_WATER_LIFT_REDRAW, b);
            continue;
        case 0x4B:
            edge_choice(st, b, FN_BUILDING_REDRAW_47D1F0, G_IMAGE_GROUPS + 8 * 729, G_IMAGE_GROUPS + 8 * 546);
            continue;
        case 0xC0:
            GAME_CALL(st, FN_BUILDING_REDRAW_47D1F0, b, S16(st, r + BLD_X), S16(st, r + BLD_Y), S32(st, G_IMAGE_GROUPS + 8 * 524) + view_dir(st, r));
            continue;
        case 0x88:
            edge_choice(st, b, FN_MAP_PLACE_2X2, G_IMAGE_GROUPS + 8 * 728, G_IMAGE_GROUPS + 8 * 520);
            continue;
        case 0x1F: case 0x21: case 0xD1:
            big_temple(st, b, type);
            continue;
        case 0x1E:
            if (U16(st, BUILDING_ADDR(S16(st, r + BLD_NEXT_PART)) + BLD_TYPE) == 0x1E) {
                GAME_CALL(st, FN_BUILDING_REDRAW_440540, b, S16(st, r + BLD_X), S16(st, r + BLD_Y));
            }
            big_temple(st, b, type);
            continue;
        default:
            break;
        }
        if (type >= 0x41 && type <= 0x45) {
            GAME_CALL(st, FN_BUILDING_REDRAW_4C0D50, b, 0);
            continue;
        }
        int32_t img;
        switch (type) {
        case 0xD5:
            GAME_CALL(st, FN_SUN_TEMPLE_REDRAW, b);
            break;
        case 0xD7:
            footprint(st, b, S32(st, G_IMAGE_GROUPS + 8 * 497), 0x48);
            break;
        case 0xDA:
            img = (orient_half(st) & 1) + S32(st, G_IMAGE_GROUPS + 8 * 760);
            if (img != 0) {
                footprint(st, b, img, 8);
            }
            break;
        case 0xDB:
            img = (orient_half(st) & 1) + S32(st, G_IMAGE_GROUPS + 8 * 761);
            if (img != 0) {
                footprint(st, b, img, 8);
            }
            break;
        case 0xDC:
            img = orient_half(st) + S32(st, G_IMAGE_GROUPS + 8 * 762);
            if (img != 0) {
                footprint(st, b, img, 8);
            }
            break;
        case 0xDE:
            GAME_CALL(st, FN_MONUMENT_PART_PLACE_4DFD90, b);
            break;
        case 0xE1:
            GAME_CALLT(st, G_MONUMENT_E1, FN_MONUMENT_REDRAW_40DCA0, b);
            break;
        case 0xE3:
            GAME_CALLT(st, G_MONUMENT_E3, FN_MONUMENT_REDRAW_4017A0, b);
            break;
        case 0xE6:
            GAME_CALLT(st, G_MONUMENT_E6, FN_MONUMENT_REDRAW_4017A0, b);
            break;
        case 0xE4:
            GAME_CALLT(st, G_MONUMENT_E4, FN_MONUMENT_REDRAW_4E69C0, b);
            break;
        case 0xE5:
            GAME_CALLT(st, G_MONUMENT_E5, FN_MONUMENT_REDRAW_5468D0, b);
            break;
        case 0xEA:
            GAME_CALLT(st, G_MONUMENT_EA, FN_MONUMENT_REDRAW_5468D0, b);
            break;
        case 0xEB:
            GAME_CALLT(st, G_MONUMENT_EB, FN_MONUMENT_REDRAW_5468D0, b);
            break;
        case 0xEC:
            GAME_CALLT(st, G_MONUMENT_EC, FN_MONUMENT_REDRAW_5468D0, b);
            break;
        default:
            break;
        }
    }
    GAME_CALL0(st, FN_MASTABAS_REDRAW_4EE560);
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
