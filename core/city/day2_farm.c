/* Imhotep - the farms' crop images (package day2): 0x00488690 and 0x00488840, called by
 * farm_draw_488c20 (day_buildings.c). With flag 0 (the simulation's calls) they set the farm's
 * 3x3 footprint image (0x0047BF50); otherwise they draw the crop sprites through the sprite
 * engine 0x0053E590 (on 0x00F55BB0), display only.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day2_farm.h"

#include "city/day_buildings.h"
#include "city/day_x87.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static int32_t sprite_index(game_state *st, int32_t s)
{
    return s < S32(st, G_IMAGE_COUNT) ? s : 0;
}

static int32_t sprite_s16(game_state *st, int32_t s, uint32_t off)
{
    return S16(st, G_IMAGE_ENTRIES + off + 0x40u * (uint32_t) sprite_index(st, s));
}

void farm_crop_small(game_state *st, int32_t id, int32_t x, int32_t y, int32_t img, int32_t stage, int32_t flag)
{
    /* @bin 0x00488690 */
    if ((uint8_t) flag == 0) {
        int32_t base = S32(st, G_UF56B24);
        int32_t road = (uint8_t) GAME_CALL(st, FN_BUILDING_ROAD_FLAG_4890F0, id) == 0;
        GAME_CALL(st, FN_MAP_BUILDING_TILES, id, x, y, 3, 3, base + road, 8);
        return;
    }
    /* @bin 0x004886DD five sprites (dx, dy), each a growth step of the crop (4 tenths each) */
    static const int32_t dxy[5][2] = {{-2, 0}, {-1, 1}, {0, 2}, {1, 1}, {2, 0}};
    static const int order[5] = {0, 4, 1, 3, 2};
    int32_t spr[5];
    int32_t s = stage / 10;
    for (int k = 0; k < 5; k++) {
        spr[k] = s < 4 ? img + s : img + 5;
        s -= 4;
        if (s < 0) {
            s = 0;
        }
    }
    for (int k = 0; k < 5; k++) {
        int i = order[k];
        int32_t px = dxy[i][0] * S32(st, G_TILE_W) + x;
        int32_t py = dxy[i][1] * S32(st, G_TILE_H) + y;
        py += S32(st, G_U70E448) * 2 - sprite_s16(st, spr[i], 0xE);
        GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, spr[i], px, py);
    }
}

