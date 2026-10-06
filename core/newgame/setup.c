/* Imhotep - the scenario setup of a custom map start (docs/newgame.md 3.3): 0x004DE820, the map
 * load 0x004DE600, the empire and trade setup 0x00444A80, 0x00444720, 0x00444B10, 0x00442800.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "newgame/setup.h"

#include <string.h>

#include "newgame/map_load.h"
#include "sim/call.h"
#include "state/ported.h"

#define CITY_INDEX 0x00EAAC28u
#define CITY_BASE 0x00EA1870u
#define CITY_SIZE 0x49D8u
#define EDITOR 0x00605A3Cu
#define MAP_NAME 0x00E37078u     /* char[65] */
#define EMPIRE_OBJECTS 0x0077F748u /* 200 x 0x62 */
#define EMPIRE_OBJECTS_END 0x007843D8u
#define OBJECT(j) (EMPIRE_OBJECTS + 0x62u * (uint32_t) (j))

/* ---- empire ---------------------------------------------------------------------------------- */

int32_t empire_route_is_kind2(game_state *st, int32_t route)
{
    if (route < 0 || route >= 0x32) {
        return 0;
    }
    return U8(st, 0x007770F0u + 0x144u * (uint32_t) route) == 2;
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
            if (GAME_CALL(st, 0x00443000u, j, g) == 0 && GAME_CALL(st, 0x00443040u, j, g) == 0) {
                empire_object_trade_amount_set(st, j, g, 0);
            }
        }
    }
}

