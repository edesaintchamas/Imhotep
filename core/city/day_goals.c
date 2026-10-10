/* Imhotep - the scenario's win and loss conditions, every tick (0x004EA2C0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day_goals.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


int32_t goals_counters_met(game_state *st)
{
    /* @bin 0x004EA930 the indices of the byte table 0x004EA978 that are tested (as 0x00469D50) */
    static const uint8_t tested[] = {0, 7, 9, 12, 14, 16, 17, 18, 19, 22, 23, 24, 25, 27, 29};
    for (unsigned i = 0; i < sizeof(tested); i++) {
        uint32_t a = G_COUNTERS + 4u * tested[i];
        if (S32(st, a) < S32(st, a - (G_COUNTERS - G_GOAL_TARGETS))) {
            return 0;
        }
    }
    return 1;
}

int32_t goal_kind_4f52f0(game_state *st, int32_t i)
{
    /* @bin 0x004F52F0 */
    uint32_t rec = G_MONUMENT_MODELS + 16u * (uint32_t) (int32_t) S16(st, G_MONUMENT_SLOTS + 2u * (uint32_t) i);
    if (S16(st, rec) == 1) {
        int16_t k = S16(st, rec + 2);
        if (k >= 1 && k <= 5) {
            return 1;
        }
    }
    return S16(st, rec + 2) == 4;
}

int32_t houses_at_least(game_state *st, int32_t level)
{
    /* @bin 0x004EA280 records 1..4000 (the last one past the array, as in the original) */
    int32_t n = 0;
    for (int32_t id = 1; id <= 4000; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 0 && U8(st, b + BLD_HOUSE_SIZE) != 0 && S16(st, b + BLD_POPULATION) != 0 && S16(st, b + BLD_TYPE) - 10 >= level) {
            n++;
        }
    }
    return n;
}

int32_t ui_video_open(game_state *st, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    /* @bin 0x00538DC0 when videos are on (0x00F54118), 0x00538E30 (the video player, platform)
     * with 0x00F53FFC := a5; on failure the window a5 is opened instead (0x00E38DDA) */
    if (S32(st, G_VIDEO_STATE) == 0) {
        return 0;
    }
    S32(st, G_VIDEO_WINDOW) = a5;
    if (GAME_CALL(st, FN_PLATFORM_VIDEO_PLAY, a1, a2, a3, a4) == 0) {
        U8(st, G_WINDOW) = (uint8_t) a5;
        if (S32(st, G_VIDEO_STATE) == 2) {
            U8(st, G_REDRAW) = 1;
        }
        return 0;
    }
    U8(st, G_WINDOW) = 0x13;
    return 1;
}

static void ui_window(game_state *st, uint32_t text)
{
    /* @bin 0x004EA60D the result window, centred on the screen (0x00E38DAC x 0x00E38DB0) */
    ui_video_open(st, (int32_t) text, (S32(st, G_SCREEN_W) - 0x190) / 2, (S32(st, G_SCREEN_H) - 0x124) / 2, 1, 0x42);
}

