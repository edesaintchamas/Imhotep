/* Imhotep - tombs: laying the stones of a part under construction (roadmap phase 8).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * A worker figure at a part of a royal burial tomb (building types E5, EA, EB, EC; their work-step
 * records are save chunk 180) calls the tomb object's 0x00547E20 (core/figure/actions_d.c,
 * actions_d3.c). The part's stone set (building +0x68) picks a table of three 16-byte slots in the
 * data segment (0x005BE508 ..): +0 the part's stage (building +0x60) the slot serves, +4 a figure
 * byte (+0x0A) it accepts, +8 a work-step record (core/city/day_anim.c) whose +0x12 is the figure
 * working it. Each stone laid takes one from the stage's counter (building +0x80 + 4 * stage);
 * below 1 the part's stage ends (0x004223F0) and, if no slot of the set still has work, the tomb
 * object advances the part (0x00547870). */
#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define B(id) BUILDING_ADDR(id)
#define F(id) FIGURE_ADDR(id)

typedef struct {
    uint32_t fn;    /* the template instance (0 : no function for this set) */
    uint32_t table; /* its slot table: three 16-byte slots */
} stone_set;

/* @bin 0x00548068 (jump table of 0x00547E20, 30 entries) and 0x004229EC (0x004223F0's, the same
 * sets): the stone set (building +0x68, 0..0x1D) -> the template instance and its slot table (the
 * instance's `mov eax, T + 8`, loops over T .. T + 0x30); sets 17, 18, 27, 28 have none */
static const stone_set stone_sets[30] = {
    {0x00423CB0u, 0x005BE5F8u}, /* 0 */
    {0x004233F0u, 0x005BE508u}, /* 1 */
    {0x00423530u, 0x005BE538u}, /* 2 */
    {0x00423670u, 0x005BE568u}, /* 3 */
    {0x004237B0u, 0x005BE748u}, /* 4 */
    {0x004238F0u, 0x005BE778u}, /* 5 */
    {0x00423DF0u, 0x005BE628u}, /* 6 */
    {0x00423F30u, 0x005BE7A8u}, /* 7 */
    {0x00424070u, 0x005BE7D8u}, /* 8 */
    {0x004241B0u, 0x005BE658u}, /* 9 */
    {0x004242F0u, 0x005BE688u}, /* 10 */
    {0x00424430u, 0x005BE898u}, /* 11 */
    {0x00424570u, 0x005BE808u}, /* 12 */
    {0x004246B0u, 0x005BE8C8u}, /* 13 */
    {0x004247F0u, 0x005BE8F8u}, /* 14 */
    {0x00423A30u, 0x005BE598u}, /* 15 */
    {0x00423B70u, 0x005BE5C8u}, /* 16 */
    {0, 0},                     /* 17 */
    {0, 0},                     /* 18 */
    {0x00424930u, 0x005BE6B8u}, /* 19 */
    {0x00424A70u, 0x005BE6E8u}, /* 20 */
    {0x00424BB0u, 0x005BE718u}, /* 21 */
    {0x00424CF0u, 0x005BE838u}, /* 22 */
    {0x00424E30u, 0x005BE868u}, /* 23 */
    {0x00424F70u, 0x005BE928u}, /* 24 */
    {0x004250B0u, 0x005BE958u}, /* 25 */
    {0x004251F0u, 0x005BE988u}, /* 26 */
    {0, 0},                     /* 27 */
    {0, 0},                     /* 28 */
    {0x004233F0u, 0x005BE508u}, /* 29 */
};

#define STONE_SLOTS 3 /* @bin 0x004233F0 loops end at T + 0x38 (by +8) and T + 0x30: 3 slots */

typedef struct {
    uint32_t obj;  /* the tomb object */
    uint32_t done; /* its stage-end wrapper */
    int32_t steps; /* the wrapper's count for 0x00547870 */
    uint32_t progress; /* its progress function (thiscall (part, 0)) */
} tomb_kind;

/* @bin 0x00422A64 (0x004223F0) and 0x00547A3C (0x00547870): building types E5, EA, EB, EC -> the
 * object, 0x00545BC0 / 0x005590A0 / 0x00556410 / 0x0054FB20 (push 0xA7 / 0x9E / 0x1B5 / 0x201),
 * 0x00545740 / 0x00558BE0 / 0x00555E80 / 0x0054F470; E6..E9 none */