void farm_crop_large(game_state *st, int32_t id, int32_t x, int32_t y, int32_t img, int32_t stage, int32_t flag)
{
    /* @bin 0x00488840 */
    if ((uint8_t) flag == 0) {
        int32_t base = S32(st, G_UF56544);
        int32_t fert = GAME_CALLT(st, G_BUILDING_MANAGER, FN_AREA_FERTILITY_489050, x, y, 3, 3);
        GAME_CALL(st, FN_MAP_BUILDING_TILES, id, x, y, 3, 3, fert / 14 + base, 8);
        return;
    }
    /* @bin 0x004888A2 the tile under the farm's corner by view orientation, its flood depth */
    uint32_t b = BUILDING_ADDR(id);
    int32_t tile = S32(st, G_MAP_START) + S16(st, b + BLD_X) + S16(st, b + BLD_Y) * 0xE4;
    int32_t t = 0;
    uint32_t sz = U8(st, b + BLD_SIZE);
    switch (S32(st, G_VIEW_ORIENTATION)) {
    case 0: t = S32(st, G_FOOTPRINT_OFFSETS - 0x30u + 0x30u * sz) + tile; break;
    case 2: t = S32(st, G_FOOTPRINT_OFFSETS) + tile; break;
    case 4: t = S32(st, G_FOOTPRINT_OFFSETS - 8u + 8u * sz) + tile; break;
    case 6: t = S32(st, G_FOOTPRINT_OFFSETS - 0x38u + 0x38u * sz) + tile; break;
    default: break;
    }
    int32_t depth = U8(st, G_GRID_SPRITE + (uint32_t) t);
    int32_t grown = (int32_t) ((uint32_t) stage * 10u); /* ftol(10.0) * stage */
    int32_t mode = 0, sel = -1;
    if (depth != 0 && S16(st, b + BLD_WORKERS) > 0) {
        int32_t a = S32(st, G_HARVEST_SIZE);
        if (grown < a / 5) {
            mode = 1;
            sel = 4;
        } else if (S32(st, G_FLOOD_STATE) == 1) {
            mode = 3;
            sel = U8(st, G_GRID_RANDOM + (uint32_t) (t + S32(st, G_TICK_DAY))) % 7;
        } else {
            /* @bin 0x004889B9 (int) (grown / (a + 1) * 9.0), x87 */
            mode = 2;
            x87_val v = x87_divv(x87_from_int(grown), x87_from_int(a + 1));
            sel = x87_ftol(x87_mul(v, x87_from_double(0x4022000000000000ull)));
        }
    }
    static const int32_t dxy[9][2] = {{0, -2}, {1, -1}, {2, 0}, {-1, -1}, {0, 0}, {1, 1}, {-2, 0}, {-1, 1}, {0, 2}};
    int32_t s = stage / 10;
    for (int32_t k = 0; k < 9; k++) {
        int32_t px = dxy[k][0] * S32(st, G_TILE_W) + x;
        int32_t py = dxy[k][1] * S32(st, G_TILE_H) + y;
        int32_t spr = s < 5 ? img + s : img + 5;
        py += S32(st, G_U70E448) * 2 - sprite_s16(st, spr, 0xE);
        GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, spr, px, py);
        s -= 2;
        if (s < 0) {
            s = 0;
        }
        if (mode != 0 && sel == k && S32(st, G_LOCUST_TIMER) <= 0) {
            /* @bin 0x00488B1E the flood or harvest overlay sprite */
            int32_t e = 0;
            if (mode == 1) {
                e = S32(st, G_UF560A4) + (((S32(st, G_TICK) >> 3) + id) & 7) + depth * 8 - 8;
            } else if (mode == 2) {
                e = S32(st, G_IMAGE_GROUP_157) + ((int32_t) U8(st, G_GRID_RANDOM + (uint32_t) t) + k) % 7 + depth * 8 - 8;
            } else {
                e = S32(st, G_UF560AC) + U8(st, G_GRID_RANDOM + (uint32_t) t) % 7 + depth * 8 - 8;
            }
            int32_t ex = S32(st, G_TILE_W) - sprite_s16(st, e, 0x1A) + px;
            int32_t ey = S32(st, G_TILE_H) * 3 - sprite_s16(st, e, 0x1C) + py - 5;
            GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, e, ex, ey);
        }
    }
}

int32_t farm_growth_pct(game_state *st, int32_t id)
{
    /* @bin 0x004883D0 (thiscall) growth +0x54 in percent of the harvest size 0x00BCA34C */
    return calc_ratio_percent(S16(st, BUILDING_ADDR(id) + BLD_INDUSTRY_PROGRESS), S32(st, G_HARVEST_SIZE));
}

void farm_fertility_cut(game_state *st, uint32_t self, int32_t id, int32_t pct)
{
    /* @bin 0x00489180 (thiscall) a 3x3 farm (0x004894A0) loses pct percent of the fertility of
     * its tiles (0x00A5D4CC, at least 1); offsets 0x005CC8C0 (3 rows of 0x30 bytes, 8 each) */
    if ((uint8_t) GAME_CALLT(st, self, FN_BUILDING_AREA_TEST_3X3, id) == 0) {
        return;
    }
    uint32_t b = BUILDING_ADDR(id);
    int32_t base = S32(st, G_MAP_START) + S16(st, b + BLD_X) + S16(st, b + BLD_Y) * 0xE4;
    S32(st, G_TILE_CURSOR) = base;
    for (uint32_t row = 0; row < 3; row++) {
        for (uint32_t k = 0; k < 3; k++) {
            int32_t t = S32(st, G_FOOTPRINT_OFFSETS + 0x30u * row + 8u * k) + base;
            int32_t v = (int32_t) ((uint32_t) U8(st, G_GRID_FERTILITY + (uint32_t) t) * (uint32_t) (100 - pct)) / 100;
            U8(st, G_GRID_FERTILITY + (uint32_t) t) = (uint8_t) (v < 1 ? 1 : v);
        }
    }
}

