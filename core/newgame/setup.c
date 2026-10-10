/* Imhotep - the scenario setup of a custom map start (docs/newgame.md 3.3): 0x004DE820, the map
 * load 0x004DE600, the empire and trade setup 0x00444A80, 0x00444720, 0x00444B10, 0x00442800.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "newgame/setup.h"

#include <string.h>

#include "newgame/map_load.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"

#define OBJECT(j) (G_EMPIRE_OBJECTS + 0x62u * (uint32_t) (j))

/* ---- empire ---------------------------------------------------------------------------------- */

int32_t empire_route_is_kind2(game_state *st, int32_t route)
{
    if (route < 0 || route >= 0x32) {
        return 0;
    }
    return U8(st, G_ROUTES + ROUTE_KIND + ROUTE_SIZE * (uint32_t) route) == 2;
}

int32_t empire_object_trade_amount(game_state *st, int32_t obj, int32_t good)
{
    uint32_t o = OBJECT(obj);
    if (U8(st, o) == 1 && U8(st, o + 0x18) != 0) {
        uint8_t v = P8(st, o + 0x3E + (uint32_t) good);
        if (v <= 0x28) {
            return v;
        }
    }
    return 0;
}

void empire_object_trade_amount_set(game_state *st, int32_t obj, int32_t good, int32_t value)
{
    uint32_t o = OBJECT(obj);
    if (U8(st, o) == 1 && U8(st, o + 0x18) != 0 && value >= 0) {
        P8(st, o + 0x3E + (uint32_t) good) = (uint8_t) value;
    }
}

void empire_trade_amounts_setup(game_state *st)
{
    /* @bin 0x00444A80 objects in use of kind 1: with no amount for any good 1..35, every amount
     * := 2; then the amounts of goods the object neither sells (0x00443000) nor buys (0x00443040)
     * := 0 */
    for (int32_t j = 0; j < 200; j++) {
        uint32_t o = OBJECT(j);
        if (U8(st, o + 1) == 0 || U8(st, o) != 1) {
            continue;
        }
        int32_t sum = 0;
        for (int32_t g = 1; g < 0x24; g++) {
            sum += empire_object_trade_amount(st, j, g);
        }
        if (sum == 0) {
            for (int32_t g = 1; g < 0x24; g++) {
                empire_object_trade_amount_set(st, j, g, 2);
            }
        }
        for (int32_t g = 1; g < 0x24; g++) {
            if (GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, j, g) == 0 && GAME_CALL(st, FN_EMPIRE_OBJECT_BUYS, j, g) == 0) {
                empire_object_trade_amount_set(st, j, g, 0);
            }
        }
    }
}

void empire_cities_setup(game_state *st)
{
    /* @bin 0x00444720 the empire cities 1..60 (0x0077DE00 + 0x6A k) from the objects in use of
     * kind 1, in object order; `c` is the city's +3 as in the original's register */
    GAME_CALL(st, FN_FILL_BYTES, G_EMPIRE_CITIES, 0x1942, 0);
    uint32_t c = G_EMPIRE_CITIES + 0x6Au + 3u;
    for (int32_t j = 0; j < 200; j++) {
        uint32_t o = OBJECT(j);
        if (U8(st, o + 1) == 0 || U8(st, o) != 1) {
            continue;
        }
        if (c >= G_EMPIRE_CITY_END + 3u) {
            return;
        }
        U8(st, c - 3) = 1;
        U8(st, c - 1) = U8(st, o + 0x18);
        U8(st, c) = U8(st, o + 0x19);
        if (U8(st, o + 0x1A) >= 0x14) {
            U8(st, o + 0x1A) = 0x13;
        }
        U8(st, c + 1) = U8(st, o + 0x1A);
        U8(st, c + 2) = U8(st, o + 0x1B);
        U16(st, c + 0x4B) = U16(st, o + 0x1C);
        U8(st, c + 0x55) = (uint8_t) empire_route_is_kind2(st, U8(st, c + 1));
        for (int32_t g = 1; g < 0x24; g++) {
            U8(st, c + (uint32_t) g + 0x27) = 0;
            U8(st, c + (uint32_t) g + 3) = 0;
            uint8_t kind = U8(st, c - 1);
            /* @bin 0x004447BA kinds 1, 3, 5 and 0: goods sold / bought */
            if (kind == 1 || kind == 3 || kind == 5 || kind == 0) {
                if (GAME_CALL(st, FN_EMPIRE_OBJECT_SELLS, j, g) != 0) {
                    U8(st, c + (uint32_t) g + 0x27) = 1;
                }
                if (GAME_CALL(st, FN_EMPIRE_OBJECT_BUYS, j, g) != 0) {
                    U8(st, c + (uint32_t) g + 3) = 1;
                }
            }
            int32_t v = empire_object_trade_amount(st, j, g);
            /* @bin 0x0044480B route limits per good: amount 1 / 2 / 3 -> 15 / 25 / 40, else 0 */
            uint32_t slot = 4u * ((uint32_t) g + 36u * U8(st, c + 1));
            U32(st, G_TRADE_ROUTE_LIMITS + slot) = v == 1 ? 0xF : v == 2 ? 0x19 : v == 3 ? 0x28 : 0;
            U32(st, G_TRADE_DONE + slot) = 0;
        }
        U16(st, c + 0x4D) = 0xA;
        U16(st, c + 0x4F) = 4;
        U16(st, c + 0x51) = 0;
        U16(st, c + 0x57) = 0;
        U16(st, c + 0x59) = 0;
        U16(st, c + 0x5B) = 0;
        U16(st, c + 0x53) = (uint16_t) j;
        c += 0x6A;
    }
}

