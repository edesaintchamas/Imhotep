/* Imhotep - the daily update of the formations (companies of soldiers, chunk 24) (package bigb).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Formation record: docs/formats/save.md §5 (chunk 24); figure record: chunk 21. */
#include "figure/formation.h"

#include <string.h>

#include "map/route_flood.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define NFORMATIONS 50

/* formation fields */
#define FM_IN_USE 0x00u    /* u8 */
#define FM_TYPE 0x04u      /* i16 */
#define FM_FIGURES 0x08u   /* 16 x i16 */
#define FM_COUNT 0x28u     /* u8 figures */
#define FM_X 0x2Eu         /* i16 */
#define FM_Y 0x30u         /* i16 */
#define FM_PLAYER 0x42u    /* u8 */
#define FM_HALTED 0x69u    /* u8 */

int32_t figure_type_is_0x44_0x46(int32_t type)
{
    /* @bin 0x004EA9A0 */
    return type >= 0x44 && type <= 0x46;
}

int32_t figure_type_is_enemy_0x52(int32_t type)
{
    /* @bin 0x004EA9C0 types 0x52 .. 0x54, 0x66 .. 0x68 */
    return (type >= 0x52 && type <= 0x54) || (type >= 0x66 && type <= 0x68);
}

int32_t formation_has_action_0x12(game_state *st, int32_t id)
{
    /* @bin 0x0040B5E0 one of the formation's figures (up to the first empty slot) in action 0x12 */
    uint32_t f = FORMATION_ADDR(id);
    for (uint32_t k = 0; k < 16; k++) {
        int16_t fid = S16(st, f + FM_FIGURES + 2 * k);
        if (fid == 0) {
            return 0;
        }
        if (U8(st, FIGURE_ADDR(fid) + 0x38) == 0x12) {
            return 1;
        }
    }
    return 0;
}

void formations_count(game_state *st)
{
    /* @bin 0x004B77D0 */
    uint32_t c = CURRENT_CITY(st) + 0x4698u;
    int32_t n = 0;
    S32(st, 0x00BCB9B8u) = 0;
    S32(st, 0x00BCB9B4u) = 0;
    S32(st, c) = 0;
    for (int32_t i = 1; i < NFORMATIONS; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FM_IN_USE) != 1 || U8(st, f + FM_PLAYER) == 0) {
            continue;
        }
        n++;
        S32(st, 0x00BCB9B8u) = i;
        if (S16(st, f + FM_TYPE) == 0xD) {
            S32(st, c) += 1;
        }
        if (S16(st, f + 0x5E) > 0) {
            continue;
        }
        for (uint32_t k = 0; k < 16; k++) {
            int16_t id = S16(st, f + FM_FIGURES + 2 * k);
            if (id == 0) {
                break;
            }
            uint32_t a = FIGURE_ADDR(id);
            uint8_t act = U8(st, a + 0x38);
            /* @bin 0x004B7843 the first alive figure not in actions 6, 7, 0x16, 0x17 */
            if (U8(st, a + 0x0E) == 1 && act != 6 && act != 7 && act != 0x16 && act != 0x17) {
                S16(st, f + FM_X) = S16(st, a + 0x14);
                S16(st, f + FM_Y) = S16(st, a + 0x16);
                break;
            }
        }
    }
    S32(st, 0x00BCB9B4u) = n;
}

void formations_soldier_counters(game_state *st)
{
    /* @bin 0x004B7920 */
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t a = FIGURE_ADDR(id);
        uint8_t t = U8(st, a + 0x0A);
        if (U8(st, a + 0x0E) == 1 && t >= 0xB && t <= 0xD && U8(st, a + 0x38) == 8 && U8(st, a + 0x1D) != 0) {
            U8(st, a + 0x1D) -= 1;
        }
    }
}

void formations_morale_player(game_state *st, int32_t delta)
{
    /* @bin 0x004B7C00 */
    for (int32_t i = 1; i < NFORMATIONS; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FM_IN_USE) == 1 && U8(st, f + FM_HALTED) == 0 && U8(st, f + FM_PLAYER) != 0) {
            GAME_CALL(st, 0x004B7B50u, i, delta);
        }
    }
}

void formations_morale_others(game_state *st, int32_t delta)
{
    /* @bin 0x004B7C40 */
    for (int32_t i = 1; i < NFORMATIONS; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FM_IN_USE) == 1 && U8(st, f + FM_HALTED) == 0 && U8(st, f + FM_PLAYER) == 0) {
            GAME_CALL(st, 0x004B7B50u, i, delta);
        }
    }
}

