/* Imhotep - actions of the scenario events: floods, the city's mood, plagues of Egypt (package
 * month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/events_actions.h"

#include "city/month.h"
#include "random/crt_rand.h"
#include "random/random.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

void city_mood_add(game_state *st, int32_t delta)
{
    /* @bin 0x004E85D0 city +0x18 += delta, then within 0..100 */
    uint32_t a = CURRENT_CITY(st) + CITY_HEALTH;
    int32_t v = (int32_t) (U32(st, a) + (uint32_t) delta);
    S32(st, a) = v;
    if (v > 0x64) {
        S32(st, a) = 0x64;
    }
    if (S32(st, a) < 0) {
        S32(st, a) = 0;
    }
}

/* the flood manager 0x00D05720 (thiscall `this`) */
int32_t flood_month_before(game_state *st, uint32_t self)
{
    /* @bin 0x004BECF0 (thiscall) the month before the flood's (+0x32FB0), 0..11 */
    int32_t m = (int32_t) (U32(st, self + 0x32FB0) - 1u);
    if (m < 0) {
        m += 0xC;
    }
    return m;
}

void flood_quality_add(game_state *st, uint32_t q, int32_t delta)
{
    /* @bin 0x004BD5E0 (thiscall on the flood manager + 0x32C40) +0x1C += delta, within 0..100 */
    int32_t v = (int32_t) (U32(st, q + 0x1C) + (uint32_t) delta);
    S32(st, q + 0x1C) = v;
    if (v < 0) {
        v = 0;
    } else if (v > 0x64) {
        S32(st, q + 0x1C) = 0x64;
        return;
    }
    S32(st, q + 0x1C) = v;
}

static int32_t rand_mod8(game_state *st)
{
    /* @bin 0x004BED08 rand() % 8 by and 0x80000007 and the sign fix */
    return crt_rand(st) % 8;
}

void flood_worse(game_state *st, uint32_t self)
{
    /* @bin 0x004BED00 (thiscall) the next flood's quality -= 30..100 (crt rand) */
    int32_t r = rand_mod8(st);
    flood_quality_add(st, self + 0x32C40, (int32_t) ((uint32_t) (-3 - r) * 10u));
}

void flood_better(game_state *st, uint32_t self)
{
    /* @bin 0x004BED40 (thiscall) the next flood's quality += 30..100 (crt rand) */
    int32_t r = rand_mod8(st);
    flood_quality_add(st, self + 0x32C40, (int32_t) ((uint32_t) (r + 3) * 10u));
}

void plague_frogs_end(game_state *st)
{
    /* @bin 0x004E6EB0 0x0078D79C := -1; buildings' +0xC2 -15 (0x004E8E80) */
    S32(st, G_LOCUST_TIMER) = -1;
    buildings_add_0xc2(st, -0xF);
}

/* the sound of a plague: channels 0 and 2 stopped when playing (0x0055F5A1 / 0x0055F561 on the
 * sound object 0x00E92BF8), 0x00F55B94 := 1, the ambient track `track` started */
static void plague_sound(game_state *st, int32_t track)
{
    if (GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 0) != 0) {
        GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, 0);
    }
    S32(st, G_UF55B94) = 1;
    if (GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 2) != 0) {
        GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, 2);
    }
    GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, track, 2, 1);
}

void plague_hail(game_state *st)
{
    /* @bin 0x004E6ED0 each figure alive draws a game RNG step (inlined): rand % 100 at most 25 (+0x38
     * == 8), 50 (types 0xB..0xD, 0x2B..0x39, 0x63, or 0x004EA9C0), 50 / 75 (0x004EADA0): +0x38 := 4;
     * the hail sound, buildings' +0xC2 -15, 0x0078D7A0 := 0x198 */
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + FIG_STATE) == 0) {
            continue;
        }
        random_next_state(st);
        int32_t r = (int32_t) (U32(st, G_R_1_15) & 0x7FFF) % 100;
        int32_t lim;
        uint8_t t = U8(st, f + FIG_TYPE);
        if (U8(st, f + FIG_ACTION) == 8) {
            lim = 0x19;
        } else if (t == 0xB || t == 0xC || t == 0xD || (t >= 0x2B && t <= 0x39) || t == 0x63) {
            lim = 0x32;
        } else if ((uint8_t) GAME_CALL(st, FN_FIGURE_TYPE_IS_ENEMY_0X52, t) != 0) {
            lim = 0x32;
        } else {
            lim = (uint8_t) GAME_CALL(st, FN_TYPE_IS_BOAT, U8(st, f + FIG_TYPE)) != 0 ? 0x32 : 0x4B;
        }
        if (r <= lim) {
            U8(st, f + FIG_ACTION) = 4;
        }
    }
    plague_sound(st, G_U5C92F8); /* "audio/ambient/hailstorm.mp3" */
    buildings_add_0xc2(st, -0xF);
    S32(st, G_HAIL_TIMER) = 0x198;
}

