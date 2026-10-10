/* Imhotep - monument progress summaries (package day2): the goods the monuments still need
 * (0x004F57B0, called by the daily trade pass 0x00443330) and the summaries of the two kinds of
 * monument it reads (0x00542AD0, 0x004DF040), which fill their caller's stack structure.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day2_monument.h"

#include "city/monument_rating.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"


static int32_t figure_kind(game_state *st, int32_t f)
{
    return U8(st, FIGURE_ADDR(f) + FIG_TYPE);
}

int32_t monument_d5_worker_busy(game_state *st, int32_t b)
{
    /* @bin 0x00542110 */
    uint32_t slot = BUILDING_ADDR(b) + BLD_U5C;
    int32_t f = S32(st, slot);
    if (f == 0) {
        return 0;
    }
    uint32_t fr = FIGURE_ADDR(f);
    if (U8(st, fr + FIG_STATE) == 0) {
        /* @bin 0x00542147 a dead figure: forgotten */
        S32(st, slot) = 0;
        return 0;
    }
    uint32_t action = U8(st, fr + FIG_ACTION);
    if (action == 6 || action == 7) {
        action = U8(st, fr + FIG_ACTION_BEFORE_ATTACK); /* @bin 0x00542163 the action before the current one */
    }
    switch (U8(st, fr + FIG_TYPE)) {
    case 0x4F: /* @bin 0x005421B0 */
        if (action == 9 || action == 10) {
            return 1;
        }
        break;
    case 0x51: /* @bin 0x005421A1 */
        if (action == 9 || action == 0xB || action == 10) {
            return 1;
        }
        break;
    case 0x55: /* @bin 0x00542184 */
        if (action == 0xF || action == 0x10 || action == 0x17 || action == 0x18 || action == 0x19) {
            return 1;
        }
        break;
    default:
        break;
    }
    S32(st, slot) = 0;
    return 0;
}

void monument_count_kind(game_state *st, int32_t kind, int32_t *out)
{
    /* @bin 0x004204A0 13 entries of 16 bytes: +0 a figure kind, +4 a pointer to an object whose
     * word +0x12 tells whether it is live */
    *out = 0;
    for (uint32_t e = G_U596084; e < G_U596154; e += 0x10) {
        uint32_t p = U32(st, e + 4);
        if (P16(st, p + 0x12) != 0 && S32(st, e) == kind) {
            (*out)++;
        }
    }
}

