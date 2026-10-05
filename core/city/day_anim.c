/* Imhotep - the animated sprites stepped every tick (0x0041FF60).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * An animation object (in the exe's data): +0 i32 set (0..0xFC), +4 i32 animation, +8 i32 frame,
 * +0x10 i16 mode (2, 3: stopped; 1: clamp at the ends), +0x12 i16 figure, +0x14 u8 the figure's
 * +0x38 it follows, +0x15 u8 direction (1: backwards). A set (0x005BE9B8 + 0x28 * set): +0x10
 * pointer to its animations (0x30 bytes, +0xC the last frame), +0x14 their count, +0x18 pointer to
 * its transitions (0x28 bytes: i32 from, u8 direction, then four (i32 to, u8 direction) of 8
 * bytes, a negative `to` ending them), +0x1C their count. */
#include "city/day_anim.h"

#include "random/crt_rand.h"
#include "state/ported.h"
#include "state/records.h"

#define SETS 0x005BE9B8u

static uint32_t set_addr(int32_t set)
{
    return SETS + 0x28u * (uint32_t) set;
}

static int32_t last_frame(game_state *st, uint32_t t, int32_t anim)
{
    return (int32_t) P32(st, P32(st, t + 0x10) + 0x30u * (uint32_t) anim + 0xC);
}

/* the targets of transition i: count them, draw one with the CRT rand, apply it; 0 when there
 * is none */
static int pick(game_state *st, uint32_t obj, uint32_t t, int32_t i, int *valid)
{
    uint32_t tr = P32(st, t + 0x18);
    int32_t k = 0;
    while (k < 4 && (int32_t) P32(st, tr + 0x28u * (uint32_t) i + 8 + 8u * (uint32_t) k) >= 0) {
        k++;
    }
    if (k == 0) {
        return 0;
    }
    int32_t idx = crt_rand(st) % k + 5 * i;
    int32_t to = (int32_t) P32(st, P32(st, t + 0x18) + 8u * (uint32_t) idx + 8);
    P32(st, obj + 4) = (uint32_t) to;
    uint8_t dir = P8(st, P32(st, t + 0x18) + 8u * (uint32_t) idx + 0xC);
    P8(st, obj + 0x15) = dir;
    *valid = to >= 0 && to < (int32_t) P32(st, t + 0x14);
    if (*valid) {
        P32(st, obj + 8) = dir != 0 ? (uint32_t) last_frame(st, t, to) : 0;
    }
    return 1;
}

void anim_restart(game_state *st, uint32_t obj, int16_t mode)
{
    /* @bin 0x0041FCA0 the first transition whose `from` is negative */
    P16(st, obj + 0x10) = mode;
    int32_t set = (int32_t) P32(st, obj);
    if (set < 0 || set >= 0xFD) {
        return;
    }
    uint32_t t = set_addr(set);
    P32(st, obj + 4) = 0xFFFFFFFFu;
    int32_t n = (int32_t) P32(st, t + 0x1C);
    for (int32_t i = 0; i < n; i++) {
        if ((int32_t) P32(st, P32(st, t + 0x18) + 0x28u * (uint32_t) i) < 0) {
            int valid = 1;
            if (pick(st, obj, t, i, &valid) && !valid) {
                P32(st, obj + 4) = 0xFFFFFFFFu;
            }
            return;
        }
    }
}

