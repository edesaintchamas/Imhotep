/* Imhotep - the land / grass sub-pass of the terrain images over any rectangle, and its helpers
 * (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004767D0 is the land pass of the terrain rebuild (core/map/terrain.c ports it for the whole
 * map, as 0x004DEA50 calls it); the game also calls it on small rectangles (0x004746D0 around a
 * building just placed, the monument tile redraw 0x004F3F70). This is the port for any
 * rectangle and flags, on the game state, with every global the original writes (the neighbour
 * scratch, the block cursor, the pyramid redraw list). */
#include "map/terrain_land.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/pre_load.h"

#define GRID 228

#define TER(o) U32(st, G_GRID_TERRAIN + 4u * (uint32_t) (o))
#define IMG(o) S32(st, G_GRID_IMAGE + 4u * (uint32_t) (o))
#define EDGE(o) U8(st, G_GRID_EDGE + (uint32_t) (o))
#define BITS(o) U8(st, G_GRID_BITFIELDS + (uint32_t) (o))
#define RND(o) U8(st, G_GRID_RANDOM + (uint32_t) (o))
#define MOIST(o) U8(st, G_GRID_MOISTURE + (uint32_t) (o))
#define BGRID(o) S16(st, G_GRID_BUILDING + 2u * (uint32_t) (o))
#define FGRID(o) S16(st, G_GRID_FIGURE + 2u * (uint32_t) (o))
#define FP_OBJECT(o) U32(st, G_FLOODPLAIN_MANAGER + 4u * (uint32_t) (o))
#define BUILDING(id) BUILDING_ADDR(id)
#define BTYPE(id) S16(st, BUILDING(id) + 0x10)
#define BPART(id) S16(st, BUILDING(id) + BLD_PART)

/* @bin 0x0047DB00 neighbour order of the scratch */
static const int32_t NEIGHBOURS[8] = {-228, -227, 1, 229, 228, 227, -1, -229};

int32_t map_near_edge(game_state *st, int32_t x, int32_t y, int32_t d)
{
    /* @bin 0x0046F9A0 inside (0x0046F930) and within d of the diamond's border (returns al) */
    if (!map_tile_inside(st, x, y)) {
        return 0;
    }
    int32_t cx = S32(st, G_MAP_CENTER_X), cy = S32(st, G_MAP_CENTER_Y);
    if (x <= cx) {
        if (y <= cy) {
            return (x - d) + y < S32(st, G_VIEW_DX) + cy;
        }
        return (x - y) - d < cx - S32(st, G_MAP_YE);
    }
    if (y <= cy) {
        return (x - y) + d > S32(st, G_MAP_XE) - cy;
    }
    return x + y + d > S32(st, G_MAP_XE) + cy + 1;
}

void land_neighbour_flags(game_state *st, uint32_t mask)
{
    /* @bin 0x0047DB00 */
    int32_t c = S32(st, G_TILE_CURSOR);
    for (int i = 0; i < 8; i++) {
        S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) i) = (TER(c + NEIGHBOURS[i]) & mask) != 0;
    }
}

static int32_t scratch_count(game_state *st)
{
    /* @bin 0x00484F40 the non-zero scratch dwords */
    int32_t n = 0;
    for (int i = 0; i < 8; i++) {
        n += S32(st, G_NEIGHBOUR_FLAGS + 4u * (uint32_t) i) != 0;
    }
    return n;
}

static int record_matches(game_state *st, uint32_t r)
{
    /* @bin 0x00484C1C byte 2: any; else (byte != 0) == (scratch != 0) */
    for (uint32_t i = 0; i < 8; i++) {
        uint8_t b = U8(st, r + i);
        if (b != 2 && (b != 0) != (S32(st, G_NEIGHBOUR_FLAGS + 4u * i) != 0)) {
            return 0;
        }
    }
    return 1;
}

