/* Imhotep - daily city-wide steps: ratings, debt, events (day dispatcher 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day_city.h"

#include "city/day_buildings.h"
#include "city/day_labour.h"
#include "city/goods.h"
#include "city/day_x87.h"
#include "random/crt_rand.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


uint32_t events_next_active(game_state *st, uint32_t self, uint32_t prev)
{
    /* @bin 0x0044BAF0 records with +2 == 1 and +0x32 set */
    int32_t i = prev == 0 ? 0 : P16(st, prev) + 1;
    for (; i < (int32_t) P32(st, self); i++) {
        uint32_t rec = self + 4 + 0x7Cu * (uint32_t) i;
        if (P8(st, rec + 2) == 1 && P8(st, rec + 0x32) != 0) {
            P16(st, rec) = (int16_t) i;
            return rec;
        }
    }
    return 0;
}

int32_t event_is_running(game_state *st, uint32_t rec)
{
    /* @bin 0x0044D4C0 +2 == 1, +0x32 set and +0x30 0 or 1 */
    return P8(st, rec + 2) == 1 && P8(st, rec + 0x32) != 0 && (uint32_t) (int32_t) P16(st, rec + 0x30) <= 1;
}

int32_t value_cap(int32_t kind, int32_t v)
{
    /* @bin 0x00559C60 */
    if (v < 0) {
        return 0;
    }
    int32_t cap;
    switch (kind) {
    case 0xA: case 0x1C: cap = 10000; break;
    case 0xC: case 0x18: case 0x19: case 0x1A: case 0x1E: cap = 30000; break;
    default: cap = 15000; break;
    }
    return v > cap ? cap : v;
}

int32_t buildings_serving(game_state *st, int32_t kind)
{
    /* @bin 0x00443080 the slot of each kind (switch at 0x00443092), then the used buildings with
     * workers (+0x40) of the counted types (as in 0x0045E6E0) whose +0x12 or +0xCE is that slot */
    int32_t slot;
    switch (kind) {
    case 0x2: slot = 0x15; break;
    case 0x8: slot = 0x16; break;
    case 0x9: slot = 0xD; break;
    case 0xA: slot = 8; break;
    case 0xB: slot = 5; break;
    case 0xC: slot = 7; break;
    case 0xD: slot = 0x18; break;
    case 0xE: slot = 9; break;
    case 0xF: slot = 0x13; break;
    case 0x10: slot = 1; break;
    case 0x11: slot = 0x10; break;
    case 0x12: slot = 0xA; break;
    case 0x14: slot = 4; break;
    case 0x15: slot = 0xF; break;
    case 0x16: slot = 0xB; break;
    case 0x17: slot = 0xC; break;
    case 0x18: slot = 3; break;
    case 0x19: slot = 6; break;
    case 0x1C: slot = 0xE; break;
    case 0x1D: slot = 0x11; break;
    case 0x1E: slot = 0x12; break;
    case 0x1F: slot = 0x17; break;
    case 0x20: slot = 0x14; break;
    case 0x21: slot = 0x19; break;
    case 0x22: slot = 0x1A; break;
    default: slot = -1; break;
    }
    int32_t count = 0, n = S32(st, G_BUILDING_HIGHEST);
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1 || S16(st, b + BLD_WORKERS) == 0) {
            continue;
        }
        int16_t type = S16(st, b + BLD_TYPE);
        if (type < 0x6E || type > 0x72) {
            switch (type) {
            case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2:
            case 0xBB: case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2:
            case 0xE7: case 0xE8: case 0xE9:
                break;
            default:
                continue;
            }
        }
        if (S16(st, b + BLD_SUBTYPE) == slot || S16(st, b + BLD_INPUT2) == slot) {
            count++;
        }
    }
    return count;
}

void day_step_0x02(game_state *st)
{
    /* @bin 0x00443330 35 values 0x00775814 (kinds 1..35): a base by kind plus 100 per unit of
     * the running events of that kind (+4, amount +0xC) and of 0x00784DEC - 0x00784E7C, capped by
     * 0x00559C60 */
    int32_t add[36] = {0};
    for (uint32_t rec = events_next_active(st, G_EVENTS, 0); rec != 0; rec = events_next_active(st, G_EVENTS, rec)) {
        if (event_is_running(st, rec)) {
            int32_t k = P16(st, rec + 4);
            if (k >= 1 && k < 0x24) {
                add[k] += P16(st, rec + 0xC) * 100;
            }
        }
    }
    for (int32_t i = 0; i < 0x23; i++) {
        int32_t kind = i + 1;
        add[kind] += (S32(st, G_GOAL_TARGETS + 4u * (uint32_t) i) - S32(st, G_COUNTERS + 4u * (uint32_t) i)) * 100;
        /* @bin 0x004433C3 the six kinds of table 0x005D2E8C (1: kinds 1..8) take their base from
         * the tables 0x007758A0.. by population (1000, 2500, 5000) */
        int32_t j, base;
        for (j = 0; j < 6; j++) {
            int32_t e = S32(st, G_U5D2E8C + 4u * (uint32_t) j);
            if ((e == 1 && i >= 0 && i <= 7) || e == kind) {
                break;
            }
        }
        if (j < 6) {
            int32_t pop = S32(st, CURRENT_CITY(st) + CITY_POPULATION);
            uint32_t table = pop < 1000 ? G_TRADE_RECOMMENDS : pop < 2500 ? G_TRADE_RECOMMENDS + 0x18 * 1 : pop < 5000 ? G_TRADE_RECOMMENDS + 0x18 * 2 : G_TRADE_RECOMMENDS + 0x18 * 3;
            base = S32(st, table + 4u * (uint32_t) j);
        } else {
            switch (i) {
            case 0x15: case 0xA: case 0x11: case 0xD: case 0xF: case 0x8: case 0x1C: case 0x13: case 0x1F:
                base = (buildings_serving(st, kind) + 1) * 200;
                break;
            case 0x17: case 0x18: case 0xB: case 0x19: case 0x1D:
                base = GAME_CALL(st, FN_MONUMENT_GOODS_NEEDED, kind) + 500;
                break;
            default:
                base = 1000;
                break;
            }
        }
        S32(st, G_U775814 + 4u * (uint32_t) i) = value_cap(kind, base + add[kind]);
    }
}

