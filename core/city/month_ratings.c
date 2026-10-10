/* Imhotep - the ratings of the month (0x0046E180 and its callees).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * City fields by their offset in the city record; their meaning is not established. */
#include "city/month_ratings.h"

#include "city/month.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define CITY(st) CURRENT_CITY(st)
#define RATING_REC 0x38u
#define RATING_RECS 11

static int32_t rating_input(game_state *st, uint32_t r)
{
    return S32(st, G_COVERAGE + 4u * U32(st, r));
}

void rating_lowest(game_state *st)
{
    /* @bin 0x00426A00 each record's value by its input against its limits (+0x20..+0x34); the
     * lowest below 100 into 0x005C9348, its index into 0x00705738 */
    int32_t low = 100;
    for (uint32_t k = 0; k < RATING_RECS; k++) {
        uint32_t r = G_RATING_TABLE + RATING_REC * k;
        int32_t v = rating_input(st, r), val;
        if (v <= S32(st, r + 0x20)) {
            val = S32(st, r + 4);
        } else if (v < S32(st, r + 0x24)) {
            val = S32(st, r + 8);
        } else if (v < S32(st, r + 0x28)) {
            val = S32(st, r + 0xC);
        } else if (v < S32(st, r + 0x2C)) {
            val = S32(st, r + 0x10);
        } else if (v < S32(st, r + 0x30)) {
            val = S32(st, r + 0x14);
        } else if (v < S32(st, r + 0x34)) {
            val = S32(st, r + 0x18);
        } else {
            val = S32(st, r + 0x1C);
        }
        if (val < low) {
            S32(st, G_RATING_LOWEST_INDEX) = (int32_t) k;
            low = val;
        }
    }
    S32(st, G_U5C9348) = low;
}

void rating_bonus(game_state *st)
{
    /* @bin 0x00426A80 5 for each record whose value by fixed steps of its input (0, 20, 40, 60, 80,
     * 100) plus 30 is below the lowest */
    int32_t low = S32(st, G_U5C9348), sum = 0;
    for (uint32_t k = 0; k < RATING_RECS; k++) {
        uint32_t r = G_RATING_TABLE + RATING_REC * k;
        int32_t v = rating_input(st, r), val;
        val = v <= 0      ? S32(st, r + 4)
              : v < 0x14  ? S32(st, r + 8)
              : v < 0x28  ? S32(st, r + 0xC)
              : v < 0x3C  ? S32(st, r + 0x10)
              : v < 0x50  ? S32(st, r + 0x14)
              : v < 0x64  ? S32(st, r + 0x18)
                          : S32(st, r + 0x1C);
        if (val + 0x1E < low) {
            sum += 5;
        }
    }
    S32(st, G_U705734) = sum;
}

void rating_0x4269b0(game_state *st)
{
    /* @bin 0x004269B0 (also 0x0046E1C0, a jump to it) city +0x42CC := lowest + bonus within 0..100,
     * +0x4434 := the lowest one's index */
    rating_lowest(st);
    rating_bonus(st);
    int32_t v = S32(st, G_U5C9348) + S32(st, G_U705734);
    if (v >= 100) {
        v = 100;
    } else if (v < 0) {
        v = 0;
    }
    uint32_t c = CITY(st);
    S32(st, c + CITY_RATING_CULTURE) = v;
    S32(st, c + CITY_RATING_CULTURE_REASON) = S32(st, G_RATING_LOWEST_INDEX);
}

void rating_0x46e7c0(game_state *st)
{
    /* @bin 0x0046E7C0 city +0x46A8: the average of 0x0070CE10 (by level +0x12, stride 0x58) over
     * the buildings' people (+0x1C), state and +5 set */
    int32_t sum = 0, n = 0;
    int32_t count = S32(st, G_BUILDING_HIGHEST);
    for (int32_t id = 1; id <= count; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b + BLD_STATE) == 0 || U8(st, b + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        int32_t pop = S16(st, b + BLD_POPULATION);
        sum += S32(st, G_HOUSE_LEVEL_U44 + 0x58u * (uint32_t) (int32_t) S16(st, b + BLD_SUBTYPE)) * pop;
        n += pop;
    }
    S32(st, CITY(st) + CITY_PROSPERITY_CAP) = n > 0 ? sum / n : 0;
}

