/* Imhotep - what walkers do to the buildings around their tile (package figa): the area scans of
 * 0x00482C10 .. 0x00484A90, called by figure_serve_tile (0x004BAA10).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Building record: docs/formats/save.md §5 (chunk 32). */
#include "figure/actions_a.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


typedef struct {
    int32_t count;
    uint32_t field; /* building field written */
    int32_t value;
} scan_ctx;

typedef void (*scan_fn)(game_state *st, int32_t tile, scan_ctx *c);

/* @bin 0x004833C0 .. the scan of the tiles within `r` of (x, y), clipped to the map. Most scans
 * (row_globals) set the row's final column and grid offset before going through it; the others
 * (0x00482C10, 0x00483000, 0x0042FF20) step the globals tile by tile around their calls. */
static void area_scan(game_state *st, int32_t x, int32_t y, int32_t r, int row_globals, scan_fn fn, scan_ctx *c)
{
    int32_t ymax = y + r, ymin = y - r, xmax = x + r, xmin = x - r;
    if (xmin < 0) {
        xmin = 0;
    }
    if (ymin < 0) {
        ymin = 0;
    }
    if (xmax >= S32(st, G_MAP_WIDTH)) {
        xmax = S32(st, G_MAP_WIDTH) - 1;
    }
    if (ymax >= S32(st, G_MAP_HEIGHT)) {
        ymax = S32(st, G_MAP_HEIGHT) - 1;
    }
    S32(st, G_ROAD_ACCESS_Y) = ymin;
    int32_t cursor = (int32_t) (U32(st, G_MAP_START) + 228u * (uint32_t) ymin + (uint32_t) xmin);
    int32_t advance = xmin - xmax + 0xE3;
    S32(st, G_TILE_CURSOR) = cursor;
    if (ymin > ymax) {
        return;
    }
    if (row_globals) {
        int32_t rows = ymax - ymin + 1;
        S32(st, G_ROAD_ACCESS_Y) = ymin + rows;
        for (; rows > 0; rows--) {
            S32(st, G_ROAD_ACCESS_X) = xmin;
            if (xmin <= xmax) {
                int32_t cols = xmax - xmin + 1;
                int32_t tile = cursor;
                cursor += cols;
                S32(st, G_ROAD_ACCESS_X) = xmin + cols;
                for (int32_t i = 0; i < cols; i++) {
                    fn(st, tile + i, c);
                }
            }
            cursor += advance;
            S32(st, G_TILE_CURSOR) = cursor;
        }
        return;
    }
    for (;;) {
        S32(st, G_ROAD_ACCESS_X) = xmin;
        if (xmin <= xmax) {
            do {
                fn(st, S32(st, G_TILE_CURSOR), c);
                S32(st, G_ROAD_ACCESS_X) += 1;
                S32(st, G_TILE_CURSOR) += 1;
            } while (S32(st, G_ROAD_ACCESS_X) <= xmax);
        }
        S32(st, G_ROAD_ACCESS_Y) += 1;
        S32(st, G_TILE_CURSOR) += advance;
        if (S32(st, G_ROAD_ACCESS_Y) > ymax) {
            return;
        }
    }
}

static int16_t building_at(game_state *st, int32_t tile)
{
    return S16(st, G_BUILDING_GRID + 2u * (uint32_t) tile);
}

/* a house (+0x05) with people (+0x1C) */
static int inhabited(game_state *st, int32_t b)
{
    uint32_t a = BUILDING_ADDR(b);
    return b > 0 && U8(st, a + BLD_HOUSE_SIZE) != 0 && S16(st, a + BLD_POPULATION) > 0;
}

/* a building of types 0x0A .. 0x1D in use */
static int housing_block(game_state *st, int32_t b)
{
    uint32_t a = BUILDING_ADDR(b);
    return b != 0 && S16(st, a + BLD_TYPE) >= 0x0A && S16(st, a + BLD_TYPE) <= 0x1D && U8(st, a + BLD_STATE) != 0;
}

