/* Imhotep - terrain grid passes reached by the map refresh 0x00486E70 on a new-game start, and the
 * 0x00B67D1C mark rectangles of 0x0046D480 (docs/newgame.md).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Grids are indexed like the original: tile = [0x00BCA2D8] + 228 * y + x; the terrain grid
 * 0x00AF598C (u32), the strength bytes 0x00A509BC, the pointer grid 0x00D05720 (u32, objects
 * whose byte +0x18 is tested). Neighbours: -228 north, +228 south, -1 west, +1 east. */
#include "newgame/start2.h"

#include "city/day_x87.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"

#define GRID(i) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (i))
#define STRENGTH(i) U8(st, G_GRID_MOISTURE + (uint32_t) (i))
#define OBJECT(i) U32(st, G_UD05720 + 4u * (uint32_t) (i))

/* ---- 0x00478420 and the 0x10000 flag passes ---- */

int32_t terrain_0x00477f60(game_state *st)
{
    /* @bin 0x00477F60 xor al, al */
    (void) st;
    return 0;
}

int32_t terrain_0x00477eb0(game_state *st, int32_t i, uint32_t mask, int32_t diag)
{
    /* @bin 0x00477EB0 with `diag` (byte): a diagonal pair NW-SE or NE-SW with mask closes it;
     * otherwise closed when each of the four corners (W|NW|N, E|NE|N, S|SW|W, SE|S|E) has mask */
    if ((uint8_t) diag != 0) {
        if (mask & GRID(i - 229) & GRID(i + 229)) {
            return 0;
        }
        if (mask & GRID(i - 227) & GRID(i + 227)) {
            return 0;
        }
    }
    if ((mask & (GRID(i - 229) | GRID(i - 1) | GRID(i - 228))) != 0 &&
        (mask & (GRID(i - 227) | GRID(i + 1) | GRID(i - 228))) != 0 &&
        (mask & (GRID(i + 227) | GRID(i + 228) | GRID(i - 1))) != 0 &&
        (mask & (GRID(i + 229) | GRID(i + 228) | GRID(i + 1))) != 0) {
        return 0;
    }
    return 1;
}

void terrain_0x00479ac0(game_state *st, int32_t x, int32_t y, int32_t mode)
{
    /* @bin 0x00479AC0 inside the map (0x0046F930 with the offsets 0x00BCA2E8 / 0x00BCA2EC): mode 1
     * sets 0x2000000, the i32 grid 0x00B7482C := (0x00ACF85C & 7) + [0x00F56BA4], 0x00B285CC &= 0xF0,
     * 0x00B67D1C |= 0x40; another mode keeps only 0x2000000 */
    if ((GAME_CALL(st, FN_MAP_TILE_INSIDE, S32(st, G_VIEW_DX) + x, S32(st, G_VIEW_DY) + y) & 0xFF) == 0) {
        return;
    }
    int32_t i = S32(st, G_MAP_START) + 228 * y + x;
    S32(st, G_TILE_CURSOR) = i;
    uint32_t t = GRID(i);
    if (mode != 1) {
        GRID(i) = t & 0x2000000u;
        return;
    }
    int32_t v = (U8(st, G_GRID_RANDOM + (uint32_t) i) & 7) + S32(st, G_IMAGE_DUNES);
    GRID(i) = t | 0x2000000u;
    S32(st, G_GRID_IMAGE + 4u * (uint32_t) i) = v;
    U8(st, G_GRID_BITFIELDS + (uint32_t) i) &= 0xF0;
    U8(st, G_GRID_EDGE + (uint32_t) i) |= 0x40;
}

