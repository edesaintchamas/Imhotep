/* Imhotep - the buildings' road access by day (day step 0x15).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include <string.h>

#include "building/day_road.h"

#include "building/day_access.h"
#include "city/day_buildings.h"
#include "city/month_message.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define BUILDING_HIGHEST 0x0078D790u
#define MAP_START 0x00BCA2D8u
#define NETWORK 0x00A76AECu  /* u8 per tile */
#define DISTANCE 0x00EED760u /* i32 per tile: distance from the city entry (0x00517A40) */

int32_t map_tile_487d00(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00487D00 (a third argument is ignored) */
    S32(st, ROAD_ACCESS_Y) = y + 6;
    S32(st, ROAD_ACCESS_X) = x + 9;
    return S32(st, MAP_START) + (y + 6) * GRID_SIZE + x + 9;
}

int32_t monument_entry_tile(game_state *st, int32_t id, int32_t *tile, int32_t *x, int32_t *y)
{
    /* @bin 0x004F30B0 the monument's first part (id - +0x12), its kind +0x54: in mode 5
     * (0x005E4EF4) the part of table 0x005F5498 (0x80 bytes per kind), two tiles below its
     * corner; else the part of table 0x005F3878 (0x3C0 per kind), beside its corner by the
     * direction of table 0x005E4EF8 (4: left, 5: two below, 6: two right, 7: above) */
    int32_t base = id - S16(st, BUILDING_ADDR(id) + 0x12);
    if (S32(st, 0x005E4EF4u) == 5) {
        int32_t n = base + S32(st, 0x005F5498u + 0x80u * U8(st, BUILDING_ADDR(base) + 0x54));
        *x = S16(st, BUILDING_ADDR(n) + 6);
        *y = S16(st, BUILDING_ADDR(n) + 8) + 2;
    } else {
        int32_t n = base + S32(st, 0x005F3878u + 0x3C0u * U8(st, BUILDING_ADDR(base) + 0x54));
        uint32_t b = BUILDING_ADDR(n);
        int32_t dir = S32(st, 0x005E4EF8u + 8u * (0x90u * U8(st, b + 0x54) + (uint32_t) (int32_t) S16(st, b + 0x12)));
        *x = S16(st, b + 6);
        *y = S16(st, b + 8);
        switch (dir) {
        case 4: *x -= 1; break;
        case 5: *y += 2; break;
        case 6: *x += 2; break;
        case 7: *y -= 1; break;
        default: break;
        }
    }
    *tile = *y * GRID_SIZE + *x + S32(st, MAP_START);
    return 1;
}

/* the building takes the network and distance of a road tile, and the access coordinates */
static void take_tile(game_state *st, uint32_t b, int32_t t)
{
    U8(st, b + 0x14) = U8(st, NETWORK + (uint32_t) t);
    S16(st, b + 0x20) = (int16_t) S32(st, DISTANCE + 4u * (uint32_t) t);
    S16(st, b + 0x26) = (int16_t) S32(st, ROAD_ACCESS_X);
    S16(st, b + 0x28) = (int16_t) S32(st, ROAD_ACCESS_Y);
}

/* a monument's own access function (thiscall on its object, writing ROAD_ACCESS_X / Y) */
static void monument_access(game_state *st, uint32_t b, int32_t id, uint32_t obj, uint32_t fn)
{
    S16(st, b + 0x20) = 0;
    if ((int8_t) GAME_CALLT(st, obj, fn, id, (int32_t) ROAD_ACCESS_X, (int32_t) ROAD_ACCESS_Y) == 0) {
        return;
    }
    take_tile(st, b, S32(st, MAP_START) + S32(st, ROAD_ACCESS_Y) * GRID_SIZE + S32(st, ROAD_ACCESS_X));
}

#define ROUTE_QUEUE 0x00F203A0u /* i32[0xCB10] */

