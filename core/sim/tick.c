/* Imhotep - the game tick (Pharaoh.exe 0x004D7090), the day dispatcher (0x00451C50) and the
 * day / month / year cascade (0x00452370), from their disassembly.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * Every call goes through sim_call (core/sim/call.h). Check: tools/oracle/lockstep.py. */
#include "sim/tick.h"

#include "random/random.h"
#include "state/layout.h"

static uint32_t city(game_state *st)
{
    return CURRENT_CITY(st);
}

static int32_t call(game_state *st, const sim_env *env, uint32_t address)
{
    return sim_call(st, env, address, NULL, 0, 0);
}

static int32_t call1(game_state *st, const sim_env *env, uint32_t address, int32_t a)
{
    return sim_call(st, env, address, &a, 1, 0);
}

static void call_n(game_state *st, const sim_env *env, uint32_t address, int32_t a, int32_t b, int32_t c,
                   int32_t d, int32_t e, int n)
{
    int32_t args[5] = {a, b, c, d, e};
    sim_call(st, env, address, args, n, 0);
}

/* @bin 0x00451FCC .. 0x00452012 a sound timer: when it runs out, the sound object 0x00E92BF8 stops
 * channel 2 (0x0055F561) and 0x00F55B94 := 0 */
static void sound_timer(game_state *st, const sim_env *env, uint32_t timer)
{
    if (S32(st, timer) > 0 && --S32(st, timer) == 0) {
        int32_t two = 2;
        sim_call(st, env, FN_SOUND_STOP_STREAM, &two, 1, G_SOUND);
        S32(st, G_UF55B94) = 0;
    }
}