void terrain_0x00477430(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00477430 */
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    int32_t w = S32(st, G_MAP_WIDTH);
    if (x2 >= w) {
        x2 = w - 1;
    }
    int32_t h = S32(st, G_MAP_HEIGHT);
    if (y2 >= h) {
        y2 = h - 1;
    }
    int32_t skip = w - x2 + x1 - 1;
    int32_t idx = S32(st, G_MAP_START) + 228 * y1 + x1;
    for (int32_t y = y1; y <= y2; y++) {
        if (x1 <= x2) {
            int32_t i = idx;
            idx += x2 - x1 + 1;
            for (int32_t x = x1; x <= x2; x++, i++) {
                uint32_t t = GRID(i);
                if ((t & 0x2000000u) != 0 && (t & 0x600u) == 0) {
                    terrain_0x00479ac0(st, x, y, 1);
                }
            }
        }
        idx += S32(st, G_MAP_STRIDE_EXTRA) + skip;
    }
}

void terrain_0x00477dd0(game_state *st, int32_t x, int32_t y, int32_t r)
{
    /* @bin 0x00477DD0 the rectangle (x, y) +/- r clamped to 0 .. map size (inclusive), then the
     * image passes in the original's order */
    int32_t x1 = x - r < 0 ? 0 : x - r;
    int32_t x2 = S32(st, G_MAP_WIDTH) < x + r ? S32(st, G_MAP_WIDTH) : x + r;
    int32_t y1 = y - r < 0 ? 0 : y - r;
    int32_t y2 = S32(st, G_MAP_HEIGHT) < y + r ? S32(st, G_MAP_HEIGHT) : y + r;
    GAME_CALL(st, FN_WATER_PASS, x1, y1, x2, y2, 1);
    GAME_CALL(st, FN_FLAG_0X40000_RECT, x1, y1, x2, y2);
    GAME_CALL(st, FN_SPECIAL_ROCKS_RECT, x1, y1, x2, y2);
    GAME_CALL0(st, FN_ROCKS_ALL);
    terrain_0x00477430(st, x1, y1, x2, y2);
    GAME_CALL(st, FN_TERRAIN_LAND, x1, y1, x2, y2, 0, 1);
    GAME_CALL(st, FN_FLOODPLAIN_PASS, x1, y1, x2, y2);
    GAME_CALL(st, FN_ROADS_RECT, x1 - 1, y1 - 1, x2 + 1, y2 + 1);
    GAME_CALL(st, FN_CANALS_RECT_REDRAW, x1 - 1, y1 - 1, x2 + 1, y2 + 1);
    GAME_CALL(st, FN_MEADOW_RECT, x1, y1, x2, y2);
}

int32_t terrain_0x00477f70(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00477F70 the 3x3 block around (x, y) (clamped to 0 .. map size): a tile without
     * 0x10004 that 0x00477EB0 finds closed gets 0x10000 (flags & 0xD9CBF680), its images redone
     * (r 2) and the spread continues from it */
    int32_t x1 = x - 1 < 0 ? 0 : x - 1;
    int32_t w = S32(st, G_MAP_WIDTH);
    int32_t x2 = w < x + 1 ? w : x + 1;
    int32_t y1 = y - 1 < 0 ? 0 : y - 1;
    int32_t y2 = S32(st, G_MAP_HEIGHT) < y + 1 ? S32(st, G_MAP_HEIGHT) : y + 1;
    int32_t changed = 0;
    int32_t skip = S32(st, G_MAP_STRIDE_EXTRA) - x2 + w + x1 - 1;
    int32_t idx = S32(st, G_MAP_START) + 228 * y1 + x1;
    for (int32_t yy = y1; yy <= y2; yy++) {
        for (int32_t xx = x1; xx <= x2; xx++, idx++) {
            if ((GRID(idx) & 0x10004u) != 0 || terrain_0x00477eb0(st, idx, 0x10004u, 1) != 0) {
                continue;
            }
            GRID(idx) = (GRID(idx) & 0xD9CBF680u) | 0x10000u;
            changed = 1;
            terrain_0x00477dd0(st, xx, yy, 2);
            terrain_0x00477f70(st, xx, yy);
        }
        idx += skip;
    }
    return changed;
}

