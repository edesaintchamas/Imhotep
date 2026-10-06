/* Imhotep - storage orders of the building window (storage yards, granaries, bazaars).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 * docs/commands.md section 3.5. The window's resource lists (0x00F53DD8 count + 0x00F53DDC[],
 * 0x00F53E6C count + 0x00F53E70[]) are built by the window code: they are command inputs. */
#include "command/orders.h"

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

#define WINDOW_BUILDING 0x00EADD6Cu   /* i32 the building of the info window */
#define LIST_A 0x00F53DD8u            /* i32 count, then i32 resources (granaries, bazaars) */
#define LIST_B 0x00F53E6Cu            /* i32 count, then i32 resources (storage yards) */
#define STORAGE_ORDERS 0x007900C2u    /* u8 order per resource, storage record stride 0xC4 */
#define STORAGE_ACCEPT 0x007900ECu    /* i16 fill limit per resource (order 0) */
#define STORAGE_GET 0x00790134u       /* i16 fill limit per resource (other orders) */
#define SCREEN_MODE 0x00E38E68u

static int is_yard(int16_t t) { return t == 0x48 || t == 0xC1; }
static int is_bazaar(int16_t t) { return t == 0x46 || t == 0xBE; }

int32_t storage_rows_visible(game_state *st)
{
    /* @bin 0x00502780 rows of the resource list by screen mode 0x00E38E68 (1: 15, 2: 21, 3: 24, else 10) */
    switch (S32(st, SCREEN_MODE)) {
    case 1:
        return 0xF;
    case 2:
        return 0x15;
    case 3:
        return 0x18;
    default:
        return 0xA;
    }
}

/* @bin 0x005013A0 (storage_kind, inline) goods 1..8, 0xD, 0xF, 0x11, 0x13 */
static int storage_kind(int32_t g)
{
    return (g >= 1 && g <= 8) || g == 0xD || g == 0xF || g == 0x11 || g == 0x13;
}

void cmd_storage_cycle(game_state *st)
{
    /* @bin 0x00406300 the order of the row p1's resource */
    int32_t id = S32(st, WINDOW_BUILDING);
    uint32_t b = BUILDING_ADDR(id);
    uint32_t sid = U8(st, b + 0xC1);
    int16_t type = S16(st, b + 0x10);
    int32_t res;
    if (is_yard(type)) {
        /* @bin 0x0040638F scroll row +0xE0: 0 -> 1, above 14 -> list count - 13 */
        int32_t scroll = S32(st, b + 0xE0);
        if (scroll == 0) {
            S32(st, b + 0xE0) = 1;
        } else if (scroll > 0xE) {
            S32(st, b + 0xE0) = S32(st, LIST_B) - 0xD;
        }
        res = S32(st, LIST_B - 4 + 4u * (uint32_t) (S32(st, b + 0xE0) + S32(st, CMD_PARAM1)));
    } else if (is_bazaar(type)) {
        /* @bin 0x00406350 rows 1.. of the 36 entries of 0x00F53E70 that are storable goods; a row
         * past them reads an unset stack slot in the original: the port leaves the state as is */
        int32_t kept[36];
        int n = 0;
        for (uint32_t i = 0; i < 36; i++) {
            int32_t g = S32(st, LIST_B + 4 + 4 * i);
            if (storage_kind(g)) {
                kept[n++] = g;
            }
        }
        int32_t row = S32(st, CMD_PARAM1) - 1;
        if (row < 0 || row >= n) {
            state_unported(st, 0x00406300u);
            return;
        }
        res = kept[row];
    } else {
        res = S32(st, LIST_A + 4u * (uint32_t) S32(st, CMD_PARAM1));
    }
    /* @bin 0x004063E8 jump table 0x00406450: 0 -> 2 (bazaars 1), 1 -> 0, 2 -> 3, 3 -> 1 */
    uint32_t order = STORAGE_ORDERS + (uint32_t) res + 0xC4u * sid;
    switch (U8(st, order)) {
    case 0:
        U8(st, order) = is_bazaar(S16(st, BUILDING_ADDR(S32(st, WINDOW_BUILDING)) + 0x10)) ? 1 : 2;
        break;
    case 1:
        U8(st, order) = 0;
        break;
    case 2:
        U8(st, order) = 3;
        break;
    case 3:
        U8(st, order) = 1;
        break;
    default:
        break;
    }
    U8(st, UI_REDRAW) = 1;
}