static void serve_house(game_state *st, int32_t tile, scan_ctx *c)
{
    int16_t b = building_at(st, tile);
    if (inhabited(st, b)) {
        U8(st, BUILDING_ADDR(b) + c->field) = (uint8_t) c->value;
        c->count++;
    }
}

static void count_house(game_state *st, int32_t tile, scan_ctx *c)
{
    if (inhabited(st, building_at(st, tile))) {
        c->count++;
    }
}

int32_t serve_houses(game_state *st, int32_t x, int32_t y, uint32_t field)
{
    /* @bin 0x004833C0 (and the same code at 0x004834D0, 0x004835E0, 0x004836F0, 0x00483800,
     * 0x00484120, 0x00484230, 0x00484340, 0x00484450, 0x00484560, 0x00484670, 0x00484780,
     * 0x00483E90) the houses within 2 tiles get 0x60 in their service counter `field`; returns
     * how many */
    scan_ctx c = {0, field, 0x60};
    area_scan(st, x, y, 2, 1, serve_house, &c);
    return c.count;
}

int32_t count_houses(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00484990 the inhabited houses within 2 tiles */
    scan_ctx c = {0, 0, 0};
    area_scan(st, x, y, 2, 1, count_house, &c);
    return c.count;
}

static void serve_temple(game_state *st, int32_t tile, scan_ctx *c)
{
    int16_t b = building_at(st, tile);
    if (inhabited(st, b)) {
        c->count++;
        if (c->value >= 0 && c->value <= 4) {
            U8(st, BUILDING_ADDR(b) + BLD_ACCESS_PRIEST + (uint32_t) c->value) = 0x60;
        }
    }
}

int32_t serve_houses_god(game_state *st, int32_t x, int32_t y, int32_t god)
{
    /* @bin 0x00483FB0 the houses within 2 tiles: +0x79 + god (0 .. 4) set to 0x60; returns how
     * many houses */
    scan_ctx c = {0, 0, god};
    area_scan(st, x, y, 2, 1, serve_temple, &c);
    return c.count;
}

static void serve_bazaar(game_state *st, int32_t tile, scan_ctx *c)
{
    int16_t b = building_at(st, tile);
    if (!inhabited(st, b)) {
        return;
    }
    uint32_t a = BUILDING_ADDR(b);
    int32_t v = S32(st, G_HOUSE_TAX_VALUE + 0x58u * (uint32_t) S16(st, a + BLD_SUBTYPE));
    if (v > S32(st, G_SERVICE_TOP_HOUSES)) {
        S32(st, G_SERVICE_TOP_HOUSES) = v;
    }
    U8(st, a + BLD_TAX_COVERAGE) = 0x32;
    c->count++;
}

int32_t serve_houses_4e(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00484A90 the houses within 2 tiles: +0x4E 0x32; 0x009AB9E8 the largest of their
     * level's value (table 0x0070CE18, 0x58 bytes per house level +0x12) */
    S32(st, G_SERVICE_TOP_HOUSES) = 0;
    scan_ctx c = {0, 0, 0};
    area_scan(st, x, y, 2, 1, serve_bazaar, &c);
    return c.count;
}

static void clear_ferry(game_state *st, int32_t tile, scan_ctx *c)
{
    (void) c;
    int16_t b = building_at(st, tile);
    if (b > 0) {
        int16_t t = S16(st, BUILDING_ADDR(b) + BLD_TYPE);
        if (t == 0x58 || t == 0x59) {
            U8(st, BUILDING_ADDR(b) + BLD_SENTIMENT) = 0;
        }
    }
}

int32_t clear_buildings_58(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00484890 the buildings of types 0x58, 0x59 within 4 tiles: +0xC2 cleared; returns 1 */
    scan_ctx c = {0, 0, 0};
    area_scan(st, x, y, 4, 1, clear_ferry, &c);
    return 1;
}

static void lower_dword(game_state *st, int32_t tile, scan_ctx *c)
{
    int16_t b = building_at(st, tile);
    if (housing_block(st, b)) {
        uint32_t a = BUILDING_ADDR(b) + c->field;
        S32(st, a) -= c->value;
        if (S32(st, a) < 0) {
            S32(st, a) = 0;
        }
    }
}