int32_t terrain_0x00478090(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00478090 the 3x3 block around (x, y): 0x10000 cleared, images redone (r 1), and the
     * clearing continues from tiles with any of 0x22341A33 */
    int32_t x1 = x - 1 < 0 ? 0 : x - 1;
    int32_t w = S32(st, G_MAP_WIDTH);
    int32_t x2 = w < x + 1 ? w : x + 1;
    int32_t y1 = y - 1 < 0 ? 0 : y - 1;
    int32_t y2 = S32(st, G_MAP_HEIGHT) < y + 1 ? S32(st, G_MAP_HEIGHT) : y + 1;
    int32_t changed = 0;
    int32_t skip = S32(st, G_MAP_STRIDE_EXTRA) - x2 + w + x1 - 1;
    int32_t idx = S32(st, G_MAP_START) + 228 * y1 + x1;
    for (int32_t yy = y1; yy <= y2; yy++) {
        if (x1 <= x2) {
            int32_t i = idx;
            idx += x2 - x1 + 1;
            for (int32_t xx = x1; xx <= x2; xx++, i++) {
                if ((GRID(i) & 0x10000u) == 0) {
                    continue;
                }
                GRID(i) &= 0xFFFEFFFFu;
                changed = 1;
                terrain_0x00477dd0(st, xx, yy, 1);
                if ((GRID(i) & 0x22341A33u) != 0) {
                    terrain_0x00478090(st, xx, yy);
                }
            }
        }
        idx += skip;
    }
    return changed;
}

int32_t terrain_0x004781a0(game_state *st, int32_t tile, uint32_t mask)
{
    /* @bin 0x004781A0 the eight neighbours in the order N, E, S, W, NE, SE, SW, NW; the first
     * with `mask` and the highest 8 - 0x00477690(mask, n) (> 0) wins; 0 when 0x00477EB0 is set */
    static const int32_t offsets[8] = {-228, 1, 228, -1, -227, 229, 227, -229};
    if (terrain_0x00477eb0(st, tile, mask, 1) != 0) {
        return 0;
    }
    int32_t best = 0, result = 0;
    for (int k = 0; k < 8; k++) {
        int32_t n = offsets[k] + tile;
        int32_t c = GAME_CALL(st, FN_TILE_NEIGHBOURS_WITH, (int32_t) mask, n);
        if ((mask & GRID(n)) != 0 && 8 - c > best) {
            best = 8 - c;
            result = offsets[k];
        }
    }
    return result;
}

int32_t terrain_0x00478270(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00478270 a tile without 0x10000: while 0x004781A0 finds a neighbour, that neighbour
     * loses 0x10000 and gets 0x80, recursively, and its images are redone (r 1) */
    int32_t i = S32(st, G_MAP_START) + 228 * y + x;
    if ((GRID(i) & 0x10000u) != 0) {
        return 0;
    }
    int32_t d = terrain_0x004781a0(st, i, 0x10000u);
    if (d == 0) {
        return 0;
    }
    do {
        int32_t n = d + i;
        GRID(n) = (GRID(n) & 0xFFFEFFFFu) | 0x80u;
        int32_t rel = n - S32(st, G_MAP_START);
        int32_t nx = rel % 228, ny = rel / 228; /* idiv / signed multiply-shift */
        terrain_0x00478270(st, nx, ny);
        terrain_0x00477dd0(st, nx, ny, 1);
        d = terrain_0x004781a0(st, i, 0x10000u);
    } while (d != 0);
    return 1;
}

void terrain_0x00478330(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00478330 */
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    int32_t w = S32(st, G_MAP_WIDTH);
    if (x2 >= w) {
        x2 = w - 1;
    }
    int32_t h = S32(st, G_MAP_HEIGHT);
    if (y2 >= h) {
        y2 = h - 1;
    }
    int32_t skip = w - x2 + x1 - 1;
    int32_t idx = S32(st, G_MAP_START) + 228 * y1 + x1;
    for (int32_t y = y1; y <= y2; y++) {
        if (x1 <= x2) {
            int32_t i = idx;
            idx += x2 - x1 + 1;
            for (int32_t x = x1; x <= x2; x++, i++) {
                uint32_t t = GRID(i);
                if (t & 4u) {
                    terrain_0x00477f60(st);
                } else if (t & 0x10000u) {
                    terrain_0x00477f70(st, x, y);
                } else if (t & 0x22341A33u) {
                    terrain_0x00478090(st, x, y);
                } else {
                    terrain_0x00478270(st, x, y);
                }
            }
        }
        idx += S32(st, G_MAP_STRIDE_EXTRA) + skip;
    }
}