void plague_frogs(game_state *st, int32_t months)
{
    /* @bin 0x004E70B0 0x0078D7A8 := months * 0x330; in the list of active buildings (0x004583F0:
     * 0x0078D83C entries at 0x0078E178, cursor 0x0078D840) the highest type (+0x10) from 10 down,
     * 0x004E7300 on buildings while their +0x1C sum stays within 65 percent of all; the frogs' sound,
     * buildings' +0xC2 -15 */
    S32(st, G_FROGS_COUNTDOWN) = (int32_t) ((uint32_t) months * 0x330u);
    GAME_CALL0(st, FN_BUILDINGS_LIST_ACTIVE);
    int32_t n = S32(st, G_HOUSE_LIST_COUNT), hi = 0, sum = 0, done = 0;
    S32(st, G_HOUSE_LIST_INDEX) = 0;
    if (n > 0) {
        S32(st, G_HOUSE_LIST_INDEX) = n;
        for (int32_t j = 0; j < n; j++) {
            uint32_t b = BUILDING_ADDR(S16(st, G_HOUSE_LIST + 2u * (uint32_t) j));
            if (S16(st, b + BLD_TYPE) > hi) {
                hi = S16(st, b + BLD_TYPE);
            }
            sum += S16(st, b + BLD_POPULATION);
        }
    }
    if (hi >= 0xA) {
        int32_t lim = (int32_t) ((uint32_t) sum * 0x41u) / 100;
        for (; done < lim && hi >= 0xA; hi--) {
            S32(st, G_HOUSE_LIST_INDEX) = 0;
            int32_t j = 0;
            while (j < S32(st, G_HOUSE_LIST_COUNT)) {
                int16_t id = S16(st, G_HOUSE_LIST + 2u * (uint32_t) j);
                j++;
                S32(st, G_HOUSE_LIST_INDEX) = j;
                uint32_t b = BUILDING_ADDR(id);
                if (S16(st, b + BLD_TYPE) == hi) {
                    done += S16(st, b + BLD_POPULATION);
                    if (done <= lim) {
                        GAME_CALL(st, FN_FROG_MAYBE_4E7300, id);
                        j = S32(st, G_HOUSE_LIST_INDEX);
                    }
                }
            }
        }
    }
    plague_sound(st, G_U5E4940); /* "audio/ambient/frogs.mp3" */
    buildings_add_0xc2(st, -0xF);
}