int32_t land_pattern_match(game_state *st, uint32_t records, int32_t count, int32_t rnd, int32_t backward)
{
    /* @bin 0x00484BE0 first matching record (from the end when `backward`): offset = byte 8 +
     * orientation / 2, extra = byte 12, variant = rnd % byte 13 (forward: rnd >= 0, backward:
     * rnd != 0) else the record's cycling counter (byte 14); returns the rank + 1, 0 if none */
    int32_t half = S32(st, G_VIEW_ORIENTATION) / 2;
    for (int32_t i = 0; i < count; i++) {
        uint32_t r = backward ? records + 16u * (uint32_t) (count - 1 - i) : records + 16u * (uint32_t) i;
        if (!record_matches(st, r)) {
            continue;
        }
        S32(st, G_MATCH_OFFSET) = U8(st, r + PATTERN_OFFSET + (uint32_t) half);
        S32(st, G_MATCH_EXTRA) = U8(st, r + PATTERN_EXTRA);
        if (backward ? rnd != 0 : rnd >= 0) {
            S32(st, G_MATCH_VARIANT) = rnd % (int32_t) U8(st, r + PATTERN_VARIANTS);
        } else {
            uint8_t c = (uint8_t) (U8(st, r + PATTERN_COUNTER) + 1);
            U8(st, r + PATTERN_COUNTER) = c;
            if (c >= U8(st, r + PATTERN_VARIANTS)) {
                U8(st, r + PATTERN_COUNTER) = 0;
            }
            S32(st, G_MATCH_VARIANT) = U8(st, r + PATTERN_COUNTER);
        }
        return i + 1;
    }
    return 0;
}

int32_t land_block_fits(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t mask, int32_t need,
                        int32_t strict)
{
    /* @bin 0x0047C880 inside the map with a margin of 3, cursor := the first tile; every tile: not
     * near the border (3), no floodplain object of flag +0x18 == 0, all on floodplain (0x10000) or
     * all off it, terrain & (mask | 0x10010000) & (strict ? 0xFEFED77F : 0xFEFFD77F) == 0, no
     * figure, and with need: image == need - 1 */
    if (x < 3 || y < 3 || x + w - 1 >= S32(st, G_MAP_WIDTH) - 3 || y + h - 1 >= S32(st, G_MAP_HEIGHT) - 3) {
        return 0;
    }
    int on = 0, off = 0;
    S32(st, G_TILE_CURSOR) = S32(st, G_MAP_START) + y * GRID + x;
    for (int32_t j = 0; j < h; j++) {
        for (int32_t i = 0; i < w; i++) {
            int32_t o = S32(st, G_FOOTPRINT_OFFSETS + 0x30u * (uint32_t) j + 8u * (uint32_t) i) + S32(st, G_TILE_CURSOR);
            if (map_near_edge(st, o % GRID, o / GRID, 3)) {
                return 0;
            }
            uint32_t v = TER(o) & (mask | 0x10010000u);
            uint32_t obj = FP_OBJECT(o);
            if (obj && P8(st, obj + FLOODOBJ_FLAG) == 0) {
                return 0;
            }
            if (v & 0x10000) {
                if (off) {
                    return 0;
                }
                on = 1;
            } else {
                if (on) {
                    return 0;
                }
                off = 1;
            }
            v &= strict ? 0xFEFED77Fu : 0xFEFFD77Fu;
            if (v != 0 || FGRID(o) != 0) {
                return 0;
            }
            if (need != 0 && IMG(o) != need - 1) {
                return 0;
            }
        }
    }
    return 1;
}

void land_block_place(game_state *st, int16_t building, int32_t x, int32_t y, int32_t w, int32_t h, int32_t image,
                      uint32_t flag)
{
    /* @bin 0x0047C140 out of the map: error flag 0x00BC9734; else cursor := the first tile and
     * every tile (row-major, image + k): bitfields & 0xE0, edge | 0x40, terrain & 0x3052E80 | flag,
     * the building id */
    if (x < 0 || y < 0 || x + w - 1 >= S32(st, G_MAP_WIDTH) || y + h - 1 >= S32(st, G_MAP_HEIGHT)) {
        S32(st, G_OUT_OF_MAP) = 1;
        return;
    }
    int32_t base = S32(st, G_MAP_START) + y * GRID + x, k = 0;
    S32(st, G_TILE_CURSOR) = base;
    for (int32_t j = 0; j < h; j++) {
        for (int32_t i = 0; i < w; i++) {
            int32_t o = S32(st, G_FOOTPRINT_OFFSETS + 0x30u * (uint32_t) j + 8u * (uint32_t) i) + base;
            uint32_t v = TER(o);
            BITS(o) &= 0xE0;
            EDGE(o) |= 0x40;
            TER(o) = (v & 0x3052E80u) | flag;
            BGRID(o) = building;
            IMG(o) = k + image;
            k++;
        }
    }
}