void terrain_0x00478420(game_state *st)
{
    /* @bin 0x00478420 */
    terrain_0x00478330(st, 0, 0, S32(st, G_MAP_WIDTH) - 1, S32(st, G_MAP_HEIGHT) - 1);
}

/* ---- 0x00487450: the 0x80 marks and their strength ---- */

int32_t terrain_0x00486d30(game_state *st, int32_t x, int32_t y, uint32_t mask, int32_t maxr)
{
    /* @bin 0x00486D30 rings r = 1 .. maxr - 1 (8r tiles, starting at the NW corner going east,
     * south, west, north); a tile inside 0 .. map size with `mask` and without 0x80000 gives r */
    int32_t result = -1;
    if (maxr <= 1) {
        return -1;
    }
    int32_t ry = y - 1;
    int32_t rbase = S32(st, G_MAP_START) + 228 * y - 228;
    int32_t xy = x - y;
    int32_t n = 8;
    for (int32_t r = 1;;) {
        if (result >= 0) {
            return result;
        }
        int32_t q = n / 4;
        int32_t tx = xy + ry, ty = ry, row = rbase;
        for (int32_t k = 0; k < n; k++) {
            switch ((uint32_t) (k / q)) {
            case 0: tx++; break;
            case 1: row += 228; ty++; break;
            case 2: tx--; break;
            case 3: row -= 228; ty--; break;
            default: break;
            }
            if (tx >= 0 && tx <= S32(st, G_MAP_WIDTH) && ty >= 0 && ty <= S32(st, G_MAP_HEIGHT)) {
                uint32_t t = GRID(row + tx);
                if ((mask & t) != 0 && (t & 0x80000u) == 0) {
                    result = r;
                    break;
                }
            }
        }
        rbase -= 228;
        ry--;
        n += 8;
        r++;
        if (r >= maxr) {
            break;
        }
    }
    return result;
}

int32_t terrain_0x00486ec0(game_state *st, int32_t a, int32_t b)
{
    /* @bin 0x00486EC0 d = max(b - a, 0); a := b if b < d, else max(b - a, 0); b := 12 if below;
     * fild a; fidiv b; fmul [0x0056F570] (96.0); __ftol; dec eax. 53-bit x87 precision
     * (city/day_x87.h). */
    (void) st;
    int32_t diff = (int32_t) ((uint32_t) b - (uint32_t) a);
    int32_t d = diff < 0 ? 0 : diff;
    a = b < d ? b : d;
    if (b < 12) {
        b = 12;
    }
    x87_val q = x87_div(a, b);
    return x87_ftol(x87_mul(q, x87_from_double(0x4058000000000000ull))) - 1; /* @bin 0x0056F570 96.0 */
}

int32_t terrain_0x00487260(game_state *st, int32_t i)
{
    /* @bin 0x00487260 */
    uint32_t p = OBJECT(i - 228);
    if (p != 0 && P8(st, p + 0x18) == 0) {
        uint32_t q = OBJECT(i + 228);
        if (q != 0 && P8(st, q + 0x18) == 0) {
            return 1;
        }
    }
    uint32_t p2 = OBJECT(i - 1);
    if (p2 != 0 && P8(st, p2 + 0x18) == 0) {
        uint32_t q2 = OBJECT(i + 1);
        if (q2 != 0 && P8(st, q2 + 0x18) == 0) {
            return 1;
        }
    }
    return 0;
}

int32_t terrain_0x004872c0(game_state *st, int32_t i, uint32_t mask)
{
    /* @bin 0x004872C0 */
    if ((GRID(i - 228) & mask) != 0 && (GRID(i + 228) & mask) != 0) {
        return 1;
    }
    if ((GRID(i - 1) & mask) != 0 && (GRID(i + 1) & mask) != 0) {
        return 1;
    }
    return 0;
}