void empire_0x77af00_clear(game_state *st)
{
    GAME_CALL(st, FN_FILL_BYTES, G_TRADER_RECORDS, 0x2260, 0);
    U32(st, G_TRADER_NEXT) = 0;
}

void empire_scroll_clamp(game_state *st)
{
    /* @bin 0x00442800 x in [0, 0x005D2A10 + 2 * 0x005D2A18 - 0x00E38D9C), y in [0, 0x005D2A14 +
     * 0x005D2A20 - 0x00E38D98 + 0x005D2A1C) */
    int32_t x = S32(st, G_SCROLL_X);
    if (x < 0) {
        x = 0;
        S32(st, G_SCROLL_X) = 0;
    }
    int32_t lim = S32(st, G_MAP_W) + (2 * S32(st, G_MAP_X) - S32(st, G_UE38D9C));
    if (x >= lim) {
        S32(st, G_SCROLL_X) = lim - 1;
    }
    int32_t y = S32(st, G_SCROLL_Y);
    if (y < 0) {
        y = 0;
        S32(st, G_SCROLL_Y) = 0;
    }
    lim = S32(st, G_MAP_H) + (S32(st, G_MAP_BOTTOM) - S32(st, G_UE38D98) + S32(st, G_MAP_Y));
    if (y >= lim) {
        S32(st, G_SCROLL_Y) = lim - 1;
    }
}

/* ---- 0x004DE600 ------------------------------------------------------------------------------ */