int32_t land_pyramid_tile_state(game_state *st, int32_t o)
{
    /* @bin 0x004F3D70 from the tile's building (type 0xB7): its origin +0x0C gives the corner
     * (bits 1, 2, 4, 8 for offsets 0, 1, 228, 229; other tiles: 2), stage +0x55, part +0x12,
     * corner flags +0x5B; stage 3 looks the part up in the tables of the pyramid kind +0x54
     * (0x005F6D58 when 0x005E4EF4 == 5, else 0x005F6CD0) */
    int16_t b = BGRID(o);
    if (b == 0 || BTYPE(b) != 0xB7) {
        return -1;
    }
    uint32_t p = BUILDING(b);
    int32_t origin = S32(st, p + BLD_GRID_OFFSET);
    uint8_t corner;
    switch (o - origin) {
    case 0: corner = 1; break;
    case 1: corner = 2; break;
    case 0xE4: corner = 4; break;
    case 0xE5: corner = 8; break;
    default: return 2;
    }
    uint32_t stage = U8(st, p + BLD_U55);
    if (stage == 3) {
        uint32_t table = S32(st, G_PYRAMID_KIND) == 5 ? G_PYRAMID_STAGE3_LISTS_KIND5 : G_PYRAMID_STAGE3_LISTS;
        uint32_t entry = table + 8u * U8(st, p + BLD_MONUMENT_KIND);
        int32_t n = S32(st, entry + 4);
        for (int32_t i = 0; i < n; i++) {
            uint32_t list = U32(st, entry);
            if (BPART(b) == S32(st, list + 8u * (uint32_t) i)) {
                return (o == S32(st, list + 8u * (uint32_t) i + 4) + S32(st, p + BLD_GRID_OFFSET)) + 2;
            }
        }
        return 2;
    }
    if (U8(st, p + BLD_U5B) & corner) {
        return (int32_t) stage;
    }
    return stage - 1 != 3 ? (int32_t) (stage - 1) : 2;
}

int32_t land_monument_tile_image(game_state *st, int32_t building, int32_t o, int32_t state, int32_t redo)
{
    /* @bin 0x004F3F70 the states of the 8 neighbours (offsets 0x005F7570, from the view
     * orientation) give a bit mask; image = table[mask & keep] + base (+ random % variants when the
     * mask is full). State 4: with redo, the tile is cleared and redrawn by the tree, land and
     * meadow passes. Returns al: 1 when it set the image, redo for state 4, else 0. */
    (void) building;
    uint32_t table, keep = 0xFF;
    int32_t base, variants = 1;
    switch (state) {
    case 3: table = G_UEAAF60; base = S32(st, G_IMAGE_GROUPS + 8u * 759); keep = 0; break;
    case 4:
        if ((uint8_t) redo) {
            int32_t r = o - S32(st, G_MAP_START);
            IMG(o) = 0;
            int32_t x = r % GRID, y = r / GRID;
            GAME_CALL(st, FN_TREES_RECT, x, y, x, y);
            terrain_land(st, x, y, x, y, 0, 1);
            GAME_CALL(st, FN_MEADOW_RECT, x, y, x, y);
        }
        return (uint8_t) redo;
    case 5: table = G_MONUMENT_EDGE_IMAGES_5; base = S32(st, G_IMAGE_GROUPS + 8u * 753); variants = 8; break;
    case 6: table = G_MONUMENT_EDGE_IMAGES; base = S32(st, G_IMAGE_GROUPS + 8u * 754); keep = 0x55; break;
    case 7: table = G_MONUMENT_EDGE_IMAGES; base = S32(st, G_IMAGE_GROUPS + 8u * 755); keep = 0x55; break;
    case 8: table = G_MONUMENT_EDGE_IMAGES; base = S32(st, G_IMAGE_GROUPS + 8u * 754); keep = 0x55; break;
    case 9: table = G_MONUMENT_EDGE_IMAGES; base = S32(st, G_IMAGE_GROUPS + 8u * 756); keep = 0x55; break;
    case 10: table = G_MONUMENT_EDGE_IMAGES; base = S32(st, G_IMAGE_GROUPS + 8u * 757); keep = 0x55; break;
    default: return 0;
    }
    int32_t orient = S32(st, G_VIEW_ORIENTATION);
    uint32_t bits = 0;
    for (int32_t i = 0; i < 8; i++) {
        uint32_t d = (uint32_t) (i + orient) & 7;
        int32_t s = land_pyramid_tile_state(st, S32(st, G_MONUMENT_NEIGHBOURS + 4u * d) + o);
        if (state == 5 ? (s >= 5 || s == 0) : s >= 0) {
            bits |= 1u << i;
        }
    }
    bits &= keep;
    int32_t img = S32(st, table + 4u * bits) + base;
    IMG(o) = img;
    if (bits == 0xFF && variants > 1) {
        IMG(o) = RND(o) % variants + img;
    }
    return 1;
}

