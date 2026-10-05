/* Imhotep - building record states handled by the day (day step 0x28).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "building/day_state.h"

#include "building/day_access.h"
#include "random/random.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define BUILDING_HIGHEST 0x0078D790u
#define TERRAIN_GRID_ADDR 0x00AF598Cu

static void population_total(game_state *st, uint32_t c)
{
    /* @bin 0x004E919A city +0x24 := the sum of the 100 i16 at +0x25C4; its maximum at +0x469C */
    S32(st, c + 0x24) = 0;
    for (uint32_t i = 0; i < 100; i++) {
        S32(st, c + 0x24) += S16(st, c + 0x25C4 + 2 * i);
    }
    if (S32(st, c + 0x24) > S32(st, c + 0x469C)) {
        S32(st, c + 0x469C) = S32(st, c + 0x24);
    }
}

void city_people_add_4e9170(game_state *st, int32_t player, int32_t n)
{
    /* @bin 0x004E9170 */
    uint32_t c = CITY_ADDR(player);
    S32(st, c + 0x26E4) += n;
    GAME_CALL(st, 0x004E9350u, player, n);
    population_total(st, c);
}

void city_people_add_4e9250(game_state *st, int32_t player, int32_t n)
{
    /* @bin 0x004E9250 */
    uint32_t c = CITY_ADDR(player);
    S32(st, c + 0x26F8) += n;
    GAME_CALL(st, 0x004E9350u, player, n);
    population_total(st, c);
}

void building_free(game_state *st, int32_t id)
{
    /* @bin 0x0046AC10 0x0046AC50(id), the record zeroed, then 0x00485200 when the last action
     * (0x00BC9690) is 9 */
    GAME_CALL(st, 0x0046AC50u, id);
    uint32_t b = BUILDING_ADDR(id);
    for (uint32_t k = 0; k < 0x108; k++) {
        U8(st, b + k) = 0;
    }
    if (S32(st, 0x00BC9690u) == 9) {
        GAME_CALL0(st, 0x00485200u);
    }
}

void building_clear_tiles_4bfee0(game_state *st, int32_t id)
{
    /* @bin 0x004BFEE0 four entries of 16 bytes at 0x005DF814 (i32 value, dx, dy, -), the offset
     * turned by the building's +0xAA (0: (dy, dx), 1: (dx, dy), else none); 0x00BC9738 and
     * 0x00BC973C := value before each call */
    uint32_t b = BUILDING_ADDR(id);
    int32_t x = S16(st, b + 6), y = S16(st, b + 8);
    for (uint32_t e = 0x005DF818u; e < 0x005DF858u; e += 0x10) {
        int32_t tx, ty;
        uint8_t turn = U8(st, b + 0xAA);
        if (turn == 0) {
            tx = S32(st, e + 4) + x;
            ty = S32(st, e) + y;
        } else if (turn == 1) {
            tx = S32(st, e) + x;
            ty = S32(st, e + 4) + y;
        } else {
            tx = x;
            ty = y;
        }
        int32_t v = S32(st, e - 4);
        S32(st, 0x00BC9738u) = v;
        S32(st, 0x00BC973Cu) = v;
        GAME_CALL(st, 0x0047C480u, id, tx, ty);
    }
}

void map_refresh_486e70(game_state *st)
{
    /* @bin 0x00486E70 when 0x00BCA30C is set */
    if (U8(st, 0x00BCA30Cu) == 0) {
        return;
    }
    GAME_CALL0(st, 0x00478420u);
    GAME_CALL(st, 0x00487450u, 0, 0, S32(st, 0x00BCA2D0u) - 1, S32(st, 0x00BCA2D4u) - 1);
    GAME_CALL(st, 0x004877F0u, 0, 0, S32(st, 0x00BCA2D0u) - 1, S32(st, 0x00BCA2D4u) - 1);
    U8(st, 0x00BCA30Cu) = 0;
}

