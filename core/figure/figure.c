/* Imhotep - the figure tick: every figure's action, deletion of dead figures.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Figure record: docs/formats/save.md §5 (chunk 21). */
#include "figure/figure.h"

#include <string.h>

#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


/* profiling of the figure actions (per type: calls, total and longest time); never read by the
 * simulation */

void figure_profile_start(game_state *st, int32_t type)
{
    /* @bin 0x00489A50 GetTickCount(): the wall clock (core/sim/platform.h) */
    S32(st, G_PROF_TYPE) = type;
    S32(st, G_PROF_START) = (int32_t) platform_clock_ms(st);
    if (U8(st, G_PROF_PRECISE)) {
        state_unported(st, FN_FIGURE_PROFILE_START); /* QueryPerformanceCounter */
    }
}

void figure_profile_end(game_state *st)
{
    /* @bin 0x00489A80 */
    if (U8(st, G_PROF_PRECISE)) {
        state_unported(st, FN_FIGURE_PROFILE_END); /* QueryPerformanceCounter, x87 statistics */
        return;
    }
    S32(st, G_PROF_END) = (int32_t) platform_clock_ms(st); /* GetTickCount() */
    int32_t elapsed = S32(st, G_PROF_END) - S32(st, G_PROF_START);
    uint32_t s = G_PROF_STATS + 32u * U32(st, G_PROF_TYPE);
    S32(st, s) += 1;
    S32(st, s + 4) += elapsed;
    if (elapsed > S32(st, s + 8)) {
        S32(st, s + 8) = elapsed;
    }
}

void figure_update_all(game_state *st)
{
    /* @bin 0x00489B70 */
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + CITY_U4634) = 0; /* 0x00EA5EA4 */
    S32(st, c + CITY_U442C) = 0; /* 0x00EA5C9C */
    S32(st, c + CITY_U4728) = 0; /* 0x00EA5F98 */
    S16(st, c + CITY_U447A) = 0; /* 0x00EA5CEA */
    S32(st, c + CITY_U4644) = 0; /* 0x00EA5EB4 */
    S32(st, c + CITY_U4678) = 0; /* 0x00EA5EE8 */
    S32(st, c + CITY_U4704) = 0; /* 0x00EA5F74 */
    S32(st, G_FIGURES_UPDATED) = 0;
    if (S32(st, c + CITY_U4544) > 0) { /* 0x00EA5DB4 */
        S32(st, c + CITY_U4544) -= 1;
    }
    for (S32(st, G_CURRENT_FIGURE) = 1; S32(st, G_CURRENT_FIGURE) < FIGURE_COUNT; S32(st, G_CURRENT_FIGURE) += 1) {
        int32_t id = S32(st, G_CURRENT_FIGURE);
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + FIG_STATE) == 0) {
            continue;
        }
        /* @bin 0x00489BE2 a dangling link at +0x84 (to a figure that is not alive, or that does
         * not link back through its +0x82) is cleared */
        int16_t other = S16(st, f + FIG_TARGETED_BY);
        if (other != 0) {
            uint32_t o = FIGURE_ADDR(other);
            if (U8(st, o + FIG_STATE) != 1) {
                S16(st, f + FIG_TARGETED_BY) = 0;
            }
            if (S16(st, o + FIG_TARGET) != id) {
                S16(st, f + FIG_TARGETED_BY) = 0;
            }
        }
        S32(st, G_FIGURES_UPDATED) += 1;
        figure_profile_start(st, U8(st, f + FIG_TYPE));
        /* @bin 0x00489C52 the type's action, re-reading the current figure */
        GAME_CALL0(st, U32(st, G_ACTION_TABLE + 4u * U8(st, FIGURE_ADDR(S32(st, G_CURRENT_FIGURE)) + FIG_TYPE)));
        figure_profile_end(st);
        if (U8(st, FIGURE_ADDR(S32(st, G_CURRENT_FIGURE)) + FIG_STATE) == 2) { /* dead: deleted */
            figure_delete(st, S32(st, G_CURRENT_FIGURE));
        }
    }
}

/* @bin 0x004B4C93 .. 0x004B4CB8 building types whose workers (figure types 4, 9) are counted at
 * +0x104 / +0x106 of the building they come from */
static int counted_workplace(int16_t t)
{
    if (t >= 0x6E && t <= 0x72) {
        return 1;
    }
    switch (t) {
    case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2:
    case 0xBB: case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2:
    case 0xE7: case 0xE8: case 0xE9:
        return 1;
    default:
        return 0;
    }
}

