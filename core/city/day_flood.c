/* Imhotep - the Nile flood, every tick (0x004BE770).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The flood's timing is computed by the original in x87 doubles (53-bit precision): the
 * period 0x00D05718 (a double), the constants 0.01, 9792.0, 1.0, 25.0, 0.5 and 1/6 of .rdata.
 * Each operation is reproduced exactly with city/day_x87.h. */
#include "city/day_flood.h"

#include "city/day_x87.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define PERIOD 0x00D05718u /* double: days between two floods */
#define TICK 0x0078D7CCu
#define LEVEL 0x32FACu  /* i32: flood level 0..30 */
#define PHASE 0x32FB4u  /* i32: 1 soon, 2 rising, 3 peak, 4 receding, 5 just over, 6 none */
#define RISE 0x32FB8u   /* i32 */

static x87_val dbl(game_state *st, uint32_t addr)
{
    return x87_from_double((uint64_t) U32(st, addr) | (uint64_t) U32(st, addr + 4) << 32);
}

static uint32_t level_record(game_state *st, uint32_t self)
{
    return self + 0x32C64u + 0x1Cu * U32(st, self + LEVEL);
}

void flood_tick(game_state *st, uint32_t self)
{
    /* @bin 0x004BE770 */
    if (U8(st, self + 0x32FBC) == 0) {
        S32(st, self + PHASE) = 6;
        S32(st, self + 0x32C54) = 0;
        return;
    }
    if (S32(st, TICK) == 1 && S32(st, self + PHASE) != 2) {
        /* @bin 0x004BE796 the quality +0x32C54 moves toward +0x32C5C by 5 a day */
        int32_t target = S32(st, self + 0x32C5C), q = S32(st, self + 0x32C54);
        if (q > target) {
            S32(st, self + 0x32C54) = q - 5 <= target ? target : q - 5;
        } else if (q < target) {
            S32(st, self + 0x32C54) = q + 5 >= target ? target : q + 5;
        }
    }
    int32_t years = S32(st, 0x0078D7BCu) - S16(st, 0x007848D8u);
    int32_t base = S32(st, self + 0x32C4C);
    int32_t rise = S32(st, self + RISE);
    x87_val period = dbl(st, PERIOD);
    /* @bin 0x004BE7DB the flood's start and end in days of 25 ticks... of the whole game */
    int32_t shift = x87_ftol(x87_mul(x87_from_int(years), period));
    int32_t start = base * 105 / 100 + shift + 0xF;
    int32_t end = S32(st, self + 0x32C50) + rise * 2 + start;
    int32_t ramp = x87_ftol(x87_mul(x87_mul(x87_from_int(S32(st, self + 0x32C60)), x87_from_int(rise)),
                                    dbl(st, 0x0056F598u)));
    int32_t md = S32(st, 0x0078D7C0u) * 16 + S32(st, 0x0078D7C4u);
    int32_t total = x87_ftol(x87_mul(x87_from_int(years), dbl(st, 0x0056F680u))) + S32(st, TICK) + md * 51;
    x87_val ftotal = x87_from_int(total);
    x87_val f25 = dbl(st, 0x0056F678u);
    int32_t before = x87_ftol(x87_divv(x87_sub(ftotal, dbl(st, 0x0056F520u)), f25));
    int32_t now = x87_ftol(x87_divv(ftotal, f25));
    if (x87_cmp(x87_mul(period, dbl(st, 0x0056F690u)), x87_from_int(start - now)) < 0) {
        int32_t p = x87_ftol(period);
        start -= p;
        end -= p;
    }
    S32(st, self + 0x32FB0) = base / 30;
    int32_t to_start = start - now, after_end = now - end;
    if (now >= start && now < end) {
        int32_t lvl;
        if (now < start + ramp) {
            S32(st, self + PHASE) = 2;
            lvl = to_start + 0x1E;
        } else if (now >= end - ramp) {
            S32(st, self + PHASE) = 4;
            lvl = after_end + 0x1F;
        } else {
            S32(st, self + PHASE) = 3;
            lvl = 0x1E - ramp;
        }
        lvl--;
        S32(st, self + LEVEL) = lvl > 0x1E ? 0x1E : lvl < 0 ? 0 : lvl;
    } else {
        S32(st, self + LEVEL) = 0x1E;
        if (to_start >= 0 && to_start < 0x1C) {
            S32(st, self + PHASE) = 1;
        } else if (after_end >= 0 && after_end < 0x1C) {
            S32(st, self + PHASE) = 5;
        } else {
            S32(st, self + PHASE) = 6;
        }
    }
    if (now != before) {
        /* @bin 0x004BE9C6 a new flood day */
        if (now >= start && now < end) {
            if (now < start + ramp || now >= end - ramp) {
                U8(st, 0x00EAAC2Cu) = 1;
            } else if (now == start + rise) {
                for (uint32_t k = 0; k < 0x1E; k++) {
                    S32(st, self + 0x32C68 + 0x1C * k) = 0;
                }
            }
        } else if (to_start == 0x31) {
            GAME_CALLT0(st, 0x00BCA350u, 0x00489510u);
        } else if (to_start == 1) {
            GAME_CALLT0(st, self, 0x004BE490u);
        } else if (after_end == 1) {
            GAME_CALLT0(st, self, 0x004BE580u);
        }
    }
    if (now >= end - rise) {
        /* @bin 0x004BEA51 the receding water, sixth of a day by sixth */
        int32_t k = rise + after_end;
        if (k < 0) {
            k += x87_ftol(period);
        }
        int32_t sixth = x87_ftol(x87_divv(ftotal, x87_mul(f25, dbl(st, 0x0056F688u)))) % 6;
        int32_t step = sixth + k * 6;
        int32_t limit = rise * 60;
        int32_t first = limit < step ? limit : step;
        if (step < limit && first != S32(st, 0x005DF760u)) {
            S32(st, 0x005DF760u) = step;
            int32_t r = step % rise;
            if (r >= rise - 1) {
                r = rise - 1;
            }
            int32_t lvl = S32(st, self + LEVEL) - r;
            if (lvl >= 0 && lvl < S32(st, self + LEVEL)) {
                GAME_CALLT0(st, self + 0x32C64u + 0x1Cu * (uint32_t) lvl, 0x004BE0D0u);
            }
        }
        S32(st, 0x005DF75Cu) = step;
    }
    /* @bin 0x004BEAE5 the progress of the flood day in percent */
    int32_t days = x87_ftol(f25);
    int32_t pct = total % days * 100 / days;
    if (S32(st, self + PHASE) == 2) {
        GAME_CALLT(st, level_record(st, self), 0x004BD850u, 1, pct, 0);
        S32(st, 0x00BC9694u) = 0;
    } else if (S32(st, self + PHASE) == 4) {
        GAME_CALLT(st, level_record(st, self), 0x004BD850u, 0, pct, 0);
        S32(st, 0x00BC9694u) = 0;
    }
}

static int32_t run_flood_tick(game_state *st, const int32_t *a)
{
    (void) a;
    flood_tick(st, 0x00D05720u);
    return 0;
}

const ported_function day_flood_ports[] = {
    {0x004BE770u, "flood_tick", 0, 0, 0x00D05720u, run_flood_tick, 0},
};
const int day_flood_ports_count = sizeof(day_flood_ports) / sizeof(day_flood_ports[0]);
