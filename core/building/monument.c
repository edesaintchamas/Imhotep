/* Imhotep - monuments: which monument a worker figure goes to (package bigb2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004BC140 asks the monuments' handlers in turn for a building that takes the figure (a
 * pyramid part, a mastaba, a sun temple, an obelisk, ...); the first that does sets the figure's
 * destination (figure +0x24 x, +0x26 y, +0x28 tile, +0x44 := 0) and returns non-zero. */
#include "building/monument.h"

#include "building/building_spawn.h"
#include "figure/actions_a.h"
#include "figure/actions_d.h"
#include "map/terrain_land.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define CLIMATE 0x005E4EF4u /* 5: no pyramids (the handlers and tables of the other set) */

#define B(id) BUILDING_ADDR(id)
#define F(id) FIGURE_ADDR(id)
#define USED(id) U8(st, B(id))
#define TYPE(id) S16(st, B(id) + 0x10)
#define PART(id) S16(st, B(id) + 0x12)

/* @bin 0x004F3000 (and the same code in 0x00420350, 0x004E5C00) the figure heads for the
 * building: +0x24 := its +0x26, +0x26 := its +0x28, +0x28 := its tile +0x0C, +0x44 := 0 */
static int32_t figure_goes_to(game_state *st, int32_t f, int32_t b)
{
    S16(st, F(f) + 0x24) = S16(st, B(b) + 0x26);
    S16(st, F(f) + 0x26) = S16(st, B(b) + 0x28);
    int32_t tile = S32(st, B(b) + 0x0C);
    S32(st, F(f) + 0x28) = tile;
    S16(st, F(f) + 0x44) = 0;
    return tile;
}

int32_t pyramid_part_ready(game_state *st, int32_t id)
{
    /* @bin 0x004EF100 (id) a pyramid part (kind +0x54) that is not done (+0x55 != 1, or its main
     * building's +0x55 != 1): the entry of its part number +0x12 in the kind's list (0x005F3878 +
     * 0x3C0 * kind, 0x14 bytes, count 0x00570070[kind]) gives two thresholds compared with
     * +0x56 + 6 * +0x57: 1 when past the first (> 0) and +0x59 == 0, 2 when past the second
     * (> 0) and +0x5A == 0, else 0 */
    if (S32(st, CLIMATE) == 5) {
        return 0;
    }
    uint32_t b = B(id);
    uint32_t kind = U8(st, b + 0x54);
    if (U8(st, b + 0x55) == 1 && U8(st, B(id - PART(id)) + 0x55) == 1) {
        return 0;
    }
    int32_t n = S32(st, 0x00570070u + 4u * kind);
    for (int32_t i = 0; i < n; i++) {
        uint32_t e = 0x005F3878u + 0x3C0u * kind + 0x14u * (uint32_t) i;
        if (PART(id) != S32(st, e)) {
            continue;
        }
        int32_t level = U8(st, b + 0x56) + U8(st, b + 0x57) * 6;
        int32_t first = S32(st, e + 4);
        if (first > 0 && first <= level && U8(st, b + 0x59) == 0) {
            return 1;
        }
        int32_t second = S32(st, e + 8);
        if (second <= 0 || second > level || U8(st, b + 0x5A) != 0) {
            return 0;
        }
        return 2;
    }
    return 0;
}

int32_t pyramid_worker_busy(game_state *st, int32_t id)
{
    /* @bin 0x004EEA10 (id) the figure in the part's +0x64 still works there (by its type +0x0A
     * and action +0x38, or +0x10 while the action is 6 or 7); a gone or finished worker clears
     * +0x64 (al) */
    uint32_t slot = B(id) + 0x64;
    int32_t f = S16(st, slot);
    if (f == 0) {
        return 0;
    }
    uint32_t a = F(f);
    if (U8(st, a + 0x0E) == 0) {
        S16(st, slot) = 0;
        return 0;
    }
    uint32_t act = U8(st, a + 0x38);
    if (act == 6 || act == 7) {
        act = U8(st, a + 0x10);
    }
    switch (U8(st, a + 0x0A)) {
    case 0x4F:
        return act == 9 || act == 0xC || act == 0xA;
    case 0x50:
        return act == 0xA || act == 0xD || act == 0xF || act == 0xC || act == 0xE || act == 0xB;
    case 0x51:
        if (pyramid_part_ready(st, id) == 0) {
            if (act == 9 || act == 0xD || act == 0xF || act == 0xB || act == 0xE || act == 0x10) {
                return 1;
            }
            if (act != 0xA) {
                return 0;
            }
            if (S16(st, 0x00B4E6FCu + 2u * U32(st, a + 0x28)) == id) {
                return 1;
            }
        }
        S16(st, slot) = 0;
        return 0;
    case 0x55:
        return act == 0xF || act == 0x10 || act == 0x11 || act == 0x1E || act == 0x12 || act == 0x13 ||
               act == 0x14 || act == 0x15 || act == 0x1D || act == 0x16;
    default:
        return 0;
    }
}

int32_t pyramid_send_figure(game_state *st, int32_t f, int32_t id)
{
    /* @bin 0x004F3000 (figure, part) to the part's main building + the kind's first offset
     * (0x005F5498 + 0x80 * kind when climate 5, else 0x005F3878 + 0x3C0 * kind); returns its tile */
    int32_t main_id = id - PART(id);
    uint32_t kind = U8(st, B(main_id) + 0x54);
    int32_t first = S32(st, CLIMATE) == 5 ? S32(st, 0x005F5498u + 0x80u * kind) : S32(st, 0x005F3878u + 0x3C0u * kind);
    return figure_goes_to(st, f, main_id + first);
}

int32_t pyramid_take_worker(game_state *st, int32_t x, int32_t y, int32_t f)
{
    /* @bin 0x004F01E0 each used pyramid (type 0xB7, buildings 1..3999, skipping its parts: count
     * 0x0056F7D8 / 0x0056F7EC by kind): the first of its parts (list 0x005F3878) that is ready
     * (0x004EF100); if that part has no worker still busy (0x004EEA10), the figure becomes its
     * worker (+0x64) and goes there */
    (void) x;
    (void) y;
    if (S32(st, CLIMATE) == 5) {
        return 0;
    }
    for (int32_t id = 1; id < 4000; id++) {
        if (USED(id) != 1 || TYPE(id) != 0xB7) {
            continue;
        }
        uint32_t kind = U8(st, B(id) + 0x54);
        for (int32_t i = 0; i < S32(st, 0x00570070u + 4u * kind); i++) {
            int32_t part = S32(st, 0x005F3878u + 0x14u * (U8(st, B(id) + 0x54) * 0x30u + (uint32_t) i)) + id;
            if (pyramid_part_ready(st, part) == 0) {
                continue;
            }
            if (S16(st, B(part) + 0x64) == 0 || !(uint8_t) pyramid_worker_busy(st, part)) {
                S16(st, B(part) + 0x64) = (int16_t) f;
                return pyramid_send_figure(st, f, part);
            }
            break;
        }
        uint32_t k = U8(st, B(id) + 0x54);
        int32_t parts = S32(st, CLIMATE) == 5 ? S32(st, 0x0056F7ECu + 4u * k) : S32(st, 0x0056F7D8u + 4u * k);
        id += parts - 1;
    }
    return 0;
}

int32_t monument_0x00420350(game_state *st, int32_t x, int32_t y, int32_t f)
{
    /* @bin 0x00420350 an entry of the table 0x005C2748 (22 x 16 bytes: +0 city value, +4 figure
     * type, +8 object with +0x12 == 0 and +0x0C > 0) for the current city's +0x433A and the
     * figure's type: the figure goes to the first used building of type 0xD2 */
    (void) x;
    (void) y;
    int32_t city = S32(st, CURRENT_PLAYER_ADDR);
    uint32_t e;
    for (e = 0x005C2750u; e < 0x005C28B0u; e += 0x10) {
        uint32_t obj = U32(st, e);
        if (P16(st, obj + 0x12) == 0 &&
            S32(st, e - 8) == (int16_t) S16(st, CITY_ADDR(city) + 0x433A) &&
            S32(st, e - 4) == (int32_t) U8(st, F(f) + 0x0A) && (int32_t) P32(st, obj + 0x0C) > 0) {
            break;
        }
    }
    if (e >= 0x005C28B0u) {
        return 0;
    }
    for (int32_t id = 1; id < 4000; id++) {
        if (USED(id) == 1 && TYPE(id) == 0xD2) {
            figure_goes_to(st, f, id);
            return id;
        }
    }
    return 0;
}

int32_t monument_stage_count(game_state *st, int32_t id)
{
    /* @bin 0x004E5A30 (id) stages done: (+0x55 - 0x21) * n / 66 + 1, at most n (n = 10 for type
     * 0xCF, else 18) */
    int32_t n = TYPE(id) != 0xCF ? 18 : 10;
    int32_t k = (U8(st, B(id) + 0x55) - 0x21) * n / 0x42 + 1;
    return k < n ? k : n;
}

int32_t monument_0x004E5C00(game_state *st, int32_t x, int32_t y, int32_t f)
{
    /* @bin 0x004E5C00 the city's monument in progress (0x004E59C0): an entry of its list (10 of
     * 0x00586B08 for type 0xCF, else 18 of 0x0058FE48; 16 bytes: +0 stage, +4 figure type, +8
     * object with +0x12 == 0 and +0x0C > 0) for its stage count and the figure's type: the figure
     * goes to the monument */
    (void) x;
    (void) y;
    int32_t id = GAME_CALL0(st, 0x004E59C0u);
    if (id == 0) {
        return 0;
    }
    for (int32_t i = 0; i < (TYPE(id) != 0xCF ? 18 : 10); i++) {
        uint32_t e = (TYPE(id) == 0xCF ? 0x00586B08u : 0x0058FE48u) + 0x10u * (uint32_t) i;
        uint32_t obj = U32(st, e + 8);
        if (P16(st, obj + 0x12) != 0) {
            continue;
        }
        if (S32(st, e) != monument_stage_count(st, id)) {
            continue;
        }
        if (S32(st, e + 4) != (int32_t) U8(st, F(f) + 0x0A) || (int32_t) P32(st, U32(st, e + 8) + 0x0C) <= 0) {
            continue;
        }
        figure_goes_to(st, f, id);
        return id;
    }
    return 0;
}

/* @bin 0x004178E0 (type 0xE3), 0x004E6E10 (0xE4), 0x0040F070 (0xE1): thiscall on the
 * monument's handler object: the first used building of the type whose method +0x2C of the
 * object's vtable (thiscall (id, figure, 1), al) takes the figure */
static int32_t take_by_method(game_state *st, uint32_t obj, int16_t type, int32_t f)
{
    for (int32_t id = 1; id < 4000; id++) {
        if (USED(id) == 1 && TYPE(id) == type) {
            uint32_t method = U32(st, U32(st, obj) + 0x2C);
            if ((uint8_t) GAME_CALLT(st, obj, method, id, f, 1)) {
                return id;
            }
        }
    }
    return 0;
}

/* @bin 0x00542A80 (0xD5, 0x005421D0), 0x004E0C00 (0xDE, 0x004E05F0): the first used building of
 * the type whose handler (id, figure, 1) (al) takes the figure */
static int32_t take_by_call(game_state *st, uint32_t fn, int16_t type, int32_t f)
{
    for (int32_t id = 1; id < 4000; id++) {
        if (USED(id) == 1 && TYPE(id) == type && (uint8_t) GAME_CALL(st, fn, id, f, 1)) {
            return id;
        }
    }
    return 0;
}

static int32_t take_mastaba(game_state *st, int32_t f)
{
    /* @bin 0x004E0C00 the first used mastaba part (type 0xDE) that takes the figure */
    for (int32_t id = 1; id < 4000; id++) {
        if (USED(id) == 1 && TYPE(id) == 0xDE && (uint8_t) mastaba_take(st, id, f, 1)) {
            return id;
        }
    }
    return 0;
}

int32_t monument_0x00401ED0(game_state *st, uint32_t obj, int32_t x, int32_t y, int32_t f, int32_t extra)
{
    /* @bin 0x00401ED0 (thiscall) type 0xE6: thiscall 0x00401D40 (id, figure, 1, extra) */
    (void) x;
    (void) y;
    for (int32_t id = 1; id < 4000; id++) {
        if (USED(id) == 1 && TYPE(id) == 0xE6 && (uint8_t) GAME_CALLT(st, obj, 0x00401D40u, id, f, 1, extra)) {
            return id;
        }
    }
    return 0;
}

static int32_t act_of(game_state *st, int32_t f)
{
    /* the figure's action +0x38, or +0x10 while it is 6 or 7 (@bin 0x004E057B, 0x004EEA4B) */
    uint32_t act = U8(st, F(f) + 0x38);
    return act == 6 || act == 7 ? U8(st, F(f) + 0x10) : act;
}

int32_t mastaba_worker_busy(game_state *st, int32_t id)
{
    /* @bin 0x004E0530 (id) type 0xDE: its figure +0x5C still works there (by type and action);
     * a gone or finished one clears +0x5C (al) */
    if (TYPE(id) != 0xDE) {
        return 0;
    }
    int32_t f = S32(st, B(id) + 0x5C);
    if (f == 0) {
        return 0;
    }
    if (U8(st, F(f) + 0x0E) == 0) {
        S32(st, B(id) + 0x5C) = 0;
        return 0;
    }
    int32_t act = act_of(st, f);
    switch (U8(st, F(f) + 0x0A)) {
    case 0x4F:
        if (act == 9 || act == 0xA) {
            return 1;
        }
        break;
    case 0x51:
        if (act == 9 || act == 0xB || act == 0xA) {
            return 1;
        }
        break;
    case 0x55:
        if (act == 0xF || act == 0x10 || act == 0x1A || act == 0x1B || act == 0x1C) {
            return 1;
        }
        break;
    default:
        break;
    }
    S32(st, B(id) + 0x5C) = 0;
    return 0;
}

/* the figure works at the building: action, +0x5C, destination (@bin 0x004E0A84, 0x004E0AF9,
 * 0x004E0B9A) */
static void mastaba_assign(game_state *st, int32_t id, int32_t f, uint8_t action)
{
    U8(st, F(f) + 0x38) = action;
    S32(st, B(id) + 0x5C) = f;
    S16(st, F(f) + 0x24) = S16(st, B(id) + 0x26);
    S16(st, F(f) + 0x26) = S16(st, B(id) + 0x28);
    S32(st, F(f) + 0x28) = S32(st, B(id) + 0x0C);
    S16(st, F(f) + 0x44) = 0;
}