int32_t route_distances_to(game_state *st, int32_t sx, int32_t sy, int32_t ex, int32_t ey)
{
    /* @bin 0x00517A40 breadth-first distances (+1) over the routing grid 0x00EE0C50 (tiles >= 0)
     * from (sx, sy) into 0x00EED760, in the queue 0x00F203A0 (head 0x00EAE00C, tail 0x00EAE008,
     * neighbours up, right, down, left), until (ex, ey) is taken out; 0x00EADFD4 counts searches.
     * Returns whether (ex, ey) was reached. */
    for (uint32_t k = 0; k < 0x32C40; k += 4) {
        U32(st, DISTANCE + k) = 0;
    }
    int32_t start = S32(st, MAP_START) + sy * GRID_SIZE + sx;
    S32(st, 0x00EAE004u) = start;
    S32(st, 0x00EADFD4u) += 1;
    S32(st, DISTANCE + 4u * (uint32_t) start) = 1;
    S32(st, ROUTE_QUEUE) = S32(st, 0x00EAE004u);
    int32_t end = S32(st, MAP_START) + ey * GRID_SIZE + ex;
    int32_t head = 0, tail = 1;
    S32(st, 0x00EAE00Cu) = 0;
    S32(st, 0x00EAE008u) = 1;
    static const int32_t around[4] = {-GRID_SIZE, 1, GRID_SIZE, -1};
    do {
        int32_t t = S32(st, ROUTE_QUEUE + 4u * (uint32_t) head);
        if (t == end) {
            break;
        }
        int32_t d = S32(st, DISTANCE + 4u * (uint32_t) t) + 1;
        for (int k = 0; k < 4; k++) {
            int32_t n = t + around[k];
            if (S32(st, DISTANCE + 4u * (uint32_t) n) != 0 || S8(st, 0x00EE0C50u + (uint32_t) n) < 0) {
                continue;
            }
            S32(st, DISTANCE + 4u * (uint32_t) n) = d;
            S32(st, ROUTE_QUEUE + 4u * (uint32_t) tail) = n;
            tail = tail + 1 > 0xCB0F ? 0 : tail + 1;
            S32(st, 0x00EAE008u) = tail;
        }
        head = head + 1 > 0xCB0F ? 0 : head + 1;
        S32(st, 0x00EAE00Cu) = head;
    } while (head != tail);
    return S32(st, DISTANCE + 4u * (uint32_t) end) != 0;
}

int32_t building_entry_corner(game_state *st, int32_t id, uint32_t px, uint32_t py)
{
    /* @bin 0x004E0C50 the building's corner (+6, +8) into *px, *py */
    uint32_t b = BUILDING_ADDR(id);
    P32(st, px) = (uint32_t) (int32_t) S16(st, b + 6);
    P32(st, py) = (uint32_t) (int32_t) S16(st, b + 8);
    return 1;
}

int32_t monument_entry_4e6ae0(game_state *st, int32_t id, uint32_t px, uint32_t py)
{
    /* @bin 0x004E6AE0 (thiscall, `this` unused) (x + 2, y + 6) */
    uint32_t b = BUILDING_ADDR(id);
    P32(st, px) = (uint32_t) (S16(st, b + 6) + 2);
    P32(st, py) = (uint32_t) (S16(st, b + 8) + 6);
    return 1;
}

int32_t monument_entry_40eb90(game_state *st, uint32_t self, int32_t id, uint32_t px, uint32_t py)
{
    /* @bin 0x0040EB90 (thiscall on 0x006FD5D8) before stage 3 (+0x58), the part's corner plus
     * half its size (the object's table +0x54, 13 entries per part); then the first part's
     * corner + (3, 11) */
    uint32_t b = BUILDING_ADDR(id);
    if (S32(st, b + 0x58) < 3) {
        int32_t half = (int32_t) P32(st, self + 0x54 + 4u * (uint32_t) (13 * S16(st, b + 0x12))) / 2;
        P32(st, px) = (uint32_t) (S16(st, b + 6) + half);
        P32(st, py) = (uint32_t) (S16(st, b + 8) + half);
        return 1;
    }
    uint32_t base = BUILDING_ADDR(id - S16(st, b + 0x12));
    P32(st, px) = (uint32_t) (S16(st, base + 6) + 3);
    P32(st, py) = (uint32_t) (S16(st, base + 8) + 0xB);
    return 1;
}