void rating_0x46e930(game_state *st)
{
    /* @bin 0x0046E930 */
    uint32_t c = CITY(st);
    S32(st, c + CITY_U42FC) = 0;
    S32(st, c + CITY_U4300) = 0;
    S32(st, c + CITY_U4658) = 0;
    S32(st, c + CITY_U4548) = 0;
    S32(st, c + CITY_U454C) = 0;
    S32(st, c + CITY_U4550) = 0;
    S32(st, c + CITY_U4554) = 0;
}

void rating_0x46e880(game_state *st)
{
    /* @bin 0x0046E880 city +0x4444 by +0x4634, +0x4644, +0x442C, then +0x42DC */
    uint32_t c = CITY(st);
    int32_t v = S32(st, c + CITY_U42DC), r;
    if (S32(st, c + CITY_U4634) != 0) {
        r = 8;
    } else if (S32(st, c + CITY_U4644) != 0) {
        r = 7;
    } else if (S32(st, c + CITY_U442C) != 0) {
        r = 6;
    } else {
        r = v < 0xA ? 0 : v < 0x1E ? 1 : v < 0x3C ? 2 : v < 0x5A ? 3 : v < 0x64 ? 4 : 5;
    }
    S32(st, c + CITY_U4444) = r;
}

void rating_0x46ec70(game_state *st)
{
    /* @bin 0x0046EC70 city +0x4580 from +0x2C9C - +0x444C; +0x4440: the first reason that holds */
    uint32_t c = CITY(st);
    S32(st, c + CITY_U4580) = 0;
    int32_t t = S32(st, c + CITY_SALARY_RANK) - S32(st, c + CITY_PLAYER_RANK);
    if (S32(st, c + CITY_PLAYER_RANK) == 0) {
        if (t > 0) {
            S32(st, c + CITY_U4580) = t;
        }
    } else if (t > 0) {
        S32(st, c + CITY_U4580) = t + 1;
    }
    rating_0x46e880(st);
    c = CITY(st);
    int32_t i = S32(st, c + CITY_U4580), m = S32(st, c + CITY_U44EC), f = S32(st, c + CITY_U4588), r;
    if (i >= 8) {
        r = 1;
    } else if (m >= 3) {
        r = 2;
    } else if (f >= 5) {
        r = 3;
    } else if (i >= 5) {
        r = 4;
    } else if (m >= 2) {
        r = 5;
    } else if (f >= 3) {
        r = 6;
    } else if (i >= 3) {
        r = 7;
    } else if (S32(st, c + CITY_U44E8) != 0) {
        r = 8;
    } else if (i >= 2) {
        r = 9;
    } else if (S32(st, c + CITY_U4584) != 0) {
        r = 10;
    } else if (i != 0) {
        r = 11;
    } else if (S32(st, c + CITY_U4590) == 2) {
        r = 12;
    } else if (S32(st, c + CITY_U4590) == 1) {
        r = 13;
    } else {
        r = 0;
    }
    S32(st, c + CITY_RATING_KINGDOM_REASON) = r;
}

static int below(game_state *st, uint32_t field, uint32_t goal, int32_t pct)
{
    int32_t want = calc_percentage(S32(st, goal), pct);
    return S32(st, CITY(st) + field) < want;
}