int32_t lower_houses(game_state *st, int32_t x, int32_t y, int32_t amount, uint32_t field)
{
    /* @bin 0x00482DD0 (+0xE8), 0x00482EF0 (+0xFC) the houses (types 0x0A .. 0x1D) within 2 tiles:
     * the dword `field` lowered by `amount`, not below 0; returns 0 */
    scan_ctx c = {0, field, amount};
    area_scan(st, x, y, 2, 1, lower_dword, &c);
    return 0;
}

static void clear_87(game_state *st, int32_t tile, scan_ctx *c)
{
    (void) c;
    int16_t b = building_at(st, tile);
    if (housing_block(st, b)) {
        U8(st, BUILDING_ADDR(b) + BLD_NO_ROOM) = 0;
    }
}

int32_t clear_houses_87(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00483160 the houses (types 0x0A .. 0x1D) within 2 tiles: +0x87 cleared */
    scan_ctx c = {0, 0, 0};
    area_scan(st, x, y, 2, 1, clear_87, &c);
    return 0;
}

static void lower_e4(game_state *st, int32_t tile, scan_ctx *c)
{
    int16_t b = building_at(st, tile);
    if (b == 0) {
        return;
    }
    uint32_t a = BUILDING_ADDR(b);
    int32_t v = S32(st, a + BLD_CRIME_RISK);
    if (v > S32(st, G_SERVICE_TOP)) {
        S32(st, G_SERVICE_TOP) = v;
    }
    v -= c->value;
    S32(st, a + BLD_CRIME_RISK) = v;
    int32_t floor = S32(st, G_HOUSE_LEVEL_U14 + 0x58u * (uint32_t) S16(st, a + BLD_SUBTYPE));
    if (v < floor) {
        S32(st, a + BLD_CRIME_RISK) = floor;
    }
    if (S32(st, a + BLD_CRIME_RISK) > 1000) {
        S32(st, a + BLD_CRIME_RISK) = 1000;
    }
}

void lower_buildings_e4(game_state *st, int32_t x, int32_t y, int32_t amount)
{
    /* @bin 0x00483260 the buildings within 2 tiles: +0xE4 lowered by `amount`, kept within the
     * floor of their level (table 0x0070CE0C) and 1000; 0x009AB9DC the highest before */
    S32(st, G_SERVICE_TOP) = 0;
    scan_ctx c = {0, 0, amount};
    area_scan(st, x, y, 2, 1, lower_e4, &c);
}

static void lower_main_part(game_state *st, int32_t tile, scan_ctx *c)
{
    int16_t b = building_at(st, tile);
    if (c->field == 0x48 ? b == 0 : b <= 0) {
        return;
    }
    int32_t m = GAME_CALL(st, FN_BUILDING_MAIN_PART, b);
    uint32_t a = BUILDING_ADDR(m) + c->field;
    int16_t v = S16(st, a);
    uint32_t top = c->field == 0x48 ? G_SERVICE_TOP_48 : G_SERVICE_TOP_46;
    if (v > S32(st, top)) {
        S32(st, top) = v;
    }
    v = (int16_t) (v - c->value);
    S16(st, a) = v;
    if (v < 0) {
        S16(st, a) = 0;
    }
    /* 0x00483000 checks the main part's id; 0x00482C10 reads the record either way */
    uint32_t ma = BUILDING_ADDR(m);
    if ((c->field == 0x48 ? m > 0 : 1) && U8(st, ma + BLD_HOUSE_SIZE) != 0 && S16(st, ma + BLD_POPULATION) > 0) {
        c->count++;
    }
}