int32_t monument_entry_417490(game_state *st, int32_t id, uint32_t px, uint32_t py)
{
    /* @bin 0x00417490 (thiscall on 0x006FD368, `this` unused) by the view rotation
     * 0x00BCA2E0 / 2, a fixed offset from the first part's corner */
    uint32_t base = BUILDING_ADDR(id - S16(st, BUILDING_ADDR(id) + 0x12));
    static const int8_t off[4][2] = {{8, 15}, {-1, 8}, {6, -1}, {15, 6}};
    uint32_t r = (uint32_t) (S32(st, 0x00BCA2E0u) / 2);
    if (r > 3) {
        return 1;
    }
    P32(st, px) = (uint32_t) (S16(st, base + 6) + off[r][0]);
    P32(st, py) = (uint32_t) (S16(st, base + 8) + off[r][1]);
    return 1;
}

int32_t monument_entry_401af0(game_state *st, int32_t id, uint32_t px, uint32_t py)
{
    /* @bin 0x00401AF0 (thiscall on 0x006DE650, `this` unused) the first part's corner + (6, -8) */
    uint32_t base = BUILDING_ADDR(id - S16(st, BUILDING_ADDR(id) + 0x12));
    P32(st, px) = (uint32_t) (S16(st, base + 6) + 6);
    P32(st, py) = (uint32_t) (S16(st, base + 8) - 8);
    return 1;
}

int32_t building_reached_road_wide(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x00481210 as building_reached_road with the size's offsets also shifted 3 and 6
     * tiles in x (and, for the nearest tile fallback, the original's shift counted twice in the
     * coordinates and the result) */
    int32_t base = S32(st, MAP_START) + y * GRID_SIZE + x;
    uint32_t offsets = 0x005CC9E0u + 0x60u * (uint32_t) size;
    uint32_t coords = 0x005CCE98u + 0xC0u * (uint32_t) size;
    int32_t best_rank = 0xC, best = base, best_shift = 0;
    for (int32_t shift = 0; shift <= 6; shift += 3) {
        for (int32_t i = 0; i < 0x18; i++) {
            int32_t d = S32(st, offsets + 4u * (uint32_t) i);
            if (d == 0) {
                break;
            }
            int32_t t = d + shift + base;
            if (!(U32(st, 0x00AF598Cu + 4u * (uint32_t) t) & 0x40) || S32(st, DISTANCE + 4u * (uint32_t) t) <= 0) {
                continue;
            }
            uint32_t network = U8(st, NETWORK + (uint32_t) t);
            int32_t rank = 0xB;
            for (int32_t k = 0; k < 10; k++) {
                if (U32(st, CURRENT_CITY(st) + 0x46ACu + 8u * (uint32_t) k) == network) {
                    rank = k;
                    break;
                }
            }
            if (rank < best_rank) {
                best_rank = rank;
                best = i;
                best_shift = shift;
            }
        }
    }
    if (best_rank < 0xC) {
        S32(st, ROAD_ACCESS_X) = S32(st, coords + 8u * (uint32_t) best) + best_shift + x;
        S32(st, ROAD_ACCESS_Y) = S32(st, coords + 8u * (uint32_t) best + 4) + y;
        return S32(st, offsets + 4u * (uint32_t) best) + best_shift + base + 1;
    }
    int32_t nearest = 100000, found = -1, fx = 0, fy = 0;
    best_shift = 0;
    for (int32_t shift = 0; shift <= 6; shift += 3) {
        for (int32_t i = 0; i < 0x18; i++) {
            int32_t d = S32(st, offsets + 4u * (uint32_t) i);
            if (d == 0) {
                break;
            }
            int32_t t = d + shift + base;
            int32_t v = S32(st, DISTANCE + 4u * (uint32_t) t);
            if (v > 0 && v < nearest) {
                fx = S32(st, coords + 8u * (uint32_t) i) + shift + x;
                fy = S32(st, coords + 8u * (uint32_t) i + 4) + y;
                nearest = v;
                found = t;
                best_shift = shift;
            }
        }
    }
    if (found < 0) {
        return 0;
    }
    S32(st, ROAD_ACCESS_Y) = fy;
    S32(st, ROAD_ACCESS_X) = fx + best_shift;
    return found + best_shift + 1;
}