void finance_totals(game_state *st)
{
    /* @bin 0x0045A990 */
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + CITY_INCOME_THIS_YEAR) = S32(st, c + CITY_GOLD_THIS_YEAR) + S32(st, c + CITY_GIFTS_THIS_YEAR) +
                                         S32(st, c + CITY_TAXES_THIS_YEAR) + S32(st, c + CITY_EXPORTS_THIS_YEAR);
    S32(st, c + CITY_EXPENSES_THIS_YEAR) = S32(st, c + CITY_REQUESTS_THIS_YEAR) + S32(st, c + CITY_THEFT_THIS_YEAR) +
                                         S32(st, c + CITY_SALARY_THIS_YEAR) + S32(st, c + CITY_INTEREST_THIS_YEAR) +
                                         S32(st, c + CITY_CONSTRUCTION_THIS_YEAR) + S32(st, c + CITY_WAGES_THIS_YEAR) +
                                         S32(st, c + CITY_IMPORTS_THIS_YEAR);
    S32(st, c + CITY_NET_LAST_YEAR) = S32(st, c + CITY_INCOME_LAST_YEAR) - S32(st, c + CITY_EXPENSES_LAST_YEAR);
    int32_t net = S32(st, c + CITY_INCOME_THIS_YEAR) - S32(st, c + CITY_EXPENSES_THIS_YEAR);
    S32(st, c + CITY_NET_THIS_YEAR) = net;
    S32(st, c + CITY_BALANCE_THIS_YEAR) = S32(st, c + CITY_BALANCE_LAST_YEAR) + net;
}

void city_0x46e840(game_state *st)
{
    /* @bin 0x0046E840 */
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + CITY_RATING_PROSPERITY) -= 3;
    if (S32(st, c + CITY_RATING_PROSPERITY) < 0) {
        S32(st, c + CITY_RATING_PROSPERITY) = 0;
    }
    S32(st, c + CITY_RATING_PROSPERITY_REASON) = 8;
}

void event_stop(game_state *st, uint32_t rec)
{
    /* @bin 0x0044EF90 */
    uint32_t flags = P32(st, rec + 0x28);
    P8(st, rec + 0x32) = 0;
    P32(st, rec + 0x28) = flags | 4;
}

void day_step_0x04(game_state *st)
{
    /* @bin 0x004460B0 the treasury (+0xC): positive clears the debt flags; negative: a loan once
     * (0x00784C94 percent of table 0x005D1960 by difficulty 0x00E39080) with messages 0x73 / 0x75,
     * then message 0x74 once */
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + CITY_TREASURY) >= 0) {
        S32(st, c + CITY_U482C) = 0;
        S32(st, c + CITY_U4830) = 0;
        S32(st, c + CITY_U449C) = 0;
    } else if (S32(st, c + CITY_U4498) == 0) {
        int32_t loan = calc_percentage(S32(st, G_LOAN_PCT), S32(st, G_TREASURY_PCT + 4u * U32(st, G_DIFFICULTY)));
        S32(st, c + CITY_TREASURY) += loan;
        c = CURRENT_CITY(st);
        loan = calc_percentage(S32(st, G_LOAN_PCT), S32(st, G_TREASURY_PCT + 4u * U32(st, G_DIFFICULTY)));
        S32(st, c + CITY_GIFTS_THIS_YEAR) += loan;
        finance_totals(st);
        GAME_CALL0(st, FN_MONTH_0X0045A7B0);
        c = CURRENT_CITY(st);
        S32(st, c + CITY_U4498) = 1;
        if (S32(st, G_LOAN_PCT) > 0) {
            S32(st, G_MESSAGE_POPUP) = 1;
            if (S32(st, c + CITY_PLAYER_RANK) == 10) {
                GAME_CALL(st, FN_MESSAGE_POST, 0x75, 0, 0, 0, 0);
            } else {
                GAME_CALL(st, FN_MESSAGE_POST, 0x73, 0, 0, 0, 0);
                city_0x46e840(st);
            }
        }
    } else if (S32(st, c + CITY_U449C) == 0) {
        S32(st, c + CITY_U449C) = 1;
        S32(st, G_MESSAGE_POPUP) = 1;
        GAME_CALL(st, FN_MESSAGE_POST, 0x74, 0, 0, 0, 0);
    }
    /* @bin 0x004461CD the first event with flag 0x10 is stopped unless it has flag 4 */
    c = CURRENT_CITY(st);
    if (S32(st, c + CITY_RATING_KINGDOM) != 0) {
        return;
    }
    uint32_t rec = (uint32_t) GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT, 0, 0);
    while (rec != 0 && !((P32(st, rec + 0x28) >> 4) & 1)) {
        rec = (uint32_t) GAME_CALLT(st, G_EVENTS, FN_EVENTS_NEXT, (int32_t) rec, 0);
    }
    if (rec == 0 || ((P32(st, rec + 0x28) >> 2) & 1)) {
        return;
    }
    P16(st, rec + 0x16) = 0;
    P16(st, rec + 0x18) = -1;
    P16(st, rec + 0x1A) = -1;
    GAME_CALLT(st, rec, FN_EVENT_START, P16(st, rec), P16(st, rec + 4) != 3 ? 0xB5 : 0xAB);
    event_stop(st, rec);
    P8(st, rec + 3) = 0xC;
}