void buildings_states_update(game_state *st)
{
    /* @bin 0x004525F0 records in state 3 become used (1); records being removed (2, 6) clear
     * their tiles (by type) and are freed; state 4 (with +5: 0x004E9170 of +0x1C) and 5 are
     * freed. Walls, the 0xB7 map refresh and the routing grids are redone after, when needed. */
    int walls = 0, routing = 0, refresh = 0;
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) == 3) {
            U8(st, b) = 1;
        }
        uint8_t state = U8(st, b);
        if (state == 1 && U8(st, b + 5) != 0) {
            continue;
        }
        if (state == 4) {
            if (U8(st, b + 5) != 0) {
                GAME_CALL(st, 0x004E9170u, S32(st, CURRENT_PLAYER_ADDR), S16(st, b + 0x1C));
            }
            building_free(st, id);
            continue;
        }
        if (state == 5) {
            building_free(st, id);
            continue;
        }
        if (state != 2 && state != 6) {
            continue;
        }
        int16_t type = S16(st, b + 0x10);
        if (type == 0x3B || type == 0xAC || type == 0xAD || type == 0x3A || type == 0xAA) {
            walls = 1;
        }
        if (type == 0xAB) {
            building_clear_tiles_4bfee0(st, id);
            building_free(st, id);
            walls = 1;
            routing = 1;
            continue;
        }
        if (type == 0xC8 || type == 0xC9) {
            walls = 1;
        }
        if (type == 0xCA) {
            building_clear_tiles_4bfee0(st, id);
            building_free(st, id);
            walls = 1;
            routing = 1;
            continue;
        }
        if (type == 0xD1 || type == 0x1E || type == 0x1F || type == 0x21) {
            /* @bin 0x00452835 */
            if (building_main_part(st, id) == id) {
                GAME_CALL(st, 0x0043FD00u, id, S16(st, b + 0x10));
                routing = 1;
            }
            building_free(st, id);
            continue;
        }
        if (type == 0xB7) {
            refresh = 1;
        }
        /* @bin 0x00452735 monuments clear through their own objects (thiscall) */
        switch (type) {
        case 0xE1: GAME_CALLT(st, 0x006FD5D8u, 0x0040D890u, id); break;
        case 0xE6: GAME_CALLT(st, 0x006DE650u, 0x00401480u, id); break;
        case 0xE3: GAME_CALLT(st, 0x006FD368u, 0x00416FB0u, id); break;
        case 0xE4: GAME_CALLT(st, 0x006DE548u, 0x004E6660u, id); break;
        case 0xE5: GAME_CALLT(st, 0x006F9320u, 0x00545720u, id); break;
        case 0xEA: GAME_CALLT(st, 0x006F5648u, 0x00558BC0u, id); break;
        case 0xEB: GAME_CALLT(st, 0x006DE9E0u, 0x00555E60u, id); break;
        case 0xEC: GAME_CALLT(st, 0x006E9188u, 0x0054F450u, id); break;
        default: GAME_CALL(st, 0x0047C480u, id, S16(st, b + 6), S16(st, b + 8)); break;
        }
        routing = 1;
        int16_t t = S16(st, b + 0x10);
        if ((S32(st, 0x00BC9690u) == 9 && t == 0xE6) || t == 0xE3 || t == 0xE1 || t == 0xE4 || t == 0xE5 ||
            t == 0xEA || t == 0xEB || t == 0xEC) {
            S32(st, 0x00BC9694u) = 0; /* the undo is dropped */
        }
        building_free(st, id);
    }
    if (refresh) {
        U8(st, 0x00BCA30Cu) = 1;
        map_refresh_486e70(st);
    }
    if (walls) {
        GAME_CALL0(st, 0x0047B1A0u);
    }
    if (routing) {
        GAME_CALL0(st, 0x00517370u);
        GAME_CALL0(st, 0x00517660u);
    }
}

int32_t building_can_collapse(game_state *st, int32_t id)
{
    /* @bin 0x004696B0 0 for the monument types 0xB7, 0xCF, 0xD2, 0xD5, 0xD7, 0xDA..0xDC, 0xDE,
     * 0xE1, 0xE3..0xE6, 0xEA..0xEC */
    switch (S16(st, BUILDING_ADDR(id) + 0x10)) {
    case 0xB7: case 0xCF: case 0xD2: case 0xD5: case 0xD7: case 0xDA: case 0xDB: case 0xDC: case 0xDE:
    case 0xE1: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xEA: case 0xEB: case 0xEC:
        return 0;
    default:
        return 1;
    }
}