int32_t mastaba_take(game_state *st, int32_t id, int32_t f, int32_t commit)
{
    /* @bin 0x004E05F0 (id, figure, commit) a mastaba part (type 0xDE) takes a worker (al). The
     * flags +0x64 of the main building: bit 0, bit 1 (the 4 corner parts +0x58 := 0xFFFF and
     * redrawn 0x004DFBB0, once, from part 0), bit 2, 0xF0 (all corners). The part's entry of
     * 0x005E2AE4 (13 dwords per part number): low byte the kind (0..3, 0xB, 0xD: none), 0xF00 the
     * corner the part needs, 0xF000 the stone corner. Figure 0x4F (bricks) / 0x51 (stones) /
     * 0x55 (builder) by its type +0x0A. */
    uint32_t fa = F(f);
    uint8_t type = U8(st, fa + 0x0A);
    int is_u = type == 0x55, is_o = type == 0x4F, is_q = type == 0x51;
    int16_t part = PART(id);
    uint32_t flags = U32(st, B(id - part) + 0x64);
    int bit0 = flags & 1, bit1 = (flags >> 1) & 1, bit2 = (flags >> 2) & 1, all = (flags & 0xF0) == 0xF0;
    if (part == 0 && bit1) {
        for (int32_t k = 0; k < 4; k++) {
            int32_t b = k - PART(id) + id;
            if (S32(st, B(b) + 0x58) < 0xFFFF) {
                S32(st, B(b) + 0x58) = 0xFFFF;
                GAME_CALL(st, 0x004DFBB0u, b);
            }
        }
    }
    if (is_o && (all || !bit0)) {
        return 0;
    }
    uint32_t entry = U32(st, 0x005E2AE4u + 0x34u * (uint32_t) (int32_t) PART(id));
    uint32_t kind = entry & 0xFF, corner = entry & 0xF00;
    int builder = 0; /* the path at 0x004E0B49 */
    if (kind <= 0xD && (kind <= 3 || kind == 0xB || kind == 0xD)) {
        return 0; /* @bin 0x004E0BF0 the kinds without work */
    }
    if (corner == 0 || S32(st, B(id) + 0x58) < 1 || bit1) {
        builder = 1;
    }
    if (!builder) {
        uint32_t bit;
        int32_t add;
        switch (corner) {
        case 0x100: bit = 0x10; add = 0; break;
        case 0x200: bit = 0x20; add = 1; break;
        case 0x400: bit = 0x40; add = 2; break;
        case 0x800: bit = 0x80; add = 3; break;
        default: return 0;
        }
        if (flags & bit) {
            if (is_o) {
                return 0;
            }
        } else if (is_q && bit0) {
            return 0;
        }
        int32_t target = id - PART(id) + add;
        if (is_u) {
            /* @bin 0x004E0868 the stone mason (+0x5C, type 0x51 at action 0xA) gets the builder */
            if (!(uint8_t) mastaba_worker_busy(st, id)) {
                return 0;
            }
            int32_t q = S32(st, B(id) + 0x5C);
            uint32_t qa = F(q);
            if (U8(st, qa + 0x0A) != 0x51 || U8(st, qa + 0x38) != 0xA) {
                return 0;
            }
            int32_t other = S16(st, qa + 0x68);
            int keep = 0;
            if (other != 0) {
                uint32_t oa = F(other);
                uint8_t act = U8(st, oa + 0x38);
                if (U8(st, oa + 0x0E) != 0 && U8(st, oa + 0x0A) == 0x55 &&
                    (act == 6 || act == 7 || act == 0xF || act == 0x10 || act == 0x1A) && S16(st, oa + 0x68) == q) {
                    keep = 1; /* @bin 0x004E0B45 the builder path */
                } else {
                    S16(st, qa + 0x68) = 0;
                }
            }
            if (!keep) {
                if (!(uint8_t) commit) {
                    return 1;
                }
                int32_t src = building_find_supplier(st, id, 0x1E, 1);
                if (src == 0) {
                    U8(st, fa + 0x0E) = 2;
                    return 1;
                }
                int16_t sx = S16(st, 0x00D3B4E4u), sy = S16(st, 0x00D3B4E0u);
                U8(st, fa + 0x7A) = 0x1E;
                S16(st, fa + 0x74) = 0;
                S16(st, fa + 0x5E) = (int16_t) src;
                S16(st, fa + 0x68) = (int16_t) q;
                U8(st, fa + 0x38) = 0xF;
                S16(st, fa + 0x36) = 0;
                S16(st, fa + 0x24) = sx;
                S16(st, fa + 0x26) = sy;
                S16(st, qa + 0x68) = (int16_t) f;
                return 1;
            }
            builder = 1;
            bit1 = (flags >> 1) & 1;
        } else if (is_q) {
            /* @bin 0x004E09A0 the target corner's stone state +0x58 */
            uint32_t w = U32(st, B(target) + 0x58), lo, hi;
            switch (entry & 0xF000) {
            case 0x1000: lo = 3; hi = 0x300; break;
            case 0x2000: lo = 0xC; hi = 0xC00; break;
            case 0x4000: lo = 0x30; hi = 0x3000; break;
            case 0x8000: lo = 0xC0; hi = 0xC000; break;
            default: return 0;
            }
            if ((w & lo) == lo) {
                if ((w & hi) == hi || !bit0) {
                    return 0;
                }
            }
            if ((uint8_t) mastaba_worker_busy(st, id)) {
                return 0;
            }
            if ((uint8_t) commit) {
                mastaba_assign(st, id, f, 9);
            }
            return 1;
        } else if (is_o) {
            if ((uint8_t) mastaba_worker_busy(st, id)) {
                return 0;
            }
            if ((uint8_t) commit) {
                mastaba_assign(st, id, f, 9);
            }
            return 1;
        } else {
            return 0;
        }
    }
    /* @bin 0x004E0B49 the builder (0x55) works the part */
    if (!is_u) {
        return 0;
    }
    int32_t w = S32(st, B(id) + 0x58);
    if (w == 2) {
        return 0;
    }
    if (w == 1 && (!bit2 || !bit1)) {
        return 0;
    }
    if ((uint8_t) mastaba_worker_busy(st, id)) {
        return 0;
    }
    if ((uint8_t) commit) {
        mastaba_assign(st, id, f, 0x1B);
    }
    return 1;
}

/* @bin 0x00417590 (E3, builder actions 0xF, 0x10, 0x22), 0x004E6B20 (E4: 0xF, 0x10, 0x23),
 * 0x0040EC20 (E1, the main building from stage 3: 0xF, 0x10, 0x1F, 0x20, 0x21): the vtable
 * method +0x24 (thiscall, `this` unused) (id): the building's figure +0x5C still works there
 * (0x4F at action 9 / 0xA, 0x51 at 9 / 0xB / 0xA, 0x55 at the builder actions; the action +0x38,
 * or +0x10 while it is 6 or 7). A gone figure, or one of another type, clears +0x5C. (al) */
static int32_t part_busy(game_state *st, int32_t id, int kind)
{
    if (kind == 0xE1 && S32(st, B(id) + 0x58) >= 3) {
        id -= PART(id);
    }
    int32_t f = S32(st, B(id) + 0x5C);
    if (f == 0) {
        return 0;
    }
    if (U8(st, F(f) + 0x0E) == 0) {
        S32(st, B(id) + 0x5C) = 0;
        return 0;
    }
    int32_t act = act_of(st, f);
    switch (U8(st, F(f) + 0x0A)) {
    case 0x4F: return act == 9 || act == 0xA;
    case 0x51: return act == 9 || act == 0xB || act == 0xA;
    case 0x55:
        if (act == 0xF || act == 0x10) {
            return 1;
        }
        if (kind == 0xE1) {
            return act == 0x1F || act == 0x20 || act == 0x21;
        }
        return act == (kind == 0xE4 ? 0x23 : 0x22);
    default:
        S32(st, B(id) + 0x5C) = 0;
        return 0;
    }
}

/* @bin 0x00420F50 (table 0x005AD210, E3), 0x00421230 (0x005B0058, E4), 0x00420910 (0x0059EDE8,
 * E1, the stage of the main building): (id, figure) an entry (16 bytes: +0 stage, +4 figure type,
 * +8 object with +0x12 == 0 and +0x0C > 0) for the building's stage +0x58 and the figure's type:
 * returns id, else 0 */
static int32_t stage_job(game_state *st, int32_t id, int32_t f, uint32_t first, uint32_t end, int main_stage)
{
    int32_t stage = S32(st, B(main_stage ? id - PART(id) : id) + 0x58);
    for (uint32_t e = first; e < end; e += 0x10) {
        uint32_t obj = U32(st, e);
        if (P16(st, obj + 0x12) == 0 && S32(st, e - 8) == stage && S32(st, e - 4) == (int32_t) U8(st, F(f) + 0x0A) &&
            (int32_t) P32(st, obj + 0x0C) > 0) {
            return id;
        }
    }
    return 0;
}

/* the vtable method +off of a monument handler object (thiscall) */
static uint32_t method(game_state *st, uint32_t obj, uint32_t off)
{
    return U32(st, U32(st, obj) + off);
}

/* @bin 0x0041774D, 0x004E6CB1, 0x0040EEBF the stone mason q (type 0x51 at action 0xA) already has
 * a builder (figure +0x68, type 0x55 at action 6, 7, 0xF, 0x10 or `act`, pointing back at q);
 * else q's +0x68 is cleared. Returns 1 when it has one. */
static int mason_has_builder(game_state *st, int32_t q, uint8_t act)
{
    uint32_t qa = F(q);
    int32_t other = S16(st, qa + 0x68);
    if (other == 0) {
        return 0;
    }
    uint32_t oa = F(other);
    uint8_t a = U8(st, oa + 0x38);
    if (U8(st, oa + 0x0E) != 0 && U8(st, oa + 0x0A) == 0x55 && (a == 6 || a == 7 || a == 0xF || a == 0x10 || a == act) &&
        S16(st, oa + 0x68) == q) {
        return 1;
    }
    S16(st, qa + 0x68) = 0;
    return 0;
}

/* @bin 0x004177CA, 0x004E6D10, 0x0040EF42 the builder fetches `good` from the supplier found by
 * 0x0045BB50 (its tile in 0x00D3B4E4 / 0x00D3B4E0), then joins the mason q; no supplier: the
 * builder is done (+0x0E := 2) */
static void builder_fetch(game_state *st, int32_t id, int32_t f, int32_t q, uint8_t good)
{
    uint32_t fa = F(f);
    int32_t src = building_find_supplier(st, id, good, 1);
    if (src == 0) {
        U8(st, fa + 0x0E) = 2;
        return;
    }
    U8(st, fa + 0x7A) = good;
    S16(st, fa + 0x74) = 0;
    S16(st, fa + 0x5E) = (int16_t) src;
    S16(st, fa + 0x68) = (int16_t) q;
    U8(st, fa + 0x38) = 0xF;
    S16(st, fa + 0x36) = 0;
    S16(st, fa + 0x24) = S16(st, 0x00D3B4E4u);
    S16(st, fa + 0x26) = S16(st, 0x00D3B4E0u);
    S16(st, F(q) + 0x68) = (int16_t) f;
}

/* @bin 0x004176A0 (type 0xE3, finder 0x00420F50, act 0x22) and 0x004E6C10 (type 0xE4, finder
 * 0x00421230, act 0x23): thiscall (id, figure, commit) on the handler object: the part's slot
 * (object +0x58 + 0x34 * part) must be 0. A builder (0x55) joins the part's mason when the
 * object's method +0x24 (id) says so; another figure goes to the building the finder gives
 * (id, figure) when method +0x24 says no. (al) */
static int32_t take_part(game_state *st, uint32_t obj, int32_t id, int32_t f, int32_t commit, int e4)
{
    int32_t main_id = id - PART(id);
    if (S32(st, obj + 0x58 + 0x34u * (uint32_t) (int32_t) PART(id)) != 0) {
        return 0;
    }
    uint32_t fa = F(f);
    if (U8(st, fa + 0x0A) == 0x55) {
        if (!(uint8_t) GAME_CALLT(st, obj, method(st, obj, 0x24), id)) {
            return 0;
        }
        int32_t q = S32(st, B(id) + 0x5C);
        if (U8(st, F(q) + 0x0A) != 0x51 || U8(st, F(q) + 0x38) != 0xA) {
            return 0;
        }
        if (mason_has_builder(st, q, e4 ? 0x23 : 0x22)) {
            return 0;
        }
        if (!(uint8_t) commit) {
            return 1;
        }
        uint8_t good = e4 ? 0x23 : (S32(st, B(main_id) + 0x58) != 0x12 ? 0x23 : 0x1A);
        builder_fetch(st, id, f, q, good);
        return 1;
    }
    if ((uint8_t) GAME_CALLT(st, obj, method(st, obj, 0x24), id)) {
        return 0;
    }
    int32_t b = GAME_CALL(st, e4 ? 0x00421230u : 0x00420F50u, id, f);
    if (b == 0) {
        return 0;
    }
    if (!(uint8_t) commit) {
        return 1;
    }
    if (U8(st, fa + 0x0A) == 0x4F || U8(st, fa + 0x0A) == 0x51) {
        U8(st, fa + 0x38) = 9;
    }
    S32(st, B(b) + 0x5C) = f;
    S16(st, fa + 0x24) = S16(st, B(b) + 0x26);
    S16(st, fa + 0x26) = S16(st, B(b) + 0x28);
    S32(st, fa + 0x28) = S32(st, B(b) + 0x0C);
    S16(st, fa + 0x44) = 0;
    S16(st, fa + 0x36) = 0;
    return 1;
}

int32_t obelisk_take(game_state *st, uint32_t obj, int32_t id, int32_t f, int32_t commit)
{
    /* @bin 0x0040ED70 thiscall (id, figure, commit) type 0xE1: the main building once the part's
     * stage +0x58 >= 3; the slot (object +0x58 + 0x34 * part) in 0..0x18. A builder (0x55) starts
     * a stage below 2 alone (+0x5C, action 0x20), or from stage 3 joins the mason (good 0x23, or
     * 0x1D for stages 0x10..0x13) when method +0x24 says so; another figure, at stage >= 3 when
     * method +0x24 says no and 0x00420910 (id, figure) finds work, goes to the main building
     * (+0x5E := main + 0xF). (al) */
    int32_t main_id = id;
    if (S32(st, B(id) + 0x58) >= 3) {
        main_id = id - PART(id);
    }
    int32_t slot = S32(st, obj + 0x58 + 0x34u * (uint32_t) (int32_t) PART(main_id));
    if (slot < 0 || slot > 0x18) {
        return 0;
    }
    uint32_t fa = F(f);
    if (U8(st, fa + 0x0A) == 0x55) {
        int32_t w = S32(st, B(main_id) + 0x58);
        if (w < 2) {
            if (S32(st, B(main_id) + 0x5C) != 0) {
                return 0;
            }
            if ((uint8_t) commit) {
                S32(st, B(main_id) + 0x5C) = f;
                U8(st, fa + 0x38) = 0x20;
                S16(st, fa + 0x5E) = (int16_t) main_id;
                S16(st, fa + 0x24) = S16(st, B(id) + 0x26);
                S16(st, fa + 0x26) = S16(st, B(id) + 0x28);
                S32(st, fa + 0x28) = S32(st, B(id) + 0x0C);
                S16(st, fa + 0x36) = 0;
                S16(st, fa + 0x44) = 0;
            }
            return 1;
        }
        if (w < 3) {
            return 0;
        }
        if (!(uint8_t) GAME_CALLT(st, obj, method(st, obj, 0x24), id)) {
            return 0;
        }
        int32_t q = S32(st, B(main_id) + 0x5C);
        if (U8(st, F(q) + 0x0A) != 0x51 || U8(st, F(q) + 0x38) != 0xA) {
            return 0;
        }
        if (mason_has_builder(st, q, 0x1F)) {
            return 0;
        }
        if (!(uint8_t) commit) {
            return 1;
        }
        int32_t w2 = S32(st, B(main_id) + 0x58);
        uint8_t good = w2 < 0x10 ? 0x23 : (w2 >= 0x14 ? 0x23 : 0x1D);
        builder_fetch(st, id, f, q, good);
        return 1;
    }
    if ((uint8_t) GAME_CALLT(st, obj, method(st, obj, 0x24), id)) {
        return 0;
    }
    if (S32(st, B(main_id) + 0x58) < 3) {
        return 0;
    }
    if (GAME_CALL(st, 0x00420910u, id, f) == 0) {
        return 0;
    }
    if ((uint8_t) commit) {
        if (U8(st, fa + 0x0A) == 0x4F || U8(st, fa + 0x0A) == 0x51) {
            U8(st, fa + 0x38) = 9;
        }
        S32(st, B(main_id) + 0x5C) = f;
        S16(st, fa + 0x5E) = (int16_t) (main_id + 0xF);
        S16(st, fa + 0x24) = S16(st, B(main_id) + 0x26);
        S16(st, fa + 0x26) = S16(st, B(main_id) + 0x28);
        S32(st, fa + 0x28) = S32(st, B(main_id) + 0x0C);
        S16(st, fa + 0x44) = 0;
        S16(st, fa + 0x36) = 0;
    }
    return 1;
}

int32_t monument_take_worker(game_state *st, int32_t x, int32_t y, int32_t f)
{
    /* @bin 0x004BC140 the first non-zero handler */
    int32_t r;
    if ((r = pyramid_take_worker(st, x, y, f)) != 0) return r;
    if ((r = monument_0x00420350(st, x, y, f)) != 0) return r;
    if ((r = monument_0x004E5C00(st, x, y, f)) != 0) return r;
    if ((r = take_by_method(st, 0x006FD368u, 0xE3, f)) != 0) return r;  /* 0x004178E0 */
    if ((r = take_by_method(st, 0x006DE548u, 0xE4, f)) != 0) return r;  /* 0x004E6E10 */
    if ((r = take_by_call(st, 0x005421D0u, 0xD5, f)) != 0) return r;    /* 0x00542A80 */
    if ((r = take_by_method(st, 0x006FD5D8u, 0xE1, f)) != 0) return r;  /* 0x0040F070 */
    if ((r = monument_0x00401ED0(st, 0x006DE650u, x, y, f, 0)) != 0) return r; /* 0x00401F30 */
    return take_mastaba(st, f);                                          /* 0x004E0C00 */
}

