/* Imhotep - the monument rating (0x0046EF50 and its value 0x004F78A0) (package month2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004F78A0 returns an x87 double: the square root of a sum of integers, times 6.32, plus 0.5,
 * minus 1 (at least 0) while a monument is unfinished. Every term of the sum is an i32 loaded by
 * fild and added with fadd: the partial sums stay integers far below 2^53, so the x87 additions
 * are exact and the sum is computed here as an i64. The rest (fsqrt, fmul, fadd, fsub, fcomp, then
 * __ftol in the caller) is reproduced with the 53-bit rounding of core/city/day_x87.h and
 * core/city/monument_rating_x87.h. */
#include "city/monument_rating.h"

#include "city/monument_rating_x87.h"
#include "sim/call.h"
#include "sim/math.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

/* @bin 0x00570128, 0x0056F690, 0x0056F520, 0x0056F5A0 the doubles 6.32, 0.5, 1.0, 0.0 (read from
 * the state's .rdata) */

static uint64_t rd64(game_state *st, uint32_t a)
{
    return (uint64_t) U32(st, a) | ((uint64_t) U32(st, a + 4) << 32);
}

int32_t monuments_planned(game_state *st)
{
    /* @bin 0x004F4FF0 the scenario's monuments (three i16 at 0x00784DE0) that are set */
    int32_t n = 0;
    for (uint32_t p = G_MONUMENT_SLOTS; p < G_MONUMENT_SLOTS + 2 * 3; p += 2) {
        if (S16(st, p) != 0) {
            n++;
        }
    }
    return n;
}

/* @bin 0x004F7D40 / 0x004F7D68 (and the copies 0x004F7E00 / 0x004F7E28) the extra terms: entries
 * 0..29 of an i32 list taken, /32 or /16 (signed, toward zero) */
static int extra_term(int32_t c, int *by16)
{
    static const uint8_t skip[30] = {0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0};
    static const uint8_t sixteen[30] = {0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0};
    if (c < 0 || c > 0x1D || skip[c]) {
        return 0;
    }
    *by16 = sixteen[c];
    return 1;
}

static int64_t extra_sum(game_state *st, uint32_t test, uint32_t value)
{
    /* entries c = 0..0x22 (only 0..0x1D can count): when the i32 at test + 4c is set, the one at
     * value + 4c /32 or /16 */
    int64_t s = 0;
    for (int32_t c = 0; c < 0x23; c++) {
        int by16;
        if (!extra_term(c, &by16)) {
            continue;
        }
        if (S32(st, test + 4u * (uint32_t) c) == 0) {
            continue;
        }
        int32_t v = S32(st, value + 4u * (uint32_t) c);
        s += by16 ? v / 16 : v / 32;
    }
    return s;
}

/* the pyramid tables of the scenario's kind 0x005E4EF4 (1..5), by the monument's size +0x54 */
#define PYR_STEPS(st, sz) (S32(st, G_PYRAMID_KIND) == 5 ? S32(st, G_PYRAMID_PARTS_KIND5 + 4u * (sz)) : S32(st, G_PYRAMID_PARTS + 4u * (sz)))

static int32_t pyr_level_cost(game_state *st, uint32_t sz, uint32_t level)
{
    return U8(st, G_PYR_SKIP + sz * 6 + level) != 0 ? 3 : 6;
}

void pyramid_level_parts(game_state *st, int32_t b)
{
    /* @bin 0x004EF450 the parts of the pyramid of part b at its lowest level being built: 0x00EAAC44
     * (i32 list, 0x00EAAF58 entries); stage +0x55, level +0x57, course +0x56 decide (stage 0 parts
     * the lowest, later stages the highest; done stages skipped) */
    S32(st, G_PLACE_COUNT) = 0;
    int32_t best = 0;
    uint32_t sz = U8(st, BUILDING_ADDR(b) + BLD_MONUMENT_KIND);
    int32_t b0 = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    for (int32_t q = b0;; q++) {
        int32_t mode = S32(st, G_PYRAMID_KIND);
        if (q >= PYR_STEPS(st, sz) + b0) {
            return;
        }
        uint32_t r = BUILDING_ADDR(q);
        uint8_t c = U8(st, r + BLD_U55);
        if (c != 0 && (U8(st, r + BLD_INVENTORY) == 0 || mode == 3 || mode == 5)) {
            continue;
        }
        int reset = 0, add = 0;
        if (best == 0) {
            reset = 1;
        } else {
            uint32_t rb = BUILDING_ADDR(best);
            if (c != U8(st, rb + BLD_U55)) {
                if (c != 0) {
                    continue;
                }
                reset = 1;
            } else {
                uint8_t d = U8(st, rb + BLD_U57), e1 = U8(st, r + BLD_U57);
                uint8_t f = U8(st, r + BLD_INVENTORY), g = U8(st, rb + BLD_INVENTORY);
                if (c == 0) {
                    if (e1 < d) {
                        reset = 1;
                    } else if (e1 != d) {
                        continue;
                    } else if (f < g) {
                        reset = 1;
                    } else if (f != g) {
                        continue;
                    } else {
                        add = 1;
                    }
                } else {
                    if (e1 > d) {
                        reset = 1;
                    } else if (e1 != d) {
                        continue;
                    } else if (f > g) {
                        reset = 1;
                    } else if (f != g) {
                        continue;
                    } else {
                        add = 1;
                    }
                }
            }
        }
        if (reset) {
            S32(st, G_PLACE_COUNT) = 0;
            best = q;
        }
        if (reset || add) {
            int32_t n = S32(st, G_PLACE_COUNT);
            S32(st, G_PLACES + 4u * (uint32_t) n) = q;
            S32(st, G_PLACE_COUNT) = n + 1;
        }
    }
}

