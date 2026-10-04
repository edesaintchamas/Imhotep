/* Imhotep - terrain image rebuild at scenario start.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Reimplements what Pharaoh.exe 2.1.0.0 does to the image grid when a scenario starts
 * (0x004D52A0): zero it, rebuild every tile image from the terrain flags and the per-tile grids
 * (0x004DEA50), build the floodplain (0x004BE160), rebuild again. Rules: docs/formats/map.md §6.
 *
 * Every rule carries @bin (address read in the reference binary). @diff terrain_check marks
 * rules whose output is also confirmed tile for tile against the image grids the game stored in
 * the 29 comparable scenario maps (tests/differential/terrain_check.c).
 *
 * The original computes three values with x87 doubles (grass level, floodplain fertility, meadow
 * levels). They are replaced by integer formulas proven equal to the double computation for every
 * input 0..255 at 53-bit precision, and confirmed by the differential test. Precision: the CRT
 * startup sets _controlfp(_PC_53, _MCW_PC) (@bin 0x005640EC, from 0x00561896); no other code
 * changes the precision bits (the other FLDCW sites are __ftol 0x0056133C, which switches the
 * rounding mode only, and CRT math error handling), and the game uses DirectDraw, not Direct3D. */
#include "map/terrain.h"

#include <string.h>

#include "map/floodplain.h"
#include "map/terrain_tables.h"
#include "state/scenario_start.h"

#define EDGE_DRAW_TILE 0x40 /* @bin 0x00430EF0 city draw routines test edge & 0x40 */

/* Neighbour order of the 8-flag scratch at 0x00BC97C0. @bin 0x0047DB00 */
static const int NEIGHBOURS[8] = {-228, -227, 1, 229, 228, 227, -1, -229};

/* Global image groups, read as addresses into the runtime group table at 0x00F55BB0:
 * group = (address - 0x00F55BB4) / 8. @diff terrain_check */
enum {
    G_WALLS = 407,           /* @bin 0x0047B1A0 DAT_00f5686c */
    G_PLAZA = 437,           /* @bin 0x00475720 DAT_00f5695c */
    G_TREES_RING = 501,      /* @bin 0x00486600 DAT_00f56b5c */
    G_WATER = 502,           /* @bin 0x0047A220 DAT_00f56b64 */
    G_SPECIAL_ROCKS = 503,   /* @bin 0x004798E0 DAT_00f56b6c */
    G_LAND_ALT = 504,        /* @bin 0x004767D0 DAT_00f56b74 */
    G_ROCKS = 505,           /* @bin 0x00475DC0 DAT_00f56b7c */
    G_CANALS = 506,          /* @bin 0x0047BAE0 DAT_00f56b84 */
    G_LAND = 507,            /* @bin 0x004767D0 DAT_00f56b8c */
    G_0X40000 = 508,         /* @bin 0x0047AB10 DAT_00f56b94 */
    G_TREES = 509,           /* @bin 0x00486600 DAT_00f56b9c */
    G_DUNES = 510,           /* @bin 0x00477210 DAT_00f56ba4 */
    G_FP_COAST = 516,        /* @bin 0x0047A220 DAT_00f56bd4 */
    G_SIGNPOSTS = 521,       /* @bin 0x0047D870 DAT_00f56bfc */
    G_FLOODPLAIN = 528,      /* @bin 0x00479E70 DAT_00f56c34 (0x210) */
    G_0X40000_FULL = 529,    /* @bin 0x00486600 DAT_00f56c3c */
    G_RUBBLE = 531,          /* @bin 0x0047A120 DAT_00f56c4c */
    G_MEADOW = 534,          /* @bin 0x00479B70 0x216 */
    G_ROCK_GOLD = 539,       /* @bin 0x00475DC0 DAT_00f56c8c */
    G_ROADS = 540,           /* @bin 0x0047B670 DAT_00f56c94 */
    G_ROADS_PAVED = 530,     /* @bin 0x0047B670 DAT_00f56c44 */
    G_FP_COAST_IRR = 541,    /* @bin 0x0047A220 DAT_00f56c9c */
    G_GRASS_EDGE = 549,      /* @bin 0x004767D0 0x225 */
    G_GRASS_FULL = 550,      /* @bin 0x004767D0 0x226 */
    G_MEADOW_A2B11 = 551,    /* @bin 0x00479B70 0x227 */
    G_MEADOW_A2B0 = 552,     /* @bin 0x00479B70 0x228 */
    G_GRASS_PART = 553,      /* @bin 0x004767D0 0x229 */
    G_ROADS_FP = 548,        /* @bin 0x0047B670 DAT_00f56cd4 */
    G_GRASS = 543,           /* @bin 0x004767D0 0x21F */
    G_WATER_ANIMS = 558,     /* @bin 0x0047A220 DAT_00f56d24 */
    G_GRASS_HIGH = 561,      /* @bin 0x004767D0 0x231 */
    G_MEADOW_DRY = 563,      /* @bin 0x00479B70 0x233 */
    G_FLAG_0X20 = 328,       /* @bin 0x00476530 DAT_00f565f4 */
    G_NEW_CLIFF = 743,       /* @bin 0x00475DC0 DAT_00f572ec */
};

/* ---- the game state ------------------------------------------------------------------------- */

#define GRID_AT(t, type, base, size, o) STATE_AT((t)->g, type, (base) + (size) * (uint32_t) (o))
#define IMG(t, o) GRID_AT(t, state_u32, 0x00B7482Cu, 4u, o)   /* image grid, chunk 7 */
#define EDGE(t, o) GRID_AT(t, uint8_t, 0x00B67D1Cu, 1u, o)    /* chunk 8 */
#define TER(t, o) GRID_AT(t, state_u32, 0x00AF598Cu, 4u, o)   /* terrain flags, chunk 10 */
#define AQ(t, o) GRID_AT(t, uint8_t, 0x00AE8E7Cu, 1u, o)      /* chunk 11 */
#define BITS(t, o) GRID_AT(t, uint8_t, 0x00B285CCu, 1u, o)    /* chunk 13 */
#define RND(t, o) GRID_AT(t, uint8_t, 0x00ACF85Cu, 1u, o)     /* random grid, chunk 15 */
#define DES(t, o) GRID_AT(t, int8_t, 0x00AC2D4Cu, 1u, o)      /* desirability, chunk 16 */
#define FERT(t, o) GRID_AT(t, uint8_t, 0x00A5D4CCu, 1u, o)    /* fertility, chunk 135 */
#define VEG(t, o) GRID_AT(t, uint8_t, 0x00A43EACu, 1u, o)     /* vegetation, chunk 144 */
#define MOIST(t, o) GRID_AT(t, uint8_t, 0x00A509BCu, 1u, o)   /* moisture, chunk 170 */
/* map geometry (@bin 0x004D52A0, 0x0046F7B0) */
#define MAP_W(t) S32((t)->g, 0x00BCA2D0u)
#define MAP_H(t) S32((t)->g, 0x00BCA2D4u)
#define MAP_START_(t) S32((t)->g, 0x00BCA2D8u)
#define MAP_X0(t) S32((t)->g, 0x00BCA2E8u)
#define MAP_Y0(t) S32((t)->g, 0x00BCA2ECu)
#define MAP_X1(t) S32((t)->g, 0x00BCA2F0u)
#define MAP_Y1(t) S32((t)->g, 0x00BCA2F4u)
#define MAP_CX(t) S32((t)->g, 0x00BCA2F8u)
#define MAP_CY(t) S32((t)->g, 0x00BCA2FCu)
#define ROW_OFF(t, r) GRID_AT(t, state_s32, 0x00BC9804u, 4u, r)
#define ROW_EX(t, r) GRID_AT(t, state_s32, 0x00BC9B94u, 4u, r)
#define ROW_SX(t, r) GRID_AT(t, state_s32, 0x00BC9F24u, 4u, r)
/* pattern matcher results (@bin 0x00484BE0) */
#define M_VARIANT(t) S32((t)->g, 0x00BC9740u)
#define M_EXTRA(t) S32((t)->g, 0x00BC9744u)
#define M_OFFSET(t) S32((t)->g, 0x00BC9748u)
/* neighbour-pattern tables, 16-byte records whose counters persist (generated copies:
 * core/map/terrain_tables.c, read from the same addresses) */
#define PATTERN(t, a) ((uint8_t(*)[16]) state_ptr((t)->g, (a), 16))
/* entry and exit points: scenario info +0x38C (0x00784C64) */
#define POINT(t, k) ((const int16_t *) state_ptr((t)->g, 0x00784C64u + 4u * (k), 4))

/* the floodplain object of a tile (0x00D05720 + 4 * offset), 0 if none: +0x14 depth, +0x18 flag */
static uint32_t fp_object(const terrain_state *t, int o)
{
    return U32(t->g, 0x00D05720u + 4u * (uint32_t) o);
}

/* @bin 0x0047A072 the object's list (object +0) holds the flood progress at +4 */
static int32_t fp_progress(const terrain_state *t, uint32_t obj)
{
    return (int32_t) P32(t->g, P32(t->g, obj) + 4);
}

/* ---- helpers ----------------------------------------------------------------------------- */

static int32_t grp(const terrain_state *t, int g)
{
    return S32(t->g, 0x00F55BB4u + 8u * (uint32_t) g);
}

static int32_t off(const terrain_state *t, int x, int y)
{
    return MAP_START_(t) + y * GRID_SIDE + x;
}

/* @bin 0x0046F930 tile inside the map diamond (absolute coordinates) */
static int inside(const terrain_state *t, int x, int y)
{
    if (MAP_CX(t) < x) {
        return y <= MAP_CY(t) ? x - y <= MAP_X1(t) - MAP_CY(t) : x + y <= MAP_X1(t) + 1 + MAP_CY(t);
    }
    return y <= MAP_CY(t) ? MAP_X0(t) + MAP_CY(t) <= x + y : MAP_CX(t) - MAP_Y1(t) <= x - y;
}

/* @bin 0x0046F9A0 inside the map but within d tiles of the diamond's border */
static int near_edge(const terrain_state *t, int x, int y, int d)
{
    if (!inside(t, x, y)) {
        return 0;
    }
    if (x <= MAP_CX(t)) {
        return y <= MAP_CY(t) ? (x - d) + y < MAP_X0(t) + MAP_CY(t) : (x - y) - d < MAP_CX(t) - MAP_Y1(t);
    }
    return y <= MAP_CY(t) ? MAP_X1(t) - MAP_CY(t) < (x - y) + d : MAP_X1(t) + 1 + MAP_CY(t) < x + y + d;
}

/* @bin 0x0047DB00 neighbour flag = terrain & mask; with exclude: @bin 0x0047DC80 */
/* the neighbour flags scratch the matcher reads (8 dwords) */
#define SCRATCH(t, i) S32((t)->g, 0x00BC97C0u + 4u * (uint32_t) (i))

static void scratch_store(terrain_state *t, const int f[8])
{
    for (int i = 0; i < 8; i++) {
        SCRATCH(t, i) = f[i];
    }
}

static int count_flags(const terrain_state *t, int o, uint32_t mask, uint32_t exclude, int f[8])
{
    int n = 0;
    for (int i = 0; i < 8; i++) {
        uint32_t v = TER(t, o + NEIGHBOURS[i]);
        f[i] = (v & mask) && !(v & exclude);
        n += f[i];
    }
    return n;
}