/* @bin 0x004B4CE0 .. 0x004B4DE0 building +0xCE (u16) -> the resource a figure of it carries */
static uint32_t carried_resource(uint16_t v)
{
    switch (v) {
    case 1: return 0x10;
    case 3: return 0x18;
    case 4: return 0x14;
    case 5: return 0x0B;
    case 6: return 0x19;
    case 7: return 0x0C;
    case 8: return 0x0A;
    case 9: return 0x0E;
    case 10: return 0x12;
    case 0x0B: return 0x16;
    case 0x0C: return 0x17;
    case 0x0D: return 9;
    case 0x0E: return 0x1C;
    case 0x0F: return 0x15;
    case 0x10: return 0x11;
    case 0x11: return 0x1D;
    case 0x12: return 0x1E;
    case 0x14: return 0x20;
    case 0x15: return 2;
    case 0x16: return 8;
    case 0x17: return 0x1F;
    case 0x18: return 0x0D;
    case 0x19: return 0x21;
    case 0x1A: return 0x22;
    default: return 0xFFFFFFFFu;
    }
}

/* the type-specific part of figure_delete: what the figure held in its building, its city, ... */
static void release_links(game_state *st, int32_t id)
{
    uint32_t f = FIGURE_ADDR(id);
    uint32_t city = CURRENT_CITY(st);
    uint8_t type = U8(st, f + FIG_TYPE);
    if (type == 0x39) {
        S32(st, city + CITY_U4694) += 1; /* @bin 0x004B49A0 0x00EA5F04 */
    }
    if (type == 0x5D || type == 0x64) {
        S32(st, city + CITY_U49A0) -= 1; /* @bin 0x004B49C0 0x00EA6210 */
    }
    if ((type == 0x5C || type == 0x65 || type == 0x4D) && U8(st, f + FIG_U92) != 0) {
        U32(st, G_FORMATION_BASE + FM_U84 + 0x90u * U8(st, f + FIG_U92)) = 0; /* @bin 0x004B49FE */
    }
    int16_t b = S16(st, f + FIG_HOME); /* the building the figure belongs to */
    switch (type) {
    case 5:
    case 0x27:
        if (b != 0) {
            S16(st, BUILDING_ADDR(b) + BLD_WALKER2) = 0; /* @bin 0x004B4A1C */
        }
        return;
    case 0x4E:
    case 0x4D:
        GAME_CALL(st, FN_FIGURE_LEAVE_BATTLE, id);
        return;
    case 0x3E:
        S16(st, BUILDING_ADDR(b) + BLD_TOWER_WALKER) = 0;
        return;
    case 0x06: case 0x0E: case 0x3B: case 0x3C: case 0x3D: case 0x47: case 0x41: case 0x44: case 0x52:
    case 0x46: case 0x54: case 0x53: case 0x67: case 0x68: case 0x66: case 0x6D: case 0x42: case 0x28:
        return; /* @bin 0x004B4A60 nothing to release */
    default:
        break;
    }
    if (type == 0x4B || type == 0x5A || (type == 0x49 && U8(st, BUILDING_ADDR(b) + BLD_SHOWS) != 0)) {
        U8(st, BUILDING_ADDR(b) + BLD_SHOWS) -= 1; /* @bin 0x004B4B20 */
        return;
    }
    if (type == 0x62) {
        if (S32(st, city + CITY_U4980) < 1) { /* @bin 0x004B4B50 0x00EA61F0 */
            S32(st, city + CITY_U4980) = 0;
        } else {
            S32(st, city + CITY_U4980) -= 1;
        }
        return;
    }
    if (type == 0x0F) {
        S16(st, BUILDING_ADDR(b) + BLD_U9E) = 0;
        return;
    }
    if (type == 0x10) {
        S16(st, BUILDING_ADDR(b) + BLD_UA0) = 0;
        return;
    }
    if (type == 0x11) {
        S16(st, BUILDING_ADDR(b) + BLD_UA2) = 0; /* @bin 0x004B4BC1 */
        return;
    }
    if (type == 0x55) {
        int16_t s = S16(st, f + FIG_DEST_BUILDING);
        if ((int8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_NEEDS_AREA_CHECK, s) != 0) {
            S16(st, BUILDING_ADDR(s) + BLD_SECOND_WALKER) = 0;
        }
        return;
    }
    if (type == 0x26) {
        /* @bin 0x004B4C00 three figure slots at +0xAE */
        for (int i = 0; i < 3; i++) {
            if (S16(st, BUILDING_ADDR(b) + BLD_DOCKERS + 2u * (uint32_t) i) == id) {
                S16(st, BUILDING_ADDR(b) + BLD_DOCKERS + 2u * (uint32_t) i) = 0;
            }
        }
        return;
    }
    if (b == 0) {
        return;
    }
    if (S16(st, BUILDING_ADDR(b) + BLD_WALKER) == id) {
        S16(st, BUILDING_ADDR(b) + BLD_WALKER) = 0;
    } else if (S16(st, BUILDING_ADDR(b) + BLD_SECOND_CART) == id) {
        S16(st, BUILDING_ADDR(b) + BLD_SECOND_CART) = 0;
    }
    if (type != 4 && type != 9) {
        return;
    }
    /* @bin 0x004B4C7C cart pushers: the source building's goods in transit. From here the original
     * reads the current figure (0x00D056F8), not the argument. */
    int16_t src = S16(st, f + FIG_DEST_BUILDING);
    uint32_t cur = FIGURE_ADDR(S32(st, G_CURRENT_FIGURE));
    if (counted_workplace(S16(st, BUILDING_ADDR(src) + BLD_TYPE)) && S16(st, cur + FIG_AMOUNT) != 0) {
        uint32_t r = carried_resource(U16(st, BUILDING_ADDR(S16(st, cur + FIG_DEST_BUILDING)) + BLD_INPUT2));
        if (S16(st, BUILDING_ADDR(src) + BLD_TYPE) == 0xCC && U8(st, f + FIG_RESOURCE) == r) {
            S16(st, BUILDING_ADDR(src) + BLD_INCOMING_AMOUNT) -= 1;
        } else {
            S16(st, BUILDING_ADDR(src) + BLD_INCOMING_CARTS) -= 1;
        }
        return;
    }
    uint32_t cb = BUILDING_ADDR(S16(st, cur + FIG_DEST_BUILDING));
    int16_t amount = S16(st, cur + FIG_AMOUNT);
    if (S16(st, cb + BLD_TYPE) == 0x47 && amount != 0) {
        S16(st, cb + BLD_INCOMING_CARTS) -= 1; /* @bin 0x004B4E20 */
        S16(st, cb + BLD_INCOMING_AMOUNT) -= amount;
    }
}

