/* Imhotep - the ratings of the month (0x0046E180 and its callees).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * City fields by their offset in the city record; their meaning is not established. */
#include "city/month_ratings.h"

#include "city/month.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/ported.h"
#include "state/records.h"

#define CITY(st) CURRENT_CITY(st)
#define BUILDING_HIGHEST 0x0078D790u
#define RATING_TABLE 0x005C9350u /* 11 records of 14 i32: index into 0x0078D5F0, 7 values, 6 limits */
#define RATING_REC 0x38u
#define RATING_RECS 11

static int32_t rating_input(game_state *st, uint32_t r)
{
    return S32(st, 0x0078D5F0u + 4u * U32(st, r));
}

void rating_lowest(game_state *st)
{
    /* @bin 0x00426A00 each record's value by its input against its limits (+0x20..+0x34); the
     * lowest below 100 into 0x005C9348, its index into 0x00705738 */
    int32_t low = 100;
    for (uint32_t k = 0; k < RATING_RECS; k++) {
        uint32_t r = RATING_TABLE + RATING_REC * k;
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
            S32(st, 0x00705738u) = (int32_t) k;
            low = val;
        }
    }
    S32(st, 0x005C9348u) = low;
}

void rating_bonus(game_state *st)
{
    /* @bin 0x00426A80 5 for each record whose value by fixed steps of its input (0, 20, 40, 60, 80,
     * 100) plus 30 is below the lowest */
    int32_t low = S32(st, 0x005C9348u), sum = 0;
    for (uint32_t k = 0; k < RATING_RECS; k++) {
        uint32_t r = RATING_TABLE + RATING_REC * k;
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
    S32(st, 0x00705734u) = sum;
}

void rating_0x4269b0(game_state *st)
{
    /* @bin 0x004269B0 (also 0x0046E1C0, a jump to it) city +0x42CC := lowest + bonus within 0..100,
     * +0x4434 := the lowest one's index */
    rating_lowest(st);
    rating_bonus(st);
    int32_t v = S32(st, 0x005C9348u) + S32(st, 0x00705734u);
    if (v >= 100) {
        v = 100;
    } else if (v < 0) {
        v = 0;
    }
    uint32_t c = CITY(st);
    S32(st, c + 0x42CC) = v;
    S32(st, c + 0x4434) = S32(st, 0x00705738u);
}

void rating_0x46e7c0(game_state *st)
{
    /* @bin 0x0046E7C0 city +0x46A8: the average of 0x0070CE10 (by level +0x12, stride 0x58) over
     * the buildings' people (+0x1C), state and +5 set */
    int32_t sum = 0, n = 0;
    int32_t count = S32(st, BUILDING_HIGHEST);
    for (int32_t id = 1; id <= count; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) == 0 || U8(st, b + 5) == 0) {
            continue;
        }
        int32_t pop = S16(st, b + 0x1C);
        sum += S32(st, 0x0070CE10u + 0x58u * (uint32_t) (int32_t) S16(st, b + 0x12)) * pop;
        n += pop;
    }
    S32(st, CITY(st) + 0x46A8) = n > 0 ? sum / n : 0;
}

void rating_0x46e930(game_state *st)
{
    /* @bin 0x0046E930 */
    uint32_t c = CITY(st);
    S32(st, c + 0x42FC) = 0;
    S32(st, c + 0x4300) = 0;
    S32(st, c + 0x4658) = 0;
    S32(st, c + 0x4548) = 0;
    S32(st, c + 0x454C) = 0;
    S32(st, c + 0x4550) = 0;
    S32(st, c + 0x4554) = 0;
}

void rating_0x46e880(game_state *st)
{
    /* @bin 0x0046E880 city +0x4444 by +0x4634, +0x4644, +0x442C, then +0x42DC */
    uint32_t c = CITY(st);
    int32_t v = S32(st, c + 0x42DC), r;
    if (S32(st, c + 0x4634) != 0) {
        r = 8;
    } else if (S32(st, c + 0x4644) != 0) {
        r = 7;
    } else if (S32(st, c + 0x442C) != 0) {
        r = 6;
    } else {
        r = v < 0xA ? 0 : v < 0x1E ? 1 : v < 0x3C ? 2 : v < 0x5A ? 3 : v < 0x64 ? 4 : 5;
    }
    S32(st, c + 0x4444) = r;
}

