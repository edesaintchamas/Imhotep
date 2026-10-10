/* Imhotep - opening an advisor (package cmdrest, docs/commands.md 3.10).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x0040A0E0 opens the advisor window 0x00D3B198 (also reached from the military advisor's
 * dispatch 0x0040A450 with advisor 4). Before the window is drawn the original refreshes the
 * figures the advisors show (0x0040A160): the labour allocation, the finance estimates, the
 * ratings and the city's summary flags; these are city data, so the command writes simulation
 * state and is ported here with them. */
#include "command/advisor.h"

#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static void advisor_rating_flag(game_state *st)
{
    /* @bin 0x00426B00 CITY+0x4434 := 0x00705738 */
    S32(st, CURRENT_CITY(st) + CITY_RATING_CULTURE_REASON) = S32(st, G_RATING_LOWEST_INDEX);
}

void advisor_complex_level(game_state *st)
{
    /* @bin 0x0045A930 with a temple complex (i16 CITY+0x4478), its level (0x004C5120, result not
     * kept); then CITY+0x2C7C := CITY+0x2C74 */
    if (S16(st, CURRENT_CITY(st) + CITY_TEMPLE_COMPLEX) != 0) {
        GAME_CALL(st, FN_BUILDING_TYPE_LEVEL, S32(st, CURRENT_CITY(st) + CITY_TEMPLE_COMPLEX_ID));
    }
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + CITY_INTEREST_THIS_YEAR) = S32(st, c + CITY_U2C74);
}

void advisor_summary_flags(game_state *st)
{
    /* @bin 0x004E9500 the city's summary codes (meanings assumed: what the advisors show first) */
    uint32_t c = CURRENT_CITY(st);
    int32_t v = S32(st, c + CITY_SENTIMENT_LOW);
    int32_t code;
    switch (v) {
    case 1: code = 2; break;
    case 2: code = 1; break;
    case 3: code = 3; break;
    case 4: code = 0; break;
    case 5: code = 4; break;
    default: code = 5; break;
    }
    S32(st, c + CITY_U4410) = code;
    /* @bin 0x004E956D the largest of four values (signed), by its index */
    S32(st, c + CITY_U441C) = 0;
    int32_t a = S32(st, c + CITY_U4328);
    int32_t m = 0;
    if (a > 0 || S32(st, c + CITY_U474C) > 0) {
        if (a < S32(st, c + CITY_U474C)) {
            S32(st, c + CITY_U441C) = 4;
            m = S32(st, c + CITY_U474C);
        } else {
            S32(st, c + CITY_U441C) = 1;
            m = S32(st, c + CITY_U4328);
        }
    }
    if (S32(st, c + CITY_U4324) > m) {
        S32(st, c + CITY_U441C) = 2;
        m = S32(st, c + CITY_U4324);
    }
    if (S32(st, c + CITY_U433C) > m) {
        S32(st, c + CITY_U441C) = 3;
    }
    /* @bin 0x004E95D3 */
    S32(st, c + CITY_U4424) = 0;
    int32_t p = S32(st, c + CITY_U4310), q = S32(st, c + CITY_U4314);
    if (p > q) {
        S32(st, c + CITY_U4424) = 1;
    } else if (p < q) {
        S32(st, c + CITY_U4424) = 2;
    } else if (p != 0 || q != 0) {
        S32(st, c + CITY_U4424) = 3;
    }
    /* @bin 0x004E960D */
    S32(st, c + CITY_U4428) = 0;
    int32_t r = S32(st, c + CITY_U430C);
    if (S32(st, c + CITY_U4388) > r) {
        S32(st, c + CITY_U4428) = 1;
    } else if (r != 0) {
        S32(st, c + CITY_U4428) = 2;
    }
    /* @bin 0x004E9633 */
    S32(st, c + CITY_U4420) = 0;
    m = 0;
    if (S32(st, c + CITY_U4350) > 0) {
        S32(st, c + CITY_U4420) = 1;
        m = S32(st, c + CITY_U4350);
    }
    if (S32(st, c + CITY_U4354) > m) {
        S32(st, c + CITY_U4420) = 2;
        m = S32(st, c + CITY_U4354);
    }
    if (S32(st, c + CITY_U4358) > m) {
        S32(st, c + CITY_U4420) = 3;
    }
}

