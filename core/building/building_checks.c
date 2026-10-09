/* Imhotep - building helpers reached from the day's updates and the figure actions (roadmap 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Buildings: 0x108-byte records at 0x007999D8 + 0x108 * id: +0 state (u8), +3 size (u8), +6 / +8
 * x / y (i16), +0x0C (i32), +0x10 type (i16), +0x3A (i16, next part), +0x48 / +0x4A (i16), +0xAA
 * orientation (u8), +0xB4 (i16), +0x16 (i16). The map grids are 228 tiles wide from 0x00BCA2D8:
 * terrain 0x00AF598C (u32), 0x00B285CC (u8), 0x00B67D1C (u8), passability 0x00EE0C50 (i8). */
#include "building/day_access.h"
#include "sim/call.h"
#include "state/ported.h"

#define BREC(id) (0x007999D8u + 0x108u * (uint32_t) (int32_t) (id))
#define FIG(id) (0x00BCD600u + 0x184u * (uint32_t) (int32_t) (id))
#define MAP_START 0x00BCA2D8u
#define MAP_W 0x00BCA2D0u
#define MAP_H 0x00BCA2D4u
#define TERRAIN(i) (0x00AF598Cu + 4u * (uint32_t) (int32_t) (i))
#define ON_TILE(i) (0x00B350DCu + 2u * (uint32_t) (int32_t) (i))
#define OUT_X 0x00D3B4E4u
#define OUT_Y 0x00D3B4E0u

int32_t goods_amount_45aac0(game_state *st, int32_t player, int32_t good)
{
    /* @bin 0x0045AAC0 (player unused) 0x0045AFE0(good, 1000000, 1) */
    (void) player;
    return GAME_CALL(st, 0x0045AFE0u, good, 1000000, 1);
}

void building_collapse_chain(game_state *st, int32_t id)
{
    /* @bin 0x00468B20 from the main part (0x00475B50), each part along +0x3A (while > 0): +0x4A
     * (i16) := 1, +0x48 (i16) := 1000, then 0x0046D100(part, 0) */
    int32_t b = building_main_part(st, id);
    for (;;) {
        S16(st, BREC(b) + 0x4A) = 1;
        S16(st, BREC(b) + 0x48) = 1000;
        GAME_CALL(st, 0x0046D100u, b, 0);
        int16_t next = S16(st, BREC(b) + 0x3A);
        if (next < 1) {
            return;
        }
        b = next;
    }
}

void building_rubble_4c03d0(game_state *st, int32_t id)
{
    /* @bin 0x004C03D0 four rubble squares (0x0047C2B0(id, x, y, size, size)) from the table
     * 0x005DF78C (16 bytes per rotation: four indices k into 0x005DF814: +0 size, +4 dx, +8 dy, 16
     * bytes each); the rotation is the building's +0xAA plus the view's 0x00BCA2E0, less 8 when
     * 8 or more. Orientation 0: (x + dy, y + dx); 1: (x + dx, y + dy); else (x, y) */
    uint32_t b = BREC(id);
    int32_t rot = (int32_t) U8(st, b + 0xAA) + S32(st, 0x00BCA2E0u);
    if (rot >= 8) {
        rot -= 8;
    }
    uint32_t e = 0x005DF78Cu + 16u * (uint32_t) rot;
    for (int n = 4; n != 0; n--, e += 4) {
        uint32_t k = 16u * U32(st, e);
        int32_t x, y;
        switch (U8(st, b + 0xAA)) {
        case 0:
            x = S16(st, b + 6) + S32(st, 0x005DF81Cu + k);
            y = S16(st, b + 8) + S32(st, 0x005DF818u + k);
            break;
        case 1:
            x = S16(st, b + 6) + S32(st, 0x005DF818u + k);
            y = S16(st, b + 8) + S32(st, 0x005DF81Cu + k);
            break;
        default:
            x = S16(st, b + 6);
            y = S16(st, b + 8);
            break;
        }
        int32_t size = S32(st, 0x005DF814u + k);
        GAME_CALL(st, 0x0047C2B0u, id, x, y, size, size);
    }
}