int32_t terrain_0x00487300(game_state *st, int32_t i)
{
    /* @bin 0x00487300 100 when the tile is between two 4 tiles or two objects (0x00487260),
     * else its strength byte */
    if ((GRID(i - 228) & 4u) != 0 && (GRID(i + 228) & 4u) != 0) {
        return 100;
    }
    if ((GRID(i - 1) & 4u) != 0 && (GRID(i + 1) & 4u) != 0) {
        return 100;
    }
    if (terrain_0x00487260(st, i) != 0) {
        return 100;
    }
    return STRENGTH(i);
}

/* @bin 0x00486F20 / 0x00487390 a strength of 100 when an object with +0x18 == 0 stands on the tile
 * or the tile has 0x40004; 0 when >= 0x80 */
static int32_t strength_open(game_state *st, int32_t n, int32_t v)
{
    uint32_t p = OBJECT(n);
    if ((p == 0 || P8(st, p + 0x18) != 0) && (GRID(n) & 0x40004u) == 0) {
        return v >= 0x80 ? 0 : v;
    }
    return 100;
}

int32_t terrain_0x00487390(game_state *st, int32_t i, int32_t d)
{
    /* @bin 0x00487390 */
    int32_t n = i + d;
    return strength_open(st, n, terrain_0x00487300(st, n));
}

/* @bin 0x00486F20 a side neighbour: 100 when between two 4 tiles (0x004872C0) or two objects
 * (0x00487260), else its strength byte; then strength_open */
static int32_t side_strength(game_state *st, int32_t n)
{
    int32_t v;
    if (terrain_0x004872c0(st, n, 4) != 0 || terrain_0x00487260(st, n) != 0) {
        v = 100;
    } else {
        v = STRENGTH(n);
    }
    return strength_open(st, n, v);
}

int32_t terrain_0x00486f20(game_state *st, int32_t i)
{
    /* @bin 0x00486F20 a tile between two 4 tiles or two objects gets strength 100; a strength in
     * 41 .. 90 sets 0x00BC97C0..0x00BC97DC (N, NE, E, SE, S, SW, W, NW at 100) and returns
     * 0x00484F40; 100 stays 100; others return 0 */
    if (((U8(st, G_GRID_TERRAIN + 4u * (uint32_t) (i - 228)) & 4) != 0 && (GRID(i + 228) & 4u) != 0) ||
        ((GRID(i - 1) & 4u) != 0 && (GRID(i + 1) & 4u) != 0) || terrain_0x00487260(st, i) != 0) {
        STRENGTH(i) = 100;
        return 0;
    }
    int32_t v = STRENGTH(i);
    if (v > 0x5A || v <= 0x28) {
        if (v == 100) {
            STRENGTH(i) = 100;
        }
        return 0;
    }
    S32(st, G_NEIGHBOUR_FLAGS) = side_strength(st, i - 228) == 100;
    S32(st, G_NEIGHBOUR_FLAGS + 4u) = side_strength(st, i - 227) == 100;
    S32(st, G_NEIGHBOUR_FLAGS + 8u) = side_strength(st, i + 1) == 100;
    S32(st, G_NEIGHBOUR_FLAGS + 0xCu) = side_strength(st, i + 229) == 100;
    S32(st, G_NEIGHBOUR_FLAGS + 0x10u) = strength_open(st, i + 228, terrain_0x00487300(st, i + 228)) == 100;
    S32(st, G_NEIGHBOUR_FLAGS + 0x14u) = strength_open(st, i + 227, terrain_0x00487300(st, i + 227)) == 100;
    S32(st, G_NEIGHBOUR_FLAGS + 0x18u) = strength_open(st, i - 1, terrain_0x00487300(st, i - 1)) == 100;
    S32(st, G_NEIGHBOUR_FLAGS + 0x1Cu) = terrain_0x00487390(st, i, -229) == 100;
    return GAME_CALL0(st, FN_NEIGHBOURS_COUNT);
}

