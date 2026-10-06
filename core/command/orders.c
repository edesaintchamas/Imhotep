/* Imhotep - player settings: finance, labour priorities, festivals, game settings.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * docs/commands.md sections 3.1-3.3, 3.9. */
#include "command/orders.h"

#include "sim/call.h"
#include "sim/math.h"
#include "state/ported.h"
#include "state/records.h"

#define BUILDING_HIGHEST 0x0078D790u
#define MONTH 0x0078D7C0u              /* i32 month of the year, 0..11 */
#define DIFFICULTY 0x00E39080u         /* i32 0..4, chunk 174 */
#define HOUSE_TAX_VALUE 0x0070CE18u    /* i32 house model field per level, stride 0x58 */
#define DIFFICULTY_TAX 0x005D1974u     /* i32 per difficulty */
#define SALARY_BY_RANK 0x005D0320u     /* i32 per rank */
#define LABOUR_ORDER 0x00605768u       /* i32[9] scratch: categories by priority */
#define LABOUR_SELECTED 0x00605988u    /* i32 the category whose priority is being set */
#define GODS_IN_CITY 0x007848E2u       /* i16 per god (scenario) */
#define FESTIVAL_TENTH 0x00775678u
#define ORIENTATION 0x00E94E04u        /* i32 city view orientation 0, 2, 4, 6 (chunk 33) */
#define VIEW_ORIENTATION 0x00BCA2E0u
#define ROTATE_LAST 0x00BC96C8u
#define MESSAGE_OPTIONS_EDIT 0x006DE510u
#define MESSAGE_OPTIONS 0x00E38EF0u
#define UNDO_POSSIBLE 0x00BC9694u

/* ---- finance (docs/commands.md 3.1) ---- */

void tax_income_estimate(game_state *st)
{
    /* @bin 0x00459D20 city +0x2C54 / +0x2C5C: tax base of the houses below / from level 0xE (house
     * model value at 0x0070CE18 + level * 0x58 times the population +0x1C), of the houses that are
     * in use (+0x00 = 1), have +0x05 set and +0x4E set */
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + 0x2C54) = 0;
    S32(st, c + 0x2C5C) = 0;
    int32_t n = S32(st, BUILDING_HIGHEST);
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1 || U8(st, b + 0x05) == 0) {
            continue;
        }
        int16_t level = S16(st, b + 0x12);
        uint32_t value = U32(st, HOUSE_TAX_VALUE + 0x58u * (uint32_t) (int32_t) level);
        if (U8(st, b + 0x4E) == 0) {
            continue;
        }
        uint32_t add = (uint32_t) (int32_t) S16(st, b + 0x1C) * value;
        if (level >= 0xE) {
            U32(st, c + 0x2C5C) += add;
        } else {
            U32(st, c + 0x2C54) += add;
        }
    }
    /* @bin 0x00459DA9 tax rate city +0x08, then the difficulty's percentage 0x005D1974[d] */
    int32_t high = calc_percentage(S32(st, c + 0x2C5C), S32(st, c + 0x08));
    high = calc_percentage(high, S32(st, DIFFICULTY_TAX + 4u * U32(st, DIFFICULTY)));
    int32_t low = calc_percentage(S32(st, c + 0x2C54), S32(st, c + 0x08));
    low = calc_percentage(low, S32(st, DIFFICULTY_TAX + 4u * U32(st, DIFFICULTY)));
    /* @bin 0x00459E1A +0x2C4C := +0x2C3C + +0x2C40; +0x45EC := it + (months left) * estimate */
    uint32_t sum = U32(st, c + 0x2C3C) + U32(st, c + 0x2C40);
    U32(st, c + 0x2C4C) = sum;
    U32(st, c + 0x45EC) = (uint32_t) (high + low) * (uint32_t) (12 - S32(st, MONTH)) + sum;
}