int32_t lower_main_parts(game_state *st, int32_t x, int32_t y, uint32_t field)
{
    /* @bin 0x00482C10 (+0x46 by 0x00706B70, 0x009AB9E0), 0x00483000 (+0x48 by 0x007078E0,
     * 0x009AB9E4) the buildings within 2 tiles (their main part 0x00475B50): the word `field`
     * lowered, not below 0, the global the highest before (from 1000); returns the inhabited
     * houses */
    int32_t amount = S32(st, field == 0x48 ? G_U7078E0 : G_U706B70);
    S32(st, field == 0x48 ? G_SERVICE_TOP_48 : G_SERVICE_TOP_46) = 1000;
    scan_ctx c = {0, field, amount};
    area_scan(st, x, y, 2, 0, lower_main_part, &c);
    return c.count;
}

static void inspect(game_state *st, int32_t tile, scan_ctx *c)
{
    (void) c;
    int16_t b = building_at(st, tile);
    if (b == 0) {
        return;
    }
    uint32_t a = BUILDING_ADDR(b);
    if (U8(st, a + BLD_STATE) != 0 && S16(st, a + BLD_TYPE) >= 0x0A && S16(st, a + BLD_TYPE) <= 0x1D && S16(st, a + BLD_POPULATION) != 0 &&
        U8(st, a + BLD_NO_ROOM) == 0) {
        GAME_CALL(st, FN_HOUSE_EMPTY, b);
    }
}

void inspect_houses(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x0042FF20 the inhabited houses within 2 tiles with +0x87 clear: 0x0042FB30 */
    scan_ctx c = {0, 0, 0};
    area_scan(st, x, y, 2, 0, inspect, &c);
}

int32_t serve_houses_e90(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x00483E90 as 0x004833C0 on +0x77, nothing while 0x0078D7B0 is positive */
    if (S32(st, G_BLOOD_RIVER_LEFT) > 0) {
        return 0;
    }
    return serve_houses(st, x, y, 0x77);
}

static int32_t run_serve_004833C0(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x6Cu);
}

static int32_t run_serve_004834D0(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x6Du);
}

static int32_t run_serve_004835E0(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x6Fu);
}

static int32_t run_serve_004836F0(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x71u);
}

static int32_t run_serve_00483800(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x86u);
}

static int32_t run_serve_00484120(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x72u);
}

static int32_t run_serve_00484230(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x73u);
}

static int32_t run_serve_00484340(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x75u);
}

static int32_t run_serve_00484450(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x6Eu);
}

static int32_t run_serve_00484560(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x76u);
}

static int32_t run_serve_00484780(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x78u);
}

static int32_t run_serve_00483E90(game_state *st, const int32_t *a)
{
    return serve_houses_e90(st, a[0], a[1]);
}

static int32_t run_serve_00484670(game_state *st, const int32_t *a)
{
    return serve_houses(st, a[0], a[1], 0x70u);
}

static int32_t run_count_houses(game_state *st, const int32_t *a)
{
    return count_houses(st, a[0], a[1]);
}

static int32_t run_serve_god(game_state *st, const int32_t *a)
{
    return serve_houses_god(st, a[0], a[1], a[2]);
}

static int32_t run_serve_4e(game_state *st, const int32_t *a)
{
    return serve_houses_4e(st, a[0], a[1]);
}

static int32_t run_clear_58(game_state *st, const int32_t *a)
{
    return clear_buildings_58(st, a[0], a[1]);
}

static int32_t run_lower_e8(game_state *st, const int32_t *a)
{
    return lower_houses(st, a[0], a[1], a[2], 0xE8u);
}

static int32_t run_lower_fc(game_state *st, const int32_t *a)
{
    return lower_houses(st, a[0], a[1], a[2], 0xFCu);
}

static int32_t run_clear_87(game_state *st, const int32_t *a)
{
    return clear_houses_87(st, a[0], a[1]);
}

static int32_t run_lower_e4(game_state *st, const int32_t *a)
{
    lower_buildings_e4(st, a[0], a[1], a[2]);
    return 0;
}

static int32_t run_lower_46(game_state *st, const int32_t *a)
{
    return lower_main_parts(st, a[0], a[1], 0x46u);
}

static int32_t run_lower_48(game_state *st, const int32_t *a)
{
    return lower_main_parts(st, a[0], a[1], 0x48u);
}

static int32_t run_inspect(game_state *st, const int32_t *a)
{
    inspect_houses(st, a[0], a[1]);
    return 0;
}