void day_city_ratings_4550e0(game_state *st)
{
    /* @bin 0x004550E0 day step 0x31: averages of four i8 of the houses (+0x80..+0x83 of used
     * records with +5) and, for types 0x1F, 0x1E, 0x21, 0x20, 0xE2, their parts (+0xA7..+0xA9)
     * done or still needed; the most needed one at +0x4380; four population thresholds */
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + CITY_U4384) = 0;
    for (uint32_t k = 0; k < 8; k++) {
        S32(st, c + CITY_U4360 + 4 * k) = 0;
    }
    S32(st, c + CITY_U44C8) = 0;
    S32(st, c + CITY_U44CC) = 0;
    S32(st, c + CITY_U4380) = 0;
    S32(st, c + CITY_U4518) = 0;
    S32(st, c + CITY_U451C) = 0;
    S32(st, c + CITY_U4520) = 0;
    int32_t n = 0;
    for (int32_t id = 1; id < 4000; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1 || U8(st, b + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        n++;
        S32(st, c + CITY_U4384) += S8(st, b + BLD_ENTERTAINMENT);
        S32(st, c + CITY_U4518) += S8(st, b + BLD_RELIGION);
        S32(st, c + CITY_U451C) += S8(st, b + BLD_EDUCATION);
        S32(st, c + CITY_U4520) += S8(st, b + BLD_HEALTH);
    }
    if (n != 0) {
        S32(st, c + CITY_U4384) /= n;
        S32(st, c + CITY_U4518) /= n;
        S32(st, c + CITY_U451C) /= n;
        S32(st, c + CITY_U4520) /= n;
    }
    for (int32_t id = 1; id < 4000; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        int16_t type = S16(st, b + BLD_TYPE);
        int32_t parts = type == 0x1F ? 1 : type == 0x1E ? 2 : type == 0x21 ? 3 : 0;
        if (parts != 0) {
            for (int32_t k = 0; k < parts; k++) {
                if (U8(st, b + BLD_UA7 + (uint32_t) k) != 0) {
                    S32(st, c + CITY_U4360 + 8 * (uint32_t) k) += 1;
                } else {
                    S32(st, c + CITY_U4364 + 8 * (uint32_t) k) += parts;
                }
            }
        } else if (type == 0x20) {
            if (U8(st, b + BLD_UA7) != 0) {
                S32(st, c + CITY_U4378) += 1;
            } else {
                S32(st, c + CITY_U437C) += 100;
            }
        } else if (type == 0xE2) {
            if (U8(st, b + BLD_UA7) != 0) {
                S32(st, c + CITY_U44C8) += 1;
            } else {
                S32(st, c + CITY_U44CC) += 100;
            }
        }
    }
    int32_t most = 0;
    if (S32(st, c + CITY_U4364) > 0) {
        most = S32(st, c + CITY_U4364);
        S32(st, c + CITY_U4380) = 1;
    }
    if (S32(st, c + CITY_U4364 + 8) > most) {
        most = S32(st, c + CITY_U4364 + 8);
        S32(st, c + CITY_U4380) = 2;
    }
    if (S32(st, c + CITY_U4364 + 16) > most) {
        most = S32(st, c + CITY_U4364 + 16);
        S32(st, c + CITY_U4380) = 3;
    }
    if (S32(st, c + CITY_U437C) > most) {
        most = S32(st, c + CITY_U437C);
        S32(st, c + CITY_U4380) = 4;
    }
    if (S32(st, c + CITY_U44CC) > most) {
        S32(st, c + CITY_U4380) = 5;
    }
    int32_t pop = S32(st, c + CITY_POPULATION);
    S32(st, c + CITY_FESTIVAL_COST_1) = pop / 20 + 10;
    S32(st, c + CITY_FESTIVAL_COST_2) = pop / 10 + 20;
    S32(st, c + CITY_FESTIVAL_COST_3) = pop / 5 + 40;
    S32(st, c + CITY_U4510) = (pop / 500 + 1) * 100;
    S32(st, c + CITY_U4514) = 0;
    if (S32(st, c + CITY_STORAGE_STOCK + 4 * 15) < S32(st, c + CITY_U4510)) {
        S32(st, c + CITY_U4514) = 1;
        if (S32(st, c + CITY_U44F4) == 3) {
            S32(st, c + CITY_U44F4) = 2;
        }
    }
}

int32_t god_building_type(int32_t god, int32_t kind)
{
    /* @bin 0x004C06E0 the building type of a god's temple (kind 0), shrine (1) or festival
     * building (2); -1 for others (god compared unsigned up to 9) */
    if ((uint32_t) god > 9) {
        return -1;
    }
    switch (kind) {
    case 0: return god < 5 ? 0x3C + god : 0x97 + (god - 5);
    case 1: return god < 5 ? 0x41 + god : 0x9C + (god - 5);
    case 2: return 0x8C + god;
    default: return -1;
    }
}

int32_t city_people_young_4e94a0(game_state *st, int32_t player)
{
    /* @bin 0x004E94A0 the first 14 i16 of the age table (+0x25C4) */
    int32_t sum = 0;
    for (uint32_t i = 0; i < 0xE; i++) {
        sum += S16(st, CITY_ADDR(player) + CITY_AGES + 2 * i);
    }
    return sum;
}

