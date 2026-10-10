/* Imhotep - the queries the info panels and advisor pages make (roadmap 8.2, package "p8ui"):
 * goods on their way to a monument, the farm's next harvest, a figure's phrase case, the figure
 * thumbnail's city view, a request row.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "info.h"

#include "command/military.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"

#define BUILDING_TYPE(b) (BUILDING_BASE + BLD_TYPE + (uint32_t) (b) * BUILDING_REC_SIZE)

static uint8_t goods_on_way_to_type(game_state *st, int32_t good, int16_t type)
{
    /* the loop of 0x00420500: a figure in use (+0x16 = 1) carrying `good` (+0x12, u8) whose tile
     * (+0x30, i32) holds a building (grid 0x00B4E6FC) of `type` */
    for (uint32_t f = FIGURE_ADDR(1) + FIG_TYPE; f < G_FIGURES_END; f += FIGURE_SIZE) {
        if (U8(st, f + (FIG_STATE - FIG_TYPE)) != 1 || U8(st, f) != (uint32_t) good) {
            continue;
        }
        int32_t b = S16(st, G_BUILDING_GRID + (uint32_t) S32(st, f + (FIG_U28 - FIG_TYPE)) * 2u);
        if (S16(st, BUILDING_TYPE(b)) == type) {
            return 1;
        }
    }
    return 0;
}

static uint8_t goods_on_way_d2(game_state *st, int32_t good)
{
    /* @bin 0x00420500 whether a figure carries `good` (0x4F, 0x51) onto a building of type 0xD2 */
    return goods_on_way_to_type(st, good, 0xD2);
}

static uint8_t goods_needed_player(game_state *st, int32_t good)
{
    /* @bin 0x00420550 the needs table 0x005C2748 (0x10 bytes, below 0x005C28A8: +0 key, +4 good, +8
     * its stage object) has an entry of the player's key (i16 player +0x5BAA) for `good` whose
     * object is open (+0x12 = 0) and still wants some (+0x0C > 0) */
    for (uint32_t e = G_WORKS_SPHINX; e < G_WORKS_SPHINX_END; e += 0x10) {
        uint32_t obj = U32(st, e + 8);
        if (P16(st, obj + 0x12) != 0) {
            continue;
        }
        int32_t key = S16(st, CITY_BASE + CITY_U433A + U32(st, G_PLAYER) * CITY_REC_SIZE);
        if (S32(st, e) == key && S32(st, e + 4) == good && (int32_t) P32(st, obj + 0x0C) > 0) {
            return 1;
        }
    }
    return 0;
}

static uint8_t goods_needed(game_state *st, uint32_t from, uint32_t to, int32_t good, int32_t key)
{
    /* the needs tables of 0x004205B0 and its copies: entries of 0x10 bytes (+0 key, +4 good, +8 the
     * stage object: open (+0x12 = 0) and still wanting (+0x0C > 0)) */
    for (uint32_t e = from; e < to; e += 0x10) {
        uint32_t obj = U32(st, e + 8);
        if (P16(st, obj + 0x12) == 0 && S32(st, e) == key && S32(st, e + 4) == good &&
            (int32_t) P32(st, obj + 0x0C) > 0) {
            return 1;
        }
    }
    return 0;
}

/* @bin 0x004205B0 table 0x00596080..0x00596150 */
static uint8_t needed_596080(game_state *st, int32_t g, int32_t k) { return goods_needed(st, G_WORKS_SUN_TEMPLE, G_WORKS_SUN_TEMPLE_END, g, k); }
/* @bin 0x00420990 table 0x0059EDE8..0x0059EF98 */
static uint8_t needed_59ede8(game_state *st, int32_t g, int32_t k) { return goods_needed(st, G_WORKS_E1, G_WORKS_E1_END, g, k); }
/* @bin 0x00420CC0 table 0x005A8140..0x005A84C0 */
static uint8_t needed_5a8140(game_state *st, int32_t g, int32_t k) { return goods_needed(st, G_WORKS_E6, G_WORKS_E6_END, g, k); }
/* @bin 0x00420FC0 table 0x005AD210..0x005AD3C0 */
static uint8_t needed_5ad210(game_state *st, int32_t g, int32_t k) { return goods_needed(st, G_WORKS_E3, G_WORKS_E3_END, g, k); }
/* @bin 0x004212A0 table 0x005B0058..0x005B0148 */
static uint8_t needed_5b0058(game_state *st, int32_t g, int32_t k) { return goods_needed(st, G_WORKS_E4, G_WORKS_E4_END, g, k); }