static int nflags(terrain_state *t, int o, uint32_t mask, uint32_t exclude, int f[8])
{
    int n = count_flags(t, o, mask, exclude, f); /* written to the scratch, as 0/1 */
    scratch_store(t, f);
    return n;
}

static int ncount(const terrain_state *t, int o, uint32_t mask, uint32_t exclude)
{
    int f[8];
    return count_flags(t, o, mask, exclude, f); /* @bin 0x00477690 / 0x004775F0 (no scratch) */
}

static int fp_adjacent_only(const terrain_state *t, int o)
{
    uint32_t obj = fp_object(t, o); /* @bin 0x004767D0 object +0x18 == 0 */
    return obj && P8(t->g, obj + 0x18) == 0;
}

static void set_image(terrain_state *t, int o, uint32_t img)
{
    IMG(t, o) = img;
    EDGE(t, o) |= EDGE_DRAW_TILE;
    BITS(t, o) &= 0xF0;
}

/* ---- pattern matching (@bin 0x00484BE0, 0x00484D70) --------------------------------------- */

static int count_variant(uint8_t *r)
{
    r[14] = (uint8_t) (r[14] + 1);
    if (r[13] <= r[14]) {
        r[14] = 0;
    }
    return r[14];
}

static int rec_matches(const uint8_t *r, const int f[8], int bit)
{
    for (int i = 0; i < 8; i++) {
        if (r[i] != 2 && (r[i] != 0) != ((f[i] & bit) != 0)) {
            return 0;
        }
    }
    return 1;
}

/* @bin 0x00484BE0 first match wins; variant = rnd % modulus (forward: rnd >= 0, backward:
 * rnd != 0) else the record's cycling counter; no match leaves the globals unchanged.
 * @diff terrain_check */
static int match(terrain_state *t, uint8_t (*recs)[16], int count, const int f[8], int rnd, int backward)
{
    scratch_store(t, f); /* the flags the original matches are its scratch */
    for (int i = 0; i < count; i++) {
        int k = backward ? count - 1 - i : i;
        uint8_t *r = recs[k];
        if (rec_matches(r, f, 1)) {
            M_OFFSET(t) = r[8 + S32(t->g, 0x00BCA2E0u) / 2]; /* byte 8 + orientation / 2 */
            M_EXTRA(t) = r[12];
            int use_rnd = backward ? rnd != 0 : rnd >= 0;
            M_VARIANT(t) = use_rnd ? rnd % r[13] : count_variant(r);
            return k + 1;
        }
    }
    return 0;
}

/* @bin 0x00484D70 the orthogonal neighbours' water bits (bit 1) pick a block of per_block
 * records through a 16-entry lookup; the block is matched on bit 0, variant = counter.
 * @diff terrain_check */
static int match_by_water(terrain_state *t, const int f2[8], int per_block)
{
    static const uint8_t BLOCK[16] = {0, 1, 3, 2, 5, 1, 4, 2, 6, 7, 1, 1, 5, 1, 1, 1};
    int mask = 0;
    for (int i = 0; i < 4; i++) {
        if (f2[2 * i + 1] & 2) {
            mask |= 1 << ((i + 1) & 3) | 1 << i;
        }
        if (f2[2 * i] & 2) {
            mask |= 1 << i;
        }
    }
    int block = mask ? BLOCK[mask] : 0;
    for (int k = 0; k < per_block; k++) {
        uint8_t *r = PATTERN(t, 0x005CAC58u)[block * per_block + k];
        if (rec_matches(r, f2, 1)) {
            M_OFFSET(t) = r[8];
            M_EXTRA(t) = r[12];
            M_VARIANT(t) = count_variant(r);
            return 1;
        }
    }
    return 0;
}

/* ---- block placement ---------------------------------------------------------------------- */

/* @bin 0x0047CAE0 (need_empty = 1): every tile's terrain & 0xEEFED77F == flag and image == 0 */
static int fits(const terrain_state *t, int x, int y, int n, uint32_t flag)
{
    if (x < 0 || y < 0 || MAP_W(t) <= x + n - 1 || MAP_H(t) <= y + n - 1) {
        return 0;
    }
    int base = off(t, x, y);
    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++) {
            int o = base + j * GRID_SIDE + i; /* offset table 0x005CC8C0: dx + 228*dy */
            if ((TER(t, o) & 0xEEFED77Fu) != flag || IMG(t, o) != 0) {
                return 0;
            }
        }
    }
    return 1;
}

/* @bin 0x0047BF50 orientation 0: terrain & 0x3052E80 | flag, size in bitfields, edge dx | dy << 3
 * (table 0x005CC8C4), draw bit on tile (0, n-1). @diff terrain_check */
static void place(terrain_state *t, int x, int y, int n, uint32_t image, uint32_t flag)
{
    int base = off(t, x, y);
    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++) {
            int o = base + j * GRID_SIDE + i;
            TER(t, o) = (TER(t, o) & 0x3052E80u) | flag;
            BITS(t, o) = (uint8_t) ((BITS(t, o) & 0xE0) | (n > 1 ? n - 1 : 0));
            IMG(t, o) = image;
            EDGE(t, o) = (uint8_t) (i | j << 3 | ((i == 0 && j == n - 1) ? EDGE_DRAW_TILE : 0));
        }
    }
}

/* @bin 0x0047C880 as (x, y, n, n, 0xFFFF, need_empty=1, strict=1); sets the cursor 0x00BC9768 to
 * the block's first tile; a tile with a figure (figure grid 0x00B350DC) does not fit */
static int fits_land(terrain_state *t, int x, int y, int n)
{
    if (x < 3 || y < 3 || MAP_W(t) - 3 <= x + n - 1 || MAP_H(t) - 3 <= y + n - 1) {
        return 0;
    }
    int base = off(t, x, y), meadow = -1;
    S32(t->g, 0x00BC9768u) = base;
    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++) {
            int o = base + j * GRID_SIDE + i;
            if (near_edge(t, o % GRID_SIDE, o / GRID_SIDE, 3) || fp_adjacent_only(t, o)) {
                return 0;
            }
            uint32_t v = TER(t, o) & (0xFFFFu | 0x10010000u);
            int m = (v & 0x10000) != 0;
            if (meadow < 0) {
                meadow = m;
            } else if (meadow != m) {
                return 0;
            }
            if ((v & 0xFEFED77Fu) || S16(t->g, 0x00B350DCu + 2u * (uint32_t) o) != 0 || IMG(t, o) != 0) {
                return 0;
            }
        }
    }
    return 1;
}

/* @bin 0x0047C140 (building, x, y, n, n, image, flag) every tile gets its own image (image + k,
 * row-major), the draw bit, the flag and the building id; out of the map: 0x00BC9734 := 1 */
static void place_subimages(terrain_state *t, int16_t building, int x, int y, int n, uint32_t image, uint32_t flag)
{
    /* outside the map: only the error flag 0x00BC9734 */
    if (x < 0 || y < 0 || MAP_W(t) <= x - 1 + n || MAP_H(t) <= y - 1 + n) {
        S32(t->g, 0x00BC9734u) = 1;
        return;
    }
    int base = off(t, x, y), k = 0;
    S32(t->g, 0x00BC9768u) = base;
    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++, k++) {
            int o = base + j * GRID_SIDE + i;
            BITS(t, o) &= 0xE0;
            EDGE(t, o) |= EDGE_DRAW_TILE;
            TER(t, o) = (TER(t, o) & 0x3052E80u) | flag;
            S16(t->g, 0x00B4E6FCu + 2u * (uint32_t) o) = building; /* building grid */
            IMG(t, o) = image + (uint32_t) k;
        }
    }
}

/* ---- rocks and dunes (@bin 0x00475DC0, 0x00477210) ---------------------------------------- */

typedef int (*terrain_test)(uint32_t);

static int is_rock(uint32_t v)
{
    return (v & 2) != 0;
}

/* @bin 0x00475FD0 cliff tile image index, or -1. Neighbour flags: cliff (0x0047DBC0(0x20000002):
 * (t & mask) == mask) unless the neighbour has 0x80000. Buildings next to cliffs (building grid
 * 0x00B4E6FC, types 0xE5/0xE6/0xEA-0xEC) also set a flag; a new scenario has none.
 * Index tables 0x005DBCC8..0x005DBD98, column orientation/2 = 0. @diff terrain_check */
static int cliff_index(const terrain_state *t, int o)
{
    int f[8], n = 0, r = RND(t, o);
    for (int i = 0; i < 8; i++) {
        uint32_t v = TER(t, o + NEIGHBOURS[i]);
        f[i] = (v & 0x80000u) ? 0 : (v & 0x20000002u) == 0x20000002u;
        n += f[i];
    }
    int c0 = f[0], c4 = f[1], c8 = f[2], cc = f[3], d0 = f[4], d4 = f[5], d8 = f[6], dc = f[7];
    if (n == 8) {
        return r % 0x18 + 0x30;
    }
    if (n == 7) {
        if (!c4) return 12 + r % 3;  /* 0x005DBCC8 */
        if (!dc) return 15 + r % 3;  /* 0x005DBCD8 */
        if (!d4) return 18 + r % 3;  /* 0x005DBCE8 */
        if (!cc) return 21 + r % 3;  /* 0x005DBCF8 */
    } else if (n == 6) {
        if (!dc && !cc) return 72 + r % 3;  /* 0x005DBD88 */
        if (!c4 && !d4) return 75 + r % 3;  /* 0x005DBD98 */
    }
    if (d8 && dc && c0 && c4 && c8 && n < 8) return 24 + r % 6;  /* 0x005DBD08 */
    if (d0 && d4 && d8 && dc && c0 && n < 8) return 30 + r % 6;  /* 0x005DBD18 */
    if (c8 && cc && d0 && d4 && d8 && n < 8) return 36 + r % 6;  /* 0x005DBD28 */
    if (c0 && c4) {
        if (c8 && cc && d0 && n < 8) return 42 + r % 6;          /* 0x005DBD38 */
        if (c4 && c8) return 0 + r % 3;                         /* 0x005DBD48 */
    }
    if (d8 && dc && c0) return 3 + r % 3;  /* 0x005DBD58 */
    if (d0 && d4 && d8) return 6 + r % 3;  /* 0x005DBD68 */
    if (c8 && cc && d0) return 9 + r % 3;  /* 0x005DBD78 */
    return -1;
}

static int is_dune(uint32_t v)
{
    return (v & 0x2000000u) != 0;
}

/* @bin 0x00482650 scan the square of n tiles grown by d (clamped) for terrain & mask, rows then
 * columns; leaves the tile it stopped at in 0x00D3B4E0 (y) and 0x00D3B4E4 (x). Returns 1 on a
 * match. The dune pass calls it for this side effect only. */