void building_collapse_biggest(game_state *st)
{
    /* @bin 0x00458270 among buildings 1 .. 0x0078D790 at state 1 or 3, the first with the largest
     * +0x16 (i16, > 0); its main part (0x00475B50) collapses: 0x00E950DC := 1, message 0x117 with
     * the building's +0x0C (0x004E0CF0), state := 4, 0x00BC9694 := 0; type 0xCA: rubble 0x004C03D0,
     * else 0x0047C2B0(id, x, y, size, size); types 0x1F, 0x1E, 0x21, 0xD1: 0x0043FD00(id, type);
     * then 0x0048C0E0(x, y, size, 1), 0x004695D0(id, 0), 0x00516F90 / 0x005173A0 on the square
     * (x - size, y - size, x + size, y + size) */
    int32_t last = S32(st, 0x0078D790u);
    int32_t best = 0, found = 0;
    for (int32_t i = 1; i <= last; i++) {
        uint8_t s = U8(st, BREC(i));
        if (s == 1 || s == 3) {
            int32_t v = S16(st, BREC(i) + 0x16);
            if (v > best) {
                best = v;
                found = i;
            }
        }
    }
    if (found == 0) {
        return;
    }
    int32_t id = building_main_part(st, found);
    uint32_t b = BREC(id);
    S32(st, 0x00E950DCu) = 1;
    GAME_CALL(st, 0x004E0CF0u, 0x117, 0, S32(st, b + 0x0C), 0, 0);
    U8(st, b) = 4;
    S32(st, 0x00BC9694u) = 0;
    if (S16(st, b + 0x10) == 0xCA) {
        building_rubble_4c03d0(st, id);
    } else {
        int32_t sz = U8(st, b + 3);
        GAME_CALL(st, 0x0047C2B0u, id, S16(st, b + 6), S16(st, b + 8), sz, sz);
    }
    int16_t t = S16(st, b + 0x10);
    if (t == 0x1F || t == 0x1E || t == 0x21 || t == 0xD1) {
        GAME_CALL(st, 0x0043FD00u, id, t);
    }
    int32_t x = S16(st, b + 6), y = S16(st, b + 8), s = U8(st, b + 3);
    GAME_CALL(st, 0x0048C0E0u, x, y, s, 1);
    GAME_CALL(st, 0x004695D0u, id, 0);
    x = S16(st, b + 6);
    y = S16(st, b + 8);
    s = U8(st, b + 3);
    GAME_CALL(st, 0x00516F90u, x - s, y - s, x + s, y + s, 1);
    x = S16(st, b + 6);
    y = S16(st, b + 8);
    s = U8(st, b + 3);
    GAME_CALL(st, 0x005173A0u, x - s, y - s, x + s, y + s);
}

int32_t building_free_dock_480b80(game_state *st, int32_t figure)
{
    /* @bin 0x00480B80 the first building 1 .. 3999 in use (state 1) of type 0x4C, 0xB5 or 0xB6
     * whose +0xB4 (i16) is 0 or the figure; its tile by orientation +0xAA into 0x00D3B4E4 /
     * 0x00D3B4E0: 0 (x + 1, y - 1), 1 (x + 2, y + 1), 2 (x + 1, y + 2), else (x - 1, y + 1).
     * Returns the building, 0 when none */
    for (int32_t i = 1; i < 4000; i++) {
        uint32_t b = BREC(i);
        if (U8(st, b) != 1) {
            continue;
        }
        int16_t t = S16(st, b + 0x10);
        if (t != 0x4C && t != 0xB5 && t != 0xB6) {
            continue;
        }
        int16_t w = S16(st, b + 0xB4);
        if (w != 0 && w != figure) {
            continue;
        }
        int32_t x = S16(st, b + 6), y = S16(st, b + 8);
        switch (U8(st, b + 0xAA)) {
        case 0: x += 1; y -= 1; break;
        case 1: x += 2; y += 1; break;
        case 2: x += 1; y += 2; break;
        default: x -= 1; y += 1; break;
        }
        S32(st, OUT_X) = x;
        S32(st, OUT_Y) = y;
        return i;
    }
    return 0;
}

