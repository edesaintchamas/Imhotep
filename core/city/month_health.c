/* Imhotep - the city's health, twice a month (0x004E8840, days 0 and 8).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Fields: state/layout/city.h, building.h (the meaning of the CITY_U / BLD_U ones is not
 * established). */
#include "city/month_health.h"

#include "city/month.h"
#include "city/month_mood.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define CITY(st) CURRENT_CITY(st)

int32_t city_0x4c38f0(game_state *st)
{
    /* @bin 0x004C38F0 */
    uint32_t c = CITY(st);
    if (S16(st, c + CITY_U4478) != 0 && building_type_level(st, S32(st, c + CITY_U472C)) == 1 &&
        building_0xa9_is(st, S32(st, CITY(st) + CITY_U472C), 2)) {
        return -2;
    }
    return 0;
}

int32_t city_0x4e8ec0(game_state *st)
{
    /* @bin 0x004E8EC0 by +0x270C, then +0x4640 (positive or not) */
    uint32_t c = CITY(st);
    int32_t r = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, c + CITY_U463C), S32(st, c + CITY_POPULATION));
    c = CITY(st);
    if (S32(st, c + CITY_U270C) > 0) {
        return r > 0x38 ? 0 : r >= 0x28 ? -3 : r >= 0x1A ? -4 : (r >= 0xA) - 6;
    }
    if (S32(st, c + CITY_U4640) > 0) {
        return r > 0x38 ? 0 : r >= 0x28 ? -2 : r >= 0x1A ? -3 : (r >= 0xA) - 5;
    }
    return r >= 0x28 ? 0 : r >= 0x1A ? -1 : (r >= 0xA) - 3;
}

