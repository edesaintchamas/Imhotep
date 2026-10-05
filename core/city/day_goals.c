/* Imhotep - the scenario's win and loss conditions, every tick (0x004EA2C0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day_goals.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define RESULT 0x00E38DD9u /* i8: 1 won, -1 (0xFF) lost, 0 neither */

int32_t goals_counters_met(game_state *st)
{
    /* @bin 0x004EA930 the indices of the byte table 0x004EA978 that are tested (as 0x00469D50) */
    static const uint8_t tested[] = {0, 7, 9, 12, 14, 16, 17, 18, 19, 22, 23, 24, 25, 27, 29};
    for (unsigned i = 0; i < sizeof(tested); i++) {
        uint32_t a = 0x00784E7Cu + 4u * tested[i];
        if (S32(st, a) < S32(st, a - 0x90)) {
            return 0;
        }
    }
    return 1;
}

int32_t goal_kind_4f52f0(game_state *st, int32_t i)
{
    /* @bin 0x004F52F0 */
    uint32_t rec = 0x005D19C8u + 16u * (uint32_t) (int32_t) S16(st, 0x00784DE0u + 2u * (uint32_t) i);
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
        if (U8(st, b) != 0 && U8(st, b + 5) != 0 && S16(st, b + 0x1C) != 0 && S16(st, b + 0x10) - 10 >= level) {
            n++;
        }
    }
    return n;
}

int32_t ui_video_open(game_state *st, int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5)
{
    /* @bin 0x00538DC0 when videos are on (0x00F54118), 0x00538E30 (the video player, platform)
     * with 0x00F53FFC := a5; on failure the window a5 is opened instead (0x00E38DDA) */
    if (S32(st, 0x00F54118u) == 0) {
        return 0;
    }
    S32(st, 0x00F53FFCu) = a5;
    if (GAME_CALL(st, 0x00538E30u, a1, a2, a3, a4) == 0) {
        U8(st, 0x00E38DDAu) = (uint8_t) a5;
        if (S32(st, 0x00F54118u) == 2) {
            U8(st, 0x00E38DCAu) = 1;
        }
        return 0;
    }
    U8(st, 0x00E38DDAu) = 0x13;
    return 1;
}

static void ui_window(game_state *st, uint32_t text)
{
    /* @bin 0x004EA60D the result window, centred on the screen (0x00E38DAC x 0x00E38DB0) */
    ui_video_open(st, (int32_t) text, (S32(st, 0x00E38DB0u) - 0x190) / 2, (S32(st, 0x00E38DACu) - 0x124) / 2, 1, 0x42);
}