void anim_step(game_state *st, uint32_t obj)
{
    /* @bin 0x0041FDB0 */
    int16_t fig = P16(st, obj + 0x12);
    if (fig != 0 && U8(st, FIGURE_ADDR(fig) + 0x38) != P8(st, obj + 0x14)) {
        P16(st, obj + 0x12) = 0;
        P8(st, obj + 0x14) = 0;
        P16(st, obj + 0x10) = 3;
        return;
    }
    int16_t mode = P16(st, obj + 0x10);
    if (mode == 2 || mode == 3) {
        return;
    }
    int32_t set = (int32_t) P32(st, obj);
    if (set < 0 || set >= 0xFD) {
        return;
    }
    uint32_t t = set_addr(set);
    int32_t cur = (int32_t) P32(st, obj + 4);
    if (cur < 0 || cur >= (int32_t) P32(st, t + 0x14)) {
        return;
    }
    int32_t frame = (int32_t) P32(st, obj + 8) + (P8(st, obj + 0x15) != 0 ? -1 : 1);
    P32(st, obj + 8) = (uint32_t) frame;
    int32_t last = last_frame(st, t, cur);
    if (frame <= last && frame >= 0) {
        return;
    }
    if (mode == 1) {
        P32(st, obj + 8) = frame > last ? (uint32_t) last : 0;
        return;
    }
    /* @bin 0x0041FE76 the transition from this animation in this direction */
    int32_t n = (int32_t) P32(st, t + 0x1C);
    for (int32_t i = 0; i < n; i++) {
        uint32_t tr = P32(st, t + 0x18) + 0x28u * (uint32_t) i;
        if ((int32_t) P32(st, tr) == cur && P8(st, tr + 4) == P8(st, obj + 0x15)) {
            int valid = 1;
            if (pick(st, obj, t, i, &valid) && valid) {
                return;
            }
            break;
        }
    }
    anim_restart(st, obj, 3);
}

/* @bin 0x0041FF20 0x0041FF40 0x00420D40 0x004216D0 0x004206D0 0x00420A10 0x00421040 lists of
 * 16-byte entries whose first dword points to an animation object */
static void step_list(game_state *st, uint32_t from, uint32_t to)
{
    for (uint32_t e = from; e < to; e += 0x10) {
        anim_step(st, U32(st, e));
    }
}

static void list_41ff20(game_state *st) { step_list(st, 0x005C2750u, 0x005C28B0u); }
static void list_41ff40(game_state *st) { step_list(st, 0x00596088u, 0x00596158u); }
static void list_420d40(game_state *st) { step_list(st, 0x005AD218u, 0x005AD3C8u); }
static void list_4206d0(game_state *st) { step_list(st, 0x0059EDF0u, 0x0059EFA0u); }
static void list_420a10(game_state *st) { step_list(st, 0x005A8148u, 0x005A84C8u); }
static void list_421040(game_state *st) { step_list(st, 0x005B0060u, 0x005B0150u); }

static void list_4216d0(game_state *st)
{
    /* @bin 0x004216D0 */
    static const uint32_t ranges[][2] = {
        {0x005BE510u, 0x005BE540u}, {0x005BE540u, 0x005BE570u}, {0x005BE570u, 0x005BE5A0u},
        {0x005BE750u, 0x005BE780u}, {0x005BE780u, 0x005BE7B0u}, {0x005BE5A0u, 0x005BE5D0u},
        {0x005BE5D0u, 0x005BE600u}, {0x005BE600u, 0x005BE630u}, {0x005BE630u, 0x005BE660u},
        {0x005BE7B0u, 0x005BE7E0u}, {0x005BE7E0u, 0x005BE810u}, {0x005BE660u, 0x005BE690u},
        {0x005BE690u, 0x005BE6C0u}, {0x005BE8A0u, 0x005BE8D0u}, {0x005BE810u, 0x005BE840u},
        {0x005BE8D0u, 0x005BE900u}, {0x005BE900u, 0x005BE930u}, {0x005BE6C0u, 0x005BE6F0u},
        {0x005BE6F0u, 0x005BE720u}, {0x005BE720u, 0x005BE750u}, {0x005BE840u, 0x005BE870u},
        {0x005BE870u, 0x005BE8A0u}, {0x005BE930u, 0x005BE960u}, {0x005BE960u, 0x005BE990u},
        {0x005BE990u, 0x005BE9C0u},
    };
    for (unsigned i = 0; i < sizeof(ranges) / sizeof(ranges[0]); i++) {
        step_list(st, ranges[i][0], ranges[i][1]);
    }
}