void day_road_access_457af0(game_state *st)
{
    /* @bin 0x00457AF0 distances over the roads from the city entry (0x00517A40 from city
     * +0x2778); houses (+5) without a road within 2 tiles are abandoned after 4 days (their
     * people leave: 0x00459020); others store their road's network (+0x14), distance (+0x20) and
     * access tile (+0x26, +0x28), found by type; when the exit (+0x2784) is unreachable, the
     * routing grids are rebuilt up to 15 times (message 0x13B when it becomes reachable) */
    uint32_t c = CURRENT_CITY(st);
    int32_t first = 0; /* the first house cut from the city's roads */
    route_distances_to(st, S16(st, c + 0x2778), S16(st, c + 0x277A), -2, -2);
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        int32_t seen = first;
        if (U8(st, b) != 1) {
            continue;
        }
        if (U8(st, b + 5) != 0) {
            if (!building_road_within(st, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3), 2)) {
                S16(st, b + 0x20) = 0;
                S16(st, b + 0x24) += 1;
                if (S16(st, b + 0x24) > 4) {
                    int16_t people = S16(st, b + 0x1C);
                    if (people != 0) {
                        spawn_figure_3(st, S32(st, CURRENT_PLAYER_ADDR), S16(st, b + 6), S16(st, b + 8), people);
                        S16(st, b + 0x1C) = 0;
                        S16(st, b + 0x24) = 0;
                    }
                    U8(st, b) = 2;
                }
                continue;
            }
            int32_t d = S32(st, DISTANCE + 4u * (uint32_t) S32(st, 0x00BC9768u));
            if (d != 0) {
                S16(st, b + 0x20) = (int16_t) d;
                S16(st, b + 0x24) = 0;
                continue;
            }
            /* @bin 0x00457BC5 a road not connected to the entry */
            S16(st, b + 0x20) = 0;
            int empty_tent = S16(st, b + 0x10) == 0xA && S16(st, b + 0x1C) == 0;
            int16_t days = S16(st, b + 0x24);
            if (days == 0 && (empty_tent || seen == 0)) {
                first = S32(st, b + 0xC);
            }
            days = (int16_t) (days + 1);
            S16(st, b + 0x24) = days;
            if (days > 8) {
                S16(st, b + 0x24) = 0;
                if (empty_tent) {
                    U8(st, b) = 2;
                }
            }
            continue;
        }
        int16_t type = S16(st, b + 0x10);
        int32_t r;
        switch (type) {
        case 0x48: case 0xC1:
            if (S32(st, c + 0x4674) == 0) {
                S32(st, c + 0x4674) = id;
            }
            S16(st, b + 0x20) = 0;
            r = building_reached_road(st, S16(st, b + 6), S16(st, b + 8), 3);
            if (r != 0) {
                take_tile(st, b, r - 1);
            }
            break;
        case 0xD2:
            S16(st, b + 0x20) = 0;
            r = map_tile_487d00(st, S16(st, b + 6), S16(st, b + 8));
            if (r != 0) {
                take_tile(st, b, r - 1);
            }
            break;
        case 0xD5:
            S16(st, b + 0x20) = 0;
            if ((int8_t) GAME_CALL(st, 0x00542F40u, id, (int32_t) ROAD_ACCESS_X, (int32_t) ROAD_ACCESS_Y) != 0) {
                S16(st, b + 0x26) = (int16_t) S32(st, ROAD_ACCESS_X);
                S16(st, b + 0x28) = (int16_t) S32(st, ROAD_ACCESS_Y);
            }
            break;
        case 0xE1:
            S16(st, b + 0x20) = 0;
            monument_entry_40eb90(st, 0x006FD5D8u, id, ROAD_ACCESS_X, ROAD_ACCESS_Y);
            take_tile(st, b, S32(st, MAP_START) + S32(st, ROAD_ACCESS_Y) * GRID_SIZE + S32(st, ROAD_ACCESS_X));
            break;
        case 0xE6:
            S16(st, b + 0x20) = 0;
            monument_entry_401af0(st, id, ROAD_ACCESS_X, ROAD_ACCESS_Y);
            take_tile(st, b, S32(st, MAP_START) + S32(st, ROAD_ACCESS_Y) * GRID_SIZE + S32(st, ROAD_ACCESS_X));
            break;
        case 0xE3:
            S16(st, b + 0x20) = 0;
            monument_entry_417490(st, id, ROAD_ACCESS_X, ROAD_ACCESS_Y);
            take_tile(st, b, S32(st, MAP_START) + S32(st, ROAD_ACCESS_Y) * GRID_SIZE + S32(st, ROAD_ACCESS_X));
            break;
        case 0xE4:
            S16(st, b + 0x20) = 0;
            monument_entry_4e6ae0(st, id, ROAD_ACCESS_X, ROAD_ACCESS_Y);
            take_tile(st, b, S32(st, MAP_START) + S32(st, ROAD_ACCESS_Y) * GRID_SIZE + S32(st, ROAD_ACCESS_X));
            break;
        case 0xDE:
            S16(st, b + 0x20) = 0;
            if (building_entry_corner(st, id, ROAD_ACCESS_X, ROAD_ACCESS_Y) != 0) {
                S16(st, b + 0x26) = (int16_t) S32(st, ROAD_ACCESS_X);
                S16(st, b + 0x28) = (int16_t) S32(st, ROAD_ACCESS_Y);
            }
            break;
        case 0xE5: monument_access(st, b, id, 0x006F9320u, 0x00545A30u); break;
        case 0xEA: monument_access(st, b, id, 0x006F5648u, 0x00558F00u); break;
        case 0xEB: monument_access(st, b, id, 0x006DE9E0u, 0x00556230u); break;
        case 0xEC: monument_access(st, b, id, 0x006E9188u, 0x0054F8A0u); break;
        case 0x41: case 0x42: case 0x43: case 0x44: case 0x45:
        case 0x9C: case 0x9D: case 0x9E: case 0x9F: case 0xA0:
            S16(st, b + 0x20) = 0;
            r = building_reached_road_wide(st, S16(st, b + 6), S16(st, b + 8), 3);
            if (r != 0) {
                take_tile(st, b, r - 1);
            }
            break;
        case 0x49: {
            S16(st, b + 0x20) = 0;
            uint32_t m = BUILDING_ADDR(building_main_part(st, id));
            U8(st, b + 0x14) = U8(st, m + 0x14);
            S16(st, b + 0x20) = S16(st, m + 0x20);
            S16(st, b + 0x26) = S16(st, m + 0x26);
            S16(st, b + 0x28) = S16(st, m + 0x28);
            break;
        }
        case 0xB7: {
            S16(st, b + 0x20) = 0;
            int32_t t, x, y;
            monument_entry_tile(st, id, &t, &x, &y);
            S32(st, ROAD_ACCESS_X) = x;
            S32(st, ROAD_ACCESS_Y) = y;
            take_tile(st, b, t);
            break;
        }
        default:
            S16(st, b + 0x20) = 0;
            r = building_reached_road(st, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3));
            if (r != 0) {
                take_tile(st, b, r - 1);
            }
            break;
        }
    }
    /* @bin 0x00458100 */
    c = CURRENT_CITY(st);
    if (S32(st, DISTANCE + 4u * (uint32_t) S32(st, c + 0x2784)) != 0) {
        if (first != 0) {
            GAME_CALL(st, 0x004E2970u, 0x3F, 0, 0);
            GAME_CALL(st, 0x004E2970u, 0x40, 0, 0);
            GAME_CALL(st, 0x00430CD0u, first);
        }
        return;
    }
    int32_t w = S32(st, 0x00BCA2D0u) - 1, h = S32(st, 0x00BCA2D4u) - 1;
    for (int tries = 0; tries < 0xF; tries++) {
        c = CURRENT_CITY(st);
        GAME_CALL(st, 0x00517C20u, S16(st, c + 0x2778), S16(st, c + 0x277A), -2, -2);
        c = CURRENT_CITY(st);
        GAME_CALL(st, 0x00517C20u, S16(st, c + 0x2780), S16(st, c + 0x2782), -2, -2);
        GAME_CALL0(st, 0x00517370u);
        GAME_CALL0(st, 0x00517660u);
        GAME_CALL0(st, 0x005178E0u);
        GAME_CALL0(st, 0x0047B1A0u);
        w = S32(st, 0x00BCA2D0u) - 1;
        h = S32(st, 0x00BCA2D4u) - 1;
        GAME_CALL(st, 0x0047BAE0u, 0, 0, w, h, 0);
        w = S32(st, 0x00BCA2D0u) - 1;
        h = S32(st, 0x00BCA2D4u) - 1;
        GAME_CALL(st, 0x004767D0u, 0, 0, w, h, 0, 0);
        w = S32(st, 0x00BCA2D0u) - 1;
        h = S32(st, 0x00BCA2D4u) - 1;
        GAME_CALL(st, 0x00477500u, 0, 0, w, h);
        GAME_CALL0(st, 0x00476530u);
        c = CURRENT_CITY(st);
        route_distances_to(st, S16(st, c + 0x2778), S16(st, c + 0x277A), -2, -2);
        c = CURRENT_CITY(st);
        if (S32(st, DISTANCE + 4u * (uint32_t) S32(st, c + 0x2784)) != 0) {
            S32(st, 0x00E950DCu) = 1;
            GAME_CALL(st, 0x004E0CF0u, 0x13B, 0, 0, 0, 0);
            S32(st, 0x00BC9694u) = 0;
            return;
        }
    }
    GAME_CALL0(st, 0x00458270u);
}