static uint8_t goods_on_way_to(game_state *st, int32_t good, int32_t building)
{
    /* @bin 0x004E60F0 whether a figure in use carries `good` to the tile of `building`, a
     * building of type 0xCF or 0xD0 (the type tested per figure, as the original) */
    for (uint32_t f = FIGURE_ADDR(1) + FIG_TYPE; f < G_FIGURES_END; f += FIGURE_SIZE) {
        if (U8(st, f + (FIG_STATE - FIG_TYPE)) != 1 || U8(st, f) != (uint32_t) good) {
            continue;
        }
        int32_t b = S16(st, G_BUILDING_GRID + (uint32_t) S32(st, f + (FIG_U28 - FIG_TYPE)) * 2u);
        int16_t t = S16(st, BUILDING_TYPE(building));
        if ((t == 0xCF || t == 0xD0) && building == b) {
            return 1;
        }
    }
    return 0;
}

static uint8_t goods_needed_stage(game_state *st, int32_t good, int32_t building)
{
    /* @bin 0x004E6150 the needs table of the building's type (0xCF: 0x00586B08, 10 entries; else
     * 0x0058FE48, 18): an open, still wanting entry for `good` whose key is the building's stage
     * count (0x004E5A30, asked per open entry) */
    for (int32_t k = 0;; k++) {
        int16_t type = S16(st, BUILDING_TYPE(building));
        if (k >= (type != 0xCF ? 8 : 0) + 0x0A) {
            return 0;
        }
        uint32_t e = (type == 0xCF ? G_WORKS_CF : G_WORKS_D0) + (uint32_t) k * 0x10u;
        uint32_t obj = U32(st, e + 8);
        if (P16(st, obj + 0x12) != 0) {
            continue;
        }
        int32_t stage = GAME_CALL(st, FN_MONUMENT_STAGE_COUNT, building);
        if (S32(st, e) == stage && S32(st, e + 4) == good && (int32_t) P32(st, U32(st, e + 8) + 0x0C) > 0) {
            return 1;
        }
    }
}

static int32_t farm_next_harvest(game_state *st, uint32_t self, int32_t b)
{
    /* @bin 0x00489240 (thiscall, the buildings object) the farm's next harvest month: -1 unless
     * 0x00488030; its season row by type (jump table 0x0048933C: 0x64..0x69 rows 0..5, 0xC3 8,
     * 0xC4 7, 0xE0 9; others -1); on a flood plain (0x004894A0) the flood's month 0x00D386D0;
     * else of the row's four months (0x0056F5B8 + row * 16, -1 ends) the next after the current
     * month 0x0078D7C0 (12 ahead at most, the same month counting as 12) */
    int32_t row = -1;
    if ((uint8_t) GAME_CALLT(st, self, FN_BUILDING_IS_TYPE_0X487F80, b) != 0) {
        switch (S16(st, BUILDING_TYPE(b))) {
        case 0x64: row = 0; break;
        case 0x65: row = 1; break;
        case 0x66: row = 2; break;
        case 0x67: row = 3; break;
        case 0x68: row = 4; break;
        case 0x69: row = 5; break;
        case 0xC3: row = 8; break;
        case 0xC4: row = 7; break;
        case 0xE0: row = 9; break;
        default: break;
        }
    }
    if ((uint8_t) GAME_CALLT(st, self, FN_BUILDING_AREA_TEST_3X3, b) != 0) {
        return S32(st, G_FLOOD_MONTH);
    }
    int32_t month = S32(st, G_MONTH);
    int32_t best = 0x0C, found = -1;
    uint32_t p = G_HARVEST_MONTHS + (uint32_t) row * 16u;
    for (int32_t i = 0; i < 4; i++, p += 4) {
        int32_t m = S32(st, p);
        if (m == -1) {
            break;
        }
        int32_t d = m - month;
        if (d <= 0) {
            d += 0x0C;
        }
        if (d < best) {
            best = d;
            found = m;
        }
    }
    return found;
}