int32_t figure_free_water_480e90(game_state *st, int32_t figure)
{
    /* @bin 0x00480E90 the figure's tile already water (terrain bit 4) with the figure first on
     * it: 0. Else, in squares of radius 1 .. 5 around it (clamped to the map), row by row, the
     * first water tile with no figure (or this one) whose neighbours two tiles away (N, E, S, W)
     * are water: 1, its x / y in 0x00D3B4E4 / 0x00D3B4E0 (the loop counters) */
    uint32_t f = FIG(figure);
    int32_t g = S32(st, f + 0x18);
    if ((U8(st, TERRAIN(g)) & 4) != 0 && S16(st, ON_TILE(g)) == figure) {
        return 0;
    }
    int32_t fx = S16(st, f + 0x0C), fy = S16(st, f + 0x0E);
    for (int32_t r = 1; r <= 5; r++) {
        int32_t x0 = fx - r, y0 = fy - r, x1 = fx + r, y1 = fy + r;
        if (x0 < 0) {
            x0 = 0;
        }
        if (y0 < 0) {
            y0 = 0;
        }
        if (x1 >= S32(st, MAP_W)) {
            x1 = S32(st, MAP_W) - 1;
        }
        if (y1 >= S32(st, MAP_H)) {
            y1 = S32(st, MAP_H) - 1;
        }
        S32(st, OUT_Y) = y0;
        int32_t c = S32(st, MAP_START) + y0 * 0xE4 + x0;
        int32_t step = x0 - x1 + 0xE3;
        if (y0 > y1) {
            continue;
        }
        do {
            S32(st, OUT_X) = x0;
            for (int32_t x = x0; x <= x1; x++, c++) {
                if ((U8(st, TERRAIN(c)) & 4) != 0) {
                    int16_t h = S16(st, ON_TILE(c));
                    if ((h == 0 || h == figure) && (U8(st, TERRAIN(c - 0x1C8)) & 4) != 0 &&
                        (U8(st, TERRAIN(c + 2)) & 4) != 0 && (U8(st, TERRAIN(c + 0x1C8)) & 4) != 0 &&
                        (U8(st, TERRAIN(c - 2)) & 4) != 0) {
                        return 1;
                    }
                }
                S32(st, OUT_X) = x + 1;
            }
            c += step;
            S32(st, OUT_Y) = S32(st, OUT_Y) + 1;
        } while (S32(st, OUT_Y) <= y1);
    }
    return 0;
}

/* one neighbour of the search 0x00517C20: 1 when the search stops (a blocked tile with a road,
 * plaza or wall bit 0x4120 is cleared) */
static int bfs_neighbour(game_state *st, int32_t n, int32_t d)
{
    if (S32(st, 0x00EED760u + 4u * (uint32_t) n) != 0) {
        return 0;
    }
    if ((int8_t) U8(st, 0x00EE0C50u + (uint32_t) n) >= 0) {
        S32(st, 0x00EED760u + 4u * (uint32_t) n) = d;
        int32_t tail = S32(st, 0x00EAE008u);
        S32(st, 0x00F203A0u + 4u * (uint32_t) tail) = n;
        tail++;
        S32(st, 0x00EAE008u) = tail;
        if (tail > 0xCB0F) {
            S32(st, 0x00EAE008u) = 0;
        }
        return 0;
    }
    if ((U32(st, TERRAIN(n)) & 0x4120u) == 0) {
        return 0;
    }
    uint8_t m = U8(st, 0x00B67D1Cu + (uint32_t) n);
    uint32_t t = U32(st, TERRAIN(n));
    uint8_t c = U8(st, 0x00B285CCu + (uint32_t) n);
    U8(st, 0x00B67D1Cu + (uint32_t) n) = (uint8_t) (m | 0x40);
    U32(st, TERRAIN(n)) = t & 0x03052E80u;
    U8(st, 0x00B285CCu + (uint32_t) n) = (uint8_t) (c & 0xF0);
    return 1;
}

int32_t route_open_517c20(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00517C20 a breadth-first search from (x1, y1) over the passable tiles (0x00EE0C50 >=
     * 0): distances in 0x00EED760 (cleared first, 0x32C40 bytes), the ring of tiles 0x00F203A0
     * (head 0x00EAE00C, tail 0x00EAE008, 0xCB10 slots), start index 0x00EAE004, the search count
     * 0x00EADFD4 + 1; neighbours N, E, S, W. The first blocked neighbour holding bits 0x4120 is
     * opened (0x00B67D1C |= 0x40, terrain &= 0x03052E80, 0x00B285CC &= 0xF0) and the search ends
     * with 0; reaching (x2, y2) ends it: 1 when that tile was reached */
    GAME_CALL(st, 0x0040F8F0u, 0x00EED760, 0x32C40, 0);
    int32_t start = S32(st, MAP_START) + y1 * 0xE4 + x1;
    S32(st, 0x00EAE004u) = start;
    S32(st, 0x00EADFD4u) = S32(st, 0x00EADFD4u) + 1;
    S32(st, 0x00EED760u + 4u * (uint32_t) start) = 1;
    S32(st, 0x00F203A0u) = S32(st, 0x00EAE004u);
    S32(st, 0x00EAE00Cu) = 0;
    S32(st, 0x00EAE008u) = 1;
    int32_t target = S32(st, MAP_START) + y2 * 0xE4 + x2;
    int32_t head = 0;
    do {
        int32_t cur = S32(st, 0x00F203A0u + 4u * (uint32_t) head);
        if (cur == target) {
            break;
        }
        int32_t d = S32(st, 0x00EED760u + 4u * (uint32_t) cur) + 1;
        if (bfs_neighbour(st, cur - 0xE4, d) || bfs_neighbour(st, cur + 1, d) ||
            bfs_neighbour(st, cur + 0xE4, d) || bfs_neighbour(st, cur - 1, d)) {
            return 0;
        }
        head = S32(st, 0x00EAE00Cu) + 1;
        S32(st, 0x00EAE00Cu) = head;
        if (head > 0xCB0F) {
            head = 0;
            S32(st, 0x00EAE00Cu) = 0;
        }
    } while (head != S32(st, 0x00EAE008u));
    return S32(st, 0x00EED760u + 4u * (uint32_t) target) != 0;
}