void formations_morale_days(game_state *st)
{
    /* @bin 0x004B79F0 formations with morale (+0x2C) at most 20 lower the others' morale once
     * (+0x66, +0x73 still 0), then count days at most 10 (+0x73) or at most 20 (+0x66) */
    for (int32_t i = 1; i < NFORMATIONS; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FM_IN_USE) != 1 || U8(st, f + FM_HALTED) != 0) {
            continue;
        }
        int32_t own, other;
        if (U8(st, f + FM_PLAYER) == 0) {
            own = 10;
            other = -10;
        } else {
            if (U8(st, f + 0x03) != 0 || S16(st, f + 0x82) >= 0) {
                continue;
            }
            own = -10;
            other = 10;
        }
        if (S16(st, f + 0x2C) <= 20 && U8(st, f + 0x66) == 0 && U8(st, f + 0x73) == 0) {
            formations_morale_player(st, own);
            formations_morale_others(st, other);
        }
        int16_t m = S16(st, f + 0x2C);
        if (m <= 10) {
            U8(st, f + 0x73) += 1;
        } else if (m <= 20) {
            U8(st, f + 0x66) += 1;
        }
    }
}

void formations_rebuild(game_state *st)
{
    /* @bin 0x004B7C80 */
    for (int32_t i = 1; i < NFORMATIONS; i++) {
        uint32_t f = FORMATION_ADDR(i);
        memset(state_ptr(st, f + FM_FIGURES, 32), 0, 32);
        U8(st, f + FM_COUNT) = 0;
        U8(st, f + 0x03) = 1;
        S16(st, f + 0x4A) = 0;
        S16(st, f + 0x4C) = 0;
    }
    /* @bin 0x004B7CCB the soldiers: types 0xB..0xD, 0x2B..0x35, 0x39, 0x36..0x38, 0x63 and those of
     * 0x004EA9A0 / 0x004EA9C0, alive and not in action 4 */
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t a = FIGURE_ADDR(id);
        if (U8(st, a + 0x0E) != 1 || U8(st, a + 0x38) == 4) {
            continue;
        }
        uint8_t t = U8(st, a + 0x0A);
        int member = (t >= 0xB && t <= 0xD) || (t >= 0x2B && t <= 0x35) || t == 0x39;
        if (!member) {
            member = figure_type_is_0x44_0x46(t) || figure_type_is_enemy_0x52(U8(st, a + 0x0A));
            t = U8(st, a + 0x0A);
            member = member || t == 0x63 || t == 0x36 || t == 0x37 || t == 0x38;
        }
        if (!member) {
            continue;
        }
        int32_t k = S16(st, a + 0x60);
        uint32_t fk = FORMATION_ADDR(k);
        U8(st, fk + FM_COUNT) += 1;
        /* @bin 0x004B7D4F the type's value at 0x007090D0 (records of 0x38 bytes) */
        S16(st, fk + 0x4C) = (int16_t) (S16(st, fk + 0x4C) + S16(st, 0x007090D0u + 0x38u * U8(st, a + 0x0A)));
        S16(st, fk + 0x4A) = (int16_t) (S16(st, fk + 0x4A) + U8(st, a + 0x1D));
        if (U8(st, a + 0x63) != 1) {
            U8(st, fk + 0x03) = 0;
        }
        for (uint32_t s = 0; s < 16; s++) {
            if (S16(st, fk + FM_FIGURES + 2 * s) == 0) {
                U8(st, a + 0x62) = (uint8_t) s;
                S16(st, fk + FM_FIGURES + 2 * s) = (int16_t) id;
                break;
            }
        }
    }
    S32(st, 0x00D0563Cu) = 0; /* enemy formations */
    S32(st, 0x00D05638u) = 0; /* their figures */
    S32(st, 0x00D05634u) = 0; /* own formations */
    S32(st, 0x00D05630u) = 0; /* their figures */
    /* empty enemy formations: their +0x78 noted (0x00BCA45C, flags 0x00BCA428), cleared */
    int32_t n = 0;
    for (int32_t i = 1; i < NFORMATIONS; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FM_IN_USE) != 1 || U8(st, f + FM_HALTED) != 0) {
            continue;
        }
        uint8_t count = U8(st, f + FM_COUNT);
        if (U8(st, f + FM_PLAYER) != 0) {
            if (count == 0) {
                continue;
            }
            /* @bin 0x004B7E0A +0x5A: all its figures in action 8 (+0x11) */
            int16_t before = S16(st, f + 0x5A);
            S16(st, f + 0x5A) = 1;
            for (uint32_t c = 0; c < U8(st, f + FM_COUNT); c++) {
                int16_t id = S16(st, f + FM_FIGURES + 2 * c);
                if (id != 0 && U8(st, FIGURE_ADDR(id) + 0x11) != 8) {
                    S16(st, f + 0x5A) = 0;
                }
            }
            S32(st, 0x00D05634u) += 1;
            S32(st, 0x00D05630u) += U8(st, f + FM_COUNT);
            if (S16(st, f + FM_TYPE) == 0xD) {
                if (before == 0 && S16(st, f + 0x5A) == 1) {
                    GAME_CALL(st, 0x0047D350u, 0x9E);
                }
                S32(st, 0x00D05630u) += U8(st, f + FM_COUNT) >> 1;
            }
            continue;
        }
        if (count != 0) {
            S32(st, 0x00D0563Cu) += 1;
            S32(st, 0x00D05638u) += count;
            continue;
        }
        int32_t v = S16(st, f + 0x78);
        int found = 0;
        for (int32_t e = 0; e < n; e++) {
            if (S32(st, 0x00BCA45Cu + 4u * (uint32_t) e) == v) {
                found = 1;
                break;
            }
        }
        if (!found) {
            S32(st, 0x00BCA45Cu + 4u * (uint32_t) n) = v;
            U8(st, 0x00BCA428u + (uint32_t) n) = 1;
            n++;
        }
        memset(state_ptr(st, f, FORMATION_SIZE), 0, FORMATION_SIZE); /* 0x0040F8F0 */
    }
    uint32_t c = CURRENT_CITY(st);
    U8(st, c + 0x466D) = 0;
    U8(st, c + 0x466F) = 0;
    U8(st, c + 0x466C) = 0;
    U8(st, c + 0x466A) = 0;
    U8(st, c + 0x466B) = 0;
    U8(st, c + 0x4665) = 0;
    U8(st, c + 0x4666) = 0;
    for (int32_t i = 1; i < NFORMATIONS; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FM_IN_USE) != 1) {
            continue;
        }
        if (U8(st, f + FM_PLAYER) == 0) {
            int32_t v = S16(st, f + 0x78);
            for (int32_t e = 0; e < n; e++) {
                if (v == S32(st, 0x00BCA45Cu + 4u * (uint32_t) e)) {
                    U8(st, 0x00BCA428u + (uint32_t) e) = 0;
                }
            }
            continue;
        }
        int r = formation_has_action_0x12(st, i);
        uint32_t cc = CURRENT_CITY(st);
        U8(st, cc + 0x466C) += 1;
        if (r) {
            U8(st, cc + 0x466A) += 1;
        } else {
            U8(st, cc + 0x466B) += 1;
        }
        if (U8(st, f + 0x67) != 0 && U8(st, f + FM_COUNT) != 0) {
            U8(st, cc + 0x466D) += 1;
            if (r) {
                U8(st, cc + 0x4665) += 1;
            } else {
                U8(st, cc + 0x4666) += 1;
            }
        }
        U8(st, cc + 0x466F) += U8(st, f + FM_COUNT);
    }
    /* @bin 0x004B803F the noted values no formation uses any more: record (0x00785D74, 0x7C bytes,
     * 0x00785D70 of them) +0x32 := 0 */
    for (int32_t e = 0; e < n; e++) {
        int32_t v = S32(st, 0x00BCA45Cu + 4u * (uint32_t) e);
        if (U8(st, 0x00BCA428u + (uint32_t) e) != 0 && v >= 0 && v < S32(st, 0x00785D70u)) {
            U8(st, 0x00785D74u + 0x7Cu * (uint32_t) v + 0x32u) = 0;
        }
    }
}

