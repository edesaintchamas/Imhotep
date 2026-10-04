/* Imhotep - scenario start (Pharaoh.exe 0x004D52A0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/scenario_start.h"

#include <string.h>

#include "model/building_model.h"
#include "model/model_text.h"
#include "city/goods.h"
#include "map/routing.h"
#include "map/terrain.h"
#include "state/pre_load.h"

#define MAP_WIDTH 0x00BCA2D0u
#define MAP_HEIGHT 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define MAP_ROW_SKIP 0x00BCA2DCu

void scenario_start_map(game_state *st)
{
    /* @bin 0x004D52A0 image grid (chunk 7, 0x32C40 bytes) := 0 */
    memset(state_ptr(st, 0x00B7482Cu, 0x32C40u), 0, 0x32C40u);
    /* @bin 0x004D52A0 0x00784F10 := (i16) scenario info +0x04, sign-extended (meaning open) */
    S32(st, 0x00784F10u) = S16(st, 0x007848DCu);
    /* @bin 0x004D52A0 width, height, start offset, row skip from scenario info +0x2C..+0x38 */
    S32(st, MAP_WIDTH) = S32(st, 0x00784904u);
    S32(st, MAP_HEIGHT) = S32(st, 0x00784908u);
    S32(st, MAP_START) = S32(st, 0x0078490Cu);
    S32(st, MAP_ROW_SKIP) = S32(st, 0x00784910u);
    map_row_ranges(st);
}

void map_row_ranges(game_state *st)
{
    /* @bin 0x0046F7B0 (C division truncates toward zero, as IDIV) */
    int32_t w = S32(st, MAP_WIDTH), h = S32(st, MAP_HEIGHT);
    int32_t x0 = (0xE4 - w) / 2, y0 = (0xE4 - h) / 2;
    int32_t x1 = x0 - 1 + w, y1 = y0 - 1 + h;
    int32_t cx = (x1 + x0) / 2, cy = (y1 + y0) / 2;
    S32(st, 0x00BCA2F0u) = x1;
    S32(st, 0x00BCA2F4u) = y1;
    S32(st, 0x00BCA2F8u) = cx;
    S32(st, 0x00BCA2FCu) = cy;
    S32(st, 0x00BCA2E8u) = x0;
    S32(st, 0x00BCA2ECu) = y0;
    int32_t a = (cy - y0) + x0;
    for (int32_t y = y0; y <= y1; y++, a--) {
        int32_t lo = y > cy ? (x0 - cy) - 1 + y : a;
        uint32_t k = 4u * (uint32_t) (y - y0);
        S32(st, 0x00BC9F24u + k) = lo - x0;
        S32(st, 0x00BC9B94u + k) = ((cx * 2 - lo) + 2) - x0;
        S32(st, 0x00BC9804u + k) = lo + y * 0xE4;
    }
}

/* @bin 0x00540C90 the building model of the current difficulty (0x00E39080) */
static int load_building_model(game_state *st, const game_env *env)
{
    int32_t d = S32(st, 0x00E39080u);
    size_t len;
    const uint8_t *text = d >= 0 && d <= 4 ? env->file(env->file_ctx, building_model_files[d], &len) : NULL;
    static building_model m;
    if (!text || building_model_parse(text, len, &m) != 0) {
        return -1;
    }
    building_model_store(&m, st);
    return 0;
}

/* a model file of the current difficulty, named by the exe's table at `table` */
static const uint8_t *model_file(game_state *st, const game_env *env, uint32_t table, size_t *len)
{
    const char *name = model_file_name(st, table);
    return name ? env->file(env->file_ctx, name, len) : NULL;
}

/* Steps of the scenario start not ported yet: they leave the state as it was. The state check
 * (tests/differential/oracle_state_check.py) lists what they write in the original. */
static void not_ported(game_state *st, uint32_t address)
{
    (void) st;
    (void) address;
}