void rating_0x46ec70(game_state *st)
{
    /* @bin 0x0046EC70 city +0x4580 from +0x2C9C - +0x444C; +0x4440: the first reason that holds */
    uint32_t c = CITY(st);
    S32(st, c + 0x4580) = 0;
    int32_t t = S32(st, c + 0x2C9C) - S32(st, c + 0x444C);
    if (S32(st, c + 0x444C) == 0) {
        if (t > 0) {
            S32(st, c + 0x4580) = t;
        }
    } else if (t > 0) {
        S32(st, c + 0x4580) = t + 1;
    }
    rating_0x46e880(st);
    c = CITY(st);
    int32_t i = S32(st, c + 0x4580), m = S32(st, c + 0x44EC), f = S32(st, c + 0x4588), r;
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
    } else if (S32(st, c + 0x44E8) != 0) {
        r = 8;
    } else if (i >= 2) {
        r = 9;
    } else if (S32(st, c + 0x4584) != 0) {
        r = 10;
    } else if (i != 0) {
        r = 11;
    } else if (S32(st, c + 0x4590) == 2) {
        r = 12;
    } else if (S32(st, c + 0x4590) == 1) {
        r = 13;
    } else {
        r = 0;
    }
    S32(st, c + 0x4440) = r;
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
    if (S16(st, 0x00784B60u) != 0) {
        S32(st, c + 0x42D8) = 0x32;
        return;
    }
    S32(st, c + 0x455C) += 1;
    if (S32(st, c + 0x455C) >= 0xC) {
        S32(st, c + 0x4560) = 0;
    }
    if (month_end != 0) {
        S32(st, c + 0x4580) = 0;
        S32(st, c + 0x4584) = 0;
        S32(st, c + 0x4588) = 0;
        if (!(S8(st, 0x00E38EA8u) < 2 && S32(st, 0x00E38EBCu) == 0)) {
            S32(st, c + 0x42D8) -= 1;
        }
        if (S32(st, c + 0x44E8) != 0) {
            int32_t m = S32(st, c + 0x44EC);
            S32(st, c + 0x42D8) += m <= 1 ? -3 : m <= 2 ? -5 : -8;
        } else if (S32(st, c + 0x2CA4) > S32(st, c + 0x2CAC)) {
            S32(st, c + 0x42D8) += 1;
        }
        int32_t t = S32(st, c + 0x2C9C) - S32(st, c + 0x444C);
        if (S32(st, c + 0x444C) == 0) {
            if (t > 0) {
                S32(st, c + 0x42D8) -= t;
                S32(st, c + 0x4580) = t;
            }
        } else if (t > 0) {
            S32(st, c + 0x42D8) -= t;
            S32(st, c + 0x4580) = t + 1;
        } else if (t < 0) {
            S32(st, c + 0x42D8) += 1;
        }
        /* @bin 0x0046EA8B in the years base (0x007848D8) + 0x00784C98 / C9C / CA0, the goals at 25,
         * 50, 75 percent: all reached +5, else -2 and +0x4584 := 2 */
        int32_t year = S32(st, 0x0078D7BCu), base = S16(st, 0x007848D8u), pct = 0;
        if (year == base + S32(st, 0x00784C98u)) {
            pct = 0x19;
        } else if (year == S32(st, 0x00784C9Cu) + base) {
            pct = 0x32;
        } else if (year == base + S32(st, 0x00784CA0u)) {
            pct = 0x4B;
        }
        if (pct != 0) {
            int ok = 1;
            if (U8(st, 0x00784C3Cu) != 0 && below(st, 0x42CC, 0x00784C24u, pct)) {
                ok = 0;
            }
            if (U8(st, 0x00784C3Du) != 0 && below(st, 0x42D0, 0x00784C28u, pct)) {
                ok = 0;
            }
            if (S32(st, 0x00784C58u) != 0 && below(st, 0x24, 0x00784C5Cu, pct)) {
                ok = 0;
            }
            if (U8(st, 0x00784C3Eu) != 0 && below(st, 0x42D4, 0x00784C2Cu, pct)) {
                ok = 0;
            }
            if (U8(st, 0x00784C3Fu) != 0 && below(st, 0x42D8, 0x00784C30u, pct)) {
                ok = 0;
            }
            c = CITY(st);
            if (ok) {
                S32(st, c + 0x42D8) += 5;
            } else {
                S32(st, c + 0x42D8) -= 2;
                S32(st, c + 0x4584) = 2;
            }
        }
        rating_0x46e930(st);
        c = CITY(st);
        int32_t prev = S32(st, c + 0x458C), cur = S32(st, c + 0x42D8);
        if (prev < cur) {
            S32(st, c + 0x4590) = 2;
        } else if (prev == cur) {
            S32(st, c + 0x4590) = 1;
        } else {
            S32(st, c + 0x4590) = 0;
        }
        S32(st, c + 0x458C) = S32(st, c + 0x42D8);
    }
    if (S32(st, c + 0x42D8) < 0) {
        S32(st, c + 0x42D8) = 0;
    }
    if (S32(st, c + 0x42D8) > 100) {
        S32(st, c + 0x42D8) = 100;
    }
    rating_0x46ec70(st);
}