void day_fires_469730(game_state *st)
{
    /* @bin 0x00469730 day step 0x2B: the type-0x63 buildings (ruins on fire): +0x4A counts up;
     * past 32 the record goes to state 4 (0x0047C2B0); else, without +0xBD, listed at 0x0078DD90
     * (count 0x0078D804, index 0x0078D7FC capped at 499) and every 4th (0x00784CB0 == 2) or 8th
     * day, when the game RNG (one step, inline in the original) matches +0x4D, the fire spreads
     * to the neighbour in the wind direction 0x0078D808 (or the two beside it): 0x0046D100,
     * 0x004695D0(id, 1), sound 0x0053B090(0x12) */
    int routing = 0;
    S32(st, 0x0078D800u) = 0;
    S32(st, 0x0078D7FCu) = 0;
    S32(st, 0x0078D804u) = 0;
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1 || S16(st, b + 0x10) != 0x63) {
            continue;
        }
        if (S16(st, b + 0x4A) < 0) {
            S16(st, b + 0x4A) = 0;
        }
        int16_t age = (int16_t) (S16(st, b + 0x4A) + 1);
        S16(st, b + 0x4A) = age;
        if (age > 0x20) {
            U8(st, b) = 4;
            S32(st, 0x00BC9694u) = 0;
            GAME_CALL(st, 0x0047C2B0u, id, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3), U8(st, b + 3));
            routing = 1;
            continue;
        }
        if (U8(st, b + 0xBD) != 0) {
            continue;
        }
        int32_t k = S32(st, 0x0078D7FCu);
        S16(st, 0x0078DD90u + 2u * (uint32_t) k) = (int16_t) id;
        S32(st, 0x0078D804u) += 1;
        S32(st, 0x0078D7FCu) = k + 1 < 0x1F4 ? k + 1 : 0x1F3;
        if ((U8(st, 0x00784CB0u) == 2 ? (age & 3) : (age & 7)) != 0) {
            continue;
        }
        random_next_state(st);
        if (((U8(st, b + 0x4D) & 3) ^ (U32(st, 0x00D3B370u) & 3)) != 0) {
            continue;
        }
        int32_t dir = S32(st, 0x0078D808u);
        int32_t left = dir - 1 < 0 ? 7 : dir - 1;
        int32_t right = dir + 1 > 7 ? 0 : dir + 1;
        int32_t dirs[3] = {dir, left, right};
        int32_t tile = S32(st, b + 0xC);
        for (int i = 0; i < 3; i++) {
            int32_t n = S16(st, 0x00B4E6FCu + 2u * (uint32_t) (tile + S32(st, 0x005F80FCu + 4u * (uint32_t) dirs[i])));
            if (n != 0 && U8(st, BUILDING_ADDR(n) + 0x4C) == 0 && building_can_collapse(st, n)) {
                GAME_CALL(st, 0x0046D100u, n, 0);
                GAME_CALL(st, 0x004695D0u, n, 1);
                GAME_CALL(st, 0x0053B090u, 0x12);
                routing = 1;
                break;
            }
        }
    }
    if (routing) {
        GAME_CALL0(st, 0x00517370u);
        GAME_CALL0(st, 0x00517660u);
    }
}