void rating_0x46e970(game_state *st, int32_t month_end)
{
    /* @bin 0x0046E970 city +0x42D8 (0..100) */
    uint32_t c = CITY(st);
    if (S16(st, G_U784B60) != 0) {
        S32(st, c + CITY_RATING_KINGDOM) = 0x32;
        return;
    }
    S32(st, c + CITY_U455C) += 1;
    if (S32(st, c + CITY_U455C) >= 0xC) {
        S32(st, c + CITY_U4560) = 0;
    }
    if (month_end != 0) {
        S32(st, c + CITY_U4580) = 0;
        S32(st, c + CITY_U4584) = 0;
        S32(st, c + CITY_U4588) = 0;
        if (!(S8(st, G_MISSION) < 2 && S32(st, G_CUSTOM) == 0)) {
            S32(st, c + CITY_RATING_KINGDOM) -= 1;
        }
        if (S32(st, c + CITY_U44E8) != 0) {
            int32_t m = S32(st, c + CITY_U44EC);
            S32(st, c + CITY_RATING_KINGDOM) += m <= 1 ? -3 : m <= 2 ? -5 : -8;
        } else if (S32(st, c + CITY_INCOME_LAST_YEAR) > S32(st, c + CITY_EXPENSES_LAST_YEAR)) {
            S32(st, c + CITY_RATING_KINGDOM) += 1;
        }
        int32_t t = S32(st, c + CITY_SALARY_RANK) - S32(st, c + CITY_PLAYER_RANK);
        if (S32(st, c + CITY_PLAYER_RANK) == 0) {
            if (t > 0) {
                S32(st, c + CITY_RATING_KINGDOM) -= t;
                S32(st, c + CITY_U4580) = t;
            }
        } else if (t > 0) {
            S32(st, c + CITY_RATING_KINGDOM) -= t;
            S32(st, c + CITY_U4580) = t + 1;
        } else if (t < 0) {
            S32(st, c + CITY_RATING_KINGDOM) += 1;
        }
        /* @bin 0x0046EA8B in the years base (0x007848D8) + 0x00784C98 / C9C / CA0, the goals at 25,
         * 50, 75 percent: all reached +5, else -2 and +0x4584 := 2 */
        int32_t year = S32(st, G_YEAR), base = S16(st, G_START_YEAR), pct = 0;
        if (year == base + S32(st, G_U784C98)) {
            pct = 0x19;
        } else if (year == S32(st, G_U784C9C) + base) {
            pct = 0x32;
        } else if (year == base + S32(st, G_U784CA0)) {
            pct = 0x4B;
        }
        if (pct != 0) {
            int ok = 1;
            if (U8(st, G_GOAL_CULTURE_ON) != 0 && below(st, CITY_RATING_CULTURE, G_GOAL_CULTURE, pct)) {
                ok = 0;
            }
            if (U8(st, G_GOAL_PROSPERITY_ON) != 0 && below(st, CITY_RATING_PROSPERITY, G_GOAL_PROSPERITY, pct)) {
                ok = 0;
            }
            if (S32(st, G_GOAL_POPULATION_ON) != 0 && below(st, CITY_POPULATION, G_GOAL_POPULATION, pct)) {
                ok = 0;
            }
            if (U8(st, G_GOAL_MONUMENT_ON) != 0 && below(st, CITY_RATING_MONUMENT, G_GOAL_MONUMENT, pct)) {
                ok = 0;
            }
            if (U8(st, G_GOAL_KINGDOM_ON) != 0 && below(st, CITY_RATING_KINGDOM, G_GOAL_KINGDOM, pct)) {
                ok = 0;
            }
            c = CITY(st);
            if (ok) {
                S32(st, c + CITY_RATING_KINGDOM) += 5;
            } else {
                S32(st, c + CITY_RATING_KINGDOM) -= 2;
                S32(st, c + CITY_U4584) = 2;
            }
        }
        rating_0x46e930(st);
        c = CITY(st);
        int32_t prev = S32(st, c + CITY_U458C), cur = S32(st, c + CITY_RATING_KINGDOM);
        if (prev < cur) {
            S32(st, c + CITY_U4590) = 2;
        } else if (prev == cur) {
            S32(st, c + CITY_U4590) = 1;
        } else {
            S32(st, c + CITY_U4590) = 0;
        }
        S32(st, c + CITY_U458C) = S32(st, c + CITY_RATING_KINGDOM);
    }
    if (S32(st, c + CITY_RATING_KINGDOM) < 0) {
        S32(st, c + CITY_RATING_KINGDOM) = 0;
    }
    if (S32(st, c + CITY_RATING_KINGDOM) > 100) {
        S32(st, c + CITY_RATING_KINGDOM) = 100;
    }
    rating_0x46ec70(st);
}

int32_t city_0x451b10(game_state *st)
{
    /* @bin 0x00451B10 a building of type 0xB7 with +0xC8 below 1 whose tile (0x005F5D08 by +0x54,
     * plus +0x0C) passes 0x004ED360(x, y, 3, 0) with 0x00EAAE90 clear, or one of type 0xD2 while
     * city +0x433A >= 16 (the loop runs to 0x0089B7A0: ids 1..4003) */
    for (uint32_t p = BUILDING_ADDR(1) + BLD_UC8; p < BUILDING_ADDR(BUILDING_COUNT) + BLD_UC8; p += BUILDING_REC_SIZE) {
        uint32_t b = p - BLD_UC8;
        if (U8(st, b + BLD_STATE) != 1) {
            continue;
        }
        int16_t t = S16(st, b + BLD_TYPE);
        if (t == 0xB7) {
            if (S16(st, b + BLD_UC8) >= 1) {
                continue;
            }
            int32_t g = (int32_t) (U32(st, G_PYRAMID_RAMP + 4u * U8(st, b + BLD_MONUMENT_KIND)) + U32(st, b + BLD_GRID_OFFSET) - U32(st, G_MAP_START));
            GAME_CALL(st, FN_RAMP_WALK, g % 0xE4, g / 0xE4, 3, 0);
            if (S32(st, G_UEAAE90) == 0) {
                return 1;
            }
        } else if (t == 0xD2 && S16(st, CITY(st) + CITY_U433A) >= 0x10) {
            return 1;
        }
    }
    return 0;
}