int32_t city_people_4e94d0(game_state *st, int32_t player)
{
    /* @bin 0x004E94D0 7 i16 of the age table from +0x25E0 */
    int32_t sum = 0;
    for (uint32_t i = 0; i < 7; i++) {
        sum += S16(st, CITY_ADDR(player) + CITY_AGES + 2 * 14 + 2 * i);
    }
    return sum;
}

static int32_t coverage(game_state *st, int32_t value)
{
    int32_t v = calc_ratio_percent(value, S32(st, CURRENT_CITY(st) + CITY_POPULATION));
    return v;
}

void day_coverage_468170(game_state *st)
{
    /* @bin 0x00468170 day step 0x21: coverage percentages of the advisors (0x0078D5F0..), by
     * building counts (0x007057B4.., 0x00705AC4) times their reach over the population, capped at
     * 100; the gods' coverage (0x0078D620, five gods of mode 0x007848E2 1 or 2: temples 8000,
     * shrines and festival buildings by mode) and their average 0x0078D60C (x87); two more from
     * the young people (0x004E94A0) and the population; then 0x004E7E60 */
    int32_t sum = 0, n = 0;
    S32(st, G_COVERAGE) = coverage(st, (int32_t) ((uint32_t) S32(st, G_U7057B8) * 400u));
    S32(st, G_COVERAGE + 4 * 1) = coverage(st, (int32_t) ((uint32_t) S32(st, G_U7057B4) * 700u));
    S32(st, G_COVERAGE + 4 * 2) = coverage(st, (int32_t) ((uint32_t) S32(st, G_U7057C0) * 1200u));
    S32(st, G_COVERAGE + 4 * 3) = coverage(st, (int32_t) ((uint32_t) S32(st, G_U7057BC) * 5000u));
    S32(st, G_COVERAGE + 4 * 4) = coverage(st, (int32_t) ((uint32_t) S32(st, G_U705AC4) * 7500u));
    for (uint32_t a = G_COVERAGE; a <= G_COVERAGE + 4 * 4; a += 4) {
        if (S32(st, a) > 100) {
            S32(st, a) = 100;
        }
    }
    for (int32_t god = 0; god < 5; god++) {
        int16_t mode = S16(st, G_GOD_MODE + 2u * (uint32_t) god);
        uint32_t out = G_GOD_COVERAGE + 4u * (uint32_t) god;
        if (mode == 2 || mode == 1) {
            /* @bin 0x004682EE weights 8000 / 375 / 150 (mode 2) or 8000 / 750 / 300 (mode 1) */
            int32_t t1 = S32(st, G_TYPE_COUNT + 4u * (uint32_t) god_building_type(god, 1));
            int32_t t0 = S32(st, G_TYPE_COUNT + 4u * (uint32_t) god_building_type(god, 0));
            int32_t t2 = S32(st, G_TYPE_COUNT + 4u * (uint32_t) god_building_type(god, 2));
            uint32_t v = (uint32_t) t1 * 8000u + (uint32_t) t0 * (mode == 2 ? 375u : 750u) +
                         (uint32_t) t2 * (mode == 2 ? 150u : 300u);
            S32(st, out) = coverage(st, (int32_t) v);
            n++;
        } else {
            S32(st, out) = 0;
        }
        if (S32(st, out) > 100) {
            S32(st, out) = 100;
        }
        sum += S32(st, out);
    }
    /* @bin 0x004683FF fild sum; fidiv n; __ftol (n = 0: the indefinite integer, low half 0) */
    S32(st, G_GOD_COVERAGE_AVERAGE) = n == 0 ? 0 : x87_ftol(x87_div(sum, n));
    int32_t player = S32(st, CURRENT_PLAYER_ADDR);
    S32(st, CITY_ADDR(player) + CITY_YOUNG) = city_people_young_4e94a0(st, player);
    player = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t c = CITY_ADDR(player);
    S32(st, c + CITY_U30) = city_people_4e94d0(st, player);
    S32(st, G_U78D604) = calc_ratio_percent((int32_t) ((uint32_t) S32(st, G_U705808) * 300u), S32(st, c + CITY_YOUNG));
    S32(st, G_U78D608) = coverage(st, (int32_t) ((uint32_t) S32(st, G_U705810) * 800u));
    if (S32(st, G_U78D604) > 100) {
        S32(st, G_U78D604) = 100;
    }
    if (S32(st, G_U78D608) > 100) {
        S32(st, G_U78D608) = 100;
    }
    houses_access_ratios(st, G_U78D614, G_U78D61C, G_U78D618, G_U78D610);
}

#define EMPIRE_CITY(i) (G_EMPIRE_CITIES + 0x6Au + 0x6Au * (uint32_t) ((i) - 1)) /* 60 records, from 1 */

int32_t trade_open_routes_559fc0(game_state *st, int32_t *kinds)
{
    /* @bin 0x00559FC0 kinds[S32(0x005D2A3C + 8 * +3) - 0x26] += 1 for the empire cities (used,
     * +5, +0x3D) that trade resource 0x13 while the city's +0x284C is 1 or 3; returns how many of
     * the 16 kinds are non-zero */
    for (int32_t i = 1; i <= 60; i++) {
        uint32_t e = EMPIRE_CITY(i);
        if (U8(st, e) == 0 || U8(st, e + 5) == 0) {
            continue;
        }
        int16_t mode = S16(st, CURRENT_CITY(st) + CITY_TRADE_STATUS + 2 * 0x13);
        if ((mode != 1 && mode != 3) || U8(st, e + 0x3D) == 0 || !empire_city_trades(st, i, 0x13)) {
            continue;
        }
        int32_t k = S32(st, G_CITY_KIND_TEXTS + 8u * U8(st, e + 3)) - 0x26;
        if (k < 0 || k >= 16) {
            state_unported(st, FN_TRADE_OPEN_ROUTES_559FC0); /* outside the caller's buffer: its stack */
            return 0;
        }
        kinds[k] += 1;
    }
    int32_t n = 0;
    for (int k = 0; k < 16; k++) {
        n += kinds[k] != 0;
    }
    return n;
}