void advisor_houses_count(game_state *st)
{
    /* @bin 0x004F6AD0 0x00EADB28 := 0x004EA280(0x00784C38) */
    S32(st, G_UEADB28) = GAME_CALL(st, FN_HOUSES_AT_LEAST, S32(st, G_GOAL_HOUSE_LEVEL));
}

void advisor_ratings(game_state *st)
{
    /* @bin 0x0046E1D0 */
    GAME_CALL0(st, FN_RATING_0X4269B0_46E1C0);
    advisor_rating_flag(st);
}

void advisors_refresh(game_state *st)
{
    /* @bin 0x0040A160 the advisors' figures, in this order */
    GAME_CALL0(st, FN_LABOUR_CATEGORIES_FILL);
    GAME_CALL0(st, FN_LABOUR_BUILDING_SHARES);
    GAME_CALL0(st, FN_NOTHING);
    GAME_CALL0(st, FN_NOTHING);
    GAME_CALL0(st, FN_LABOUR_ALLOCATE);
    GAME_CALL0(st, FN_TAX_INCOME_ESTIMATE);
    GAME_CALL0(st, FN_WAGES_ESTIMATE);
    advisor_complex_level(st);
    GAME_CALL0(st, FN_SALARY_ESTIMATE);
    GAME_CALL0(st, FN_FINANCE_TOTALS);
    GAME_CALL0(st, FN_MONTH_0X0045A7B0);
    advisor_summary_flags(st);
    GAME_CALL0(st, FN_DAY_COVERAGE_468170);
    GAME_CALL0(st, FN_DAY_STEP_0X11);
    GAME_CALL0(st, FN_FORMATIONS_REBUILD);
    GAME_CALL0(st, FN_SHIPS_COUNT);
    advisor_ratings(st);
    GAME_CALL0(st, FN_RATING_0X46E410);
    GAME_CALL0(st, FN_RATING_0X46EC70);
    advisor_houses_count(st);
}

void advisor_open(game_state *st)
{
    /* @bin 0x0040A0E0 an advisor the mission does not offer (outside custom games) posts notice
     * 0xD (closing the message window 0x004C7F30 first when it is open, window 5) */
    int32_t custom = S32(st, G_CUSTOM);
    int32_t adv = S32(st, G_PARAM1);
    if (custom == 0 && U8(st, G_ADVISOR_AVAILABLE + (uint32_t) adv) == 0) {
        if (U8(st, G_WINDOW) == 5) {
            GAME_CALL0(st, FN_MESSAGE_WINDOW_CLOSE);
            U8(st, G_WINDOW) = 1;
        }
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0xD, 0, 0);
        return;
    }
    uint32_t c = CURRENT_CITY(st);
    U8(st, G_WINDOW) = 7;
    S32(st, G_PAGE) = adv;
    S32(st, G_LAST_ADVISOR) = adv;
    U8(st, c + CITY_U49CC) = 0;
    advisors_refresh(st);
    U8(st, G_REDRAW) = 1;
    S32(st, G_ADVISOR_SLIDE) = 6;
}

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_open, advisor_open)
V(run_refresh, advisors_refresh)
V(run_complex, advisor_complex_level)
V(run_summary, advisor_summary_flags)
V(run_houses, advisor_houses_count)
V(run_ratings, advisor_ratings)
V(run_rating_flag, advisor_rating_flag)

const ported_function advisor_ports[] = {
    {0x0040A0E0u, "advisor_open", 0, 0, 0, run_open, 0},
    {0x0040A160u, "advisors_refresh", 0, 0, 0, run_refresh, 0},
    {0x0045A930u, "advisor_complex_level", 0, 0, 0, run_complex, 0},
    {0x004E9500u, "advisor_summary_flags", 0, 0, 0, run_summary, 0},
    {0x004F6AD0u, "advisor_houses_count", 0, 0, 0, run_houses, 0},
    {0x0046E1D0u, "advisor_ratings", 0, 0, 0, run_ratings, 0},
    {0x00426B00u, "advisor_rating_flag", 0, 0, 0, run_rating_flag, 0},
};
const int advisor_ports_count = sizeof(advisor_ports) / sizeof(advisor_ports[0]);