int32_t monument_d5_status(game_state *st, int32_t b, int32_t *out)
{
    /* @bin 0x00542AD0 the 100 parts from the first one (b - S16 +0x12): their figures (+0x5C,
     * kinds 0x4F, 0x51, 0x55), their stage (+0x58) by role (table 0x00601DE4, 0x34-byte entries,
     * by the part's S16 +0x12) */
    int32_t p = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    uint32_t flags = U32(st, BUILDING_ADDR(p) + BLD_U64);
    uint8_t flag2 = (uint8_t) ((flags >> 1) & 1);
    int32_t zero = 0, left = 0, n1 = 0, n2 = 0, n3 = 0, q = 0, q10 = 0, u = 0, o = 0, n4 = 0;
    int32_t stage = 1;
    uint32_t work = 0;
    for (int32_t k = 0; k < 100; k++, p++) {
        uint32_t r = BUILDING_ADDR(p);
        if ((uint8_t) monument_d5_worker_busy(st, p) != 0) {
            int32_t f = S32(st, r + BLD_U5C);
            if (f == 0) {
                return 0; /* @bin 0x00542B55 (out not written) */
            }
            switch (figure_kind(st, f)) {
            case 0x4F: o++; break;
            case 0x51:
                if (U8(st, FIGURE_ADDR(f) + FIG_ACTION) == 0xA) {
                    q10++;
                }
                q++;
                break;
            case 0x55: u++; break;
            default: break;
            }
        }
        uint32_t role = U32(st, G_MONUMENT_PART_KINDS + 0x34u * (uint32_t) (int32_t) S16(st, r + BLD_PART));
        uint32_t v = U32(st, r + BLD_MONUMENT_STAGE);
        if (role > 0xC) {
            continue;
        }
        switch (role) {
        case 0: /* @bin 0x00542BC3 */
            left += (int32_t) (0xDu - v);
            if (v > 0xD) {
                break;
            }
            if (v == 0) {
                stage = (flag2 != 0) + 1;
            } else if (v <= 3) {
                stage = 2;
            } else if (v <= 0xC) {
                stage = 3;
            } else {
                stage = 4;
            }
            break;
        case 1: /* @bin 0x00542C1D */
            if (v == 0) {
                zero++;
            } else if (v != 1) {
                break;
            }
            n1++;
            work += 400;
            break;
        case 2: case 3: case 4: case 5: case 6: case 7: /* @bin 0x00542C33 */
            if (v == 0) {
                zero++;
            } else if (v != 1) {
                break;
            }
            n2++;
            work += 400;
            break;
        case 8: case 9: case 10: case 11: /* @bin 0x00542C49 */
            if (v == 0) {
                zero++;
            } else if (v != 1) {
                break;
            }
            n3++;
            break;
        default: /* 0xC, @bin 0x00542C5F: falls through the later stages */
            if (v > 3) {
                break;
            }
            if (v == 0) {
                zero++;
            }
            n4 += (int32_t) (v == 3 ? 1 : v == 2 ? 2 : 3);
            work += 400u * (v == 3 ? 1u : v == 2 ? 2u : 3u);
            break;
        }
    }
    int32_t c4f = 0, c51 = 0;
    monument_count_kind(st, 0x4F, &c4f);
    monument_count_kind(st, 0x51, &c51);
    /* @bin 0x00542CD5 (signed division by 213) */
    int32_t pct = (int32_t) ((uint32_t) (0xD5 - n4 - n3 - n2 - left - zero) * 100u) / 0xD5;
    if (out != NULL) {
        out[0] = stage;
        out[1] = zero;
        out[2] = left;
        out[3] = n1;
        out[4] = n2;
        out[5] = n3;
        out[6] = n4;
        out[7] = c51 + q;
        out[8] = q10;
        out[9] = u;
        out[10] = c4f + o;
        out[0xB] = (int32_t) work;
        out[0xC] = pct;
        out[0xD] = 0x9470;
    }
    return pct;
}

int32_t monument_de_status(game_state *st, int32_t b, int32_t *out)
{
    /* @bin 0x004DF040 the 116 parts from the first one; roles from table 0x005E2AE4 (low byte of
     * 0x34-byte entries) */
    int32_t p = b - S16(st, BUILDING_ADDR(b) + BLD_PART);
    uint32_t flags = U32(st, BUILDING_ADDR(p) + BLD_U64);
    uint8_t bit0 = (uint8_t) (flags & 1), bit1 = (uint8_t) ((flags >> 1) & 1);
    int32_t level = 0;
    if (bit0 != 0) {
        /* @bin 0x004DF0B0 */
        level = (flags & 0x10) != 0 ? 3 : 4;
        if (flags & 0x20) {
            level--;
        }
        if (flags & 0x40) {
            level--;
        }
        if (flags & 0x80) {
            level--;
        }
    }
    int32_t zero = 0, missing = 0, n = 0, q = 0, q10 = 0, u = 0, o = 0, stage = 1;
    uint32_t work = 0;
    for (int32_t k = 0; k < 0x74; k++, p++) {
        uint32_t r = BUILDING_ADDR(p);
        if ((uint8_t) GAME_CALL(st, FN_MASTABA_WORKER_BUSY, p) != 0) {
            int32_t f = S32(st, r + BLD_U5C);
            if (f == 0) {
                return 0; /* @bin 0x004DF104 (out not written) */
            }
            switch (figure_kind(st, f)) {
            case 0x4F: o++; break;
            case 0x51:
                if (U8(st, FIGURE_ADDR(f) + FIG_ACTION) == 0xA) {
                    q10++;
                }
                q++;
                break;
            case 0x55: u++; break;
            default: break;
            }
        }
        uint32_t role = U32(st, G_MONUMENT_PARTS + 0x34u * (uint32_t) (int32_t) S16(st, r + BLD_PART)) & 0xFF;
        uint32_t v = U32(st, r + BLD_MONUMENT_STAGE);
        if (role <= 3) {
            /* @bin 0x004DF173 16 blocks, one bit each */
            int32_t set = 0;
            for (int i = 0; i < 16; i++) {
                if (v & (1u << i)) {
                    set++;
                }
            }
            missing += 16 - set;
            work += (uint32_t) (16 - set) * 400u;
            if (set != 0) {
                stage = 2;
            }
        } else if (role <= 0xC && role != 0xB) {
            /* @bin 0x004DF1BC */
            if (v == 0) {
                zero++;
                n++;
            } else if (v == 1) {
                n++;
            }
        }
    }
    if (zero == 0 && stage == 1) {
        stage = 2;
    }
    if (bit1 != 0 || bit0 != 0) {
        stage = 3;
    }
    /* @bin 0x004DF219 (signed division by 288) */
    int32_t pct = (int32_t) ((uint32_t) (0x120 - n - missing - zero) * 100u) / 0x120;
    if (out != NULL) {
        out[0] = stage;
        out[1] = zero;
        out[2] = missing;
        out[3] = n;
        out[4] = q;
        out[5] = q10;
        out[6] = u;
        out[7] = (int32_t) work;
        out[8] = pct;
        out[9] = 0x6400;
        out[10] = o;
        out[0xB] = level;
    }
    return pct;
}