void land_pyramid_register(game_state *st, int32_t id)
{
    /* @bin 0x004F60D0 a pyramid's main building (id - part) joins the list (at most 4) unless
     * already there */
    if (BTYPE(id) != 0xB7) {
        return;
    }
    for (int32_t i = 0; i < S32(st, G_PYRAMID_REDRAW_COUNT); i++) {
        if (S32(st, G_PYRAMID_REDRAW + 4u * (uint32_t) i) == id - BPART(id)) {
            return;
        }
    }
    int32_t n = S32(st, G_PYRAMID_REDRAW_COUNT);
    if (n < 4) {
        S32(st, G_PYRAMID_REDRAW + 4u * (uint32_t) n) = id - BPART(id);
        S32(st, G_PYRAMID_REDRAW_COUNT) = n + 1;
    }
}

void land_pyramids_redraw(game_state *st)
{
    /* @bin 0x004F6130 each listed pyramid of a kind (+0x54) 0x004ECEA0 accepts: 0x004ED360 at its
     * origin + the kind's offset 0x005F5D08, size 4; then the list is empty */
    for (int32_t i = 0; i < S32(st, G_PYRAMID_REDRAW_COUNT); i++) {
        int32_t id = S32(st, G_PYRAMID_REDRAW + 4u * (uint32_t) i);
        uint32_t kind = U8(st, BUILDING(id) + BLD_MONUMENT_KIND);
        if ((uint8_t) GAME_CALL(st, FN_PYRAMID_KIND_OK, (int32_t) kind)) {
            int32_t r = S32(st, G_PYRAMID_RAMP_START + 4u * kind) + S32(st, BUILDING(id) + BLD_GRID_OFFSET) - S32(st, G_MAP_START);
            GAME_CALL(st, FN_RAMP_WALK, r % GRID, r / GRID, 4, 0);
        }
    }
    S32(st, G_PYRAMID_REDRAW_COUNT) = 0;
}

/* ---- 0x004767D0 ---------------------------------------------------------------------------- */

static int plain_around(game_state *st, int32_t o)
{
    /* @bin 0x00476BAB the 8 neighbours have no terrain & 0xEEFFD77F */
    for (int i = 0; i < 8; i++) {
        if (TER(o + NEIGHBOURS[i]) & 0xEEFFD77Fu) {
            return 0;
        }
    }
    return 1;
}

/* Grass level: x87 trunc(v * (12.0 / 96.0)) for v <= 100 equals v >> 3 (0.125 is exact, the
 * product is exact). @bin 0x00476B1C FLD [0x0056F578] = 12.0, FDIV [0x0056F570] = 96.0 */
static int32_t grass_level(int32_t v)
{
    return v >> 3;
}