static const tomb_kind *kind_of(int32_t type)
{
    static const tomb_kind kinds[4] = {
        {0x006F9320u, 0x00545BC0u, 0xA7, 0x00545740u},
        {0x006F5648u, 0x005590A0u, 0x9E, 0x00558BE0u},
        {0x006DE9E0u, 0x00556410u, 0x1B5, 0x00555E80u},
        {0x006E9188u, 0x0054FB20u, 0x201, 0x0054F470u},
    };
    switch (type) {
    case 0xE5: return &kinds[0];
    case 0xEA: return &kinds[1];
    case 0xEB: return &kinds[2];
    case 0xEC: return &kinds[3];
    default: return 0;
    }
}

void tomb_part_advance(game_state *st, uint32_t self, int32_t id, int32_t steps)
{
    /* @bin 0x00547870 (thiscall on the tomb object; part, count) the part's base is id - its kind
     * (+0x12). If its stage (+0x60) is below 4 (signed): stage += 1; on reaching 3, each of the six
     * neighbour links +0x6C.. that is > -1 (base-relative) gets stage += 1 and +0x64 := 1; then
     * +0x64 := 0 if the stage is 4; then the object's method +0x18 on base + i for i < count. Then,
     * if +0x64 is 0 and 0x004EA930 (al) is set: the base part's type (+0x10) picks the tomb's
     * progress function (thiscall (base, 0)); at 100, base +0x58 := 1 and 0x005468D0 (base) */
    uint32_t b = B(id);
    int32_t base = id - (int32_t) S16(st, b + 0x12);
    int32_t stage = S32(st, b + 0x60);
    if (stage < 4) {
        stage++;
        S32(st, b + 0x60) = stage;
        if (stage == 3) {
            for (int k = 0; k < 6; k++) {
                int32_t e = S32(st, b + 0x6Cu + 4u * (uint32_t) k);
                if (e > -1) {
                    uint32_t n = B(e + base);
                    U32(st, n + 0x60) = U32(st, n + 0x60) + 1u;
                    S32(st, n + 0x64) = 1;
                }
            }
        }
        if (S32(st, b + 0x60) == 4) {
            S32(st, b + 0x64) = 0;
        }
        for (int32_t i = 0; i < steps; i++) {
            GAME_CALLT(st, self, U32(st, U32(st, self) + 0x18u), i + base);
        }
    }
    if (S32(st, b + 0x64) != 0 || (uint8_t) GAME_CALL0(st, 0x004EA930u) == 0) {
        return;
    }
    base = id - (int32_t) S16(st, b + 0x12);
    uint32_t bb = B(base);
    const tomb_kind *k = kind_of(S16(st, bb + 0x10));
    if (!k) {
        return;
    }
    if (GAME_CALLT(st, k->obj, k->progress, base, 0) == 0x64) {
        U32(st, bb + 0x58) = 1;
        GAME_CALLT(st, k->obj, 0x005468D0u, base);
    }
}

void tomb_stage_end(game_state *st, int32_t id)
{
    /* @bin 0x004223F0 (part) the part's stone set (+0x68, unsigned <= 0x1D, with a table): if a
     * slot serves the part's stage (+0x60) and the stage's counter is still > 0 (signed), nothing;
     * otherwise the part's type (+0x10, E5 / EA / EB / EC) picks the tomb's stage-end wrapper, which
     * runs 0x00547870 (part, count) on its object */
    uint32_t b = B(id);
    uint32_t set = U32(st, b + 0x68);
    int32_t stage = S32(st, b + 0x60);
    if (set <= 0x1D && stone_sets[set].fn) {
        uint32_t t = stone_sets[set].table;
        for (int i = 0; i < STONE_SLOTS; i++, t += 0x10) {
            if (S32(st, t) == stage && S32(st, b + 0x80u + 4u * (uint32_t) stage) > 0) {
                return;
            }
        }
    }
    const tomb_kind *k = kind_of(S16(st, b + 0x10));
    if (k) {
        tomb_part_advance(st, k->obj, id, k->steps);
    }
}