void wages_estimate(game_state *st)
{
    /* @bin 0x0045A8C0 +0x2C18 := +0x2C14; +0x46A0 := (12 - month) * (wages +0x2C08 * +0x2BF4
     * / 10 / 12) + +0x2C14 (32-bit product, signed divisions) */
    uint32_t c = CURRENT_CITY(st);
    int32_t v = (int32_t) (U32(st, c + 0x2C08) * U32(st, c + 0x2BF4));
    v = v / 10 / 12;
    U32(st, c + 0x2C18) = U32(st, c + 0x2C14);
    U32(st, c + 0x46A0) = (uint32_t) (12 - S32(st, MONTH)) * (uint32_t) v + U32(st, c + 0x2C14);
}

void salary_estimate(game_state *st)
{
    /* @bin 0x0045A970 +0x2C94 := +0x2CA0 */
    uint32_t c = CURRENT_CITY(st);
    U32(st, c + 0x2C94) = U32(st, c + 0x2CA0);
}

/* @bin 0x0040A260 / 0x0040A1D0 parameter 1: 0 = +1, 1 = -1, other: no change; then clamped */
static void step_clamped(game_state *st, uint32_t field, int32_t max)
{
    int32_t p1 = S32(st, CMD_PARAM1);
    if (p1 == 1) {
        S32(st, field) = (int32_t) (U32(st, field) - 1u);
    } else if (p1 == 0) {
        S32(st, field) = (int32_t) (U32(st, field) + 1u);
    }
    if (S32(st, field) < 0) {
        S32(st, field) = 0;
    } else if (S32(st, field) > max) {
        S32(st, field) = max;
    }
}

void cmd_tax_rate(game_state *st)
{
    /* @bin 0x0040A260 tax rate city +0x08, 0..25 */
    step_clamped(st, CURRENT_CITY(st) + 0x08, 0x19);
    tax_income_estimate(st);
    GAME_CALL0(st, 0x0045A990u);
    GAME_CALL0(st, 0x0045A7B0u);
    U8(st, UI_REDRAW) = 1;
}

void cmd_wages(game_state *st)
{
    /* @bin 0x0040A1D0 wages city +0x2C08, 0..100 */
    step_clamped(st, CURRENT_CITY(st) + 0x2C08, 0x64);
    wages_estimate(st);
    GAME_CALL0(st, 0x0045A990u);
    GAME_CALL0(st, 0x0045A7B0u);
    U8(st, UI_REDRAW) = 1;
}

void cmd_salary(game_state *st)
{
    /* @bin 0x0040AF30 only while city +0x44D4 <= 0: rank +0x2C9C := p1, salary +0x2C98 :=
     * 0x005D0320[p1]; back to window 7 from window 0x2C, else 9 */
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + 0x44D4) > 0) {
        return;
    }
    int32_t rank = S32(st, CMD_PARAM1);
    S32(st, c + 0x2C9C) = rank;
    U32(st, c + 0x2C98) = U32(st, SALARY_BY_RANK + 4u * (uint32_t) rank);
    salary_estimate(st);
    GAME_CALL0(st, 0x0046EC70u);
    uint8_t w = U8(st, UI_WINDOW);
    U8(st, UI_REDRAW) = 1;
    U8(st, UI_WINDOW) = w == 0x2C ? 7 : 9;
}

/* @bin 0x0040B079 donation city +0x445C clamped to the personal savings +0x4450, then to >= 0 */
static void donation_clamp(game_state *st, uint32_t c)
{
    if (S32(st, c + 0x445C) > S32(st, c + 0x4450)) {
        S32(st, c + 0x445C) = S32(st, c + 0x4450);
    }
    if (S32(st, c + 0x445C) < 0) {
        S32(st, c + 0x445C) = 0;
    }
}

void cmd_donation_amount(game_state *st)
{
    /* @bin 0x0040AFF0 p1 0..4: 0, 500, 2000, 5000, 1000000 */
    uint32_t c = CURRENT_CITY(st);
    static const int32_t amount[5] = {0, 0x1F4, 0x7D0, 0x1388, 0xF4240};
    int32_t p1 = S32(st, CMD_PARAM1);
    if (p1 >= 0 && p1 <= 4) {
        S32(st, c + 0x445C) = amount[p1];
    }
    donation_clamp(st, c);
    U8(st, UI_REDRAW) = 1;
}