static int scan_square(terrain_state *t, int x, int y, int n, int d, uint32_t mask)
{
    int xa = x - d, xb = x + n - 1 + d, yb = n + y - 1 + d;
    int32_t *sy = &S32(t->g, 0x00D3B4E0u), *sx = &S32(t->g, 0x00D3B4E4u);
    *sy = y - d;
    if (xa < 0) {
        xa = 0;
    }
    if (*sy < 0) {
        *sy = 0;
    }
    if (MAP_W(t) <= xb) {
        xb = MAP_W(t) - 1;
    }
    if (MAP_H(t) <= yb) {
        yb = MAP_H(t) - 1;
    }
    for (; *sy <= yb; (*sy)++) {
        for (*sx = xa; *sx <= xb; (*sx)++) {
            if (TER(t, off(t, *sx, *sy)) & mask) {
                return 1;
            }
        }
    }
    return 0;
}

static void blocks(terrain_state *t, terrain_test test, int group, int gold_group)
{
    /* @bin 0x00475DC0 / 0x00477210 clear, then scan rows: 3x3, 2x2, 1x1. @diff terrain_check */
    for (int r = 0; r < MAP_H(t); r++) {
        for (int x = ROW_SX(t, r), o = ROW_OFF(t, r); x < ROW_EX(t, r); x++, o++) {
            uint32_t v = TER(t, o);
            if (test(v) && !(BITS(t, o) & 0x80) && !(v & 0x600)) {
                set_image(t, o, 0);
            }
        }
    }
    for (int r = 0; r < MAP_H(t); r++) {
        for (int x = ROW_SX(t, r), o = ROW_OFF(t, r); x < ROW_EX(t, r); x++, o++) {
            uint32_t v = TER(t, o);
            if (!(test(v) && !(BITS(t, o) & 0x80) && !(v & 0x600) && IMG(t, o) == 0)) {
                continue;
            }
            uint32_t flag = test == is_dune ? 0x2000000u : 2;
            int32_t base = grp(t, group);
            if (gold_group && (v & 0x100002u) == 0x100002u) {
                flag = 0x100002u;
                base = grp(t, gold_group);
            } else if (gold_group && (v & 0x20000002u) == 0x20000002u) {
                int c = cliff_index(t, o); /* @bin 0x00475DC0 cliff branch */
                IMG(t, o) = (uint32_t) (c < 0 ? (RND(t, o) & 7) + base : c + grp(t, G_NEW_CLIFF));
                continue;
            }
            int rnd = RND(t, o), dune = test == is_dune;
            if (fits(t, x, r, 3, flag)) {
                if (dune) {
                    scan_square(t, x, r, 3, 4, 0x200); /* @bin 0x00477210 */
                }
                place(t, x, r, 3, (uint32_t) ((rnd & 1) + 12 + base), flag);
            } else if (fits(t, x, r, 2, flag)) {
                if (dune) {
                    scan_square(t, x, r, 2, 4, 0x200);
                }
                place(t, x, r, 2, (uint32_t) ((rnd & 3) + 8 + base), flag);
            } else {
                if (dune) {
                    scan_square(t, x, r, 1, 4, 0x200);
                }
                IMG(t, o) = (uint32_t) ((rnd & 7) + base);
            }
        }
    }
}

static void pass_rocks(terrain_state *t)
{
    blocks(t, is_rock, G_ROCKS, G_ROCK_GOLD);
}

static void pass_dunes(terrain_state *t)
{
    blocks(t, is_dune, G_DUNES, 0); /* 0x00482650's result does not change the image */
}

/* ---- flag 0x20 (@bin 0x00476530) ---------------------------------------------------------- */

static void pass_flag_0x20(terrain_state *t)
{
    int32_t base = grp(t, G_FLAG_0X20);
    for (int r = 0; r < MAP_H(t); r++) {
        for (int x = ROW_SX(t, r), o = ROW_OFF(t, r); x < ROW_EX(t, r); x++, o++) {
            if ((TER(t, o) & 0x20) && !(TER(t, o) & 0x600)) {
                set_image(t, o, 0);
            }
        }
    }
    for (int r = 0; r < MAP_H(t); r++) {
        for (int x = ROW_SX(t, r), o = ROW_OFF(t, r); x < ROW_EX(t, r); x++, o++) {
            uint32_t v = TER(t, o);
            if (!((v & 0x20) && !(v & 0x600) && IMG(t, o) == 0)) {
                continue;
            }
            if (fits(t, x, r, 3, 0x20)) {
                place(t, x, r, 3, (uint32_t) (base + 7), 0x20);
            } else if (fits(t, x, r, 2, 0x20)) {
                int rnd = RND(t, o) & 3;
                place(t, x, r, 2, (uint32_t) (base + (rnd < 2 ? 6 : rnd < 3 ? 5 : 4)), 0x20);
            } else {
                static const int EVEN[4] = {0, 1, 0, 1}, ODD[4] = {2, 3, 2, 3};
                IMG(t, o) = (uint32_t) (base + ((r & 1) == 0 ? EVEN[x & 3] : ODD[x & 3]));
            }
        }
    }
}

/* ---- grass, land (@bin 0x004767D0) -------------------------------------------------------- */

static int plain_around(const terrain_state *t, int p)
{
    for (int i = 0; i < 8; i++) {
        if (TER(t, p + NEIGHBOURS[i]) & 0xEEFFD77Fu) {
            return 0;
        }
    }
    return 1;
}

/* Grass level: x87 trunc(v * 12.0 / 96.0) for v <= 100 equals v >> 3 (0.125 is exact).
 * @bin 0x00476B1C FLD [0x0056F578]=12.0, FDIV [0x0056F570]=96.0 @diff terrain_check */
static int grass_level(int v)
{
    return v >> 3;
}

static uint32_t grass_image(terrain_state *t, int o)
{
    /* @bin 0x00476A93..0x00476D85 (disassembly) @diff terrain_check */
    int group, idx, f[8];
    if (ncount(t, o, 0x10000, 0)) {
        nflags(t, o, 0x10000, 0, f);
        match(t, PATTERN(t, 0x005CBFB8u), 0x1A, f, -1, 0);
        group = G_FLOODPLAIN;
        idx = M_OFFSET(t) + 0x30;
    } else {
        int v = MOIST(t, o), lvl;
        if (v > 100) { /* FCOM [0x0056F470] = 100.0 */
            lvl = v;
            int orient = S32(t->g, 0x00BCA2E0u);
            lvl = (((lvl & 8) == 0 ? ((lvl - orient) & 7) : ((lvl - orient / 2) & 3) + (lvl & 8)) & 0xFF) | 0x80;
        } else {
            lvl = grass_level(v);
        }
        int rnd = RND(t, o);
        if (lvl < 0xB) {
            group = G_GRASS;
            idx = lvl + (rnd % 3) * 12;
        } else if (lvl < 0x80) {
            if (!(TER(t, o) & 0x10000000u) && !plain_around(t, o)) {
                group = G_GRASS_EDGE;
            } else {
                int ok = 1;
                for (int i = 0; i < 8 && ok; i++) {
                    int n = o + NEIGHBOURS[i];
                    int nv = MOIST(t, n) > 100 ? 100 : MOIST(t, n);
                    if (grass_level(nv) != 0xB || (TER(t, n) & 0x10801u)
                        || (!(TER(t, n) & 0x10000000u) && !plain_around(t, n))) {
                        ok = 0;
                    }
                }
                group = ok ? G_GRASS_FULL : G_GRASS_PART;
            }
            idx = rnd % 12;
        } else {
            group = G_GRASS_HIGH;
            idx = lvl & 0x7F;
        }
    }
    if (TER(t, o) & 0x40) { /* @bin 0x0051C5F0 */
        idx = idx == 0x34 ? 0x56 : idx == 0x30 ? 0x54 : idx == 0x36 ? 0x57 : idx == 0x32 ? 0x55 : idx;
    }
    return (uint32_t) (grp(t, group) + idx);
}

/* @bin 0x004767D0(x0, y0, x1, y1, param_5 = !fill, param_6 = 0): the rectangle grows by one tile */

#define BUILDING_AT(t, id) (0x007999D8u + 0x108u * (uint32_t) (id))
#define BUILDING_TYPE(t, id) S16((t)->g, BUILDING_AT(t, id) + 0x10)
#define BUILDING_PART(t, id) S16((t)->g, BUILDING_AT(t, id) + 0x12)
#define BGRID(t, o) S16((t)->g, 0x00B4E6FCu + 2u * (uint32_t) (o))

static void land_rect(terrain_state *t, int x0, int y0, int x1, int y1, int fill, int grow);
static void trees_rect(terrain_state *t, int x0, int y0, int x1, int y1);
static void meadow_rect(terrain_state *t, int x0, int y0, int x1, int y1);

/* @bin 0x004F3D70 a pyramid tile's state, from its building (grid offset +0x0C, kind +0x54,
 * stage +0x55, part +0x12, corner flags +0x5B): -1 when the tile has no pyramid; stage 3 uses the
 * part tables of the pyramid kind (0x005F6D58 for kind 5, else 0x005F6CD0) */
int32_t pyramid_tile_state(game_state *g, int32_t o)
{
    int16_t b = S16(g, 0x00B4E6FCu + 2u * (uint32_t) o);
    if (b == 0) {
        return -1;
    }
    uint32_t p = 0x007999D8u + 0x108u * (uint32_t) b;
    if (S16(g, p + 0x10) != 0xB7) {
        return -1;
    }
    int32_t origin = S32(g, p + 0x0C);
    uint8_t corner;
    switch (o - origin) {
    case 0: corner = 1; break;
    case 1: corner = 2; break;
    case 0xE4: corner = 4; break;
    case 0xE5: corner = 8; break;
    default: return 2;
    }
    int32_t stage = U8(g, p + 0x55);
    if (stage == 3) {
        uint32_t table = S32(g, 0x005E4EF4u) == 5 ? 0x005F6D58u : 0x005F6CD0u;
        uint32_t entry = table + 8u * U8(g, p + 0x54);
        int32_t n = S32(g, entry + 4);
        uint32_t list = U32(g, entry);
        for (int32_t i = 0; i < n; i++) {
            if (S16(g, p + 0x12) == S32(g, list + 8u * (uint32_t) i)) {
                return (o == S32(g, list + 8u * (uint32_t) i + 4) + origin) + 2;
            }
        }
        return 2;
    }
    if (U8(g, p + 0x5B) & corner) {
        return stage;
    }
    return stage - 1 != 3 ? stage - 1 : 2;
}

/* @bin 0x004F3F70 (building, tile, state, redo) a monument tile's image from the states of its 8
 * neighbours (offsets 0x005F7570, rotated by the view orientation): state 3 table 0x00EAAF60 with
 * mask 0, state 5 table 0x005F6D70 (any neighbour state 0 or >= 5 counts) with 8 variants, states
 * 6..10 table 0x005F7170 with mask 0x55; state 4: with redo, the tile is cleared and redrawn by
 * the tree, land and meadow passes. Returns 1 when it set the image (0 for state 4 without redo
 * and for other states). */