/* @bin 0x0046F71D the monument message: 0x00E950DC := 1, then 0x004E0CF0(0x92, -1, 0, block, 0).
 * The 30-byte block is a local of the original (esp + 0xC): +4 is set to -1 at the function's
 * start (0x0046F20E, with +0 and +2), then +0 the body and +2 the title phrase of the monument
 * (0x0046F4B7 ..); its other 24 bytes are never written */
static void monument_message(game_state *st, int16_t body, int16_t title)
{
    uint8_t block[30];
    /* @unverified UNINIT-46F200 bytes +6 .. +0x1D are what the original's stack held (see
     * docs/unverified.md): 0, what the oracle shows there in every probed tick */
    memset(block, 0, sizeof(block));
    const int16_t none = -1; /* @bin 0x0046F20E +4 := -1 (no subtitle) */
    memcpy(block, &body, 2);
    memcpy(block + 2, &title, 2);
    memcpy(block + 4, &none, 2);
    S32(st, 0x00E950DCu) = 1;
    message_post(st, 0x92, -1, 0, block, 0);
}

/* a monument finished (its object's progress function >= 100) and not yet announced */
static int monument_done(game_state *st, int32_t id, uint32_t obj, uint32_t fn, uint32_t flag)
{
    int32_t progress = obj != 0 ? GAME_CALLT(st, obj, fn, id, 0) : GAME_CALL(st, fn, id, 0);
    return progress >= 100 && U8(st, BUILDING_ADDR(id) + flag) == 0;
}