void cmd_donation_step(game_state *st)
{
    /* @bin 0x0040B0B0 p1 1: -10, 0: +10 */
    uint32_t c = CURRENT_CITY(st);
    int32_t p1 = S32(st, CMD_PARAM1);
    if (p1 == 1) {
        U32(st, c + 0x445C) -= 10u;
    } else if (p1 == 0) {
        U32(st, c + 0x445C) += 10u;
    }
    donation_clamp(st, c);
    U8(st, UI_REDRAW) = 1;
}

void cmd_donate(game_state *st)
{
    /* @bin 0x0040AF90 treasury +0x0C and donated +0x4458 += amount, savings +0x4450 -= amount */
    uint32_t c = CURRENT_CITY(st);
    U32(st, c + 0x0C) += U32(st, c + 0x445C);
    U32(st, c + 0x4458) += U32(st, c + 0x445C);
    U32(st, c + 0x4450) -= U32(st, c + 0x445C);
    GAME_CALL0(st, 0x0045A990u);
    GAME_CALL0(st, 0x0045A7B0u);
    U8(st, UI_WINDOW) = 7;
    U8(st, UI_REDRAW) = 1;
}

void cmd_gift_size(game_state *st)
{
    /* @bin 0x0040B140 p1 1..3: refused when its cost (+0x4574, +0x4578, +0x457C) is above the
     * savings +0x4450; else (and for any other p1) size +0x4558 := p1 - 1 */
    uint32_t c = CURRENT_CITY(st);
    int32_t p1 = S32(st, CMD_PARAM1);
    if (p1 >= 1 && p1 <= 3 && S32(st, c + 0x4570 + 4u * (uint32_t) p1) > S32(st, c + 0x4450)) {
        return;
    }
    S32(st, c + 0x4558) = p1 - 1;
    U8(st, UI_REDRAW) = 1;
}

void cmd_gift_send(game_state *st)
{
    /* @bin 0x0040B1C0 refused when the smallest gift (+0x4574) costs more than the savings */
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + 0x4574) > S32(st, c + 0x4450)) {
        return;
    }
    gift_send(st);
    U8(st, UI_WINDOW) = 7;
    U8(st, UI_REDRAW) = 1;
}

void gift_send(game_state *st)
{
    /* @bin 0x0046EDD0 gift of size +0x4558 (0, 1, else 2), cost +0x4574 / +0x4578 / +0x457C */
    uint32_t c = CURRENT_CITY(st);
    int32_t size = S32(st, c + 0x4558);
    int32_t cost = S32(st, c + (size == 0 ? 0x4574u : size == 1 ? 0x4578u : 0x457Cu));
    if (cost > S32(st, c + 0x4450)) {
        return;
    }
    /* @bin 0x0046EE10 the gifts counter +0x4560 (<= 0, 1, 2, 3; 4 and more: no rating) and the
     * size give the change of +0x42D8 (0x004E96C0) */
    static const int8_t rating[4][3] = {{3, 5, 10}, {1, 3, 5}, {0, 1, 3}, {0, 0, 1}};
    int32_t n = S32(st, c + 0x4560);
    int k = n <= 0 ? 0 : n == 1 ? 1 : n == 2 ? 2 : n == 3 ? 3 : -1;
    if (k >= 0) {
        S32(st, c + 0x4560) = k + 1;
        size = S32(st, c + 0x4558);
        if (size >= 0 && size <= 2) {
            GAME_CALL(st, 0x004E96C0u, rating[k][size]);
        }
    }
    /* @bin 0x0046EED7 +0x455C := 0; per-size counters +0x4568 / +0x456C / +0x4570, each reset at 4 */
    c = CURRENT_CITY(st);
    S32(st, c + 0x455C) = 0;
    size = S32(st, c + 0x4558);
    if (size >= 0 && size <= 2) {
        U32(st, c + 0x4568 + 4u * (uint32_t) size) += 1u;
    }
    for (uint32_t i = 0; i < 3; i++) {
        if (S32(st, c + 0x4568 + 4 * i) >= 4) {
            S32(st, c + 0x4568 + 4 * i) = 0;
        }
    }
    U32(st, c + 0x4450) -= (uint32_t) cost;
}