void pyramids_update(game_state *st)
{
    /* @bin 0x004F3890 every used pyramid (type 0xB7, skipping its parts): stage +0x55 > 1:
     * 0x004F3850 (id), 0x004EE5D0 (id) when that says no, 0x00517370. Else 0x004EF450 (id),
     * 0x004F25C0, then with a part in progress (0x00EAAF58 > 0, part 0x00EAAC44): 0x004F31F0,
     * 0x004EF200 when the part is ready (0x004EF100), 0x004EDF50 (part, orientation / 2, kind);
     * none: the kind's placement preview 0x004ED360 (3) and, when it found a tile 0x00EAAE90,
     * the building there (0x0046A400, 0x00470990), canals redrawn and two routing grids rebuilt */
    int32_t half = S32(st, 0x00BCA2E0u) / 2;
    for (int32_t id = 1; id < 4000; id++) {
        if (USED(id) != 1 || TYPE(id) != 0xB7) {
            continue;
        }
        if (U8(st, B(id) + 0x55) > 1) {
            if (!(uint8_t) GAME_CALL(st, 0x004F3850u, id)) {
                GAME_CALL(st, 0x004EE5D0u, id);
            }
            GAME_CALL0(st, 0x00517370u);
        } else {
            GAME_CALL(st, 0x004EF450u, id);
            GAME_CALL0(st, 0x004F25C0u);
            if (S32(st, 0x00EAAF58u) > 0) {
                int32_t part = S32(st, 0x00EAAC44u);
                GAME_CALL(st, 0x004F31F0u, part);
                if (pyramid_part_ready(st, part) != 0) {
                    GAME_CALL(st, 0x004EF200u, part);
                }
                GAME_CALL(st, 0x004EDF50u, part, half, (int32_t) U8(st, B(part - PART(part)) + 0x54));
            } else {
                uint32_t kind = U8(st, B(id) + 0x54);
                if ((uint8_t) GAME_CALL(st, 0x004ECEA0u, (int32_t) kind)) {
                    int32_t r = S32(st, 0x005F5D08u + 4u * kind) + S32(st, B(id) + 0x0C) - S32(st, 0x00BCA2D8u);
                    GAME_CALL(st, 0x004ED360u, r % 228, r / 228, 3, 0);
                    int32_t t = S32(st, 0x00EAAE90u);
                    if (t != 0) {
                        int32_t tx = (t - S32(st, 0x00BCA2D8u)) % 228, ty = (t - S32(st, 0x00BCA2D8u)) / 228;
                        int32_t b = GAME_CALL(st, 0x0046A400u, S32(st, 0x00EAAC28u), S32(st, 0x00EAAE8Cu), tx, ty, 0, 0);
                        GAME_CALL(st, 0x00470990u, S32(st, 0x00EAAC28u), S32(st, 0x00EAAE8Cu), b, tx, ty, 2, 1, 0);
                        GAME_CALL(st, 0x0047BAE0u, 0, 0, S32(st, 0x00BCA2D0u) - 1, S32(st, 0x00BCA2D4u) - 1, 0);
                        GAME_CALL0(st, 0x00517370u);
                        GAME_CALL0(st, 0x00517660u);
                    }
                }
            }
        }
        uint32_t k = U8(st, B(id) + 0x54);
        id += (S32(st, CLIMATE) == 5 ? S32(st, 0x0056F7ECu + 4u * k) : S32(st, 0x0056F7D8u + 4u * k)) - 1;
    }
}

int32_t pyramid_assign(game_state *st, int32_t x, int32_t y, int32_t f, int32_t mode)
{
    /* @bin 0x004EFE30 (x, y, figure, mode) each used pyramid (type 0xB7, skipping its parts):
     * with a worker count +0xC8, its parts in progress (0x004EF450: list 0x00EAAC44, count
     * 0x00EAAF58): the first part not ready (0x004EF100) without a busy worker (+0x64,
     * 0x004EEA10) takes the figure when the climate (0x005E4EF4) is not 2, or when climate 2 and
     * 0x004EFD90 (part) == (mode != 0): the figure heads for the pyramid's first part (climate 5:
     * x, y + 2 of the 0x005F5498 entry; else the 0x005F3878 entry); returns the part. Mode 1 and
     * stage +0x55 <= 1: the kind's placement preview (0x004ED360, 3) gives a tile 0x00EAAE90; if
     * no other figure of the same type is headed there (+0x28) for this pyramid (+0x5E), the
     * figure goes there; returns the pyramid. */
    (void) x;
    (void) y;
    for (int32_t id = 1; id < 4000; id++) {
        if (USED(id) != 1 || TYPE(id) != 0xB7) {
            continue;
        }
        if (S16(st, B(id) + 0xC8) != 0) {
            GAME_CALL(st, 0x004EF450u, id);
            for (int32_t i = 0; i < S32(st, 0x00EAAF58u); i++) {
                int32_t part = S32(st, 0x00EAAC44u + 4u * (uint32_t) i);
                if (pyramid_part_ready(st, part) != 0) {
                    continue;
                }
                uint32_t slot = B(part) + 0x64;
                if (S16(st, slot) != 0) {
                    if (!(uint8_t) pyramid_worker_busy(st, part)) {
                        S16(st, slot) = 0;
                    }
                    if (S16(st, slot) != 0) {
                        continue;
                    }
                }
                uint8_t unfinished = (uint8_t) GAME_CALL(st, 0x004EFD90u, part);
                int32_t climate = S32(st, CLIMATE);
                if (climate < 1 || climate > 5) {
                    continue;
                }
                if (climate == 2 && (mode == 0 ? !unfinished : unfinished)) {
                    continue;
                }
                S16(st, slot) = (int16_t) f;
                uint32_t fa = F(f);
                int32_t b;
                if (climate == 5) {
                    b = id + S32(st, 0x005F5498u + 0x80u * U8(st, B(id) + 0x54));
                    S16(st, fa + 0x24) = S16(st, B(b) + 6);
                    S16(st, fa + 0x26) = (int16_t) (S16(st, B(b) + 8) + 2);
                } else {
                    b = id + S32(st, 0x005F3878u + 0x3C0u * U8(st, B(id) + 0x54));
                    S16(st, fa + 0x24) = S16(st, B(b) + 0x26);
                    S16(st, fa + 0x26) = S16(st, B(b) + 0x28);
                }
                S32(st, fa + 0x28) = S32(st, B(b) + 0x0C);
                S16(st, fa + 0x44) = 0;
                return part;
            }
        }
        if (mode == 1 && U8(st, B(id) + 0x55) <= 1) {
            uint32_t kind = U8(st, B(id) + 0x54);
            if ((uint8_t) GAME_CALL(st, 0x004ECEA0u, (int32_t) kind)) {
                int32_t r = S32(st, 0x005F5D08u + 4u * kind) + S32(st, B(id) + 0x0C) - S32(st, 0x00BCA2D8u);
                GAME_CALL(st, 0x004ED360u, r % 228, r / 228, 3, 0);
                int32_t t = S32(st, 0x00EAAE90u);
                if (t != 0) {
                    int taken = 0;
                    for (int32_t g = 1; g < 2000; g++) {
                        uint32_t ga = F(g);
                        if (g != f && U8(st, ga + 0x0E) != 0 && U8(st, ga + 0x0A) == U8(st, F(f) + 0x0A) &&
                            S32(st, ga + 0x28) == t && S16(st, ga + 0x5E) == id) {
                            taken = 1;
                            break;
                        }
                    }
                    if (!taken) {
                        int32_t d = t - S32(st, 0x00BCA2D8u);
                        uint32_t fa = F(f);
                        S16(st, fa + 0x24) = (int16_t) (d % 228);
                        S16(st, fa + 0x26) = (int16_t) (d / 228);
                        S32(st, fa + 0x28) = t;
                        S16(st, fa + 0x44) = 0;
                        S16(st, fa + 0x5E) = (int16_t) id;
                        return id;
                    }
                }
            }
        }
        uint32_t k = U8(st, B(id) + 0x54);
        id += (S32(st, CLIMATE) == 5 ? S32(st, 0x0056F7ECu + 4u * k) : S32(st, 0x0056F7D8u + 4u * k)) - 1;
    }
    return 0;
}

void pyramid_parts_sort(game_state *st)
{
    /* @bin 0x004F25C0 with two parts or more in progress: the level of the first (+0x56 + 6 *
     * +0x57) picks the first entry of the kind's level list (climate 5: 16 of 0x005F5498 + 0x80 *
     * kind, else 60 of 0x005F4B38 + 0x1E0 * kind; 8 bytes: part offset, level) at or above it;
     * its building's corner (+6, +8) goes to 0x00EAAC40 / 0x00EAAC3C, and the list 0x00EAAC44 is
     * sorted (CRT qsort with the comparator 0x004F24B0) */
    int32_t n = S32(st, 0x00EAAF58u);
    if (n < 2) {
        return;
    }
    int32_t part = S32(st, 0x00EAAC44u), main_id = part - PART(part), b = part;
    int32_t level = U8(st, B(part) + 0x56) + U8(st, B(part) + 0x57) * 6;
    uint32_t kind = U8(st, B(part) + 0x54), e;
    int32_t count;
    if (S32(st, CLIMATE) == 5) {
        count = 0x10;
        e = 0x005F5498u + 0x80u * kind;
    } else {
        count = 0x3C;
        e = 0x005F4B38u + 0x1E0u * kind;
    }
    for (int32_t i = 0; i < count; i++, e += 8) {
        b = S32(st, e) + main_id;
        if (S32(st, e + 4) >= level) {
            break;
        }
    }
    S32(st, 0x00EAAC40u) = S16(st, B(b) + 6);
    S32(st, 0x00EAAC3Cu) = S16(st, B(b) + 8);
    GAME_CALL(st, 0x00562069u, (int32_t) 0x00EAAC44u, n, 4, (int32_t) 0x004F24B0u);
}

/* @bin 0x00545BE0 (0xE5), 0x005590C0 (0xEA), 0x00556430 (0xEB), 0x0054FB40 (0xEC, +0x60 < 4):
 * thiscall on the monument's object: the first used building of the type whose thiscall
 * 0x00547C10 (id, figure, 1) (al) takes the figure */
static int32_t take_by_547c10(game_state *st, uint32_t obj, int16_t type, int32_t f)
{
    for (int32_t id = 1; id < 4000; id++) {
        if (USED(id) == 1 && TYPE(id) == type && (type != 0xEC || S32(st, B(id) + 0x60) < 4) &&
            (uint8_t) GAME_CALLT(st, obj, 0x00547C10u, id, f, 1)) {
            return id;
        }
    }
    return 0;
}

/* x87 trunc(sqrt((double) n)) for 0 <= n < 2^31: the double square root is correctly rounded
 * (53-bit precision, CRT start-up), so it never reaches the next integer above the true root,
 * and perfect squares are exact: the result is the integer square root. */
static int32_t isqrt_trunc(int32_t n)
{
    uint32_t v = (uint32_t) n, r = 0, bit = 1u << 30;
    while (bit > v) {
        bit >>= 2;
    }
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (int32_t) r;
}

/* rotate a corner code 0..7 by the view (@bin 0x004F045D, 0x004F1FB2) */
static int32_t corner_rotate(int32_t v, int32_t half)
{
    if (v < 4) {
        v += half;
        if (v >= 4) {
            v -= 4;
        }
    } else if (v < 8) {
        v += half;
        if (v >= 8) {
            v -= 4;
        }
    }
    return v;
}

/* the corner code of a pyramid part: climate 5 table 0x005E6578, else 0x005E4EF8 (8 bytes per
 * part, kind +0x54) */
static int32_t part_corner(game_state *st, int32_t b)
{
    uint32_t kind = U8(st, B(b) + 0x54);
    int32_t part = PART(b);
    if (S32(st, CLIMATE) == 5) {
        return S32(st, 0x005E6578u + 8u * (uint32_t) (part + (int32_t) kind * 0x24));
    }
    return S32(st, 0x005E4EF8u + 8u * (uint32_t) ((int32_t) kind * 0x90 + part));
}

void figure_pyramid_target(game_state *st, int32_t f, int32_t b, int32_t level, int32_t corner)
{
    /* @bin 0x004F02F0 (figure, part, level, corner) the figure moves from its part (+0x4E, level
     * +0x50) to b: +0x4A/+0x4C := the old ones, destination := b's corner (+6, +8, tile +0x0C),
     * progress +0x52 := 0, length +0x54 := 15 * the distance from its tile (+0x14, +0x16),
     * direction +0x11 (0x0051CD80 from the old part); with `corner`, +0x120 := 3 for b's rotated
     * corner code 0, 1, 4, 7, else 2 */
    uint32_t fa = F(f);
    int32_t old = S16(st, fa + 0x4A);
    S16(st, fa + 0x4A) = S16(st, fa + 0x4E);
    S16(st, fa + 0x4C) = S16(st, fa + 0x50);
    S16(st, fa + 0x4E) = (int16_t) b;
    S16(st, fa + 0x50) = (int16_t) level;
    int16_t bx = S16(st, B(b) + 6), by = S16(st, B(b) + 8);
    S16(st, fa + 0x24) = bx;
    S16(st, fa + 0x26) = by;
    S32(st, fa + 0x28) = S32(st, B(b) + 0x0C);
    int32_t dx = S16(st, fa + 0x24) - S16(st, fa + 0x14), dy = S16(st, fa + 0x26) - S16(st, fa + 0x16);
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    S16(st, fa + 0x52) = 0;
    int32_t d = isqrt_trunc((int32_t) ((uint32_t) dy * (uint32_t) dy + (uint32_t) dx * (uint32_t) dx));
    S16(st, fa + 0x54) = (int16_t) (d * 15);
    U8(st, fa + 0x11) = (uint8_t) GAME_CALL(st, 0x0051CD80u, S16(st, B(old) + 6), S16(st, B(old) + 8), bx, by);
    if (!(uint8_t) corner) {
        return;
    }
    int32_t v = corner_rotate(part_corner(st, b), S32(st, 0x00BCA2E0u) / 2);
    S16(st, fa + 0x120) = (int16_t) ((v == 0 || v == 1 || v == 4 || v == 7) ? 3 : 2);
}

int32_t pyramid_next_part(game_state *st, int32_t id)
{
    /* @bin 0x004F2190 (part) the neighbour part along the ramps of table 0x005F5CA8 (6 entries of
     * 4 part numbers: two ranges); outside them, id itself */
    int32_t part = PART(id), main_id = id - part;
    for (uint32_t i = 0; i < 6; i++) {
        uint32_t e = 0x005F5CA8u + 16u * i;
        if (part < S32(st, e) || part > S32(st, e + 0xC)) {
            continue;
        }
        if (part >= S32(st, e) && part <= S32(st, e + 4)) {
            return part == S32(st, e) ? S32(st, e + 4) + main_id + 1 : id - 1;
        }
        if (part >= S32(st, e + 8) && part <= S32(st, e + 0xC)) {
            return part == S32(st, e + 0xC) ? S32(st, e + 8) + main_id - 1 : id + 1;
        }
        return (part & 1) ? id - 2 : id + 2;
    }
    return id;
}

/* the building of the part next to tile o (offsets 0x005700F4 + 12 * k, 3 of them) that belongs
 * to the pyramid main..last, or keep *x, *y (@bin 0x004F1EF2, 0x004F20A5) */
static void part_beside(game_state *st, int32_t o, uint32_t k, int32_t main_id, int32_t last, int32_t *x, int32_t *y)
{
    for (uint32_t i = 0; i < 3; i++) {
        int32_t n = S16(st, 0x00B4E6FCu + 2u * (uint32_t) (o + S32(st, 0x005700F4u + 12u * k + 4u * i)));
        if (n >= main_id && n <= last) {
            *x = S16(st, B(n) + 6);
            *y = S16(st, B(n) + 8);
            return;
        }
    }
}

static int32_t pyramid_parts_count(game_state *st, int32_t id)
{
    uint32_t k = U8(st, B(id) + 0x54);
    return S32(st, CLIMATE) == 5 ? S32(st, 0x0056F7ECu + 4u * k) : S32(st, 0x0056F7D8u + 4u * k);
}