void plague_flies(game_state *st, int32_t months)
{
    /* @bin 0x004E7200 months within 3..9; each building in use (+5): +0xE8 and +0xFC += 5000 /
     * (+0xF4 + 1), +0x77 at most +0xF4 * 3; figures of type 0x5F leave the tile lists (0x004B5420),
     * type 0x19 going out (+0x38 9 / 10) turn back (+0x38 := 13, 0x004B3250); 0x0078D7B0 and
     * 0x0078D7AC := months * 0x330, buildings' +0xC2 -15 */
    int32_t m = months < 3 ? 3 : months > 9 ? 9 : months;
    int32_t n = S32(st, G_BUILDING_HIGHEST);
    for (int32_t i = 1; i <= n; i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        S32(st, b + BLD_MALARIA_RISK) += 5000 / (S32(st, b + BLD_DIST_TERRAIN_4) + 1);
        S32(st, b + BLD_DISEASE_RISK) += 5000 / (S32(st, b + BLD_DIST_TERRAIN_4) + 1);
        int32_t c = (int32_t) ((uint32_t) S32(st, b + BLD_DIST_TERRAIN_4) * 3u);
        if (c < (int32_t) U8(st, b + BLD_ACCESS_WATER)) {
            U8(st, b + BLD_ACCESS_WATER) = (uint8_t) c;
        }
    }
    for (int32_t id = 1; id < FIGURE_COUNT; id++) {
        uint32_t f = FIGURE_ADDR(id);
        if (U8(st, f + FIG_STATE) == 0) {
            continue;
        }
        if (U8(st, f + FIG_TYPE) == 0x5F) {
            GAME_CALL(st, FN_FIGURE_TILE_LIST_REMOVE, id);
        } else if (U8(st, f + FIG_TYPE) == 0x19 && (U8(st, f + FIG_ACTION) == 0xA || U8(st, f + FIG_ACTION) == 9)) {
            S16(st, f + FIG_WAIT_TICKS) = 0;
            U8(st, f + FIG_ACTION) = 0xD;
            S16(st, f + FIG_DEST_X) = S16(st, f + FIG_SOURCE_X);
            S16(st, f + FIG_DEST_Y) = S16(st, f + FIG_SOURCE_Y);
            GAME_CALL(st, FN_FIGURE_ROUTE_RELEASE, id);
        }
    }
    int32_t v = (int32_t) ((uint32_t) m * 0x330u);
    S32(st, G_BLOOD_RIVER_LEFT) = v;
    S32(st, G_BLOOD_RIVER_DURATION) = v;
    buildings_add_0xc2(st, -0xF);
}

void plague_fire(game_state *st, int32_t times)
{
    /* @bin 0x00469C60 `times` times: the working building in use (+0 == 1, +5, +0x1C) with the
     * highest +0xE4 (its +0x2A figure dropped unless a living 0x17 / 0x18 of it) is set on fire
     * (0x00469E40 with 0x00469D50); then the record after the last building gets +0xE4 from table
     * 0x0070CE0C by its +0x12 */
    for (int32_t k = times; k > 0; k--) {
        int32_t n = S32(st, G_BUILDING_HIGHEST);
        int32_t best = 0, bi = 0, i = 1;
        if (n < 1) {
            continue;
        }
        for (; i <= n; i++) {
            uint32_t b = BUILDING_ADDR(i);
            if (U8(st, b + BLD_STATE) != 1 || U8(st, b + BLD_HOUSE_SIZE) == 0 || S16(st, b + BLD_POPULATION) == 0) {
                continue;
            }
            int16_t f = S16(st, b + BLD_WALKER);
            if (f > 0) {
                uint32_t fr = FIGURE_ADDR(f);
                if (U8(st, fr + FIG_STATE) != 0 && (U8(st, fr + FIG_TYPE) == 0x17 || U8(st, fr + FIG_TYPE) == 0x18) &&
                    S16(st, fr + FIG_HOME) == i) {
                    continue;
                }
                S16(st, b + BLD_WALKER) = 0;
            }
            if (S32(st, b + BLD_CRIME_RISK) > best) {
                best = S32(st, b + BLD_CRIME_RISK);
                bi = i;
            }
        }
        if (bi > 0) {
            int32_t r = GAME_CALL0(st, FN_MISSION_GOALS_PENDING);
            GAME_CALL(st, FN_BUILDING_SPAWN_469E40, bi, r);
            uint32_t b = BUILDING_ADDR(i);
            S32(st, b + BLD_CRIME_RISK) = S32(st, G_HOUSE_LEVEL_U40 + 0x58u * (uint32_t) (int32_t) S16(st, b + BLD_SUBTYPE));
        }
    }
}

int32_t building_monument_reachable(game_state *st, int32_t b)
{
    /* @bin 0x004E7C90 a building of types 0x3C..0x40, 0x2F, 0xB7, 0xDE, 0xE5, 0xEA, 0xEB, 0xEC with a
     * road within 6 (0x0047FE60) */
    uint32_t r = BUILDING_ADDR(b);
    int16_t t = S16(st, r + BLD_TYPE);
    if ((t > 0x3B && t < 0x41) || t == 0x2F || t == 0xB7 || t == 0xDE || t == 0xE5 || t == 0xEA || t == 0xEB ||
        t == 0xEC) {
        if (GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, r + BLD_ROAD_ACCESS_X), S16(st, r + BLD_ROAD_ACCESS_Y), 1, 6) != 0) {
            return 1;
        }
    }
    return 0;
}