/* ---- labour (3.2) ---- */

void cmd_labour_priority(game_state *st)
{
    /* @bin 0x0040A730 priority p1 (0 = none, 1..9) of the category 0x00605988; city +0x2B3C +
     * 0x14 * category */
    int32_t sel = S32(st, LABOUR_SELECTED);
    int32_t p = S32(st, CMD_PARAM1);
    U8(st, UI_WINDOW) = 7;
    uint32_t prio = CURRENT_CITY(st) + 0x2B3C;
    U8(st, UI_REDRAW) = 1;
    int32_t old = S32(st, prio + 0x14u * (uint32_t) sel);
    if (old == p) {
        return;
    }
    /* @bin 0x0040A773 the direction the category moves among equal priorities */
    int32_t dir;
    if (old == 0 && p != 0) {
        dir = 1;
    } else if (old != 0 && p == 0) {
        dir = -1;
    } else {
        dir = old >= p ? 1 : -1;
    }
    S32(st, prio + 0x14u * (uint32_t) sel) = p;
    for (uint32_t i = 0; i < 9; i++) {
        S32(st, LABOUR_ORDER + 4 * i) = (int32_t) i;
    }
    /* @bin 0x0040A7D0 9 bubble passes over the 9 categories: a category with a priority moves before
     * one without, before a larger priority, and the selected one passes its equals in `dir` */
    for (int pass = 0; pass < 9; pass++) {
        for (uint32_t j = 1; j < 9; j++) {
            int32_t cur = S32(st, LABOUR_ORDER + 4 * j);
            int32_t prev = S32(st, LABOUR_ORDER + 4 * (j - 1));
            int32_t pc = S32(st, prio + 0x14u * (uint32_t) cur);
            if (pc == 0) {
                continue;
            }
            int32_t pp = S32(st, prio + 0x14u * (uint32_t) prev);
            int swap = pp == 0 || pp > pc;
            if (pp == pc) {
                int32_t s = S32(st, LABOUR_SELECTED);
                swap = (s == cur && dir == 1) || (s == prev && dir == -1);
            }
            if (swap) {
                S32(st, LABOUR_ORDER + 4 * (j - 1)) = cur;
                S32(st, LABOUR_ORDER + 4 * j) = prev;
            }
        }
    }
    /* @bin 0x0040A84B renumber the categories with a priority 1.. in that order */
    for (uint32_t i = 0; i < 9; i++) {
        uint32_t f = prio + 0x14u * U32(st, LABOUR_ORDER + 4 * i);
        if (S32(st, f) != 0) {
            S32(st, f) = (int32_t) i + 1;
        }
    }
    GAME_CALL0(st, 0x00459400u);
    GAME_CALL0(st, 0x004595A0u);
    GAME_CALL0(st, 0x0051F780u);
    GAME_CALL0(st, 0x0051F780u);
    GAME_CALL0(st, 0x00459670u);
}

/* ---- festivals (3.3) ---- */

void cmd_festival_god(game_state *st)
{
    /* @bin 0x0040AC50 god p1 when the scenario has it (i16 0x007848E2[god] != 0): city +0x44F0 */
    int32_t god = S32(st, CMD_PARAM1);
    if (S16(st, GODS_IN_CITY + 2u * (uint32_t) god) != 0) {
        S32(st, CURRENT_CITY(st) + 0x44F0) = god;
    }
    U8(st, UI_REDRAW) = 1;
}

void cmd_festival_size(game_state *st)
{
    /* @bin 0x0040AC80 refused with a treasury <= -5000, size 3 while +0x4514 != 0, or when
     * +0x47A0 or +0x47A4 is -1; else size +0x44F4 := p1 and 0x00775678 := 10 * p1 */
    uint32_t c = CURRENT_CITY(st);
    if (S32(st, c + 0x0C) <= -5000) {
        return;
    }
    int32_t p1 = S32(st, CMD_PARAM1);
    if (S32(st, c + 0x4514) != 0 && p1 == 3) {
        return;
    }
    if (S32(st, c + 0x47A0) == -1 || S32(st, c + 0x47A4) == -1) {
        return;
    }
    S32(st, c + 0x44F4) = p1;
    U32(st, FESTIVAL_TENTH) = (uint32_t) p1 * 10u;
    U8(st, UI_REDRAW) = 1;
}

