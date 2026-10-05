/* Imhotep - the floodplain tiles under the Nile flood (package day2): the per-level tile lists of
 * the flood manager (0x00D05720, flood_tick in day_flood.c) and the tile worker 0x004BD950.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day2_flood.h"

#include "city/day_x87.h"
#include "random/random.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define TERRAIN 0x00AF598Cu      /* u32 per tile */
#define BITS 0x00B285CCu         /* u8 per tile */
#define BUILDING_GRID 0x00B4E6FCu /* i16 per tile */
#define FIGURE_GRID 0x00B350DCu  /* i16 per tile: first figure */
#define MAP_START 0x00BCA2D8u
#define MAP 0x00BCA350u
#define PS32(st, a) PTR_AT(st, state_s32, a) /* objects reached through `this` */
#define PU32(st, a) PTR_AT(st, state_u32, a)          /* `this` of the building-area helpers */

static uint32_t terrain_at(int32_t t)
{
    return TERRAIN + 4u * (uint32_t) t;
}

/* the area passes around (x, y) with radius r (cdecl, x0, y0, x1, y1[, 0]) */
static void call4(game_state *st, uint32_t f, int32_t x, int32_t y, int32_t r)
{
    GAME_CALL(st, f, x - r, y - r, x + r, y + r);
}

static void call5(game_state *st, uint32_t f, int32_t x, int32_t y, int32_t r)
{
    GAME_CALL(st, f, x - r, y - r, x + r, y + r, 0);
}

static void figures_on_tile_flooded(game_state *st, int32_t t)
{
    /* @bin 0x004BDC20 every figure on the tile but types 0x0E and 0x1C: 0x004BD250 */
    int32_t f = S16(st, FIGURE_GRID + 2u * (uint32_t) t);
    if (f == 0) {
        return;
    }
    while (S16(st, FIGURE_ADDR(f) + 8) != f) {
        uint8_t type = U8(st, FIGURE_ADDR(f) + 0xA);
        if (type != 0xE && type != 0x1C) {
            GAME_CALL(st, 0x004BD250u, f);
        }
        f = S16(st, FIGURE_ADDR(f) + 8);
        if (f == 0) {
            return;
        }
    }
}

static void figures_on_tile_drained(game_state *st, int32_t t)
{
    /* @bin 0x004BDD89 boats and the like (types 0x14, 0x19, 0x4D, 0x4E, 0x64, 0x65) move one tile
     * toward the middle of the longest run of water neighbours (0x0047DB00, flags 0x00BC97C0) */
    int32_t f = S16(st, FIGURE_GRID + 2u * (uint32_t) t);
    if (S16(st, FIGURE_ADDR(f) + 8) == f) {
        return;
    }
    for (;;) {
        if (f == 0) {
            return;
        }
        uint32_t r = FIGURE_ADDR(f);
        uint8_t type = U8(st, r + 0xA);
        if (type == 0x14 || type == 0x19 || type == 0x4D || type == 0x4E || type == 0x64 || type == 0x65) {
            S32(st, 0x00BC9768u) = t;
            GAME_CALL(st, 0x0047DB00u, 4);
            int32_t best = 0, run = 0, best_start = -1, start = -1;
            for (int32_t k = 0; k < 16; k++) {
                int32_t v = S32(st, 0x00BC97C0u + 4u * (uint32_t) (k & 7));
                if (v != 0) {
                    if (start < 0) {
                        start = k;
                    }
                    run++;
                    if (k != 15) {
                        continue;
                    }
                }
                /* @bin 0x004BDE1F a shorter run is not reset: it goes on after the gap */
                if (run > best) {
                    best_start = start;
                    best = run;
                    start = -1;
                    run = 0;
                }
            }
            int32_t dx = 0, dy = 0, d = 0;
            switch ((best / 2 + best_start) & 7) { /* @bin 0x004BE0A8 */
            case 0: dy = -1; d = -0xE4; break;
            case 1: dy = -1; dx = 1; d = -0xE3; break;
            case 2: dx = 1; d = 1; break;
            case 3: dy = 1; dx = 1; d = 0xE5; break;
            case 4: dy = 1; d = 0xE4; break;
            case 5: dy = 1; dx = -1; d = 0xE3; break;
            case 6: dx = -1; d = -1; break;
            default: dy = -1; dx = -1; d = -0xE5; break;
            }
            GAME_CALL(st, 0x004B5420u, f);
            U16(st, r + 0x4A) = (uint16_t) (U16(st, r + 0x4A) + dx * 32);
            U16(st, r + 0x14) = (uint16_t) (U16(st, r + 0x14) + dx * 2);
            U16(st, r + 0x4C) = (uint16_t) (U16(st, r + 0x4C) + dy * 32);
            U16(st, r + 0x16) = (uint16_t) (U16(st, r + 0x16) + dy * 2);
            U32(st, r + 0x20) = U32(st, r + 0x20) + (uint32_t) (d * 2);
            GAME_CALL(st, 0x004B52F0u, f);
        }
        f = S16(st, r + 8);
        if (S16(st, FIGURE_ADDR(f) + 8) == f) {
            return;
        }
    }
}