void tomb_lay_stone(game_state *st, uint32_t table, int32_t id, int32_t fig, uint8_t action)
{
    /* @bin 0x004233F0 (template of 25 instances, table T; part, figure, action) figure +0x65 := 1.
     * The slot whose record's figure (+0x12, i16) is this one: if it serves another stage, figure
     * +0x38 := action and stop. Else the first free slot (record's figure 0) serving the part's
     * stage, for the figure's +0x0A (byte), while the stage's counter is > 0; none: +0x38 := action
     * and stop. A free slot is taken: the record's 0x0041FD80 (figure), then 0x0041FCA0 (0). Then
     * the counter -= 1; below 1, +0x5C := 0, the stage's end (0x004223F0) and +0x38 := action */
    uint32_t b = B(id);
    uint32_t f = F(fig);
    int32_t stage = S32(st, b + 0x60);
    uint32_t slot = 0;
    U8(st, f + 0x65) = 1;
    for (int i = 0; i < STONE_SLOTS; i++) {
        if ((int32_t) S16(st, U32(st, table + 0x10u * (uint32_t) i + 8u) + 0x12u) == fig) {
            slot = table + 0x10u * (uint32_t) i;
            break;
        }
    }
    if (slot) {
        if (S32(st, slot) != stage) {
            U8(st, f + 0x38) = action;
            return;
        }
    } else {
        for (int i = 0; i < STONE_SLOTS; i++) {
            uint32_t s = table + 0x10u * (uint32_t) i;
            if (S16(st, U32(st, s + 8u) + 0x12u) == 0 && S32(st, s) == stage
                && U32(st, s + 4u) == (uint32_t) U8(st, f + 0x0A)
                && S32(st, b + 0x80u + 4u * (uint32_t) stage) > 0) {
                slot = s;
                break;
            }
        }
        if (!slot) {
            U8(st, f + 0x38) = action;
            return;
        }
    }
    if (S16(st, U32(st, slot + 8u) + 0x12u) == 0) {
        GAME_CALLT(st, U32(st, slot + 8u), 0x0041FD80u, fig);
        GAME_CALLT(st, U32(st, slot + 8u), 0x0041FCA0u, 0);
    }
    uint32_t c = b + 0x80u + 4u * (uint32_t) stage;
    int32_t left = (int32_t) (U32(st, c) - 1u);
    S32(st, c) = left;
    if (left < 1) {
        S32(st, b + 0x5C) = 0;
        tomb_stage_end(st, id);
        U8(st, f + 0x38) = action;
    }
}

void tomb_work(game_state *st, int32_t id, int32_t fig, uint8_t action)
{
    /* @bin 0x00547E20 (thiscall on a tomb object, `this` unused; part, figure, action) the part's
     * stone set (+0x68, unsigned <= 0x1D) picks the template instance; none: nothing */
    uint32_t set = U32(st, B(id) + 0x68);
    if (set <= 0x1D && stone_sets[set].fn) {
        tomb_lay_stone(st, stone_sets[set].table, id, fig, action);
    }
}

static uint32_t this_of(const int32_t *a, uint32_t def)
{
    return a[PORT_THIS_ARG] ? (uint32_t) a[PORT_THIS_ARG] : def;
}

static int32_t run_work(game_state *st, const int32_t *a)
{
    tomb_work(st, a[0], a[1], (uint8_t) a[2]);
    return 0;
}

static int32_t run_stage_end(game_state *st, const int32_t *a)
{
    tomb_stage_end(st, a[0]);
    return 0;
}

static int32_t run_advance(game_state *st, const int32_t *a)
{
    tomb_part_advance(st, this_of(a, 0), a[0], a[1]);
    return 0;
}

/* @bin 0x00545BC0, 0x005590A0, 0x00556410, 0x0054FB20 (thiscall, ret 4): 0x00547870 (part, 0xA7 /
 * 0x9E / 0x1B5 / 0x201) on the same object */
static int32_t run_e5(game_state *st, const int32_t *a)
{
    tomb_part_advance(st, this_of(a, 0x006F9320u), a[0], 0xA7);
    return 0;
}

static int32_t run_ea(game_state *st, const int32_t *a)
{
    tomb_part_advance(st, this_of(a, 0x006F5648u), a[0], 0x9E);
    return 0;
}