const ported_function figa_service_ports[] = {
    {0x004833C0u, "serve_houses_6c", 2, 32, 0, run_serve_004833C0, 0},
    {0x004834D0u, "serve_houses_6d", 2, 32, 0, run_serve_004834D0, 0},
    {0x004835E0u, "serve_houses_6f", 2, 32, 0, run_serve_004835E0, 0},
    {0x004836F0u, "serve_houses_71", 2, 32, 0, run_serve_004836F0, 0},
    {0x00483800u, "serve_houses_86", 2, 32, 0, run_serve_00483800, 0},
    {0x00484120u, "serve_houses_72", 2, 32, 0, run_serve_00484120, 0},
    {0x00484230u, "serve_houses_73", 2, 32, 0, run_serve_00484230, 0},
    {0x00484340u, "serve_houses_75", 2, 32, 0, run_serve_00484340, 0},
    {0x00484450u, "serve_houses_6e", 2, 32, 0, run_serve_00484450, 0},
    {0x00484560u, "serve_houses_76", 2, 32, 0, run_serve_00484560, 0},
    {0x00484780u, "serve_houses_78", 2, 32, 0, run_serve_00484780, 0},
    {0x00483E90u, "serve_houses_77", 2, 32, 0, run_serve_00483E90, 0},
    {0x00484670u, "serve_houses_70", 2, 0, 0, run_serve_00484670, 0},
    {0x00484990u, "count_houses", 2, 32, 0, run_count_houses, 0},
    {0x00483FB0u, "serve_houses_god", 3, 32, 0, run_serve_god, 0},
    {0x00484A90u, "serve_houses_4e", 2, 32, 0, run_serve_4e, 0},
    {0x00484890u, "clear_buildings_58", 2, 32, 0, run_clear_58, 0},
    {0x00482DD0u, "lower_houses_e8", 3, 32, 0, run_lower_e8, 0},
    {0x00482EF0u, "lower_houses_fc", 3, 0, 0, run_lower_fc, 0},
    {0x00483160u, "clear_houses_87", 2, 32, 0, run_clear_87, 0},
    {0x00483260u, "lower_buildings_e4", 3, 0, 0, run_lower_e4, 0},
    {0x00482C10u, "lower_main_parts_46", 2, 32, 0, run_lower_46, 0},
    {0x00483000u, "lower_main_parts_48", 2, 32, 0, run_lower_48, 0},
    {0x0042FF20u, "inspect_houses", 2, 0, 0, run_inspect, 0},
};
const int figa_service_ports_count = sizeof(figa_service_ports) / sizeof(figa_service_ports[0]);

/* @bin 0x00483A95 (jump table 0x00483E00) goods -> the index of the house / bazaar stock words */
static int32_t goods_index(int32_t g)
{
    switch (g) {
    case 1: return 0;
    case 2: return 1;
    case 3: return 2;
    case 4: return 3;
    case 5: return 4;
    case 6: return 5;
    case 7: return 6;
    case 8: return 7;
    case 0x0D: return 9;
    case 0x0F: return 8;
    case 0x11: return 0x0A;
    case 0x13: return 0x0B;
    default: return -1;
    }
}

/* one of the four other goods: up to twice the level's amount (x4 for big houses) */
static void bazaar_extra(game_state *st, uint32_t baz, uint32_t h, uint32_t amount_at, uint32_t timer, uint32_t from,
                         uint32_t to)
{
    int32_t amount = S32(st, amount_at);
    if (U8(st, h + BLD_SIZE) > 1) {
        amount <<= 2;
    }
    if (amount == 0) {
        return;
    }
    S16(st, baz + timer) = 10;
    int16_t stock = S16(st, baz + from);
    if (stock <= 0) {
        return;
    }
    int16_t have = S16(st, h + to);
    int32_t need = amount * 2 - have;
    if (need <= 0) {
        return;
    }
    if (need > stock) {
        S16(st, h + to) = (int16_t) (have + stock);
        S16(st, baz + from) = 0;
    } else {
        S16(st, h + to) = (int16_t) (have + need);
        S16(st, baz + from) -= (int16_t) need;
    }
}