void goals_check(game_state *st)
{
    /* @bin 0x004EA2C0 the goals of the scenario (0x00784C3C..0x00784C5C: culture, prosperity,
     * population, monuments, kingdom, house level), the time limit, the city's ruin; then the
     * result: a won city asks once (+0x4718) and shows its window; a lost one posts message
     * 0x137 once (+0x4714) or ends (0x0041AA20) */
    int32_t goals = 0;
    U8(st, G_UEA17FC) = 0;
    if (S16(st, G_U784B60) != 0) {
        return;
    }
    U8(st, G_UE38DD9) = 1;
    uint32_t c = CURRENT_CITY(st);
    if (U8(st, G_GOAL_CULTURE_ON) != 0) {
        goals = 1;
        if (S32(st, c + CITY_RATING_CULTURE) < S32(st, G_GOAL_CULTURE)) {
            U8(st, G_UE38DD9) = 0;
        }
    }
    if (U8(st, G_GOAL_PROSPERITY_ON) != 0) {
        goals++;
        if (S32(st, c + CITY_RATING_PROSPERITY) < S32(st, G_GOAL_PROSPERITY)) {
            U8(st, G_UE38DD9) = 0;
        }
    }
    if (S32(st, G_GOAL_POPULATION_ON) != 0) {
        goals++;
        if (S32(st, c + CITY_POPULATION) < S32(st, G_GOAL_POPULATION)) {
            U8(st, G_UE38DD9) = 0;
        }
    }
    if (U8(st, G_GOAL_MONUMENT_ON) != 0) {
        goals++;
        if (S32(st, c + CITY_RATING_MONUMENT) < S32(st, G_GOAL_MONUMENT)) {
            U8(st, G_UE38DD9) = 0;
        }
        if (!goals_counters_met(st)) {
            U8(st, G_UE38DD9) = 0;
        }
        for (int32_t i = 0; i < 3; i++) {
            goal_kind_4f52f0(st, i); /* its result is not used */
        }
        c = CURRENT_CITY(st);
    }
    if (U8(st, G_GOAL_KINGDOM_ON) != 0) {
        goals++;
        if (S32(st, c + CITY_RATING_KINGDOM) < S32(st, G_GOAL_KINGDOM)) {
            U8(st, G_UE38DD9) = 0;
        }
    }
    if (S32(st, G_GOAL_HOUSES) != 0) {
        goals++;
        if (houses_at_least(st, S32(st, G_GOAL_HOUSE_LEVEL)) < S32(st, G_GOAL_HOUSES)) {
            U8(st, G_UE38DD9) = 0;
        }
        c = CURRENT_CITY(st);
    }
    if (S32(st, G_CUSTOM) == 0 && (int32_t) S8(st, G_MISSION) == S32(st, G_MISSION_LISTS + G_MISSION_LIST_STRIDE * 11) && U8(st, G_MISSION_LIST_FLAGS + 11) == 0 &&
        U8(st, G_MISSION_LIST_FLAGS + 10) != 0) {
        if (houses_at_least(st, 9) > 0) {
            GAME_CALL(st, FN_TUTORIAL_STEP, 0xB);
        }
        c = CURRENT_CITY(st);
    }
    int32_t limit = S32(st, G_GOAL_TIME), survive = S32(st, G_GOAL_DEADLINE);
    int timed = 1;
    if (goals <= 0) {
        U8(st, G_UE38DD9) = 0;
        if (limit != 0) {
            goals++;
        }
        if (survive != 0) {
            goals++;
        }
    } else if (U8(st, G_UE38DD9) == 1 && survive == 0) {
        timed = 0;
    }
    if (timed) {
        /* @bin 0x004EA473 the year limit 0x0078D7B8, extended by +0x47A8 (0: 7 years, 1: 2) */
        int32_t year = S32(st, G_DEADLINE_YEAR);
        if (limit != 0) {
            int32_t ext = S32(st, c + CITY_U47A8);
            if (ext == 0) {
                year += 7;
            } else if (ext == 1) {
                year += 2;
            }
        }
        if (S32(st, G_YEAR) >= year) {
            if (limit != 0) {
                U8(st, G_UE38DD9) = 0xFF;
            } else if (survive != 0 && U8(st, G_UE38DD9) == 0) {
                U8(st, G_UE38DD9) = 0xFF;
            }
        } else if (survive != 0) {
            U8(st, G_UE38DD9) = 0;
        }
    }
    /* @bin 0x004EA4D0 the city's ruin */
    int32_t left = S32(st, c + CITY_U4644) + S32(st, c + CITY_U4634);
    if (left > S32(st, c + CITY_U4678) + 2 && S32(st, c + CITY_POPULATION) < S32(st, c + CITY_POPULATION_HIGHEST) / 4) {
        U8(st, G_UE38DD9) = 0xFF;
        U8(st, G_UEA17FC) = 1;
    }
    if (left > 0 && S32(st, c + CITY_POPULATION) <= 0) {
        U8(st, G_UE38DD9) = 0xFF;
        U8(st, G_UEA17FC) = 1;
    }
    if (S32(st, G_EDITOR) == 1) {
        U8(st, G_UE38DD9) = 0;
    }
    if (goals <= 0) {
        U8(st, G_UE38DD9) = 0;
    }
    if (S32(st, c + CITY_U44D0) != 0) {
        U8(st, G_UE38DD9) = S32(st, c + CITY_U44D4) <= 0;
    }
    if (U8(st, G_CHEAT_WIN) != 0) {
        U8(st, G_UE38DD9) = 1;
    } else if (U8(st, G_UE38DD9) == 0) {
        return;
    }
    S32(st, G_CONSTRUCTION_UI_TYPE) = 0;
    U8(st, G_FAST_FORWARD) = 0;
    if (S8(st, G_UE38DD9) >= 1) {
        /* @bin 0x004EA59C won (after a call to the empty function 0x0051F780(0)) */
        GAME_CALL(st, FN_NOTHING, 0);
        uint32_t w = CURRENT_CITY(st) + CITY_U4718;
        if (S32(st, w) == 0) {
            S32(st, w) = 1;
            S32(st, G_MESSAGE_POPUP) = 1;
            U8(st, G_WINDOW) = 0x43;
            U8(st, G_REDRAW) = 1;
            return;
        }
        U8(st, G_BUTTONS) = 0;
        U8(st, G_RIGHT_RELEASED) = 0;
        U8(st, G_LEFT_RELEASED) = 0;
        S32(st, G_VIDEO_STATE) = 2;
        GAME_CALL0(st, FN_SCREEN_BUFFER_CLEAR);
        if (S8(st, G_MISSION) >= 10 && S32(st, G_CUSTOM) == 0) {
            ui_window(st, G_U5E4C0C);
            U8(st, G_CHEAT_WIN) = 0;
            return;
        }
        ui_window(st, S32(st, G_UE3907C) == 0 ? G_U5E4BF4 : G_U5E4BDC);
        S32(st, G_UE3907C) = S32(st, G_UE3907C) == 0;
        U8(st, G_CHEAT_WIN) = 0;
        return;
    }
    /* @bin 0x004EA6CD lost */
    if (S32(st, c + CITY_U4714) == 0 && !(U8(st, G_UEA17FC) == 0 && limit != 0 && S32(st, c + CITY_U47A8) > 0)) {
        S32(st, c + CITY_U4714) = 1;
        S32(st, G_MESSAGE_POPUP) = 1;
        GAME_CALL(st, FN_MESSAGE_POST, 0x137, 0, 0, 0, 0);
        U8(st, G_CHEAT_WIN) = 0;
        return;
    }
    U8(st, G_UEA1380) = (uint8_t) GAME_CALL0(st, FN_REPLAY_POSSIBLE); /* @bin 0x004EA920 */
    GAME_CALL(st, FN_FADE_START, 1, 0x33, 0x3E8);
    U8(st, G_CHEAT_WIN) = 0;
}