static int32_t run_eb(game_state *st, const int32_t *a)
{
    tomb_part_advance(st, this_of(a, 0x006DE9E0u), a[0], 0x1B5);
    return 0;
}

static int32_t run_ec(game_state *st, const int32_t *a)
{
    tomb_part_advance(st, this_of(a, 0x006E9188u), a[0], 0x201);
    return 0;
}

#define LAY(n, tbl) \
    static int32_t run_lay_##n(game_state *st, const int32_t *a) \
    { \
        tomb_lay_stone(st, tbl, a[0], a[1], (uint8_t) a[2]); \
        return 0; \
    }
LAY(0, 0x005BE5F8u)
LAY(1, 0x005BE508u)
LAY(2, 0x005BE538u)
LAY(3, 0x005BE568u)
LAY(4, 0x005BE748u)
LAY(5, 0x005BE778u)
LAY(6, 0x005BE628u)
LAY(7, 0x005BE7A8u)
LAY(8, 0x005BE7D8u)
LAY(9, 0x005BE658u)
LAY(10, 0x005BE688u)
LAY(11, 0x005BE898u)
LAY(12, 0x005BE808u)
LAY(13, 0x005BE8C8u)
LAY(14, 0x005BE8F8u)
LAY(15, 0x005BE598u)
LAY(16, 0x005BE5C8u)
LAY(19, 0x005BE6B8u)
LAY(20, 0x005BE6E8u)
LAY(21, 0x005BE718u)
LAY(22, 0x005BE838u)
LAY(23, 0x005BE868u)
LAY(24, 0x005BE928u)
LAY(25, 0x005BE958u)
LAY(26, 0x005BE988u)

#define LAY_ENTRY(n, addr) {addr, "tomb_lay_stone set " #n, 3, 0, 0, run_lay_##n, 0}

const ported_function tomb_stone_ports[] = {
    {0x00547E20u, "tomb_work", 3, 0, 0, run_work, 0},
    {0x004223F0u, "tomb_stage_end", 1, 0, 0, run_stage_end, 0},
    {0x00547870u, "tomb_part_advance", 2, 0, 0, run_advance, 0},
    {0x00545BC0u, "tomb_e5_stage_end", 1, 0, 0x006F9320u, run_e5, 0},
    {0x005590A0u, "tomb_ea_stage_end", 1, 0, 0x006F5648u, run_ea, 0},
    {0x00556410u, "tomb_eb_stage_end", 1, 0, 0x006DE9E0u, run_eb, 0},
    {0x0054FB20u, "tomb_ec_stage_end", 1, 0, 0x006E9188u, run_ec, 0},
    LAY_ENTRY(0, 0x00423CB0u),
    LAY_ENTRY(1, 0x004233F0u),
    LAY_ENTRY(2, 0x00423530u),
    LAY_ENTRY(3, 0x00423670u),
    LAY_ENTRY(4, 0x004237B0u),
    LAY_ENTRY(5, 0x004238F0u),
    LAY_ENTRY(6, 0x00423DF0u),
    LAY_ENTRY(7, 0x00423F30u),
    LAY_ENTRY(8, 0x00424070u),
    LAY_ENTRY(9, 0x004241B0u),
    LAY_ENTRY(10, 0x004242F0u),
    LAY_ENTRY(11, 0x00424430u),
    LAY_ENTRY(12, 0x00424570u),
    LAY_ENTRY(13, 0x004246B0u),
    LAY_ENTRY(14, 0x004247F0u),
    LAY_ENTRY(15, 0x00423A30u),
    LAY_ENTRY(16, 0x00423B70u),
    LAY_ENTRY(19, 0x00424930u),
    LAY_ENTRY(20, 0x00424A70u),
    LAY_ENTRY(21, 0x00424BB0u),
    LAY_ENTRY(22, 0x00424CF0u),
    LAY_ENTRY(23, 0x00424E30u),
    LAY_ENTRY(24, 0x00424F70u),
    LAY_ENTRY(25, 0x004250B0u),
    LAY_ENTRY(26, 0x004251F0u),
};
const int tomb_stone_ports_count = sizeof(tomb_stone_ports) / sizeof(tomb_stone_ports[0]);