static int32_t monument_4e59c0(game_state *st)
{
    /* @bin 0x004E59C0 the first of the current city's three buildings at +0x473C that is used,
     * of type 0xCF or 0xD0, with +0x55 != 100 */
    uint32_t c = CURRENT_CITY(st);
    for (uint32_t i = 0; i < 3; i++) {
        int32_t id = S32(st, c + 0x473Cu + 4 * i);
        if (id == 0) {
            continue;
        }
        uint32_t b = BUILDING_ADDR(id);
        int16_t type = S16(st, b + 0x10);
        if (U8(st, b) == 1 && (type == 0xCF || type == 0xD0) && U8(st, b + 0x55) != 100) {
            return id;
        }
    }
    return 0;
}

static void monument_anims_4e5a80(game_state *st)
{
    /* @bin 0x004E5A80 type 0xCF: 10 objects of the list 0x00586B08; else 18 of 0x0058FE48 (the
     * object pointer at +8 of each 16-byte entry) */
    int32_t id = monument_4e59c0(st);
    if (id == 0) {
        return;
    }
    int16_t type = S16(st, BUILDING_ADDR(id) + 0x10);
    int32_t n = type == 0xCF ? 10 : 18;
    uint32_t list = type == 0xCF ? 0x00586B08u : 0x0058FE48u;
    for (int32_t i = 0; i < n; i++) {
        anim_step(st, U32(st, list + 0x10u * (uint32_t) i + 8));
    }
}

void anim_step_all(game_state *st)
{
    /* @bin 0x0041FF60 */
    list_41ff20(st);
    monument_anims_4e5a80(st);
    list_41ff40(st);
    list_420d40(st);
    list_4216d0(st);
    list_4206d0(st);
    list_420a10(st);
    list_421040(st);
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
VOID0(run_step_all, anim_step_all)
VOID0(run_41ff20, list_41ff20)
VOID0(run_41ff40, list_41ff40)
VOID0(run_420d40, list_420d40)
VOID0(run_4216d0, list_4216d0)
VOID0(run_4206d0, list_4206d0)
VOID0(run_420a10, list_420a10)
VOID0(run_421040, list_421040)
VOID0(run_4e5a80, monument_anims_4e5a80)

static int32_t run_4e59c0(game_state *st, const int32_t *a)
{
    (void) a;
    return monument_4e59c0(st);
}

/* thiscall on an object of the lists: `this` in args[PORT_THIS_ARG] */
static int32_t run_anim_step(game_state *st, const int32_t *a)
{
    anim_step(st, (uint32_t) a[PORT_THIS_ARG]);
    return 0;
}

static int32_t run_anim_restart(game_state *st, const int32_t *a)
{
    anim_restart(st, (uint32_t) a[PORT_THIS_ARG], (int16_t) a[0]);
    return 0;
}

const ported_function day_anim_ports[] = {
    {0x0041FF60u, "anim_step_all", 0, 0, 0, run_step_all, 0},
    {0x0041FF20u, "anim_list_41ff20", 0, 0, 0, run_41ff20, 0},
    {0x0041FF40u, "anim_list_41ff40", 0, 0, 0, run_41ff40, 0},
    {0x00420D40u, "anim_list_420d40", 0, 0, 0, run_420d40, 0},
    {0x004216D0u, "anim_list_4216d0", 0, 0, 0, run_4216d0, 0},
    {0x004206D0u, "anim_list_4206d0", 0, 0, 0, run_4206d0, 0},
    {0x00420A10u, "anim_list_420a10", 0, 0, 0, run_420a10, 0},
    {0x00421040u, "anim_list_421040", 0, 0, 0, run_421040, 0},
    {0x004E59C0u, "monument_4e59c0", 0, 32, 0, run_4e59c0, 0},
    {0x004E5A80u, "monument_anims_4e5a80", 0, 0, 0, run_4e5a80, 0},
    {0x0041FDB0u, "anim_step", 0, 0, 0, run_anim_step, 0},
    {0x0041FCA0u, "anim_restart", 1, 0, 0, run_anim_restart, 0},
};
const int day_anim_ports_count = sizeof(day_anim_ports) / sizeof(day_anim_ports[0]);