static int32_t run_counters_met(game_state *st, const int32_t *a)
{
    (void) a;
    return goals_counters_met(st);
}

static int32_t run_kind(game_state *st, const int32_t *a)
{
    return goal_kind_4f52f0(st, a[0]);
}

static int32_t run_houses(game_state *st, const int32_t *a)
{
    return houses_at_least(st, a[0]);
}

static int32_t run_4ea920(game_state *st, const int32_t *a)
{
    (void) a;
    U8(st, G_UEA1380) = (uint8_t) GAME_CALL0(st, FN_REPLAY_POSSIBLE);
    return 0;
}

static int32_t run_video_open(game_state *st, const int32_t *a)
{
    return ui_video_open(st, a[0], a[1], a[2], a[3], a[4]);
}

static int32_t run_goals(game_state *st, const int32_t *a)
{
    (void) a;
    goals_check(st);
    return 0;
}

const ported_function day_goals_ports[] = {
    {0x004EA930u, "goals_counters_met", 0, 8, 0, run_counters_met, 0},
    {0x004F52F0u, "goal_kind_4f52f0", 1, 8, 0, run_kind, 0},
    {0x004EA280u, "houses_at_least", 1, 32, 0, run_houses, 0},
    {0x004EA920u, "goals_4ea920", 0, 0, 0, run_4ea920, 0},
    {0x00538DC0u, "ui_video_open", 5, 32, 0, run_video_open, 0},
    {0x004EA2C0u, "goals_check", 0, 0, 0, run_goals, 0},
};
const int day_goals_ports_count = sizeof(day_goals_ports) / sizeof(day_goals_ports[0]);