int32_t figure_pyramid_step(game_state *st, int32_t f, int32_t step)
{
    /* @bin 0x004F1D10 (figure, step) progress +0x52 += step up to the length +0x54 (al: 1 when
     * arrived, or with no length); the figure's tile (+0x14, +0x16, +0x20) is the corner of the
     * part it walks along: from the old part +0x4A to the new +0x4E by the rotated corner code
     * of the old one, or interpolated on the ramp (+0x120 == 4); a moved figure is taken off and
     * put back on its tile list (0x004B5420, 0x004B52F0) */
    uint32_t fa = F(f);
    if (S16(st, fa + 0x54) < 1) {
        return 1;
    }
    int done = 0;
    S16(st, fa + 0x52) = (int16_t) (S16(st, fa + 0x52) + (int16_t) step);
    if (S16(st, fa + 0x52) >= S16(st, fa + 0x54)) {
        done = 1;
        S16(st, fa + 0x52) = S16(st, fa + 0x54);
    }
    int32_t prev = S16(st, fa + 0x4A), target = S16(st, fa + 0x4E);
    if (prev != 0) {
        U8(st, fa + 0x11) = (uint8_t) GAME_CALL(st, 0x0051CD80u, S16(st, B(prev) + 6), S16(st, B(prev) + 8),
                                                S16(st, B(target) + 6), S16(st, B(target) + 8));
    } else {
        U8(st, fa + 0x11) = 0;
    }
    int32_t half = S32(st, 0x00BCA2E0u) / 2, x, y, tile;
    if (prev == 0) {
        y = S16(st, B(target) + 8);
        x = S16(st, B(target) + 6);
        tile = S32(st, 0x00BCA2D8u) + y * GRID_SIZE + x;
    } else if (S16(st, fa + 0x120) == 4) {
        /* @bin 0x004F1E1A on the ramp */
        int32_t main_id = prev - PART(prev), last = pyramid_parts_count(st, prev) + main_id - 1;
        int32_t x0 = S16(st, B(prev) + 6), y0 = S16(st, B(prev) + 8);
        int32_t p = S16(st, fa + 0x52), q = S16(st, fa + 0x54);
        int32_t ty = (S16(st, B(target) + 8) - y0) * p / q + y0;
        int32_t tx = (S16(st, B(target) + 6) - x0) * p / q;
        int32_t n = S16(st, 0x00B4E6FCu + 2u * (uint32_t) (x0 + tx + ty * GRID_SIZE + S32(st, 0x00BCA2D8u)));
        x = S16(st, B(n) + 6);
        y = S16(st, B(n) + 8);
        part_beside(st, S32(st, B(n) + 0x0C), (uint32_t) half, main_id, last, &x, &y);
        tile = S32(st, 0x00BCA2D8u) + y * GRID_SIZE + x;
    } else {
        int32_t v = corner_rotate(part_corner(st, prev), half);
        int32_t d = (int8_t) U8(st, F(S32(st, 0x00D056F8u)) + 0x11) - S32(st, 0x00BCA2E0u) + 1;
        if (d < 0) {
            d += 8;
        }
        int side = d <= 1 || d >= 6;
        int beside = 0;
        int32_t b;
        switch (v) {
        case 0: case 7: beside = 1; b = prev; break;
        case 1: case 3: b = target; break;
        case 5: b = side ? prev : target; break;
        default: b = prev; break;
        }
        x = S16(st, B(b) + 6);
        y = S16(st, B(b) + 8);
        tile = S32(st, B(b) + 0x0C);
        if (beside) {
            int32_t main_id = prev - PART(prev), last = pyramid_parts_count(st, prev) + main_id - 1;
            part_beside(st, S32(st, B(prev) + 0x0C), (uint32_t) ((half - 2) & 3), main_id, last, &x, &y);
            tile = S32(st, 0x00BCA2D8u) + y * GRID_SIZE + x;
        }
    }
    int moved = !(S16(st, fa + 0x14) == x && S16(st, fa + 0x16) == y);
    if (moved) {
        GAME_CALL(st, 0x004B5420u, f);
    }
    S16(st, fa + 0x14) = (int16_t) x;
    S16(st, fa + 0x16) = (int16_t) y;
    S32(st, fa + 0x20) = tile;
    if (moved) {
        GAME_CALL(st, 0x004B52F0u, f);
    }
    return done;
}

void figure_pyramid_work(game_state *st, int32_t f, int32_t mode, int32_t idle_action, int32_t ramp_action)
{
    /* @bin 0x004F04D0 (figure, mode, idle action, ramp action) a worker on a pyramid (the
     * building at its tile +0x28; else the current figure 0x00D056F8 is done, +0x0E := 2): once
     * 0x004F1D10 (figure, 1) says it arrived, it moves on to the next part of the kind's level
     * list (climate 5: 16 of 0x005F5498, else 60 of 0x005F4B38 by kind; 8 bytes: part, level):
     * mode 1 (stones): while its part (+0x64) is not ready it stays (idle action); mode 0 / 2
     * against the first part in progress (0x004EF450). Climbing (+0x120 == 1) it heads for the
     * level of the first part in progress; at the list's end it goes down the ramp (ramp action,
     * +0x120 := 4); a stone mason (0x51, mode 0) at the top of a finished pyramid turns back
     * (action 0x10, 0x004F2190). */
    uint32_t fa = F(f);
    int32_t bid = S16(st, 0x00B4E6FCu + 2u * U32(st, fa + 0x28));
    if (TYPE(bid) != 0xB7) {
        U8(st, F(S32(st, 0x00D056F8u)) + 0x0E) = 2;
        return;
    }
    if ((uint8_t) figure_pyramid_step(st, f, 1) != 1) {
        return;
    }
    int32_t count;
    uint32_t tab;
    if (S32(st, CLIMATE) == 5) {
        count = 0x10;
        tab = 0x005F5498u + 0x80u * U8(st, B(bid) + 0x54);
    } else {
        count = 0x3C;
        tab = 0x005F4B38u + 0x1E0u * U8(st, B(bid) + 0x54);
    }
    GAME_CALL(st, 0x004B5420u, f);
    S16(st, fa + 0x14) = S16(st, fa + 0x24);
    S16(st, fa + 0x16) = S16(st, fa + 0x26);
    S32(st, fa + 0x20) = S32(st, fa + 0x28);
    GAME_CALL(st, 0x004B52F0u, f);
    int32_t i = 0, first = 0;
    if (mode == 1) {
        if (S16(st, B(bid) + 0x64) == (int16_t) f) {
            if (pyramid_part_ready(st, bid) != 0) {
                goto ramp;
            }
            goto idle;
        }
        int32_t main_id = bid - PART(bid), k;
        for (k = 0; k < count; k++) {
            int32_t p = main_id + S32(st, tab + 8u * (uint32_t) k);
            if (S16(st, B(p) + 0x64) == (int16_t) f && pyramid_part_ready(st, p) != 0) {
                break;
            }
        }
        if (k >= count) {
            goto idle;
        }
    }
    if (S16(st, fa + 0x120) == 1) {
        GAME_CALL(st, 0x004EF450u, bid - PART(bid));
        if (S32(st, 0x00EAAF58u) < 1) {
            goto idle;
        }
        int32_t q = S32(st, 0x00EAAC44u);
        if (S16(st, fa + 0x50) < U8(st, B(q) + 0x56) + U8(st, B(q) + 0x57) * 6) {
            goto next; /* i = 0 */
        }
        goto ramp;
    }
    if (mode == 0 || mode == 2) {
        GAME_CALL(st, 0x004EF450u, bid - PART(bid));
        if (S32(st, 0x00EAAF58u) < 1) {
            U8(st, fa + 0x38) = (uint8_t) idle_action;
            S16(st, fa + 0x120) = 0;
            return;
        }
        first = S32(st, 0x00EAAC44u);
        if (S16(st, fa + 0x50) >= U8(st, B(first) + 0x56) + U8(st, B(first) + 0x57) * 6) {
            goto ramp;
        }
    } else {
        first = 0;
    }
    for (i = 0; i < count; i++) {
        if (S32(st, tab + 8u * (uint32_t) i) == PART(bid) && S32(st, tab + 8u * (uint32_t) i + 4) == S16(st, fa + 0x50)) {
            break;
        }
    }
    if (i >= count) {
        goto idle;
    }
    i++;
    if (S32(st, tab + 8u * (uint32_t) i) >= 0) {
        goto next;
    }
    if (mode == 1) {
        goto idle;
    }
    if (mode != 0 || U8(st, fa + 0x0A) != 0x51 || U8(st, B(first) + 0x55) != 1) {
        goto ramp;
    }
    U8(st, fa + 0x38) = 0x10;
    figure_pyramid_target(st, f, pyramid_next_part(st, S16(st, fa + 0x4E)), S16(st, fa + 0x50), 0);
    return;
next: {
        int32_t p = S32(st, tab + 8u * (uint32_t) i) - PART(bid) + bid;
        if (mode == 2 && pyramid_part_ready(st, p) != 0) {
            U8(st, fa + 1) = 0;
            return;
        }
        int same = p == S16(st, fa + 0x4E);
        figure_pyramid_target(st, f, p, S32(st, tab + 8u * (uint32_t) i + 4), 1);
        if (same) {
            S16(st, fa + 0x52) = S16(st, fa + 0x54);
        }
        return;
    }
ramp:
    S16(st, fa + 0x120) = 4;
    U8(st, fa + 0x38) = (uint8_t) ramp_action;
    return;
idle:
    U8(st, fa + 0x38) = (uint8_t) idle_action;
    S16(st, fa + 0x120) = 0;
}

int32_t pyramid_linked_part(game_state *st, int32_t id, uint8_t *below)
{
    /* @bin 0x004F27B0 (part, out flag) the pair list of the kind (climate 5: 0x005F5D58 + 0xA0 *
     * kind, else 0x005F5F38 + 0x270 * kind; pairs (a, b) until a < 0): for the first pair with
     * b == part, the part in progress (list 0x00EAAC44) numbered a (*below := 1); for a == part,
     * the one numbered b; -1 if none */
    *below = 0;
    uint32_t kind = U8(st, B(id) + 0x54);
    uint32_t tab = S32(st, CLIMATE) == 5 ? 0x005F5D58u + 0xA0u * kind : 0x005F5F38u + 0x270u * kind;
    int32_t part = PART(id), n;
    if (S32(st, tab) < 0) {
        return -1;
    }
    for (uint32_t e = tab;; e += 8) {
        if (S32(st, e + 4) == part) {
            n = S32(st, 0x00EAAF58u);
            for (int32_t i = 0; i < n; i++) {
                int32_t q = S32(st, 0x00EAAC44u + 4u * (uint32_t) i);
                if (PART(q) == S32(st, e)) {
                    *below = 1;
                    return q;
                }
            }
            return -1;
        }
        if (S32(st, e) == part) {
            n = S32(st, 0x00EAAF58u);
            for (int32_t i = 0; i < n; i++) {
                int32_t q = S32(st, 0x00EAAC44u + 4u * (uint32_t) i);
                if (PART(q) == S32(st, e + 4)) {
                    return q;
                }
            }
            return -1;
        }
        if (S32(st, e + 8) < 0) {
            return -1;
        }
    }
}

static int32_t pyramid_has_below(game_state *st, int32_t id)
{
    /* @bin 0x004F28B0 a linked part in progress below this one (al) */
    uint8_t below;
    return pyramid_linked_part(st, id, &below) >= 1 && below != 0;
}

static int32_t pyramid_linked_worker(game_state *st, int32_t id)
{
    /* @bin 0x004F28E0 the worker (+0x64) of the linked part when it is a figure 0x50 at action
     * 0xB or 0x51 at action 0xA; else 0; -1 when no linked part */
    uint8_t below;
    int32_t r = pyramid_linked_part(st, id, &below);
    if (r < 1) {
        return -1;
    }
    int32_t w = S16(st, B(r) + 0x64);
    if (w != 0) {
        uint32_t wa = F(w);
        if (U8(st, wa + 0x0A) == 0x50) {
            return U8(st, wa + 0x38) == 0xB ? w : 0;
        }
        if (U8(st, wa + 0x0A) == 0x51) {
            return U8(st, wa + 0x38) == 0xA ? w : 0;
        }
    }
    return 0;
}

int32_t pyramid_waiting_worker(game_state *st, int32_t id, int32_t release)
{
    /* @bin 0x004F29E0 (part, release) the parts in progress of the pyramid (0x004EF450, sorted
     * 0x004F25C0): the first without a linked part below whose worker (+0x64, still busy:
     * 0x004EEA10) is a 0x51 at action 0xA or a 0x50 at action 0xB, and whose linked part has its
     * worker too (0x004F28E0 non-zero): returned when it has no builder (+0x68 working on it,
     * 0x004F26A0); with release and a builder not at one of the actions 0x11 .. 0x14, 0x1D, 0x1E,
     * the builder link +0x68 is cleared and it is returned */
    GAME_CALL(st, 0x004EF450u, id - PART(id));
    pyramid_parts_sort(st);
    for (int32_t i = 0; i < S32(st, 0x00EAAF58u); i++) {
        int32_t part = S32(st, 0x00EAAC44u + 4u * (uint32_t) i);
        if ((uint8_t) pyramid_has_below(st, part)) {
            continue;
        }
        uint32_t slot = B(part) + 0x64;
        int32_t w = S16(st, slot);
        if (w == 0) {
            continue;
        }
        if (!(uint8_t) pyramid_worker_busy(st, part)) {
            if (S16(st, slot) != 0) {
                S16(st, slot) = 0;
            }
            continue;
        }
        uint32_t wa = F(w);
        uint8_t type = U8(st, wa + 0x0A);
        if (!((type == 0x51 && U8(st, wa + 0x38) == 0xA) || (type == 0x50 && U8(st, wa + 0x38) == 0xB))) {
            continue;
        }
        int has_builder = 0, building = 0;
        int16_t bld = S16(st, wa + 0x68);
        if (bld != 0) {
            if ((uint8_t) GAME_CALL(st, 0x004F26A0u, bld, w)) {
                has_builder = 1;
                int32_t act = act_of(st, bld);
                if (act == 0x12 || act == 0x13 || act == 0x14 || act == 0x1D || act == 0x11 || act == 0x1E) {
                    building = 1;
                }
            }
        }
        if (pyramid_linked_worker(st, part) == 0) {
            continue;
        }
        if (!has_builder) {
            return w;
        }
        if (!building && (uint8_t) release) {
            S16(st, F(w) + 0x68) = 0;
            return w;
        }
    }
    return 0;
}

void pyramid_part_advance(game_state *st, int32_t id)
{
    /* @bin 0x004EF200 (part) a ready part (0x004EF100; not climate 5): its entry in the kind's
     * list 0x005F3878 (0x14 bytes: part, first level, second level, partners) whose first level
     * is reached with +0x59 still 0 marks +0x59 := level (partner +0x0C), else the second with
     * +0x5A (partner +0x10); the part is redrawn (0x004EDF50 (part, orientation / 2, kind)); in
     * climate 3 the partner part gets the same mark and is redrawn */
    if (S32(st, CLIMATE) == 5) {
        return;
    }
    uint32_t b = B(id), kind = U8(st, b + 0x54);
    if (pyramid_part_ready(st, id) == 0) {
        return;
    }
    int32_t count = S32(st, 0x00570070u + 4u * kind), value = 0, partner = 0, first = 0;
    for (int32_t i = 0; i < count; i++) {
        uint32_t e = 0x005F3878u + 0x3C0u * kind + 0x14u * (uint32_t) i;
        if (PART(id) != S32(st, e)) {
            continue;
        }
        int32_t level = U8(st, b + 0x56) + U8(st, b + 0x57) * 6;
        int32_t a = S32(st, e + 4);
        if (a > 0 && a <= level && U8(st, b + 0x59) == 0) {
            partner = S32(st, e + 0x0C);
            value = a;
            U8(st, b + 0x59) = (uint8_t) value;
            first = 1;
        } else {
            int32_t c = S32(st, e + 8);
            if (c > 0 && c <= level && U8(st, b + 0x5A) == 0) {
                partner = S32(st, e + 0x10);
                value = c;
                U8(st, b + 0x5A) = (uint8_t) value;
                first = 0;
            }
        }
        if (value != 0) {
            int32_t half = S32(st, 0x00BCA2E0u) / 2;
            GAME_CALL(st, 0x004EDF50u, id, half, (int32_t) kind);
            if (S32(st, CLIMATE) != 3 || partner == 0) {
                return;
            }
            int32_t t = partner + (id - PART(id));
            U8(st, B(t) + (first ? 0x59 : 0x5A)) = (uint8_t) value;
            GAME_CALL(st, 0x004EDF50u, t, half, (int32_t) kind);
            return;
        }
    }
}

int32_t pyramid_mason_free(game_state *st, int32_t f)
{
    /* @bin 0x004F2BA0 (figure) with +0x3A > 0 at a pyramid tile (+0x28): 0 when a part's worker
     * (+0x64: 0x51 at action 0xA or 0x50 at 0xB) has a builder (+0x68: 0x55 at action 0x11, +0x3A
     * > 0) whose +0x3E - +0x3C is within 7 of the figure's; else 1 (al) */
    uint32_t fa = F(f);
    if (S16(st, fa + 0x3A) <= 0) {
        return 1;
    }
    int32_t b = S16(st, 0x00B4E6FCu + 2u * U32(st, fa + 0x28));
    if (b == 0 || TYPE(b) != 0xB7) {
        return 1;
    }
    int32_t main_id = b - PART(b), span = S16(st, fa + 0x3E) - S16(st, fa + 0x3C);
    uint32_t kind = U8(st, B(main_id) + 0x54);
    for (int32_t id = main_id;; id++) {
        int32_t n = S32(st, CLIMATE) == 5 ? S32(st, 0x0056F7ECu + 4u * kind) : S32(st, 0x0056F7D8u + 4u * kind);
        if (id >= n + main_id) {
            return 1;
        }
        uint32_t wa = F(S16(st, B(id) + 0x64));
        uint8_t t = U8(st, wa + 0x0A);
        if (!((t == 0x51 && U8(st, wa + 0x38) == 0xA) || (t == 0x50 && U8(st, wa + 0x38) == 0xB))) {
            continue;
        }
        int32_t u = S16(st, wa + 0x68);
        if (u == 0) {
            continue;
        }
        uint32_t ua = F(u);
        if (U8(st, ua + 0x0A) != 0x55 || U8(st, ua + 0x38) != 0x11 || S16(st, ua + 0x3A) <= 0) {
            continue;
        }
        int32_t d = S16(st, ua + 0x3E) - S16(st, ua + 0x3C) - span;
        if ((d < 0 ? -d : d) < 7) {
            return 0;
        }
    }
}