static int32_t money_in(game_state *st, uint32_t c)
{
    return (int32_t) (U32(st, c + CITY_GOLD_LAST_YEAR) + U32(st, c + CITY_GIFTS_LAST_YEAR) + U32(st, c + CITY_TAXES_LAST_YEAR) + U32(st, c + CITY_EXPORTS_LAST_YEAR));
}

static int32_t money_out(game_state *st, uint32_t c)
{
    return (int32_t) (U32(st, c + CITY_SALARY_LAST_YEAR) + U32(st, c + CITY_INTEREST_LAST_YEAR) + U32(st, c + CITY_WAGES_LAST_YEAR) + U32(st, c + CITY_IMPORTS_LAST_YEAR) +
                      U32(st, c + CITY_CONSTRUCTION_LAST_YEAR));
}

void rating_0x46e410(game_state *st)
{
    /* @bin 0x0046E410 the same steps as 0x0046E1E0 (with +0x2C8C in the money test), keeping the
     * worst one: city +0x42E4 (0 rising, 1 falling or capped, 2 steady) and +0x4438 (the reason) */
    int32_t r = GAME_CALL0(st, FN_CITY_0X4C38F0);
    uint32_t c = CITY(st);
    int32_t worst = 0, reason = 4, delta;
    int32_t in = money_in(st, c), out = money_out(st, c), x = S32(st, c + CITY_CONSTRUCTION_THIS_YEAR);
    int32_t d = (int32_t) ((uint32_t) in - (uint32_t) out);
    if (d > 0) {
        delta = 5;
    } else {
        delta = 0;
        if ((int32_t) ((uint32_t) (x / 2) - (uint32_t) out + (uint32_t) in) > 0) {
            delta = 3;
        } else if (d < 0) {
            worst = 8;
            reason = 4;
            delta = -8;
        } else if ((int32_t) ((uint32_t) x - (uint32_t) out + (uint32_t) in) < 0) {
            worst = 5;
            reason = 4;
            delta = -5;
        }
    }
    if (S32(st, c + CITY_U2974) >= 0 && S32(st, c + CITY_U2974) <= 1) {
        delta--;
    }
    if (S32(st, c + CITY_EXPORTS_LAST_YEAR) > S32(st, c + CITY_IMPORTS_LAST_YEAR)) {
        delta++;
    } else if (S32(st, c + CITY_EXPORTS_LAST_YEAR) < S32(st, c + CITY_IMPORTS_LAST_YEAR)) {
        if (worst < 1) {
            worst = 1;
            reason = 0xB;
        }
        delta--;
    } else if (worst == 0) {
        worst = 1;
        reason = 0xB;
    }
    if (S32(st, c + CITY_LABOUR_SHORTAGE) > 0) {
        if (worst < 1) {
            worst = 1;
            reason = 0xA;
        }
        delta--;
    } else {
        int32_t y = S32(st, c + CITY_UNEMPLOYMENT);
        if (y < 5) {
            delta += 2;
        } else if (y < 0xA) {
            if (worst == 0) {
                worst = 1;
                reason = 5;
            }
        } else if (y < 0xF) {
            delta--;
            if (worst < 1) {
                worst = 1;
                reason = 5;
            }
        } else if (y < 0x14) {
            delta -= 2;
            if (worst < 2) {
                worst = 2;
                reason = 5;
            }
        } else {
            delta -= 3;
            if (worst < 3) {
                worst = 3;
                reason = 5;
            }
        }
    }
    int32_t t = S32(st, c + CITY_U45A8) / 12 - S32(st, c + CITY_WAGE_KINGDOM) - r;
    if (t > 5) {
        delta += 2;
    } else if (t > 0) {
        delta++;
    } else if (t < -10) {
        if (worst < 3) {
            worst = 3;
            reason = 6;
        }
        delta -= 3;
    } else if (t < -5) {
        delta -= 2;
        if (worst < 2) {
            worst = 2;
            reason = 6;
        }
    } else if (t < 0) {
        delta--;
        if (worst < 1) {
            worst = 1;
            reason = 6;
        }
    }
    int32_t p1 = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + CITY_U2708), S32(st, c + CITY_POPULATION));
    c = CITY(st);
    int32_t p2 = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + CITY_U270C), S32(st, c + CITY_POPULATION));
    if (p1 > 0x5F) {
        if (worst < 1) {
            worst = 1;
            reason = 7;
        }
        delta--;
    }
    if (p2 > 0xA) {
        delta += 2;
    }
    c = CITY(st);
    if (S32(st, c + CITY_U44E8) != 0) {
        delta--;
        if (worst < 1) {
            worst = 1;
            reason = 8;
        }
    }
    if (S32(st, c + CITY_U4378) > 0) {
        delta++;
    }
    if (city_0x451b10(st)) {
        delta++;
    }
    c = CITY(st);
    int32_t cur = S32(st, c + CITY_RATING_PROSPERITY), now = (int32_t) ((uint32_t) cur + (uint32_t) delta);
    if (delta == 0 || (cur <= 0 && S32(st, G_YEAR) == S16(st, G_START_YEAR))) {
        S32(st, c + CITY_RATING_PROSPERITY_REASON) = 0;
        S32(st, c + CITY_U42E4) = 2;
        return;
    }
    if (delta <= 0) {
        S32(st, c + CITY_U42E4) = 1;
        S32(st, c + CITY_RATING_PROSPERITY_REASON) = reason;
        return;
    }
    S32(st, c + CITY_U42E4) = 0;
    int32_t cap = S32(st, c + CITY_PROSPERITY_CAP);
    if (now > cap) {
        if (cur > cap) {
            S32(st, c + CITY_U42E4) = 1;
        } else {
            S32(st, c + CITY_U42E4) = cur >= cap ? 2 : 0;
        }
        S32(st, c + CITY_RATING_PROSPERITY_REASON) = 3;
    } else {
        S32(st, c + CITY_RATING_PROSPERITY_REASON) = worst > 0 ? reason : 0;
    }
}