void day_trade_444c20(game_state *st)
{
    /* @bin 0x00444C20 day step 0x20: the trade routes. Land (+0x58 == 1) and sea routes open
     * per city are counted (+0x447C / +0x447E); each open trading city scores its traded goods
     * (0x0077646C by its +4 kind: > 25: 3, > 15: 2, > 0: 1), averaged up; when its next slot
     * (+0x5A, +0x5C, +0x5E) is free and its delay +0x52 has run out (reset to 30 land, 4 sea),
     * one caravan (type 0x14) or ship (0x13 with two 0x15) leaves, and the step ends */
    int32_t kinds[16] = {0};
    int32_t player = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t c = CITY_ADDR(player);
    S16(st, c + CITY_U447C) = 0;
    S16(st, c + CITY_U447E) = 0;
    S32(st, c + CITY_U46A4) = 0;
    if (S32(st, G_U78D6DC + 4 * 19) > 0) {
        kinds[0] = 1;
    }
    S32(st, c + CITY_U46A4) += trade_open_routes_559fc0(st, kinds);
    S16(st, c + CITY_U4480) = S16(st, c + CITY_U4480) > 0 ? (int16_t) (S16(st, c + CITY_U4480) - 1) : 0;
    S16(st, c + CITY_U4482) = S16(st, c + CITY_U4482) > 0 ? (int16_t) (S16(st, c + CITY_U4482) - 1) : 0;
    for (int32_t i = 1; i <= 60; i++) {
        uint32_t e = EMPIRE_CITY(i);
        if (U8(st, e) == 0 || U8(st, e + 5) == 0) {
            continue;
        }
        int land = U8(st, e + 0x58) == 1;
        if (land) {
            if (S32(st, G_DOCK_COUNT) <= 0) {
                /* @bin 0x00444D27 no trade building: message 0x13C every 384 days */
                if (S32(st, G_MESSAGE_COUNTERS + 4 * 10) <= 0) {
                    S32(st, G_MESSAGE_POPUP) = 1;
                    GAME_CALL(st, FN_MESSAGE_POST, 0x13C, 0, 0, 0, 0);
                    S32(st, G_MESSAGE_COUNTERS + 4 * 10) = 0x180;
                } else {
                    S32(st, G_MESSAGE_COUNTERS + 4 * 10) -= 1;
                }
                continue;
            }
            if (S16(st, G_ENTRY_X) == -1 && S16(st, G_ENTRY_Y) == -1) {
                continue;
            }
            S16(st, CURRENT_CITY(st) + CITY_U447C) += 1;
        } else {
            S16(st, CURRENT_CITY(st) + CITY_U447E) += 1;
        }
        int32_t score = 0, traded = 0;
        for (int32_t g = 1; g < 0x24; g++) {
            if ((U8(st, e + 6 + (uint32_t) g) != 0 && empire_city_trades(st, i, g)) ||
                (U8(st, e + 0x2A + (uint32_t) g) != 0 && empire_city_trades(st, i, g))) {
                traded++;
                int32_t v = S32(st, G_TRADE_ROUTE_LIMITS + 4u * ((uint32_t) g + 0x24u * U8(st, e + 4)));
                score += v > 0x19 ? 3 : v > 0xF ? 2 : v > 0 ? 1 : 0;
            }
        }
        if (traded > 1) {
            score = score / traded + (score % traded != 0);
        }
        if (score <= 0 || S16(st, e + 0x54) != 0) {
            continue;
        }
        int32_t slot;
        if (score == 1) {
            if (S16(st, e + 0x5A) != 0) {
                continue;
            }
            slot = 0;
        } else if (score == 2) {
            if (S16(st, e + 0x5A) == 0) {
                slot = 0;
            } else if (S16(st, e + 0x5C) != 0) {
                continue;
            } else {
                slot = 1;
            }
        } else {
            if (S16(st, e + 0x5A) == 0) {
                slot = 0;
            } else if (S16(st, e + 0x5C) == 0) {
                slot = 1;
            } else if (S16(st, e + 0x5E) != 0) {
                continue;
            } else {
                slot = 2;
            }
        }
        if (S16(st, e + 0x52) > 0) {
            S16(st, e + 0x52) -= 1;
            continue;
        }
        S16(st, e + 0x52) = U8(st, e + 0x58) != 1 ? 4 : 0x1E;
        uint32_t trader = G_EMPIRE_CITIES + 0x5Au + 2u * (uint32_t) (i * 53 + slot);
        if (U8(st, e + 0x58) == 1) {
            if (S32(st, G_DOCK_COUNT) <= 0) {
                continue;
            }
            int16_t x = S16(st, G_ENTRY_X);
            if (x == -1 && S16(st, G_ENTRY_Y) == -1) {
                continue;
            }
            player = S32(st, CURRENT_PLAYER_ADDR);
            c = CITY_ADDR(player);
            if (S16(st, c + CITY_U4480) != 0 || S32(st, c + CITY_U47B4) > 0) {
                continue;
            }
            int32_t f = GAME_CALL(st, FN_FIGURE_CREATE, player, 0x14, x, S16(st, G_ENTRY_Y), 0);
            uint32_t fa = FIGURE_ADDR(f);
            S16(st, fa + FIG_AMOUNT) = 0xC;
            U8(st, fa + FIG_U6F) = 0;
            U8(st, fa + FIG_U6E) = (uint8_t) i;
            U8(st, fa + FIG_ACTION) = 8;
            S16(st, fa + FIG_WAIT_TICKS) = 0xA;
            S16(st, trader) = (int16_t) f;
            U8(st, fa + FIG_COLLECTING) = 0;
            return;
        }
        player = S32(st, CURRENT_PLAYER_ADDR);
        c = CITY_ADDR(player);
        if (S16(st, c + CITY_U4482) != 0 || S32(st, c + CITY_U47B4) > 0) {
            continue;
        }
        int32_t ship = GAME_CALL(st, FN_FIGURE_CREATE, player, 0x13, S16(st, c + CITY_ENTRY_X), S16(st, c + CITY_ENTRY_Y), 0);
        S16(st, trader) = (int16_t) ship;
        U8(st, FIGURE_ADDR(ship) + FIG_U6E) = (uint8_t) i;
        U8(st, FIGURE_ADDR(ship) + FIG_ACTION) = 8;
        S16(st, FIGURE_ADDR(ship) + FIG_WAIT_TICKS) = 0xA;
        int32_t prev = ship;
        for (int k = 0; k < 2; k++) {
            player = S32(st, CURRENT_PLAYER_ADDR);
            c = CITY_ADDR(player);
            int32_t f = GAME_CALL(st, FN_FIGURE_CREATE, player, 0x15, S16(st, c + CITY_ENTRY_X), S16(st, c + CITY_ENTRY_Y), 0);
            U8(st, FIGURE_ADDR(f) + FIG_ACTION) = 8;
            S16(st, FIGURE_ADDR(f) + FIG_LEADER) = (int16_t) prev;
            prev = f;
        }
        return;
    }
}

