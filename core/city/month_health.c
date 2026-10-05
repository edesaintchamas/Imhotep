/* Imhotep - the city's health, twice a month (0x004E8840, days 0 and 8).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Fields by their offset (city record CITY_BASE, building record BUILDING_BASE); their meaning
 * is not established. */
#include "city/month_health.h"

#include "city/month.h"
#include "city/month_mood.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define CITY(st) CURRENT_CITY(st)

int32_t city_0x4c38f0(game_state *st)
{
    /* @bin 0x004C38F0 */
    uint32_t c = CITY(st);
    if (S16(st, c + 0x4478) != 0 && building_type_level(st, S32(st, c + 0x472C)) == 1 &&
        building_0xa9_is(st, S32(st, CITY(st) + 0x472C), 2)) {
        return -2;
    }
    return 0;
}

int32_t city_0x4e8ec0(game_state *st)
{
    /* @bin 0x004E8EC0 by +0x270C, then +0x4640 (positive or not) */
    uint32_t c = CITY(st);
    int32_t r = GAME_CALL(st, 0x004CDE60u, S32(st, c + 0x463C), S32(st, c + 0x24));
    c = CITY(st);
    if (S32(st, c + 0x270C) > 0) {
        return r > 0x38 ? 0 : r >= 0x28 ? -3 : r >= 0x1A ? -4 : (r >= 0xA) - 6;
    }
    if (S32(st, c + 0x4640) > 0) {
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
    if (counted < S32(st, CITY_ADDR(player) + 0x24)) {
        people_remove(st, player, S32(st, CITY_ADDR(player) + 0x24) - counted);
    }
    uint32_t c = CITY(st);
    /* @bin 0x004E888E the four city factors: e (table 0x00EA1384 by +0x2C38 and +8), a (by
     * +0x2C08 - +0x2C0C - r), b (by +0x2BFC), d (0x004E8EC0, every other call) */
    int32_t a = 0, b = 0, cc = 0;
    int32_t k = 0;
    if (S32(st, c + 0x2C38) != 0) {
        k = (S32(st, c + 0x2C38) + 10) / 10;
        if (k > 10) {
            k = 10;
        }
    }
    int32_t e = S32(st, 0x00EA1384u + 4u * (uint32_t) (k + S32(st, c + 8) * 11));
    int32_t t = (int32_t) ((uint32_t) S32(st, c + 0x2C08) - (uint32_t) S32(st, c + 0x2C0C) - (uint32_t) r);
    if (t < 0) {
        a = t / 2;
        if (a == 0) {
            a = -1;
        }
    } else {
        a = t > 7 ? 4 : t > 4 ? 3 : t > 1 ? 2 : t > 0 ? 1 : 0;
    }
    S32(st, c + 0x4638) = S32(st, c + 0x2C08);
    int32_t y = S32(st, c + 0x2BFC);
    b = y > 0x19 ? -3 : y > 0x11 ? -2 : y > 0xA ? -1 : y < 5;
    S32(st, c + 0x4534) = y;
    int32_t d;
    if (U8(st, c + 0x468B) != 0) {
        d = city_0x4e8ec0(st);
        c = CITY(st);
        U8(st, c + 0x468B) = 0;
    } else {
        d = 0;
        U8(st, c + 0x468B) = 1;
    }
    /* @bin 0x004E89B8 the houses (the loop runs to 0x0089B7BC: ids 1..4003). `last` is the
     * register the original keeps the per-house step in: the average replaces it only when some
     * house counted */
    int32_t diff = S32(st, 0x00E39080u);
    int32_t n1 = 0, n2 = 0, s1 = 0, s2 = 0, last = 0;
    for (uint32_t p = 0x00799BC4u; p < 0x0089B7BCu; p += BUILDING_REC_SIZE) {
        uint32_t h = p - 0xE4; /* the building */
        if (U8(st, h) != 1 || U8(st, h + 5) == 0) {
            continue;
        }
        uint8_t base = U8(st, 0x005D199Cu + 4u * (uint32_t) diff);
        if (S16(st, h + 0x1C) == 0) {
            U8(st, h + 0xC2) = (uint8_t) (base + 0xA);
            continue;
        }
        if (S32(st, c + 0x24) < 0x12C) {
            last = 0;
            b = 0;
            e = 0;
            a = 0;
            cc = 0;
            U8(st, h + 0xC2) = S32(st, c + 0x24) < 0xC8 ? (uint8_t) (base + 0xA) : base;
            S32(st, p) = 0;
            continue;
        }
        int32_t level = S16(st, h + 0x12);
        S32(st, p) += S32(st, 0x0070CE08u + 0x58u * (uint32_t) level);
        U8(st, h + 0xC2) = (uint8_t) (U8(st, h + 0xC2) + (uint8_t) ((uint8_t) b + (uint8_t) a + (uint8_t) e));
        n2++;
        uint8_t add_d = 0;
        cc = 0;
        if (S32(st, 0x0070CDF4u + 0x58u * (uint32_t) level) == 0) {
            last = 0;
            s2 += d;
            U8(st, h + 0xBC) = 0;
            cc = d;
            add_d = (uint8_t) d;
        } else {
            n1++;
            uint8_t g = U8(st, h + 0x7F);
            if (g >= 2) {
                last = 2;
                U8(st, h + 0xBC) = 0;
            } else if (g > 0) {
                last = 1;
                U8(st, h + 0xBC) = 0;
            } else {
                uint8_t m = (uint8_t) (U8(st, h + 0xBC) + 1);
                U8(st, h + 0xBC) = m;
                if (m > 3) {
                    U8(st, h + 0xBC) = 3;
                }
                m = U8(st, h + 0xBC);
                last = m >= 3 ? -3 : m >= 2 ? -2 : -1;
            }
            s1 += last;
        }
        int8_t v = (int8_t) (U8(st, h + 0xC2) + (uint8_t) (add_d + (uint8_t) last));
        S8(st, h + 0xC2) = v;
        if (v < 0) {
            S8(st, h + 0xC2) = 0;
        }
        if (S8(st, h + 0xC2) > 100) {
            S8(st, h + 0xC2) = 100;
        }
        diff = S32(st, 0x00E39080u);
    }
    if (n1 != 0) {
        last = s1 / n1;
    }
    if (n2 != 0) {
        cc = s2 / n2;
    }
    /* @bin 0x004E8B88 +0x10: the average +0xC2 (i8) of the houses with people (ids 1..4003) */
    int32_t sum = 0, n = 0;
    for (uint32_t h = BUILDING_ADDR(1); h + 5 < 0x0089B6DDu; h += BUILDING_REC_SIZE) {
        if (U8(st, h) == 1 && U8(st, h + 5) != 0 && S16(st, h + 0x1C) != 0) {
            n++;
            sum += S8(st, h + 0xC2);
        }
    }
    if (n == 0) {
        S32(st, c + 0x10) = 0x3C;
    } else {
        S32(st, c + 0x10) = sum / n;
        if (S16(st, c + 0x4478) != 0 && building_type_level(st, S32(st, c + 0x472C)) == 4 &&
            building_0xa9_is(st, S32(st, CITY(st) + 0x472C), 2)) {
            uint32_t c2 = CITY(st);
            int32_t v = S32(st, c2 + 0x10) + 10;
            S32(st, c2 + 0x10) = v >= 100 ? 100 : v;
        }
    }
    /* @bin 0x004E8C42 +0x4540: which factor is the lowest below 0 (1..5) */
    c = CITY(st);
    int32_t low = 0;
    if (last < 0) {
        S32(st, c + 0x4540) = 1;
        low = last;
    }
    if (b < low) {
        S32(st, c + 0x4540) = 2;
        low = b;
    }
    if (e < low) {
        S32(st, c + 0x4540) = 3;
        low = e;
    }
    if (a < low) {
        S32(st, c + 0x4540) = 4;
        low = a;
    }
    if (cc < low) {
        S32(st, c + 0x4540) = 5;
    }
    /* @bin 0x004E8CC8 a message when +0x10 changes its ten (population above 300) */
    if (!(S8(st, 0x00E38EA8u) < 2 && S32(st, 0x00E38EBCu) == 0)) {
        int32_t hval = S32(st, c + 0x10);
        if (S32(st, c + 0x4538) / 10 != hval / 10 && S32(st, c + 0x24) > 0x12C) {
            int32_t msg, extra = 0;
            if (hval <= 0) {
                msg = 0x69;
                if (S32(st, c + 0x4540) != 0) {
                    extra = S32(st, c + 0x4540) + 0x74;
                }
            } else if (hval >= 100) {
                msg = 0x74;
            } else {
                msg = hval / 10 + 0x6A;
                if (hval < 0x32 && S32(st, c + 0x4540) != 0) {
                    extra = S32(st, c + 0x4540) + 0x74;
                }
            }
            GAME_CALL(st, 0x004E2970u, msg, 0, 0);
            if (extra != 0) {
                GAME_CALL(st, 0x004E2970u, extra, 0, 0);
            }
        }
    }
    c = CITY(st);
    S32(st, c + 0x4538) = S32(st, c + 0x10);
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