void cmd_storage_limit(game_state *st)
{
    /* @bin 0x00406460 fill limit of the row p2's resource: p1 1: -800 (at least 800), 0: +800 (at
     * most 3200) */
    int32_t id = S32(st, WINDOW_BUILDING);
    uint32_t b = BUILDING_ADDR(id);
    uint32_t sid = U8(st, b + 0xC1);
    if (S32(st, b + 0xE0) == 0) {
        S32(st, b + 0xE0) = 1;
    } else if (S32(st, b + 0xE0) > storage_rows_visible(st)) {
        int32_t rows = storage_rows_visible(st);
        S32(st, b + 0xE0) = S32(st, LIST_B) - rows + 1;
    }
    int32_t res;
    if (is_yard(S16(st, b + 0x10))) {
        res = S32(st, LIST_B + 4u * (uint32_t) (S32(st, b + 0xE0) + S32(st, CMD_PARAM2)));
    } else {
        res = S32(st, LIST_A + 4 + 4u * (uint32_t) S32(st, CMD_PARAM2));
    }
    int32_t p1 = S32(st, CMD_PARAM1);
    if (p1 == 1 || p1 == 0) {
        uint32_t base = U8(st, STORAGE_ORDERS + (uint32_t) res + 0xC4u * sid) == 0 ? STORAGE_ACCEPT : STORAGE_GET;
        uint32_t f = base + 2u * (uint32_t) res + 0xC4u * sid;
        int32_t v = S16(st, f);
        if (p1 == 1) {
            v -= 0x320;
            if (v < 0x320) {
                v = 0x320;
            }
        } else {
            v += 0x320;
            if (v > 0xC80) {
                v = 0xC80;
            }
        }
        S16(st, f) = (int16_t) v;
    }
    U8(st, UI_REDRAW) = 1;
}

void cmd_storage_all(game_state *st)
{
    /* @bin 0x00406620 the first (count 0x00F53DD8) resources of 0x00F53E70[] get order 1 */
    uint32_t sid = U8(st, BUILDING_ADDR(S32(st, WINDOW_BUILDING)) + 0xC1);
    int32_t n = S32(st, LIST_A);
    for (int32_t i = 0; i < n; i++) {
        U8(st, STORAGE_ORDERS + 0xC4u * sid + U32(st, LIST_B + 4 + 4u * (uint32_t) i)) = 1;
    }
    U8(st, UI_REDRAW) = 1;
}

void cmd_storage_special(game_state *st)
{
    /* @bin 0x00406680 p1 1: city +0x4674 := the building; p1 2: the listed resources of
     * 0x00F53E70[] (count 0x00F53E6C) get order 1 */
    int32_t id = S32(st, WINDOW_BUILDING);
    uint32_t sid = U8(st, BUILDING_ADDR(id) + 0xC1);
    int32_t p1 = S32(st, CMD_PARAM1);
    if (p1 == 1) {
        S32(st, CURRENT_CITY(st) + 0x4674) = id;
    } else if (p1 == 2) {
        int32_t n = S32(st, LIST_B);
        for (int32_t i = 0; i < n; i++) {
            U8(st, STORAGE_ORDERS + 0xC4u * sid + U32(st, LIST_B + 4 + 4u * (uint32_t) i)) = 1;
        }
    }
    U8(st, UI_REDRAW) = 1;
}

void cmd_storage_toggle_dc(game_state *st)
{
    /* @bin 0x0040C630 building +0xDC: 1 <-> 0x10 (other values unchanged) */
    uint32_t f = BUILDING_ADDR(S32(st, WINDOW_BUILDING)) + 0xDC;
    if (S32(st, f) == 1) {
        S32(st, f) = 0x10;
    } else if (S32(st, f) == 0x10) {
        S32(st, f) = 1;
    }
    U8(st, UI_REDRAW) = 1;
}

#define V(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
V(run_cycle, cmd_storage_cycle)
V(run_limit, cmd_storage_limit)
V(run_all, cmd_storage_all)
V(run_special, cmd_storage_special)
V(run_toggle_dc, cmd_storage_toggle_dc)

static int32_t run_rows(game_state *st, const int32_t *a)
{
    (void) a;
    return storage_rows_visible(st);
}

const ported_function orders_storage_ports[] = {
    {0x00406300u, "cmd_storage_cycle", 0, 0, 0, run_cycle, 0},
    {0x00406460u, "cmd_storage_limit", 0, 0, 0, run_limit, 0},
    {0x00406620u, "cmd_storage_all", 0, 0, 0, run_all, 0},
    {0x00406680u, "cmd_storage_special", 0, 0, 0, run_special, 0},
    {0x0040C630u, "cmd_storage_toggle_dc", 0, 0, 0, run_toggle_dc, 0},
    {0x00502780u, "storage_rows_visible", 0, 32, 0, run_rows, 0},
};
const int orders_storage_ports_count = sizeof(orders_storage_ports) / sizeof(orders_storage_ports[0]);