int32_t city_0x451b10(game_state *st)
{
    /* @bin 0x00451B10 a building of type 0xB7 with +0xC8 below 1 whose tile (0x005F5D08 by +0x54,
     * plus +0x0C) passes 0x004ED360(x, y, 3, 0) with 0x00EAAE90 clear, or one of type 0xD2 while
     * city +0x433A >= 16 (the loop runs to 0x0089B7A0: ids 1..4003) */
    for (uint32_t p = BUILDING_ADDR(1) + 0xC8; p < 0x0089B7A0u; p += BUILDING_REC_SIZE) {
        uint32_t b = p - 0xC8;
        if (U8(st, b) != 1) {
            continue;
        }
        int16_t t = S16(st, b + 0x10);
        if (t == 0xB7) {
            if (S16(st, b + 0xC8) >= 1) {
                continue;
            }
            int32_t g = (int32_t) (U32(st, 0x005F5D08u + 4u * U8(st, b + 0x54)) + U32(st, b + 0x0C) - U32(st, 0x00BCA2D8u));
            GAME_CALL(st, 0x004ED360u, g % 0xE4, g / 0xE4, 3, 0);
            if (S32(st, 0x00EAAE90u) == 0) {
                return 1;
            }
        } else if (t == 0xD2 && S16(st, CITY(st) + 0x433A) >= 0x10) {
            return 1;
        }
    }
    return 0;
}

static int32_t money_in(game_state *st, uint32_t c)
{
    return (int32_t) (U32(st, c + 0x47AC) + U32(st, c + 0x4454) + U32(st, c + 0x2C50) + U32(st, c + 0x2C68));
}

static int32_t money_out(game_state *st, uint32_t c)
{
    return (int32_t) (U32(st, c + 0x2C90) + U32(st, c + 0x2C78) + U32(st, c + 0x2C1C) + U32(st, c + 0x2C70) +
                      U32(st, c + 0x2C88));
}