#define GODS 5
#define GOD_MODE(g) (G_GOD_MODE + 2u * (uint32_t) (g)) /* i16: 0 absent, 1 minor, 2 patron */

void city_month_timers_4c51d0(game_state *st)
{
    /* @bin 0x004C51D0 three i16 (+0x4984..+0x4988) and two i32 (+0x45CC, +0x47B4) timers of
     * the current city count down while not negative */
    uint32_t c = CURRENT_CITY(st);
    for (uint32_t k = 0; k < 3; k++) {
        if (S16(st, c + CITY_U4984 + 2 * k) >= 0) {
            S16(st, c + CITY_U4984 + 2 * k) -= 1;
        }
    }
    if (S32(st, c + CITY_U45CC) >= 0) {
        S32(st, c + CITY_U45CC) -= 1;
    }
    if (S32(st, c + CITY_U47B4) >= 0) {
        S32(st, c + CITY_U47B4) -= 1;
    }
}

int32_t gods_most_angry(game_state *st)
{
    /* @bin 0x004E9F50 the god (1..5) with the most wrath, else the one in the worst mood below
     * 40, into +0x4408; whether there is one */
    int32_t player = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t c = CITY_ADDR(player);
    int32_t god = 0, best = 0;
    for (int32_t g = 0; g < GODS; g++) {
        if (S16(st, GOD_MODE(g)) != 0 && S8(st, c + CITY_GOD_WRATH + (uint32_t) g) > best) {
            god = g + 1;
            best = S8(st, c + CITY_GOD_WRATH + (uint32_t) g);
        }
    }
    if (god > 0) {
        S32(st, c + CITY_U4408) = god;
        return 1;
    }
    best = 0x28;
    for (int32_t g = 0; g < GODS; g++) {
        if (S16(st, GOD_MODE(g)) != 0 && S8(st, c + CITY_GOD_MOOD + (uint32_t) g) < best) {
            god = g + 1;
            best = S8(st, c + CITY_GOD_MOOD + (uint32_t) g);
        }
    }
    S32(st, c + CITY_U4408) = god;
    return god > 0;
}