void plague_tomb_robbers(game_state *st, int32_t arg)
{
    /* @bin 0x004E7AC0 0x0078D798 := arg; one of the buildings of 0x004E7C90 picked by a game RNG step
     * (inlined) gets a figure 0x6D from its road (0x0047FE60, 0x00D3B4E4 / 0x00D3B4E0): +0x5A the
     * building, +0x38 := 9, +0x36 := 0, 0x004B1DF0, +0x44 := 0 */
    S32(st, G_MUMMY_ATTACK_PARAM) = arg;
    if (S32(st, G_BUILDING_HIGHEST) < 1) {
        return;
    }
    int32_t cnt = 0;
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        if ((uint8_t) building_monument_reachable(st, i) != 0) {
            cnt++;
        }
    }
    if (cnt <= 0) {
        return;
    }
    random_next_state(st);
    int32_t want = (int32_t) (U32(st, G_R_1_15) & 0x7FFF) % cnt + 1;
    int32_t seen = 0, i;
    for (i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        if ((uint8_t) building_monument_reachable(st, i) != 0 && ++seen == want) {
            break;
        }
    }
    if (i > S32(st, G_BUILDING_HIGHEST)) {
        return;
    }
    uint32_t r = BUILDING_ADDR(i);
    GAME_CALL(st, FN_BUILDING_ROAD_WITHIN, S16(st, r + BLD_ROAD_ACCESS_X), S16(st, r + BLD_ROAD_ACCESS_Y), 1, 6);
    int32_t f = GAME_CALL(st, FN_FIGURE_CREATE, S32(st, CURRENT_PLAYER_ADDR), 0x6D, S32(st, G_ROAD_ACCESS_X),
                          S32(st, G_ROAD_ACCESS_Y), 0);
    uint32_t fr = FIGURE_ADDR(f);
    S16(st, fr + FIG_HOME) = (int16_t) i;
    U8(st, fr + FIG_ACTION) = 9;
    S16(st, fr + FIG_WAIT_TICKS) = 0;
    GAME_CALL(st, FN_FIGURE_WALKER_START, f);
    S16(st, fr + FIG_ROAM_LENGTH) = 0;
}

int32_t building_destroy_first_of_type(game_state *st, int32_t type)
{
    /* @bin 0x00469F80 the first building in use of this type collapses (+0 := 4, 0x0047C2B0, sound
     * 0x11, routing fills around it); returns its +0xC, or 0 */
    for (int32_t i = 1; i <= S32(st, G_BUILDING_HIGHEST); i++) {
        uint32_t b = BUILDING_ADDR(i);
        if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_TYPE) != type) {
            continue;
        }
        S32(st, G_UNDO_POSSIBLE) = 0;
        int32_t ret = S32(st, b + BLD_GRID_OFFSET);
        U8(st, b + BLD_STATE) = 4;
        GAME_CALL(st, FN_BUILDING_RUBBLE_TILES, i, S16(st, b + BLD_X), S16(st, b + BLD_Y), U8(st, b + BLD_SIZE), U8(st, b + BLD_SIZE));
        GAME_CALL(st, FN_UI_SOUND, 0x11);
        int32_t x = S16(st, b + BLD_X), y = S16(st, b + BLD_Y), s = U8(st, b + BLD_SIZE);
        GAME_CALL(st, FN_ROUTING_FILL_EE0C50, x - s, y - s, x + s, y + s, 1);
        x = S16(st, b + BLD_X);
        y = S16(st, b + BLD_Y);
        s = U8(st, b + BLD_SIZE);
        GAME_CALL(st, FN_ROUTING_FILL_EC7630, x - s, y - s, x + s, y + s);
        return ret;
    }
    return 0;
}

/* the terrain grid 0x00AF598C (u32 by tile) and the image grid 0x00B7482C, the building grid
 * 0x00B4E6FC (i16 by tile) */