void cmd_festival_hold(game_state *st)
{
    /* @bin 0x0040AB00 refused with a treasury <= -5000 */
    int32_t player = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t c = CITY_ADDR(player);
    if (S32(st, c + 0x0C) <= -5000) {
        return;
    }
    /* @bin 0x0040AB1F god +0x4500 := +0x44F0, size +0x44F8 := +0x44F4, +0x44FC := 2 / 3 / 4 by size */
    S32(st, c + 0x4500) = S32(st, c + 0x44F0);
    S32(st, c + 0x44F8) = S32(st, c + 0x44F4);
    int32_t size = S32(st, c + 0x44F4);
    S32(st, c + 0x44FC) = size == 1 ? 2 : size == 2 ? 3 : 4;
    /* @bin 0x0040AB6A months +0x4528 / +0x452C */
    int32_t m = S32(st, c + 0x4528);
    if (m == 0) {
        S32(st, c + 0x4528) = 12;
    } else if (S32(st, c + 0x452C) != 0) {
        U32(st, c + 0x44FC) += (uint32_t) m;
    } else {
        S32(st, c + 0x452C) = 12;
    }
    /* @bin 0x0040AB9D cost +0x4504 / +0x4508 / +0x450C by size: treasury -= it, +0x4758 += it */
    size = S32(st, c + 0x44F4);
    uint32_t cost = c + (size == 1 ? 0x4504u : size == 2 ? 0x4508u : 0x450Cu);
    U32(st, c + 0x0C) -= U32(st, cost);
    U32(st, c + 0x4758) += U32(st, cost);
    /* @bin 0x0040AC0D size 3 takes +0x4510 of good 0xF from the storage yards */
    if (S32(st, c + 0x44F4) == 3) {
        GAME_CALL(st, 0x0045B4D0u, player, 0xF, S32(st, c + 0x4510));
    }
    U8(st, UI_WINDOW) = S32(st, CMD_PARAM2) != 0 ? 9 : 7;
    U8(st, UI_REDRAW) = 1;
}

/* ---- game settings (3.9) ---- */

void cmd_difficulty(game_state *st)
{
    /* @bin 0x00402E50 p1 0: +1, else -1; clamped to 0..4 */
    int32_t d = S32(st, DIFFICULTY);
    d = S32(st, CMD_PARAM1) == 0 ? d + 1 : d - 1;
    S32(st, DIFFICULTY) = d;
    if (d < 0) {
        S32(st, DIFFICULTY) = 0;
    } else if (S32(st, DIFFICULTY) >= 5) {
        S32(st, DIFFICULTY) = 4;
    }
    /* @bin 0x00402E8A the model files of the new difficulty are read again (file input: these
     * calls reach the platform's files) */
    GAME_CALL0(st, 0x00540C90u);
    GAME_CALL0(st, 0x00540ED0u);
    GAME_CALL0(st, 0x00541160u);
    GAME_CALL0(st, 0x004BC720u);
    /* @bin 0x00402E9E in window 0x35 (new game setup) the treasury := the scenario's initial funds
     * 0x007848F8 times the difficulty's percentage 0x005D1960[d] */
    if (U8(st, UI_WINDOW) == 0x35) {
        int32_t funds = calc_percentage(S32(st, 0x007848F8u), S32(st, 0x005D1960u + 4u * U32(st, DIFFICULTY)));
        S32(st, CURRENT_CITY(st) + 0x0C) = funds;
    }
    U8(st, UI_REDRAW) = 1;
}