static void grass_tile(game_state *st, int32_t o)
{
    /* @bin 0x00476AB1..0x00476D92 */
    int32_t saved = S32(st, G_TILE_CURSOR);
    S32(st, G_TILE_CURSOR) = o;
    land_neighbour_flags(st, 0x10000);
    S32(st, G_TILE_CURSOR) = saved;
    int32_t group, idx;
    if (scratch_count(st) != 0) {
        land_pattern_match(st, G_PATTERN_DEEP, 0x1A, -1, 0);
        group = 0x210;
        idx = S32(st, G_MATCH_OFFSET) + 0x30;
    } else {
        int32_t v = MOIST(o), lvl;
        if (v > 100) { /* @bin 0x00476B0F FCOM [0x0056F470] = 100.0 */
            int32_t orient = S32(st, G_VIEW_ORIENTATION);
            lvl = (v & 8) == 0 ? ((v - orient) & 7) : (((v - orient / 2) & 3) + (v & 8));
            lvl = (lvl & 0xFF) | 0x80;
        } else {
            lvl = grass_level(v);
        }
        uint32_t rnd = RND(o);
        if (lvl < 0xB) {
            group = 0x21F;
            idx = lvl + (int32_t) (rnd % 3) * 12;
        } else if (lvl < 0x80) {
            if (!(TER(o) & 0x10000000u) && !plain_around(st, o)) {
                group = 0x225;
            } else {
                int full = 1;
                for (int i = 0; i < 8 && full; i++) {
                    int32_t n = o + NEIGHBOURS[i];
                    int32_t nv = MOIST(n) > 100 ? 100 : MOIST(n);
                    if (grass_level(nv) != 0xB || (TER(n) & 0x10801u) ||
                        (!(TER(n) & 0x10000000u) && !plain_around(st, n))) {
                        full = 0;
                    }
                }
                group = full ? 0x226 : 0x229;
            }
            idx = (int32_t) (rnd % 12);
        } else {
            group = 0x231;
            idx = lvl & 0x7F;
        }
    }
    if (TER(o) & 0x40) {
        /* @bin 0x0051C5F0 */
        idx = idx == 0x34 ? 0x56 : idx == 0x30 ? 0x54 : idx == 0x36 ? 0x57 : idx == 0x32 ? 0x55 : idx;
    }
    IMG(o) = S32(st, G_IMAGE_GROUPS + 8u * (uint32_t) group) + idx;
}

/* a monument tile (terrain 0x10000000): 1 when the pass leaves it to the monument */
static int monument_keeps(game_state *st, int32_t o)
{
    /* @bin 0x004768A2..0x00476A8E */
    int32_t b = BGRID(o);
    int16_t type = BTYPE(b);
    if (type == 0xB7) {
        int32_t s = land_pyramid_tile_state(st, o);
        return s >= 0xB || (s >= 3 && (uint8_t) land_monument_tile_image(st, b, o, s, 0));
    }
    if (type == 0xD5 || type == 0xE3 || type == 0xE5 || type == 0xEA || type == 0xEB || type == 0xEC ||
        type == 0xE4) {
        return S32(st, BUILDING(b) + BLD_MONUMENT_STAGE) != 0 || S32(st, BUILDING(b - BPART(b)) + BLD_MONUMENT_STAGE) != 0;
    }
    if (type == 0xE6) {
        return S32(st, BUILDING(b - BPART(b)) + BLD_MONUMENT_KIND) > 0;
    }
    if (type == 0xE1) {
        int32_t main_id = b - BPART(b), least = 0x15;
        if (main_id >= main_id + 0x19) {
            return 1;
        }
        for (int32_t i = 0; i < 0x19; i++) {
            int32_t w = S32(st, BUILDING(main_id + i) + BLD_MONUMENT_STAGE);
            if (w < least) {
                least = w;
            }
        }
        return least > 0;
    }
    /* a pyramid nearby: tiles 20 back while they are monument tiles without a building */
    int32_t k = o - 0x14, id = 0;
    while (k > S32(st, G_MAP_START)) {
        id = BGRID(k);
        if (id != 0 || !(TER(k) & 0x10000000u)) {
            break;
        }
        k -= 0x14;
    }
    if (id != 0 && BTYPE(id) == 0xB7) {
        land_pyramid_register(st, id);
    }
    return 0;
}