void day_monuments_46f200(game_state *st)
{
    /* @bin 0x0046F200 day step 0x20: the first monument found finished posts message 0x92 and
     * sets its flag; the parts of a monument are skipped by their count */
    for (int32_t id = 1; id < 0xFA0; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1) {
            continue;
        }
        switch (S16(st, b + 0x10)) {
        case 0xB7:
            if ((int8_t) GAME_CALL(st, 0x004F6020u, id) != 0 && U8(st, b + 0x80) == 0) {
                /* @bin 0x0046F49C the pyramid's phrases by its variant 0x005E4EF4 (jump table
                 * 0x0046F79C): 1 0x22B / 0x22A, 2 0x22D / 0x22C, 3 0x229 / 0x228, 4 0x227 /
                 * 0x226, 5 0x22F / 0x22E (body / title); other variants keep -1 */
                static const int16_t phr[5][2] = {{0x22B, 0x22A}, {0x22D, 0x22C}, {0x229, 0x228},
                                                  {0x227, 0x226}, {0x22F, 0x22E}};
                uint32_t v = U32(st, 0x005E4EF4u) - 1u;
                U8(st, b + 0x80) = 1;
                monument_message(st, v <= 4 ? phr[v][0] : -1, v <= 4 ? phr[v][1] : -1);
                return;
            }
            id += S32(st, (S32(st, 0x005E4EF4u) == 5 ? 0x0056F7ECu : 0x0056F7D8u) + 4u * U8(st, b + 0x54)) - 1;
            break;
        case 0xD2: {
            uint32_t c = CURRENT_CITY(st);
            if (S16(st, c + 0x433A) >= 0x10 && S32(st, c + 0x2988) == 0) {
                S32(st, c + 0x2988) = 1;
                monument_message(st, 0x231, 0x230); /* @bin 0x0046F55A */
                return;
            }
            break;
        }
        case 0xCF: case 0xD0:
            if (U8(st, b + 0x55) == 100 && U8(st, b + 0x80) == 0) {
                U8(st, b + 0x80) = 1;
                monument_message(st, 0x233, 0x232); /* @bin 0x0046F579 */
                return;
            }
            break;
        case 0xD5:
            if (monument_done(st, id, 0, 0x00542AD0u, 0x68)) {
                U8(st, b + 0x68) = 1;
                monument_message(st, 0x235, 0x234); /* @bin 0x0046F59A */
                return;
            }
            id += 0x63;
            break;
        case 0xE1:
            if (monument_done(st, id, 0x006FD5D8u, 0x0040D940u, 0x68)) {
                U8(st, b + 0x68) = 1;
                monument_message(st, 0x237, 0x236); /* @bin 0x0046F5BB */
                return;
            }
            id += 0x18;
            break;
        case 0xE6:
            if (monument_done(st, id, 0x006DE650u, 0x00401540u, 0x60)) {
                U8(st, b + 0x60) = 1;
                monument_message(st, 0x239, 0x238); /* @bin 0x0046F5DC */
                return;
            }
            id += 0xF;
            break;
        case 0xE3:
            if (monument_done(st, id, 0x006FD368u, 0x00417050u, 0x68)) {
                U8(st, b + 0x68) = 1;
                monument_message(st, 0x23B, 0x23A); /* @bin 0x0046F5FD */
                return;
            }
            id += 8;
            break;
        case 0xE4:
            if (monument_done(st, id, 0x006DE548u, 0x004E6710u, 0x68)) {
                U8(st, b + 0x68) = 1;
                monument_message(st, 0x23D, 0x23C); /* @bin 0x0046F61E */
                return;
            }
            id += 1;
            break;
        case 0xDE:
            if (monument_done(st, id, 0, 0x004DF040u, 0x74)) {
                U8(st, b + 0x74) = 1;
                monument_message(st, 0x23F, 0x23E); /* @bin 0x0046F63C */
                return;
            }
            id += 0x73;
            break;
        case 0xE5:
            if (GAME_CALLT(st, 0x006F9320u, 0x00545740u, id, 0) >= 100 && S16(st, b + 0x16) >= -1 &&
                U8(st, b + 0x9D) == 0) {
                GAME_CALL0(st, 0x00547A60u);
                GAME_CALL0(st, 0x004DEA50u);
                U8(st, b + 0x9D) = 1;
                monument_message(st, 0x241, 0x240); /* @bin 0x0046F65A */
                return;
            }
            id += 0xA6;
            break;
        case 0xEA: case 0xEB: case 0xEC: {
            int16_t type = S16(st, b + 0x10);
            uint32_t obj = type == 0xEA ? 0x006F5648u : type == 0xEB ? 0x006DE9E0u : 0x006E9188u;
            uint32_t fn = type == 0xEA ? 0x00558BE0u : type == 0xEB ? 0x00555E80u : 0x0054F470u;
            if (monument_done(st, id, obj, fn, 0x9D)) {
                GAME_CALL0(st, 0x00547A60u);
                GAME_CALL0(st, 0x004DEA50u);
                U8(st, b + 0x9D) = 1;
                /* @bin 0x0046F677 EA 0x243 / 0x242, 0x0046F6AC EB 0x245 / 0x244, 0x0046F6E1 EC
                 * 0x247 / 0x246 */
                monument_message(st, type == 0xEA ? 0x243 : type == 0xEB ? 0x245 : 0x247,
                                 type == 0xEA ? 0x242 : type == 0xEB ? 0x244 : 0x246);
                return;
            }
            id += type == 0xEA ? 0x9D : type == 0xEB ? 0x1B4 : 0x200;
            break;
        }
        default:
            break;
        }
    }
}