static void bazaar_house(game_state *st, int32_t tile, scan_ctx *c)
{
    int16_t hid = building_at(st, tile);
    if (!inhabited(st, hid)) {
        return;
    }
    uint32_t h = BUILDING_ADDR(hid);
    uint32_t baz = BUILDING_ADDR(c->value);
    int32_t level = S16(st, h + BLD_SUBTYPE);
    c->count++;
    if (level < 0x13) {
        level++;
    }
    uint32_t t = 0x58u * (uint32_t) level;
    int16_t pop = S16(st, h + BLD_POPULATION);
    int32_t limit2 = 2 * pop, limit6 = 6 * pop;
    int32_t slots = S32(st, G_HOUSE_LEVEL_U28 + t);
    if (slots > 0 && slots < 4) {
        slots++;
    }
    for (int32_t i = 0, n = S32(st, G_GOODS_COUNT); i < n; i++) {
        int32_t g = S32(st, G_GOODS_LIST + 4u * (uint32_t) i);
        if (g == 0) {
            continue;
        }
        int32_t idx = goods_index(g);
        uint32_t bs = baz + BLD_INVENTORY + 2u * (uint32_t) idx;
        int16_t stock = S16(st, bs);
        if (stock <= 0 || slots == 0) {
            continue;
        }
        uint32_t hs = h + BLD_HOUSE_INVENTORY + 2u * (uint32_t) idx;
        int16_t have = S16(st, hs);
        int32_t want = limit6 - have;
        if (limit2 < want) {
            want = limit2;
        }
        if (want <= 0) {
            continue;
        }
        if ((uint32_t) idx <= 7) {
            int whole = idx == 0 || idx == 6; /* @bin 0x00483E4C the others take half */
            if (stock >= want) {
                if (!whole) {
                    want >>= 1;
                }
                S16(st, hs) = (int16_t) (have + want);
                S16(st, bs) -= (int16_t) want;
            } else {
                if (!whole) {
                    want >>= 1;
                }
                if (whole || stock < want) {
                    S16(st, hs) = (int16_t) (have + stock);
                    S16(st, bs) = 0;
                } else {
                    S16(st, hs) = (int16_t) (have + want);
                    S16(st, bs) -= (int16_t) want;
                }
            }
        }
        slots--;
    }
    bazaar_extra(st, baz, h, G_HOUSE_GOODS_D + t, 0xA4, 0x66, 0x64);
    bazaar_extra(st, baz, h, G_HOUSE_GOODS_A + t, 0x9E, 0x68, 0x66);
    bazaar_extra(st, baz, h, G_HOUSE_GOODS_B + t, 0xA2, 0x6A, 0x68);
    bazaar_extra(st, baz, h, G_HOUSE_GOODS_C + t, 0xA0, 0x6C, 0x6A);
}

int32_t bazaar_sell(game_state *st, int32_t bazaar, int32_t x, int32_t y)
{
    /* @bin 0x00483910 (jump tables 0x00483E00, 0x00483E4C, 0x00483E6C) a bazaar trader serves the
     * houses within 2 tiles: for each good of the list 0x00F53DDC, as long as the house's level
     * allows goods (table 0x0070CDF4 + 1), up to 6x its people (2x per visit; half for most goods)
     * from the bazaar's stock; then the four other goods (levels' tables 0x0070CDF8 ..) up to
     * twice the level's amount. Returns the houses served */
    scan_ctx c = {0, 0, bazaar};
    area_scan(st, x, y, 2, 1, bazaar_house, &c);
    return c.count;
}

static int32_t run_bazaar_sell(game_state *st, const int32_t *a)
{
    return bazaar_sell(st, a[0], a[1], a[2]);
}

const ported_function figa_service2_ports[] = {
    {0x00483910u, "bazaar_sell", 3, 32, 0, run_bazaar_sell, 0},
};
const int figa_service2_ports_count = sizeof(figa_service2_ports) / sizeof(figa_service2_ports[0]);