static void mark_clear(game_state *st, int32_t idx)
{
    GRID(idx) &= ~0x80u;
    STRENGTH(idx) = 0;
}

void terrain_0x00487450(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    /* @bin 0x00487450 pass 1: tiles with 0x50004 lose the mark; others take the strength from the
     * nearest 4 tile (0x00486D30 within 0x2E / 0x10 / 8 by [0x00784CB0] = 0 / other / 2) and the
     * nearest 0x50000 tile (within 8), scaled by 0x00486EC0, at least 1. Pass 2: marked tiles
     * accepted by 0x00486F20 lose 0x800 and take the pattern 0x00484BE0(0x005CBFB8, 26, -1, 0)
     * of 0x00BC9748 (14 -> 100, else | 0x80), with 0x00BCA2E0 at 0 during the match.
     * 0x00BC9768 is restored at the end. */
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    int32_t w = S32(st, G_MAP_WIDTH);
    if (x2 >= w) {
        x2 = w - 1;
    }
    if (y2 >= S32(st, G_MAP_HEIGHT)) {
        y2 = S32(st, G_MAP_HEIGHT) - 1;
    }
    uint32_t saved = U32(st, G_TILE_CURSOR);
    int32_t skip = S32(st, G_MAP_STRIDE_EXTRA) - x2 + w + x1 - 1;
    int32_t radius = 0x10;
    uint8_t mode = U8(st, G_U784CB0);
    if (mode == 0) {
        radius = 0x2E;
    } else if (mode == 2) {
        radius = 8;
    }
    int32_t idx = S32(st, G_MAP_START) + 228 * y1 + x1;
    for (int32_t y = y1; y <= y2; y++) {
        if (x1 <= x2) {
            for (int32_t x = x1; x <= x2; x++, idx++) {
                uint32_t t = GRID(idx);
                if (t & 0x50004u) {
                    STRENGTH(idx) = 0;
                    GRID(idx) = t & ~0x80u;
                    continue;
                }
                int32_t a = terrain_0x00486d30(st, x, y, 4, radius);
                int32_t b = terrain_0x00486d30(st, x, y, 0x50000u, 8);
                if (a >= 0) {
                    int32_t lim = radius - 12, div = radius, e = a - lim + 1;
                    if (a < lim) {
                        a = 0;
                    } else if (e >= 0) {
                        a = e;
                        div = 12;
                    }
                    a = terrain_0x00486ec0(st, a - 1 <= 0 ? 0 : a - 1, div);
                } else {
                    if (b < 0) {
                        mark_clear(st, idx);
                        continue;
                    }
                    a = 0;
                }
                int32_t c = 0;
                if (b >= 0) {
                    c = terrain_0x00486ec0(st, b - 1 <= 0 ? 0 : b - 1, 8);
                }
                if (a > c) {
                    c = a;
                }
                if (c < 1) {
                    c = 1;
                }
                GRID(idx) |= 0x80u;
                STRENGTH(idx) = (uint8_t) c;
            }
        }
        idx += skip;
    }
    idx = S32(st, G_MAP_START) + 228 * y1 + x1;
    for (int32_t y = y1; y <= y2; y++) {
        if (x1 <= x2) {
            for (int32_t x = x1; x <= x2; x++, idx++) {
                if ((U8(st, G_GRID_TERRAIN + 4u * (uint32_t) idx) & 0x80) == 0 || terrain_0x00486f20(st, idx) == 0) {
                    continue;
                }
                uint32_t t = GRID(idx);
                uint32_t keep = U32(st, G_VIEW_ORIENTATION);
                GRID(idx) = t & ~0x800u;
                U32(st, G_VIEW_ORIENTATION) = 0;
                GAME_CALL(st, FN_LAND_PATTERN_MATCH, G_PATTERN_DEEP, 0x1A, -1, 0);
                int32_t m = S32(st, G_MATCH_OFFSET);
                STRENGTH(idx) = m == 0xE ? 100 : (uint8_t) (m | 0x80);
                U32(st, G_VIEW_ORIENTATION) = keep;
            }
        }
        idx += skip;
    }
    U32(st, G_TILE_CURSOR) = saved;
}