void goals_check(game_state *st)
{
    /* @bin 0x004EA2C0 the goals of the scenario (0x00784C3C..0x00784C5C: culture, prosperity,
     * population, monuments, kingdom, house level), the time limit, the city's ruin; then the
     * result: a won city asks once (+0x4718) and shows its window; a lost one posts message
     * 0x137 once (+0x4714) or ends (0x0041AA20) */
    int32_t goals = 0;
    U8(st, 0x00EA17FCu) = 0;
    if (S16(st, 0x00784B60u) != 0) {
        return;
    }
    U8(st, RESULT) = 1;
    uint32_t c = CURRENT_CITY(st);
    if (U8(st, 0x00784C3Cu) != 0) {
        goals = 1;
        if (S32(st, c + 0x42CC) < S32(st, 0x00784C24u)) {
            U8(st, RESULT) = 0;
        }
    }
    if (U8(st, 0x00784C3Du) != 0) {
        goals++;
        if (S32(st, c + 0x42D0) < S32(st, 0x00784C28u)) {
            U8(st, RESULT) = 0;
        }
    }
    if (S32(st, 0x00784C58u) != 0) {
        goals++;
        if (S32(st, c + 0x24) < S32(st, 0x00784C5Cu)) {
            U8(st, RESULT) = 0;
        }
    }
    if (U8(st, 0x00784C3Eu) != 0) {
        goals++;
        if (S32(st, c + 0x42D4) < S32(st, 0x00784C2Cu)) {
            U8(st, RESULT) = 0;
        }
        if (!goals_counters_met(st)) {
            U8(st, RESULT) = 0;
        }
        for (int32_t i = 0; i < 3; i++) {
            goal_kind_4f52f0(st, i); /* its result is not used */
        }
        c = CURRENT_CITY(st);
    }
    if (U8(st, 0x00784C3Fu) != 0) {
        goals++;
        if (S32(st, c + 0x42D8) < S32(st, 0x00784C30u)) {
            U8(st, RESULT) = 0;
        }
    }
    if (S32(st, 0x00784C34u) != 0) {
        goals++;
        if (houses_at_least(st, S32(st, 0x00784C38u)) < S32(st, 0x00784C34u)) {
            U8(st, RESULT) = 0;
        }
        c = CURRENT_CITY(st);
    }
    if (S32(st, 0x00E38EBCu) == 0 && (int32_t) S8(st, 0x00E38EA8u) == S32(st, 0x005795E4u) && U8(st, 0x007027C3u) == 0 &&
        U8(st, 0x007027C2u) != 0) {
        if (houses_at_least(st, 9) > 0) {
            GAME_CALL(st, 0x00417AE0u, 0xB);
        }
        c = CURRENT_CITY(st);
    }
    int32_t limit = S32(st, 0x00784C48u), survive = S32(st, 0x00784C50u);
    int timed = 1;
    if (goals <= 0) {
        U8(st, RESULT) = 0;
        if (limit != 0) {
            goals++;
        }
        if (survive != 0) {
            goals++;
        }
    } else if (U8(st, RESULT) == 1 && survive == 0) {
        timed = 0;
    }
    if (timed) {
        /* @bin 0x004EA473 the year limit 0x0078D7B8, extended by +0x47A8 (0: 7 years, 1: 2) */
        int32_t year = S32(st, 0x0078D7B8u);
        if (limit != 0) {
            int32_t ext = S32(st, c + 0x47A8);
            if (ext == 0) {
                year += 7;
            } else if (ext == 1) {
                year += 2;
            }
        }
        if (S32(st, 0x0078D7BCu) >= year) {
            if (limit != 0) {
                U8(st, RESULT) = 0xFF;
            } else if (survive != 0 && U8(st, RESULT) == 0) {
                U8(st, RESULT) = 0xFF;
            }
        } else if (survive != 0) {
            U8(st, RESULT) = 0;
        }
    }
    /* @bin 0x004EA4D0 the city's ruin */
    int32_t left = S32(st, c + 0x4644) + S32(st, c + 0x4634);
    if (left > S32(st, c + 0x4678) + 2 && S32(st, c + 0x24) < S32(st, c + 0x469C) / 4) {
        U8(st, RESULT) = 0xFF;
        U8(st, 0x00EA17FCu) = 1;
    }
    if (left > 0 && S32(st, c + 0x24) <= 0) {
        U8(st, RESULT) = 0xFF;
        U8(st, 0x00EA17FCu) = 1;
    }
    if (S32(st, 0x00605A3Cu) == 1) {
        U8(st, RESULT) = 0;
    }
    if (goals <= 0) {
        U8(st, RESULT) = 0;
    }
    if (S32(st, c + 0x44D0) != 0) {
        U8(st, RESULT) = S32(st, c + 0x44D4) <= 0;
    }
    if (U8(st, 0x00E38DD8u) != 0) {
        U8(st, RESULT) = 1;
    } else if (U8(st, RESULT) == 0) {
        return;
    }
    S32(st, 0x006059E8u) = 0;
    U8(st, 0x00E3908Cu) = 0;
    if (S8(st, RESULT) >= 1) {
        /* @bin 0x004EA59C won (after a call to the empty function 0x0051F780(0)) */
        GAME_CALL(st, 0x0051F780u, 0);
        uint32_t w = CURRENT_CITY(st) + 0x4718;
        if (S32(st, w) == 0) {
            S32(st, w) = 1;
            S32(st, 0x00E950DCu) = 1;
            U8(st, 0x00E38DDAu) = 0x43;
            U8(st, 0x00E38DCAu) = 1;
            return;
        }
        U8(st, 0x00E38E07u) = 0;
        U8(st, 0x00E38DF8u) = 0;
        U8(st, 0x00E38E04u) = 0;
        S32(st, 0x00F54118u) = 2;
        GAME_CALL0(st, 0x004CF1C0u);
        if (S8(st, 0x00E38EA8u) >= 10 && S32(st, 0x00E38EBCu) == 0) {
            ui_window(st, 0x005E4C0Cu);
            U8(st, 0x00E38DD8u) = 0;
            return;
        }
        ui_window(st, S32(st, 0x00E3907Cu) == 0 ? 0x005E4BF4u : 0x005E4BDCu);
        S32(st, 0x00E3907Cu) = S32(st, 0x00E3907Cu) == 0;
        U8(st, 0x00E38DD8u) = 0;
        return;
    }
    /* @bin 0x004EA6CD lost */
    if (S32(st, c + 0x4714) == 0 && !(U8(st, 0x00EA17FCu) == 0 && limit != 0 && S32(st, c + 0x47A8) > 0)) {
        S32(st, c + 0x4714) = 1;
        S32(st, 0x00E950DCu) = 1;
        GAME_CALL(st, 0x004E0CF0u, 0x137, 0, 0, 0, 0);
        U8(st, 0x00E38DD8u) = 0;
        return;
    }
    U8(st, 0x00EA1380u) = (uint8_t) GAME_CALL0(st, 0x004183D0u); /* @bin 0x004EA920 */
    GAME_CALL(st, 0x0041AA20u, 1, 0x33, 0x3E8);
    U8(st, 0x00E38DD8u) = 0;
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
    U8(st, 0x00EA1380u) = (uint8_t) GAME_CALL0(st, 0x004183D0u);
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