void formations_reset_0x29(game_state *st)
{
    /* @bin 0x004B8080 */
    for (int32_t i = 1; i < NFORMATIONS; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FM_IN_USE) == 1 && U8(st, f + FM_PLAYER) != 0) {
            U8(st, f + 0x29) = 0x10;
        }
    }
}

void formations_facing(game_state *st)
{
    /* @bin 0x004B8130 */
    for (int32_t i = 1; i < NFORMATIONS; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FM_IN_USE) != 1 || U8(st, f + FM_HALTED) != 0) {
            continue;
        }
        int16_t x = S16(st, f + FM_X), px = S16(st, f + 0x6C);
        int16_t y = S16(st, f + FM_Y), py = S16(st, f + 0x6E);
        int32_t dx = x < px ? px - x : x - px;
        int32_t dy = y < py ? py - y : y - py;
        if (U8(st, f + 0x70) != 0) {
            U8(st, f + 0x70) -= 1;
        } else if (S16(st, f + 0x5C) != 0) {
            /* @bin 0x004B81A8 the direction of its first figure */
            U8(st, f + 0x6B) = U8(st, FIGURE_ADDR(S16(st, f + FM_FIGURES)) + 0x11);
        } else {
            int16_t layout = S16(st, f + 0x2A);
            int horizontal;
            switch (layout) {
            case 1: case 3:
                horizontal = 0;
                break;
            case 2: case 4:
                horizontal = 1;
                break;
            case 0: case 5:
                horizontal = dx > dy;
                break;
            default:
                horizontal = -1;
                break;
            }
            if (horizontal == 1) {
                if (x < px) {
                    U8(st, f + 0x6B) = 6;
                } else if (x > px) {
                    U8(st, f + 0x6B) = 2;
                }
            } else if (horizontal == 0) {
                if (y < py) {
                    U8(st, f + 0x6B) = 0;
                } else if (y > py) {
                    U8(st, f + 0x6B) = 4;
                }
            }
        }
        S16(st, f + 0x6C) = S16(st, f + FM_X);
        S16(st, f + 0x6E) = S16(st, f + FM_Y);
    }
}