/* @bin 0x004F5954 by the kind of an empty slot (word table 0x00784DE0, 1..27): the quantity
 * needed of goods 0xC, 0x18, 0x19, 0x1A, 0x1E (jump table 0x004F5B5C) */
static int32_t slot_plan_need(int32_t kind, int32_t good)
{
    switch (kind) {
    case 1: case 13:
        return good == 0x18 ? 0x12C0 : good == 0x19 ? 0x4B00 : 0;
    case 2: case 14:
        return good == 0x18 ? 0x5460 : good == 0x19 ? 0xA8C0 : 0;
    case 3:
        return good == 0xC ? 0x12C0 : good == 0x19 ? 0x4B00 : 0;
    case 4:
        return good == 0xC ? 0x5460 : good == 0x19 ? 0xA8C0 : 0;
    case 5:
        return good == 0xC ? 0x10680 : good == 0x19 ? 0x12C00 : 0;
    case 6:
        return good == 0xC ? 0x23280 : good == 0x19 ? 0x1D4C0 : 0;
    case 7:
        return good == 0xC ? 0x40740 : good == 0x19 ? 0x27100 : 0;
    case 8: return good == 0x18 ? 0x12C0 : 0;
    case 9: return good == 0x18 ? 0x5460 : 0;
    case 10: return good == 0x18 ? 0x10680 : 0;
    case 11: return good == 0x18 ? 0x23280 : 0;
    case 12: return good == 0x18 ? 0x40740 : 0;
    case 15:
        return good == 0x18 ? 0x10680 : good == 0x19 ? 0x12C00 : 0;
    case 16:
        return good == 0x18 ? 0x23280 : good == 0x19 ? 0x1D4C0 : 0;
    case 17:
        return good == 0x18 ? 0x40740 : good == 0x19 ? 0x27100 : 0;
    case 18: return good == 0xC ? 0x2A30 : 0;
    case 19: return good == 0xC ? 0x5DC0 : 0;
    case 20: return good == 0xC ? 0xA410 : 0;
    case 22: return good == 0x1A ? 0x2710 : 0;
    case 23: return good == 0x1A ? 0x4E20 : 0;
    case 24: return good == 0x1E ? 0x9470 : 0;
    case 25: case 26: case 27: return good == 0x1E ? 0xC1C0 : 0;
    default: return 0; /* 21 and out of range */
    }
}