void rating_0x46e1e0(game_state *st)
{
    /* @bin 0x0046E1E0 city +0x42D0 (0..100, at most +0x46A8) moves by the quarter's factors, then
     * 0x0046E410 (a tail jump) */
    int32_t r = GAME_CALL0(st, FN_CITY_0X4C38F0);
    uint32_t c = CITY(st);
    int32_t in = money_in(st, c), out = money_out(st, c), x = S32(st, c + CITY_CONSTRUCTION_LAST_YEAR), delta;
    int32_t d = (int32_t) ((uint32_t) in - (uint32_t) out);
    if (d > 0) {
        delta = 5;
    } else if ((int32_t) ((uint32_t) (x / 2) - (uint32_t) out + (uint32_t) in) > 0) {
        delta = 3;
    } else if (d < 0) {
        delta = -8;
    } else if ((int32_t) ((uint32_t) x - (uint32_t) out + (uint32_t) in) < 0) {
        delta = -5;
    } else {
        delta = 0;
    }
    if (S32(st, c + CITY_EXPORTS_LAST_YEAR) > S32(st, c + CITY_IMPORTS_LAST_YEAR)) {
        delta++;
    } else if (S32(st, c + CITY_EXPORTS_LAST_YEAR) < S32(st, c + CITY_IMPORTS_LAST_YEAR)) {
        delta--;
    }
    if (S32(st, c + CITY_LABOUR_SHORTAGE) > 0) {
        delta--;
    } else {
        int32_t y = S32(st, c + CITY_UNEMPLOYMENT);
        if (y < 5) {
            delta += 2;
        } else if (y >= 0xA) {
            delta -= y < 0xF ? 1 : y < 0x14 ? 2 : 3;
        }
    }
    if (S32(st, c + CITY_U2974) >= 0 && S32(st, c + CITY_U2974) <= 1) {
        delta--;
    }
    int32_t t = S32(st, c + CITY_U45A8) / 12 - S32(st, c + CITY_WAGE_KINGDOM) - r;
    if (t > 5) {
        delta += 2;
    } else if (t > 0) {
        delta++;
    } else if (t < -10) {
        delta -= 3;
    } else if (t < -5) {
        delta -= 2;
    } else if (t < 0) {
        delta--;
    }
    int32_t p1 = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + CITY_U2708), S32(st, c + CITY_POPULATION));
    c = CITY(st);
    int32_t p2 = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + CITY_U270C), S32(st, c + CITY_POPULATION));
    if (p1 > 0x5F) {
        delta--;
    }
    if (p2 > 0xA) {
        delta += 2;
    }
    c = CITY(st);
    if (S32(st, c + CITY_U44E8) != 0) {
        delta--;
    }
    if (S32(st, c + CITY_U4378) > 0) {
        delta++;
    }
    if (city_0x451b10(st)) {
        delta++;
    }
    c = CITY(st);
    int32_t v = (int32_t) ((uint32_t) S32(st, c + CITY_RATING_PROSPERITY) + (uint32_t) delta);
    S32(st, c + CITY_RATING_PROSPERITY) = v;
    if (v > S32(st, c + CITY_PROSPERITY_CAP)) {
        S32(st, c + CITY_RATING_PROSPERITY) = S32(st, c + CITY_PROSPERITY_CAP);
    }
    if (S32(st, c + CITY_RATING_PROSPERITY) < 0) {
        S32(st, c + CITY_RATING_PROSPERITY) = 0;
    }
    if (S32(st, c + CITY_RATING_PROSPERITY) > 100) {
        S32(st, c + CITY_RATING_PROSPERITY) = 100;
    }
    rating_0x46e410(st);
}