static int32_t phrase_dock_case(game_state *st, int32_t f)
{
    /* @bin 0x00495B60 the figure's building (i16 +0x66, 0x184-byte records from 0x00BCD5F0): in
     * use and of type 0x4B or 0xC0 (a dock), its three dockers (i16 +0x9E..+0xA2): one in use doing
     * action 9, 0x0B, 0x0E or 0x0F: 1; 0x0A, 0x0C, 0x0D or 0x10: 2; none: 0; else 1 */
    int32_t b = S16(st, FIGURE_ADDR(f) + FIG_DEST_BUILDING);
    uint32_t r = (uint32_t) b * BUILDING_REC_SIZE;
    if (U8(st, BUILDING_BASE + BLD_STATE + r) != 1) {
        return 1;
    }
    int16_t t = S16(st, BUILDING_BASE + BLD_TYPE + r);
    if (t != 0x4B && t != 0xC0) {
        return 1;
    }
    for (uint32_t i = 0; i <= 2; i++) {
        int32_t d = S16(st, BUILDING_BASE + BLD_DOCKERS + r + i * 2u);
        if (d == 0 || U8(st, FIGURE_ADDR(d) + FIG_STATE) != 1) {
            continue;
        }
        uint8_t act = U8(st, FIGURE_ADDR(d) + FIG_ACTION);
        if (act == 9 || act == 0x0B || act == 0x0F || act == 0x0E) {
            return 1;
        }
        if (act == 0x0A || act == 0x0C || act == 0x0D || act == 0x10) {
            return 2;
        }
    }
    return 0;
}

static void thumbnail_view(game_state *st, int32_t x, int32_t y, int32_t start, int32_t f)
{
    /* @bin 0x00430E60 the city view of the figure thumbnail: camera x, y and map start into
     * 0x0070E478 / 0x0070E474 / 0x0070E484, the clip 0x00D3B550..0x00D3B55C set to the viewport
     * 0x0070E430..0x0070E43C; ground 0x00430EF0, upper 0x00436B60(f), 0x00439740(0); the clip back
     * to the whole screen */
    S32(st, G_U70E478) = x;
    S32(st, G_U70E474) = y;
    S32(st, G_CV_MAP_START) = start;
    S32(st, G_CLIP_LEFT) = S32(st, G_CV_VIEW_LEFT);
    S32(st, G_CLIP_RIGHT) = S32(st, G_CV_VIEW_RIGHT);
    S32(st, G_CLIP_TOP) = S32(st, G_CV_VIEW_TOP);
    S32(st, G_CLIP_BOTTOM) = S32(st, G_CV_VIEW_BOTTOM);
    GAME_CALL0(st, FN_CITY_GROUND_DRAW);
    GAME_CALL(st, FN_CITY_UPPER_DRAW, f);
    GAME_CALL(st, FN_CITY_FIGURES_ON_TOP, 0);
    int32_t w = S32(st, G_SCREEN_W), h = S32(st, G_SCREEN_H);
    S32(st, G_CLIP_LEFT) = 0;
    S32(st, G_CLIP_RIGHT) = w;
    S32(st, G_CLIP_TOP) = 0;
    S32(st, G_CLIP_BOTTOM) = h;
}