static int monument_tile_image(terrain_state *t, int16_t building, int o, int state, int redo)
{
    (void) building;
    uint32_t table, mask = 0xFF;
    int32_t base, variants = 1;
    switch (state) {
    case 3: table = 0x00EAAF60u; base = S32(t->g, 0x00F5736Cu); mask = 0; break;
    case 4:
        if (redo) {
            IMG(t, o) = 0;
            int x = (o - MAP_START_(t)) % GRID_SIDE, y = (o - MAP_START_(t)) / GRID_SIDE;
            trees_rect(t, x, y, x, y);
            land_rect(t, x, y, x, y, 1, 0);
            meadow_rect(t, x, y, x, y);
        }
        return redo;
    case 5: table = 0x005F6D70u; base = S32(t->g, 0x00F5733Cu); variants = 8; break;
    case 6: table = 0x005F7170u; base = S32(t->g, 0x00F57344u); mask = 0x55; break;
    case 7: table = 0x005F7170u; base = S32(t->g, 0x00F5734Cu); mask = 0x55; break;
    case 8: table = 0x005F7170u; base = S32(t->g, 0x00F57344u); mask = 0x55; break;
    case 9: table = 0x005F7170u; base = S32(t->g, 0x00F57354u); mask = 0x55; break;
    case 10: table = 0x005F7170u; base = S32(t->g, 0x00F5735Cu); mask = 0x55; break;
    default: return 0;
    }
    uint32_t bits = 0;
    for (int i = 0; i < 8; i++) {
        uint32_t d = (uint32_t) (i + S32(t->g, 0x00BCA2E0u)) & 7;
        int32_t s = pyramid_tile_state(t->g, o + S32(t->g, 0x005F7570u + 4 * d));
        if (state == 5 ? (s >= 5 || s == 0) : s >= 0) {
            bits |= 1u << i;
        }
    }
    int32_t img = S32(t->g, table + 4 * (bits & mask)) + base;
    if ((bits & mask) == 0xFF && variants > 1) {
        img += RND(t, o) % variants;
    }
    IMG(t, o) = (uint32_t) img;
    return 1;
}

/* @bin 0x004F60D0 a pyramid (its main building: id - part) joins the list 0x00EAAC30 (count
 * 0x00EAAF64, at most 4) unless already there */
static void pyramid_register(terrain_state *t, int16_t building)
{
    if (BUILDING_TYPE(t, building) != 0xB7) {
        return;
    }
    int32_t main_id = building - BUILDING_PART(t, building), n = S32(t->g, 0x00EAAF64u);
    for (int32_t i = 0; i < n; i++) {
        if (S32(t->g, 0x00EAAC30u + 4u * (uint32_t) i) == main_id) {
            return;
        }
    }
    if (n < 4) {
        S32(t->g, 0x00EAAC30u + 4u * (uint32_t) n) = main_id;
        S32(t->g, 0x00EAAF64u) = n + 1;
    }
}

/* @bin 0x004767D0 a tile with terrain 0x10000000 (part of a monument): 1 when the land pass leaves
 * it alone (the monument draws it), else 0 and *v = the terrain without 0x10000008 */
static int monument_tile(terrain_state *t, int o, uint32_t *v)
{
    int16_t b = BGRID(t, o);
    *v &= 0xEFFFFFF7u;
    int16_t type = BUILDING_TYPE(t, b);
    if (type == 0xB7) {
        int st = pyramid_tile_state(t->g, o);
        return 10 < st || (2 < st && monument_tile_image(t, b, o, st, 0));
    }
    if (type == 0xD5 || type == 0xE3 || type == 0xE5 || type == 0xEA || type == 0xEB || type == 0xEC ||
        type == 0xE4) {
        return S32(t->g, BUILDING_AT(t, b) + 0x58) != 0 ||
               S32(t->g, BUILDING_AT(t, b - BUILDING_PART(t, b)) + 0x58) != 0;
    }
    if (type == 0xE6) {
        return 0 < S32(t->g, BUILDING_AT(t, b - BUILDING_PART(t, b)) + 0x54);
    }
    if (type != 0xE1) {
        /* a pyramid nearby: rows of 20 tiles back, while the tiles are monument tiles */
        int k = o - 0x14, id = 0;
        while (S32(t->g, 0x00BCA2D8u) < k) {
            id = BGRID(t, k);
            if (id != 0 || !(TER(t, k) & 0x10000000u)) {
                break;
            }
            k -= 0x14;
        }
        if (id != 0 && BUILDING_TYPE(t, id) == 0xB7) {
            pyramid_register(t, (int16_t) id); /* @bin 0x004F60D0 */
        }
        return 0;
    }
    int main_id = b - BUILDING_PART(t, b), least = 0x15; /* 25 parts: the smallest +0x58 */
    for (int i = 0; i < 0x19; i++) {
        int32_t w = S32(t->g, BUILDING_AT(t, main_id + i) + 0x58);
        if (w < least) {
            least = w;
        }
    }
    return least >= 1;
}

/* @bin 0x004767D0(x0, y0, x1, y1, param_5 = !fill, param_6 = !grow): with grow, the rectangle
 * grows by one tile; clamped to the map */
static void land_rect(terrain_state *t, int x0, int y0, int x1, int y1, int fill, int grow)
{
    int32_t land = grp(t, G_LAND), alt = grp(t, G_LAND_ALT);
    if (grow) {
        x0--, y0--, x1++, y1++;
    }
    x0 = x0 < 0 ? 0 : x0;
    y0 = y0 < 0 ? 0 : y0;
    x1 = x1 > MAP_W(t) - 1 ? MAP_W(t) - 1 : x1;
    y1 = y1 > MAP_H(t) - 1 ? MAP_H(t) - 1 : y1;
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            int o = off(t, x, y);
            uint32_t v = TER(t, o);
            if ((v & 0x10000000u) && monument_tile(t, o, &v)) {
                continue;
            }
            uint32_t plain = fp_adjacent_only(t, o) ? (v & 8) : (v & 0xEEFFD77Fu);
            if (plain) {
                continue;
            }
            if (!(v & 0x80)) {
                if (fill) {
                    set_image(t, o, 0);
                }
            } else if (!(v & 0x22310803u)) {
                IMG(t, o) = grass_image(t, o);
            }
        }
    }
    if (!fill) {
        return;
    }
    for (int y = y0; y <= y1; y++) { /* round 1: single tiles where random & 0xF0 == 0 */
        for (int x = x0; x <= x1; x++) {
            int o = off(t, x, y);
            uint32_t v = TER(t, o), flag = 0;
            int16_t bid = 0;
            if (v & 0x10000000u) { /* a monument tile keeps its building and flags */
                flag = 0x10000008u;
                v &= 0xEFFFFFF7u;
                bid = BGRID(t, o);
            }
            if (!(v & 0xEEFED77Fu) && IMG(t, o) == 0 && !(RND(t, o) & 0xF0)) {
                int32_t base = (BITS(t, o) & 0x20) ? alt : land;
                place_subimages(t, bid, x, y, 1, (uint32_t) ((RND(t, o) & 7) + base), flag);
            }
        }
    }
    for (int y = y0; y <= y1; y++) { /* round 2: largest block that fits */
        for (int x = x0; x <= x1; x++) {
            int o = off(t, x, y);
            uint32_t v = TER(t, o), flag = 0;
            int16_t bid = 0;
            if (v & 0x10000000u) {
                flag = 0x10000008u;
                v &= 0xEFFFFFF7u;
                bid = BGRID(t, o);
            }
            if ((v & 0xEEFED77Fu) || IMG(t, o) != 0) {
                continue;
            }
            int32_t base = (BITS(t, o) & 0x20) ? alt : land;
            int rnd = RND(t, o);
            if (fits_land(t, x, y, 4)) {
                place_subimages(t, bid, x, y, 4, (uint32_t) (base + 0x2A), flag);
            } else if (fits_land(t, x, y, 3)) {
                place_subimages(t, bid, x, y, 3, (uint32_t) ((rnd & 1) * 9 + 0x18 + base), flag);
            } else if (fits_land(t, x, y, 2)) {
                place_subimages(t, bid, x, y, 2, (uint32_t) (base + 8 + (rnd & 3) * 4), flag);
            } else {
                place_subimages(t, bid, x, y, 1, (uint32_t) ((rnd & 7) + base), flag);
            }
        }
    }
}

static void pass_land(terrain_state *t)
{
    land_rect(t, 0, 0, MAP_W(t) - 1, MAP_H(t) - 1, 1, 1);
}

/* ---- floodplain (@bin 0x004777C0 -> 0x00479E70) ------------------------------------------- */

/* Fertility term: x87 trunc(f * 0.01 * 8.0) equals (f * 8) / 100 for every f in 0..255.
 * @bin 0x0047A049 FMUL [0x0056F598]=0.01, FMUL [0x0056F590]=8.0 @diff terrain_check */
static int fertility_level(int f)
{
    return f * 8 / 100;
}

static void floodplain_tile(terrain_state *t, int x, int y)
{
    if (!inside(t, MAP_X0(t) + x, MAP_Y0(t) + y)) {
        return;
    }
    TER(t, off(t, x, y)) |= 0x10000;
    int32_t base = grp(t, G_FLOODPLAIN);
    int ya = y - 1 < 0 ? 0 : y - 1, yb = y + 1 > MAP_H(t) - 1 ? MAP_H(t) - 1 : y + 1;
    int xa = x - 1 < 0 ? 0 : x - 1, xb = x + 1 > MAP_W(t) - 1 ? MAP_W(t) - 1 : x + 1;
    for (int yy = ya; yy <= yb; yy++) {
        for (int xx = xa; xx <= xb; xx++) {
            int o = off(t, xx, yy), f[8], w[8];
            uint32_t v = TER(t, o);
            if (!((v & 0x10000) && !(v & 0x148))) {
                continue;
            }
            nflags(t, o, 0x10000, 0, f);
            for (int i = 0; i < 8; i++) {
                f[i] = !f[i]; /* @bin 0x00484F00 invert */
            }
            if (fp_adjacent_only(t, o)) {
                match(t, PATTERN(t, 0x005CAC58u), 0x18, f, 0, 1);
            } else {
                nflags(t, o, 4, 0, w);
                for (int i = 0; i < 8; i++) {
                    f[i] |= w[i] ? 2 : 0; /* @bin 0x0047DDD0(4, 1) */
                }
                f[1] = f[3] = f[5] = f[7] = 0;
                match_by_water(t, f, 0x18);
            }
            M_OFFSET(t) = 0;
            int32_t img = base + M_VARIANT(t);
            uint32_t obj = fp_object(t, o);
            if (obj) {
                int lvl = fertility_level(FERT(t, o));
                img += fp_progress(t, obj) + ((lvl & ~1) | (RND(t, o) & 1)) * 6;
            }
            set_image(t, o, (uint32_t) img);
        }
    }
}

static void pass_floodplain(terrain_state *t)
{
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            uint32_t v = TER(t, off(t, x, y));
            if ((v & 0x10000) && !(v & 8)) {
                floodplain_tile(t, x, y);
            }
        }
    }
}

/* @bin 0x004BE160 (this = 0x00D05720) floodplain build over the rectangle grown by one tile and
 * clamped to the map: passes from depth 30 down while a pass adds a tile. A tile not yet in the
 * local depth grid, on floodplain (terrain 0x10000) or next to it, gets depth 30 when next to
 * water (terrain 4 without 0x90000), else one less than the deepest floodplain neighbour (with
 * the marker rule); surrounded tiles of plain water are skipped. It gets terrain & 0xFDFBF7FF |
 * 0x80, the land pass around it (0x004746D0), and an object in the list of its depth
 * (0x004BD740). Then the deepest non-empty lists are counted (+0x32FB8) and the terrain rebuilt.
 * @diff terrain_check */