void figure_delete(game_state *st, int32_t id)
{
    /* @bin 0x004B4980 */
    release_links(st, id);
    uint32_t f = FIGURE_ADDR(id);
    uint8_t slot = U8(st, f + FIG_U6E);
    if (slot != 0) {
        /* @bin 0x004B4E5B a record of 0x6A bytes at 0x0077DE54 that lists up to three figures */
        uint32_t r = G_EMPIRE_CITIES + EMPCITY_U54 + 0x6Au * slot;
        S16(st, r) = 0;
        if (S16(st, r + 6) == id) {
            S16(st, r + 6) = 0;
        } else if (S16(st, r + 8) == id) {
            S16(st, r + 8) = 0;
        } else if (S16(st, r + 0x0A) == id) {
            S16(st, r + 0x0A) = 0;
        }
    }
    if (S32(st, f + FIG_U11C) != 0) {
        U8(st, G_FERRY_SLOTS + 0x2Cu * U32(st, f + FIG_U11C)) = 0; /* @bin 0x004B4EC8 */
    }
    if (S16(st, f + FIG_TARGET_HOUSE) != 0) {
        S16(st, BUILDING_ADDR(S16(st, f + FIG_HOME)) + BLD_IMMIGRANT) = 0;
    }
    GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
    GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
    memset(state_ptr(st, f, FIGURE_SIZE), 0, FIGURE_SIZE);
}

static int32_t run_update_all(game_state *st, const int32_t *a)
{
    (void) a;
    figure_update_all(st);
    return 0;
}

static int32_t run_delete(game_state *st, const int32_t *a)
{
    figure_delete(st, a[0]);
    return 0;
}

static int32_t run_profile_start(game_state *st, const int32_t *a)
{
    figure_profile_start(st, a[0]);
    return 0;
}

static int32_t run_profile_end(game_state *st, const int32_t *a)
{
    (void) a;
    figure_profile_end(st);
    return 0;
}

const ported_function figure_ports[] = {
    {0x00489B70u, "figure_update_all", 0, 0, 0, run_update_all, 0},
    {0x004B4980u, "figure_delete", 1, 0, 0, run_delete, 0},
    {0x00489A50u, "figure_profile_start", 1, 0, 0, run_profile_start, 0},
    {0x00489A80u, "figure_profile_end", 0, 0, 0, run_profile_end, 0},
};
const int figure_ports_count = sizeof(figure_ports) / sizeof(figure_ports[0]);