int scenario_start(game_state *st, const game_env *env)
{
    /* @bin 0x004D52A0, in order (edi = 0 throughout) */
    size_t len;
    const uint8_t *text;
    scenario_start_map(st);
    if (load_building_model(st, env) != 0) {
        return -1;
    }
    if (!(text = model_file(st, env, 0x0060191Cu, &len)) || model_load_figures(st, text, len) != 0) {
        return -1;
    }
    if (!(text = model_file(st, env, 0x00601930u, &len)) || model_load_tax_sentiment(st, text, len) != 0) {
        return -1;
    }
    not_ported(st, 0x004F4520u); /* graphics slots for the monuments (and the pyramid kind) */
    not_ported(st, 0x0053D970u); /* graphics */
    not_ported(st, 0x004BC720u);
    not_ported(st, 0x004771C0u);
    /* @bin 0x004D52A0 16 map points (x at 0x00784BC0, y at 0x00784BE0, i16, scenario info): a point
     * outside the map becomes (-1, -1); the test runs twice */
    for (uint32_t i = 0; i < 16; i++) {
        for (int k = 0; k < 2; k++) {
            int32_t x = S32(st, 0x00BCA2E8u) + S16(st, 0x00784BC0u + 2 * i);
            int32_t y = S32(st, 0x00BCA2ECu) + S16(st, 0x00784BE0u + 2 * i);
            if (!map_tile_inside(st, x, y)) {
                S16(st, 0x00784BC0u + 2 * i) = -1;
                S16(st, 0x00784BE0u + 2 * i) = -1;
            }
        }
    }
    /* @bin 0x004E7E20 0x00EA1820 := (byte 0x00EA624B != 0) */
    S32(st, 0x00EA1820u) = U8(st, 0x00EA624Bu) != 0;
    not_ported(st, 0x004677C0u); /* industry and building counts */
    not_ported(st, 0x00468170u);
    /* @bin 0x004D52A0 view orientation 0..6, else 0; 0x00BCA2E0 := it */
    if (S32(st, 0x00E94E04u) < 0 || S32(st, 0x00E94E04u) > 6) {
        S32(st, 0x00E94E04u) = 0;
    }
    S32(st, 0x00BCA2E0u) = S32(st, 0x00E94E04u);
    not_ported(st, 0x004E59A0u);
    not_ported(st, 0x00485F70u); /* building images and building grid */
    if (S32(st, 0x00E945C0u) < 0x8D) {
        U8(st, 0x00BCA30Cu) = 1;
        not_ported(st, 0x00486E70u);
    }
    if (S32(st, 0x00E945C0u) < 0x8E) {
        not_ported(st, 0x00489840u);
    }
    terrain_state t = {st, {0, 0, 0}};
    terrain_rebuild(&t);                                                           /* 0x004DEA50 */
    terrain_floodplain_build(&t, 0, 0, S32(st, 0x00BCA2D0u) - 1, S32(st, 0x00BCA2D4u) - 1); /* 0x004BE160 */
    not_ported(st, 0x00430530u); /* view projection tables: display */
    not_ported(st, 0x004DDF90u);
    not_ported(st, 0x004DE080u);
    routing_clear_ee0c50(st);
    routing_rebuild_ed4140(st);
    routing_rebuild_ee0c50(st);
    routing_rebuild_ec7630(st);
    routing_rebuild_ebab20(st);
    not_ported(st, 0x004B32C0u);
    not_ported(st, 0x00468860u);
    not_ported(st, 0x00457AF0u);
    not_ported(st, 0x0045E040u);
    not_ported(st, 0x00404350u);
    not_ported(st, 0x004051A0u);
    not_ported(st, 0x004E1830u); /* reads the clock (timeGetTime) */
    not_ported(st, 0x0041D730u); /* ambient sounds */
    not_ported(st, 0x0041DF60u);
    S32(st, 0x00BC9694u) = 0;
    S32(st, 0x00E94E0Cu) = 0;
    S32(st, 0x00E94E10u) = 0;
    S32(st, 0x007027A8u) = 1; /* @bin 0x00417E70 (byte store: 0x007027A8 := 1) */
    uint32_t city = 0x00EA1870u + 0x49D8u * U32(st, 0x00EAAC28u);
    S32(st, city + 0x4724) = 1; /* 0x00EA5F94 */
    S32(st, city + 0x4720) = 1; /* 0x00EA5F90 */
    not_ported(st, 0x00476750u);
    not_ported(st, 0x0046D840u);
    S32(st, 0x00E9511Cu) = 0;
    S32(st, 0x00E95120u) = 0;
    U8(st, 0x00E38E60u) = 0;
    not_ported(st, 0x004BC7A0u);
    not_ported(st, 0x004BCA90u);
    U8(st, 0x00EAAC2Cu) = 1;
    not_ported(st, 0x00442660u);
    not_ported(st, 0x004426C0u);
    not_ported(st, 0x0046F190u); /* its result goes to 0x00BCB9B0 */
    not_ported(st, 0x004D5AD0u);
    not_ported(st, 0x004D5BC0u);
    if (S32(st, 0x00E38EBCu) != 0) {
        S32(st, 0x0078D828u) = 0;
    }
    S32(st, 0x00E945C0u) = 0xA0;
    not_ported(st, 0x00443330u);
    goods_available_update(st);
    not_ported(st, 0x0045AA30u);
    not_ported(st, 0x004F61E0u);
    not_ported(st, 0x004A3560u);
    return 0;
}