void day_risks_468b70(game_state *st)
{
    /* @bin 0x00468B70 day step 0x2C: collapse (+0x46) and fire (+0x48) risks of the buildings
     * (each counted on its main part), unless 0x00978D70 == 1. Collapse risk grows by the type's
     * i16 at 0x00705EC0 (none for single-occupant houses or terrain 0x10000); at 1000 the
     * building collapses (message 0x70). Fire risk grows, on the day the building's tile hash
     * matches the RNG, by the type's 0x00705EBC times 1..5 (houses: only when inhabited); at 1000
     * it catches fire (message 0x6F). The game RNG steps once per day and once per candidate
     * (inline in the original). */
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + 0x4414) = 0;
    S32(st, c + 0x4418) = 0;
    int routing = 0;
    if (S32(st, 0x00978D70u) == 1) {
        return;
    }
    random_next_state(st);
    uint32_t day_key = U32(st, 0x00D3B370u) & 7;
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        int16_t type = S16(st, b + 0x10);
        if (U8(st, b) != 1 || type == 0x63 || S16(st, b + 0x4A) != 0) {
            continue;
        }
        if ((type == 0x20 || type == 0xE2 || type == 0xD2) && S16(st, b + 0x38) != 0) {
            continue;
        }
        if (type == 0x49 || (type >= 0x41 && type <= 0x45 && S16(st, b + 0x38) != 0)) {
            continue;
        }
        if (building_main_part(st, id) != id) {
            continue;
        }
        int32_t tile = S32(st, b + 0xC);
        uint32_t key = (U8(st, 0x00ACF85Cu + (uint32_t) tile) + (uint32_t) id) & 7;
        uint32_t t28 = 0x28u * (uint32_t) (int32_t) type;
        S16(st, b + 0x46) = (int16_t) (S16(st, b + 0x46) + S16(st, 0x00705EC0u + t28));
        if (S16(st, b + 0x4A) != 0) {
            S16(st, b + 0x46) = (int16_t) (S16(st, b + 0x46) + S16(st, 0x00705EC0u + t28) * 10);
        }
        if (U8(st, b + 5) != 0 && S16(st, b + 0x12) <= 1) {
            S16(st, b + 0x46) = 0;
        }
        if (U32(st, TERRAIN_GRID_ADDR + 4u * (uint32_t) tile) & 0x10000) {
            S16(st, b + 0x46) = 0;
        }
        if (S16(st, b + 0x46) >= 1000) {
            /* @bin 0x00468D75 the collapse */
            int32_t now = S32(st, 0x00E92E0Cu);
            if (now - S32(st, 0x00E950ECu) > 15000) {
                S32(st, 0x00E950ECu) = now;
            } else {
                S32(st, 0x00E950E0u) = 0;
            }
            if (U8(st, 0x007027BBu) == 0) {
                GAME_CALL(st, 0x00417AE0u, 3);
            } else {
                if (S32(st, 0x00E9512Cu) <= 0) {
                    S32(st, 0x00E950DCu) = 1;
                    S32(st, 0x00E9512Cu) = 0xC;
                }
                GAME_CALL(st, 0x004E0CF0u, 0x70, S16(st, b + 0x10), S32(st, b + 0xC), 0, 0);
                S32(st, 0x00E9517Cu) += 1;
            }
            U8(st, b) = 4;
            S32(st, 0x00BC9694u) = 0;
            if (S16(st, b + 0x10) == 0xCA) {
                GAME_CALL(st, 0x004C03D0u, id);
            } else {
                GAME_CALL(st, 0x0047C2B0u, id, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3), U8(st, b + 3));
            }
            GAME_CALL(st, 0x0048C0E0u, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3), 1);
            GAME_CALL(st, 0x004695D0u, id, 0);
            routing = 1;
            continue;
        }
        random_next_state(st);
        int16_t factor = (int16_t) ((U32(st, 0x00D3B370u) % 5) + 1);
        if (key != day_key) {
            continue;
        }
        int16_t fire = S16(st, 0x00705EBCu + t28);
        if (U8(st, b + 5) != 0) {
            if (S16(st, b + 0x1C) <= 0) {
                S16(st, b + 0x48) = 0;
            } else {
                S16(st, b + 0x48) = (int16_t) (S16(st, b + 0x48) + (int16_t) (fire * factor));
            }
        } else {
            if (U8(st, 0x00784CB0u) == 2 && S32(st, 0x00705EBCu + t28) != 0) {
                S16(st, b + 0x48) += 3;
            }
            S16(st, b + 0x48) = (int16_t) (S16(st, b + 0x48) + (int16_t) (S16(st, 0x00705EBCu + t28) * factor));
            if (S32(st, 0x00705EBCu + t28) == 0) {
                S16(st, b + 0x48) = 0;
            }
        }
        if (S16(st, b + 0x48) < 1000) {
            continue;
        }
        /* @bin 0x00468FB5 the fire */
        int32_t now = S32(st, 0x00E92E0Cu);
        int32_t last = S32(st, 0x00E950F0u);
        S16(st, b + 0x4A) = 1;
        if (now - last > 15000) {
            last = now;
            S32(st, 0x00E950F0u) = now;
        } else {
            S32(st, 0x00E950E0u) = 0;
        }
        if (U8(st, 0x007027B8u) == 0) {
            GAME_CALL(st, 0x00417AE0u, 0);
        } else {
            if (now - last > 15000) {
                S32(st, 0x00E950F0u) = now;
            } else {
                S32(st, 0x00E950E0u) = 0;
            }
            if (S32(st, 0x00E95128u) <= 0) {
                S32(st, 0x00E950DCu) = 1;
                S32(st, 0x00E95128u) = 0xC;
            }
            GAME_CALL(st, 0x004E0CF0u, 0x6F, S16(st, b + 0x10), S32(st, b + 0xC), 0, 0);
            S32(st, 0x00E95178u) += 1;
        }
        type = S16(st, b + 0x10);
        if (type == 0x1F || type == 0x1E || type == 0x21 || type == 0xD1 || (type >= 0x41 && type <= 0x45)) {
            GAME_CALL(st, 0x00468B20u, id);
        } else {
            S16(st, b + 0x4A) = 1;
            GAME_CALL(st, 0x0046D100u, id, 0);
            GAME_CALL(st, 0x004695D0u, id, 1);
        }
    }
    if (routing) {
        GAME_CALL0(st, 0x00517370u);
        GAME_CALL0(st, 0x00517660u);
    }
}