void farm_flood_notice(game_state *st, uint32_t self)
{
    /* @bin 0x00489510 (thiscall on 0x00BCA350, from flood_tick 49 days before the flood) the
     * floodplain farms' coming yield ((0x00488140 + 0x004883D0) * 8, at least 800, doubled with
     * city +0x4994; farm types 0x64, 0x65, 0xE0 of 0x00488030 not counted) against the food
     * stored (+0x8C of types 0x47 and 0xBF): notice 0xCD over 3200 more, 0xCC over 50 */
    int32_t yield = 0, stored = 0;
    for (int32_t id = 1; id <= 4003; id++) { /* @bin 0x0048963F ends at 0x0089B6E8: 4003 */
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        int16_t type = S16(st, b + BLD_TYPE);
        if ((uint8_t) GAME_CALLT(st, G_BUILDING_MANAGER, FN_BUILDING_NEEDS_AREA_CHECK, id) == 0) {
            if (type == 0x47 || type == 0xBF) {
                stored += S16(st, b + BLD_INVENTORY);
            }
            continue;
        }
        int32_t v = (int32_t) ((uint32_t) (GAME_CALLT(st, self, FN_FARM_YIELD_488140, id, 1) + farm_growth_pct(st, id)) << 3);
        if (v < 0x320) {
            v = 0x320;
        } else {
            v = (int32_t) ((uint32_t) (GAME_CALLT(st, self, FN_FARM_YIELD_488140, id, 1) + farm_growth_pct(st, id)) << 3);
            if (v == 0) {
                continue;
            }
        }
        /* @bin 0x00489591 the three switches (tables 0x004896A0, 0x0048972C, 0x004897B8) */
        if ((uint8_t) GAME_CALLT(st, self, FN_BUILDING_IS_TYPE_0X487F80, id) != 0 && S16(st, b + BLD_TYPE) == 0x64) {
            continue;
        }
        if ((uint8_t) GAME_CALLT(st, self, FN_BUILDING_IS_TYPE_0X487F80, id) != 0 && S16(st, b + BLD_TYPE) == 0x65) {
            continue;
        }
        if ((uint8_t) GAME_CALLT(st, self, FN_BUILDING_IS_TYPE_0X487F80, id) != 0 && S16(st, b + BLD_TYPE) == 0xE0) {
            continue;
        }
        if (S32(st, CURRENT_CITY(st) + CITY_U4994) != 0) {
            v += v;
        }
        yield += v;
    }
    int32_t d = yield - stored;
    if (d > 0xC80) {
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xCD, 0, 0);
    } else if (d > 0x32) {
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xCC, 0, 0);
    }
}

static int32_t run_small(game_state *st, const int32_t *a)
{
    farm_crop_small(st, a[0], a[1], a[2], a[3], a[4], a[5]);
    return 0;
}

static int32_t run_large(game_state *st, const int32_t *a)
{
    farm_crop_large(st, a[0], a[1], a[2], a[3], a[4], a[5]);
    return 0;
}

static int32_t run_growth_pct(game_state *st, const int32_t *a)
{
    return farm_growth_pct(st, a[0]);
}

static int32_t run_fertility_cut(game_state *st, const int32_t *a)
{
    farm_fertility_cut(st, G_BUILDING_MANAGER, a[0], a[1]);
    return 0;
}

static int32_t run_flood_notice(game_state *st, const int32_t *a)
{
    (void) a;
    farm_flood_notice(st, G_BUILDING_MANAGER);
    return 0;
}

const ported_function day2_farm_ports[] = {
    {0x004883D0u, "farm_growth_pct", 1, 32, G_BUILDING_MANAGER, run_growth_pct, 0},
    {0x00489180u, "farm_fertility_cut", 2, 0, G_BUILDING_MANAGER, run_fertility_cut, 0},
    {0x00489510u, "farm_flood_notice", 0, 0, G_BUILDING_MANAGER, run_flood_notice, 0},
    {0x00488690u, "farm_crop_small", 6, 0, G_BUILDING_MANAGER, run_small, 0},
    {0x00488840u, "farm_crop_large", 6, 0, G_BUILDING_MANAGER, run_large, 0},
};
const int day2_farm_ports_count = sizeof(day2_farm_ports) / sizeof(day2_farm_ports[0]);