int32_t figure_pyramid_block(game_state *st, int32_t f)
{
    /* @bin 0x004F4320 (figure) at a pyramid (+0x5E, type 0xB7) past stage 1: a block slot of the
     * pyramid (0x004F3370 (main, &x, &y, 1)) becomes the destination: a line from its part
     * (+0x4A, +0x4C) to (15 x, 15 y) (0x004B4220), +0x44 := 0, +0x5E := the building there, whose
     * +0x64 := the figure (al) */
    uint32_t fa = F(f);
    int32_t b = S16(st, fa + 0x5E);
    if (TYPE(b) != 0xB7 || U8(st, B(b) + 0x55) <= 1) {
        return 0;
    }
    int32_t x = 0, y = 0;
    int8_t r = pyramid_block_slot(st, b - PART(b), &x, &y, 1);
    if (r == 0) {
        return 0;
    }
    S16(st, fa + 0x24) = (int16_t) x;
    S16(st, fa + 0x26) = (int16_t) y;
    figure_line_to(st, f, S16(st, fa + 0x4A), S16(st, fa + 0x4C), S16(st, fa + 0x24) * 15, (int16_t) y * 15, 0);
    S16(st, fa + 0x44) = 0;
    int16_t t = S16(st, 0x00B4E6FCu + 2u * (uint32_t) (S32(st, 0x00BCA2D8u) + y * GRID_SIZE + x));
    S16(st, fa + 0x5E) = t;
    S16(st, B(t) + 0x64) = (int16_t) f;
    return 1;
}

static int32_t pyramid_tile_work(game_state *st, int32_t o)
{
    /* @bin 0x004F4420 (tile) 50 for a pyramid tile state 2..12 (0x004F3D70), else 0 */
    int32_t s = land_pyramid_tile_state(st, o);
    return s > 1 && s < 0xD ? 0x32 : 0;
}

int32_t figure_pyramid_lay(game_state *st, int32_t f)
{
    /* @bin 0x004F4440 (figure) on a building tile (+0x14, +0x16): +0x36 counts up to the tile's
     * work (0x004F4420); then the block is laid (0x004F3690 (+0x24, +0x26)), the building's
     * +0x64 := 0 and 0; else 1 (al) */
    uint32_t fa = F(f);
    int32_t o = S32(st, 0x00BCA2D8u) + S16(st, fa + 0x14) + S16(st, fa + 0x16) * GRID_SIZE;
    int32_t b = S16(st, 0x00B4E6FCu + 2u * (uint32_t) o);
    if (b == 0) {
        return 0;
    }
    int32_t lim = pyramid_tile_work(st, o);
    S16(st, fa + 0x36) = (int16_t) (S16(st, fa + 0x36) + 1);
    if (S16(st, fa + 0x36) >= lim) {
        GAME_CALL(st, 0x004F3690u, S16(st, fa + 0x24), S16(st, fa + 0x26));
        S16(st, B(b) + 0x64) = 0;
        return 0;
    }
    return 1;
}

static int32_t parts_index(game_state *st, int32_t id)
{
    for (int32_t i = 0; i < S32(st, 0x00EAAF58u); i++) {
        if (S32(st, 0x00EAAC44u + 4u * (uint32_t) i) == id) {
            return i;
        }
    }
    return -1;
}

void pyramid_builder_pair(game_state *st, int32_t f)
{
    /* @bin 0x004F2CF0 (builder) at a pyramid tile (+0x28): keeps its mason (+0x68, still busy at
     * its part: 0x004EEA10) or takes a waiting one (0x004F29E0 (part, 1)); then among the parts
     * in progress (0x004EF450, sorted) earlier than its mason's, the earliest whose worker (+0x64:
     * 0x51 at action 0xA or 0x50 at 0xB, no linked part below) has a builder of the same good
     * (+0x7A) that is fetching or working (0x55 at action 0xF, 0x10, 0x1E, 0x11) swaps masons
     * with it */
    uint32_t fa = F(f);
    int32_t bid = S16(st, 0x00B4E6FCu + 2u * U32(st, fa + 0x28));
    if (bid == 0 || TYPE(bid) != 0xB7) {
        return;
    }
    int32_t main_id = bid - PART(bid), bw = 0, bu = 0;
    uint32_t kind = U8(st, B(main_id) + 0x54);
    int32_t m = S16(st, fa + 0x68), keep = 0;
    if (m != 0) {
        int32_t mb = S16(st, 0x00B4E6FCu + 2u * U32(st, F(m) + 0x28));
        if (mb != 0 && (uint8_t) pyramid_worker_busy(st, mb)) {
            keep = 1;
        }
    }
    if (!keep) {
        m = pyramid_waiting_worker(st, bid, 1);
        if (m == 0) {
            return;
        }
        S16(st, fa + 0x68) = (int16_t) m;
        S16(st, F(m) + 0x68) = (int16_t) f;
    }
    int32_t mpart = S16(st, 0x00B4E6FCu + 2u * U32(st, F(m) + 0x28));
    GAME_CALL(st, 0x004EF450u, main_id);
    pyramid_parts_sort(st);
    int32_t found = parts_index(st, mpart);
    if (found <= 0) {
        return;
    }
    int32_t best = found;
    for (int32_t id = main_id;; id++) {
        int32_t n = S32(st, CLIMATE) == 5 ? S32(st, 0x0056F7ECu + 4u * kind) : S32(st, 0x0056F7D8u + 4u * kind);
        if (id >= n + main_id) {
            break;
        }
        int32_t w = S16(st, B(id) + 0x64);
        if (w == 0 || (uint8_t) pyramid_has_below(st, id)) {
            continue;
        }
        uint32_t wa = F(w);
        uint8_t t = U8(st, wa + 0x0A);
        if (!((t == 0x51 && U8(st, wa + 0x38) == 0xA) || (t == 0x50 && U8(st, wa + 0x38) == 0xB))) {
            continue;
        }
        int32_t u = S16(st, wa + 0x68);
        if (u == f) {
            continue;
        }
        if (u != 0) {
            uint32_t ua = F(u);
            if (S16(st, ua + 0x68) != w) {
                S16(st, wa + 0x68) = 0;
                u = 0;
            } else {
                uint8_t a = U8(st, ua + 0x38);
                if (U8(st, ua + 0x0A) != 0x55 || !(a == 0xF || a == 0x10 || a == 0x1E || a == 0x11)) {
                    continue;
                }
            }
        }
        int32_t idx = parts_index(st, id);
        if (idx < 0) {
            idx = 999;
        }
        if (idx < best && u != 0 && U8(st, fa + 0x7A) == U8(st, F(u) + 0x7A)) {
            best = idx;
            bw = w;
            bu = u;
        }
    }
    if (best == found) {
        return;
    }
    int32_t old = S16(st, fa + 0x68);
    S16(st, F(bw) + 0x68) = (int16_t) f;
    S16(st, fa + 0x68) = (int16_t) bw;
    if (bu != 0) {
        S16(st, F(old) + 0x68) = (int16_t) bu;
        S16(st, F(bu) + 0x68) = (int16_t) old;
    }
}

/* ---- the monuments' work-animation lists: 16-byte entries (+0 key, +4 figure type, +8 object
 * with +0x0C the work left, +0x10 state, +0x12 its figure, +0x14 that figure's action) ---- */

void anim_object_figure(game_state *st, uint32_t obj, int32_t f)
{
    /* @bin 0x0041FD80 (thiscall on the object) (figure) +0x12 := figure, +0x14 := its action */
    S16(st, obj + 0x12) = (int16_t) f;
    if (f != 0) {
        U8(st, obj + 0x14) = U8(st, F(f) + 0x38);
    }
}

/* the object of entry e takes the figure when free, then one unit of work is done; returns 1
 * when no work is left (@bin 0x004201FF, 0x00420BF1, 0x00420EF2, 0x004211E7) */
static int anim_work(game_state *st, uint32_t e, int32_t f)
{
    uint32_t obj = U32(st, e + 8);
    if (S16(st, obj + 0x12) == 0) {
        anim_object_figure(st, obj, f);
        GAME_CALLT(st, U32(st, e + 8), 0x0041FCA0u, 0);
    }
    obj = U32(st, e + 8);
    S32(st, obj + 0x0C) = S32(st, obj + 0x0C) - 1;
    return S32(st, U32(st, e + 8) + 0x0C) < 1;
}

/* the first entry of [first, end) with a free object, the key, the figure's type and work left */
static uint32_t anim_free_entry(game_state *st, uint32_t first, uint32_t end, int32_t key, int32_t f)
{
    for (uint32_t e = first; e < end; e += 0x10) {
        uint32_t obj = U32(st, e + 8);
        if (S16(st, obj + 0x12) == 0 && S32(st, e) == key && S32(st, e + 4) == (int32_t) U8(st, F(f) + 0x0A) &&
            S32(st, obj + 0x0C) > 0) {
            return e;
        }
    }
    return 0;
}

static int anim_key_left(game_state *st, uint32_t first, uint32_t end, int32_t key)
{
    for (uint32_t e = first; e < end; e += 0x10) {
        if (S32(st, e) == key && S32(st, U32(st, e + 8) + 0x0C) > 0) {
            return 1;
        }
    }
    return 0;
}

static int32_t city_monument_key(game_state *st)
{
    return S16(st, CITY_ADDR(S32(st, CURRENT_PLAYER_ADDR)) + 0x433A);
}

void city_monument_check(game_state *st)
{
    /* @bin 0x004200B0 no entry of 0x005C2748 for the city's key has work left: 0x0053D820 */
    if (!anim_key_left(st, 0x005C2748u, 0x005C28A8u, city_monument_key(st))) {
        GAME_CALL0(st, 0x0053D820u);
    }
}

void city_monument_work(game_state *st, int32_t x, int32_t f, int32_t action)
{
    /* @bin 0x00420130 (x, figure, action) figure +0x65 := 1; the entry of 0x005C2748 whose object
     * holds the figure (any key other than the city's +0x433A: action and done), else a free one
     * for the city and the figure's type (none: action); one unit of work; none left:
     * 0x004200B0 and the action */
    (void) x;
    uint32_t fa = F(f), e = 0;
    U8(st, fa + 0x65) = 1;
    for (uint32_t k = 0x005C2748u; k < 0x005C28A8u; k += 0x10) {
        if (S16(st, U32(st, k + 8) + 0x12) == f) {
            if (S32(st, k) != city_monument_key(st)) {
                U8(st, fa + 0x38) = (uint8_t) action;
                return;
            }
            e = k;
            break;
        }
    }
    if (e == 0) {
        e = anim_free_entry(st, 0x005C2748u, 0x005C28A8u, city_monument_key(st), f);
        if (e == 0) {
            U8(st, fa + 0x38) = (uint8_t) action;
            return;
        }
    }
    if (anim_work(st, e, f)) {
        city_monument_check(st);
        U8(st, fa + 0x38) = (uint8_t) action;
    }
}

void monument_e6_done(game_state *st, int32_t id, int32_t idx)
{
    /* @bin 0x00420A90 (id, entry) thiscall 0x00401D30 on 0x006DE650 (id, entry); no entry of
     * 0x005A8140 for the main building's key +0x54 (dword) with work left: 0x00401BE0 (id) */
    int32_t key = S32(st, B(id - PART(id)) + 0x54);
    GAME_CALLT(st, 0x006DE650u, 0x00401D30u, id, idx);
    if (!anim_key_left(st, 0x005A8140u, 0x005A84C0u, key)) {
        GAME_CALLT(st, 0x006DE650u, 0x00401BE0u, id);
    }
}

void monument_e6_work(game_state *st, int32_t id, int32_t f, int32_t action)
{
    /* @bin 0x00420B10 (id, figure, action) key: the main building's +0x54 (dword); the entry of
     * 0x005A8140 holding the figure with the key and work left, else a free one; one unit of
     * work; none left: 0x00420A90 (id, entry), action, object state +0x10 := 3 */
    uint32_t fa = F(f), e = 0;
    int32_t key = S32(st, B(id - PART(id)) + 0x54);
    U8(st, fa + 0x65) = 1;
    for (uint32_t k = 0x005A8140u; k < 0x005A84C0u; k += 0x10) {
        uint32_t obj = U32(st, k + 8);
        if (S16(st, obj + 0x12) == f && S32(st, k) == key && S32(st, obj + 0x0C) > 0) {
            e = k;
            break;
        }
    }
    if (e == 0) {
        e = anim_free_entry(st, 0x005A8140u, 0x005A84C0u, key, f);
        if (e == 0) {
            U8(st, fa + 0x38) = (uint8_t) action;
            return;
        }
    }
    if (anim_work(st, e, f)) {
        monument_e6_done(st, id, (int32_t) ((e - 0x005A8140u) / 0x10));
        U8(st, fa + 0x38) = (uint8_t) action;
        S16(st, U32(st, e + 8) + 0x10) = 3;
    }
}

static void stage_done(game_state *st, int32_t id, uint32_t first, uint32_t end, uint32_t obj, uint32_t fn)
{
    /* @bin 0x00420DC0 (E3: 0x005AD210, thiscall 0x00417640 on 0x006FD368), 0x004210C0 (E4:
     * 0x005B0058, 0x004E6BD0 on 0x006DE548) (id) no entry for the stage +0x58 with work left:
     * the object's method (id) */
    if (!anim_key_left(st, first, end, S32(st, B(id) + 0x58))) {
        GAME_CALLT(st, obj, fn, id);
    }
}

/* @bin 0x00420E20 (E3: table 0x005AD210, the search for the figure from the stage's entry,
 * stage < 0x1B) and 0x00421120 (E4: 0x005B0058) (id, figure, action) */
static void stage_work(game_state *st, int32_t id, int32_t f, int32_t action, int e3)
{
    uint32_t fa = F(f), e = 0;
    uint32_t first = e3 ? 0x005AD210u : 0x005B0058u, end = e3 ? 0x005AD3C0u : 0x005B0148u;
    int32_t key = S32(st, B(id) + 0x58);
    U8(st, fa + 0x65) = 1;
    if (!e3 || key < 0x1B) {
        for (uint32_t k = e3 ? first + 0x10u * (uint32_t) key : first; k < end; k += 0x10) {
            if (S16(st, U32(st, k + 8) + 0x12) == f) {
                if (S32(st, k) != key) {
                    U8(st, fa + 0x38) = (uint8_t) action;
                    return;
                }
                e = k;
                break;
            }
        }
    }
    if (e == 0) {
        e = anim_free_entry(st, first, end, key, f);
        if (e == 0) {
            U8(st, fa + 0x38) = (uint8_t) action;
            return;
        }
    }
    if (anim_work(st, e, f)) {
        if (e3) {
            stage_done(st, id, first, end, 0x006FD368u, 0x00417640u);
            S32(st, B(id) + 0x5C) = 0;
        } else {
            stage_done(st, id, first, end, 0x006DE548u, 0x004E6BD0u);
        }
        U8(st, fa + 0x38) = (uint8_t) action;
    }
}

/* the entry holding the figure from `from` on: its key must be `key` (else the figure gets the
 * action and *done := 1) (@bin 0x0042014F, 0x004207E0) */
static uint32_t anim_held_entry(game_state *st, uint32_t from, uint32_t end, int32_t key, int32_t f, int *done)
{
    for (uint32_t k = from; k < end; k += 0x10) {
        if (S16(st, U32(st, k + 8) + 0x12) == f) {
            if (S32(st, k) != key) {
                *done = 1;
                return 0;
            }
            return k;
        }
    }
    return 0;
}