void day_cascade(game_state *st, const sim_env *env)
{
    /* @bin 0x00452370 */
    S32(st, G_TOTAL_DAYS) += 1;
    S32(st, G_TICK_DAY) += 1;
    if (S32(st, G_TICK_DAY) > 0xF) { /* a new month */
        S32(st, G_TICK_DAY) = 0;
        uint32_t c = city(st);
        S32(st, c + CITY_U475C) = S32(st, c + CITY_U273C); /* 0x00EA5FCC := 0x00EA3FAC */
        S32(st, c + CITY_U273C) = 0;
        S32(st, c + CITY_U4760) = 0;
        S32(st, c + CITY_U4764) = 0;
        S32(st, c + CITY_U438C) += 1; /* 0x00EA5BFC */
        static const uint32_t A[] = {FN_CITY_MOOD_MONTH, FN_MONTH_0X004599D0, FN_MONTH_0X0045A2E0,
                                     FN_MONTH_0X0045A350, FN_MONTH_0X0045A3F0, FN_MONTH_0X00452160,
                                     FN_MONTH_0X004474E0, FN_MONTH_0X0045E420, FN_MONTH_0X004464F0};
        for (unsigned i = 0; i < sizeof(A) / sizeof(A[0]); i++) {
            call(st, env, A[i]);
        }
        sim_call(st, env, FN_EVENTS_MONTH, NULL, 0, G_EVENTS);
        call(st, env, FN_MONTH_0X004EA740);
        call(st, env, FN_MONTH_0X004B7960);
        call(st, env, FN_MONTH_0X004E1850);
        call(st, env, FN_ROADS_ALL);
        call_n(st, env, FN_WATER_PASS, 0, 0, S32(st, G_MAP_WIDTH) - 1, S32(st, G_MAP_HEIGHT) - 1, 0, 5);
        call(st, env, FN_ROUTING_REBUILD_EE0C50);
        call(st, env, FN_MESSAGE_SORT);
        S32(st, G_MONTH) += 1;
        call(st, env, FN_MONTH_0X004E7700);
        call(st, env, FN_MONTH_0X004BC7A0);
        call(st, env, FN_MONTH_0X00452200);
        if (S32(st, G_MONTH) > 0xB) { /* a new year */
            S32(st, G_MONTH) = 0;
            call(st, env, FN_NOTHING);
            U8(st, G_FAST_FORWARD) = 0;
            S32(st, G_UNDO_POSSIBLE) = 0;
            S32(st, G_YEAR) += 1;
            static const uint32_t Y[] = {FN_MONTH_0X004E8630, FN_MONTH_0X00459E50, FN_MONTH_0X0045A450,
                                         FN_MONTH_0X0045A490, FN_MONTH_0X0045A4D0, FN_MONTH_0X0045A500,
                                         FN_MONTH_0X0045A530, FN_MONTH_0X0045A560, FN_MONTH_0X0045A5A0,
                                         FN_MONTH_0X0045A5D0, FN_MONTH_0X004448E0, FN_MONTH_0X00455530};
            for (unsigned i = 0; i < sizeof(Y) / sizeof(Y[0]); i++) {
                call(st, env, Y[i]);
            }
            call1(st, env, FN_MONTH_0X0046E180, 1);
            call(st, env, FN_MONTH_0X004C5300);
        } else {
            call1(st, env, FN_MONTH_0X0046E180, 0);
        }
        if (S32(st, G_MONTH) == 0 || S32(st, G_MONTH) == 6) {
            S32(st, city(st) + CITY_U471C) = 2; /* 0x00EA5F8C */
        }
        call(st, env, FN_MONTH_0X004E9700);
        call(st, env, FN_MONTH_0X004EA000);
        if (U8(st, G_FAST_FORWARD) == 0 && U8(st, G_AUTOSAVE_OPTION) == 1) {
            call1(st, env, FN_FRAME_SAVE_GAME, (int32_t) G_STR_LAST_SAV); /* the autosave, "last.sav" */
        }
    }
    if (S32(st, G_TICK_DAY) == 0 || S32(st, G_TICK_DAY) == 8) {
        call(st, env, FN_CITY_HEALTH_UPDATE);
        if (S32(st, G_TICK_DAY) == 8) {
            sim_call(st, env, FN_MONTH_0X004BE640, NULL, 0, G_UD05720);
        }
    }
    call(st, env, FN_MONTH_0X004E8DB0);
    uint32_t c = city(st);
    if (U8(st, G_MISSION_LIST_FLAGS) != 0 && S32(st, c + CITY_U4720) == 0) { /* 0x00EA5F90 */
        S32(st, c + CITY_U4720) = 1;
        S32(st, G_MESSAGE_POPUP) = 1;
    }
    if (U8(st, G_MISSION_LIST_FLAGS + 6) != 0 && S32(st, c + CITY_U4724) == 0) { /* 0x00EA5F94 */
        S32(st, c + CITY_U4724) = 1;
        S32(st, G_MESSAGE_POPUP) = 1;
        call_n(st, env, FN_MESSAGE_POST, 0x13E, 0, 0, 0, 0, 5);
        c = city(st);
    }
    if (U8(st, G_MISSION_LIST_FLAGS + 1) == 0 && S32(st, c + CITY_POPULATION) >= 0x96) {
        call1(st, env, FN_TUTORIAL_STEP, 1);
    }
}