int32_t flood_0x4bd590(game_state *st, uint32_t self)
{
    /* @bin 0x004BD590 (thiscall) this +0x14, 10 more when city +0x4478 is set and building +0x472C
     * is of level 0; at most 100 */
    int32_t v = S32(st, self + 0x14);
    uint32_t c = CITY(st);
    if (S16(st, c + CITY_U4478) != 0 && building_type_level(st, S32(st, c + CITY_U472C)) == 0) {
        v += 0xA;
    }
    return v > 100 ? 100 : v;
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_426a00, rating_lowest)
VOID0(run_426a80, rating_bonus)
VOID0(run_4269b0, rating_0x4269b0)
VOID0(run_46e7c0, rating_0x46e7c0)
VOID0(run_46e930, rating_0x46e930)
VOID0(run_46e880, rating_0x46e880)
VOID0(run_46ec70, rating_0x46ec70)
VOID0(run_46e410, rating_0x46e410)
VOID0(run_46e1e0, rating_0x46e1e0)

static int32_t run_451b10(game_state *st, const int32_t *a)
{
    (void) a;
    return city_0x451b10(st);
}

static int32_t run_46e970(game_state *st, const int32_t *a)
{
    rating_0x46e970(st, a[0]);
    return 0;
}

static int32_t run_4bd590(game_state *st, const int32_t *a)
{
    (void) a;
    return flood_0x4bd590(st, G_UD05720 + 0x32C40u);
}

const ported_function month_ratings_ports[] = {
    {0x00426A00u, "rating_lowest", 0, 0, 0, run_426a00, 0},
    {0x00426A80u, "rating_bonus", 0, 0, 0, run_426a80, 0},
    {0x004269B0u, "rating_0x4269b0", 0, 0, 0, run_4269b0, 0},
    {0x0046E1C0u, "rating_0x4269b0 (jump)", 0, 0, 0, run_4269b0, 0},
    {0x0046E7C0u, "rating_0x46e7c0", 0, 0, 0, run_46e7c0, 0},
    {0x0046E930u, "rating_0x46e930", 0, 0, 0, run_46e930, 0},
    {0x0046E880u, "rating_0x46e880", 0, 0, 0, run_46e880, 0},
    {0x0046EC70u, "rating_0x46ec70", 0, 0, 0, run_46ec70, 0},
    {0x0046E970u, "rating_0x46e970", 1, 0, 0, run_46e970, 0},
    {0x00451B10u, "city_0x451b10", 0, 8, 0, run_451b10, 0},
    {0x0046E410u, "rating_0x46e410", 0, 0, 0, run_46e410, 0},
    {0x0046E1E0u, "rating_0x46e1e0", 0, 0, 0, run_46e1e0, 0},
    /* thiscall on the flood record 0x00D05720 + 0x32C40 (as 0x004BE640 calls it) */
    {0x004BD590u, "flood_0x4bd590", 0, 32, G_FLOODPLAIN, run_4bd590, 0},
};
const int month_ratings_ports_count = sizeof(month_ratings_ports) / sizeof(month_ratings_ports[0]);