uint8_t monument_entry_542f40(game_state *st, int32_t id, uint32_t out_x, uint32_t out_y)
{
    /* @bin 0x00542F40 the entry tile of a building of type 0xD5 by its part's kind (+0x12) class
     * 0x00601DE4 + 0x34 * kind (jump table 0x005430AC): 0 (x + 2, y + 5), 1 (x + 1, y + 2),
     * 2 (x + 1, y + 1), 3 (x - 1, y + 1), 4 (x - 1, y - 1), 5 (x + 1, y - 1), 6 (x - 1, y) for
     * an odd kind else (x + 1, y), 12 (x + 1, y + 1), 7 .. 11 and others (x, y); always 1 */
    uint32_t b = BREC(id);
    int16_t kind = S16(st, b + 0x12);
    uint32_t cls = U32(st, 0x00601DE4u + 0x34u * (uint32_t) (int32_t) kind);
    int32_t x = S16(st, b + 6), y = S16(st, b + 8);
    switch (cls) {
    case 0: x += 2; y += 5; break;
    case 1: x += 1; y += 2; break;
    case 2: x += 1; y += 1; break;
    case 3: x -= 1; y += 1; break;
    case 4: x -= 1; y -= 1; break;
    case 5: x += 1; y -= 1; break;
    case 6: x += (kind & 1) ? -1 : 1; break;
    case 7: case 8: case 9: case 10: case 11: break; /* @bin 0x00543064 jumps past the dec */
    case 12: x += 1; y += 1; break;
    default: break;
    }
    P32(st, out_x) = (uint32_t) x;
    P32(st, out_y) = (uint32_t) y;
    return 1;
}

static int32_t run_45aac0(game_state *st, const int32_t *a)
{
    return goods_amount_45aac0(st, a[0], a[1]);
}

static int32_t run_468b20(game_state *st, const int32_t *a)
{
    building_collapse_chain(st, a[0]);
    return 0;
}

static int32_t run_4c03d0(game_state *st, const int32_t *a)
{
    building_rubble_4c03d0(st, a[0]);
    return 0;
}

static int32_t run_458270(game_state *st, const int32_t *a)
{
    (void) a;
    building_collapse_biggest(st);
    return 0;
}

static int32_t run_480b80(game_state *st, const int32_t *a)
{
    return building_free_dock_480b80(st, a[0]);
}

static int32_t run_480e90(game_state *st, const int32_t *a)
{
    return figure_free_water_480e90(st, a[0]);
}

static int32_t run_517c20(game_state *st, const int32_t *a)
{
    return route_open_517c20(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_542f40(game_state *st, const int32_t *a)
{
    if (!state_ptr(st, (uint32_t) a[1], 4) || !state_ptr(st, (uint32_t) a[2], 4)) {
        state_unported(st, 0x00542F40u); /* outputs in the caller's frame */
        return 0;
    }
    return monument_entry_542f40(st, a[0], (uint32_t) a[1], (uint32_t) a[2]);
}

const ported_function building_check_ports[] = {
    {0x0045AAC0u, "goods_amount_45aac0", 2, 32, 0, run_45aac0, 0},
    {0x00468B20u, "building_collapse_chain", 1, 0, 0, run_468b20, 0},
    {0x004C03D0u, "building_rubble_4c03d0", 1, 0, 0, run_4c03d0, 0},
    {0x00458270u, "building_collapse_biggest", 0, 0, 0, run_458270, 0},
    {0x00480B80u, "building_free_dock_480b80", 1, 32, 0, run_480b80, 0},
    {0x00480E90u, "figure_free_water_480e90", 1, 32, 0, run_480e90, 0},
    {0x00517C20u, "route_open_517c20", 4, 32, 0, run_517c20, 0},
    {0x00542F40u, "monument_entry_542f40", 3, 8, 0, run_542f40, 0},
};
const int building_check_ports_count = sizeof(building_check_ports) / sizeof(building_check_ports[0]);