void terrain_land(game_state *st, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t keep, int32_t exact)
{
    /* @bin 0x004767D0 without `exact` the rectangle grows by one tile; clamped to the map. Pass
     * 1: grass images on plain tiles with 0x80, cleared images on plain tiles without (unless
     * `keep`). Without `keep`, pass 2 places single land tiles where random & 0xF0 == 0, pass 3
     * the largest block (4, 3, 2, 1) that fits. Then the pyramid list is redrawn. */
    if (!(uint8_t) exact) {
        x0--, y0--, x1++, y1++;
    }
    x0 = x0 < 0 ? 0 : x0;
    y0 = y0 < 0 ? 0 : y0;
    if (x1 >= S32(st, G_MAP_WIDTH)) {
        x1 = S32(st, G_MAP_WIDTH) - 1;
    }
    if (y1 >= S32(st, G_MAP_HEIGHT)) {
        y1 = S32(st, G_MAP_HEIGHT) - 1;
    }
    int32_t step = S32(st, G_MAP_WIDTH) - x1 - 1 + x0; /* + 0x00BCA2DC, re-read per row */
    int32_t o = y0 * GRID + S32(st, G_MAP_START) + x0;
    for (int32_t y = y0; y <= y1; y++) {
        for (int32_t x = x0; x <= x1; x++, o++) {
            uint32_t v = TER(o);
            if (v & 0x10000000u) {
                v &= 0xEFFFFFF7u;
                if (monument_keeps(st, o)) {
                    continue;
                }
            }
            uint32_t obj = FP_OBJECT(o);
            uint32_t busy = (obj && P8(st, obj + FLOODOBJ_FLAG) == 0) ? (v & 8) : (v & 0xEEFFD77Fu);
            if (busy) {
                continue;
            }
            if (!(v & 0x80)) {
                if (!(uint8_t) keep) {
                    IMG(o) = 0;
                    BITS(o) &= 0xF0;
                    EDGE(o) |= 0x40;
                }
            } else if (!(v & 0x22310803u)) {
                grass_tile(st, o);
            }
        }
        o += S32(st, G_MAP_STRIDE_EXTRA) + step;
    }
    if (!(uint8_t) keep) {
        o = y0 * GRID + S32(st, G_MAP_START) + x0;
        for (int32_t y = y0; y <= y1; y++) {
            for (int32_t x = x0; x <= x1; x++, o++) {
                uint32_t v = TER(o), flag = 0;
                int16_t b = 0;
                if (v & 0x10000000u) {
                    flag = 0x10000008u;
                    v &= 0xEFFFFFF7u;
                    b = BGRID(o);
                }
                if ((v & 0xEEFED77Fu) || IMG(o) != 0 || (RND(o) & 0xF0)) {
                    continue;
                }
                int32_t g = (BITS(o) & 0x20) ? S32(st, G_IMAGE_GROUPS + 8u * 0x1F8u) : S32(st, G_IMAGE_GROUPS + 8u * 0x1FBu);
                land_block_place(st, b, x, y, 1, 1, (RND(o) & 7) + g, flag);
            }
            o += S32(st, G_MAP_STRIDE_EXTRA) + step;
        }
        o = y0 * GRID + S32(st, G_MAP_START) + x0;
        for (int32_t y = y0; y <= y1; y++) {
            for (int32_t x = x0; x <= x1; x++, o++) {
                uint32_t v = TER(o), flag = 0;
                int16_t b = 0;
                if (v & 0x10000000u) {
                    flag = 0x10000008u;
                    v &= 0xEFFFFFF7u;
                    b = BGRID(o);
                }
                if ((v & 0xEEFED77Fu) || IMG(o) != 0) {
                    continue;
                }
                uint32_t group = (BITS(o) & 0x20) ? 0x1F8u : 0x1FBu; /* @bin 0x00476FA9 */
                int32_t n, image;
                if (land_block_fits(st, x, y, 4, 4, 0xFFFF, 1, 1)) {
                    n = 4;
                    image = S32(st, G_IMAGE_GROUPS + 8u * group) + 0x2A;
                } else if (land_block_fits(st, x, y, 3, 3, 0xFFFF, 1, 1)) {
                    n = 3;
                    image = (RND(o) & 1) * 9 + S32(st, G_IMAGE_GROUPS + 8u * group) + 0x18;
                } else if (land_block_fits(st, x, y, 2, 2, 0xFFFF, 1, 1)) {
                    n = 2;
                    image = S32(st, G_IMAGE_GROUPS + 8u * group) + (RND(o) & 3) * 4 + 8;
                } else {
                    n = 1;
                    image = (RND(o) & 7) + S32(st, G_IMAGE_GROUPS + 8u * group);
                }
                land_block_place(st, b, x, y, n, n, image, flag);
            }
            o += S32(st, G_MAP_STRIDE_EXTRA) + step;
        }
    }
    land_pyramids_redraw(st);
}

void terrain_land_around(game_state *st, int32_t x, int32_t y, int32_t size)
{
    /* @bin 0x004746D0 */
    terrain_land(st, x - 5, y - 5, x + 5 + size, y + 5 + size, 1, 0);
}