void gods_update_4e9890(game_state *st, int32_t moods)
{
    /* @bin 0x004E9890 day step 1 (moods = 1): the gods' target moods from their coverage
     * (0x0078D620) and festivals, clamped by population; with `moods`, each mood steps toward
     * its target, one random god gains blessing or wrath by its mood (tables 0x0056F720.. by
     * difficulty 0x00E39080); on day 0 the targets are scaled by the kingdom's temple complex
     * (+0x4478, +0x472C) and 0x004C47A0 runs the gods' actions */
    if (S32(st, G_CUSTOM) == 0 && U8(st, G_MISSION_LIST_FLAGS + 4) == 0) {
        uint32_t c = CURRENT_CITY(st);
        for (uint32_t g = 0; g < GODS; g++) {
            U8(st, c + CITY_GOD_TARGET_MOOD + g) = 0x1E;
            U8(st, c + CITY_GOD_MOOD + g) = 0x1E;
        }
        return;
    }
    int32_t player = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t c = CITY_ADDR(player);
    for (uint32_t g = 0; g < GODS; g++) {
        if (S16(st, GOD_MODE(g)) != 0) {
            U8(st, c + CITY_GOD_TARGET_MOOD + g) = U8(st, G_GOD_COVERAGE + 4 * g);
        }
    }
    for (uint32_t g = 0; g < GODS; g++) {
        if (S16(st, GOD_MODE(g)) != 0) {
            int32_t months = S32(st, c + CITY_GOD_MONTHS + 4 * g);
            if (months > 0x28) {
                months = 0x28;
            }
            U8(st, c + CITY_GOD_TARGET_MOOD + g) += (uint8_t) (0xC - months);
        }
    }
    int32_t pop = S32(st, c + CITY_POPULATION);
    int32_t low, high;
    if (pop < 400) {
        low = 50, high = 50;
    } else if (pop < 450) {
        low = 40, high = 60;
    } else if (pop < 500) {
        low = 30, high = 70;
    } else if (pop < 550) {
        low = 20, high = 80;
    } else if (pop < 600) {
        low = 10, high = 90;
    } else {
        low = 0, high = 100;
    }
    for (uint32_t g = 0; g < GODS; g++) {
        if (S16(st, GOD_MODE(g)) == 0) {
            continue;
        }
        uint32_t t = c + CITY_GOD_TARGET_MOOD + g;
        if (S8(st, t) > 100) {
            S8(st, t) = 100;
        }
        if (S8(st, t) < low) {
            S8(st, t) = (int8_t) low;
        }
        if (S8(st, t) > high) {
            S8(st, t) = (int8_t) high;
        }
    }
    if (moods == 0) {
        return;
    }
    uint8_t campaign = U8(st, G_FLOW2_PERIOD);
    int32_t editor = S32(st, G_CUSTOM);
    for (uint32_t g = 0; g < GODS; g++) {
        if (S16(st, GOD_MODE(g)) == 0) {
            continue;
        }
        uint32_t m = c + CITY_GOD_MOOD + g;
        int8_t mood = S8(st, m), target = S8(st, c + CITY_GOD_TARGET_MOOD + g);
        if (mood > target) {
            S8(st, m) = (int8_t) (mood - 1);
        } else if (mood < target) {
            S8(st, m) = (int8_t) (mood + 1);
        }
        if (U8(st, G_MISSION) == 0 && editor == 0 && campaign == 0 && S8(st, m) < 0x32) {
            S8(st, m) = 0x32;
        }
    }
    int32_t god = crt_rand(st) % 5;
    uint32_t c2 = CURRENT_CITY(st);
    if (god <= 4 && S16(st, GOD_MODE(god)) != 0) {
        /* @bin 0x004E9AD4 */
        uint32_t m = c2 + (uint32_t) god;
        int8_t mood = S8(st, m + CITY_GOD_MOOD);
        uint32_t d = 4u * U32(st, G_DIFFICULTY);
        uint32_t table = mood >= 0x5A ? G_U56F720 : mood >= 0x50 ? G_U56F734 : mood >= 0x46 ? G_U56F748
                         : mood > 0x32 ? G_U56F75C : 0;
        if (mood > 0x32) {
            U8(st, m + CITY_GOD_BLESSING) += U8(st, table + d);
            U8(st, m + CITY_GOD_WRATH) = 0;
        } else if (mood == 0x32) {
            U8(st, m + CITY_GOD_WRATH) = 0;
        } else {
            table = mood > 0x1E ? G_U56F784 : mood > 0x14 ? G_U56F798 : mood > 0xA ? G_U56F7AC : G_U56F7C0;
            U8(st, m + CITY_GOD_BLESSING) = 0;
            U8(st, m + CITY_GOD_WRATH) += U8(st, table + d);
        }
        if (S8(st, m + CITY_GOD_WRATH) >= 0x32) {
            U8(st, m + CITY_GOD_WRATH) = 0x32;
        }
        if (S8(st, m + CITY_GOD_BLESSING) >= 0x32) {
            U8(st, m + CITY_GOD_BLESSING) = 0x32;
        }
    }
    for (uint32_t g = 0; g < GODS; g++) {
        int8_t mood = S8(st, c2 + CITY_GOD_MOOD + g);
        if (mood < 0x32) {
            U8(st, c2 + CITY_GOD_BLESSING + g) = 0;
        } else if (mood > 0x32) {
            U8(st, c2 + CITY_GOD_WRATH + g) = 0;
        }
    }
    if (S32(st, G_CUSTOM) == 0 && (int32_t) S8(st, G_MISSION) == S32(st, G_MISSION_LISTS + G_MISSION_LIST_STRIDE * 5) && U8(st, G_MISSION_LIST_FLAGS + 5) == 0 &&
        U8(st, G_MISSION_LIST_FLAGS + 4) != 0 && S8(st, c2 + CITY_GOD_MOOD + 4) >= 0x28) {
        GAME_CALL(st, FN_TUTORIAL_STEP, 5);
    }
    if (S32(st, G_TICK_DAY) != 0) {
        return;
    }
    /* @bin 0x004E9C79 the first day of the month */
    city_month_timers_4c51d0(st);
    uint32_t c3 = CURRENT_CITY(st);
    if (S16(st, c3 + CITY_U4478) == 0) {
        for (uint32_t g = 0; g < GODS; g++) {
            if (S16(st, GOD_MODE(g)) == 2) {
                S8(st, c3 + CITY_GOD_TARGET_MOOD + g) = (int8_t) ((S8(st, c3 + CITY_GOD_TARGET_MOOD + g) * 98 + 50) / 100);
            }
        }
    } else {
        for (uint32_t g = 0; g < GODS; g++) {
            int16_t mode = S16(st, GOD_MODE(g));
            if (mode == 0) {
                continue;
            }
            uint32_t c4 = CURRENT_CITY(st);
            int32_t shrine = god_building_type((int32_t) g, 1);
            int32_t t = S8(st, c4 + CITY_GOD_TARGET_MOOD + g), v;
            if (S16(st, BUILDING_ADDR(S32(st, c4 + CITY_U472C)) + BLD_TYPE) == shrine) {
                v = (t * 115 + 50) / 100;
            } else if (mode == 2) {
                v = (t * 90 + 50) / 100;
            } else if (mode == 1) {
                v = (t * 95 + 50) / 100;
            } else {
                continue;
            }
            S8(st, c4 + CITY_GOD_TARGET_MOOD + g) = (int8_t) v;
        }
    }
    for (uint32_t g = 0; g < GODS; g++) {
        if (S16(st, GOD_MODE(g)) == 0) {
            continue;
        }
        uint32_t t = CURRENT_CITY(st) + CITY_GOD_TARGET_MOOD + g;
        if (S8(st, t) > 100) {
            S8(st, t) = 100;
        }
        if (S8(st, t) < low) {
            S8(st, t) = (int8_t) low;
        }
    }
    uint32_t c5 = CURRENT_CITY(st);
    for (uint32_t g = 0; g < GODS; g++) {
        if (S16(st, GOD_MODE(g)) != 0) {
            S32(st, c5 + CITY_GOD_MONTHS + 4 * g) += 1;
        }
    }
    if (god > 4 && gods_most_angry(st) != 0) {
        god = S32(st, CURRENT_CITY(st) + CITY_U4408) - 1;
    }
    if (S32(st, G_UE39084) == 0) {
        return;
    }
    int32_t kind = crt_rand(st) % 3;
    GAME_CALL(st, FN_GOD_ACT, god, kind, -1);
    uint32_t c6 = CURRENT_CITY(st);
    int32_t lowest = 100;
    for (uint32_t g = 0; g < GODS; g++) {
        if (S16(st, GOD_MODE(g)) != 0 && S8(st, c6 + CITY_GOD_MOOD + g) < lowest) {
            lowest = S8(st, c6 + CITY_GOD_MOOD + g);
        }
    }
    uint32_t a = c6 + CITY_U45D8;
    if (S32(st, a) != 0) {
        S32(st, a) -= 1;
    } else if (lowest < 0x1E) {
        S32(st, a) = 0x14;
    }
}