/* ---- 0x0046D480's mark rectangles ---- */

static void mark_rect(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r, int set)
{
    /* @bin 0x00482A90 / 0x00482B50 the rectangle x - r .. x + size - 1 + r (same for y) clamped to
     * the map; the loop counters stay in 0x00D3B4E0 (row end), 0x00D3B4E4 (column end) and
     * 0x00BC9768 (tile) */
    int32_t x1 = x - r, x2 = x + size + r - 1;
    int32_t y1 = y - r, y2 = y + size + r - 1;
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 >= S32(st, G_MAP_WIDTH)) {
        x2 = S32(st, G_MAP_WIDTH) - 1;
    }
    if (y2 >= S32(st, G_MAP_HEIGHT)) {
        y2 = S32(st, G_MAP_HEIGHT) - 1;
    }
    S32(st, G_ROAD_ACCESS_Y) = y1;
    int32_t idx = S32(st, G_MAP_START) + 228 * y1 + x1;
    S32(st, G_TILE_CURSOR) = idx;
    if (y1 > y2) {
        return;
    }
    int32_t rows = y2 - y1 + 1;
    S32(st, G_ROAD_ACCESS_Y) = y1 + rows;
    for (; rows != 0; rows--) {
        S32(st, G_ROAD_ACCESS_X) = x1;
        if (x1 <= x2) {
            int32_t n = x2 - x1 + 1;
            S32(st, G_ROAD_ACCESS_X) = n + x1;
            for (; n != 0; n--, idx++) {
                if (set) {
                    U8(st, G_GRID_EDGE + (uint32_t) idx) |= 0x80;
                } else {
                    U8(st, G_GRID_EDGE + (uint32_t) idx) &= 0x7F;
                }
            }
        }
        idx += x1 - x2 + 0xE3;
    }
    S32(st, G_TILE_CURSOR) = idx;
}

void terrain_0x00482a90(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r)
{
    mark_rect(st, x, y, size, r, 1); /* @bin 0x00482A90 */
}

void terrain_0x00482b50(game_state *st, int32_t x, int32_t y, int32_t size, int32_t r)
{
    mark_rect(st, x, y, size, r, 0); /* @bin 0x00482B50 */
}

/* ---- constant functions of the pointer table at 0x00570000.. (reached by mission 44's carry-over) ---- */

static int32_t run_545f70(game_state *st, const int32_t *a) { (void) st; (void) a; return 0xA7; }  /* @bin 0x00545F70 mov eax, 0xA7 */
static int32_t run_5567c0(game_state *st, const int32_t *a) { (void) st; (void) a; return 0x1B5; } /* @bin 0x005567C0 mov eax, 0x1B5 */
static int32_t run_559470(game_state *st, const int32_t *a) { (void) st; (void) a; return 0x9E; }  /* @bin 0x00559470 mov eax, 0x9E */

/* ---- table ---- */