#define TERRAIN(t) (G_GRID_TERRAIN + 4u * (uint32_t) (t))
#define IMAGE(t) (G_IMAGE + 4u * (uint32_t) (t))
#define BUILDING_AT(st, t) S16(st, G_BUILDING_GRID + 2u * (uint32_t) (t))

static int near_edge(game_state *st, int32_t x, int32_t y)
{
    return (uint8_t) GAME_CALL(st, FN_MAP_NEAR_EDGE, (int32_t) (U32(st, G_VIEW_DX) + (uint32_t) x),
                               (int32_t) (U32(st, G_VIEW_DY) + (uint32_t) y), 3) != 0;
}

static void tiles_or(game_state *st, int32_t t, uint32_t v)
{
    U32(st, TERRAIN(t)) |= v;
    U32(st, TERRAIN(t + 1)) |= v;
    U32(st, TERRAIN(t + 0xE4)) |= v;
    U32(st, TERRAIN(t + 0xE5)) |= v;
}

static void tiles_and(game_state *st, int32_t t, uint32_t v)
{
    U32(st, TERRAIN(t)) &= v;
    U32(st, TERRAIN(t + 1)) &= v;
    U32(st, TERRAIN(t + 0xE4)) &= v;
    U32(st, TERRAIN(t + 0xE5)) &= v;
}

static int blocked(game_state *st, int32_t t, uint32_t m)
{
    return (U32(st, TERRAIN(t + 1)) & m) || (U32(st, TERRAIN(t + 0xE4)) & m) || (U32(st, TERRAIN(t + 0xE5)) & m);
}

static void collapse(game_state *st, int32_t b)
{
    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b);
    S32(st, G_CONSTRUCTION_COUNT) += 1;
    U8(st, BUILDING_ADDR(b) + BLD_STATE) = 6;
}