void cmd_difficulty_lower(game_state *st)
{
    /* @bin 0x004EA840 difficulty := city +0x47A8 - 1, at most 1; +0x47A8 := it */
    uint32_t c = CURRENT_CITY(st);
    int32_t d = S32(st, c + 0x47A8) - 1;
    S32(st, DIFFICULTY) = d;
    if (d > 1) {
        d = 1;
        S32(st, DIFFICULTY) = d;
    }
    S32(st, c + 0x47A8) = d;
    U8(st, UI_WINDOW) = 1;
    U8(st, UI_REDRAW) = 1;
}

void cmd_keep_governing(game_state *st)
{
    /* @bin 0x004EA770 +0x44D0 := 1; p1 1 / 2: +0x44D4 += 24 / 60 months, +0x44D8 := them, salary
     * rank and salary := 0 */
    uint32_t c = CURRENT_CITY(st);
    int32_t p1 = S32(st, CMD_PARAM1);
    S32(st, c + 0x44D0) = 1;
    if (p1 == 1 || p1 == 2) {
        int32_t months = p1 == 1 ? 0x18 : 0x3C;
        U32(st, c + 0x44D4) += (uint32_t) months;
        S32(st, c + 0x44D8) = months;
        S32(st, c + 0x2C9C) = 0;
        S32(st, c + 0x2C98) = 0;
        salary_estimate(st);
    }
    U8(st, 0x00E38DD8u) = 0;
    U8(st, 0x00E38DD9u) = 0;
    U8(st, 0x00E38DD7u) = 0;
    U8(st, UI_WINDOW) = 1;
    U8(st, UI_REDRAW) = 1;
}

void cmd_stop_governing(game_state *st)
{
    /* @bin 0x004EA7F0 +0x44D0, +0x44D4, +0x44D8 := 0, then 0x004CB6A0 (end of the mission) */
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + 0x44D0) = 0;
    S32(st, c + 0x44D4) = 0;
    S32(st, c + 0x44D8) = 0;
    GAME_CALL0(st, 0x004CB6A0u);
    S32(st, UNDO_POSSIBLE) = 0;
    U8(st, UI_REDRAW) = 1;
}

void cmd_message_options(game_state *st)
{
    /* @bin 0x00402CC0 jump table 0x00402D64: p1 1..12 toggles bit p1 of the edited options
     * 0x006DE510; 13 applies them to 0x00E38EF0 (read by the message post 0x004E0F90) */
    int32_t p1 = S32(st, CMD_PARAM1);
    if (p1 >= 1 && p1 <= 12) {
        U32(st, MESSAGE_OPTIONS_EDIT) ^= 1u << p1;
    } else if (p1 == 13) {
        uint32_t v = U32(st, MESSAGE_OPTIONS_EDIT);
        U8(st, UI_WINDOW) = 1;
        U8(st, UI_REDRAW) = 1;
        U32(st, MESSAGE_OPTIONS) = v;
    }
}

/* @bin 0x00409EE8 the common tail: images and camera, notice 1 */
static void rotate_finish(game_state *st)
{
    GAME_CALL0(st, 0x0047D6E0u);
    GAME_CALL0(st, 0x004DE080u);
    GAME_CALL(st, 0x004E2970u, 1, 0, 0);
    U8(st, UI_REDRAW) = 1;
}