static int32_t run_478420(game_state *st, const int32_t *a) { (void) a; terrain_0x00478420(st); return 0; }
static int32_t run_478330(game_state *st, const int32_t *a) { terrain_0x00478330(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_477f60(game_state *st, const int32_t *a) { (void) a; return terrain_0x00477f60(st); }
static int32_t run_477f70(game_state *st, const int32_t *a) { return terrain_0x00477f70(st, a[0], a[1]); }
static int32_t run_478090(game_state *st, const int32_t *a) { return terrain_0x00478090(st, a[0], a[1]); }
static int32_t run_478270(game_state *st, const int32_t *a) { return terrain_0x00478270(st, a[0], a[1]); }
static int32_t run_4781a0(game_state *st, const int32_t *a) { return terrain_0x004781a0(st, a[0], (uint32_t) a[1]); }
static int32_t run_477eb0(game_state *st, const int32_t *a) { return terrain_0x00477eb0(st, a[0], (uint32_t) a[1], a[2]); }
static int32_t run_477dd0(game_state *st, const int32_t *a) { terrain_0x00477dd0(st, a[0], a[1], a[2]); return 0; }
static int32_t run_477430(game_state *st, const int32_t *a) { terrain_0x00477430(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_479ac0(game_state *st, const int32_t *a) { terrain_0x00479ac0(st, a[0], a[1], a[2]); return 0; }
static int32_t run_487450(game_state *st, const int32_t *a) { terrain_0x00487450(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_486d30(game_state *st, const int32_t *a) { return terrain_0x00486d30(st, a[0], a[1], (uint32_t) a[2], a[3]); }
static int32_t run_486ec0(game_state *st, const int32_t *a) { return terrain_0x00486ec0(st, a[0], a[1]); }
static int32_t run_486f20(game_state *st, const int32_t *a) { return terrain_0x00486f20(st, a[0]); }
static int32_t run_487260(game_state *st, const int32_t *a) { return terrain_0x00487260(st, a[0]); }
static int32_t run_4872c0(game_state *st, const int32_t *a) { return terrain_0x004872c0(st, a[0], (uint32_t) a[1]); }
static int32_t run_487300(game_state *st, const int32_t *a) { return terrain_0x00487300(st, a[0]); }
static int32_t run_487390(game_state *st, const int32_t *a) { return terrain_0x00487390(st, a[0], a[1]); }
static int32_t run_482a90(game_state *st, const int32_t *a) { terrain_0x00482a90(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_482b50(game_state *st, const int32_t *a) { terrain_0x00482b50(st, a[0], a[1], a[2], a[3]); return 0; }

const ported_function newgame_start2_ports[] = {
    {0x00478420u, "terrain_0x00478420", 0, 0, 0, run_478420, 0},
    {0x00478330u, "terrain_0x00478330", 4, 0, 0, run_478330, 0},
    {0x00477F60u, "terrain_0x00477f60", 3, 8, 0, run_477f60, 0},
    {0x00477F70u, "terrain_0x00477f70", 2, 8, 0, run_477f70, 0},
    {0x00478090u, "terrain_0x00478090", 2, 8, 0, run_478090, 0},
    {0x00478270u, "terrain_0x00478270", 2, 8, 0, run_478270, 0},
    {0x004781A0u, "terrain_0x004781a0", 2, 32, 0, run_4781a0, 0},
    {0x00477EB0u, "terrain_0x00477eb0", 3, 8, 0, run_477eb0, 0},
    {0x00477DD0u, "terrain_0x00477dd0", 3, 0, 0, run_477dd0, 0},
    {0x00477430u, "terrain_0x00477430", 4, 0, 0, run_477430, 0},
    {0x00479AC0u, "terrain_0x00479ac0", 3, 0, 0, run_479ac0, 0},
    {0x00487450u, "terrain_0x00487450", 4, 0, 0, run_487450, 0},
    {0x00486D30u, "terrain_0x00486d30", 4, 32, 0, run_486d30, 0},
    {0x00486EC0u, "terrain_0x00486ec0", 2, 32, 0, run_486ec0, 0},
    {0x00486F20u, "terrain_0x00486f20", 1, 32, 0, run_486f20, 0},
    {0x00487260u, "terrain_0x00487260", 1, 8, 0, run_487260, 0},
    {0x004872C0u, "terrain_0x004872c0", 2, 8, 0, run_4872c0, 0},
    {0x00487300u, "terrain_0x00487300", 1, 32, 0, run_487300, 0},
    {0x00487390u, "terrain_0x00487390", 2, 32, 0, run_487390, 0},
    {0x00482A90u, "terrain_0x00482a90", 4, 0, 0, run_482a90, 0},
    {0x00482B50u, "terrain_0x00482b50", 4, 0, 0, run_482b50, 0},
    {0x00545F70u, "const_0x00545f70", 0, 32, 0, run_545f70, 0},
    {0x005567C0u, "const_0x005567c0", 0, 32, 0, run_5567c0, 0},
    {0x00559470u, "const_0x00559470", 0, 32, 0, run_559470, 0},
};
const int newgame_start2_ports_count = sizeof(newgame_start2_ports) / sizeof(newgame_start2_ports[0]);