static int32_t run_tile_487d00(game_state *st, const int32_t *a)
{
    return map_tile_487d00(st, a[0], a[1]);
}

static int32_t run_entry_tile(game_state *st, const int32_t *a)
{
    int32_t t, x, y;
    int32_t r = monument_entry_tile(st, a[0], &t, &x, &y);
    P32(st, (uint32_t) a[2]) = (uint32_t) x;
    P32(st, (uint32_t) a[3]) = (uint32_t) y;
    P32(st, (uint32_t) a[1]) = (uint32_t) t;
    return r;
}

static int32_t run_road_access(game_state *st, const int32_t *a)
{
    (void) a;
    day_road_access_457af0(st);
    return 0;
}

static int32_t run_monuments(game_state *st, const int32_t *a)
{
    (void) a;
    day_monuments_46f200(st);
    return 0;
}

static int32_t run_distances_to(game_state *st, const int32_t *a)
{
    return route_distances_to(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_entry_corner(game_state *st, const int32_t *a)
{
    return building_entry_corner(st, a[0], (uint32_t) a[1], (uint32_t) a[2]);
}

static int32_t run_entry_4e6ae0(game_state *st, const int32_t *a)
{
    return monument_entry_4e6ae0(st, a[0], (uint32_t) a[1], (uint32_t) a[2]);
}

static int32_t run_entry_40eb90(game_state *st, const int32_t *a)
{
    return monument_entry_40eb90(st, 0x006FD5D8u, a[0], (uint32_t) a[1], (uint32_t) a[2]);
}

static int32_t run_entry_417490(game_state *st, const int32_t *a)
{
    return monument_entry_417490(st, a[0], (uint32_t) a[1], (uint32_t) a[2]);
}

static int32_t run_entry_401af0(game_state *st, const int32_t *a)
{
    return monument_entry_401af0(st, a[0], (uint32_t) a[1], (uint32_t) a[2]);
}

static int32_t run_reached_wide(game_state *st, const int32_t *a)
{
    return building_reached_road_wide(st, a[0], a[1], a[2]);
}

const ported_function day_road_ports[] = {
    {0x004E0C50u, "building_entry_corner", 3, 8, 0, run_entry_corner, 0},
    {0x004E6AE0u, "monument_entry_4e6ae0", 3, 8, 0x006DE548u, run_entry_4e6ae0, 0},
    {0x0040EB90u, "monument_entry_40eb90", 3, 8, 0x006FD5D8u, run_entry_40eb90, 0},
    {0x00417490u, "monument_entry_417490", 3, 8, 0x006FD368u, run_entry_417490, 0},
    {0x00401AF0u, "monument_entry_401af0", 3, 8, 0x006DE650u, run_entry_401af0, 0},
    {0x00481210u, "building_reached_road_wide", 3, 32, 0, run_reached_wide, 0},
    {0x00517A40u, "route_distances_to", 4, 32, 0, run_distances_to, 0},
    /* the monument message path is not reproducible (see monument_message): state_unported */
    {0x0046F200u, "day_monuments_46f200", 0, 0, 0, run_monuments, 0},
    {0x00487D00u, "map_tile_487d00", 3, 32, 0, run_tile_487d00, 0},
    {0x004F30B0u, "monument_entry_tile", 4, 8, 0, run_entry_tile, 0},
    {0x00457AF0u, "day_road_access_457af0", 0, 0, 0, run_road_access, 0},
};
const int day_road_ports_count = sizeof(day_road_ports) / sizeof(day_road_ports[0]);