static int32_t building_xy_tile(game_state *st, int32_t b)
{
    return S32(st, MAP_START) + S16(st, BUILDING_ADDR(b) + 6) + S16(st, BUILDING_ADDR(b) + 8) * 0xE4;
}

void flood_tile_step(game_state *st, uint32_t self, int32_t flood)
{
    /* @bin 0x004BD950 (thiscall on a tile object: +0 its level record, +8 the tile, +0xC x,
     * +0x10 y, +0x18 a flag) the tile is flooded (flood != 0) or drained */
    int32_t t = (int32_t) P32(st, self + 8);
    if ((uint8_t) flood != 0) {
        uint32_t ter = U32(st, terrain_at(t));
        U8(st, 0x00ADC36Cu + (uint32_t) t) = 0;
        ter = (ter & 0xFFFFEFFFu) | 4;
        U32(st, terrain_at(t)) = ter;
        uint8_t depth = 0;
        if (P8(st, self + 0x18) != 0) {
            /* @bin 0x004BD993 (int) (level +8 * (1/30) * 99), x87 */
            x87_val v = x87_from_int((int32_t) P32(st, P32(st, self) + 8));
            v = x87_mul(v, x87_from_double(0x3FA1111111111111ull));
            v = x87_mul(v, x87_from_double(0x4058C00000000000ull));
            depth = (uint8_t) x87_ftol(v);
        }
        int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) t);
        U8(st, 0x00A5D4CCu + (uint32_t) t) = depth;
        uint32_t kept = ter & 0xEEFED77Fu;
        if (kept & 0x40) {
            ter = (ter & ~0x40u) | 0x8000000u;
            U32(st, terrain_at(t)) = ter;
        }
        int32_t x = (int32_t) P32(st, self + 0xC), y = (int32_t) P32(st, self + 0x10);
        call4(st, 0x00517690u, x, y, 1);
        call5(st, 0x00516F90u, x, y, 1);
        call4(st, 0x005173A0u, x, y, 1);
        call4(st, 0x004777C0u, x, y, 1);
        call5(st, 0x00475CC0u, x, y, 2);
        call4(st, 0x0047B590u, x, y, 2);
        call5(st, 0x0047BAE0u, x, y, 2);
        if ((kept & 8) && b != 0) {
            /* @bin 0x004BDA93 a building on a farmable tile */
            uint32_t r = BUILDING_ADDR(b);
            int16_t type = S16(st, r + 0x10);
            if (type != 0x63 && type != 0x58 && type != 0x59 && type != 0x5D && type != 0x51 && type != 0x5A &&
                type != 0x8A && U8(st, r) != 6) {
                if ((uint8_t) GAME_CALLT(st, MAP, 0x00488030u, b) == 0) {
                    U8(st, r) = 6;
                    U8(st, r + 0xBF) = 1;
                } else if (S32(st, CURRENT_CITY(st) + 0x4998) != 0) {
                    S32(st, CURRENT_CITY(st) + 0x4998) = 2;
                    U8(st, r) = 6;
                } else {
                    /* @bin 0x004BDB4E the farm's 3x3 meadow is cleared while every tile has it */
                    int32_t o = building_xy_tile(st, b);
                    int ok = 1;
                    for (int row = 0; row < 3 && ok; row++) {
                        for (int k = 0; k < 3; k++, o++) {
                            uint32_t v = U32(st, terrain_at(o));
                            if (v & 8) {
                                U8(st, BITS + (uint32_t) o) = 0;
                                U32(st, terrain_at(o)) = v & ~8u;
                            } else {
                                ok = 0;
                            }
                        }
                        o += 0xE1;
                    }
                    U16(st, r + 0x54) = 0;
                    U16(st, r + 0x40) = 0;
                    U8(st, r + 0xA6) = 0;
                    U8(st, r + 0xA7) = 0;
                    call4(st, 0x004777C0u, (int32_t) P32(st, self + 0xC), (int32_t) P32(st, self + 0x10), 2);
                    call5(st, 0x00475CC0u, (int32_t) P32(st, self + 0xC), (int32_t) P32(st, self + 0x10), 2);
                }
            }
        }
        figures_on_tile_flooded(st, t);
        return;
    }
    int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) t);
    uint32_t ter = U32(st, terrain_at(t));
    int32_t x = (int32_t) P32(st, self + 0xC), y = (int32_t) P32(st, self + 0x10);
    if (ter & 4) {
        /* @bin 0x004BDC93 the water leaves the tile */
        ter &= 0xFBFFFFFBu;
        U32(st, terrain_at(t)) = ter;
        if (ter & 0x8000000u) {
            U32(st, terrain_at(t)) = (ter & 0xF7FFFFFFu) | 0x40;
        }
        call4(st, 0x0047BD50u, x, y, 3);
        call4(st, 0x00517690u, x, y, 1);
        call5(st, 0x00516F90u, x, y, 1);
        call4(st, 0x005173A0u, x, y, 1);
        call4(st, 0x004777C0u, x, y, 2);
        call5(st, 0x00475CC0u, x, y, 2);
        call4(st, 0x0047B590u, x, y, 3);
        call5(st, 0x0047BAE0u, x, y, 3);
        figures_on_tile_drained(st, t);
    } else {
        GAME_CALL(st, 0x004777C0u, x, y, x + 1, y + 1);
        call5(st, 0x00475CC0u, x, y, 1);
        call4(st, 0x0047B590u, x, y, 2);
        call5(st, 0x0047BAE0u, x, y, 2);
    }
    if ((uint8_t) GAME_CALLT(st, MAP, 0x00488030u, b) != 0 && (uint8_t) GAME_CALLT(st, MAP, 0x004894D0u, b) == 0) {
        /* @bin 0x004BDFBF a farm's 3x3 meadow comes back */
        int32_t o = building_xy_tile(st, b);
        for (int row = 0; row < 3; row++, o += 0xE4) {
            for (int k = 0; k < 3; k++) {
                U8(st, BITS + (uint32_t) (o + k)) = 2;
                U32(st, terrain_at(o + k)) |= 8;
            }
        }
        GAME_CALLT(st, MAP, 0x00488E60u, b, 0);
    }
}