static int32_t run_people_4e9170(game_state *st, const int32_t *a)
{
    city_people_add_4e9170(st, a[0], a[1]);
    return 0;
}

static int32_t run_people_4e9250(game_state *st, const int32_t *a)
{
    city_people_add_4e9250(st, a[0], a[1]);
    return 0;
}

static int32_t run_free(game_state *st, const int32_t *a)
{
    building_free(st, a[0]);
    return 0;
}

static int32_t run_clear_tiles(game_state *st, const int32_t *a)
{
    building_clear_tiles_4bfee0(st, a[0]);
    return 0;
}

static int32_t run_refresh(game_state *st, const int32_t *a)
{
    (void) a;
    map_refresh_486e70(st);
    return 0;
}

static int32_t run_states_update(game_state *st, const int32_t *a)
{
    (void) a;
    buildings_states_update(st);
    return 0;
}

static int32_t run_can_collapse(game_state *st, const int32_t *a)
{
    return building_can_collapse(st, a[0]);
}

static int32_t run_fires(game_state *st, const int32_t *a)
{
    (void) a;
    day_fires_469730(st);
    return 0;
}

static int32_t run_risks(game_state *st, const int32_t *a)
{
    (void) a;
    day_risks_468b70(st);
    return 0;
}

const ported_function day_state_ports[] = {
    {0x00468B70u, "day_risks_468b70", 0, 0, 0, run_risks, 0},
    {0x004696B0u, "building_can_collapse", 1, 8, 0, run_can_collapse, 0},
    {0x00469730u, "day_fires_469730", 0, 0, 0, run_fires, 0},
    {0x004E9170u, "city_people_add_4e9170", 2, 0, 0, run_people_4e9170, 0},
    {0x004E9250u, "city_people_add_4e9250", 2, 0, 0, run_people_4e9250, 0},
    {0x0046AC10u, "building_free", 1, 0, 0, run_free, 0},
    {0x004BFEE0u, "building_clear_tiles_4bfee0", 1, 0, 0, run_clear_tiles, 0},
    {0x00486E70u, "map_refresh_486e70", 0, 0, 0, run_refresh, 0},
    {0x004525F0u, "buildings_states_update", 0, 0, 0, run_states_update, 0},
};
const int day_state_ports_count = sizeof(day_state_ports) / sizeof(day_state_ports[0]);