int32_t pyramid_part_unfinished(game_state *st, int32_t b)
{
    /* @bin 0x004EFD90 part b's level +0x57 below +0x58, and not on its last course (+0x56 full: 6, 3
     * by table 0x005E68D8 in kind 1, 1 for the first part in kind 5) of the last level */
    uint32_t r = BUILDING_ADDR(b);
    uint8_t l = U8(st, r + BLD_U57), top = U8(st, r + BLD_MONUMENT_STAGE);
    if (l >= top) {
        return 0;
    }
    int32_t full;
    if (S32(st, G_PYRAMID_KIND) == 1) {
        full = pyr_level_cost(st, U8(st, r + BLD_MONUMENT_KIND), l);
    } else if (S32(st, G_PYRAMID_KIND) == 5 && S16(st, r + BLD_PART) == 0) {
        full = 1;
    } else {
        full = 6;
    }
    if ((int32_t) U8(st, r + BLD_INVENTORY) != full || l + 1 != top) {
        return 1;
    }
    return 0;
}

int32_t figure_works_on(game_state *st, int32_t f, int32_t b)
{
    /* @bin 0x004F26A0 figure f alive, of type 0x55, at work (+0x38, or +0x10 while 6 / 7: 0xF..0x14,
     * 0x1D, 0x1E) on building b (+0x68) */
    uint32_t r = FIGURE_ADDR(f);
    if (U8(st, r + FIG_STATE) == 0 || U8(st, r + FIG_TYPE) != 0x55) {
        return 0;
    }
    uint32_t a = U8(st, r + FIG_ACTION);
    if (a == 6 || a == 7) {
        a = U8(st, r + FIG_ACTION_BEFORE_ATTACK);
    }
    if (a == 0xF || a == 0x10 || a == 0x11 || a == 0x1E || a == 0x12 || a == 0x13 || a == 0x1D || a == 0x14) {
        return S16(st, r + FIG_WORKSITE) == b;
    }
    return 0;
}

int32_t pyramid_part_done(game_state *st, int32_t b)
{
    /* @bin 0x004F53D0 the part's stage at most 1, no part left at its level (0x004EF450), and for kinds
     * with a ramp (0x004ECEA0) the ramp from the tile 0x005F5D08 by size free (0x004ED360 mode 3) */
    uint32_t r = BUILDING_ADDR(b);
    if (U8(st, r + BLD_U55) > 1) {
        return 0;
    }
    pyramid_level_parts(st, b);
    if (S32(st, G_PLACE_COUNT) != 0) {
        return 0;
    }
    uint32_t sz = U8(st, r + BLD_MONUMENT_KIND);
    if ((uint8_t) GAME_CALL(st, FN_PYRAMID_KIND_OK, (int32_t) sz) != 0) {
        int32_t g = (int32_t) (U32(st, G_PYRAMID_RAMP_START + 4u * sz) + U32(st, r + BLD_GRID_OFFSET) - U32(st, G_MAP_START));
        GAME_CALL(st, FN_RAMP_WALK, g % 0xE4, g / 0xE4, 3, 0);
        if (S32(st, G_RAMP_WALK_TILE) != 0) {
            return 0;
        }
    }
    return 1;
}