static uint32_t next_object(game_state *st, uint32_t obj)
{
    /* the list node of an object is at its +4; the node's +0 the next node, whose +0xC its object */
    uint32_t node = P32(st, P32(st, obj + 4));
    return node != 0 ? P32(st, node + 0xC) : 0;
}

void flood_level_step(game_state *st, uint32_t self, int32_t flood, int32_t pct, int32_t all)
{
    /* @bin 0x004BD850 (thiscall on a level record: +0 its tile count, +0x10 the list, +0x18 the
     * cursor) the next tiles of the level: a share by the day's progress pct, or (all != 0) every
     * remaining one */
    int32_t count = PS32(st, self);
    if (count == 0) {
        return;
    }
    if (PS32(st, self + 0x18) == 0) {
        uint32_t list = PU32(st, self + 0x10);
        uint32_t c = list != 0 ? P32(st, list + 0xC) : 0;
        PS32(st, self + 0x18) = (int32_t) c;
        if (c != 0 && pct != 0) {
            int32_t k = (int32_t) ((uint32_t) (pct < 0 ? 0 : pct) * (uint32_t) count) / 100;
            while (k-- != 0) {
                PS32(st, self + 0x18) = (int32_t) next_object(st, PU32(st, self + 0x18));
                if (PS32(st, self + 0x18) == 0) {
                    break;
                }
            }
        }
    }
    uint32_t obj = PU32(st, self + 0x18);
    if (obj == 0) {
        return;
    }
    int32_t n = count;
    if ((uint8_t) all == 0) {
        /* @bin 0x004BD8D9 (int) (count * 100.0 / (25.0 * 100.0)) + 1, at least 1: the quotient is
         * never within an ulp of an integer it is not, so it truncates as count / 25 */
        int32_t q = count / 25 + 1;
        n = q < 1 ? 1 : q;
    }
    while (n-- != 0) {
        if (P8(st, obj + 0x18) != 0) {
            flood_tile_step(st, obj, flood);
        }
        PS32(st, self + 0x18) = (int32_t) next_object(st, PU32(st, self + 0x18));
        obj = PU32(st, self + 0x18);
        if (obj == 0) {
            return;
        }
    }
}

void flood_level_more(game_state *st, uint32_t self)
{
    /* @bin 0x004BE0D0 (thiscall on a level record) up to 5 times: every tile of the level drains */
    if (PS32(st, self + 4) < 5) {
        PS32(st, self + 4) += 1;
        flood_level_step(st, self, 0, 1, 1);
    }
}