static int32_t run_next_active(game_state *st, const int32_t *a)
{
    return (int32_t) events_next_active(st, G_EVENTS, (uint32_t) a[0]);
}

static int32_t run_is_running(game_state *st, const int32_t *a)
{
    return event_is_running(st, (uint32_t) a[PORT_THIS_ARG]);
}

static int32_t run_event_stop(game_state *st, const int32_t *a)
{
    event_stop(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

static int32_t run_value_cap(game_state *st, const int32_t *a)
{
    (void) st;
    return value_cap(a[0], a[1]);
}

static int32_t run_serving(game_state *st, const int32_t *a)
{
    return buildings_serving(st, a[0]);
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_step_0x02, day_step_0x02)
VOID0(run_finance_totals, finance_totals)
VOID0(run_46e840, city_0x46e840)
VOID0(run_step_0x04, day_step_0x04)
VOID0(run_ratings_4550e0, day_city_ratings_4550e0)
VOID0(run_coverage, day_coverage_468170)
VOID0(run_trade_444c20, day_trade_444c20)

static int32_t run_month_timers(game_state *st, const int32_t *a)
{
    (void) a;
    city_month_timers_4c51d0(st);
    return 0;
}

static int32_t run_most_angry(game_state *st, const int32_t *a)
{
    (void) a;
    return gods_most_angry(st);
}

static int32_t run_gods(game_state *st, const int32_t *a)
{
    gods_update_4e9890(st, a[0]);
    return 0;
}

static int32_t run_god_type(game_state *st, const int32_t *a)
{
    (void) st;
    return god_building_type(a[0], a[1]);
}

static int32_t run_young(game_state *st, const int32_t *a)
{
    return city_people_young_4e94a0(st, a[0]);
}

static int32_t run_people_4e94d0(game_state *st, const int32_t *a)
{
    return city_people_4e94d0(st, a[0]);
}

const ported_function day_city_ports[] = {
    {0x0044BAF0u, "events_next_active", 1, 32, G_EVENTS, run_next_active, 0},
    {0x0044D4C0u, "event_is_running", 0, 8, 0, run_is_running, 0},
    {0x0044EF90u, "event_stop", 0, 0, 0, run_event_stop, 0},
    {0x00559C60u, "value_cap", 2, 32, 0, run_value_cap, 0},
    {0x00443080u, "buildings_serving", 1, 32, 0, run_serving, 0},
    {0x00443330u, "day_step_0x02", 0, 0, 0, run_step_0x02, 0},
    {0x0045A990u, "finance_totals", 0, 0, 0, run_finance_totals, 0},
    {0x0046E840u, "city_0x46e840", 0, 0, 0, run_46e840, 0},
    {0x004460B0u, "day_step_0x04", 0, 0, 0, run_step_0x04, 0},
    {0x004C06E0u, "god_building_type", 2, 32, 0, run_god_type, 0},
    {0x004E94A0u, "city_people_young_4e94a0", 1, 32, 0, run_young, 0},
    {0x004E94D0u, "city_people_4e94d0", 1, 32, 0, run_people_4e94d0, 0},
    {0x00468170u, "day_coverage_468170", 0, 0, 0, run_coverage, 0},
    {0x00559FC0u, "trade_open_routes_559fc0 (in day_city.c: its argument is a stack buffer)", 1, 32, 0, NULL, 0},
    {0x00444C20u, "day_trade_444c20", 0, 0, 0, run_trade_444c20, 0},
    {0x004C51D0u, "city_month_timers_4c51d0", 0, 0, 0, run_month_timers, 0},
    {0x004E9F50u, "gods_most_angry", 0, 32, 0, run_most_angry, 0},
    {0x004E9890u, "gods_update_4e9890", 1, 0, 0, run_gods, 0},
    {0x004550E0u, "day_city_ratings_4550e0", 0, 0, 0, run_ratings_4550e0, 0},
};
const int day_city_ports_count = sizeof(day_city_ports) / sizeof(day_city_ports[0]);