void pyramid_status(game_state *st, int32_t b, int32_t *out)
{
    /* @bin 0x004F1310 (cdecl: building, the caller's 0x70-byte structure) the state of the pyramid
     * (type 0xB7) whose part b is: its parts from b - S16(+0x12) on (one per step; record +0x55 the
     * stage, +0x56..+0x58 levels, +0x64 the worker figure), the work done and left (+0x10.. +0x2C),
     * the workers (+0x48.. +0x64), the percents +0x30 and +0x34. Its callees 0x004EEA10,
     * 0x004EF450, 0x004EFD90, 0x004F26A0 are called in the original's order */
    for (int i = 0; i < 0x1C; i++) {
        out[i] = 0;
    }
    if (S16(st, BUILDING_ADDR(b) + BLD_TYPE) != 0xB7) {
        return;
    }
    int32_t b0 = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    uint32_t r0 = BUILDING_ADDR(b0);
    uint32_t sz = U8(st, r0 + BLD_MONUMENT_KIND);
    int32_t mode = S32(st, G_PYRAMID_KIND);
    int32_t total = mode <= 0 ? 0 : mode <= 4 ? S32(st, G_U5F5C88 + 4 * sz) : mode == 5 ? S32(st, G_U5F5C9C + 4 * sz) : 0;
    out[0x11] = total;
    int32_t base;
    switch (S32(st, G_PYRAMID_KIND)) {
    case 1: base = S32(st, G_PYRAMID_WORK_CLIMATE1 + 4 * sz); break;
    case 2: case 4: base = S32(st, G_PYRAMID_WORK_CLIMATE2 + 4 * sz); break;
    case 3: base = S32(st, G_PYRAMID_WORK_CLIMATE3 + 4 * sz); break;
    case 5: base = S32(st, G_PYRAMID_WORK_CLIMATE5 + 4 * sz); break;
    default: base = 0; break;
    }
    int32_t extra; /* @bin 0x004F13D3 (table 0x004F1C60) */
    switch (S32(st, G_PYRAMID_KIND)) {
    case 1: extra = S32(st, G_U56F82C + 4 * sz); break;
    case 2: case 4: extra = (int32_t) ((uint32_t) S32(st, G_PYRAMID_PARTS + 4 * sz) * 6u); break;
    default: extra = 0; break;
    }
    if (U8(st, r0 + BLD_U55) > 1) {
        /* @bin 0x004F143C the stone stages: +0x40 the work by stage (+0x55) and level bits (+0x5B) */
        static const int32_t bits[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
        out[0] = 1;
        out[0xF] = 0;
        out[0xE] = 999;
        out[0x10] = 0;
        for (int32_t n = 0; n < PYR_STEPS(st, sz); n++) {
            uint32_t r = BUILDING_ADDR(b0 + n);
            int32_t v = U8(st, r + BLD_U55);
            out[0xF] = v > out[0xF] ? v : out[0xF];
            out[0xE] = v < out[0xE] ? v : out[0xE];
            int16_t f = S16(st, r + BLD_U64);
            if (f != 0 && U8(st, FIGURE_ADDR(f) + FIG_TYPE) == 0x55 && (uint8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, b0 + n) != 0) {
                out[0x19]++;
            }
            uint8_t lb = U8(st, r + BLD_U5B);
            if (lb > 0xF && v >= 4 && v <= 10 && v != 5) {
                /* the original reads past its 16-entry table, into its own stack frame */
                state_unported(st, FN_PYRAMID_STATUS);
                return;
            }
            int32_t l = bits[lb & 0xF];
            switch (v) {
            case 4: out[0x10] += l; break;
            case 5: out[0x10] += 5; break;
            case 6: out[0x10] += l + 5; break;
            case 7: out[0x10] += l + 9; break;
            case 8: out[0x10] += l + 0xD; break;
            case 9: out[0x10] += l + 0x11; break;
            case 10: out[0x10] += l + 0x15; break;
            default: break;
            }
        }
        if (out[0xF] == 0xB) {
            out[0x10]++;
        }
        out[0xC] = (int32_t) ((uint32_t) out[0x10] * 100u) / out[0x11];
        out[0xD] = (int32_t) ((uint32_t) out[0xC] * 5u) / 100;
        for (int32_t k = 0; k < PYR_STEPS(st, sz);) {
            int32_t v;
            if (S32(st, G_PYRAMID_KIND) == 5) {
                v = S32(st, G_U5E657C + 8u * ((uint32_t) k + sz * 0x24));
                if (k == 0) {
                    out[0xA]++;
                    k++;
                    continue;
                }
            } else {
                v = S32(st, G_PYRAMID_PART_CORNER + 4u + 8u * (sz * 0x90 + (uint32_t) k));
                if (S32(st, G_PYRAMID_KIND) == 1) {
                    for (int32_t i = 0; i < v; i++) {
                        out[0xA] += pyr_level_cost(st, sz, (uint32_t) i);
                    }
                    out[0xB] += pyr_level_cost(st, sz, U8(st, BUILDING_ADDR(b0 + k) + BLD_MONUMENT_STAGE));
                    k++;
                    continue;
                }
            }
            out[0xA] = (int32_t) ((uint32_t) out[0xA] + (uint32_t) v * 6u);
            if (extra != 0) {
                out[0xB] += 6;
            } else {
                out[0xA] += 6;
            }
            k++;
        }
    } else {
        /* @bin 0x004F171A the earlier stages */
        out[0x10] = total;
        pyramid_level_parts(st, b0);
        int32_t bb = S32(st, G_PLACE_COUNT) > 0 ? S32(st, G_PLACES) : b0;
        uint32_t rb = BUILDING_ADDR(bb);
        out[1] = (int32_t) (U8(st, rb + BLD_INVENTORY) + U8(st, rb + BLD_U57) * 6u);
        out[2] = out[1] + 1;
        switch (S32(st, G_PYRAMID_KIND)) {
        case 1: out[3] = S32(st, G_U56F80C + 4 * sz); break;
        case 5: out[3] = S32(st, G_U56F820 + 4 * sz); break;
        default: out[3] = S32(st, G_U56F7F8 + 4 * sz); break;
        }
        if (S32(st, G_PLACE_COUNT) < 1) {
            out[0] = 4;
            out[0xC] = 100;
            out[0xD] = 100;
        } else {
            int32_t done = S16(st, r0 + BLD_UC8);
            int32_t e2 = base - done, num, den;
            if (U8(st, rb + BLD_U55) == 0) {
                out[0] = 2;
                num = base - done;
                den = base - extra;
            } else {
                out[0] = 3;
                num = extra - done;
                den = extra;
            }
            out[0xC] = (int32_t) ((uint32_t) num * 100u) / den;
            out[0xD] = (int32_t) ((uint32_t) e2 * 0x5Fu) / base + 5;
        }
        for (int32_t n = 0; n < PYR_STEPS(st, sz); n++) {
            int32_t q = b0 + n;
            int32_t m = S32(st, G_PYRAMID_KIND);
            int32_t v = m == 5 ? S32(st, G_U5E657C + 8u * ((uint32_t) n + 0x24u * sz))
                               : S32(st, G_PYRAMID_PART_CORNER + 4u + 8u * (sz * 0x90 + (uint32_t) n));
            uint32_t r = BUILDING_ADDR(q);
            int first5 = 0; /* @bin 0x004F19DF reached directly: +0x28 + 1 */
            if (U8(st, r + BLD_U55) == 0) {
                int32_t a = U8(st, r + BLD_U57);
                int32_t c = a < v ? a : v;
                if (c < 0) {
                    c = 0;
                }
                if (m == 1) {
                    for (int32_t i = 0; i < c; i++) {
                        out[4] += pyr_level_cost(st, sz, (uint32_t) i);
                    }
                } else if (m != 5 || n != 0) {
                    out[4] = (int32_t) ((uint32_t) out[4] + (uint32_t) c * 6u);
                }
                if (U8(st, r + BLD_U57) < U8(st, r + BLD_MONUMENT_STAGE) || extra == 0) {
                    out[4] += U8(st, r + BLD_INVENTORY);
                } else {
                    out[5] += U8(st, r + BLD_INVENTORY);
                }
            } else if (m == 1) {
                for (int32_t i = 0; i < v; i++) {
                    out[4] += pyr_level_cost(st, sz, (uint32_t) i);
                }
                out[5] += pyr_level_cost(st, sz, U8(st, r + BLD_U57));
            } else {
                if (m == 5 && n == 0) {
                    out[4]++;
                } else {
                    out[4] = (int32_t) ((uint32_t) out[4] + (uint32_t) v * 6u);
                }
                if (extra != 0) {
                    out[5] += 6;
                } else if (S32(st, G_PYRAMID_KIND) == 5 && n == 0) {
                    first5 = 1;
                } else {
                    out[4] += 6;
                }
            }
            /* @bin 0x004F19D1 the work planned */
            if (first5) {
                out[0xA]++;
            } else if (S32(st, G_PYRAMID_KIND) == 5) {
                if (n == 0) {
                    out[0xA]++;
                } else {
                    out[0xA] = (int32_t) ((uint32_t) out[0xA] + (uint32_t) v * 6u);
                    if (extra != 0) {
                        out[0xB] += 6;
                    } else {
                        out[0xA] += 6;
                    }
                }
            } else if (S32(st, G_PYRAMID_KIND) == 1) {
                for (int32_t i = 0; i < v; i++) {
                    out[0xA] += pyr_level_cost(st, sz, (uint32_t) i);
                }
                out[0xB] += pyr_level_cost(st, sz, U8(st, r + BLD_MONUMENT_STAGE));
            } else {
                out[0xA] = (int32_t) ((uint32_t) out[0xA] + (uint32_t) v * 6u);
                if (extra != 0) {
                    out[0xB] += 6;
                } else {
                    out[0xA] += 6;
                }
            }
            /* @bin 0x004F1A5C the part's worker figure (+0x64) by its type (table 0x004F1C90) */
            int16_t f = S16(st, r + BLD_U64);
            if (f == 0) {
                continue;
            }
            uint32_t fr = FIGURE_ADDR(f);
            int busy = 0;
            switch (U8(st, fr + FIG_TYPE)) {
            case 0x4F:
                if ((uint8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, q) != 0) {
                    out[0x16]++;
                }
                continue;
            case 0x55:
                if ((uint8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, q) != 0) {
                    out[0x19]++;
                }
                continue;
            case 0x50:
            case 0x51: {
                if ((uint8_t) GAME_CALL(st, FN_PYRAMID_WORKER_BUSY, q) == 0) {
                    continue;
                }
                int is50 = U8(st, fr + FIG_TYPE) == 0x50;
                if (is50) {
                    out[0x13]++;
                } else {
                    out[0x12]++;
                }
                out[0x18]++;
                if (U8(st, fr + FIG_ACTION) != (is50 ? 0xB : 0xA)) {
                    continue;
                }
                int16_t g = S16(st, fr + FIG_LEADER);
                if (g != 0 && figure_works_on(st, g, f) != 0) {
                    continue;
                }
                busy = 1;
                break;
            }
            default:
                continue;
            }
            if (busy) {
                /* @bin 0x004F1B19 */
                out[0x17]++;
                if (pyramid_part_unfinished(st, q) == 0 && extra != 0) {
                    out[0x15]++;
                } else {
                    out[0x14]++;
                }
            }
        }
        if (out[0] == 2) {
            for (int32_t j = 0; j < S32(st, G_PLACE_COUNT); j++) {
                if (pyramid_part_unfinished(st, S32(st, G_PLACES + 4u * (uint32_t) j)) == 0 && extra != 0) {
                    out[7]++;
                } else {
                    out[6]++;
                }
            }
        }
    }
    /* @bin 0x004F1BC2 halves (toward zero) and the worker totals * 200 */
    out[8] = (out[0xA] - out[4]) / 2;
    out[9] = (out[0xB] - out[5]) / 2;
    out[0xA] /= 2;
    out[0xB] /= 2;
    out[4] /= 2;
    out[5] /= 2;
    out[6] /= 2;
    out[7] /= 2;
    out[0x1A] = (int32_t) ((uint32_t) out[0x14] * 200u);
    out[0x1B] = (int32_t) ((uint32_t) out[0x15] * 200u);
}

/* The progress of the other monuments (thiscall on their manager objects, the record of the
 * monument's first part b0 = b - its +0x12). Each asks the manager (its virtual function +0x24, on
 * b0) whether work is going on; a working monument without its worker figure (+0x5C) is at 0 (the
 * stage-counted ones). Their second argument `out` (NULL: none) is a record of up to 14 dwords the caller passes (the
 * info panel's 0x00EADB38 in the state, the victory scoring's in its frame): filled with the stage, the workers by figure type (0x4F,
 * 0x51, 0x51 with +0x40 = 0xA, 0x55; for the part-built ones counts of 0x51, 0x55, 0x6C), the time
 * left and the percent, by each function's layout below. */

static int32_t manager_busy(game_state *st, uint32_t self, int32_t b0)
{
    uint32_t fn = U32(st, U32(st, self) + 0x24);
    return (uint8_t) GAME_CALLT(st, self, fn, b0) != 0;
}

static int32_t pct_by_stage(game_state *st, int32_t v, uint32_t k)
{
    /* fild v; fmul k (1 / stages); fmul 100.0 (0x0056F470); __ftol */
    x87_val r = x87_mul(x87_from_int(v), x87_from_double(rd64(st, k)));
    r = x87_mul(r, x87_from_double(rd64(st, G_K_100)));
    return x87_ftol(r);
}

/* the worker of a busy stage-built monument (+0x5C of b0): its figure type +0x12 (0x00BCD602) sets
 * f[0] (0x4F), f[1] (0x51), f[2] (0x51 with +0x40 = 0xA), f[3] (0x55); 0 when it has none */
static int32_t stage_workers(game_state *st, uint32_t r, int32_t f[4])
{
    int32_t w = S32(st, r + BLD_U5C);
    if (w == 0) {
        return 0;
    }
    uint32_t fig = (uint32_t) w * 0x184u;
    switch (U8(st, FIGURE_BASE + FIG_TYPE + fig)) {
    case 0x4F: f[0] = 1; break;
    case 0x51:
        f[1] = 1;
        if (U8(st, FIGURE_BASE + FIG_ACTION + fig) == 0xA) {
            f[2] = 1;
        }
        break;
    case 0x55: f[3] = 1; break;
    default: break;
    }
    return 1;
}

/* @bin 0x0040DA28 the time left (i32 arithmetic): (k - stage terms) * 400, 0 when negative */
static int32_t time_left(int32_t a)
{
    int32_t t = (int32_t) ((uint32_t) a * 400u);
    return t < 0 ? 0 : t;
}

int32_t monument_progress_401540(game_state *st, int32_t b, int32_t *out)
{
    /* @bin 0x00401540 (stdcall, called as thiscall) the first part's +0x54 of 56 (0x0056F478);
     * out: +0 the stage, +0xC the percent */
    int32_t b0 = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    int32_t v = S32(st, BUILDING_ADDR(b0) + BLD_MONUMENT_STAGES);
    int32_t p = v == 0x38 ? 100 : pct_by_stage(st, v, G_U56F478);
    if (out != NULL) {
        out[0] = v;
        out[3] = p;
    }
    return p;
}

int32_t monument_progress_40d940(game_state *st, uint32_t self, int32_t b, int32_t *out)
{
    /* @bin 0x0040D940 (thiscall) the stage +0x58 of 21 (0x0056F4C8); 0 when working without its
     * worker (out untouched); time left 0x3840 up to stage 3, then (0x24 - (stage - 3) * self +4) *
     * 400; out: +0 stage, +0x1C / +0x20 / +0x24 / +0x28 the workers 0x51 / 0x51 at 0xA / 0x55 /
     * 0x4F, +0x2C the time, +0x30 the percent, +0x34 0x3840 */
    int32_t b0 = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    uint32_t r = BUILDING_ADDR(b0);
    int32_t v = S32(st, r + BLD_MONUMENT_STAGE);
    int32_t p = v == 0x15 ? 100 : pct_by_stage(st, v, G_U56F4C8);
    int32_t f[4] = {0, 0, 0, 0};
    if (manager_busy(st, self, b0) && stage_workers(st, r, f) == 0) {
        return 0;
    }
    int32_t t = v > 3 ? time_left(0x24 - (int32_t) ((uint32_t) (v - 3) * U32(st, self + 4))) : 0x3840;
    if (out != NULL) {
        out[11] = t;
        out[8] = f[2];
        out[9] = f[3];
        out[7] = f[1];
        out[0] = v;
        out[10] = f[0];
        out[12] = p;
        out[13] = 0x3840;
    }
    return p;
}

int32_t monument_progress_417050(game_state *st, uint32_t self, int32_t b, int32_t *out)
{
    /* @bin 0x00417050 (thiscall) stage +0x58: 19 is 100, else 5 per stage + 5; time left (0x26 -
     * self +4 * stage) * 400; out: +0 stage, +4 / +8 / +0xC / +0x10 the workers 0x51 / 0x51 at 0xA /
     * 0x55 / 0x4F, +0x14 the time, +0x18 the percent, +0x1C 0x3B60 */
    int32_t b0 = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    uint32_t r = BUILDING_ADDR(b0);
    int32_t v = S32(st, r + BLD_MONUMENT_STAGE);
    int32_t p = v == 0x13 ? 100 : (int32_t) ((uint32_t) v * 5u + 5u);
    int32_t f[4] = {0, 0, 0, 0};
    if (manager_busy(st, self, b0) && stage_workers(st, r, f) == 0) {
        return 0;
    }
    int32_t t = time_left(0x26 - (int32_t) (U32(st, self + 4) * (uint32_t) v));
    if (out != NULL) {
        out[0] = v;
        out[2] = f[2];
        out[3] = f[3];
        out[5] = t;
        out[1] = f[1];
        out[4] = f[0];
        out[6] = p;
        out[7] = 0x3B60;
    }
    return p;
}

int32_t monument_progress_4e6710(game_state *st, uint32_t self, int32_t b, int32_t *out)
{
    /* @bin 0x004E6710 (thiscall) stage +0x58: 9 is 100, else 10 per stage + 10; time left (9 -
     * stage) * self +4 * 400; out: +0 stage, +4 / +8 / +0xC / +0x10 the workers 0x51 / 0x51 at 0xA /
     * 0x55 / 0x4F, +0x14 the time, +0x18 the percent, +0x1C self +4 * 3600 */
    int32_t b0 = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    uint32_t r = BUILDING_ADDR(b0);
    int32_t v = S32(st, r + BLD_MONUMENT_STAGE);
    int32_t p = v == 9 ? 100 : (int32_t) (((uint32_t) v * 5u + 5u) * 2u);
    int32_t f[4] = {0, 0, 0, 0};
    if (manager_busy(st, self, b0) && stage_workers(st, r, f) == 0) {
        return 0;
    }
    int32_t t = time_left((int32_t) ((uint32_t) (9 - v) * U32(st, self + 4)));
    if (out != NULL) {
        out[0] = v;
        out[1] = f[1];
        out[2] = f[2];
        out[3] = f[3];
        out[4] = f[0];
        out[5] = t;
        out[6] = p;
        out[7] = (int32_t) (U32(st, self + 4) * 3600u);
    }
    return p;
}

int32_t monument_progress_parts(game_state *st, uint32_t self, int32_t b, int32_t n, int32_t *out)
{
    /* @bin 0x00546790 (thiscall, n parts) the work +0x60 of the n parts from b0: 100.0f / (n * 4.0f)
     * * sum (0x0056F6DC, 0x0056F550; fdivr, fimul), 100 above 99.9 (0x005701E0), else __ftol; the
     * manager is asked once per part (on b0) until a working part has no worker; the workers of the
     * working parts counted by figure type 0x51, 0x55, 0x6C; out: +4 / +8 / +0x10 those counts, +0x14
     * the percent, +0x18 b0's dword +0x98 */
    int32_t b0 = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    int32_t sum = 0;
    for (int32_t i = 0; i < n; i++) {
        sum = (int32_t) ((uint32_t) sum + U32(st, BUILDING_ADDR(b0 + i) + BLD_MONUMENT_WORK));
    }
    x87_val t = x87_mul(x87_from_int(n), x87_from_float(U32(st, G_U56F550)));
    t = x87_divv(x87_from_float(U32(st, G_K_100F)), t);
    t = x87_mul_int(t, sum);
    int32_t p = x87_cmp(t, x87_from_double(rd64(st, G_U5701E0))) > 0 ? 100 : x87_ftol(t);
    int32_t c51 = 0, c55 = 0, c6c = 0;
    for (int32_t i = 0; i < n; i++) {
        if (!manager_busy(st, self, b0)) {
            continue;
        }
        int32_t w = S32(st, BUILDING_ADDR(b0 + i) + BLD_U5C);
        if (w == 0) {
            break;
        }
        switch (U8(st, FIGURE_BASE + FIG_TYPE + (uint32_t) w * 0x184u)) {
        case 0x51: c51++; break;
        case 0x55: c55++; break;
        case 0x6C: c6c++; break;
        default: break;
        }
    }
    if (out != NULL) {
        out[5] = p;
        out[1] = c51;
        out[2] = c55;
        out[4] = c6c;
        out[6] = S32(st, BUILDING_ADDR(b0) + BLD_U98);
    }
    return p;
}

/* the percent a monument of the city is done, by its building type (table 0x004F7DC0, handlers
 * 0x004F7D88): 0 no term, 1 a term (*p), 2 left to the original (type 0xB7: 0x004F1310 fills a
 * structure in the caller's stack) */
static int monument_percent(game_state *st, int32_t b, int *unfinished, int32_t *p)
{
    uint32_t r = BUILDING_ADDR(b);
    switch (S16(st, r + BLD_TYPE)) {
    case 0xB7: {
        /* @bin 0x004F79D9 a pyramid part not done (+0x38): its percent 0x004F1310 (+0x34),
         * unfinished unless 0x004F53D0 */
        if (S16(st, r + BLD_PREV_PART) != 0) {
            return 0;
        }
        int32_t s70[0x1C];
        pyramid_status(st, b, s70);
        if (st->unported) {
            return 2;
        }
        if (pyramid_part_done(st, b) == 0) {
            *unfinished = 1;
        }
        *p = s70[0xD];
        return 1;
    }
    case 0xCF:
    case 0xD0:
        *p = U8(st, r + BLD_U55);
        break;
    case 0xD2: {
        /* @bin 0x004F7A22 city +0x433A (i16) of 16 */
        int16_t a = S16(st, CURRENT_CITY(st) + CITY_U433A);
        if (a < 0x10) {
            *unfinished = 1;
        }
        *p = (int32_t) a * 100 / 16;
        return 1;
    }
    case 0xD5: *p = GAME_CALL(st, FN_MONUMENT_D5_STATUS, b, 0); break;
    case 0xDE: *p = GAME_CALL(st, FN_MONUMENT_DE_STATUS, b, 0); break;
    case 0xE1: *p = monument_progress_40d940(st, G_MONUMENT_E1, b, NULL); break;
    case 0xE3: *p = monument_progress_417050(st, G_MONUMENT_E3, b, NULL); break;
    case 0xE4: *p = monument_progress_4e6710(st, G_MONUMENT_E4, b, NULL); break;
    case 0xE5: *p = monument_progress_parts(st, G_MONUMENT_E5, b, 9, NULL); break;      /* 0x00545740 */
    case 0xE6: *p = monument_progress_401540(st, b, NULL); break;
    case 0xEA: *p = monument_progress_parts(st, G_MONUMENT_EA, b, 0xA, NULL); break;    /* 0x00558BE0 */
    case 0xEB: *p = monument_progress_parts(st, G_MONUMENT_EB, b, 0x15, NULL); break;   /* 0x00555E80 */
    case 0xEC: *p = monument_progress_parts(st, G_MONUMENT_EC, b, 0x1F, NULL); break;   /* 0x0054F470 */
    default: return 0;
    }
    if (*p < 0x64) {
        *unfinished = 1;
    }
    return 1;
}

int monument_rating_value(game_state *st, int32_t planned, x87_val *out)
{
    /* @bin 0x004F78A0 (cdecl, byte argument): with `planned`, the scenario's monuments (their
     * values 0x005D19D4 by type) and the extra terms of 0x00784DE8; else the city's three
     * monuments (building city +0x473C, type city +0x4730: weight i16 0x005D19C8 + 16 type + 0xC
     * times the percent done, 0x004CDE40) and the extra terms of 0x00784E7C when 0x00784DEC is set.
     * Returns 0 when the value is left to the original */
    int64_t sum = 0;
    int unfinished = 0;
    if (monuments_planned(st) != 0) {
        if ((int8_t) planned != 0) {
            for (uint32_t p = G_MONUMENT_SLOTS; p < G_MONUMENT_SLOTS + 2 * 3; p += 2) {
                int16_t t = S16(st, p);
                if (t != 0) {
                    sum += S16(st, G_MONUMENT_MODELS + MONMODEL_U0C + 16u * (uint32_t) (int32_t) t);
                }
            }
            sum += extra_sum(st, G_GOAL_TARGETS, G_GOAL_TARGETS);
        } else {
            uint32_t c = CURRENT_CITY(st);
            for (uint32_t k = 0; k < 3; k++) {
                int32_t b = S32(st, c + CITY_MONUMENTS + 4 * k);
                if (b == 0) {
                    continue;
                }
                uint32_t w = G_MONUMENT_MODELS + 16u * U32(st, c + CITY_MONUMENT_TYPES + 4 * k);
                int32_t p = 0;
                int k2 = monument_percent(st, b, &unfinished, &p);
                if (k2 == 2) {
                    return 0;
                }
                if (k2 == 1) {
                    sum += calc_percentage(S16(st, w + 0xC), p);
                }
            }
            sum += extra_sum(st, G_GOAL_TARGETS, G_COUNTERS);
        }
    }
    if (sum < 0) {
        /* fsqrt of a negative number: the indefinite NaN, which the caller's __ftol makes 0 */
        out->neg = 0;
        out->m = 0;
        out->e = -1000;
        return -1;
    }
    x87_val r = x87_sqrt_int((uint64_t) sum);
    r = x87_mul(r, x87_from_double(rd64(st, G_K_SCALE)));
    r = x87_add(r, x87_from_double(rd64(st, G_K_HALF)));
    if (unfinished) {
        r = x87_sub(r, x87_from_double(rd64(st, G_K_ONE)));
        x87_val zero = x87_from_double(rd64(st, G_K_ZERO));
        if (x87_cmp(zero, r) > 0) {
            r = zero;
        }
    }
    *out = r;
    return 1;
}

void monument_rating_update(game_state *st)
{
    /* @bin 0x0046EF50 city +0x42D4 := __ftol(0x004F78A0(0)) within 0..100; city +0x443C := 0 */
    x87_val v;
    int ok = monument_rating_value(st, 0, &v);
    if (ok == 0) {
        state_unported(st, FN_MONUMENT_RATING_UPDATE);
        return;
    }
    int32_t r = ok < 0 ? 0 : x87_ftol(v);
    uint32_t c = CURRENT_CITY(st);
    S32(st, c + CITY_RATING_MONUMENT) = r;
    if (r < 0) {
        S32(st, c + CITY_RATING_MONUMENT) = 0;
    }
    if (S32(st, c + CITY_RATING_MONUMENT) > 0x64) {
        S32(st, c + CITY_RATING_MONUMENT) = 0x64;
    }
    S32(st, CURRENT_CITY(st) + CITY_RATING_MONUMENT_REASON) = 0;
}

static int32_t run_update(game_state *st, const int32_t *a)
{
    (void) a;
    monument_rating_update(st);
    return 0;
}

static int32_t run_planned(game_state *st, const int32_t *a)
{
    (void) a;
    return monuments_planned(st);
}

static int32_t run_level_parts(game_state *st, const int32_t *a) { pyramid_level_parts(st, a[0]); return 0; }
static int32_t run_part_unfinished(game_state *st, const int32_t *a) { return pyramid_part_unfinished(st, a[0]); }
static int32_t run_works_on(game_state *st, const int32_t *a) { return figure_works_on(st, a[0], a[1]); }
static int32_t run_part_done(game_state *st, const int32_t *a) { return pyramid_part_done(st, a[0]); }

#define THIS_OBJ ((uint32_t) a[PORT_THIS_ARG])
/* the output record at a state address (0: none), as the original's callers pass it */
#define OUT_REC 14
static int32_t *out_in(game_state *st, uint32_t a, int32_t *rec)
{
    if (a == 0) {
        return NULL;
    }
    for (uint32_t i = 0; i < OUT_REC; i++) {
        rec[i] = S32(st, a + 4 * i);
    }
    return rec;
}
static int32_t out_back(game_state *st, uint32_t a, const int32_t *rec, int32_t r)
{
    for (uint32_t i = 0; a != 0 && i < OUT_REC; i++) {
        S32(st, a + 4 * i) = rec[i];
    }
    return r;
}
#define OUT_CALL(expr) \
    int32_t rec[OUT_REC]; \
    int32_t *out = out_in(st, (uint32_t) a[1], rec); \
    return out_back(st, (uint32_t) a[1], rec, expr)
static int32_t run_p401540(game_state *st, const int32_t *a) { OUT_CALL(monument_progress_401540(st, a[0], out)); }
static int32_t run_p40d940(game_state *st, const int32_t *a) { OUT_CALL(monument_progress_40d940(st, THIS_OBJ, a[0], out)); }
static int32_t run_p417050(game_state *st, const int32_t *a) { OUT_CALL(monument_progress_417050(st, THIS_OBJ, a[0], out)); }
static int32_t run_p4e6710(game_state *st, const int32_t *a) { OUT_CALL(monument_progress_4e6710(st, THIS_OBJ, a[0], out)); }
static int32_t run_p545740(game_state *st, const int32_t *a) { OUT_CALL(monument_progress_parts(st, THIS_OBJ, a[0], 9, out)); }
static int32_t run_p558be0(game_state *st, const int32_t *a) { OUT_CALL(monument_progress_parts(st, THIS_OBJ, a[0], 0xA, out)); }
static int32_t run_p555e80(game_state *st, const int32_t *a) { OUT_CALL(monument_progress_parts(st, THIS_OBJ, a[0], 0x15, out)); }
static int32_t run_p54f470(game_state *st, const int32_t *a) { OUT_CALL(monument_progress_parts(st, THIS_OBJ, a[0], 0x1F, out)); }
static int32_t run_p546790(game_state *st, const int32_t *a)
{
    int32_t rec[OUT_REC];
    int32_t *out = out_in(st, (uint32_t) a[2], rec);
    return out_back(st, (uint32_t) a[2], rec, monument_progress_parts(st, THIS_OBJ, a[0], a[1], out));
}

const ported_function month2_monument_ports[] = {
    {0x00401540u, "monument_progress_401540", 2, 32, G_MONUMENT_E6, run_p401540, 0},
    {0x0040D940u, "monument_progress_40d940", 2, 32, G_MONUMENT_E1, run_p40d940, 0},
    {0x00417050u, "monument_progress_417050", 2, 32, G_MONUMENT_E3, run_p417050, 0},
    {0x004E6710u, "monument_progress_4e6710", 2, 32, G_MONUMENT_E4, run_p4e6710, 0},
    {0x00546790u, "monument_progress_parts", 3, 32, 0, run_p546790, 0},
    {0x00545740u, "monument_progress_parts 9", 2, 32, G_MONUMENT_E5, run_p545740, 0},
    {0x00558BE0u, "monument_progress_parts 10", 2, 32, G_MONUMENT_EA, run_p558be0, 0},
    {0x00555E80u, "monument_progress_parts 21", 2, 32, G_MONUMENT_EB, run_p555e80, 0},
    {0x0054F470u, "monument_progress_parts 31", 2, 32, G_MONUMENT_EC, run_p54f470, 0},
    {0x004EF450u, "pyramid_level_parts", 1, 0, 0, run_level_parts, 0},
    {0x004EFD90u, "pyramid_part_unfinished", 1, 8, 0, run_part_unfinished, 0},
    {0x004F26A0u, "figure_works_on", 2, 8, 0, run_works_on, 0},
    {0x004F53D0u, "pyramid_part_done", 1, 8, 0, run_part_done, 0},
    {0x0046EF50u, "monument_rating_update", 0, 0, 0, run_update, 0},
    {0x004F4FF0u, "monuments_planned", 0, 32, 0, run_planned, 0},
    {0x004F78A0u, "monument_rating_value (x87 result, in monument_rating.c)", 1, 0, 0, NULL, 0},
    {0x004F1310u, "pyramid_status (writes its caller's stack, in monument_rating.c)", 2, 0, 0, NULL, 0},
};
const int month2_monument_ports_count = sizeof(month2_monument_ports) / sizeof(month2_monument_ports[0]);