void ramp_walk(game_state *st, int32_t x, int32_t y, int32_t mode, int32_t arg4)
{
    /* @bin 0x004ED360 (cdecl) the 2x2 tiles from (x, y) eastwards by 2 (0x00EAAEA0) while free:
     * mode 2 / 4 mark them (+0x10000000, image 0x00F5736C), 3 finds the first without a building
     * (0x00EAAE90, 0x00EAAE8C := 0xDA / 0xDB / 0xDC), 5 clears them (buildings collapse, 0x00485100),
     * 6 finds the building 0x00EAAE88 (0x00EAAE84); 0x00EAAE98 counts the steps (at most 20),
     * 0x00EAAE9C the last tile. Mode 1 (drawing, 0x004F0BA0 with outputs in the caller's stack) is
     * left to the original */
    (void) arg4;
    if (mode == 1) {
        state_unported(st, FN_RAMP_WALK);
        return;
    }
    U8(st, G_UEAAE84) = 0;
    S32(st, G_UEAAEA0) = 1;
    S32(st, G_UEAAE94) = 2;
    uint32_t m14 = 0xEEFFD76Eu, m8 = 0xFBFFFF7Bu;
    S32(st, G_RAMP_LAST_TILE) = 0;
    S32(st, G_UEAAE98) = 0;
    int32_t t = (int32_t) (U32(st, G_MAP_START) + (uint32_t) y * 0xE4u + (uint32_t) x);
    S32(st, G_UEAAE90) = 0;
    S32(st, G_UEAAEA4) = t;
    S32(st, G_UEAAE8C) = 0;
    if (mode == 3 || mode == 4 || mode == 5 || mode == 6) {
        m14 = 0xEEFFD766u;
        m8 = 0xEBFFFF73u;
    }
    if (near_edge(st, x, y) || near_edge(st, x + 1, y + 1) || blocked(st, t, m14)) {
        return;
    }
    switch (mode) {
    case 2: {
        tiles_or(st, t, 0x10000000u);
        uint32_t v = U32(st, G_UF5736C);
        U32(st, IMAGE(t)) = v;
        U32(st, IMAGE(t + 1)) = v;
        U32(st, IMAGE(t + 0xE4)) = v;
        U32(st, IMAGE(t + 0xE5)) = v;
        break;
    }
    case 3:
        if (BUILDING_AT(st, t) == 0) {
            S32(st, G_UEAAE90) = t;
            S32(st, G_UEAAE8C) = 0xDA;
            return;
        }
        break;
    case 4:
        if (BUILDING_AT(st, t) == 0) {
            U32(st, TERRAIN(t)) |= 0x10000000u;
            U32(st, TERRAIN(t + 1)) |= 0x10000000u;
            U32(st, TERRAIN(t + 0xE4)) |= 0x10000000u;
            uint32_t v = U32(st, G_UF5736C);
            U32(st, IMAGE(t)) = v;
            U32(st, IMAGE(t + 1)) = v;
            U32(st, IMAGE(t + 0xE4)) = v;
            U32(st, TERRAIN(t + 0xE5)) |= 0x10000000u;
            U32(st, IMAGE(t + 0xE5)) = v;
        }
        break;
    case 5: {
        int16_t b = BUILDING_AT(st, t);
        if (b != 0) {
            collapse(st, b);
        } else {
            tiles_and(st, t, 0x03052E84u);
        }
        break;
    }
    case 6:
        if (BUILDING_AT(st, t) == S32(st, G_UEAAE88)) {
            U8(st, G_UEAAE84) = 1;
            return;
        }
        break;
    default:
        break;
    }
    /* @bin 0x004ED4D3 the next 2x2 eastwards */
    t += S32(st, G_UEAAEA0) * 2;
    int32_t x2 = x + 2;
    if (near_edge(st, x2, y)) {
        return;
    }
    for (;;) {
        if (near_edge(st, x2 + 1, y + 1)) {
            return;
        }
        if (blocked(st, t, m14)) {
            /* @bin 0x004ED92F the walk ends on the next obstacle when it is a road (bit 4) */
            if (S32(st, G_UEAAE98) == 0 || (m8 & U32(st, TERRAIN(t + 1))) || (U32(st, TERRAIN(t + 0xE4)) & m14) ||
                (m8 & U32(st, TERRAIN(t + 0xE5))) || !(U32(st, TERRAIN(t + 1)) & 4) || !(U32(st, TERRAIN(t + 0xE5)) & 4)) {
                return;
            }
            if (near_edge(st, x2, y) || near_edge(st, x2 + 1, y + 1)) {
                return;
            }
            switch (mode) {
            case 4:
                if (BUILDING_AT(st, t) != 0) {
                    S32(st, G_RAMP_LAST_TILE) = t;
                    return;
                }
                /* fall through */
            case 2: {
                S32(st, G_RAMP_LAST_TILE) = t;
                tiles_or(st, t, 0x10000000u);
                uint32_t v = U32(st, G_UF5736C);
                U32(st, IMAGE(t)) = v;
                U32(st, IMAGE(t + 0xE4)) = v;
                return;
            }
            case 3:
                if (BUILDING_AT(st, t) != 0) {
                    S32(st, G_RAMP_LAST_TILE) = t;
                    return;
                }
                S32(st, G_UEAAE90) = t;
                S32(st, G_UEAAE8C) = 0xDC;
                return;
            case 5: {
                int16_t b = BUILDING_AT(st, t);
                if (b != 0) {
                    GAME_CALL(st, FN_BUILDING_KEEP_REMOVED, b);
                    S32(st, G_RAMP_LAST_TILE) = t;
                    S32(st, G_CONSTRUCTION_COUNT) += 1;
                    U8(st, BUILDING_ADDR(b) + BLD_STATE) = 6;
                    return;
                }
                S32(st, G_RAMP_LAST_TILE) = t;
                tiles_and(st, t, 0x03052E84u);
                return;
            }
            case 6:
                if (BUILDING_AT(st, t) == S32(st, G_UEAAE88)) {
                    U8(st, G_UEAAE84) = 1;
                    return;
                }
                S32(st, G_RAMP_LAST_TILE) = t;
                return;
            default:
                S32(st, G_RAMP_LAST_TILE) = t;
                return;
            }
        }
        switch (mode) {
        case 2:
            tiles_or(st, t, 0x10000000u);
            break;
        case 4:
            if (BUILDING_AT(st, t) == 0) {
                tiles_or(st, t, 0x10000000u);
            }
            break;
        case 3:
            if (BUILDING_AT(st, t) == 0) {
                S32(st, G_UEAAE90) = t;
                S32(st, G_UEAAE8C) = 0xDB;
                return;
            }
            break;
        case 5: {
            int16_t b = BUILDING_AT(st, t);
            if (b != 0) {
                collapse(st, b);
            } else {
                tiles_and(st, t, 0x03052E84u);
            }
            break;
        }
        case 6:
            if (BUILDING_AT(st, t) == S32(st, G_UEAAE88)) {
                U8(st, G_UEAAE84) = 1;
                return;
            }
            break;
        default:
            break;
        }
        /* @bin 0x004ED8BD */
        S32(st, G_UEAAE98) += 1;
        if (S32(st, G_UEAAE98) > 0x14) {
            return;
        }
        if ((int32_t) (U32(st, G_VIEW_DY) + (uint32_t) y) >= S32(st, G_MAP_YE)) {
            return;
        }
        t += S32(st, G_UEAAEA0) * 2;
        x2 += 2;
        if (near_edge(st, x2, y)) {
            return;
        }
    }
}

