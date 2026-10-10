/* Imhotep - the saved-game loader's steps after reading the chunks (Pharaoh.exe 0x004D41E0).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Each step works on the version-160 chunks through the address of the Pharaoh.exe variable
 * it writes (save_schema), so the code follows the binary address for address. Writes to
 * variables that are not saved are dropped (counted in stats). docs/formats/save.md §2, §4. */
#include "io/save_load.h"

#include "state/layout.h"
#include "state/vec.h"

#include <string.h>

#include "io/save_relayout.h"
#include "io/save_tables.h"

/* Variables (save.md §3): state/layout.h; the formations (chunk 24, 50 x 144) */
#define FORMATION(id) (G_FORMATION_BASE + 0x90u * (uint32_t) (id))

typedef struct {
    save_file *s;
    save_load_env *env;
    save_load_stats *st;
    game_state *gs; /* state mode: every access goes to the game state */
    const int32_t *restore[4]; /* state mode: dword 0 of the restored tables before the load */
} ctx;

static uint8_t *mem(ctx *c, uint32_t addr, uint32_t n)
{
    if (c->gs) {
        return state_ptr(c->gs, addr, n);
    }
    for (int i = 0; i < SAVE_SCHEMA_RECORDS; i++) {
        const save_schema_record *t = &save_schema[i];
        uint32_t size = t->elem_size * t->elem_count;
        if (addr >= t->address && addr - t->address + n <= size) {
            return c->s->chunk[i] + (addr - t->address);
        }
    }
    return NULL;
}

static uint32_t rd(ctx *c, uint32_t addr, uint32_t n)
{
    const uint8_t *p = mem(c, addr, n);
    uint32_t v = 0;
    for (uint32_t k = 0; p && k < n; k++) {
        v |= (uint32_t) p[k] << (8 * k);
    }
    return v;
}
#define rd8(c, a) ((uint8_t) rd(c, a, 1))
#define rd16(c, a) ((int16_t) rd(c, a, 2))
#define rd32(c, a) ((int32_t) rd(c, a, 4))

static void wr(ctx *c, uint32_t addr, uint32_t n, uint32_t v)
{
    uint8_t *p = mem(c, addr, n);
    if (!p) {
        c->st->dropped_writes++;
        return;
    }
    for (uint32_t k = 0; k < n; k++) {
        p[k] = (uint8_t) (v >> (8 * k));
    }
}
#define wr8(c, a, v) wr(c, a, 1, (uint32_t) (v))
#define wr16(c, a, v) wr(c, a, 2, (uint32_t) (v))
#define wr32(c, a, v) wr(c, a, 4, (uint32_t) (v))

/* @bin 0x00562348 MSVC CRT rand() */
static int32_t crt_rand(ctx *c)
{
    c->st->rand_draws++;
    c->env->crt_rand = c->env->crt_rand * 214013u + 2531011u;
    return (int32_t) ((c->env->crt_rand >> 16) & 0x7FFF);
}

/* ---- empire routes, chunk 143 (@bin 0x004D41E0 loop after the chunks, 0x004D4720) ---------- */