/* ---- registration -------------------------------------------------------------------------- */

static int32_t run_near_edge(game_state *st, const int32_t *a)
{
    return map_near_edge(st, a[0], a[1], a[2]);
}

static int32_t run_pattern_match(game_state *st, const int32_t *a)
{
    if (!state_ptr(st, (uint32_t) a[0], 16u * (uint32_t) (a[1] > 0 ? a[1] : 0))) {
        state_unported(st, FN_LAND_PATTERN_MATCH);
        return 0;
    }
    return land_pattern_match(st, (uint32_t) a[0], a[1], a[2], (uint8_t) a[3]);
}

static int32_t run_block_fits(game_state *st, const int32_t *a)
{
    return land_block_fits(st, a[0], a[1], a[2], a[3], (uint32_t) a[4], a[5], (uint8_t) a[6]);
}

static int32_t run_block_place(game_state *st, const int32_t *a)
{
    land_block_place(st, (int16_t) a[0], a[1], a[2], a[3], a[4], a[5], (uint32_t) a[6]);
    return 0;
}

static int32_t run_pyramid_tile_state(game_state *st, const int32_t *a)
{
    return land_pyramid_tile_state(st, a[0]);
}

static int32_t run_monument_tile_image(game_state *st, const int32_t *a)
{
    return land_monument_tile_image(st, a[0], a[1], a[2], a[3]);
}

static int32_t run_pyramid_register(game_state *st, const int32_t *a)
{
    land_pyramid_register(st, a[0]);
    return 0;
}

static int32_t run_pyramids_redraw(game_state *st, const int32_t *a)
{
    (void) a;
    land_pyramids_redraw(st);
    return 0;
}

static int32_t run_pyramid_kind_ok(game_state *st, const int32_t *a)
{
    /* @bin 0x004ECEA0 (kind) 0 when 0x005E4EF4 == 5, else table 0x005F5D08[kind] != 0 (al) */
    if (S32(st, G_PYRAMID_KIND) == 5) {
        return 0;
    }
    return S32(st, G_PYRAMID_RAMP_START + 4u * (uint32_t) a[0]) != 0;
}

static int32_t run_terrain_land(game_state *st, const int32_t *a)
{
    terrain_land(st, a[0], a[1], a[2], a[3], a[4], a[5]);
    return 0;
}

static int32_t run_terrain_land_around(game_state *st, const int32_t *a)
{
    terrain_land_around(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_grass_index(game_state *st, const int32_t *a)
{
    /* @bin 0x0051C5F0 (tile, index) the index of a grass image on a tile with terrain 0x40 */
    (void) st;
    int32_t i = a[1];
    return i == 0x34 ? 0x56 : i == 0x30 ? 0x54 : i == 0x36 ? 0x57 : i == 0x32 ? 0x55 : i;
}

const ported_function bigb2_land_ports[] = {
    {0x0046F9A0u, "map_near_edge", 3, 8, 0, run_near_edge, 0},
    {0x00484BE0u, "land_pattern_match", 4, 32, 0, run_pattern_match, 0},
    {0x0047C880u, "land_block_fits", 7, 32, 0, run_block_fits, 0},
    {0x0047C140u, "land_block_place", 7, 0, 0, run_block_place, 0},
    {0x004F3D70u, "pyramid_tile_state", 1, 32, 0, run_pyramid_tile_state, 0},
    {0x004F3F70u, "monument_tile_image", 4, 8, 0, run_monument_tile_image, 0},
    {0x004F60D0u, "pyramid_register", 1, 0, 0, run_pyramid_register, 0},
    {0x004F6130u, "pyramids_redraw", 0, 0, 0, run_pyramids_redraw, 0},
    {0x004ECEA0u, "pyramid_kind_ok", 1, 8, 0, run_pyramid_kind_ok, 0},
    {0x004767D0u, "terrain_land", 6, 0, 0, run_terrain_land, 0},
    {0x004746D0u, "terrain_land_around", 3, 0, 0, run_terrain_land_around, 0},
    {0x0051C5F0u, "grass_index", 2, 32, 0, run_grass_index, 0},
};
const int bigb2_land_ports_count = sizeof(bigb2_land_ports) / sizeof(bigb2_land_ports[0]);