static void thumbnail_geometry(game_state *st)
{
    /* @bin 0x004DDF70 the thumbnail's view geometry: 0x004303A0(0xE4, 0xE4, 0, 0x1E, 5, 0x0F, 0x3A, 0x1E) */
    GAME_CALL(st, FN_VIEW_GEOMETRY_SET, 0xE4, 0xE4, 0, 0x1E, 5, 0x0F, 0x3A, 0x1E);
}

static int32_t request_row(game_state *st, int32_t row)
{
    /* @bin 0x0044D500 0x0044D520(row, &row): the row's event written over the argument, unused */
    int32_t ev = row;
    return request_row_check(st, row, &ev);
}

/* ---- port table ---- */

static int32_t run_d2(game_state *st, const int32_t *a) { return goods_on_way_d2(st, a[0]); }
static int32_t run_player(game_state *st, const int32_t *a) { return goods_needed_player(st, a[0]); }
static int32_t run_596080(game_state *st, const int32_t *a) { return needed_596080(st, a[0], a[1]); }
static int32_t run_59ede8(game_state *st, const int32_t *a) { return needed_59ede8(st, a[0], a[1]); }
static int32_t run_5a8140(game_state *st, const int32_t *a) { return needed_5a8140(st, a[0], a[1]); }
static int32_t run_5ad210(game_state *st, const int32_t *a) { return needed_5ad210(st, a[0], a[1]); }
static int32_t run_5b0058(game_state *st, const int32_t *a) { return needed_5b0058(st, a[0], a[1]); }
static int32_t run_way_to(game_state *st, const int32_t *a) { return goods_on_way_to(st, a[0], a[1]); }
static int32_t run_stage(game_state *st, const int32_t *a) { return goods_needed_stage(st, a[0], a[1]); }
static int32_t run_harvest(game_state *st, const int32_t *a) { return farm_next_harvest(st, G_BUILDING_MANAGER, a[0]); }
static int32_t run_phrase(game_state *st, const int32_t *a) { return phrase_dock_case(st, a[0]); }
static int32_t run_view(game_state *st, const int32_t *a) { thumbnail_view(st, a[0], a[1], a[2], a[3]); return 0; }
static int32_t run_geom(game_state *st, const int32_t *a) { (void) a; thumbnail_geometry(st); return 0; }
static int32_t run_row(game_state *st, const int32_t *a) { return request_row(st, a[0]); }

const ported_function info_query_ports[] = {
    {0x00420500u, "goods_on_way_d2", 1, 8, 0, run_d2, 0},
    {0x00420550u, "goods_needed_player", 1, 8, 0, run_player, 0},
    {0x004205B0u, "goods_needed_596080", 2, 8, 0, run_596080, 0},
    {0x00420990u, "goods_needed_59ede8", 2, 8, 0, run_59ede8, 0},
    {0x00420CC0u, "goods_needed_5a8140", 2, 8, 0, run_5a8140, 0},
    {0x00420FC0u, "goods_needed_5ad210", 2, 8, 0, run_5ad210, 0},
    {0x004212A0u, "goods_needed_5b0058", 2, 8, 0, run_5b0058, 0},
    {0x004E60F0u, "goods_on_way_to", 2, 8, 0, run_way_to, 0},
    {0x004E6150u, "goods_needed_stage", 2, 8, 0, run_stage, 0},
    {0x00489240u, "farm_next_harvest", 1, 32, G_BUILDING_MANAGER, run_harvest, 0},
    {0x00495B60u, "phrase_dock_case", 1, 32, 0, run_phrase, 0},
    {0x00430E60u, "thumbnail_view", 4, 0, 0, run_view, 0},
    {0x004DDF70u, "thumbnail_geometry", 0, 0, 0, run_geom, 0},
    {0x0044D500u, "request_row", 1, 32, 0, run_row, 0},
};
const int info_query_ports_count = sizeof(info_query_ports) / sizeof(info_query_ports[0]);