void day_dispatch(game_state *st, const sim_env *env)
{
    /* @bin 0x00451C50 one step per tick of the day, by the tick counter */
    uint32_t c;
    switch (S32(st, G_TICK)) {
    case 0: call(st, env, FN_NOTHING); break;
    case 1: call1(st, env, FN_GODS_UPDATE_4E9890, 1); break;
    case 2: call(st, env, FN_DAY_STEP_0X02); break;
    case 3: S32(st, G_UF53FD0) = 1; break;
    case 4: call(st, env, FN_DAY_STEP_0X04); break;
    case 5: call1(st, env, FN_FORMATIONS_DAILY, 0); break;
    case 6: call(st, env, FN_NOTHING); break;
    case 7: call(st, env, FN_ROAD_NETWORKS_UPDATE); break;
    case 8: call(st, env, FN_DAY_STOCK_TOTALS_45E040); break;
    case 9: call(st, env, FN_CLEAR_0X0078D784); break;
    case 0xA: call(st, env, FN_BUILDINGS_HIGHEST_UPDATE); break;
    case 0xB: { /* city +0x24 := sum of the 100 i16 at +0x25C4; its maximum at +0x469C */
        c = city(st);
        S32(st, c + CITY_POPULATION) = 0;
        for (uint32_t i = 0; i < 100; i++) {
            S32(st, c + CITY_POPULATION) += S16(st, c + CITY_POPULATION_AGES + 2 * i);
        }
        if (S32(st, c + CITY_POPULATION) > S32(st, c + CITY_POPULATION_HIGHEST)) {
            S32(st, c + CITY_POPULATION_HIGHEST) = S32(st, c + CITY_POPULATION);
        }
        break;
    }
    case 0xC: call(st, env, FN_DAY_DECAY_0X18); break;
    case 0x10: call(st, env, FN_DAY_STEP_0X10); break;
    case 0x11: call(st, env, FN_DAY_STEP_0X11); break;
    case 0x12: call(st, env, FN_DAY_STEP_0X12); break;
    case 0x13: call(st, env, FN_BUILDINGS_ENTRY_ROAD_CHECK); break;
    case 0x14: call(st, env, FN_DAY_PRODUCTION_455900); call(st, env, FN_DAY_FERTILITY_4860D0); break;
    case 0x15: call(st, env, FN_DAY_ROAD_ACCESS_457AF0); break;
    case 0x16: call(st, env, FN_DAY_STEP_0X16); break;
    case 0x17: call(st, env, FN_DAY_POPULATION_458540); break;
    case 0x18: call(st, env, FN_DAY_LIST_45D83C); break;
    case 0x19:
        call(st, env, FN_LABOUR_CATEGORIES_NEED);
        call(st, env, FN_LABOUR_UPDATE);
        call(st, env, FN_LABOUR_BUILDING_SHARES);
        call(st, env, FN_NOTHING);
        call(st, env, FN_NOTHING);
        call(st, env, FN_LABOUR_ALLOCATE);
        break;
    case 0x1B: call(st, env, FN_DAY_WATER_4555D0); break;
    case 0x1C: call(st, env, FN_DAY_STEP_0X1C); break;
    case 0x1D: call1(st, env, FN_FORMATIONS_DAILY, 1); break;
    case 0x1E: S32(st, G_UF53FD0) = 1; break;
    case 0x1F: call(st, env, FN_BUILDING_UPDATE_ALL); break;
    case 0x20: call(st, env, FN_DAY_TRADE_444C20); call(st, env, FN_DAY_MONUMENTS_46F200); break;
    case 0x21: call(st, env, FN_BUILDINGS_COUNT_4677C0); call(st, env, FN_DAY_COVERAGE_468170); break;
    case 0x22: call(st, env, FN_DAY_SERVICE_SHARES); break;
    case 0x23:
        c = city(st);
        if (S16(st, c + CITY_U4478) == 0 || call1(st, env, FN_BUILDING_TYPE_LEVEL, S32(st, c + CITY_U472C)) != 4) {
            call(st, env, FN_HOUSE_SERVICE_DECAY);
        } else if (S32(st, city(st) + CITY_U4990) != 0) { /* 0x00EA6200: every other day */
            S32(st, city(st) + CITY_U4990) = 0;
        } else {
            call(st, env, FN_HOUSE_SERVICE_DECAY);
            S32(st, city(st) + CITY_U4990) = 10;
        }
        break;
    case 0x24: call(st, env, FN_HOUSE_ACCESS_RATINGS); break;
    case 0x25: call(st, env, FN_DESIRABILITY_BUILDINGS); call(st, env, FN_DESIRABILITY_TILES); break;
    case 0x26: call(st, env, FN_BUILDINGS_WATER_LEVEL); break;
    case 0x27: call(st, env, FN_HOUSES_EVOLVE_ALL); break;
    case 0x28: call(st, env, FN_BUILDINGS_STATES_UPDATE); break;
    case 0x2B: call(st, env, FN_DAY_FIRES_469730); break;
    case 0x2C: call(st, env, FN_DAY_RISKS_468B70); break;
    case 0x2D: call(st, env, FN_DAY_SPAWN_469B90); call(st, env, FN_MAP_GRID_DECAY_978D9C); break;
    case 0x2E: call(st, env, FN_NOTHING); break;
    case 0x2F: call(st, env, FN_DAY_CLEAR_COUNTERS); break;
    case 0x30: call(st, env, FN_DAY_BUILDING_COUNTDOWN); break;
    case 0x31: call(st, env, FN_DAY_CITY_RATINGS_4550E0); break;
    case 0x32: call(st, env, FN_NOTHING); break;
    default: break; /* 0xD..0xF, 0x1A, 0x29, 0x2A: nothing */
    }
    sound_timer(st, env, G_LOCUST_TIMER);
    sound_timer(st, env, G_FROGS_COUNTDOWN);
    if (U8(st, G_WINDOW) == 1) {
        sound_timer(st, env, G_HAIL_TIMER);
    }
    /* @bin 0x0045203E the map is revealed column by column while 0x0078D7B0 counts down */
    if (S32(st, G_BLOOD_RIVER_LEFT) > 0) {
        int32_t left = --S32(st, G_BLOOD_RIVER_LEFT);
        int32_t w = S32(st, G_MAP_WIDTH), h = S32(st, G_MAP_HEIGHT);
        if (left == 0) {
            call_n(st, env, FN_WATER_PASS, 0, 0, h - 1, w - 1, 1, 5);
            call_n(st, env, FN_FLAG_0X40000_RECT, 0, 0, h - 1, w - 1, 0, 4);
            call(st, env, FN_BUILDINGS_REDRAW);
            for (int32_t id = 1; id < 2000; id++) {
                uint32_t f = FIGURE_ADDR(id);
                if (U8(st, f + FIG_STATE) != 0 && U8(st, f + FIG_TYPE) == 0x5F) {
                    call1(st, env, FN_FIGURE_TILE_LIST_ADD, id);
                }
            }
        } else {
            int32_t k = (S32(st, G_BLOOD_RIVER_DURATION) - left) / 2;
            if (k < 0xE4) {
                call_n(st, env, FN_WATER_PASS, 0xE5 - k, 0, 0xE6 - k, h - 1, 1, 5);
                call_n(st, env, FN_FLAG_0X40000_RECT, 0xE5 - k, 0, 0xE6 - k, h - 1, 0, 4);
                call(st, env, FN_BUILDINGS_REDRAW);
            }
        }
        U8(st, G_REDRAW) = 1;
    }
    if (S32(st, G_PYRAMID_CURSE_COUNTDOWN) > 0 && --S32(st, G_PYRAMID_CURSE_COUNTDOWN) == 0) {
        U8(st, G_GOD_CURSE_ON) = 0;
    }
    S32(st, G_TICK) += 1;
    if (S32(st, G_TICK) >= 0x33) {
        S32(st, G_TICK) = 0;
        day_cascade(st, env);
    }
    if (U8(st, G_GOD_CURSE_ON) != 0) {
        call(st, env, FN_PYRAMIDS_UPDATE);
    }
}

void game_tick(game_state *st, const sim_env *env)
{
    /* @bin 0x004D7090 the RNG step (inline in the original), two calls, the day when the game is not
     * paused, then four calls (0x004EA2C0 is a tail jump) */
    random_next_state(st);
    call(st, env, FN_ANIM_COUNTERS);
    call(st, env, FN_UNDO_CHECK_485DC0);
    if (S32(st, G_EDITOR) == 0) {
        day_dispatch(st, env);
        call(st, env, FN_COUNTDOWN_0X00EA89DC);
        sim_call(st, env, FN_FLOOD_TICK, NULL, 0, G_UD05720);
        if (S32(st, G_UD386D4) == 3 && S32(st, city(st) + CITY_U4998) == 2) { /* 0x00EA6208 */
            S32(st, city(st) + CITY_U4998) = 0;
        }
    }
    call(st, env, FN_FIGURE_UPDATE_ALL);
    call(st, env, FN_ANIM_STEP_ALL);
    sim_call(st, env, FN_EVENTS_TICK_44BCA0, NULL, 0, G_EVENTS);
    call(st, env, FN_GOALS_CHECK);
}