void empire_cities_setup(game_state *st)
{
    /* @bin 0x00444720 the empire cities 1..60 (0x0077DE00 + 0x6A k) from the objects in use of
     * kind 1, in object order; `c` is the city's +3 as in the original's register */
    GAME_CALL(st, 0x0040F8F0u, 0x0077DE00, 0x1942, 0);
    uint32_t c = 0x0077DE6Du;
    for (int32_t j = 0; j < 200; j++) {
        uint32_t o = OBJECT(j);
        if (U8(st, o + 1) == 0 || U8(st, o) != 1) {
            continue;
        }
        if (c >= 0x0077F745u) {
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
                if (GAME_CALL(st, 0x00443000u, j, g) != 0) {
                    U8(st, c + (uint32_t) g + 0x27) = 1;
                }
                if (GAME_CALL(st, 0x00443040u, j, g) != 0) {
                    U8(st, c + (uint32_t) g + 3) = 1;
                }
            }
            int32_t v = empire_object_trade_amount(st, j, g);
            /* @bin 0x0044480B route limits per good: amount 1 / 2 / 3 -> 15 / 25 / 40, else 0 */
            uint32_t slot = 4u * ((uint32_t) g + 36u * U8(st, c + 1));
            U32(st, 0x0077646Cu + slot) = v == 1 ? 0xF : v == 2 ? 0x19 : v == 3 ? 0x28 : 0;
            U32(st, 0x0077592Cu + slot) = 0;
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
    GAME_CALL(st, 0x0040F8F0u, 0x0077AF00, 0x2260, 0);
    U32(st, 0x0077AEFCu) = 0;
}

void empire_scroll_clamp(game_state *st)
{
    /* @bin 0x00442800 x in [0, 0x005D2A10 + 2 * 0x005D2A18 - 0x00E38D9C), y in [0, 0x005D2A14 +
     * 0x005D2A20 - 0x00E38D98 + 0x005D2A1C) */
    int32_t x = S32(st, 0x00775910u);
    if (x < 0) {
        x = 0;
        S32(st, 0x00775910u) = 0;
    }
    int32_t lim = S32(st, 0x005D2A10u) + (2 * S32(st, 0x005D2A18u) - S32(st, 0x00E38D9Cu));
    if (x >= lim) {
        S32(st, 0x00775910u) = lim - 1;
    }
    int32_t y = S32(st, 0x0077590Cu);
    if (y < 0) {
        y = 0;
        S32(st, 0x0077590Cu) = 0;
    }
    lim = S32(st, 0x005D2A14u) + (S32(st, 0x005D2A20u) - S32(st, 0x00E38D98u) + S32(st, 0x005D2A1Cu));
    if (y >= lim) {
        S32(st, 0x0077590Cu) = lim - 1;
    }
}

/* ---- 0x004DE600 ------------------------------------------------------------------------------ */

void scenario_map_load(game_state *st)
{
    GAME_CALL0(st, 0x0046FA40u); /* grids cleared */
    file_name_strip_extension(st, MAP_NAME);
    GAME_CALL(st, 0x004CBEA0u, 0x00576274, 0x00E38AEC, 3); /* "map" */
    file_name_add_extension(st, MAP_NAME);
    /* @bin 0x004DE631 local := "maps\\" (0x0057626C) + the name (strcpy, strcat) */
    char path[100 + 256];
    size_t n = 0;
    for (uint32_t a = 0x0057626Cu; U8(st, a) != 0 && n < 99; a++) {
        path[n++] = (char) U8(st, a);
    }
    for (uint32_t a = MAP_NAME; n < sizeof(path) - 1; a++) {
        const uint8_t *c = state_ptr(st, a, 1);
        if (!c || *c == 0) {
            break;
        }
        path[n++] = (char) *c;
    }
    path[n] = 0;
    map_file_load(st, path, 0);
    file_name_strip_extension(st, MAP_NAME);
    empire_trade_amounts_setup(st);
    /* @bin 0x004DE6A2 the map size from the scenario info */
    U32(st, 0x00BCA2D0u) = U32(st, 0x00784904u);
    U32(st, 0x00BCA2D4u) = U32(st, 0x00784908u);
    U32(st, 0x00BCA2D8u) = U32(st, 0x0078490Cu);
    U32(st, 0x00BCA2DCu) = U32(st, 0x00784910u);
    GAME_CALL0(st, 0x0046F7B0u);
    /* @bin 0x004DE6D5 the 16 map points (x 0x00784BC0, y 0x00784BE0): off the map -> -1, -1
     * (checked twice) */
    for (uint32_t p = 0x00784BE0u; p < 0x00784C00u; p += 2) {
        for (int k = 0; k < 2; k++) {
            int32_t x = S32(st, 0x00BCA2E8u) + S16(st, p - 0x20), y = S32(st, 0x00BCA2ECu) + S16(st, p);
            if ((GAME_CALL(st, 0x0046F930u, x, y) & 0xFF) == 0) {
                U16(st, p - 0x20) = 0xFFFF;
                U16(st, p) = 0xFFFF;
            }
        }
    }
    GAME_CALL(st, 0x004CC560u, 5, 0x00784914, 0x18, 0x118, 0, (int32_t) U32(st, 0x00F56F84u));
    GAME_CALL(st, 0x004CC560u, 6, 0x00784954, 500, 400, 0, (int32_t) U32(st, 0x00F56F84u));
    GAME_CALL(st, 0x00430530u, (int32_t) U32(st, 0x00E94E04u), 1);
    if (U32(st, EDITOR) == 1) {
        GAME_CALL0(st, 0x004BB090u);
        if (S16(st, 0x00784C8Cu) != -1 && S16(st, 0x00784C8Eu) != -1 && S16(st, 0x00784C90u) != -1 &&
            S16(st, 0x00784C92u) != -1) {
            GAME_CALL(st, 0x004BBD40u, S16(st, 0x00784C8Cu), S16(st, 0x00784C8Eu), 1);
        }
    }
    GAME_CALL0(st, 0x004BBC10u);
    GAME_CALL0(st, 0x004BB3F0u);
    map_setup_grids(st);
    GAME_CALL0(st, 0x004DEA50u); /* terrain rebuild */
    map_flagged_buildings(st);
    GAME_CALL0(st, 0x004DDF90u);
    GAME_CALL0(st, 0x004DE080u);
    GAME_CALL0(st, 0x00517A20u);
    GAME_CALL0(st, 0x005178B0u);
    GAME_CALL0(st, 0x00517370u);
    GAME_CALL0(st, 0x00517660u);
    GAME_CALL0(st, 0x005178E0u);
}

/* ---- 0x004DE820 ------------------------------------------------------------------------------ */

void scenario_setup(game_state *st)
{
    U32(st, CITY_INDEX) = 1;
    scenario_map_load(st);
    GAME_CALL(st, 0x0040F860u, 0x007848D8, 0x007848D8, 0x638); /* the scenario info onto itself */
    if (U32(st, EDITOR) == 0) {
        int16_t ax = S16(st, 0x00784C8Cu), cx = S16(st, 0x00784C8Eu);
        int32_t all = ax != -1 && cx != -1 && S16(st, 0x00784C90u) != -1 && S16(st, 0x00784C92u) != -1;
        GAME_CALL(st, 0x004BBD40u, ax, cx, all); /* initial figures */
    }
    GAME_CALL0(st, 0x005178B0u);
    GAME_CALL0(st, 0x00517370u);
    GAME_CALL0(st, 0x00517660u);
    GAME_CALL0(st, 0x005178E0u);
    /* @bin 0x004DE8A9 entry point (0x00784C64 / 0x00784C66; -1: the middle of the east edge) and
     * exit point (0x00784C68 / 0x00784C6A; -1: the entry point) into the city record */
    int16_t ex = S16(st, 0x00784C64u), ey = S16(st, 0x00784C66u);
    if (ex == -1 || ey == -1) {
        ex = (int16_t) (S32(st, 0x00BCA2D0u) - 1);
        ey = (int16_t) (S32(st, 0x00BCA2D4u) / 2);
        S16(st, 0x00784C64u) = ex;
        S16(st, 0x00784C66u) = ey;
    }
    uint32_t c = CITY_BASE + CITY_SIZE * U32(st, CITY_INDEX);
    int32_t grid = S32(st, 0x00BCA2D8u);
    U16(st, c + 0x2778u) = U8(st, 0x00784C64u);
    U16(st, c + 0x277Au) = U8(st, 0x00784C66u);
    S32(st, c + 0x277Cu) = grid + S16(st, c + 0x2778u) + S16(st, c + 0x277Au) * 0xE4;
    if (S16(st, 0x00784C68u) == -1 || S16(st, 0x00784C6Au) == -1) {
        S16(st, 0x00784C68u) = ex;
        S16(st, 0x00784C6Au) = ey;
    }
    U16(st, c + 0x2780u) = U8(st, 0x00784C68u);
    U16(st, c + 0x2782u) = U8(st, 0x00784C6Au);
    S32(st, c + 0x2784u) = grid + S16(st, c + 0x2780u) + S16(st, c + 0x2782u) * 0xE4;
    /* @bin 0x004DE98C treasury: scenario funds 0x007848F8 scaled by the difficulty's percentage
     * 0x005D1960[d] (0x004CDE40) into +0x0C and +0x2CBC */
    int32_t money = GAME_CALL(st, 0x004CDE40u, S32(st, 0x007848F8u), S32(st, 0x005D1960u + 4u * U32(st, 0x00E39080u)));
    c = CITY_BASE + CITY_SIZE * U32(st, CITY_INDEX);
    S32(st, c + 0x0Cu) = money;
    S32(st, c + 0x2CBCu) = money;
    /* @bin 0x004DE9C8 start year, deadline: +0x00784C4C when 0x00784C48, else +0x00784C54 when
     * 0x00784C50, else +1000000 */
    int32_t year = S16(st, 0x007848D8u);
    S32(st, 0x0078D7BCu) = year;
    if (U32(st, 0x00784C48u) != 0) {
        year += S32(st, 0x00784C4Cu);
    } else if (U32(st, 0x00784C50u) != 0) {
        year += S32(st, 0x00784C54u);
    } else {
        year += 1000000;
    }
    S32(st, 0x0078D7B8u) = year;
    S32(st, 0x00784F10u) = S16(st, 0x007848DCu);
    empire_cities_setup(st);
    empire_0x77af00_clear(st);
    /* @bin 0x004DEA1C the empire map scroll from the empire's record (0x007843D8 + 0x20 e) */
    uint32_t e = 0x007843D8u + 0x20u * U32(st, 0x00784F10u);
    S32(st, 0x00775910u) = S16(st, e);
    S32(st, 0x0077590Cu) = S16(st, e + 2);
    empire_scroll_clamp(st);
    GAME_CALL0(st, 0x00404350u);
    GAME_CALL0(st, 0x004051A0u);
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