void terrain_floodplain_build(terrain_state *t, int32_t ax, int32_t ay, int32_t bx, int32_t by)
{
    static uint8_t depth[GRID_SIZE]; /* the stack buffer of 0x004BE160 */
    floodplain_manager_reset(t->g);  /* @bin 0x004BE0F0 */
    memset(depth, 0, sizeof(depth));
    int32_t xa = ax <= bx ? ax : bx, xb = ax <= bx ? bx : ax;
    int32_t ya = ay <= by ? ay : by, yb = ay <= by ? by : ay;
    xa--, xb++, ya--, yb++;
    if (xa < 0) {
        xa = 0;
    }
    if (MAP_W(t) <= xb) {
        xb = MAP_W(t) - 1;
    }
    if (ya < 0) {
        ya = 0;
    }
    if (MAP_H(t) <= yb) {
        yb = MAP_H(t) - 1;
    }
    int marker = FLOODPLAIN_MAX_DEPTH, changed;
    do {
        changed = 0;
        for (int32_t y = ya; y <= yb; y++) {
            for (int32_t x = xa; x <= xb; x++) {
                int o = off(t, x, y);
                if (depth[o]) {
                    continue;
                }
                uint8_t flag = (uint8_t) ((TER(t, o) >> 16) & 1);
                int n_fp = ncount(t, o, 0x10000, 0);
                if (!flag && !n_fp) {
                    continue;
                }
                int d = FLOODPLAIN_MAX_DEPTH;
                int n_water = ncount(t, o, 4, 0x90000);
                if (n_water + n_fp == 8 && (TER(t, o) & 0x10004) == 4) {
                    continue;
                }
                if (n_water == 0) {
                    d = 0;
                    for (int i = 0; i < 8; i++) {
                        int n = o + NEIGHBOURS[i], nd = depth[n];
                        if ((TER(t, n) & 0x10000) && d < nd && (nd == 1 || nd != marker)) {
                            d = nd - 1;
                            if (d < 1) {
                                d = 1;
                            }
                        }
                    }
                    if (d == 0) {
                        continue;
                    }
                }
                TER(t, o) = (TER(t, o) & 0xFDFBF7FFu) | 0x80;
                land_rect(t, x - 5, y - 5, x + 6, y + 6, 0, 1); /* @bin 0x004746D0(x, y, 1) */
                depth[o] = (uint8_t) d;
                U32(t->g, FLOODPLAIN_MANAGER + 4u * (uint32_t) o) = floodplain_object_create(
                    t->g, FLOODPLAIN_LISTS + FLOODPLAIN_LIST_SIZE * (uint32_t) (d - 1), o, x, y, d, flag);
                changed = 1;
                U8(t->g, FLOODPLAIN_MANAGER + 0x32FBCu) = 1;
                t->stats.floodplain_objects++;
            }
        }
        marker--;
    } while (changed);
    S32(t->g, FLOODPLAIN_MANAGER + 0x32FB8u) = 0;
    for (int k = 0x1D; k >= 0 && S32(t->g, FLOODPLAIN_LISTS + FLOODPLAIN_LIST_SIZE * (uint32_t) k) != 0; k--) {
        S32(t->g, FLOODPLAIN_MANAGER + 0x32FB8u) += 1;
    }
    terrain_rebuild(t); /* @bin 0x004DEA50 */
}

/* ---- water (@bin 0x00475CC0 -> 0x0047A220; 'blood river' region is zero on a new map) ------ */

static int fp_irrigation_index(int offset, const int f[8])
{
    /* @bin 0x0047A220 second switch on DAT_00bc9748 (orientation 0) */
    int c0 = f[0], c2 = f[2], c4 = f[4], c6 = f[6], i8;
    if (offset == 8) return c4 ? 3 : -1;
    if (offset == 0xC) return c6 ? 0 : -1;
    if (offset == 0x10) return c0 ? 1 : -1;
    if (offset == 0x14) return c2 ? 2 : -1;
    int l7bf = c2 ? (c0 ? 5 : (c4 ? 6 : -1)) : -1;
    if (c6 == 0) {
        i8 = l7bf;
    } else if (c4 == 0) {
        i8 = c0 == 0 ? l7bf : 4;
    } else {
        i8 = 7;
    }
    if (offset == 0x18) return c4 ? 0xB : (c6 ? 8 : i8);
    if (offset == 0x1C) return c6 ? 0xD : (c0 ? 10 : i8);
    if (offset == 0x20) return c2 ? 0xC : (c0 ? 0xF : i8);
    if (offset == 0x24) return c2 ? 9 : (c4 ? 0xE : i8);
    return i8;
}

static uint32_t water_image(terrain_state *t, int o)
{
    /* @bin 0x0047A220 @diff terrain_check */
    int f[8], fpn[8];
    uint32_t v = TER(t, o);
    nflags(t, o, 4, 0, f);
    for (int i = 0; i < 8; i++) {
        f[i] = !f[i];
    }
    match(t, PATTERN(t, 0x005CA658u), 0x30, f, RND(t, o), 0);
    int32_t img = -1;
    if (v & 0x10000) {
        if (M_OFFSET(t) >= 8) {
            img = grp(t, G_FP_COAST) + M_VARIANT(t) + M_OFFSET(t);
        }
    } else {
        int n = nflags(t, o, 0x10000, 0, fpn);
        if (n) {
            int adj[8], i8 = -1, via_5e2 = 0, i11 = (n > 2 ? 2 : n) * 2, offv = M_OFFSET(t);
            for (int i = 0; i < 8; i++) {
                adj[i] = fp_adjacent_only(t, o + NEIGHBOURS[i]); /* local_8 */
            }
            switch (offv) {
            case 8: i8 = adj[4] ? 4 : -1; break;
            case 0xC: i8 = adj[6] ? 6 : -1; break;
            case 0x10: i8 = adj[0] ? 0 : -1; break;
            case 0x14: i8 = adj[2] ? 2 : -1; break;
            case 0x18: i8 = adj[6] ? 6 : (adj[4] ? 4 : -1); via_5e2 = i8 >= 0; break;
            case 0x1C: i8 = adj[6] ? 6 : (adj[0] ? 0 : -1); via_5e2 = i8 >= 0; break;
            case 0x20: i8 = adj[2] ? 2 : (adj[0] ? 0 : -1); via_5e2 = i8 >= 0; break;
            case 0x24: i8 = adj[2] ? 2 : (adj[4] ? 4 : -1); via_5e2 = i8 >= 0; break;
            default: break;
            }
            if (i8 >= 0) {
                if (via_5e2) {
                    if (i11 == 2 && (fpn[6] || fpn[4] || fpn[2] || fpn[0])) {
                        i11 = 4;
                    }
                } else {
                    i11 = 0;
                }
                img = grp(t, G_FLOODPLAIN) + (((i8 - 4) & 7) + 0x14) * 3 + i11; /* LAB_0047a60b */
                if (fpn[(1 + i8) & 7] || fpn[(2 + i8) & 7]) {
                    img += 1;
                }
            } else { /* case 9 */
                int f2[8];
                img = grp(t, G_FP_COAST) + M_VARIANT(t) + M_OFFSET(t);
                nflags(t, o, 0x100, 4, f2);
                f2[1] = f2[3] = f2[5] = f2[7] = 0;
                if (f2[0] + f2[2] + f2[4] + f2[6]) {
                    int k = fp_irrigation_index(M_OFFSET(t), f2);
                    if (k >= 0) {
                        img = grp(t, G_FP_COAST_IRR) + k;
                    }
                }
            }
        }
    }
    if (img < 0) { /* LAB_0047a8e6 */
        if (!(v & 0x4000000u)) {
            img = M_OFFSET(t) == 0 ? grp(t, G_FP_COAST) + M_VARIANT(t)
                                    : grp(t, G_WATER) + M_VARIANT(t) + M_OFFSET(t);
        } else {
            for (int k = 0; k < 0x1A; k++) {
                PATTERN(t, 0x005CBFB8u)[k][14] = 0; /* @bin 0x00484EE0 */
            }
            nflags(t, o, 0x4000000u, 0, f);
            for (int i = 0; i < 8; i++) {
                f[i] = !f[i];
            }
            match(t, PATTERN(t, 0x005CBFB8u), 0x1A, f, RND(t, o), 1);
            img = grp(t, G_WATER_ANIMS) + M_VARIANT(t) * 15 + M_OFFSET(t);
        }
    }
    return (uint32_t) img;
}

static void pass_water(terrain_state *t)
{
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            uint32_t v = TER(t, off(t, x, y));
            if (!((v & 4) && !(v & 8)) || !inside(t, MAP_X0(t) + x, MAP_Y0(t) + y)) {
                continue;
            }
            int ya = y - 1 < 0 ? 0 : y - 1, yb = y + 1 > MAP_H(t) - 1 ? MAP_H(t) - 1 : y + 1;
            int xa = x - 1 < 0 ? 0 : x - 1, xb = x + 1 > MAP_W(t) - 1 ? MAP_W(t) - 1 : x + 1;
            for (int yy = ya; yy <= yb; yy++) {
                for (int xx = xa; xx <= xb; xx++) {
                    int o = off(t, xx, yy);
                    if ((TER(t, o) & 4) && !(TER(t, o) & 8)) {
                        set_image(t, o, water_image(t, o));
                    }
                }
            }
        }
    }
}

/* ---- trees, vegetation (@bin 0x00475BF0 -> 0x00479770 -> 0x00486600) ---------------------- */

/* @bin 0x00481490 ring k (rings built by 0x0046F8D0: max(|dx|, |dy|) == k); tiles more than one
 * tile outside the map square are skipped (0x00481AA0). */
static int ring_full(const terrain_state *t, int x, int y, int k)
{
    if (k <= 0) {
        return 1;
    }
    for (int dy = -k; dy <= k; dy++) {
        for (int dx = -k; dx <= k; dx++) {
            if ((dx < 0 ? -dx : dx) != k && (dy < 0 ? -dy : dy) != k) {
                continue;
            }
            int nx = x + dx, ny = y + dy;
            if (nx < -1 || ny < -1 || MAP_W(t) < nx || MAP_H(t) < ny) {
                continue;
            }
            if (!(TER(t, off(t, x, y) + dx + dy * GRID_SIDE) & 3)) {
                return 0;
            }
        }
    }
    return 1;
}

static void vegetation_image(terrain_state *t, int o)
{
    /* @bin 0x00486600 @diff terrain_check */
    uint32_t v = TER(t, o);
    int rnd = RND(t, o) & 7;
    if (!(v & 1) || (v & 0x80000)) {
        if ((v & 0x40000) && !(v & 0x80000) && ncount(t, o, 0x40000, 0) == 8) {
            IMG(t, o) = (uint32_t) (rnd + grp(t, VEG(t, o) == 0xFF ? G_0X40000_FULL : G_0X40000));
        }
        return;
    }
    int m = MOIST(t, o) > 100 ? 100 : MOIST(t, o);
    if (VEG(t, o) != 0xFF) {
        IMG(t, o) = (uint32_t) (grp(t, G_TREES) + rnd + (m / 34) * 8);
        return;
    }
    int x = (o - MAP_START_(t)) % GRID_SIDE, y = (o - MAP_START_(t)) / GRID_SIDE, k = 3, found;
    for (;;) {
        if (ring_full(t, x, y, k)) {
            found = k;
            break;
        }
        found = k - 1;
        if (k == 0) {
            break;
        }
        k--;
    }
    int extra;
    if (found < 2) {
        extra = (m / 34) << 4;
    } else {
        extra = 0x30;
        found -= 2;
    }
    IMG(t, o) = (uint32_t) (grp(t, G_TREES_RING) + rnd + found * 8 + extra);
}