static int32_t isqrt64(uint64_t d)
{
    uint64_t r = 0, bit = (uint64_t) 1 << 62;
    while (bit > d) {
        bit >>= 2;
    }
    while (bit) {
        if (d >= r + bit) {
            d -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (int32_t) r;
}

static int route_rebuild(ctx *c, uint32_t r)
{
    int n = rd8(c, r + 0x141), step = rd8(c, r);
    int32_t est = 0;
    /* @bin 0x004D4720 estimate: sum over segments of max(|dx|, |dy|) + 1 */
    for (int i = 1; i < n; i++) {
        int32_t dx = rd16(c, r + 8 + 6 * i) - rd16(c, r + 8 + 6 * (i - 1));
        int32_t dy = rd16(c, r + 10 + 6 * i) - rd16(c, r + 10 + 6 * (i - 1));
        dx = dx < 0 ? -dx : dx;
        dy = dy < 0 ? -dy : dy;
        est += 1 + (dy < dx ? dx : dy);
    }
    /* @bin 0x004D4720 point buffer: when it grows, the old one is freed (+4 := 0) and a new one of
     * (estimate + 50) points of 6 bytes is allocated; capacity := estimate + 50. In state mode the
     * buffer is allocated in the heap arena as the game does. Its points (x87 arithmetic, drawn on
     * the empire map) are not computed: the simulation does not read them (docs/formats/state.md). */
    int32_t cap = rd32(c, r + 0x138);
    if (cap < est && cap < est + 0x32) {
        if (c->gs) {
            if (rd32(c, r + 4) != 0) {
                state_dealloc(c->gs, (uint32_t) rd32(c, r + 4));
                wr32(c, r + 4, 0);
            }
            wr32(c, r + 4, state_alloc(c->gs, (uint32_t) (est + 0x32) * 6));
        }
        wr32(c, r + 0x138, est + 0x32);
    }
    wr32(c, r + 0x134, est);
    int32_t count = 0;
    for (int i = 1; i < n; i++) {
        if (rd32(c, r + 0x134) < count) {
            return 0; /* @bin 0x004D4720 more points than estimated: give up, endpoints unchanged */
        }
        int32_t dx = rd16(c, r + 8 + 6 * i) - rd16(c, r + 8 + 6 * (i - 1));
        int32_t dy = rd16(c, r + 10 + 6 * i) - rd16(c, r + 10 + 6 * (i - 1));
        uint64_t d = (uint64_t) ((int64_t) dx * dx) + (uint64_t) ((int64_t) dy * dy);
        if (d == 0 || step == 0) {
            continue;
        }
        /* @bin 0x004D4720 one point per step while k * step <= f, f = float(sqrt(d)) (FSQRT, FSTP
         * float). For sqrt(d) < 2896 a float rounds no sqrt(m^2 - 1) up to m, so the count is
         * isqrt(d) / step + 1; checked on 2,650 saved routes (save.md §4). */
        int32_t root = isqrt64(d);
        if (root >= 2896) {
            c->st->route_segments_unproven++;
        }
        count += root / step + 1;
    }
    wr32(c, r + 0x134, count);
    if (!c->env->image_size) {
        c->st->endpoints_kept++; /* without image sizes the stored endpoints stay */
        return 1;
    }
    wr32(c, r + 0x13C, -1); /* both endpoints := 0xFFFF */
    /* @bin 0x004D4720 endpoints: the last empire object (in use, kind 1) whose image rectangle
     * holds the first / last point */
    int32_t fx = rd16(c, r + 8), fy = rd16(c, r + 10), lx = rd16(c, r + 6 * n + 2), ly = rd16(c, r + 6 * n + 4);
    for (int j = 0; n != 0 && j < 200; j++) { /* object scan only when the route has points */
        uint32_t o = G_EMPIRE_OBJECTS + 0x62u * (uint32_t) j;
        if (rd8(c, o + 1) == 0 || rd8(c, o) != 1) {
            continue;
        }
        int32_t id = (uint16_t) rd16(c, o + 0xC);
        if (id >= c->env->image_count) {
            id = 0;
        }
        int16_t w = 0, h = 0;
        c->env->image_size(c->env->image_ctx, id, &w, &h);
        int32_t x0 = rd16(c, o + 4), y0 = rd16(c, o + 6);
        if (x0 <= fx && fx <= x0 + w && y0 <= fy && fy <= y0 + h) {
            wr16(c, r + 0x13C, j);
        }
        if (x0 <= lx && lx <= x0 + w && y0 <= ly && ly <= y0 + h) {
            wr16(c, r + 0x13E, j);
        }
    }
    return 1; /* @bin 0x004D4720 al := 1 */
}

static void routes(ctx *c)
{
    wr8(c, G_ROUTES + 0x141, 0); /* @bin 0x004D41E0 DAT_007770f1 := 0, DAT_007770e4 := 0 */
    wr32(c, G_ROUTES + 0x134, 0);
    for (int k = 0; k < 50; k++) {
        uint32_t r = G_ROUTES + 324u * (uint32_t) k;
        if (rd32(c, r + 4) == 0 && rd32(c, r + 0x138) == 0 && rd8(c, r + 0x141) < 0x33 && rd8(c, r + 0x140) < 3 &&
            rd8(c, r) != 0 && rd8(c, r) < 0x33) {
            route_rebuild(c, r);
            c->st->routes_rebuilt++;
        } else {
            wr32(c, r + 0x134, 0);
            wr32(c, r + 4, 0);
            wr8(c, r + 0x141, 0);
            wr32(c, r + 0x138, 0);
            wr8(c, r, 5);
            wr8(c, r + 0x142, 0);
            wr16(c, r + 0x13C, 0xFFFF);
            wr16(c, r + 0x13E, 0xFFFF);
            wr8(c, r + 0x140, 1);
            c->st->routes_reset++;
        }
    }
}

/* ---- monument work-step records (@bin 0x0041FCA0 and the clean-ups) ------------------------ */

/* @bin 0x0041FCA0(record, 3): state := 3, then a random option of the record's script */
static void work_step_reset(ctx *c, uint32_t rec)
{
    wr16(c, rec + 0x10, 3);
    int32_t id = rd32(c, rec);
    if (id < 0 || id >= SAVE_OPTION_DESCS) {
        return;
    }
    wr32(c, rec + 4, -1);
    const save_option_desc *d = &save_option_descs[id];
    if (d->states <= 0) {
        return;
    }
    const int32_t *st = save_option_states + 10 * d->first_state;
    int k = 0;
    while (st[10 * k] >= 0) { /* first state whose dword 0 is negative */
        k++;
        if (d->states <= k) {
            return;
        }
    }
    int n = 0;
    while (n < 4 && st[10 * k + 2 + 2 * n] >= 0) {
        n++;
    }
    if (n == 0) {
        return;
    }
    int r = crt_rand(c) % n + 5 * k;
    int32_t option = st[2 + 2 * r];
    uint8_t flag = (uint8_t) st[3 + 2 * r];
    wr32(c, rec + 4, option);
    wr8(c, rec + 0x15, flag);
    if (option >= 0 && option < d->items) {
        wr32(c, rec + 8, flag ? save_option_items[d->first_item + option] : 0);
        return;
    }
    wr32(c, rec + 4, -1);
}

static void cleanup(ctx *c, const save_cleanup_entry *prog, int count)
{
    for (int i = 0; i < count; i++) {
        wr16(c, prog[i].record + 0x12, 0);
        work_step_reset(c, prog[i].record);
        wr32(c, prog[i].record + 0xC, prog[i].value);
    }
}

/* ---- figure names (@bin 0x004D32F0, every load) -------------------------------------------- */

static void name(ctx *c, uint32_t f, int32_t group, uint32_t counter, int32_t limit)
{
    wr32(c, f + FIG_NAME_GROUP, group);
    if (rd32(c, counter) > limit) {
        wr32(c, counter, 0);
    }
}

static void figure_name(ctx *c, uint32_t f, int type)
{
    switch (type) {
    case 1: case 2: case 4: case 5: case 9: case 0x23: case 0x26: case 0x36: case 0x37: case 0x38:
    case 0x4B: case 0x4F: case 0x50: case 0x51: case 0x55: case 0x57: case 0x5A: case 0x60: case 0x62:
    case 0x6C:
        name(c, f, 0xFE, G_NAME_COUNTER_FE, 0x7F);
        break;
    case 3: name(c, f, 0xFF, G_NAME_COUNTER_FF, 0x1F); break;
    case 7: name(c, f, 0x115, G_NAME_COUNTER_115, 0xF); break;
    case 8: name(c, f, 0x113, G_NAME_COUNTER_113, 0xF); break;
    case 10: name(c, f, 0x112, G_NAME_COUNTER_112, 0xF); break;
    case 0xF: case 0x12: case 0x69: name(c, f, 0x107, G_NAME_COUNTER_107, 0x1F); break;
    case 0x10: name(c, f, 0x108, G_NAME_COUNTER_108, 0x1F); break;
    case 0x11: name(c, f, 0x109, G_NAME_COUNTER_109, 0xF); break;
    case 0x13: case 0x15: name(c, f, 0x106, G_NAME_COUNTER_106, 0xF); break;
    case 0x14: name(c, f, 0x105, G_NAME_COUNTER_105, 0xF); break;
    case 0x16: case 0x17: case 0x18: name(c, f, 0x100, G_NAME_COUNTER_100, 0xF); break;
    case 0x1A: case 0x27: name(c, f, 0x103, G_NAME_COUNTER_103, 0x1F); break;
    case 0x1B: {
        /* @bin 0x004C5120 god of the home building (+0x5A): temples 0x3C-0x40, complexes 0x41-0x45 */
        int bt = rd16(c, BUILDING_ADDR(rd16(c, f + FIG_HOME)) + BLD_TYPE);
        int god = bt >= 0x3C && bt <= 0x40 ? bt - 0x3C : bt >= 0x41 && bt <= 0x45 ? bt - 0x41 : -1;
        static const uint32_t COUNTER[5] = {G_NAME_COUNTER_10A, G_NAME_COUNTER_10B, G_NAME_COUNTER_10C, G_NAME_COUNTER_10D, G_NAME_COUNTER_10E};
        if (god >= 0) {
            name(c, f, 0x10A + god, COUNTER[god], 0xF);
        }
        break;
    }
    case 0x1D: case 0x1E: name(c, f, 0x10F, G_NAME_COUNTER_10F, 0x1F); break;
    case 0x1F: case 0x20: case 0x21: name(c, f, 0x110, G_NAME_COUNTER_110, 0x1F); break;
    case 0x22: name(c, f, 0x111, G_NAME_COUNTER_111, 0xF); break;
    case 0x28: name(c, f, 0x101, G_NAME_COUNTER_101, 0x1F); break;
    case 0x2A: name(c, f, 0x118, G_NAME_COUNTER_118, 0x1F); break;
    case 0x2B: case 0x2C: case 0x2D: {
        /* enemy class: formation (+0x60) byte +0x6A */
        static const struct { int32_t group; uint32_t counter; int32_t limit; } E[12] = {
            {0x11D, G_NAME_COUNTER_11D, 0xF}, {0x11F, G_NAME_COUNTER_11F, 0xF}, {0x11E, G_NAME_COUNTER_11E, 0xF},
            {0x120, G_NAME_COUNTER_120, 0x10}, {0x11C, G_NAME_COUNTER_11C, 0xF}, {0x11B, G_NAME_COUNTER_11B, 0xF},
            {0x11A, G_NAME_COUNTER_11A, 0xF}, {0x119, G_NAME_COUNTER_119, 0xF}, {300, G_NAME_COUNTER_12C, 0xF},
            {0x12D, G_NAME_COUNTER_12D, 0xF}, {0x12E, G_NAME_COUNTER_12E, 0xF}, {0x12F, G_NAME_COUNTER_12F, 0xF},
        };
        int cls = rd8(c, FORMATION(rd16(c, f + FIG_FORMATION)) + 0x6A);
        if (cls < 12) {
            name(c, f, E[cls].group, E[cls].counter, E[cls].limit);
        } else {
            wr32(c, f + FIG_NAME_GROUP, -1);
        }
        break;
    }
    case 0x42: name(c, f, 0x104, G_NAME_COUNTER_104, 0xF); break;
    case 0x49: name(c, f, 0x102, G_NAME_COUNTER_102, 0xF); break;
    case 0x4C: name(c, f, 0x117, G_NAME_COUNTER_117, 0xF); break;
    case 0x58: name(c, f, 0x114, G_NAME_COUNTER_114, 0xF); break;
    case 0x59: name(c, f, 0x116, G_NAME_COUNTER_116, 0xF); break;
    case 0x5B: {
        /* +0x0B := the walker kind for the home building's type, then named as that type
         * (second jump table at 0x004D3BCC: 0xF, 0x10, 0x11, 0x1B, 0x1D, 0x28; others -1) */
        int bt = (uint16_t) rd16(c, BUILDING_ADDR(rd16(c, f + FIG_HOME)) + BLD_TYPE);
        int k;
        switch (bt) {
        case 0x1E: case 0x22: k = 0x10; break;
        case 0x1F: case 0x20: case 0x24: case 0xE2: k = 0xF; break;
        case 0x21: case 0x23: k = 0x11; break;
        case 0x33: case 0x35: k = 0x1D; break;
        case 0x3C: case 0x3D: case 0x3E: case 0x3F: case 0x40: case 0x41: case 0x42: case 0x43: case 0x44:
        case 0x45: case 0x97: case 0x9C: k = 0x1B; break;
        default: k = 0x28; break;
        }
        wr8(c, f + FIG_RESOURCE, k);
        if (k == 0xF || k == 0x10 || k == 0x11 || k == 0x1B || k == 0x1D || k == 0x28) {
            figure_name(c, f, k);
        } else {
            wr32(c, f + FIG_NAME_GROUP, -1);
        }
        break;
    }
    case 0x5C: case 0x5D: case 100: case 0x65: name(c, f, 0x121, G_NAME_COUNTER_121, 0xF); break;
    case 99: name(c, f, 0x122, G_NAME_COUNTER_122, 0xF); break;
    default:
        wr32(c, f + FIG_NAME_GROUP, -2);
        wr16(c, f + FIG_NAME_INDEX, 0xFFFF);
        break;
    }
}

/* ---- version upgrades (save.md §4), in the order of 0x004D41E0 ------------------------------ */

static void relayout(ctx *c, uint32_t addr, uint32_t size, const save_relayout_run *runs, int count)
{
    uint8_t *p = mem(c, addr, size), old[0x49D8];
    if (!p) {
        return;
    }
    memcpy(old, p, size);
    for (int i = 0; i < count; i++) {
        if (runs[i].src < 0) {
            memset(p + runs[i].dst, 0, runs[i].len);
        } else {
            memcpy(p + runs[i].dst, old + runs[i].src, runs[i].len);
        }
    }
}

static void upgrade_124(ctx *c)
{
    /* @bin 0x004D18F0, 0x004D1AD0, 0x004D2FA0, 0x004D3200 (tables from their disassembly) */
    for (uint32_t k = 0; k < 61; k++) {
        relayout(c, G_EMPIRE_CITIES + 0x6Au * k, 0x6A, save_relayout_chunk50, save_relayout_chunk50_count);
    }
    for (uint32_t p = 0; p < 2; p++) {
        relayout(c, CITY_ADDR(p), CITY_REC_SIZE, save_relayout_city, save_relayout_city_count);
    }
    for (uint32_t id = 0; id < 4000; id++) {
        int16_t t = rd16(c, BUILDING_ADDR(id) + BLD_TYPE);
        int keep = (t >= 10 && t <= 0x1D) || t == 0xB7 || t == 0xD2;
        relayout(c, BUILDING_ADDR(id), BUILDING_REC_SIZE, keep ? save_relayout_building_house : save_relayout_building_other,
                 keep ? save_relayout_building_house_count : save_relayout_building_other_count);
    }
    for (uint32_t k = 0; k < 200; k++) {
        relayout(c, G_STORAGE + 0xC4u * k, 0xC4, save_relayout_chunk113, save_relayout_chunk113_count);
    }
}

static void upgrade_146_buildings(ctx *c)
{
    /* @bin 0x004D3DA0 */
    int32_t cur = rd32(c, CURRENT_PLAYER_ADDR);
    int guard = 0;
    for (int32_t id = 1; id < 4000 && guard < 100000; id++, guard++) {
        uint32_t b = BUILDING_ADDR(id);
        if (rd8(c, b + BLD_STATE) != 1) {
            continue;
        }
        switch (rd16(c, b + BLD_TYPE)) {
        case 0xB7: {
            wr8(c, b + BLD_MON_ANNOUNCED_80, 0);
            const int32_t *t = c->env->previous_pyramid_kind == 5 ? save_pyramid_slots_kind5 : save_pyramid_slots;
            id += t[rd8(c, b + BLD_MONUMENT_KIND)] - 1;
            break;
        }
        case 0xCF: case 0xD0: wr8(c, b + BLD_MON_ANNOUNCED_80, 0); break;
        case 0xD2: wr32(c, CITY_ADDR(cur) + CITY_U2988, 0); break;
        case 0xD5: wr8(c, b + BLD_MON_ANNOUNCED_68, 0); id += 99; break;
        case 0xDE: wr8(c, b + BLD_MON_ANNOUNCED_74, 0); id += 0x73; break;
        case 0xE1: wr8(c, b + BLD_MON_ANNOUNCED_68, 0); id += 0x18; break;
        case 0xE3: wr8(c, b + BLD_MON_ANNOUNCED_68, 0); id += 8; break;
        case 0xE4: wr8(c, b + BLD_MON_ANNOUNCED_68, 0); break;
        case 0xE5: wr8(c, b + BLD_MON_ANNOUNCED_9D, 0); id += 0xA6; break;
        case 0xE6: wr8(c, b + BLD_MON_ANNOUNCED_60, 0); id += 0xF; break;
        case 0xEA: wr8(c, b + BLD_MON_ANNOUNCED_9D, 0); id += 0x9D; break;
        case 0xEB: wr8(c, b + BLD_MON_ANNOUNCED_9D, 0); id += 0x1B4; break;
        case 0xEC: wr8(c, b + BLD_MON_ANNOUNCED_9D, 0); id += 0x200; break;
        default: break;
        }
    }
}

static void upgrade_152_events(ctx *c)
{
    /* @bin 0x004D40C0: events of a flagged kind (0x004493C0: table 0x005D9C78[event +2]) */
    int n = rd16(c, G_EVENTS);
    for (int k = 0; k < n; k++) {
        uint32_t e = G_EVENTS + 4 + 0x7Cu * (uint32_t) k;
        if (!save_event_flags[rd8(c, e + 2)]) {
            continue;
        }
        int16_t v4 = rd16(c, e + 4), a = rd16(c, e + 6), b = rd16(c, e + 8), x = rd16(c, e + 0xA);
        if (v4 >= 0x1F) { v4 = (int16_t) (v4 + 5); }
        if (a >= 0x1F) { a = (int16_t) (a + 5); }
        if (b >= 0x1F) { b = (int16_t) (b + 5); }
        if (x >= 0x1F) { x = (int16_t) (x + 5); }
        wr16(c, e + 4, v4);
        /* pack the non-negative values of (a, b, x) first (asm 0x004D411C-0x004D4148) */
        int16_t ax = a, dx = b, cx = x;
        if (a < 0) {
            if (b >= 0) {
                ax = b;
                dx = -1;
                goto test_c;
            }
            if (x >= 0) {
                ax = x;
                cx = -1;
            }
        }
        if (b >= 0) {
            goto store;
        }
    test_c:
        if (cx >= 0) {
            dx = cx;
            cx = -1;
        }
    store:
        wr16(c, e + 6, ax);
        wr16(c, e + 8, dx);
        wr16(c, e + 0xA, cx);
    }
    for (int k = 0; k < 10; k++) { /* 5 price pairs 0x005D2E60 -> 0x005D2D40 (chunk 53) */
        wr32(c, G_PRICE_BUY + 8u * 31u + 4u * (uint32_t) k, save_default_prices[k]);
    }
}

static void upgrade_153_empire_objects(ctx *c)
{
    /* @bin 0x004D19E0: three bit masks (+0x40, +0x44, +0x48) -> u8[32] at +0x3E (3/2/1/0), then
     * 4 bytes of 2 at +0x5E */
    for (uint32_t j = 0; j < 200; j++) {
        uint32_t o = G_EMPIRE_OBJECTS + 0x62u * j;
        uint32_t m0 = (uint32_t) rd32(c, o + 0x40), m1 = (uint32_t) rd32(c, o + 0x44), m2 = (uint32_t) rd32(c, o + 0x48);
        uint8_t *p = mem(c, o, 0x62), buf[0x62] = {0};
        if (!p) {
            continue;
        }
        memcpy(buf, p, 0x4C);
        for (int k = 0; k < 32; k++) {
            uint32_t bit = 1u << k;
            buf[0x3E + k] = (uint8_t) ((m0 & bit) ? 3 : (m1 & bit) ? 2 : (m2 & bit) ? 1 : 0);
        }
        memset(buf + 0x5E, 2, 4);
        memcpy(p, buf, 0x62);
    }
}

static void upgrades(ctx *c)
{
    int32_t v = (int32_t) c->s->version;
    save_load_stats *st = c->st;
    if (v < 0x75) { /* scenario info +0x408: u16[100] := 1 for the first 50, 0 after */
        for (uint32_t k = 0; k < 100; k++) {
            wr16(c, G_START_YEAR + 0x408 + 2 * k, k < 50);
        }
        st->upgrades++;
    }
    if (v < 0x97) {
        wr16(c, G_INVASION_SUB, 0); /* 0x00784B94, scenario info +0x2BC (word store, 0x004D4370) */
        st->upgrades++;
    }
    if (v < 0x76) { /* @bin 0x004D17E0 */
        for (uint32_t j = 0; j < 200; j++) {
            uint32_t o = G_EMPIRE_OBJECTS + 0x62u * j;
            if (rd8(c, o + 1) != 0 && rd8(c, o) == 1) {
                static const int8_t MAP[7] = {4, 0, 1, 4, 6, 4, 6};
                int t = rd8(c, o + 0x18);
                if (t < 7) {
                    wr8(c, o + 0x18, MAP[t]);
                }
            }
        }
        st->upgrades++;
    }
    if (v < 0x78) { /* @bin 0x004D1840 formations 1-49 */
        for (uint32_t k = 1; k < 50; k++) {
            wr16(c, FORMATION(k) + 0x82, 0xFFFF);
        }
        st->upgrades++;
    }
    if (v < 0x79) { /* @bin 0x004D1860 messages */
        for (uint32_t k = 0; k < 1000; k++) {
            wr16(c, MESSAGE_ADDR(k) + MSG_DATA, 0xFFFF);
            wr16(c, MESSAGE_ADDR(k) + MSG_DATA + 2, 0xFFFF);
            wr16(c, MESSAGE_ADDR(k) + MSG_DATA + 4, 0xFFFF);
        }
        st->upgrades++;
    }
    if (v < 0x7A) { /* @bin 0x004D1880 both city records: 7 dwords +0x2900 -> +0x483C */
        for (uint32_t p = 0; p < 2; p++) {
            for (uint32_t k = 0; k < 7; k++) {
                wr32(c, CITY_ADDR(p) + CITY_GRANARY_FOOD + 4 * k, rd32(c, CITY_ADDR(p) + CITY_U2900 + 4 * k));
            }
        }
        st->upgrades++;
    }
    if (v < 0x7B) { /* @bin 0x004D18B0 i16[30] +0x2796 -> i32 +0x4860, +0x27DE -> +0x48F0 */
        for (uint32_t p = 0; p < 2; p++) {
            for (uint32_t k = 0; k < 30; k++) {
                wr32(c, CITY_ADDR(p) + CITY_STORAGE_STOCK + 4 * k, rd16(c, CITY_ADDR(p) + CITY_U27DE + 2 * k));
                wr32(c, CITY_ADDR(p) + CITY_STORAGE_FREE + 4 * k, rd16(c, CITY_ADDR(p) + CITY_U2796 + 2 * k));
            }
        }
        st->upgrades++;
    }
    if (v < 0x7C) {
        upgrade_124(c);
        st->upgrades++;
    }
    if (v < 0x7D) { /* @bin 0x004BD330 on the floodplain settings */
        static const int32_t F[8] = {180, 70, 100, 180, 70, 100, 0, 100};
        for (uint32_t k = 0; k < 8; k++) {
            wr32(c, G_FLOODPLAIN_SETTINGS + 4 * k, F[k]);
        }
        st->upgrades++;
    }
    if (v < 0x84) { /* @bin 0x004BD5D0 */
        wr32(c, G_FLOODPLAIN_SETTINGS + 0x1C, rd32(c, G_FLOODPLAIN_SETTINGS + 8));
        st->upgrades++;
    }
    /* @bin 0x004D0A40(1): the 10 shipwright queues (0x009783C0 + j * 0x1C) from chunk 140's
     * 0x334-byte records (0x009763B8 + j * 0x334), by 0x004F67E0: the array is re-reserved to
     * the saved capacity (zeroed: the saved elements are not restored), then +0x0C, +0x00 (byte),
     * +0x08, +0x04 from the record's dwords 0, 4, 3, 2. Outside every chunk: state mode only. */
    for (uint32_t j = 0; c->gs && j < 10; j++) {
        uint32_t q = G_SHIPWRIGHT_QUEUES + G_SHIPWRIGHT_QUEUE_STRIDE * j, d = G_SHIPWRIGHT_QUEUES_SAVED + 0x334u * j;
        vec_reserve(c->gs, q, rd32(c, d + 4));
        wr32(c, q + 0x0C, rd32(c, d));
        wr8(c, q, rd8(c, d + 0x10));
        wr32(c, q + 0x08, rd32(c, d + 0x0C));
        wr32(c, q + 0x04, rd32(c, d + 0x08));
    }
    if (v < 0x7F) { /* @bin 0x004D32D0 figures 1-1999 */
        for (uint32_t id = 1; id < 2000; id++) {
            wr8(c, FIGURE_ADDR(id) + FIG_U02, 0);
            wr16(c, FIGURE_ADDR(id) + FIG_U132, 0xFFFF);
        }
        st->upgrades++;
    }
    if (v < 0x80) { /* chunk 148 (0x00978D9C) := 0 */
        uint8_t *p = mem(c, G_TILE_VISITS, 4 * 51984);
        if (p) {
            memset(p, 0, 4 * 51984);
        }
        st->upgrades++;
    }
    if (v < 0x81) { /* @bin 0x004D32A0 message type +0x0A remap until the first 0 (0x004E1600) */
        for (uint32_t k = 0; k < 1000; k++) {
            int16_t t = rd16(c, MESSAGE_ADDR(k) + MSG_ID);
            if (t == 0) {
                break;
            }
            wr16(c, MESSAGE_ADDR(k) + MSG_ID, t < 0x33 ? t + 99 : t + 199);
        }
        st->upgrades++;
    }
    for (uint32_t id = 1; id < 2000; id++) { /* @bin 0x004D32F0, every load */
        figure_name(c, FIGURE_ADDR(id), rd8(c, FIGURE_ADDR(id) + FIG_TYPE));
    }
    if (v < 0x85) {
        wr8(c, G_CAMPAIGN_KIND, 0);
        st->upgrades++;
    }
    if (v < 0x86) { /* current player's city data +0x47A8 := difficulty */
        wr32(c, CITY_ADDR(rd32(c, CURRENT_PLAYER_ADDR)) + CITY_DIFFICULTY, rd32(c, G_DIFFICULTY));
        st->upgrades++;
    }
    if (v < 0x88) { /* @bin 0x004D3C20 -> 0x004E59C0, 0x004E5B90 */
        int32_t cur = rd32(c, CURRENT_PLAYER_ADDR), found = 0;
        for (uint32_t k = 0; k < 3 && !found; k++) {
            int32_t id = rd32(c, CITY_ADDR(cur) + CITY_MONUMENTS + 4 * k);
            int16_t t = rd16(c, BUILDING_ADDR(id) + BLD_TYPE);
            if (id != 0 && rd8(c, BUILDING_ADDR(id) + BLD_STATE) == 1 && (t == 0xCF || t == 0xD0) && rd8(c, BUILDING_ADDR(id) + BLD_U55) != 100) {
                found = id;
            }
        }
        if (found) {
            int small = rd16(c, BUILDING_ADDR(found) + BLD_TYPE) == 0xCF;
            cleanup(c, small ? save_cleanup_obelisk_small : save_cleanup_obelisk_large,
                    small ? save_cleanup_obelisk_small_count : save_cleanup_obelisk_large_count);
        }
        st->upgrades++;
    }
    if (v < 0x8B) { /* @bin 0x004D3CC0 Warships (0x4E) */
        for (uint32_t id = 1; id < 2000; id++) {
            uint32_t f = FIGURE_ADDR(id);
            if (rd8(c, f + FIG_TYPE) == 0x4E && rd8(c, f + FIG_STATE) == 1 && rd8(c, f + FIG_ACTION) != 4) {
                wr16(c, f + FIG_U14C, rd16(c, f + FIG_X));
                wr16(c, f + FIG_U14E, rd16(c, f + FIG_Y));
            }
        }
        st->upgrades++;
    }
    if (v < 0x8F) { /* @bin 0x004DE140 chunk 130 := -1 */
        for (uint32_t k = 0; k < 8; k++) {
            wr32(c, G_VIEW_BOOKMARKS + 4 * k, -1);
        }
        st->upgrades++;
    }
    if (v < 0x90) { /* @bin 0x004D3D80 4 bytes at event +0x6A := 6 (150 events from chunk +4) */
        for (uint32_t k = 0; k < 150; k++) {
            for (uint32_t b = 0; b < 4; b++) {
                wr8(c, G_EVENTS + 0x6E + 0x7C * k + b, 6);
            }
        }
        st->upgrades++;
    }
    if (v < 0x92) {
        upgrade_146_buildings(c);
        for (uint32_t k = 1; k < 50; k++) { /* @bin 0x004D3F40 byte store */
            wr8(c, FORMATION(k) + 0x68, 0);
        }
        st->upgrades++;
    }
    if (v < 0x93) {
        wr32(c, G_FLOODPLAIN_SETTINGS + 0x20, rd32(c, G_FLOODPLAIN_SETTINGS + 0x14));
        st->upgrades++;
    }
    if (v < 0x94) { /* @bin 0x004D3D00 formation +0x84: transport figure */
        for (uint32_t k = 0; k < 50; k++) {
            int t = rd8(c, FIGURE_ADDR(rd32(c, FORMATION(k) + 0x84)) + FIG_TYPE);
            if (t != 0x4D && t != 0x5C && t != 0x65) {
                wr32(c, FORMATION(k) + 0x84, 0);
            }
        }
        for (uint32_t id = 1; id < 2000; id++) {
            uint32_t f = FIGURE_ADDR(id);
            if (rd8(c, f + FIG_TYPE) == 0x4D && rd8(c, f + FIG_STATE) == 1 && rd8(c, f + FIG_ACTION) != 4 && rd8(c, f + FIG_U130) != 0) {
                wr32(c, FORMATION(rd8(c, f + FIG_U92)) + 0x84, (int32_t) id);
            }
        }
        st->upgrades++;
    }
    if (v < 0x95) { /* @bin 0x004D3F60 buildings 1-3999 (in use or not) */
        for (uint32_t id = 1; id < 4000; id++) {
            uint32_t b = BUILDING_ADDR(id) + BLD_STORAGE_ID; /* the byte moved between +0xC1, +0xCA, +0xCB */
            switch ((uint16_t) rd16(c, BUILDING_ADDR(id) + BLD_TYPE)) {
            case 0x46: case 0x47: case 0x48: case 0xBE: case 0xBF: case 0xC1:
                wr8(c, b + (BLD_UCB - BLD_STORAGE_ID), 0);
                wr8(c, b + (BLD_UCA - BLD_STORAGE_ID), 0);
                break;
            case 0x4A:
                wr8(c, b + (BLD_UCB - BLD_STORAGE_ID), rd8(c, b));
                wr8(c, b, 0);
                wr8(c, b + (BLD_UCA - BLD_STORAGE_ID), 0);
                break;
            case 0x88:
                wr8(c, b + (BLD_UCA - BLD_STORAGE_ID), rd8(c, b));
                wr8(c, b, 0);
                wr8(c, b + (BLD_UCB - BLD_STORAGE_ID), 0);
                break;
            default:
                wr8(c, b, 0);
                wr8(c, b + (BLD_UCB - BLD_STORAGE_ID), 0);
                wr8(c, b + (BLD_UCA - BLD_STORAGE_ID), 0);
                break;
            }
        }
        st->upgrades++;
    }
    if (v < 0x96) { /* @bin 0x004D4040 current player's city data +0x298C..+0x29B8 := 0 */
        uint32_t base = CITY_ADDR(rd32(c, CURRENT_PLAYER_ADDR));
        for (uint32_t k = CITY_U298C; k < CITY_U298C + 0x2D; k++) {
            wr8(c, base + k, 0);
        }
        st->upgrades++;
    }
    if (v < 0x98) {
        upgrade_152_events(c);
        st->upgrades++;
    }
    if (v < 0x99) {
        upgrade_153_empire_objects(c);
        st->upgrades++;
    }
    if (v < 0x9B) { /* @bin 0x004D4080, 0x004D40A0, scenario info +0x462 (110 bytes) */
        for (uint32_t id = 0; id < 4000; id++) {
            wr8(c, BUILDING_ADDR(id) + BLD_ACCESS_ZOOKEEPER, 0);
        }
        for (uint32_t p = 0; p < 2; p++) {
            wr32(c, CITY_ADDR(p) + CITY_U44C8, 0);
            wr32(c, CITY_ADDR(p) + CITY_U44CC, 0);
        }
        for (uint32_t k = 0; k < 110; k++) {
            wr8(c, G_START_YEAR + 0x462 + k, 0);
        }
        st->upgrades++;
    }
    if (v < 0x9C) { /* @bin 0x004D1A70 i16[20] +0x291C / +0x2944 -> i32 +0x3C64 / +0x3D04, upper halves 0 */
        for (uint32_t p = 0; p < 2; p++) {
            for (uint32_t k = 0; k < 20; k++) {
                wr32(c, CITY_ADDR(p) + CITY_U3D04 + 4 * k, rd16(c, CITY_ADDR(p) + CITY_U2944 + 2 * k));
                wr32(c, CITY_ADDR(p) + CITY_U3C64 + 4 * k, rd16(c, CITY_ADDR(p) + CITY_U291C + 2 * k));
            }
            for (uint32_t k = 0; k < 20; k++) {
                wr32(c, CITY_ADDR(p) + CITY_U3C64 + 0x50 + 4 * k, 0);
                wr32(c, CITY_ADDR(p) + CITY_U3D04 + 0x50 + 4 * k, 0);
            }
        }
        st->upgrades++;
    }
    if (v < 0x9D) {
        wr32(c, G_BLOOD_RIVER_LEFT, 0); /* chunk 131, blood river region */
        wr32(c, G_FROGS_COUNTDOWN, 0);
        wr32(c, G_PYRAMID_CURSE_COUNTDOWN, 0);
        wr8(c, G_MONUMENT_MODE, 1); /* scenario info +0x3E4 */
        st->upgrades++;
    }
    if (v < 0x9E) {
        wr32(c, G_BLOOD_RIVER_DURATION, 0);
        st->upgrades++;
    }
    if (v < 0x9F) {
        wr8(c, G_START_YEAR + 0x3E5, 0);
        st->upgrades++;
    }
    if (v < 0xA0) { /* @bin 0x004D4180 statues; its flag is never reset between statues */
        wr32(c, G_LOCUST_TIMER, 0);
        int flag = 0;
        for (uint32_t id = 1; id < 4000; id++) {
            uint32_t b = BUILDING_ADDR(id);
            int16_t t = rd16(c, b + BLD_TYPE);
            if (rd8(c, b + BLD_STATE) == 1 && (t == 0x29 || t == 0x2A || t == 0x2B)) {
                int u = rd8(c, b + BLD_ORIENTATION);
                if (u & 0x40) {
                    flag = 1;
                    u -= 0x40;
                }
                int n = u + 1;
                if (n == 4) {
                    n = 0;
                }
                wr8(c, b + BLD_ORIENTATION, flag * 16 + n);
            }
        }
        wr32(c, G_MUMMY_ATTACK_PARAM, 10);
        wr32(c, G_HAIL_TIMER, 0);
        st->upgrades++;
    }
}

static void load_steps(ctx *cp);

int save_file_load_state(game_state *gs, save_file *s, save_load_env *env, save_load_stats *stats)
{
    ctx c = {s, env, stats, gs, {NULL, NULL, NULL, NULL}};
    memset(stats, 0, sizeof(*stats));
    /* @bin 0x004D41E0 before the chunks: 0x00E945C0 := 0xA0, 0x00E945C4 := 0; dword 0 of every record
     * of the tables 0x005E45B0 (10), 0x005E46A0 (18), 0x005C1140 (22), 0x005C1350 (13) kept on the
     * stack (restored after the load: step 7); the empire routes' point buffers released (+0x04 :=
     * 0, +0x138 := 0) */
    static const uint32_t T[4] = {G_WORKSTEPS_OBELISK_SMALL, G_WORKSTEPS_OBELISK_LARGE, G_WORKSTEPS_SPHINX, G_WORKSTEPS_SUN_TEMPLE};
    static const int N[4] = {10, 18, 22, 13};
    int32_t keep[4][22];
    S32(gs, G_VERSION) = 0xA0;
    S32(gs, G_USE_FILE_SCHEMA) = 0;
    for (int t = 0; t < 4; t++) {
        for (int k = 0; k < N[t]; k++) {
            keep[t][k] = S32(gs, T[t] + G_WORKSTEP_STRIDE * (uint32_t) k);
        }
        c.restore[t] = keep[t];
    }
    for (uint32_t k = 0; k < 50; k++) {
        uint32_t r = G_ROUTES + 324u * k;
        if (U32(gs, r + 4) != 0) {
            state_dealloc(gs, U32(gs, r + 4)); /* @bin 0x00561A2B */
            U32(gs, r + 4) = 0;
            U32(gs, r + 0x138) = 0;
        }
    }
    for (uint32_t i = 0; i < s->read_chunks && i < SAVE_SCHEMA_RECORDS; i++) {
        const save_schema_record *t = &save_schema[i];
        size_t n = (size_t) t->elem_size * t->elem_count;
        if (i == 175 && s->read_chunks < SAVE_SCHEMA_RECORDS) {
            /* @bin 0x004D3C40 writes +0..+4 and +8..+11 only */
            state_load_section(gs, t->address, s->chunk[i], 5);
            state_load_section(gs, t->address + 8, s->chunk[i] + 8, n - 8);
        } else {
            state_load_section(gs, t->address, s->chunk[i], n);
        }
    }
    if (s->read_chunks < SAVE_SCHEMA_RECORDS) {
        /* @bin 0x004D3C40 also writes the carry-over records around chunk 175: at 0x006FDCBC,
         * 0x006FDCC8, 0x006FDCD4, 0x006FDCE4 (chunk 175 +4), 0x006FDCF0, 0x006FDCFC: byte := 0xFF,
         * dword +4 := -1, byte +8 := 0; chunk 175 +0 := 0 */
        static const uint32_t E[6] = {G_CARRY + 4u, G_CARRY + 0x10u, G_CARRY + 0x1Cu,
                                      G_CARRY_COPY + 4u, G_CARRY_COPY + 0x10u, G_CARRY_COPY + 0x1Cu};
        for (int k = 0; k < 6; k++) {
            U8(gs, E[k]) = 0xFF;
            S32(gs, E[k] + 4) = -1;
            U8(gs, E[k] + 8) = 0;
        }
        S32(gs, G_CARRY_COPY) = 0;
    }
    /* @bin 0x00562348 the CRT rand() state is the game's variable 0x00604950 */
    env->crt_rand = U32(gs, G_CRT_RAND_STATE);
    load_steps(&c);
    U32(gs, G_CRT_RAND_STATE) = env->crt_rand;
    return 0;
}

int save_route_rebuild_state(game_state *gs, uint32_t route, save_load_env *env)
{
    save_load_stats stats;
    memset(&stats, 0, sizeof(stats));
    ctx c = {NULL, env, &stats, gs, {NULL, NULL, NULL, NULL}};
    return route_rebuild(&c, route);
}

int save_file_load(save_file *s, save_load_env *env, save_load_stats *stats)
{
    ctx c = {s, env, stats, NULL, {NULL, NULL, NULL, NULL}};
    memset(stats, 0, sizeof(*stats));
    if (s->read_chunks < SAVE_SCHEMA_RECORDS) {
        /* chunks 176-180 are not in the file and keep what the game held: here the exe's initial
         * data, i.e. a load right after the game started (save.md §3a) */
        memcpy(s->chunk[176], save_initial_chunk176, sizeof(save_initial_chunk176));
        memcpy(s->chunk[177], save_initial_chunk177, sizeof(save_initial_chunk177));
        memcpy(s->chunk[178], save_initial_chunk178, sizeof(save_initial_chunk178));
        memcpy(s->chunk[179], save_initial_chunk179, sizeof(save_initial_chunk179));
        memcpy(s->chunk[180], save_initial_chunk180, sizeof(save_initial_chunk180));
    }
    load_steps(&c);
    return 0;
}

static void load_steps(ctx *cp)
{
    ctx c = *cp;
    routes(&c);
    upgrades(&c);
    /* @bin 0x004D41E0 step 6: clean-ups for the Cleopatra monuments not present (buildings 1-3999) */
    int caesareum = 0, pharos = 0, library = 0, abu_simbel = 0, tomb = 0;
    for (uint32_t id = 1; id < 4000; id++) {
        if (rd8(&c, BUILDING_ADDR(id) + BLD_STATE) != 1) {
            continue;
        }
        int16_t t = rd16(&c, BUILDING_ADDR(id) + BLD_TYPE);
        caesareum |= t == 0xE3;
        pharos |= t == 0xE4;
        library |= t == 0xE1;
        abu_simbel |= t == 0xE6;
        tomb |= t == 0xE5 || t == 0xEA || t == 0xEB || t == 0xEC;
    }
    if (!caesareum) {
        cleanup(&c, save_cleanup_caesareum, save_cleanup_caesareum_count);
    }
    if (!pharos) {
        cleanup(&c, save_cleanup_pharos, save_cleanup_pharos_count);
    }
    if (!library) {
        cleanup(&c, save_cleanup_library, save_cleanup_library_count);
    }
    if (!abu_simbel) {
        cleanup(&c, save_cleanup_abu_simbel, save_cleanup_abu_simbel_count);
    }
    if (!tomb) {
        cleanup(&c, save_cleanup_tombs, save_cleanup_tombs_count);
    }
    /* @bin 0x004D41E0 step 7: dword 0 of every record of four tables comes back from memory
     * (static data: the exe's initial values) */
    static const struct { int chunk, count; const int32_t *v; } R[4] = {
        {171, 10, save_restore_chunk171}, {172, 18, save_restore_chunk172},
        {146, 22, save_restore_chunk146}, {149, 13, save_restore_chunk149},
    };
    for (int t = 0; t < 4; t++) {
        for (int k = 0; k < R[t].count; k++) {
            wr32(&c, save_schema[R[t].chunk].address + G_WORKSTEP_STRIDE * (uint32_t) k, c.restore[t] ? c.restore[t][k] : R[t].v[k]);
        }
    }
    /* @bin 0x004D41E0 0x00EAAF5C := (0x0078D7A4 > 0): not saved (dropped in chunk mode) */
    wr8(&c, G_GOD_CURSE_ON, rd32(&c, G_PYRAMID_CURSE_COUNTDOWN) > 0);
}