void rating_0x46e410(game_state *st)
{
    /* @bin 0x0046E410 the same steps as 0x0046E1E0 (with +0x2C8C in the money test), keeping the
     * worst one: city +0x42E4 (0 rising, 1 falling or capped, 2 steady) and +0x4438 (the reason) */
    int32_t r = GAME_CALL0(st, 0x004C38F0u);
    uint32_t c = CITY(st);
    int32_t worst = 0, reason = 4, delta;
    int32_t in = money_in(st, c), out = money_out(st, c), x = S32(st, c + 0x2C8C);
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
    if (S32(st, c + 0x2974) >= 0 && S32(st, c + 0x2974) <= 1) {
        delta--;
    }
    if (S32(st, c + 0x2C68) > S32(st, c + 0x2C70)) {
        delta++;
    } else if (S32(st, c + 0x2C68) < S32(st, c + 0x2C70)) {
        if (worst < 1) {
            worst = 1;
            reason = 0xB;
        }
        delta--;
    } else if (worst == 0) {
        worst = 1;
        reason = 0xB;
    }
    if (S32(st, c + 0x2C04) > 0) {
        if (worst < 1) {
            worst = 1;
            reason = 0xA;
        }
        delta--;
    } else {
        int32_t y = S32(st, c + 0x2BFC);
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
    int32_t t = S32(st, c + 0x45A8) / 12 - S32(st, c + 0x2C0C) - r;
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
    int32_t p1 = GAME_CALL(st, 0x004CDE60u, S32(st, c + 0x2708), S32(st, c + 0x24));
    c = CITY(st);
    int32_t p2 = GAME_CALL(st, 0x004CDE60u, S32(st, c + 0x270C), S32(st, c + 0x24));
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
    if (S32(st, c + 0x44E8) != 0) {
        delta--;
        if (worst < 1) {
            worst = 1;
            reason = 8;
        }
    }
    if (S32(st, c + 0x4378) > 0) {
        delta++;
    }
    if (city_0x451b10(st)) {
        delta++;
    }
    c = CITY(st);
    int32_t cur = S32(st, c + 0x42D0), now = (int32_t) ((uint32_t) cur + (uint32_t) delta);
    if (delta == 0 || (cur <= 0 && S32(st, 0x0078D7BCu) == S16(st, 0x007848D8u))) {
        S32(st, c + 0x4438) = 0;
        S32(st, c + 0x42E4) = 2;
        return;
    }
    if (delta <= 0) {
        S32(st, c + 0x42E4) = 1;
        S32(st, c + 0x4438) = reason;
        return;
    }
    S32(st, c + 0x42E4) = 0;
    int32_t cap = S32(st, c + 0x46A8);
    if (now > cap) {
        if (cur > cap) {
            S32(st, c + 0x42E4) = 1;
        } else {
            S32(st, c + 0x42E4) = cur >= cap ? 2 : 0;
        }
        S32(st, c + 0x4438) = 3;
    } else {
        S32(st, c + 0x4438) = worst > 0 ? reason : 0;
    }
}

void rating_0x46e1e0(game_state *st)
{
    /* @bin 0x0046E1E0 city +0x42D0 (0..100, at most +0x46A8) moves by the quarter's factors, then
     * 0x0046E410 (a tail jump) */
    int32_t r = GAME_CALL0(st, 0x004C38F0u);
    uint32_t c = CITY(st);
    int32_t in = money_in(st, c), out = money_out(st, c), x = S32(st, c + 0x2C88), delta;
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
    if (S32(st, c + 0x2C68) > S32(st, c + 0x2C70)) {
        delta++;
    } else if (S32(st, c + 0x2C68) < S32(st, c + 0x2C70)) {
        delta--;
    }
    if (S32(st, c + 0x2C04) > 0) {
        delta--;
    } else {
        int32_t y = S32(st, c + 0x2BFC);
        if (y < 5) {
            delta += 2;
        } else if (y >= 0xA) {
            delta -= y < 0xF ? 1 : y < 0x14 ? 2 : 3;
        }
    }
    if (S32(st, c + 0x2974) >= 0 && S32(st, c + 0x2974) <= 1) {
        delta--;
    }
    int32_t t = S32(st, c + 0x45A8) / 12 - S32(st, c + 0x2C0C) - r;
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
    int32_t p1 = GAME_CALL(st, 0x004CDE60u, S32(st, c + 0x2708), S32(st, c + 0x24));
    c = CITY(st);
    int32_t p2 = GAME_CALL(st, 0x004CDE60u, S32(st, c + 0x270C), S32(st, c + 0x24));
    if (p1 > 0x5F) {
        delta--;
    }
    if (p2 > 0xA) {
        delta += 2;
    }
    c = CITY(st);
    if (S32(st, c + 0x44E8) != 0) {
        delta--;
    }
    if (S32(st, c + 0x4378) > 0) {
        delta++;
    }
    if (city_0x451b10(st)) {
        delta++;
    }
    c = CITY(st);
    int32_t v = (int32_t) ((uint32_t) S32(st, c + 0x42D0) + (uint32_t) delta);
    S32(st, c + 0x42D0) = v;
    if (v > S32(st, c + 0x46A8)) {
        S32(st, c + 0x42D0) = S32(st, c + 0x46A8);
    }
    if (S32(st, c + 0x42D0) < 0) {
        S32(st, c + 0x42D0) = 0;
    }
    if (S32(st, c + 0x42D0) > 100) {
        S32(st, c + 0x42D0) = 100;
    }
    rating_0x46e410(st);
}

int32_t flood_0x4bd590(game_state *st, uint32_t self)
{
    /* @bin 0x004BD590 (thiscall) this +0x14, 10 more when city +0x4478 is set and building +0x472C
     * is of level 0; at most 100 */
    int32_t v = S32(st, self + 0x14);
    uint32_t c = CITY(st);
    if (S16(st, c + 0x4478) != 0 && building_type_level(st, S32(st, c + 0x472C)) == 0) {
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
    return flood_0x4bd590(st, 0x00D05720u + 0x32C40u);
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
    {0x004BD590u, "flood_0x4bd590", 0, 32, 0x00D38360u, run_4bd590, 0},
};
const int month_ratings_ports_count = sizeof(month_ratings_ports) / sizeof(month_ratings_ports[0]);