/* @bin 0x00475BF0 (x0, y0, x1, y1) clamped to the map */
static void trees_rect(terrain_state *t, int x0, int y0, int x1, int y1)
{
    x0 = x0 < 0 ? 0 : x0;
    y0 = y0 < 0 ? 0 : y0;
    x1 = x1 >= MAP_W(t) ? MAP_W(t) - 1 : x1;
    y1 = y1 >= MAP_H(t) ? MAP_H(t) - 1 : y1;
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            uint32_t v = TER(t, off(t, x, y));
            if (!((v & 1) && !(v & 0x600)) || !inside(t, MAP_X0(t) + x, MAP_Y0(t) + y)) {
                continue;
            }
            TER(t, off(t, x, y)) |= 1; /* @bin 0x00479770 */
            int ya = y - 1 < 0 ? 0 : y - 1, yb = y + 1 > MAP_H(t) - 1 ? MAP_H(t) - 1 : y + 1;
            int xa = x - 1 < 0 ? 0 : x - 1, xb = x + 1 > MAP_W(t) - 1 ? MAP_W(t) - 1 : x + 1;
            for (int yy = ya; yy <= yb; yy++) {
                for (int xx = xa; xx <= xb; xx++) {
                    int p = off(t, xx, yy);
                    if ((TER(t, p) & 1) && !(TER(t, p) & 0x600)) {
                        vegetation_image(t, p);
                        BITS(t, p) &= 0xF0;
                        EDGE(t, p) |= EDGE_DRAW_TILE;
                    }
                }
            }
        }
    }
}

static void pass_trees(terrain_state *t)
{
    trees_rect(t, 0, 0, MAP_W(t) - 1, MAP_H(t) - 1);
}

/* ---- special rocks (@bin 0x004770F0 -> 0x004798E0) ---------------------------------------- */

static void pass_special_rocks(terrain_state *t)
{
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            int o = off(t, x, y);
            if (!((TER(t, o) & 2) && (BITS(t, o) & 0x80)) || !inside(t, MAP_X0(t) + x, MAP_Y0(t) + y)) {
                continue;
            }
            TER(t, o) |= 2;
            BITS(t, o) |= 0x80;
            int32_t base = grp(t, G_SPECIAL_ROCKS);
            int ya = y - 1 < 0 ? 0 : y - 1, yb = y + 1 > MAP_H(t) - 1 ? MAP_H(t) - 1 : y + 1;
            int xa = x - 1 < 0 ? 0 : x - 1, xb = x + 1 > MAP_W(t) - 1 ? MAP_W(t) - 1 : x + 1;
            for (int yy = ya; yy <= yb; yy++) {
                for (int xx = xa; xx <= xb; xx++) {
                    int p = off(t, xx, yy), f[8];
                    if (!((TER(t, p) & 2) && (BITS(t, p) & 0x80))) {
                        continue;
                    }
                    for (int i = 0; i < 8; i++) { /* @bin 0x0047DEE0, 0x00484F20 */
                        int n = p + NEIGHBOURS[i];
                        f[i] = (TER(t, n) & 2) && (BITS(t, n) & 0x80);
                    }
                    int hit = match(t, PATTERN(t, 0x005CBEA8u), 0x11, f, -1, 0);
                    IMG(t, p) = (uint32_t) (hit ? base + M_VARIANT(t) + M_OFFSET(t) : base);
                    BITS(t, p) &= 0xF0;
                    EDGE(t, p) |= EDGE_DRAW_TILE;
                }
            }
        }
    }
}

/* ---- entry/exit signposts (@bin 0x0047D870(0); gate DAT_00605a3c is never set) ------------- */

static int edge_dir(const terrain_state *t, const int16_t pt[2])
{
    if (pt[0] == 0) return 2;
    if (pt[0] == MAP_W(t) - 1) return 6;
    if (pt[1] == 0) return 0;
    return pt[1] == MAP_H(t) - 1 ? 4 : -1;
}

static void pass_signposts(terrain_state *t)
{
    const int16_t *pts[2] = {POINT(t, 0), POINT(t, 1)};
    for (int e = 0; e < 2; e++) {
        int d = edge_dir(t, pts[e]);
        if (d < 0) {
            continue;
        }
        int px = pts[e][0], py = pts[e][1], exclude = off(t, px, py), gx = 0, gy = 0, found = 0;
        for (int k = 1; k < 10 && !found; k++) { /* @bin 0x00482810 */
            int x0 = px - k < 0 ? 0 : px - k, x1 = px + k > MAP_W(t) - 1 ? MAP_W(t) - 1 : px + k;
            int y1 = py + k > MAP_H(t) - 1 ? MAP_H(t) - 1 : py + k;
            for (gy = py - k < 0 ? 0 : py - k; gy <= y1 && !found; gy += found ? 0 : 1) {
                for (gx = x0; gx <= x1; gx++) {
                    int o = off(t, gx, gy);
                    if (o != exclude && TER(t, o) == 0) {
                        found = 1;
                        break;
                    }
                }
            }
        }
        int o = off(t, gx, gy);
        TER(t, o) |= 2;
        d = d <= 7 ? d : d - 8; /* orientation 0 */
        IMG(t, o) = (uint32_t) (d / 2 + (e ? 4 : 0) + grp(t, G_SIGNPOSTS));
        t->stats.signposts_placed++;
    }
}

/* ---- meadow (@bin 0x00477500 -> 0x00479B70) ----------------------------------------------- */

static int is_meadow(uint32_t v)
{
    return (v & 0x800) && !(v & 0x1700) && !(v & 0x40) && (!(v & 8) || (v & 0x10000000u)) && !(v & 0x20);
}

/* x87 trunc((f - 1) * 0.0101... * 3.0) equals (f - 1) * 3 / 99 (0 for f = 0); x87
 * trunc((m - 1) / 96.0 * 12.0) equals (m - 1) >> 3 (0 for m = 0), for every input.
 * @bin 0x00479D1F FMUL [0x0056F588]=1/99, FMUL [0x0056F580]=3.0; 0x00479D55 FDIV [0x0056F570]=96.0,
 * FMUL [0x0056F578]=12.0 @diff terrain_check */
static int meadow_a(int f)
{
    return f == 0 ? 0 : (f - 1) * 3 / 99;
}

static int meadow_b(int m)
{
    return m == 0 ? 0 : (m - 1) >> 3;
}

/* @bin 0x00477500 (x0, y0, x1, y1) clamped to the map */
static void meadow_rect(terrain_state *t, int x0, int y0, int x1, int y1)
{
    x0 = x0 < 0 ? 0 : x0;
    y0 = y0 < 0 ? 0 : y0;
    x1 = x1 >= MAP_W(t) ? MAP_W(t) - 1 : x1;
    y1 = y1 >= MAP_H(t) ? MAP_H(t) - 1 : y1;
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            if (!is_meadow(TER(t, off(t, x, y))) || !inside(t, MAP_X0(t) + x, MAP_Y0(t) + y)) {
                continue;
            }
            TER(t, off(t, x, y)) |= 0x800;
            int ya = y - 1 < 0 ? 0 : y - 1, yb = y + 1 > MAP_H(t) - 1 ? MAP_H(t) - 1 : y + 1;
            int xa = x - 1 < 0 ? 0 : x - 1, xb = x + 1 > MAP_W(t) - 1 ? MAP_W(t) - 1 : x + 1;
            for (int yy = ya; yy <= yb; yy++) {
                for (int xx = xa; xx <= xb; xx++) {
                    int p = off(t, xx, yy);
                    if (!is_meadow(TER(t, p))) {
                        continue;
                    }
                    int a = meadow_a(FERT(t, p));
                    int b = meadow_b(MOIST(t, p) > 100 ? 100 : MOIST(t, p));
                    int group = G_MEADOW, idx = b;
                    if (a == 0) {
                        if (b == 0) {
                            group = G_MEADOW_DRY;
                            idx = RND(t, p) & 7;
                        }
                    } else if (a == 2) {
                        if (b == 0) {
                            group = G_MEADOW_A2B0;
                            idx = RND(t, p) & 7;
                            a = 0;
                        } else if (b == 0xB) {
                            group = G_MEADOW_A2B11;
                            idx = RND(t, p) & 7;
                            a = 0;
                        }
                    }
                    set_image(t, p, (uint32_t) (grp(t, group) + a * 12 + idx));
                }
            }
        }
    }
}

static void pass_meadow(terrain_state *t)
{
    meadow_rect(t, 0, 0, MAP_W(t) - 1, MAP_H(t) - 1);
}

/* ---- flag 0x40000 (@bin 0x004776F0 -> 0x0047AB10) ------------------------------------------ */

static void pass_flag_0x40000(terrain_state *t)
{
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            uint32_t v = TER(t, off(t, x, y));
            if (!((v & 0x40000) && !(v & 8)) || !inside(t, MAP_X0(t) + x, MAP_Y0(t) + y)) {
                continue;
            }
            TER(t, off(t, x, y)) |= 0x40000;
            int ya = y - 1 < 0 ? 0 : y - 1, yb = y + 1 > MAP_H(t) - 1 ? MAP_H(t) - 1 : y + 1;
            int xa = x - 1 < 0 ? 0 : x - 1, xb = x + 1 > MAP_W(t) - 1 ? MAP_W(t) - 1 : x + 1;
            for (int yy = ya; yy <= yb; yy++) {
                for (int xx = xa; xx <= xb; xx++) {
                    int p = off(t, xx, yy), f[8];
                    if (!((TER(t, p) & 0x40000) && !(TER(t, p) & 8))) {
                        continue;
                    }
                    nflags(t, p, 0x40000, 0, f);
                    for (int i = 0; i < 8; i++) {
                        f[i] = !f[i];
                    }
                    match(t, PATTERN(t, 0x005CA658u), 0x30, f, RND(t, p), 0);
                    IMG(t, p) = (uint32_t) (grp(t, G_0X40000) + M_VARIANT(t) + M_OFFSET(t));
                    BITS(t, p) &= 0xF0;
                    EDGE(t, p) |= EDGE_DRAW_TILE;
                    vegetation_image(t, p);
                }
            }
        }
    }
}

/* ---- rubble (@bin 0x00477890 -> 0x0047A120) ---------------------------------------------- */

static void pass_rubble(terrain_state *t)
{
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            int o = off(t, x, y);
            uint32_t v = TER(t, o);
            if ((v & 0x1000) && !(v & 0x700) && !(v & 0x68) && inside(t, MAP_X0(t) + x, MAP_Y0(t) + y)) {
                BITS(t, o) &= 0xF0;
                IMG(t, o) = (uint32_t) ((RND(t, o) & 7) + grp(t, G_RUBBLE));
                EDGE(t, o) |= EDGE_DRAW_TILE;
                TER(t, o) |= 0x1000;
            }
        }
    }
}