int32_t monument_goods_needed(game_state *st, int32_t good)
{
    /* @bin 0x004F57B0 three slots (0x00EA5FAC, 0x1276 dwords per city 0x00EAAC28): a slot with a
     * building in use asks that monument what is left; an empty slot counts the whole plan */
    uint32_t need = 0;
    for (int32_t slot = 0; slot < 3; slot++) {
        uint32_t i = (uint32_t) S32(st, G_PLAYER) * 0x1276u + (uint32_t) slot;
        int32_t b = S32(st, CITY_BASE + CITY_MONUMENTS + 4u * i);
        if (b == 0 || U8(st, BUILDING_ADDR(b) + BLD_STATE) != 1) {
            need += (uint32_t) slot_plan_need(S16(st, G_MONUMENT_SLOTS + 2u * (uint32_t) slot), good);
            continue;
        }
        int16_t type = S16(st, BUILDING_ADDR(b) + BLD_TYPE);
        if (type == 0xDE) {
            int32_t s[0xC] = {0}; /* always written: 0x004E0530 is true only with a figure */
            monument_de_status(st, b, s);
            if (good == 0x1E) {
                need += (uint32_t) s[7];
            }
        } else if (type == 0xD5) {
            int32_t s[0xE] = {0}; /* always written, as above (0x00542110) */
            monument_d5_status(st, b, s);
            if (good == 0x1E) {
                need += (uint32_t) s[0xB];
            }
        } else if (type == 0xB7) {
            int32_t s[0x1C];
            pyramid_status(st, b, s); /* month2's port of 0x004F1310 */
            /* @bin 0x004F5867 by pyramid kind (jump table 0x004F5B48): the stones left, s[8]
             * and s[9], 400 each */
            int32_t a = s[8], c = s[9];
            switch (S32(st, G_PYRAMID_KIND)) {
            case 1: case 4:
                if (good == 0x19) {
                    if (c > 0) {
                        need += (uint32_t) c * 400u;
                    }
                } else if (good == 0x18 && a > 0) {
                    need += (uint32_t) a * 400u;
                }
                break;
            case 2:
                if (good == 0x19) {
                    if (c > 0) {
                        need += (uint32_t) c * 400u;
                    }
                } else if (good == 0xC && a > 0) {
                    need += (uint32_t) a * 400u;
                }
                break;
            case 3:
                if (good == 0x18 && (a > 0 || c > 0)) {
                    need += (uint32_t) (a + c) * 400u;
                }
                break;
            case 5:
                if (good == 0xC && (a > 0 || c > 0)) {
                    need += (uint32_t) (a + c) * 400u;
                }
                break;
            default:
                break;
            }
        }
    }
    return (int32_t) need;
}

static int32_t run_d5_busy(game_state *st, const int32_t *a)
{
    return monument_d5_worker_busy(st, a[0]);
}

/* The structure the summaries fill: written through when it lies in the state; the callers'
 * stack is not state (their ports call the C functions above). */
static int32_t run_status(game_state *st, const int32_t *a, uint32_t address, int n,
                          int32_t (*f)(game_state *, int32_t, int32_t *))
{
    if (a[1] == 0) {
        return f(st, a[0], NULL);
    }
    uint8_t *dst = state_ptr(st, (uint32_t) a[1], 4u * (uint32_t) n);
    if (dst == NULL) {
        state_unported(st, address);
        return 0;
    }
    int32_t out[0xE];
    for (int i = 0; i < n; i++) {
        out[i] = PTR_AT(st, state_s32, (uint32_t) a[1] + 4u * (uint32_t) i);
    }
    int32_t r = f(st, a[0], out);
    for (int i = 0; i < n; i++) {
        PTR_AT(st, state_s32, (uint32_t) a[1] + 4u * (uint32_t) i) = out[i];
    }
    return r;
}

static int32_t run_d5_status(game_state *st, const int32_t *a)
{
    return run_status(st, a, FN_MONUMENT_D5_STATUS, 0xE, monument_d5_status);
}

static int32_t run_de_status(game_state *st, const int32_t *a)
{
    return run_status(st, a, FN_MONUMENT_DE_STATUS, 0xC, monument_de_status);
}

static int32_t run_count_kind(game_state *st, const int32_t *a)
{
    if (state_ptr(st, (uint32_t) a[1], 4) == NULL) {
        state_unported(st, FN_MONUMENT_COUNT_KIND);
        return 0;
    }
    int32_t n;
    monument_count_kind(st, a[0], &n);
    PTR_AT(st, state_s32, (uint32_t) a[1]) = n;
    return 0;
}

static int32_t run_goods_needed(game_state *st, const int32_t *a)
{
    return monument_goods_needed(st, a[0]);
}

const ported_function day2_monument_ports[] = {
    {0x00542110u, "monument_d5_worker_busy", 1, 8, 0, run_d5_busy, 0},
    {0x004204A0u, "monument_count_kind", 2, 0, 0, run_count_kind, 0},
    {0x00542AD0u, "monument_d5_status", 2, 32, 0, run_d5_status, 0},
    {0x004DF040u, "monument_de_status", 2, 32, 0, run_de_status, 0},
    {0x004F57B0u, "monument_goods_needed", 1, 32, 0, run_goods_needed, 0},
};
const int day2_monument_ports_count = sizeof(day2_monument_ports) / sizeof(day2_monument_ports[0]);