void scenario_map_load(game_state *st)
{
    GAME_CALL0(st, FN_PRE_LOAD_CLEAR); /* grids cleared */
    file_name_strip_extension(st, G_MAP_NAME);
    GAME_CALL(st, FN_TEXT_COPY, G_STR_MAP, G_START_EXT, 3); /* "map" */
    file_name_add_extension(st, G_MAP_NAME);
    /* @bin 0x004DE631 local := "maps\\" (0x0057626C) + the name (strcpy, strcat) */
    char path[100 + 256];
    size_t n = 0;
    for (uint32_t a = G_STR_MAPS_DIR; U8(st, a) != 0 && n < 99; a++) {
        path[n++] = (char) U8(st, a);
    }
    for (uint32_t a = G_MAP_NAME; n < sizeof(path) - 1; a++) {
        const uint8_t *c = state_ptr(st, a, 1);
        if (!c || *c == 0) {
            break;
        }
        path[n++] = (char) *c;
    }
    path[n] = 0;
    map_file_load(st, path, 0);
    file_name_strip_extension(st, G_MAP_NAME);
    empire_trade_amounts_setup(st);
    /* @bin 0x004DE6A2 the map size from the scenario info */
    U32(st, G_MAP_WIDTH) = U32(st, G_SCENARIO_MAP_WIDTH);
    U32(st, G_MAP_HEIGHT) = U32(st, G_SCENARIO_MAP_HEIGHT);
    U32(st, G_MAP_START) = U32(st, G_SCENARIO_MAP_START);
    U32(st, G_MAP_STRIDE_EXTRA) = U32(st, G_SCENARIO_MAP_STRIDE_EXTRA);
    GAME_CALL0(st, FN_MAP_ROW_RANGES);
    /* @bin 0x004DE6D5 the 16 map points (x 0x00784BC0, y 0x00784BE0): off the map -> -1, -1
     * (checked twice) */
    for (uint32_t p = G_MAP_POINTS_Y; p < G_MAP_POINTS_Y + 0x20u; p += 2) {
        for (int k = 0; k < 2; k++) {
            int32_t x = S32(st, G_VIEW_DX) + S16(st, p - 0x20), y = S32(st, G_VIEW_DY) + S16(st, p);
            if ((GAME_CALL(st, FN_MAP_TILE_INSIDE, x, y) & 0xFF) == 0) {
                U16(st, p - 0x20) = 0xFFFF;
                U16(st, p) = 0xFFFF;
            }
        }
    }
    GAME_CALL(st, FN_TEXT_BOX_SET, 5, G_SCENARIO_TITLE, 0x18, 0x118, 0, (int32_t) U32(st, G_FONT_SMALL));
    GAME_CALL(st, FN_TEXT_BOX_SET, 6, G_U784954, 500, 400, 0, (int32_t) U32(st, G_FONT_SMALL));
    GAME_CALL(st, FN_VIEW_TABLE_BUILD, (int32_t) U32(st, G_UE94E04), 1);
    if (U32(st, G_EDITOR) == 1) {
        GAME_CALL0(st, 0x004BB090u);
        if (S16(st, G_ENTRY_X) != -1 && S16(st, G_ENTRY_Y) != -1 && S16(st, G_EXIT_X) != -1 &&
            S16(st, G_EXIT_Y) != -1) {
            GAME_CALL(st, FN_NEWGAME_POINTS_41_CREATE, S16(st, G_ENTRY_X), S16(st, G_ENTRY_Y), 1);
        }
    }
    GAME_CALL0(st, FN_NEWGAME_POINTS_5F_CREATE);
    GAME_CALL0(st, FN_NEWGAME_HERDS_CREATE);
    map_setup_grids(st);
    GAME_CALL0(st, FN_TERRAIN_REBUILD); /* terrain rebuild */
    map_flagged_buildings(st);
    GAME_CALL0(st, FN_VIEW_BOUNDS);
    GAME_CALL0(st, FN_CAMERA_CLAMP);
    GAME_CALL0(st, FN_ROUTING_CLEAR_EE0C50);
    GAME_CALL0(st, FN_ROUTING_REBUILD_ED4140);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EE0C50);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EC7630);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EBAB20);
}

/* ---- 0x004DE820 ------------------------------------------------------------------------------ */