void flood_quality_roll(game_state *st, uint32_t self)
{
    /* @bin 0x004BD380 (on 0x00D38360) one step of the game's generator, then the flood quality
     * +0x1C moves by a weighted draw on 1..99: weights 2,3,5,10,15,30,15,10,5,3,2 for +100,
     * +80, +60, +40, +20, 0, -20, -40, -60, -80, -100; kept within 0..100 */
    static const int32_t table[11][2] = {{2, 100}, {3, 80}, {5, 60}, {10, 40}, {15, 20}, {30, 0},
                                         {15, -20}, {10, -40}, {5, -60}, {3, -80}, {2, -100}};
    random_next_state(st);
    int32_t r = S32(st, 0x00D3B370u) % 99 + 1;
    int32_t v = 9999, acc = 0;
    for (uint32_t i = 0; v == 9999 && i < 11; i++) {
        acc += table[i][0];
        if (r < acc) {
            v = table[i][1];
            break;
        }
    }
    int32_t q = PS32(st, self + 0x1C) + v;
    if (q >= 100) {
        q = 100;
    }
    PS32(st, self + 0x1C) = q <= 0 ? 0 : q;
}

void flood_season_start(game_state *st, uint32_t self)
{
    /* @bin 0x004BE490 -> 0x004BD550 (on self + 0x32C40) the settings of the coming flood */
    uint32_t s = self + 0x32C40u;
    PS32(st, s + 0x10) = PS32(st, s + 4);
    PS32(st, s + 0xC) = PS32(st, s);
    PS32(st, s + 0x1C) = PS32(st, s + 8);
    PS32(st, s + 0x20) = GAME_CALLT0(st, s, 0x004BD590u);
    flood_quality_roll(st, s);
}

void flood_season_end(game_state *st, uint32_t self)
{
    /* @bin 0x004BE580 (0x004BD580 on self + 0x32C40, then the boats and the like left on land:
     * 0x004BE4A0 for figures of types 0x14 (but actions 10 and 12), 0x19 (but 12), 0x4D, 0x4E,
     * 0x5C, 0x5D, 0x64, 0x65) */
    uint32_t s = self + 0x32C40u;
    PS32(st, s + 0xC) = PS32(st, s);
    PS32(st, s + 0x10) = PS32(st, s + 4);
    for (int32_t f = 1; f < 2000; f++) {
        uint32_t r = FIGURE_ADDR(f);
        uint8_t action = U8(st, r + 0x38);
        switch (U8(st, r + 0xA)) {
        case 0x14:
            if (action == 10 || action == 12) {
                break;
            }
            GAME_CALLT(st, self, 0x004BE4A0u, f);
            break;
        case 0x19:
            if (action == 12) {
                break;
            }
            GAME_CALLT(st, self, 0x004BE4A0u, f);
            break;
        case 0x4D: case 0x4E: case 0x5C: case 0x5D: case 0x64: case 0x65:
            GAME_CALLT(st, self, 0x004BE4A0u, f);
            break;
        default:
            break;
        }
    }
}

static int32_t run_tile_step(game_state *st, const int32_t *a)
{
    flood_tile_step(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
    return 0;
}

static int32_t run_level_step(game_state *st, const int32_t *a)
{
    flood_level_step(st, (uint32_t) a[PORT_THIS_ARG], a[0], a[1], a[2]);
    return 0;
}

static int32_t run_level_more(game_state *st, const int32_t *a)
{
    flood_level_more(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

static int32_t run_quality_roll(game_state *st, const int32_t *a)
{
    flood_quality_roll(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

static int32_t run_season_reset(game_state *st, const int32_t *a)
{
    /* @bin 0x004BD550 */
    uint32_t s = (uint32_t) a[PORT_THIS_ARG];
    flood_season_start(st, s - 0x32C40u);
    return 0;
}

static int32_t run_season_start(game_state *st, const int32_t *a)
{
    flood_season_start(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

static int32_t run_settings_copy(game_state *st, const int32_t *a)
{
    /* @bin 0x004BD580 */
    uint32_t s = (uint32_t) a[PORT_THIS_ARG];
    PS32(st, s + 0xC) = PS32(st, s);
    PS32(st, s + 0x10) = PS32(st, s + 4);
    return 0;
}

static int32_t run_season_end(game_state *st, const int32_t *a)
{
    flood_season_end(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

const ported_function day2_flood_ports[] = {
    {0x004BD950u, "flood_tile_step", 1, 0, 0, run_tile_step, 0},
    {0x004BD850u, "flood_level_step", 3, 0, 0, run_level_step, 0},
    {0x004BE0D0u, "flood_level_more", 0, 0, 0, run_level_more, 0},
    {0x004BD380u, "flood_quality_roll", 0, 0, 0, run_quality_roll, 0},
    {0x004BD550u, "flood_season_reset", 0, 0, 0, run_season_reset, 0},
    {0x004BE490u, "flood_season_start", 0, 0, 0, run_season_start, 0},
    {0x004BD580u, "flood_settings_copy", 0, 0, 0, run_settings_copy, 0},
    {0x004BE580u, "flood_season_end", 0, 0, 0, run_season_end, 0},
};
const int day2_flood_ports_count = sizeof(day2_flood_ports) / sizeof(day2_flood_ports[0]);