static int32_t run_mood(game_state *st, const int32_t *a) { city_mood_add(st, a[0]); return 0; }
static int32_t run_before(game_state *st, const int32_t *a) { (void) a; return flood_month_before(st, G_UD05720); }
static int32_t run_quality(game_state *st, const int32_t *a) { flood_quality_add(st, (uint32_t) a[PORT_THIS_ARG], a[0]); return 0; }
static int32_t run_worse(game_state *st, const int32_t *a) { (void) a; flood_worse(st, G_UD05720); return 0; }
static int32_t run_better(game_state *st, const int32_t *a) { (void) a; flood_better(st, G_UD05720); return 0; }
static int32_t run_frogs_end(game_state *st, const int32_t *a) { (void) a; plague_frogs_end(st); return 0; }
static int32_t run_ramp(game_state *st, const int32_t *a) { ramp_walk(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_hail(game_state *st, const int32_t *a) { (void) a; plague_hail(st); return 0; }
static int32_t run_frogs(game_state *st, const int32_t *a) { plague_frogs(st, a[0]); return 0; }
static int32_t run_flies(game_state *st, const int32_t *a) { plague_flies(st, a[0]); return 0; }
static int32_t run_fire(game_state *st, const int32_t *a) { plague_fire(st, a[0]); return 0; }
static int32_t run_reach(game_state *st, const int32_t *a) { return building_monument_reachable(st, a[0]); }
static int32_t run_robbers(game_state *st, const int32_t *a) { plague_tomb_robbers(st, a[0]); return 0; }
static int32_t run_destroy(game_state *st, const int32_t *a) { return building_destroy_first_of_type(st, a[0]); }

const ported_function month2_actions_ports[] = {
    {0x004E85D0u, "city_mood_add", 1, 0, 0, run_mood, 0},
    {0x004BECF0u, "flood_month_before", 0, 32, G_UD05720, run_before, 0},
    {0x004BD5E0u, "flood_quality_add", 1, 0, G_FLOODPLAIN, run_quality, 0},
    {0x004BED00u, "flood_worse", 0, 0, G_UD05720, run_worse, 0},
    {0x004BED40u, "flood_better", 0, 0, G_UD05720, run_better, 0},
    {0x004E6EB0u, "plague_frogs_end", 0, 0, 0, run_frogs_end, 0},
    {0x004E6ED0u, "plague_hail", 0, 0, 0, run_hail, 0},
    {0x004ED360u, "ramp_walk", 4, 0, 0, run_ramp, 0},
    {0x004E70B0u, "plague_frogs", 1, 0, 0, run_frogs, 0},
    {0x004E7200u, "plague_flies", 1, 0, 0, run_flies, 0},
    {0x00469C60u, "plague_fire", 1, 0, 0, run_fire, 0},
    {0x004E7C90u, "building_monument_reachable", 1, 8, 0, run_reach, 0},
    {0x004E7AC0u, "plague_tomb_robbers", 1, 0, 0, run_robbers, 0},
    {0x00469F80u, "building_destroy_first_of_type", 1, 32, 0, run_destroy, 0},
};
const int month2_actions_ports_count = sizeof(month2_actions_ports) / sizeof(month2_actions_ports[0]);