void obelisk_stage_up(game_state *st, uint32_t obj, int32_t id)
{
    /* @bin 0x0040ECF0 (thiscall on the obelisk object) (id) below stage 0x15 (+0x58 of the main
     * building), the 25 parts get the next stage, each redrawn by the object's method +0x18 */
    int32_t main_id = id - PART(id), v = S32(st, B(main_id) + 0x58);
    if (v >= 0x15) {
        return;
    }
    for (int32_t b = main_id; b < id - PART(id) + 0x19; b++) {
        S32(st, B(b) + 0x58) = v + 1;
        GAME_CALLT(st, obj, U32(st, U32(st, obj) + 0x18), b);
    }
}

void obelisk_stage_check(game_state *st, int32_t id)
{
    /* @bin 0x00420770 (id) no entry of 0x0059EDE8 for the main building's stage with work left:
     * 0x0040ECF0 on 0x006FD5D8 */
    if (!anim_key_left(st, 0x0059EDE8u, 0x0059EF98u, S32(st, B(id - PART(id)) + 0x58))) {
        obelisk_stage_up(st, 0x006FD5D8u, id);
    }
}

void obelisk_work(game_state *st, int32_t id, int32_t f, int32_t action)
{
    /* @bin 0x004207E0 (id, figure, action) the work list 0x0059EDE8 for the main building's
     * stage +0x58: the entry holding the figure, else a free one; one unit of work; none left:
     * 0x00420770 and the action */
    uint32_t fa = F(f);
    int32_t key = S32(st, B(id - PART(id)) + 0x58);
    int done = 0;
    U8(st, fa + 0x65) = 1;
    uint32_t e = anim_held_entry(st, 0x0059EDE8u, 0x0059EF98u, key, f, &done);
    if (e == 0 && !done) {
        e = anim_free_entry(st, 0x0059EDE8u, 0x0059EF98u, key, f);
    }
    if (e == 0) {
        U8(st, fa + 0x38) = (uint8_t) action;
        return;
    }
    if (anim_work(st, e, f)) {
        obelisk_stage_check(st, id);
        U8(st, fa + 0x38) = (uint8_t) action;
    }
}

static int32_t monument_stage_step(game_state *st, int32_t id)
{
    /* @bin 0x004E5AE0 (id) x87 trunc(67.0 / n + 0.5), n = 10 for type 0xCF else 18 (the
     * constants 0x0056F6D0, 0x0056F690): 7 or 4 */
    return TYPE(id) != 0xCF ? 4 : 7;
}

int32_t monument_stage_advance(game_state *st, int32_t id)
{
    /* @bin 0x004E5B20 (id) below 100, +0x55 grows by the step (at most 100) and the monument is
     * redrawn (0x004E5840 (id, x, y)); al: +0x55 == 100 */
    uint32_t b = B(id);
    if (U8(st, b + 0x55) == 100) {
        return 1;
    }
    uint8_t v = (uint8_t) (U8(st, b + 0x55) + monument_stage_step(st, id));
    U8(st, b + 0x55) = v;
    if (v > 100) {
        U8(st, b + 0x55) = 100;
    }
    GAME_CALL(st, 0x004E5840u, id, S16(st, b + 6), S16(st, b + 8));
    return U8(st, b + 0x55) == 100;
}

void monument_list_work(game_state *st, int32_t id, int32_t f, int32_t action)
{
    /* @bin 0x004E5D10 (id, figure, action) the monument's list (10 of 0x00586B08 for type 0xCF,
     * else 18 of 0x0058FE48) keyed by its stage count (0x004E5A30): the entry holding the
     * figure, else a free one; one unit of work; none left: 0x004E5B20 and the action */
    uint32_t fa = F(f), e = 0;
    U8(st, fa + 0x65) = 1;
    for (int32_t i = 0; i < (TYPE(id) != 0xCF ? 18 : 10); i++) {
        uint32_t k = (TYPE(id) == 0xCF ? 0x00586B08u : 0x0058FE48u) + 0x10u * (uint32_t) i;
        if (S16(st, U32(st, k + 8) + 0x12) == f) {
            if (S32(st, k) != monument_stage_count(st, id)) {
                U8(st, fa + 0x38) = (uint8_t) action;
                return;
            }
            e = k;
            break;
        }
    }
    if (e == 0) {
        for (int32_t i = 0; i < (TYPE(id) != 0xCF ? 18 : 10); i++) {
            uint32_t k = (TYPE(id) == 0xCF ? 0x00586B08u : 0x0058FE48u) + 0x10u * (uint32_t) i;
            if (S16(st, U32(st, k + 8) + 0x12) != 0) {
                continue;
            }
            if (S32(st, k) == monument_stage_count(st, id) && S32(st, k + 4) == (int32_t) U8(st, fa + 0x0A) &&
                S32(st, U32(st, k + 8) + 0x0C) > 0) {
                e = k;
                break;
            }
        }
        if (e == 0) {
            U8(st, fa + 0x38) = (uint8_t) action;
            return;
        }
    }
    if (anim_work(st, e, f)) {
        monument_stage_advance(st, id);
        U8(st, fa + 0x38) = (uint8_t) action;
    }
}

/* @bin 0x0040EB90 (the obelisk object's method +0x20, as core/building/day_road.c ports it)
 * the entry of a part: before stage 3 its corner plus half the object's size for the part, else
 * the main building's corner + (3, 11); here into locals (the original writes its caller's
 * frame) */
static void obelisk_entry(game_state *st, uint32_t obj, int32_t id, int32_t *x, int32_t *y)
{
    uint32_t b = B(id);
    if (S32(st, b + 0x58) < 3) {
        int32_t half = (int32_t) P32(st, obj + 0x54 + 4u * (uint32_t) (13 * S16(st, b + 0x12))) / 2;
        *x = S16(st, b + 6) + half;
        *y = S16(st, b + 8) + half;
        return;
    }
    uint32_t m = B(id - S16(st, b + 0x12));
    *x = S16(st, m + 6) + 3;
    *y = S16(st, m + 8) + 0xB;
}

static int obelisk_all_raised(game_state *st, int32_t main_id)
{
    for (int32_t b = main_id; b < main_id + 0x19; b++) {
        if (S32(st, B(b) + 0x58) < 2) {
            return 0;
        }
    }
    return 1;
}

void obelisk_build(game_state *st, uint32_t obj, int32_t id, int32_t f, int32_t action)
{
    /* @bin 0x0040F0D0 (thiscall on the obelisk object) (id, figure, action) once the 25 parts
     * are at stage 2: the work list (0x004207E0). Before: a builder (0x55) on a part below stage
     * 2 counts +0x36 past 100, then the part goes up a stage (action 8, method +0x18 redraws it);
     * when all are at stage 2, 0x0040ECF0 and each part gets its entry (method +0x20 into +0x26,
     * +0x28) and is redrawn. Other figures: the action. */
    if (obelisk_all_raised(st, id - PART(id))) {
        obelisk_work(st, id, f, action);
        return;
    }
    uint32_t fa = F(f);
    int32_t v = S32(st, B(id) + 0x58);
    if (U8(st, fa + 0x0A) != 0x55 || v >= 2) {
        U8(st, fa + 0x38) = (uint8_t) action;
        return;
    }
    S16(st, fa + 0x36) = (int16_t) (S16(st, fa + 0x36) + 1);
    if (S16(st, fa + 0x36) <= 100) {
        return;
    }
    S16(st, fa + 0x36) = 0;
    U8(st, fa + 0x38) = 8;
    S32(st, B(id) + 0x58) = v + 1;
    GAME_CALLT(st, obj, U32(st, U32(st, obj) + 0x18), id);
    int32_t main_id = id - PART(id);
    if (!obelisk_all_raised(st, main_id)) {
        return;
    }
    obelisk_stage_up(st, obj, main_id);
    for (int32_t b = main_id; b < main_id + 0x19; b++) {
        int32_t x, y;
        if (U32(st, U32(st, obj) + 0x20) != 0x0040EB90u) {
            state_unported(st, 0x0040F0D0u); /* another method +0x20: its outputs live in this frame */
            return;
        }
        obelisk_entry(st, obj, b, &x, &y);
        S16(st, B(b) + 0x26) = (int16_t) x;
        S16(st, B(b) + 0x28) = (int16_t) y;
        GAME_CALLT(st, obj, U32(st, U32(st, obj) + 0x18), b);
    }
}

void pyramid_ramp_build(game_state *st, int32_t id, int32_t f, int32_t action)
{
    /* @bin 0x004EEF10 (pyramid, figure, action) a kind with a ramp (0x004ECEA0): the placement
     * preview (0x004ED360, 3) gives a tile 0x00EAAE90; a figure standing on it (+0x28) counts
     * +0x36 to 200, then the building of kind 0x00EAAE8C is made there (0x0046A400, 0x00470990
     * (.., 2, 1, 0)), canals redrawn, the two routing grids filled over it (0x00516F90,
     * 0x005173A0), the action set, and 0x00470990 (.., 2, 0, 0); elsewhere: the action */
    uint32_t kind = U8(st, B(id) + 0x54), fa = F(f);
    if (!(uint8_t) GAME_CALL(st, 0x004ECEA0u, (int32_t) kind)) {
        U8(st, fa + 0x38) = (uint8_t) action;
        return;
    }
    int32_t r = S32(st, 0x005F5D08u + 4u * kind) + S32(st, B(id) + 0x0C) - S32(st, 0x00BCA2D8u);
    GAME_CALL(st, 0x004ED360u, r % 228, r / 228, 3, 0);
    int32_t t = S32(st, 0x00EAAE90u);
    if (t != S32(st, fa + 0x28) || t == 0) {
        U8(st, fa + 0x38) = (uint8_t) action;
        return;
    }
    S16(st, fa + 0x36) = (int16_t) (S16(st, fa + 0x36) + 1);
    if (S16(st, fa + 0x36) < 0xC8) {
        return;
    }
    int32_t d = t - S32(st, 0x00BCA2D8u), x = d % 228, y = d / 228;
    int32_t b = GAME_CALL(st, 0x0046A400u, S32(st, 0x00EAAC28u), S32(st, 0x00EAAE8Cu), x, y, 0, 0);
    GAME_CALL(st, 0x00470990u, S32(st, 0x00EAAC28u), S32(st, 0x00EAAE8Cu), b, x, y, 2, 1, 0);
    GAME_CALL(st, 0x0047BAE0u, 0, 0, S32(st, 0x00BCA2D0u) - 1, S32(st, 0x00BCA2D4u) - 1, 0);
    int32_t bx = S16(st, B(b) + 6), by = S16(st, B(b) + 8), sz = U8(st, B(b) + 3);
    GAME_CALL(st, 0x00516F90u, bx - sz, by - sz, bx + sz, by + sz, 1);
    bx = S16(st, B(b) + 6), by = S16(st, B(b) + 8), sz = U8(st, B(b) + 3);
    GAME_CALL(st, 0x005173A0u, bx - sz, by - sz, bx + sz, by + sz);
    U8(st, fa + 0x38) = (uint8_t) action;
    GAME_CALL(st, 0x00470990u, S32(st, 0x00EAAC28u), S32(st, 0x00EAAE8Cu), b, x, y, 2, 0, 0);
}

static int32_t dist2(game_state *st, int32_t tx, int32_t ty, int32_t b)
{
    int32_t dx = tx - S16(st, B(b) + 6), dy = ty - S16(st, B(b) + 8);
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return dy * dy + dx * dx;
}

void figure_pyramid_choose(game_state *st, int32_t f, int32_t reset_level, int32_t idle_action, int32_t busy_action,
                           int32_t go_action)
{
    /* @bin 0x004F0EF0 (figure, reset, idle, busy, go actions) at the pyramid of its tile (+0x20):
     * the parts in progress (0x004EF450; none: idle) at the figure's level +0x50 (reset: := the
     * first part's level; another level: idle); the target is the level's part of the kind's list;
     * among the parts in progress (climate 2: the 0x50 figures take the unfinished ones
     * (0x004EFD90), the others the finished ones), the farthest from the target (twice the
     * squared distance, the linked part's when farther: 0x004F27B0) whose worker +0x64 is free
     * or the figure, ties to one whose linked part has a worker (0x004F28E0) or that has a linked
     * part; a busy worker elsewhere (0x004EEA10): idle; a finished part (+0x55 == 1): its worker
     * cleared, busy action; else the figure works it (+0x64, +0x120 := 4, go action, 0x004F02F0
     * (figure, part, +0x4C, 0)) */
    uint32_t fa = F(f);
    int32_t b = S16(st, 0x00B4E6FCu + 2u * U32(st, fa + 0x20));
    if (TYPE(b) != 0xB7) {
        goto idle;
    }
    int32_t main_id = b - PART(b);
    GAME_CALL(st, 0x004EF450u, main_id);
    if (S32(st, 0x00EAAF58u) < 1) {
        goto idle;
    }
    int32_t first = S32(st, 0x00EAAC44u), mainf = first - PART(first);
    int32_t level = U8(st, B(first) + 0x56) + U8(st, B(first) + 0x57) * 6;
    if ((uint8_t) reset_level) {
        S16(st, fa + 0x50) = (int16_t) level;
    }
    if (level != S16(st, fa + 0x50)) {
        goto idle;
    }
    int32_t count;
    uint32_t tab;
    if (S32(st, CLIMATE) == 5) {
        count = 0x10;
        tab = 0x005F5498u + 0x80u * U8(st, B(main_id) + 0x54);
    } else {
        count = 0x3C;
        tab = 0x005F4B38u + 0x1E0u * U8(st, B(main_id) + 0x54);
    }
    int32_t target = first;
    for (int32_t i = 0; i < count; i++) {
        target = mainf + S32(st, tab + 8u * (uint32_t) i);
        if (S32(st, tab + 8u * (uint32_t) i + 4) >= level) {
            break;
        }
    }
    int32_t tx = S16(st, B(target) + 6), ty = S16(st, B(target) + 8);
    int32_t best = 0, best_dist = 0;
    int best_pos = 0, best_linked = 0;
    for (int32_t i = 0; i < S32(st, 0x00EAAF58u); i++) {
        int32_t q = S32(st, 0x00EAAC44u + 4u * (uint32_t) i);
        uint8_t unfinished = (uint8_t) GAME_CALL(st, 0x004EFD90u, q);
        int32_t climate = S32(st, CLIMATE);
        if (climate < 1 || climate > 5) {
            continue;
        }
        if (climate == 2 && (U8(st, fa + 0x0A) == 0x50 ? unfinished == 0 : unfinished != 0)) {
            continue;
        }
        int32_t d = dist2(st, tx, ty, q);
        uint8_t below;
        int32_t r = pyramid_linked_part(st, q, &below);
        int linked;
        if (r > 0) {
            int32_t d2 = dist2(st, tx, ty, r);
            d = (d <= d2 ? d2 : d) * 2;
            linked = 1;
        } else {
            d *= 2;
            linked = 0;
        }
        int pos = pyramid_linked_worker(st, q) > 0;
        int take = 0;
        if (best == 0) {
            take = 1;
        } else {
            int16_t aw = S16(st, B(best) + 0x64), qw = S16(st, B(q) + 0x64);
            int check_q = 1;
            if (aw > 0) {
                if (aw != f && (qw == 0 || qw == f)) {
                    take = 1;
                }
                if (!take && aw != f) {
                    check_q = 0; /* @bin 0x004F11C8 straight to the distance */
                }
            }
            if (!take && check_q && qw != 0 && qw != f) {
                continue;
            }
            if (!take) {
                if (d > best_dist) {
                    take = 1;
                } else if (d == best_dist && ((pos && !best_pos) || (linked && !best_linked))) {
                    take = 1;
                }
            }
        }
        if (take) {
            best_pos = pos;
            best_linked = linked;
            best = q;
            best_dist = d;
        }
    }
    if (best == 0) {
        goto idle;
    }
    int16_t w = S16(st, B(best) + 0x64);
    if (w != 0 && w != f && (uint8_t) pyramid_worker_busy(st, best)) {
        goto idle;
    }
    if (U8(st, B(best) + 0x55) == 1) {
        S16(st, B(best) + 0x64) = 0;
        U8(st, fa + 0x38) = (uint8_t) busy_action;
        return;
    }
    S16(st, B(best) + 0x64) = (int16_t) f;
    S16(st, fa + 0x120) = 4;
    U8(st, fa + 0x38) = (uint8_t) go_action;
    figure_pyramid_target(st, f, best, S16(st, fa + 0x4C), 0);
    return;
idle:
    U8(st, fa + 0x38) = (uint8_t) idle_action;
    S16(st, fa + 0x120) = 0;
}