void cmd_rotate_map(game_state *st)
{
    /* @bin 0x00409EB0 p1 2: rotate one way (0x00430B20), 1: the other (0x00430BE0), 0: back to
     * orientation 0 */
    int32_t p1 = S32(st, CMD_PARAM1);
    if (p1 == 0) {
        /* @bin 0x00409F10 */
        int32_t o = S32(st, VIEW_ORIENTATION);
        if (o == 0) {
            return;
        }
        if (o == 2) {
            S32(st, ROTATE_LAST) = 1;
            GAME_CALL0(st, 0x00430BE0u);
            S32(st, VIEW_ORIENTATION) = S32(st, ORIENTATION);
        } else {
            S32(st, ROTATE_LAST) = 0;
            if (o == 6) {
                GAME_CALL0(st, 0x00430B20u);
                S32(st, VIEW_ORIENTATION) = S32(st, ORIENTATION);
            } else {
                GAME_CALL0(st, 0x00430B20u);
                GAME_CALL0(st, 0x00430B20u);
                S32(st, VIEW_ORIENTATION) = S32(st, ORIENTATION);
                GAME_CALL0(st, 0x0047D6E0u);
            }
        }
        rotate_finish(st);
        return;
    }
    if (p1 == 2) {
        GAME_CALL0(st, 0x00430B20u);
    } else if (p1 == 1) {
        GAME_CALL0(st, 0x00430BE0u);
    }
    S32(st, ROTATE_LAST) = S32(st, CMD_PARAM1);
    S32(st, VIEW_ORIENTATION) = S32(st, ORIENTATION);
    rotate_finish(st);
}

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_tax, cmd_tax_rate)
V(run_wages, cmd_wages)
V(run_salary, cmd_salary)
V(run_donation_amount, cmd_donation_amount)
V(run_donation_step, cmd_donation_step)
V(run_donate, cmd_donate)
V(run_gift_size, cmd_gift_size)
V(run_gift_send_cmd, cmd_gift_send)
V(run_gift_send, gift_send)
V(run_tax_estimate, tax_income_estimate)
V(run_wages_estimate, wages_estimate)
V(run_salary_estimate, salary_estimate)
V(run_labour, cmd_labour_priority)
V(run_festival_god, cmd_festival_god)
V(run_festival_size, cmd_festival_size)
V(run_festival_hold, cmd_festival_hold)
V(run_difficulty, cmd_difficulty)
V(run_difficulty_lower, cmd_difficulty_lower)
V(run_keep_governing, cmd_keep_governing)
V(run_stop_governing, cmd_stop_governing)
V(run_message_options, cmd_message_options)
V(run_rotate, cmd_rotate_map)

const ported_function orders_ports[] = {
    {0x0040A260u, "cmd_tax_rate", 0, 0, 0, run_tax, 0},
    {0x0040A1D0u, "cmd_wages", 0, 0, 0, run_wages, 0},
    {0x0040AF30u, "cmd_salary", 0, 0, 0, run_salary, 0},
    {0x0040AFF0u, "cmd_donation_amount", 0, 0, 0, run_donation_amount, 0},
    {0x0040B0B0u, "cmd_donation_step", 0, 0, 0, run_donation_step, 0},
    {0x0040AF90u, "cmd_donate", 0, 0, 0, run_donate, 0},
    {0x0040B140u, "cmd_gift_size", 0, 0, 0, run_gift_size, 0},
    {0x0040B1C0u, "cmd_gift_send", 0, 0, 0, run_gift_send_cmd, 0},
    {0x0046EDD0u, "gift_send", 0, 0, 0, run_gift_send, 0},
    {0x00459D20u, "tax_income_estimate", 0, 0, 0, run_tax_estimate, 0},
    {0x0045A8C0u, "wages_estimate", 0, 0, 0, run_wages_estimate, 0},
    {0x0045A970u, "salary_estimate", 0, 0, 0, run_salary_estimate, 0},
    {0x0040A730u, "cmd_labour_priority", 0, 0, 0, run_labour, 0},
    {0x0040AC50u, "cmd_festival_god", 0, 0, 0, run_festival_god, 0},
    {0x0040AC80u, "cmd_festival_size", 0, 0, 0, run_festival_size, 0},
    {0x0040AB00u, "cmd_festival_hold", 0, 0, 0, run_festival_hold, 0},
    {0x00402E50u, "cmd_difficulty", 0, 0, 0, run_difficulty, 0},
    {0x004EA840u, "cmd_difficulty_lower", 0, 0, 0, run_difficulty_lower, 0},
    {0x004EA770u, "cmd_keep_governing", 0, 0, 0, run_keep_governing, 0},
    {0x004EA7F0u, "cmd_stop_governing", 0, 0, 0, run_stop_governing, 0},
    {0x00402CC0u, "cmd_message_options", 0, 0, 0, run_message_options, 0},
    {0x00409EB0u, "cmd_rotate_map", 0, 0, 0, run_rotate, 0},
};
const int orders_ports_count = sizeof(orders_ports) / sizeof(orders_ports[0]);