void city_health_update(game_state *st)
{
    /* @bin 0x004E8840 */
    int32_t r = city_0x4c38f0(st);
    int32_t player = S32(st, CURRENT_PLAYER_ADDR);
    int32_t counted = city_sum_building_1c(st, player);
    if (counted < S32(st, CITY_ADDR(player) + CITY_POPULATION)) {
        people_remove(st, player, S32(st, CITY_ADDR(player) + CITY_POPULATION) - counted);
    }
    uint32_t c = CITY(st);
    /* @bin 0x004E888E the four city factors: e (table 0x00EA1384 by +0x2C38 and +8), a (by
     * +0x2C08 - +0x2C0C - r), b (by +0x2BFC), d (0x004E8EC0, every other call) */
    int32_t a = 0, b = 0, cc = 0;
    int32_t k = 0;
    if (S32(st, c + CITY_U2C38) != 0) {
        k = (S32(st, c + CITY_U2C38) + 10) / 10;
        if (k > 10) {
            k = 10;
        }
    }
    int32_t e = S32(st, G_SENTIMENT_TABLE + 4u * (uint32_t) (k + S32(st, c + CITY_TAX_RATE) * 11));
    int32_t t = (int32_t) ((uint32_t) S32(st, c + CITY_WAGE_RATE) - (uint32_t) S32(st, c + CITY_WAGE_KINGDOM) - (uint32_t) r);
    if (t < 0) {
        a = t / 2;
        if (a == 0) {
            a = -1;
        }
    } else {
        a = t > 7 ? 4 : t > 4 ? 3 : t > 1 ? 2 : t > 0 ? 1 : 0;
    }
    S32(st, c + CITY_U4638) = S32(st, c + CITY_WAGE_RATE);
    int32_t y = S32(st, c + CITY_UNEMPLOYMENT);
    b = y > 0x19 ? -3 : y > 0x11 ? -2 : y > 0xA ? -1 : y < 5;
    S32(st, c + CITY_U4534) = y;
    int32_t d;
    if (U8(st, c + CITY_U468B) != 0) {
        d = city_0x4e8ec0(st);
        c = CITY(st);
        U8(st, c + CITY_U468B) = 0;
    } else {
        d = 0;
        U8(st, c + CITY_U468B) = 1;
    }
    /* @bin 0x004E89B8 the houses (the loop runs to 0x0089B7BC: ids 1..4003). `last` is the
     * register the original keeps the per-house step in: the average replaces it only when some
     * house counted */
    int32_t diff = S32(st, G_DIFFICULTY);
    int32_t n1 = 0, n2 = 0, s1 = 0, s2 = 0, last = 0;
    for (uint32_t p = BUILDING_ADDR(1) + BLD_CRIME_RISK; p < BUILDING_ADDR(BUILDING_COUNT) + BLD_CRIME_RISK; p += BUILDING_REC_SIZE) {
        uint32_t h = p - BLD_CRIME_RISK; /* the building */
        if (U8(st, h + BLD_STATE) != 1 || U8(st, h + BLD_HOUSE_SIZE) == 0) {
            continue;
        }
        uint8_t base = U8(st, G_SENTIMENT_BASE + 4u * (uint32_t) diff);
        if (S16(st, h + BLD_POPULATION) == 0) {
            U8(st, h + BLD_SENTIMENT) = (uint8_t) (base + 0xA);
            continue;
        }
        if (S32(st, c + CITY_POPULATION) < 0x12C) {
            last = 0;
            b = 0;
            e = 0;
            a = 0;
            cc = 0;
            U8(st, h + BLD_SENTIMENT) = S32(st, c + CITY_POPULATION) < 0xC8 ? (uint8_t) (base + 0xA) : base;
            S32(st, p + BLD_STATE) = 0;
            continue;
        }
        int32_t level = S16(st, h + BLD_SUBTYPE);
        S32(st, p + BLD_STATE) += S32(st, G_HOUSE_LEVEL_U3C + G_HOUSE_LEVEL_STRIDE * (uint32_t) level);
        U8(st, h + BLD_SENTIMENT) = (uint8_t) (U8(st, h + BLD_SENTIMENT) + (uint8_t) ((uint8_t) b + (uint8_t) a + (uint8_t) e));
        n2++;
        uint8_t add_d = 0;
        cc = 0;
        if (S32(st, G_HOUSE_LEVEL_U28 + G_HOUSE_LEVEL_STRIDE * (uint32_t) level) == 0) {
            last = 0;
            s2 += d;
            U8(st, h + BLD_UBC) = 0;
            cc = d;
            add_d = (uint8_t) d;
        } else {
            n1++;
            uint8_t g = U8(st, h + BLD_U7F);
            if (g >= 2) {
                last = 2;
                U8(st, h + BLD_UBC) = 0;
            } else if (g > 0) {
                last = 1;
                U8(st, h + BLD_UBC) = 0;
            } else {
                uint8_t m = (uint8_t) (U8(st, h + BLD_UBC) + 1);
                U8(st, h + BLD_UBC) = m;
                if (m > 3) {
                    U8(st, h + BLD_UBC) = 3;
                }
                m = U8(st, h + BLD_UBC);
                last = m >= 3 ? -3 : m >= 2 ? -2 : -1;
            }
            s1 += last;
        }
        int8_t v = (int8_t) (U8(st, h + BLD_SENTIMENT) + (uint8_t) (add_d + (uint8_t) last));
        S8(st, h + BLD_SENTIMENT) = v;
        if (v < 0) {
            S8(st, h + BLD_SENTIMENT) = 0;
        }
        if (S8(st, h + BLD_SENTIMENT) > 100) {
            S8(st, h + BLD_SENTIMENT) = 100;
        }
        diff = S32(st, G_DIFFICULTY);
    }
    if (n1 != 0) {
        last = s1 / n1;
    }
    if (n2 != 0) {
        cc = s2 / n2;
    }
    /* @bin 0x004E8B88 +0x10: the average +0xC2 (i8) of the houses with people (ids 1..4003) */
    int32_t sum = 0, n = 0;
    for (uint32_t h = BUILDING_ADDR(1); h + BLD_HOUSE_SIZE < BUILDING_ADDR(BUILDING_COUNT) + BLD_HOUSE_SIZE; h += BUILDING_REC_SIZE) {
        if (U8(st, h + BLD_STATE) == 1 && U8(st, h + BLD_HOUSE_SIZE) != 0 && S16(st, h + BLD_POPULATION) != 0) {
            n++;
            sum += S8(st, h + BLD_SENTIMENT);
        }
    }
    if (n == 0) {
        S32(st, c + CITY_SENTIMENT) = 0x3C;
    } else {
        S32(st, c + CITY_SENTIMENT) = sum / n;
        if (S16(st, c + CITY_U4478) != 0 && building_type_level(st, S32(st, c + CITY_U472C)) == 4 &&
            building_0xa9_is(st, S32(st, CITY(st) + CITY_U472C), 2)) {
            uint32_t c2 = CITY(st);
            int32_t v = S32(st, c2 + CITY_SENTIMENT) + 10;
            S32(st, c2 + CITY_SENTIMENT) = v >= 100 ? 100 : v;
        }
    }
    /* @bin 0x004E8C42 +0x4540: which factor is the lowest below 0 (1..5) */
    c = CITY(st);
    int32_t low = 0;
    if (last < 0) {
        S32(st, c + CITY_SENTIMENT_LOW) = 1;
        low = last;
    }
    if (b < low) {
        S32(st, c + CITY_SENTIMENT_LOW) = 2;
        low = b;
    }
    if (e < low) {
        S32(st, c + CITY_SENTIMENT_LOW) = 3;
        low = e;
    }
    if (a < low) {
        S32(st, c + CITY_SENTIMENT_LOW) = 4;
        low = a;
    }
    if (cc < low) {
        S32(st, c + CITY_SENTIMENT_LOW) = 5;
    }
    /* @bin 0x004E8CC8 a message when +0x10 changes its ten (population above 300) */
    if (!(S8(st, G_MISSION) < 2 && S32(st, G_CUSTOM) == 0)) {
        int32_t hval = S32(st, c + CITY_SENTIMENT);
        if (S32(st, c + CITY_SENTIMENT_LAST) / 10 != hval / 10 && S32(st, c + CITY_POPULATION) > 0x12C) {
            int32_t msg, extra = 0;
            if (hval <= 0) {
                msg = 0x69;
                if (S32(st, c + CITY_SENTIMENT_LOW) != 0) {
                    extra = S32(st, c + CITY_SENTIMENT_LOW) + 0x74;
                }
            } else if (hval >= 100) {
                msg = 0x74;
            } else {
                msg = hval / 10 + 0x6A;
                if (hval < 0x32 && S32(st, c + CITY_SENTIMENT_LOW) != 0) {
                    extra = S32(st, c + CITY_SENTIMENT_LOW) + 0x74;
                }
            }
            GAME_CALL(st, FN_NOTICE_ADD_4E2970, msg, 0, 0);
            if (extra != 0) {
                GAME_CALL(st, FN_NOTICE_ADD_4E2970, extra, 0, 0);
            }
        }
    }
    c = CITY(st);
    S32(st, c + CITY_SENTIMENT_LAST) = S32(st, c + CITY_SENTIMENT);
}

static int32_t run_4c38f0(game_state *st, const int32_t *a)
{
    (void) a;
    return city_0x4c38f0(st);
}

static int32_t run_4e8ec0(game_state *st, const int32_t *a)
{
    (void) a;
    return city_0x4e8ec0(st);
}

static int32_t run_health(game_state *st, const int32_t *a)
{
    (void) a;
    city_health_update(st);
    return 0;
}

const ported_function month_health_ports[] = {
    {0x004C38F0u, "city_0x4c38f0", 0, 32, 0, run_4c38f0, 0},
    {0x004E8EC0u, "city_0x4e8ec0", 0, 32, 0, run_4e8ec0, 0},
    {0x004E8840u, "city_health_update", 0, 0, 0, run_health, 0},
};
const int month_health_ports_count = sizeof(month_health_ports) / sizeof(month_health_ports[0]);