void figure_pyramid_part_work(game_state *st, int32_t id, int32_t f)
{
    /* @bin 0x004EEB60 (part, figure) a worker at a pyramid part (actions by type: 0x51 idle 0xC,
     * busy 0x10, go 0xF; 0x50: 9, 9, 0xF; others 0): no work left (+0xC8 of the main building):
     * idle. Another busy worker there: elsewhere (0x004F0EF0 (figure, 1, ..)) or, on a finished
     * part, on to the next (0x004F2190, 0x004F02F0 (.., 0)). Else +0x36 counts to the stage's
     * time (stage 0: 0x005E4EE8 + 0x005E4EEC, else 0x005E4EF0); then the part's course +0x56 goes
     * up (stage 0: a full course of 6, 3 or 1 by climate moves to the next level +0x57 up to
     * +0x58, or finishes the part) or down (marks +0x59 / +0x5A above it cleared), the part is
     * redrawn (0x004EDF50), no work left: 0x004F5E70 (main, 0) and in climate 3 the marks of
     * every part cleared and redrawn; else a finished part of a started pyramid sends the figure
     * on (busy action, 0x004F02F0 (.., 1)), or 0x004F0EF0 (figure, 0, ..). */
    uint32_t fa = F(f), b = B(id);
    int32_t idle, busy, go;
    if (U8(st, fa + 0x0A) == 0x51) {
        idle = 0xC, busy = 0x10, go = 0xF;
    } else if (U8(st, fa + 0x0A) == 0x50) {
        idle = 9, busy = 9, go = 0xF;
    } else {
        idle = busy = go = 0;
    }
    int32_t main_id = id - PART(id);
    uint32_t mb = B(main_id), kind = U8(st, mb + 0x54);
    if (S16(st, mb + 0xC8) < 1) {
        goto idle;
    }
    if (S16(st, b + 0x64) != f && (uint8_t) pyramid_worker_busy(st, id)) {
        if (busy == idle) {
            goto idle;
        }
        if (U8(st, b + 0x55) != 1) {
            figure_pyramid_choose(st, f, 1, idle, busy, go);
            return;
        }
        U8(st, fa + 0x38) = (uint8_t) busy;
        figure_pyramid_target(st, f, pyramid_next_part(st, S16(st, fa + 0x4E)), S16(st, fa + 0x50), 0);
        return;
    }
    int32_t lim = U8(st, b + 0x55) == 0 ? S32(st, 0x005E4EE8u) + S32(st, 0x005E4EECu) : S32(st, 0x005E4EF0u);
    S16(st, fa + 0x36) = (int16_t) (S16(st, fa + 0x36) + 1);
    if (S16(st, fa + 0x36) < lim) {
        return;
    }
    S16(st, b + 0x64) = 0;
    S16(st, mb + 0xC8) = (int16_t) (S16(st, mb + 0xC8) - 1);
    S16(st, fa + 0x36) = 0;
    if (U8(st, b + 0x55) == 0) {
        uint8_t v = (uint8_t) (U8(st, b + 0x56) + 1);
        U8(st, b + 0x56) = v;
        int32_t full;
        if (S32(st, CLIMATE) == 1) {
            full = U8(st, 0x005E68D8u + kind * 6u + U8(st, b + 0x57)) != 0 ? 3 : 6;
        } else if (S32(st, CLIMATE) == 5 && main_id == id) {
            full = 1;
        } else {
            full = 6;
        }
        if ((int32_t) v >= full) {
            if (U8(st, b + 0x57) < U8(st, b + 0x58)) {
                if ((int32_t) v > full) {
                    U8(st, b + 0x57) = (uint8_t) (U8(st, b + 0x57) + 1);
                    U8(st, b + 0x56) = 1;
                }
            } else {
                U8(st, b + 0x55) = 1;
                U8(st, b + 0x56) = (uint8_t) (full < 1 ? 1 : full);
            }
        }
    } else {
        U8(st, b + 0x56) = (uint8_t) (U8(st, b + 0x56) - 1);
        if (U8(st, b + 0x59) > U8(st, b + 0x56)) {
            U8(st, b + 0x59) = 0;
        }
        if (U8(st, b + 0x5A) > U8(st, b + 0x56)) {
            U8(st, b + 0x5A) = 0;
        }
    }
    int32_t half = S32(st, 0x00BCA2E0u) / 2;
    GAME_CALL(st, 0x004EDF50u, id, half, (int32_t) kind);
    if (S16(st, mb + 0xC8) == 0) {
        GAME_CALL(st, 0x004F5E70u, main_id, 0);
    }
    if (S32(st, CLIMATE) == 3 && S16(st, mb + 0xC8) == 0) {
        for (int32_t i = 0;; i++) {
            int32_t n = S32(st, CLIMATE) == 5 ? S32(st, 0x0056F7ECu + 4u * kind) : S32(st, 0x0056F7D8u + 4u * kind);
            if (i >= n) {
                return;
            }
            uint32_t pb = B(main_id + i);
            if (U8(st, pb + 0x59) != 0 || U8(st, pb + 0x5A) != 0) {
                U8(st, pb + 0x59) = 0;
                U8(st, pb + 0x5A) = 0;
                GAME_CALL(st, 0x004EDF50u, main_id + i, half, (int32_t) kind);
            }
        }
    }
    if (U8(st, b + 0x55) == 1 && U8(st, B(id - PART(id)) + 0x55) >= 1) {
        if (busy == 0) {
            goto idle;
        }
        U8(st, fa + 0x38) = (uint8_t) busy;
        figure_pyramid_target(st, f, pyramid_next_part(st, S16(st, fa + 0x4E)), S16(st, fa + 0x50), 1);
        return;
    }
    figure_pyramid_choose(st, f, 0, idle, busy, go);
    return;
idle:
    U8(st, fa + 0x38) = (uint8_t) idle;
    S16(st, fa + 0x120) = 0;
}

void mastaba_work(game_state *st, int32_t id, int32_t f, int32_t action)
{
    /* @bin 0x004E0050 (part, figure, action) a mastaba worker (0x55 builder, 0x4F bricks, 0x51
     * stones; others: the action): +0x36 counts to 50 (from 0 if negative for 0x4F / 0x51),
     * then the work is done, the part's figure +0x5C cleared and the action set. Builder: the
     * part's stage +0x58 0 or 1 goes up (kinds 10 and 12 raise the next part too), parts redrawn
     * (0x004DFBB0); all working parts (table 0x005E2AE4, kind >= 4) started: main flags +0x64 |= 4,
     * all at stage 2: |= 8; another stage is clamped to 0..2 and the figure released. Bricks: the
     * corner's flag (0x10, 0x20, 0x40, 0x80 by the part's corner 0xF00) joins the main flags.
     * Stones: the next free stone bit of the corner part's +0x58 (by 0xF000), redrawn; the four
     * corners full (+0x58 >= 0xFFFF): flags |= 2, all low bytes 0xFF: |= 1. */
    uint32_t b = B(id), fa = F(f);
    int16_t part = PART(id);
    uint32_t entry = U32(st, 0x005E2AE4u + 0x34u * (uint32_t) (int32_t) part);
    uint32_t corner = entry & 0xF00;
    uint8_t type = U8(st, fa + 0x0A);
    if (type == 0x55) {
        int32_t w = S32(st, b + 0x58);
        if (w != 0 && w != 1) {
            if (w < 0) {
                S32(st, b + 0x58) = 0;
            } else if (w > 2) {
                S32(st, b + 0x58) = 2;
            }
            S32(st, b + 0x5C) = 0;
            U8(st, fa + 0x38) = (uint8_t) action;
            return;
        }
        S16(st, fa + 0x36) = (int16_t) (S16(st, fa + 0x36) + 1);
        if (S16(st, fa + 0x36) < 0x32) {
            return;
        }
        S32(st, b + 0x58) = w + 1;
        uint32_t k = entry & 0xFF;
        if (k == 10 || k == 12) {
            S32(st, B(id + 1) + 0x58) = w + 1;
            GAME_CALL(st, 0x004DFBB0u, id + 1);
        }
        GAME_CALL(st, 0x004DFBB0u, id);
        int32_t main_id = id - PART(id);
        int started = 1, raised = 1;
        for (uint32_t i = 0; 0x005E2AE4u + 0x34u * i < 0x005E4274u; i++) {
            if ((int32_t) (U32(st, 0x005E2AE4u + 0x34u * i) & 0xFF) >= 4) {
                int32_t v = S32(st, B(main_id + (int32_t) i) + 0x58);
                if (v == 0) {
                    started = 0;
                }
                if (v < 2) {
                    raised = 0;
                }
            }
        }
        if (started) {
            U32(st, B(main_id) + 0x64) |= 4;
        }
        if (raised) {
            U32(st, B(main_id) + 0x64) |= 8;
        }
        goto done;
    }
    int bricks;
    if (type == 0x4F) {
        bricks = 1;
    } else if (type == 0x51) {
        bricks = 0;
    } else {
        U8(st, fa + 0x38) = (uint8_t) action;
        return;
    }
    uint32_t flag;
    int32_t target;
    switch (corner) {
    case 0x100: flag = 0x10; target = id - part; break;
    case 0x200: flag = 0x20; target = id - part + 1; break;
    case 0x400: flag = 0x40; target = id - part + 2; break;
    case 0x800: flag = 0x80; target = id - part + 3; break;
    default: goto release;
    }
    if (target == 0) {
        goto release;
    }
    uint32_t stones = U32(st, B(target) + 0x58), bit = 0;
    static const uint32_t ORDER[4][4] = {
        {1, 2, 0x100, 0x200}, {4, 8, 0x400, 0x800}, {0x10, 0x20, 0x1000, 0x2000}, {0x40, 0x80, 0x4000, 0x8000}};
    int row = -1;
    switch (entry & 0xF000) {
    case 0x1000: row = 0; break;
    case 0x2000: row = 1; break;
    case 0x4000: row = 2; break;
    case 0x8000: row = 3; break;
    default: break;
    }
    if (row >= 0) {
        for (int i = 0; i < 4; i++) {
            if (!(stones & ORDER[row][i])) {
                bit = ORDER[row][i];
                break;
            }
        }
    }
    if (bricks) {
        if (flag == 0) {
            goto release;
        }
    } else if (bit == 0) {
        goto release;
    }
    if (S16(st, fa + 0x36) < 0) {
        S16(st, fa + 0x36) = 0;
    }
    S16(st, fa + 0x36) = (int16_t) (S16(st, fa + 0x36) + 1);
    if (S16(st, fa + 0x36) < 0x32) {
        return;
    }
    if (bricks) {
        S32(st, b + 0x5C) = 0;
        U8(st, fa + 0x38) = (uint8_t) action;
        U32(st, B(id - PART(id)) + 0x64) |= flag;
        GAME_CALL(st, 0x004DFBB0u, target);
        return;
    }
    U32(st, B(target) + 0x58) = stones | bit;
    GAME_CALL(st, 0x004DFBB0u, target);
    {
        int full = 1, solid = 1;
        int32_t main_id = id - PART(id);
        for (int32_t k = 0; k < 4; k++) {
            int32_t v = S32(st, B(main_id + k) + 0x58);
            if (v < 0xFFFF) {
                full = 0;
            }
            if ((uint8_t) v != 0xFF) {
                solid = 0;
            }
        }
        if (full) {
            U32(st, B(id - PART(id)) + 0x64) |= 2;
        }
        if (solid) {
            U32(st, B(id - PART(id)) + 0x64) |= 1;
        }
    }
done:
    S32(st, b + 0x5C) = 0;
    U8(st, fa + 0x38) = (uint8_t) action;
    return;
release:
    S32(st, b + 0x5C) = 0;
    U8(st, fa + 0x38) = (uint8_t) action;
}

void sun_temple_check(game_state *st, int32_t id)
{
    /* @bin 0x004200F0 (id) no entry of 0x00596080 for the stage +0x58 with work left:
     * 0x00542DD0 (id) */
    if (!anim_key_left(st, 0x00596080u, 0x00596150u, S32(st, B(id) + 0x58))) {
        GAME_CALL(st, 0x00542DD0u, id);
    }
}

void sun_temple_list_work(game_state *st, int32_t id, int32_t f, int32_t action)
{
    /* @bin 0x00420240 (id, figure, action) the work list 0x00596080 for the stage +0x58: the
     * entry holding the figure (another stage: the action), else a free one; one unit of work;
     * none left: 0x004200F0 and the action */
    uint32_t fa = F(f);
    int32_t key = S32(st, B(id) + 0x58);
    int done = 0;
    U8(st, fa + 0x65) = 1;
    uint32_t e = anim_held_entry(st, 0x00596080u, 0x00596150u, key, f, &done);
    if (e == 0 && !done) {
        e = anim_free_entry(st, 0x00596080u, 0x00596150u, key, f);
    }
    if (e == 0) {
        U8(st, fa + 0x38) = (uint8_t) action;
        return;
    }
    if (anim_work(st, e, f)) {
        sun_temple_check(st, id);
        U8(st, fa + 0x38) = (uint8_t) action;
    }
}

void sun_temple_work(game_state *st, int32_t id, int32_t f, int32_t action)
{
    /* @bin 0x00541DE0 (part, figure, action) by the part's kind (table 0x00601DE4, 13 dwords per
     * part): 0: the work list (0x00420240); 1..0xC: +0x36 counts to 50 (kind 1 from 0 if
     * negative), then the part's stage +0x58 goes up, it is redrawn (0x00541920), the figure
     * released (+0x5C := 0, action) and the main flags +0x64 updated: kind 1 at stage 2: |= 4;
     * kinds 2..7 at stage 2 when no part of those kinds is below 2 among the 100: |= 8; kinds
     * 8..0xB: all such parts started |= 2, all at 2 |= 0x10; kind 0xC at stage 4: |= 0x20 */
    uint32_t b = B(id), fa = F(f);
    uint32_t kind = U32(st, 0x00601DE4u + 0x34u * (uint32_t) (int32_t) PART(id));
    if (kind > 0xC) {
        return;
    }
    if (kind == 0) {
        sun_temple_list_work(st, id, f, action);
        return;
    }
    int32_t w = S32(st, b + 0x58);
    if (kind == 1 && S16(st, fa + 0x36) < 0) {
        S16(st, fa + 0x36) = 0;
    }
    S16(st, fa + 0x36) = (int16_t) (S16(st, fa + 0x36) + 1);
    if (S16(st, fa + 0x36) < 0x32) {
        return;
    }
    S32(st, b + 0x58) = w + 1;
    GAME_CALL(st, 0x00541920u, id);
    int32_t main_id = id - PART(id);
    uint32_t flags = B(main_id) + 0x64;
    if (kind == 1) {
        if (S32(st, b + 0x58) == 2) {
            U32(st, flags) |= 4;
        }
    } else if (kind <= 7) {
        if (S32(st, b + 0x58) == 2) {
            int32_t i;
            for (i = 0; i < 100; i++) {
                uint32_t k = U32(st, 0x00601DE4u + 0x34u * (uint32_t) i);
                if (k >= 2 && k <= 7 && S32(st, B(main_id + i) + 0x58) < 2) {
                    break;
                }
            }
            if (i == 100) {
                U32(st, flags) |= 8;
            }
        }
    } else if (kind <= 0xB) {
        int started = 1, raised = 1;
        for (int32_t i = 0; i < 100; i++) {
            uint32_t k = U32(st, 0x00601DE4u + 0x34u * (uint32_t) i);
            if (k >= 8 && k <= 0xB) {
                int32_t v = S32(st, B(main_id + i) + 0x58);
                if (v == 0) {
                    started = 0;
                }
                if (v < 2) {
                    raised = 0;
                }
            }
        }
        if (started) {
            U32(st, flags) |= 2;
        }
        if (raised) {
            U32(st, flags) |= 0x10;
        }
    } else if (S32(st, b + 0x58) == 4) {
        U32(st, flags) |= 0x20;
    }
    S32(st, b + 0x5C) = 0;
    U8(st, fa + 0x38) = (uint8_t) action;
}

/* ---- registration -------------------------------------------------------------------------- */

#define THIS(a) ((uint32_t) (a)[PORT_THIS_ARG])