void scenario_setup(game_state *st)
{
    U32(st, CURRENT_PLAYER_ADDR) = 1;
    scenario_map_load(st);
    GAME_CALL(st, FN_COPY_BYTES, G_START_YEAR, G_START_YEAR, 0x638); /* the scenario info onto itself */
    if (U32(st, G_EDITOR) == 0) {
        int16_t ax = S16(st, G_ENTRY_X), cx = S16(st, G_ENTRY_Y);
        int32_t all = ax != -1 && cx != -1 && S16(st, G_EXIT_X) != -1 && S16(st, G_EXIT_Y) != -1;
        GAME_CALL(st, FN_NEWGAME_POINTS_41_CREATE, ax, cx, all); /* initial figures */
    }
    GAME_CALL0(st, FN_ROUTING_REBUILD_ED4140);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EE0C50);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EC7630);
    GAME_CALL0(st, FN_ROUTING_REBUILD_EBAB20);
    /* @bin 0x004DE8A9 entry point (0x00784C64 / 0x00784C66; -1: the middle of the east edge) and
     * exit point (0x00784C68 / 0x00784C6A; -1: the entry point) into the city record */
    int16_t ex = S16(st, G_ENTRY_POINT_X), ey = S16(st, G_ENTRY_POINT_Y);
    if (ex == -1 || ey == -1) {
        ex = (int16_t) (S32(st, G_MAP_WIDTH) - 1);
        ey = (int16_t) (S32(st, G_MAP_HEIGHT) / 2);
        S16(st, G_ENTRY_POINT_X) = ex;
        S16(st, G_ENTRY_POINT_Y) = ey;
    }
    uint32_t c = CURRENT_CITY(st);
    int32_t grid = S32(st, G_MAP_START);
    U16(st, c + CITY_ENTRY_X) = U8(st, G_ENTRY_POINT_X);
    U16(st, c + CITY_ENTRY_Y) = U8(st, G_ENTRY_POINT_Y);
    S32(st, c + CITY_ENTRY_OFFSET) = grid + S16(st, c + CITY_ENTRY_X) + S16(st, c + CITY_ENTRY_Y) * 0xE4;
    if (S16(st, G_EXIT_POINT_X) == -1 || S16(st, G_EXIT_POINT_Y) == -1) {
        S16(st, G_EXIT_POINT_X) = ex;
        S16(st, G_EXIT_POINT_Y) = ey;
    }
    U16(st, c + CITY_EXIT_X) = U8(st, G_EXIT_POINT_X);
    U16(st, c + CITY_EXIT_Y) = U8(st, G_EXIT_POINT_Y);
    S32(st, c + CITY_EXIT_OFFSET) = grid + S16(st, c + CITY_EXIT_X) + S16(st, c + CITY_EXIT_Y) * 0xE4;
    /* @bin 0x004DE98C treasury: scenario funds 0x007848F8 scaled by the difficulty's percentage
     * 0x005D1960[d] (0x004CDE40) into +0x0C and +0x2CBC */
    int32_t money = GAME_CALL(st, FN_CALC_PERCENTAGE, S32(st, G_SCENARIO_FUNDS), S32(st, G_TREASURY_PCT + 4u * U32(st, G_DIFFICULTY)));
    c = CURRENT_CITY(st);
    S32(st, c + CITY_TREASURY) = money;
    S32(st, c + CITY_BALANCE_LAST_YEAR) = money;
    /* @bin 0x004DE9C8 start year, deadline: +0x00784C4C when 0x00784C48, else +0x00784C54 when
     * 0x00784C50, else +1000000 */
    int32_t year = S16(st, G_START_YEAR);
    S32(st, G_YEAR) = year;
    if (U32(st, G_GOAL_TIME) != 0) {
        year += S32(st, G_TIME_LIMIT_YEARS);
    } else if (U32(st, G_GOAL_DEADLINE) != 0) {
        year += S32(st, G_SURVIVE_YEARS);
    } else {
        year += 1000000;
    }
    S32(st, G_DEADLINE_YEAR) = year;
    S32(st, G_EMPIRE_CURRENT) = S16(st, G_SCENARIO_EMPIRE);
    empire_cities_setup(st);
    empire_0x77af00_clear(st);
    /* @bin 0x004DEA1C the empire map scroll from the empire's record (0x007843D8 + 0x20 e) */
    uint32_t e = G_EMPIRE_OBJECTS_END + 0x20u * U32(st, G_EMPIRE_CURRENT);
    S32(st, G_SCROLL_X) = S16(st, e);
    S32(st, G_SCROLL_Y) = S16(st, e + 2);
    empire_scroll_clamp(st);
    GAME_CALL0(st, FN_BUILD_MENU_UPDATE);
    GAME_CALL0(st, FN_BUILD_BUTTONS_UPDATE);
}

/* ---- registration ---------------------------------------------------------------------------- */

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_scenario_setup, scenario_setup)
VOID0(run_scenario_map_load, scenario_map_load)
VOID0(run_trade_amounts, empire_trade_amounts_setup)
VOID0(run_cities_setup, empire_cities_setup)
VOID0(run_77af00_clear, empire_0x77af00_clear)
VOID0(run_scroll_clamp, empire_scroll_clamp)

static int32_t run_route_is_kind2(game_state *st, const int32_t *a)
{
    return empire_route_is_kind2(st, a[0]);
}

static int32_t run_trade_amount(game_state *st, const int32_t *a)
{
    return empire_object_trade_amount(st, a[0], a[1]);
}

static int32_t run_trade_amount_set(game_state *st, const int32_t *a)
{
    empire_object_trade_amount_set(st, a[0], a[1], a[2]);
    return 0;
}

const ported_function newgame_setup_ports[] = {
    {0x004DE820u, "scenario_setup", 0, 0, 0, run_scenario_setup, 0},
    {0x004DE600u, "scenario_map_load", 0, 0, 0, run_scenario_map_load, 0},
    {0x00444A80u, "empire_trade_amounts_setup", 0, 0, 0, run_trade_amounts, 0},
    {0x00444720u, "empire_cities_setup", 0, 0, 0, run_cities_setup, 0},
    {0x00444B10u, "empire_0x77af00_clear", 0, 0, 0, run_77af00_clear, 0},
    {0x00442800u, "empire_scroll_clamp", 0, 0, 0, run_scroll_clamp, 0},
    {0x00444950u, "empire_route_is_kind2", 1, 32, 0, run_route_is_kind2, 0},
    {0x00444A00u, "empire_object_trade_amount", 2, 32, 0, run_trade_amount, 0},
    {0x00444A40u, "empire_object_trade_amount_set", 3, 0, 0, run_trade_amount_set, 0},
};
const int newgame_setup_ports_count = sizeof(newgame_setup_ports) / sizeof(newgame_setup_ports[0]);