void formations_timers(game_state *st)
{
    /* @bin 0x004B8B20 formations 1 .. 6 of the player */
    for (int32_t i = 1; i <= 6; i++) {
        uint32_t f = FORMATION_ADDR(i);
        if (U8(st, f + FM_IN_USE) != 1 || U8(st, f + FM_PLAYER) == 0) {
            continue;
        }
        static const uint8_t timers[4] = {0x64, 0x5C, 0x5E, 0x50};
        for (int k = 0; k < 4; k++) {
            if (S16(st, f + timers[k]) != 0) {
                S16(st, f + timers[k]) -= 1;
            }
        }
        uint32_t c = CURRENT_CITY(st);
        if (S32(st, c + 0x4634) <= 0) {
            S16(st, f + 0x50) = 0;
            S16(st, f + 0x5E) = 0;
            S16(st, f + 0x5C) = 0;
        }
        for (uint32_t k = 0; k < 16; k++) {
            if (U8(st, FIGURE_ADDR(S16(st, f + FM_FIGURES + 2 * k)) + 0x38) == 5) {
                S16(st, f + 0x50) = 6;
            }
        }
        if (U8(st, f + 0x73) == 0 && U8(st, f + 0x66) == 0) {
            if (S16(st, f + 0x2A) != 6) {
                continue;
            }
            /* @bin 0x004B8BD8 no enemies left: back to the layout kept at +0x62 */
            if (S32(st, c + 0x442C) + S32(st, c + 0x4728) + S32(st, 0x00D0563Cu) <= 0) {
                S16(st, f + 0x2A) = S16(st, f + 0x62);
                continue;
            }
            for (uint32_t k = 0; k < 16; k++) {
                int16_t id = S16(st, f + FM_FIGURES + 2 * k);
                if (id > 0) {
                    uint32_t a = FIGURE_ADDR(id);
                    if (U8(st, a + 0x38) != 5 && U8(st, a + 0x38) != 4) {
                        U8(st, a + 0x38) = 0xE;
                    }
                }
            }
            continue;
        }
        for (uint32_t k = 0; k < 16; k++) {
            int16_t id = S16(st, f + FM_FIGURES + 2 * k);
            uint32_t a = FIGURE_ADDR(id);
            uint8_t act = U8(st, a + 0x38);
            if (act != 5 && act != 4 && act != 3) {
                U8(st, a + 0x38) = 3;
                GAME_CALL(st, 0x004B3250u, id);
            }
        }
    }
}