static int32_t run_take_worker(game_state *st, const int32_t *a) { return monument_take_worker(st, a[0], a[1], a[2]); }
static int32_t run_pyramid_take(game_state *st, const int32_t *a) { return pyramid_take_worker(st, a[0], a[1], a[2]); }
static int32_t run_part_ready(game_state *st, const int32_t *a) { return pyramid_part_ready(st, a[0]); }
static int32_t run_worker_busy(game_state *st, const int32_t *a) { return pyramid_worker_busy(st, a[0]); }
static int32_t run_send_figure(game_state *st, const int32_t *a) { return pyramid_send_figure(st, a[0], a[1]); }
static int32_t run_420350(game_state *st, const int32_t *a) { return monument_0x00420350(st, a[0], a[1], a[2]); }
static int32_t run_stage_count(game_state *st, const int32_t *a) { return monument_stage_count(st, a[0]); }
static int32_t run_4e5c00(game_state *st, const int32_t *a) { return monument_0x004E5C00(st, a[0], a[1], a[2]); }
static int32_t run_4178e0(game_state *st, const int32_t *a) { return take_by_method(st, THIS(a), 0xE3, a[2]); }
static int32_t run_4e6e10(game_state *st, const int32_t *a) { return take_by_method(st, THIS(a), 0xE4, a[2]); }
static int32_t run_40f070(game_state *st, const int32_t *a) { return take_by_method(st, THIS(a), 0xE1, a[2]); }
static int32_t run_542a80(game_state *st, const int32_t *a) { return take_by_call(st, 0x005421D0u, 0xD5, a[2]); }
static int32_t run_4176a0(game_state *st, const int32_t *a) { return take_part(st, THIS(a), a[0], a[1], a[2], 0); }
static int32_t run_4e6c10(game_state *st, const int32_t *a) { return take_part(st, THIS(a), a[0], a[1], a[2], 1); }
static int32_t run_40ed70(game_state *st, const int32_t *a) { return obelisk_take(st, THIS(a), a[0], a[1], a[2]); }
static int32_t run_busy_e3(game_state *st, const int32_t *a) { return part_busy(st, a[0], 0xE3); }
static int32_t run_busy_e4(game_state *st, const int32_t *a) { return part_busy(st, a[0], 0xE4); }
static int32_t run_busy_e1(game_state *st, const int32_t *a) { return part_busy(st, a[0], 0xE1); }
static int32_t run_job_e3(game_state *st, const int32_t *a) { return stage_job(st, a[0], a[1], 0x005AD218u, 0x005AD3C8u, 0); }
static int32_t run_job_e4(game_state *st, const int32_t *a) { return stage_job(st, a[0], a[1], 0x005B0060u, 0x005B0150u, 0); }
static int32_t run_job_e1(game_state *st, const int32_t *a) { return stage_job(st, a[0], a[1], 0x0059EDF0u, 0x0059EFA0u, 1); }
static int32_t run_pyramids_update(game_state *st, const int32_t *a) { (void) a; pyramids_update(st); return 0; }
static int32_t run_pyramid_assign(game_state *st, const int32_t *a) { return pyramid_assign(st, a[0], a[1], a[2], a[3]); }
static int32_t run_4f0190(game_state *st, const int32_t *a) { return S32(st, CLIMATE) == 5 ? 0 : pyramid_assign(st, a[0], a[1], a[2], 1); }
static int32_t run_parts_sort(game_state *st, const int32_t *a) { (void) a; pyramid_parts_sort(st); return 0; }
static int32_t run_545be0(game_state *st, const int32_t *a) { return take_by_547c10(st, THIS(a), 0xE5, a[2]); }
static int32_t run_5590c0(game_state *st, const int32_t *a) { return take_by_547c10(st, THIS(a), 0xEA, a[2]); }
static int32_t run_556430(game_state *st, const int32_t *a) { return take_by_547c10(st, THIS(a), 0xEB, a[2]); }
static int32_t run_54fb40(game_state *st, const int32_t *a) { return take_by_547c10(st, THIS(a), 0xEC, a[2]); }
static int32_t run_pyr_target(game_state *st, const int32_t *a) { figure_pyramid_target(st, a[0], a[1], (int16_t) a[2], a[3]); return 0; }
static int32_t run_next_part(game_state *st, const int32_t *a) { return pyramid_next_part(st, a[0]); }
static int32_t run_pyr_step(game_state *st, const int32_t *a) { return figure_pyramid_step(st, a[0], (int16_t) a[1]); }
static int32_t run_pyr_work(game_state *st, const int32_t *a) { figure_pyramid_work(st, a[0], a[1], (uint8_t) a[2], (uint8_t) a[3]); return 0; }
static int32_t run_linked_part(game_state *st, const int32_t *a)
{
    uint8_t *out = state_ptr(st, (uint32_t) a[1], 1);
    uint8_t below;
    int32_t r = pyramid_linked_part(st, a[0], &below);
    if (!out) {
        state_unported(st, 0x004F27B0u); /* the flag lives in the caller's frame */
        return r;
    }
    *out = below;
    return r;
}
static int32_t run_has_below(game_state *st, const int32_t *a) { return pyramid_has_below(st, a[0]); }
static int32_t run_linked_worker(game_state *st, const int32_t *a) { return pyramid_linked_worker(st, a[0]); }
static int32_t run_waiting_worker(game_state *st, const int32_t *a) { return pyramid_waiting_worker(st, a[0], a[1]); }
static int32_t run_part_advance(game_state *st, const int32_t *a) { pyramid_part_advance(st, a[0]); return 0; }
static int32_t run_mason_free(game_state *st, const int32_t *a) { return pyramid_mason_free(st, a[0]); }
static int32_t run_pyr_block(game_state *st, const int32_t *a) { return figure_pyramid_block(st, a[0]); }
static int32_t run_tile_work(game_state *st, const int32_t *a) { return pyramid_tile_work(st, a[0]); }
static int32_t run_pyr_lay(game_state *st, const int32_t *a) { return figure_pyramid_lay(st, a[0]); }
static int32_t run_builder_pair(game_state *st, const int32_t *a) { pyramid_builder_pair(st, a[0]); return 0; }
static int32_t run_anim_fig(game_state *st, const int32_t *a) { anim_object_figure(st, THIS(a), a[0]); return 0; }
static int32_t run_city_check(game_state *st, const int32_t *a) { (void) a; city_monument_check(st); return 0; }
static int32_t run_city_work(game_state *st, const int32_t *a) { city_monument_work(st, a[0], a[1], (uint8_t) a[2]); return 0; }
static int32_t run_e6_done(game_state *st, const int32_t *a) { monument_e6_done(st, a[0], a[1]); return 0; }
static int32_t run_e6_work(game_state *st, const int32_t *a) { monument_e6_work(st, a[0], a[1], (uint8_t) a[2]); return 0; }
static int32_t run_e3_done(game_state *st, const int32_t *a) { stage_done(st, a[0], 0x005AD210u, 0x005AD3C0u, 0x006FD368u, 0x00417640u); return 0; }
static int32_t run_e4_done(game_state *st, const int32_t *a) { stage_done(st, a[0], 0x005B0058u, 0x005B0148u, 0x006DE548u, 0x004E6BD0u); return 0; }
static int32_t run_e3_work(game_state *st, const int32_t *a) { stage_work(st, a[0], a[1], (uint8_t) a[2], 1); return 0; }
static int32_t run_e4_work(game_state *st, const int32_t *a) { stage_work(st, a[0], a[1], (uint8_t) a[2], 0); return 0; }
static int32_t run_ob_up(game_state *st, const int32_t *a) { obelisk_stage_up(st, THIS(a), a[0]); return 0; }
static int32_t run_ob_check(game_state *st, const int32_t *a) { obelisk_stage_check(st, a[0]); return 0; }
static int32_t run_ob_work(game_state *st, const int32_t *a) { obelisk_work(st, a[0], a[1], (uint8_t) a[2]); return 0; }
static int32_t run_stage_step(game_state *st, const int32_t *a) { return monument_stage_step(st, a[0]); }
static int32_t run_stage_adv(game_state *st, const int32_t *a) { return monument_stage_advance(st, a[0]); }
static int32_t run_list_work(game_state *st, const int32_t *a) { monument_list_work(st, a[0], a[1], (uint8_t) a[2]); return 0; }
static int32_t run_ob_build(game_state *st, const int32_t *a) { obelisk_build(st, THIS(a), a[0], a[1], (uint8_t) a[2]); return 0; }
static int32_t run_ramp_build(game_state *st, const int32_t *a) { pyramid_ramp_build(st, a[0], a[1], (uint8_t) a[2]); return 0; }
static int32_t run_pyr_choose(game_state *st, const int32_t *a) { figure_pyramid_choose(st, a[0], a[1], (uint8_t) a[2], (uint8_t) a[3], (uint8_t) a[4]); return 0; }
static int32_t run_part_work(game_state *st, const int32_t *a) { figure_pyramid_part_work(st, a[0], a[1]); return 0; }
static int32_t run_mastaba_work(game_state *st, const int32_t *a) { mastaba_work(st, a[0], a[1], (uint8_t) a[2]); return 0; }
static int32_t run_sun_check(game_state *st, const int32_t *a) { sun_temple_check(st, a[0]); return 0; }
static int32_t run_sun_list(game_state *st, const int32_t *a) { sun_temple_list_work(st, a[0], a[1], (uint8_t) a[2]); return 0; }
static int32_t run_sun_work(game_state *st, const int32_t *a) { sun_temple_work(st, a[0], a[1], (uint8_t) a[2]); return 0; }
static int32_t run_4e0c00(game_state *st, const int32_t *a) { return take_mastaba(st, a[2]); }
static int32_t run_mastaba_busy(game_state *st, const int32_t *a) { return mastaba_worker_busy(st, a[0]); }
static int32_t run_mastaba_take(game_state *st, const int32_t *a) { return mastaba_take(st, a[0], a[1], a[2]); }
static int32_t run_401ed0(game_state *st, const int32_t *a) { return monument_0x00401ED0(st, THIS(a), a[0], a[1], a[2], a[3]); }
static int32_t run_401f30(game_state *st, const int32_t *a) { return monument_0x00401ED0(st, THIS(a), a[0], a[1], a[2], 0); }

const ported_function bigb2_monument_ports[] = {
    {0x004BC140u, "monument_take_worker", 3, 32, 0, run_take_worker, 0},
    {0x004F3890u, "pyramids_update", 0, 0, 0, run_pyramids_update, 0},
    {0x004EFE30u, "pyramid_assign", 4, 32, 0, run_pyramid_assign, 0},
    {0x004F0190u, "pyramid_assign (mode 1)", 3, 32, 0, run_4f0190, 0},
    {0x004F25C0u, "pyramid_parts_sort", 0, 0, 0, run_parts_sort, 0},
    {0x004F02F0u, "figure_pyramid_target", 4, 0, 0, run_pyr_target, 0},
    {0x004F2190u, "pyramid_next_part", 1, 32, 0, run_next_part, 0},
    {0x004F1D10u, "figure_pyramid_step", 2, 8, 0, run_pyr_step, 0},
    {0x004F04D0u, "figure_pyramid_work", 4, 0, 0, run_pyr_work, 0},
    {0x004F27B0u, "pyramid_linked_part", 2, 32, 0, run_linked_part, 0},
    {0x004F28B0u, "pyramid_has_below", 1, 8, 0, run_has_below, 0},
    {0x004F28E0u, "pyramid_linked_worker", 1, 32, 0, run_linked_worker, 0},
    {0x004F29E0u, "pyramid_waiting_worker", 2, 32, 0, run_waiting_worker, 0},
    {0x004EF200u, "pyramid_part_advance", 1, 0, 0, run_part_advance, 0},
    {0x004F2BA0u, "pyramid_mason_free", 1, 8, 0, run_mason_free, 0},
    {0x004F2CF0u, "pyramid_builder_pair", 1, 0, 0, run_builder_pair, 0},
    {0x0041FD80u, "anim_object_figure", 1, 0, 0, run_anim_fig, 0},
    {0x004200B0u, "city_monument_check", 0, 0, 0, run_city_check, 0},
    {0x00420130u, "city_monument_work", 3, 0, 0, run_city_work, 0},
    {0x00420A90u, "monument_e6_done", 2, 0, 0, run_e6_done, 0},
    {0x00420B10u, "monument_e6_work", 3, 0, 0, run_e6_work, 0},
    {0x00420DC0u, "monument_e3_done", 1, 0, 0, run_e3_done, 0},
    {0x004210C0u, "monument_e4_done", 1, 0, 0, run_e4_done, 0},
    {0x00420E20u, "monument_e3_work", 3, 0, 0, run_e3_work, 0},
    {0x00421120u, "monument_e4_work", 3, 0, 0, run_e4_work, 0},
    {0x0040ECF0u, "obelisk_stage_up", 1, 0, 0, run_ob_up, 0},
    {0x00420770u, "obelisk_stage_check", 1, 0, 0, run_ob_check, 0},
    {0x004207E0u, "obelisk_work", 3, 0, 0, run_ob_work, 0},
    {0x0040F0D0u, "obelisk_build (method +0x30)", 3, 0, 0, run_ob_build, 0},
    {0x004EEF10u, "pyramid_ramp_build", 3, 0, 0, run_ramp_build, 0},
    {0x004F0EF0u, "figure_pyramid_choose", 5, 0, 0, run_pyr_choose, 0},
    {0x004EEB60u, "figure_pyramid_part_work", 2, 0, 0, run_part_work, 0},
    {0x004E0050u, "mastaba_work", 3, 0, 0, run_mastaba_work, 0},
    {0x004200F0u, "sun_temple_check", 1, 0, 0, run_sun_check, 0},
    {0x00420240u, "sun_temple_list_work", 3, 0, 0, run_sun_list, 0},
    {0x00541DE0u, "sun_temple_work", 3, 0, 0, run_sun_work, 0},
    {0x004E5AE0u, "monument_stage_step", 1, 8, 0, run_stage_step, 0},
    {0x004E5B20u, "monument_stage_advance", 1, 8, 0, run_stage_adv, 0},
    {0x004E5D10u, "monument_list_work", 3, 0, 0, run_list_work, 0},
    {0x004F4320u, "figure_pyramid_block", 1, 8, 0, run_pyr_block, 0},
    {0x004F4420u, "pyramid_tile_work", 1, 32, 0, run_tile_work, 0},
    {0x004F4440u, "figure_pyramid_lay", 1, 8, 0, run_pyr_lay, 0},
    {0x00545BE0u, "monument_take_0xe5", 3, 32, 0, run_545be0, 0},
    {0x005590C0u, "monument_take_0xea", 3, 32, 0, run_5590c0, 0},
    {0x00556430u, "monument_take_0xeb", 3, 32, 0, run_556430, 0},
    {0x0054FB40u, "monument_take_0xec", 3, 32, 0, run_54fb40, 0},
    {0x004F01E0u, "pyramid_take_worker", 3, 32, 0, run_pyramid_take, 0},
    {0x004EF100u, "pyramid_part_ready", 1, 32, 0, run_part_ready, 0},
    {0x004EEA10u, "pyramid_worker_busy", 1, 8, 0, run_worker_busy, 0},
    {0x004F3000u, "pyramid_send_figure", 2, 32, 0, run_send_figure, 0},
    {0x00420350u, "monument_0x00420350", 3, 32, 0, run_420350, 0},
    {0x004E5A30u, "monument_stage_count", 1, 32, 0, run_stage_count, 0},
    {0x004E5C00u, "monument_0x004E5C00", 3, 32, 0, run_4e5c00, 0},
    {0x004178E0u, "monument_take_0xe3", 3, 32, 0, run_4178e0, 0},
    {0x004E6E10u, "monument_take_0xe4", 3, 32, 0, run_4e6e10, 0},
    {0x0040F070u, "monument_take_0xe1", 3, 32, 0, run_40f070, 0},
    {0x00542A80u, "monument_take_0xd5", 3, 32, 0, run_542a80, 0},
    {0x004E0C00u, "monument_take_0xde", 3, 32, 0, run_4e0c00, 0},
    {0x00401ED0u, "monument_take_0xe6", 4, 32, 0, run_401ed0, 0},
    {0x00401F30u, "monument_take_0xe6 (0)", 3, 32, 0, run_401f30, 0},
    {0x004E0530u, "mastaba_worker_busy", 1, 8, 0, run_mastaba_busy, 0},
    {0x004176A0u, "monument_0xe3_take (method +0x2C)", 3, 8, 0, run_4176a0, 0},
    {0x004E6C10u, "monument_0xe4_take (method +0x2C)", 3, 8, 0, run_4e6c10, 0},
    {0x0040ED70u, "obelisk_take (method +0x2C)", 3, 8, 0, run_40ed70, 0},
    {0x00417590u, "monument_0xe3_busy (method +0x24)", 1, 8, 0, run_busy_e3, 0},
    {0x004E6B20u, "monument_0xe4_busy (method +0x24)", 1, 8, 0, run_busy_e4, 0},
    {0x0040EC20u, "obelisk_busy (method +0x24)", 1, 8, 0, run_busy_e1, 0},
    {0x00420F50u, "monument_0xe3_job", 2, 32, 0, run_job_e3, 0},
    {0x00421230u, "monument_0xe4_job", 2, 32, 0, run_job_e4, 0},
    {0x00420910u, "obelisk_job", 2, 32, 0, run_job_e1, 0},
    {0x004E05F0u, "mastaba_take", 3, 8, 0, run_mastaba_take, 0},
};
const int bigb2_monument_ports_count = sizeof(bigb2_monument_ports) / sizeof(bigb2_monument_ports[0]);