/* ---- canals (@bin 0x0047BAE0, 0x0047E870, 0x0047EB60). No canal tile reaches this code in the
 * reference corpus (docs/formats/campaign.md §3), so these rules carry @bin only. ------------- */

#define ROAD_OR_MARK 0x8000040u

/* @bin 0x0047B920 paved road test: never on floodplain; desirability > 4 yes, < 1 no, else bit
 * 0x2000 of the terrain. */
static int road_paved(const terrain_state *t, int o)
{
    if (TER(t, o) & 0x10000) {
        return 0;
    }
    if (DES(t, o) > 4) {
        return 1;
    }
    if (DES(t, o) < 1) {
        return 0;
    }
    return (TER(t, o) >> 13) & 1;
}

/* @bin 0x0047EB60 per-direction canal link: direction table {neighbour, side A, side B (if this
 * tile is a road), side C, side D (if not)} for N, E, S, W. */
static int canal_link(const terrain_state *t, int o, int dir)
{
    static const int D[4][5] = {
        {-228, -1, 1, -229, -227}, {1, -228, 228, -227, 229}, {228, -1, 1, 227, 229}, {-1, -228, 228, -229, 227},
    };
    const state_u32 *T = (const state_u32 *) state_ptr(t->g, 0x00AF598Cu, 4u * GRID_SIZE);
    uint32_t n = T[o + D[dir][0]];
    if (!(n & 0x100)) {
        return 0;
    }
    int road_n = (n & ROAD_OR_MARK) != 0;
    if (!((T[o] >> 6) & 1)) {
        return !road_n || (T[o + D[dir][3]] & ROAD_OR_MARK) || (T[o + D[dir][4]] & ROAD_OR_MARK);
    }
    if (road_n) {
        return 0;
    }
    return (T[o + D[dir][1]] & ROAD_OR_MARK) || (T[o + D[dir][2]] & ROAD_OR_MARK);
}

/* @bin 0x0047E870 canal neighbour flags (orthogonal only), then Water Lifts (building type 0x5A)
 * next to the tile: north / south with orientation bit 0 clear, east / west with it set, unless
 * the tile and the opposite neighbour both have a road (0x40). */
static void canal_flags(const terrain_state *t, int o, int f[8])
{
    for (int i = 0; i < 8; i++) {
        f[i] = 0;
    }
    if (TER(t, o) & 0x10000) {
        for (int i = 0; i < 8; i += 2) {
            f[i] = (TER(t, o + NEIGHBOURS[i]) & 4) != 0;
        }
    }
    for (int dir = 0; dir < 4; dir++) {
        if (canal_link(t, o, dir)) {
            f[2 * dir] = 1;
        }
    }
    static const struct { int d, opposite, bit; } L[4] = {{-228, 228, 0}, {1, -1, 1}, {228, -228, 0}, {-1, 1, 1}};
    for (int dir = 0; dir < 4; dir++) {
        int n = o + L[dir].d;
        if (!(TER(t, n) & 8)) {
            continue;
        }
        uint32_t b = 0x007999D8u + 0x108u * (uint32_t) S16(t->g, 0x00B4E6FCu + 2u * (uint32_t) n);
        if (S16(t->g, b + 0x10) == 0x5A && !(TER(t, o + L[dir].opposite) & TER(t, o) & 0x40) &&
            (U8(t->g, b + 0xAA) & 1) == L[dir].bit) {
            f[2 * dir] = 1;
        }
    }
}

static void pass_canals(terrain_state *t)
{
    /* @bin 0x0047BAE0(0, 0, w-1, h-1, 0): the rectangle grows by one tile, clipped to the map. */
    int32_t base = grp(t, G_CANALS);
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            int o = off(t, x, y), f[8];
            uint32_t v = TER(t, o);
            if (!((v & 0x100) && !(v & 0xC) && AQ(t, o) < 0x10)) {
                continue;
            }
            int rel = (int) IMG(t, o) - base;
            int shift = (rel >= 0 && rel < 0x30) ? 0 : 0x30;
            canal_flags(t, o, f);
            match(t, PATTERN(t, 0x005CC678u), 0x10, f, -1, 0);
            if (v & 0x40) { /* road across the canal */
                BITS(t, o) &= 0x7F;
                if (M_EXTRA(t) == 0) {
                    M_OFFSET(t) = !(TER(t, o - 228) & 0x40);
                }
                M_OFFSET(t) += road_paved(t, o) ? 0x2A : 0xF;
            }
            int32_t img = base + M_VARIANT(t) + M_OFFSET(t) + shift;
            if (v & 0x10000) {
                img += 0x15; /* @bin 0x0051C620 */
            }
            set_image(t, o, (uint32_t) img);
            AQ(t, o) = (uint8_t) M_EXTRA(t);
        }
    }
    S32(t->g, 0x00BCA328u) = 0; /* @bin 0x0047BAE0 */
}

/* ---- roads and plaza (@bin 0x0047B570 -> 0x0047B590 -> 0x0047B670; 0x00475720) ------------ */

/* @bin 0x0047E5C0 a gatehouse-type building (types 0x3A, 0xAA, 0xAB, 200, 0xC9, 0xCA) north or
 * south with i16 +0x12 == 1, east or west with +0x12 == 2, counts as a road neighbour */
static void gatehouse_flags(terrain_state *t, int o, int f[8])
{
    static const struct { int d, flag, side; } N[4] = {{-228, 0, 1}, {1, 2, 2}, {228, 4, 1}, {-1, 6, 2}};
    for (int k = 0; k < 4; k++) {
        int n = o + N[k].d;
        if (!(TER(t, n) & 0x8000)) {
            continue;
        }
        uint32_t b = 0x007999D8u + 0x108u * (uint32_t) S16(t->g, 0x00B4E6FCu + 2u * (uint32_t) n);
        int16_t type = S16(t->g, b + 0x10);
        if ((type == 0x3A || type == 0xAA || type == 0xAB || type == 200 || type == 0xC9 || type == 0xCA) &&
            S16(t->g, b + 0x12) == N[k].side) {
            f[N[k].flag] = 1;
        }
    }
    scratch_store(t, f);
}

/* @bin 0x00484F20 every scratch flag := (flag != 0) */
static void scratch_normalize(terrain_state *t)
{
    for (int i = 0; i < 8; i++) {
        SCRATCH(t, i) = SCRATCH(t, i) != 0;
    }
}

/* @bin 0x0051C5F0 a floodplain image (relative to group 528) with a road: 0x34 -> 0x56, 0x30 ->
 * 0x54, 0x36 -> 0x57, 0x32 -> 0x55 */
static int floodplain_road_image(int i)
{
    return i == 0x34 ? 0x56 : i == 0x30 ? 0x54 : i == 0x36 ? 0x57 : i == 0x32 ? 0x55 : i;
}

/* @bin 0x0047B670 one road tile (flags already in the scratch) */
static void road_tile(terrain_state *t, int o, int f[8])
{
    if (S16(t->g, 0x00B4E6FCu + 2u * (uint32_t) o) != 0 || (BITS(t, o) & 0x80)) {
        return;
    }
    uint32_t v = TER(t, o);
    int32_t img;
    if (S32(t->g, 0x00978D74u) == 1) { /* every road paved */
        match(t, PATTERN(t, 0x005CC378u), 0x30, f, -1, 0);
        scratch_normalize(t);
        img = grp(t, G_ROADS_PAVED) + M_VARIANT(t) + M_OFFSET(t);
    } else if (v & 0x100) { /* road on a canal (@bin 0x0047E870 canal flags) */
        int rel = (int) IMG(t, o) - grp(t, G_CANALS);
        int shift = rel < 0x30 ? 0 : 0x30;
        int cf[8];
        canal_flags(t, o, cf);
        match(t, PATTERN(t, 0x005CC678u), 0x10, cf, -1, 0);
        if (M_EXTRA(t) == 0) {
            M_OFFSET(t) = (TER(t, o - 1) & 0x100) ? 0 : ((TER(t, o + 1) & 0x100) ? 0 : 1);
        }
        if (road_paved(t, o)) {
            img = grp(t, G_CANALS) + M_OFFSET(t) + 0x2A + shift;
        } else {
            img = grp(t, G_CANALS) + M_OFFSET(t) + 0xF + shift;
            if (v & 0x10000) {
                img += 0x15; /* @bin 0x0051C620 */
            }
        }
    } else if (!(v & 0x10000) && !fp_adjacent_only(t, o)) {
        if (!road_paved(t, o)) {
            match(t, PATTERN(t, 0x005CC268u), 0x11, f, -1, 0);
            scratch_normalize(t);
            img = grp(t, G_ROADS) + M_VARIANT(t) + M_OFFSET(t);
        } else {
            match(t, PATTERN(t, 0x005CC378u), 0x30, f, -1, 0);
            scratch_normalize(t);
            img = grp(t, G_ROADS_PAVED) + M_VARIANT(t) + M_OFFSET(t);
        }
    } else {
        int i3 = (int) IMG(t, o) - grp(t, G_FLOODPLAIN);
        int i1 = floodplain_road_image(i3);
        if (i3 == i1 && (i3 < 0x54 || 0x57 < i3)) {
            match(t, PATTERN(t, 0x005CC268u), 0x11, f, -1, 0);
            scratch_normalize(t);
            img = M_VARIANT(t) + grp(t, G_ROADS_FP) + M_OFFSET(t);
        } else {
            img = i1 + grp(t, G_FLOODPLAIN);
        }
    }
    set_image(t, o, (uint32_t) img);
}

/* @bin 0x0047B570 -> 0x0047B590 every tile with terrain 0x40 and not 0xC: flags (0x0047DB00 with
 * 0x8000040), gatehouses (0x0047E5C0), then the tile (0x0047B670) */
static void pass_roads(terrain_state *t)
{
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            int o = off(t, x, y), f[8];
            uint32_t v = TER(t, o);
            if (!((v & 0x40) && !(v & 0xC))) {
                continue;
            }
            nflags(t, o, 0x8000040u, 0, f);
            gatehouse_flags(t, o, f);
            road_tile(t, o, f);
        }
    }
}

static int plaza_2x2(const terrain_state *t, int o)
{
    /* @bin 0x00475670 E, S, SE must be empty plaza tiles */
    static const int D[3] = {1, GRID_SIDE, GRID_SIDE + 1};
    int u = 0;
    for (int i = 0; i < 3; i++) {
        uint32_t v = TER(t, o + D[i]);
        u += (v & 0x40) ? 1 : 0;
        u -= (BITS(t, o + D[i]) & 0x80) ? 0 : 1;
        u -= ((v & 4) ? 1 : 0) + ((v & 8) ? 1 : 0) + ((v & 0x100) ? 1 : 0);
        u -= IMG(t, o + D[i]) ? 1 : 0;
    }
    return u == 3;
}