void formations_target_building(game_state *st, int32_t id)
{
    /* @bin 0x004BA690 from formation `id`'s position, over grid 0x00EC7630 (0x0051B040), the reachable
     * building in use (not of types 0x50, 0x58, 0x59, 0x5D, 0x48, 0xC1, 0x39) nearest to the city's
     * point (+0x45A0, +0x45A4): its position and id at +0x3A, +0x3C, +0x3E */
    uint32_t f = FORMATION_ADDR(id);
    route_flood_ec7630(st, S16(st, f + FM_X), S16(st, f + FM_Y), -2, -2);
    int32_t best = 0, lowest = 10000;
    for (int32_t b = 1; b < BUILDING_COUNT; b++) {
        uint32_t a = BUILDING_ADDR(b);
        if (U8(st, a) != 1) {
            continue;
        }
        int16_t t = S16(st, a + 0x10);
        if (t == 0x50 || t == 0x58 || t == 0x59 || t == 0x5D || t == 0x48 || t == 0xC1 || t == 0x39) {
            continue;
        }
        if (S32(st, 0x00EED760u + 4u * (uint32_t) S32(st, a + 0x0C)) <= 0) {
            continue;
        }
        uint32_t c = CURRENT_CITY(st);
        int32_t cx = S32(st, c + 0x45A0), cy = S32(st, c + 0x45A4);
        int32_t bx = S16(st, a + 6), by = S16(st, a + 8);
        int32_t dx = cx > bx ? cx - bx : cx < bx ? bx - cx : 0;
        int32_t dy = cy > by ? cy - by : cy < by ? by - cy : 0;
        S32(st, 0x00D3B4FCu) = dx;
        S32(st, 0x00D3B4F8u) = dy;
        int32_t d = dx < dy ? dy : dx;
        if (d < lowest) {
            best = b;
            lowest = d;
        }
    }
    if (best > 0) {
        uint32_t a = BUILDING_ADDR(best);
        S16(st, f + 0x3A) = S16(st, a + 6);
        S16(st, f + 0x3C) = S16(st, a + 8);
        S16(st, f + 0x3E) = (int16_t) best;
    }
}

void formations_daily(game_state *st, int32_t arg)
{
    /* @bin 0x004B7790 */
    formations_count(st);
    formations_rebuild(st);
    formations_facing(st);
    formations_soldier_counters(st);
    if (arg == 0) {
        formations_morale_days(st);
    }
    formations_reset_0x29(st);
    formations_timers(st);
    GAME_CALL0(st, 0x004B8CF0u);
    formations_invaders(st);
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_count, formations_count)
VOID0(run_rebuild, formations_rebuild)
VOID0(run_facing, formations_facing)
VOID0(run_soldier_counters, formations_soldier_counters)
VOID0(run_morale_days, formations_morale_days)
VOID0(run_reset_0x29, formations_reset_0x29)
VOID0(run_timers, formations_timers)


static int32_t run_target_building(game_state *st, const int32_t *a)
{
    formations_target_building(st, a[0]);
    return 0;
}

static int32_t run_type_0x44(game_state *st, const int32_t *a)
{
    (void) st;
    return figure_type_is_0x44_0x46(a[0]);
}

static int32_t run_type_0x52(game_state *st, const int32_t *a)
{
    (void) st;
    return figure_type_is_enemy_0x52(a[0]);
}

static int32_t run_action_0x12(game_state *st, const int32_t *a)
{
    return formation_has_action_0x12(st, a[0]);
}

static int32_t run_daily(game_state *st, const int32_t *a)
{
    formations_daily(st, a[0]);
    return 0;
}

static int32_t run_morale_player(game_state *st, const int32_t *a)
{
    formations_morale_player(st, a[0]);
    return 0;
}

static int32_t run_morale_others(game_state *st, const int32_t *a)
{
    formations_morale_others(st, a[0]);
    return 0;
}

const ported_function bigb_formation_ports[] = {
    {0x004B7790u, "formations_daily", 1, 0, 0, run_daily, 0},
    {0x004B77D0u, "formations_count", 0, 0, 0, run_count, 0},
    {0x004B7C80u, "formations_rebuild", 0, 0, 0, run_rebuild, 0},
    {0x004B8130u, "formations_facing", 0, 0, 0, run_facing, 0},
    {0x004B7920u, "formations_soldier_counters", 0, 0, 0, run_soldier_counters, 0},
    {0x004B79F0u, "formations_morale_days", 0, 0, 0, run_morale_days, 0},
    {0x004B7C00u, "formations_morale_player", 1, 0, 0, run_morale_player, 0},
    {0x004B7C40u, "formations_morale_others", 1, 0, 0, run_morale_others, 0},
    {0x004B8080u, "formations_reset_0x29", 0, 0, 0, run_reset_0x29, 0},
    {0x004B8B20u, "formations_timers", 0, 0, 0, run_timers, 0},
    {0x004BA690u, "formations_target_building", 1, 0, 0, run_target_building, 0},
    {0x004EA9A0u, "figure_type_is_0x44_0x46", 1, 8, 0, run_type_0x44, 0},
    {0x004EA9C0u, "figure_type_is_enemy_0x52", 1, 8, 0, run_type_0x52, 0},
    {0x0040B5E0u, "formation_has_action_0x12", 1, 32, 0, run_action_0x12, 0},
};
const int bigb_formation_ports_count = sizeof(bigb_formation_ports) / sizeof(bigb_formation_ports[0]);