static void pass_plaza(terrain_state *t)
{
    int32_t base = grp(t, G_PLAZA);
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            int o = off(t, x, y);
            if ((TER(t, o) & 0x40) && (BITS(t, o) & 0x80) &&
                (BGRID(t, o) == 0 || BUILDING_TYPE(t, BGRID(t, o)) != 0x8A)) { /* @bin 0x00475720 */
                set_image(t, o, 0);
            }
        }
    }
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            int o = off(t, x, y);
            if (!((TER(t, o) & 0x40) && IMG(t, o) == 0 && (BITS(t, o) & 0x80))) {
                continue;
            }
            if (plaza_2x2(t, o)) {
                place(t, x, y, 2, (uint32_t) (base + ((RND(t, o) & 1) ? 7 : 6)), 0x40);
            } else {
                IMG(t, o) = (uint32_t) (base + ((x & 1) && (y & 1) ? 1 : (!(x & 1) && !(y & 1)) ? 0 : 2));
                EDGE(t, o) |= EDGE_DRAW_TILE;
            }
        }
    }
}

/* ---- walls (@bin 0x0047B1A0, 0x0047E000, 0x0047E750) -------------------------------------- */

static void pass_walls(terrain_state *t)
{
    int32_t base = grp(t, G_WALLS);
    for (int y = 0; y < MAP_H(t); y++) {
        for (int x = 0; x < MAP_W(t); x++) {
            int o = off(t, x, y), f[8];
            if (!((TER(t, o) & 0x4000) && !(TER(t, o) & 8))) {
                continue;
            }
            nflags(t, o, 0x4000, 0, f);
            for (int i = 0; i < 8; i++) {
                f[i] = !f[i];
            }
            match(t, PATTERN(t, 0x005CB9D8u), 0x30, f, -1, 0);
            BITS(t, o) &= 0xF0;
            EDGE(t, o) |= EDGE_DRAW_TILE;
            IMG(t, o) = (uint32_t) (base + M_VARIANT(t) + M_OFFSET(t));
            int b[8] = {0}, any = 0;
            for (int i = 0; i < 8; i += 2) {
                b[i] = (TER(t, o + NEIGHBOURS[i]) & 0x8000) != 0;
                any |= b[i];
            }
            if (any) {
                for (int i = 0; i < 8; i++) {
                    b[i] += (int) ((TER(t, o + NEIGHBOURS[i]) >> 14) & 1);
                }
                if (match(t, PATTERN(t, 0x005CBCD8u), 0x0F, b, -1, 0)) {
                    IMG(t, o) = (uint32_t) (base + M_VARIANT(t) + M_OFFSET(t));
                }
            }
        }
    }
}

/* ---- sequence ---------------------------------------------------------------------------- */

/* @bin 0x004DEA50 sub-passes in order (all ported). @diff terrain_check except canals (no data) */
void terrain_rebuild(terrain_state *t)
{
    pass_special_rocks(t); /* 0x004770F0 */
    pass_rocks(t);         /* 0x00475DC0 */
    pass_trees(t);         /* 0x00475BF0 */
    pass_flag_0x20(t);     /* 0x00476530 */
    pass_signposts(t);     /* 0x0047D870 */
    pass_land(t);          /* 0x004767D0 */
    pass_floodplain(t);    /* 0x004777C0 */
    pass_meadow(t);        /* 0x00477500 */
    pass_flag_0x40000(t);  /* 0x004776F0 */
    pass_dunes(t);         /* 0x00477210 */
    pass_floodplain(t);    /* 0x004777C0 */
    pass_water(t);         /* 0x00475CC0 */
    pass_rubble(t);        /* 0x00477890 */
    pass_roads(t);         /* 0x0047B570 */
    pass_plaza(t);         /* 0x00475720 */
    pass_walls(t);         /* 0x0047B1A0 */
    pass_canals(t);        /* 0x0047BAE0 */
}

void terrain_scenario_start(terrain_state *t)
{
    /* @bin 0x004D52A0 FUN_0040f8f0(image grid, 0x32C40, 0) */
    memset(state_ptr(t->g, 0x00B7482Cu, 4u * GRID_SIZE), 0, 4u * GRID_SIZE);
    terrain_rebuild(t);
    terrain_floodplain_build(t, 0, 0, MAP_W(t) - 1, MAP_H(t) - 1);
}

void terrain_store_groups(game_state *g, const image_groups *groups)
{
    for (int i = 0; i < groups->group_count; i++) {
        S32(g, 0x00F55BB4u + 8u * (uint32_t) i) = groups->group_first[i];
    }
}

enum { IN_IMAGE, IN_EDGE, IN_TERRAIN, IN_BITFIELDS, IN_RANDOM, IN_SCENARIO, IN_FERTILITY, IN_VEGETATION, IN_MOISTURE, IN_COUNT };

/* the variables the chunks are loaded into (@bin 0x005E17D8 map schema, 0x005E09A0 save schema) */
static const uint32_t IN_ADDR[IN_COUNT] = {
    0x00B7482C, 0x00B67D1C, 0x00AF598C, 0x00B285CC, 0x00ACF85C, 0x007848D8, 0x00A5D4CC, 0x00A43EAC, 0x00A509BC,
};
static const uint32_t IN_SIZE[IN_COUNT] = {
    4 * GRID_SIZE, GRID_SIZE, 4 * GRID_SIZE, GRID_SIZE, GRID_SIZE, 0x394, GRID_SIZE, GRID_SIZE, GRID_SIZE,
};

static int init_from(terrain_state *t, game_state *g, const map_file *m, const int index[IN_COUNT],
                     const image_groups *groups);

int terrain_init(terrain_state *t, game_state *g, const map_file *m, const image_groups *groups)
{
    static const int MAP[IN_COUNT] = {
        MAP_CHUNK_IMAGE, MAP_CHUNK_EDGE, MAP_CHUNK_TERRAIN, MAP_CHUNK_BITFIELDS, MAP_CHUNK_RANDOM,
        MAP_CHUNK_SCENARIO, MAP_CHUNK_FERTILITY, MAP_CHUNK_VEGETATION, MAP_CHUNK_MOISTURE,
    };
    return init_from(t, g, m, MAP, groups);
}

int terrain_init_campaign(terrain_state *t, game_state *g, const map_file *m, const image_groups *groups)
{
    static const int CAMPAIGN[IN_COUNT] = {
        CAMPAIGN_CHUNK_IMAGE, CAMPAIGN_CHUNK_EDGE, CAMPAIGN_CHUNK_TERRAIN, CAMPAIGN_CHUNK_BITFIELDS,
        CAMPAIGN_CHUNK_RANDOM, CAMPAIGN_CHUNK_SCENARIO, CAMPAIGN_CHUNK_FERTILITY, CAMPAIGN_CHUNK_VEGETATION,
        CAMPAIGN_CHUNK_MOISTURE,
    };
    if (init_from(t, g, m, CAMPAIGN, groups) != 0) {
        return -1;
    }
    /* runtime grids a saved game carries (docs/formats/save.md §3) */
    static const int OPT[2] = {CAMPAIGN_CHUNK_AQUEDUCT, CAMPAIGN_CHUNK_DESIRABILITY};
    static const uint32_t OPT_ADDR[2] = {0x00AE8E7C, 0x00AC2D4C};
    for (int i = 0; i < 2; i++) {
        if ((uint32_t) OPT[i] >= m->count || !m->chunk[OPT[i]] || m->size[OPT[i]] < GRID_SIZE) {
            return -1;
        }
        state_load_section(g, OPT_ADDR[i], m->chunk[OPT[i]], GRID_SIZE);
    }
    return 0;
}

static int init_from(terrain_state *t, game_state *g, const map_file *m, const int index[IN_COUNT],
                     const image_groups *groups)
{
    memset(t, 0, sizeof(*t));
    t->g = g;
    for (int i = 0; i < IN_COUNT; i++) {
        if ((uint32_t) index[i] >= m->count || !m->chunk[index[i]] || m->size[index[i]] < IN_SIZE[i]) {
            return -1;
        }
        state_load_section(g, IN_ADDR[i], m->chunk[index[i]], i == IN_SCENARIO ? m->size[index[i]] : IN_SIZE[i]);
    }
    /* scenario_info: width, height, start offset at +44, +48, +52 (map.md §5: consistent on 33/33
     * maps, and the row ranges reproduce every map's drawn area) @diff terrain_check */
    if (S32(g, 0x00784904u) <= 0 || S32(g, 0x00784908u) <= 0 || S32(g, 0x00784904u) > GRID_SIDE - 2 ||
        S32(g, 0x00784908u) > GRID_SIDE - 2) {
        return -1;
    }
    scenario_start_map(g); /* @bin 0x004D52A0 globals, 0x0046F7B0 rows (also clears the image grid) */
    state_load_section(g, IN_ADDR[IN_IMAGE], m->chunk[index[IN_IMAGE]], IN_SIZE[IN_IMAGE]);
    terrain_store_groups(g, groups);
    /* the pattern tables' counters start from the exe's values (core/map/terrain_tables.c holds
     * the same bytes, generated from the same addresses) */
    static const struct { uint32_t addr; const void *data; size_t size; } P[] = {
        {0x005CA658u, terrain_pattern_shore, sizeof(terrain_pattern_shore)},
        {0x005CBFB8u, terrain_pattern_deep, sizeof(terrain_pattern_deep)},
        {0x005CAC58u, terrain_pattern_floodplain, sizeof(terrain_pattern_floodplain)},
        {0x005CBEA8u, terrain_pattern_special_rocks, sizeof(terrain_pattern_special_rocks)},
        {0x005CC268u, terrain_pattern_roads, sizeof(terrain_pattern_roads)},
        {0x005CB9D8u, terrain_pattern_walls, sizeof(terrain_pattern_walls)},
        {0x005CBCD8u, terrain_pattern_walls_building, sizeof(terrain_pattern_walls_building)},
        {0x005CC678u, terrain_pattern_canals, sizeof(terrain_pattern_canals)},
    };
    for (size_t i = 0; i < sizeof(P) / sizeof(P[0]); i++) {
        state_load_section(g, P[i].addr, (const uint8_t *) P[i].data, P[i].size);
    }
    return 0;
}

/* One sub-pass of 0x004DEA50 by its original address, over the whole map (tests). Returns 0, or
 * -1 for an address that is not a sub-pass. */
int terrain_subpass(terrain_state *t, uint32_t address)
{
    switch (address) {
    case 0x004770F0u: pass_special_rocks(t); return 0;
    case 0x00475DC0u: pass_rocks(t); return 0;
    case 0x00475BF0u: pass_trees(t); return 0;
    case 0x00476530u: pass_flag_0x20(t); return 0;
    case 0x0047D870u: pass_signposts(t); return 0;
    case 0x004767D0u: pass_land(t); return 0;
    case 0x004777C0u: pass_floodplain(t); return 0;
    case 0x00477500u: pass_meadow(t); return 0;
    case 0x004776F0u: pass_flag_0x40000(t); return 0;
    case 0x00477210u: pass_dunes(t); return 0;
    case 0x00475CC0u: pass_water(t); return 0;
    case 0x00477890u: pass_rubble(t); return 0;
    case 0x0047B570u: pass_roads(t); return 0;
    case 0x00475720u: pass_plaza(t); return 0;
    case 0x0047B1A0u: pass_walls(t); return 0;
    case 0x0047BAE0u: pass_canals(t); return 0;
    default: return -1;
    }
}
