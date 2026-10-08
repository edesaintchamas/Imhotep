/* Imhotep - small daily steps of the city (day dispatcher 0x00451C50).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "city/day_basic.h"

#include "building/day_access.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"
#include "state/records.h"

#define BUILDING_HIGHEST 0x0078D790u
#define CITY1_FLAG 0x00EA624Au /* u8: city record 1, +0x2 */

void day_clear_counters(game_state *st)
{
    /* @bin 0x0046A060 */
    S32(st, 0x0078D5E0u) = 0;
    S32(st, 0x0078D5DCu) = 0;
    S32(st, 0x0078D5CCu) = 0;
    S32(st, 0x0078D5C8u) = 0;
}

void day_building_countdown(game_state *st)
{
    /* @bin 0x004599A0 the highest id is read once */
    int32_t n = S32(st, BUILDING_HIGHEST);
    for (int32_t id = 1; id <= n; id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) == 1 && U8(st, b + 0x4E) != 0) {
            U8(st, b + 0x4E) -= 1;
        }
    }
}

void day_step_0x10(game_state *st)
{
    /* @bin 0x0045AAA0 */
    if (U8(st, CITY1_FLAG) != 0) {
        day_totals_45aae0(st, 1);
    }
}

void day_totals_45aae0(game_state *st, int32_t player)
{
    /* @bin 0x0045AAE0 two arrays of 36 i32 per city: +0x4860 and +0x48F0 */
    uint32_t c = CITY_ADDR(player);
    for (uint32_t i = 0; i < 0x24; i++) {
        S32(st, c + 0x4860u + 4 * i) = 0;
        S32(st, c + 0x48F0u + 4 * i) = 0;
    }
    /* @bin 0x0045AB25 types 0x48 and 0xC1: +0x44 := 1 with road access, 2 with access by size 3 */
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1 || (S16(st, b + 0x10) != 0x48 && S16(st, b + 0x10) != 0xC1)) {
            continue;
        }
        U8(st, b + 0x44) = 0;
        if (building_road_access(st, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3))) {
            U8(st, b + 0x44) = 1;
        } else if (building_road_access(st, S16(st, b + 6), S16(st, b + 8), 3)) {
            U8(st, b + 0x44) = 2;
        }
    }
    /* @bin 0x0045ABA2 type 0x49 takes +0x44 of its main part; when set, 400 split between the
     * slot +0x12 of the two arrays by +0x3C */
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1 || S16(st, b + 0x10) != 0x49) {
            continue;
        }
        uint8_t v = U8(st, BUILDING_ADDR(building_main_part(st, id)) + 0x44);
        if (v == 0) {
            continue;
        }
        U8(st, b + 0x44) = v;
        int16_t slot = S16(st, b + 0x12);
        if (slot == 0) {
            S32(st, c + 0x4860u) += 400;
        } else {
            int32_t k = S16(st, b + 0x3C);
            uint32_t i = (uint32_t) (0x1276 * player + slot);
            S32(st, 0x00EA60D0u + 4 * i) += 400 - k;
            S32(st, 0x00EA6160u + 4 * i) += k;
        }
    }
    /* @bin 0x0045AC2E three one-time messages of the scenario 0x00E38EA8 */
    if ((int32_t) S8(st, 0x00E38EA8u) == S32(st, 0x005795ACu) && U8(st, 0x007027C1u) == 0 &&
        S32(st, 0x00E38EBCu) == 0 && S32(st, c + 0x4924u) >= 200) {
        GAME_CALL(st, 0x00417AE0u, 9);
    }
    if ((int32_t) S8(st, 0x00E38EA8u) == S32(st, 0x005795C8u) && U8(st, 0x007027C2u) == 0 &&
        S32(st, 0x00E38EBCu) == 0 && S32(st, c + 0x492Cu) >= 300) {
        GAME_CALL(st, 0x00417AE0u, 10);
    }
    if ((int32_t) S8(st, 0x00E38EA8u) == S32(st, 0x0057961Cu) && U8(st, 0x007027C5u) == 0 &&
        U8(st, 0x007027C4u) != 0 && S32(st, 0x00E38EBCu) == 0 && S32(st, c + 0x4920u) >= 800) {
        GAME_CALL(st, 0x00417AE0u, 0xD);
    }
}

void day_step_0x12(game_state *st)
{
    /* @bin 0x0045E6C0 */
    if (U8(st, CITY1_FLAG) != 0) {
        day_totals_45e6e0(st, 1);
    }
}

/* @bin 0x0045E78D the capacity by type (switch on type - 0x20, up to 0xBD) */
static int32_t capacity_45e6e0(int16_t type)
{
    switch (type) {
    case 0x20: case 0x33: return 300;
    case 0x2F: case 0x35: return 500;
    case 0x54: case 0x55: case 0x56: case 0x57: case 0xB8: case 0xBB: case 0xBC: case 0xBD: return 100000;
    default: return 200;
    }
}

void day_totals_45e6e0(game_state *st, int32_t player)
{
    /* @bin 0x0045E6E0 two arrays of 27 i32 per city: +0x3C64 and +0x3D04 */
    uint32_t c = CITY_ADDR(player);
    for (uint32_t i = 0; i < 0x1B; i++) {
        S32(st, c + 0x3C64u + 4 * i) = 0;
        S32(st, c + 0x3D04u + 4 * i) = 0;
    }
    for (int32_t id = 1; id <= S32(st, BUILDING_HIGHEST); id++) {
        uint32_t b = BUILDING_ADDR(id);
        if (U8(st, b) != 1) {
            continue;
        }
        int16_t type = S16(st, b + 0x10);
        if (type < 0x6E || type > 0x72) {
            /* @bin 0x0045E742 the types counted */
            switch (type) {
            case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2:
            case 0xBB: case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2:
            case 0xE7: case 0xE8: case 0xE9:
                break;
            default:
                continue;
            }
        }
        U8(st, b + 0x44) = 0;
        if (!building_road_access(st, S16(st, b + 6), S16(st, b + 8), U8(st, b + 3))) {
            continue;
        }
        U8(st, b + 0x44) = 1;
        int16_t used = S16(st, b + 0x3C);
        int32_t free = capacity_45e6e0(type) - used;
        if (free < 0) {
            free = 0;
        }
        uint32_t i = (uint32_t) (0x1276 * player + S16(st, b + 0x12));
        S32(st, 0x00EA5574u + 4 * i) += free;
        S32(st, 0x00EA54D4u + 4 * i) += used;
        int16_t slot2 = S16(st, b + 0xCE);
        if (slot2 != 0) {
            used = S16(st, b + 0xCC);
            free = capacity_45e6e0(type) - used;
            if (free < 0) {
                free = 0;
            }
            i = (uint32_t) (0x1276 * player + slot2);
            S32(st, 0x00EA5574u + 4 * i) += free;
            S32(st, 0x00EA54D4u + 4 * i) += used;
        }
    }
}

void sound_stop_stream(game_state *st, uint32_t self, int32_t channel)
{
    /* @bin 0x0055F561 with the sound system on (+0x200 == 0) the stream +0x1D8 + 4 * channel is
     * closed (AIL_close_stream: the platform's request, core/sim/platform.c) and its handle cleared */
    (void) self; /* the sound object 0x00E92BF8 */
    platform_stream_stop(st, channel);
}

void map_grid_decay_978d9c(game_state *st)
{
    /* @bin 0x0047E290 every 20 days (counter 0x00BCA344), each 3-bit field of the u32 grid
     * 0x00978D9C over the map's rows (first 0x00BC9804, from 0x00BC9F24 to 0x00BC9B94) down by
     * one; the mask is (7 << shift) ^ 0xFFFF as in the original, which also clears the bits above
     * 16 outside the field */
    S32(st, 0x00BCA344u) += 1;
    if (S32(st, 0x00BCA344u) < 0x14) {
        return;
    }
    S32(st, 0x00BCA344u) = 0;
    for (int32_t row = 0; row < S32(st, 0x00BCA2D4u); row++) {
        int32_t from = S32(st, 0x00BC9F24u + 4u * (uint32_t) row);
        int32_t to = S32(st, 0x00BC9B94u + 4u * (uint32_t) row);
        if (from >= to) {
            continue;
        }
        uint32_t p = 0x00978D9Cu + 4u * (uint32_t) S32(st, 0x00BC9804u + 4u * (uint32_t) row);
        for (int32_t n = to - from; n > 0; n--, p += 4) {
            uint32_t v = U32(st, p);
            if (v != 0) {
                for (uint32_t shift = 0; shift < 30; shift += 3) {
                    uint32_t f = (v >> shift) & 7;
                    if (f != 0) {
                        v = (((7u << shift) ^ 0xFFFFu) & v) | ((f - 1) << shift);
                    }
                }
            }
            U32(st, p) = v;
        }
    }
}

void figure_tile_list_add(game_state *st, int32_t id)
{
    /* @bin 0x004B52F0 the figure joins the list of figures on its tile (+0x20): the head in grid
     * 0x00B350DC, the next one at +0x08; +0x8A its place in the list (at most 20). Every path
     * ends with a call to the empty function 0x0051F780(id). */
    uint32_t f = FIGURE_ADDR(id);
    int32_t tile = S32(st, f + 0x20);
    if (tile >= 0) {
        int32_t head = S16(st, 0x00B350DCu + 2u * (uint32_t) tile);
        U8(st, f + 0x8A) = 0;
        if (head == 0) {
            S16(st, 0x00B350DCu + 2u * (uint32_t) S32(st, f + 0x20)) = (int16_t) id;
        } else {
            U8(st, f + 0x8A) = 1;
            int32_t last = head;
            int found = 0;
            for (int16_t next = S16(st, FIGURE_ADDR(last) + 8); next != 0; next = S16(st, FIGURE_ADDR(last) + 8)) {
                last = next;
                if (last == id) {
                    found = 1;
                    break;
                }
                U8(st, f + 0x8A) += 1;
            }
            if (!found) {
                if (U8(st, f + 0x8A) > 0x14) {
                    U8(st, f + 0x8A) = 0x14;
                }
                S16(st, FIGURE_ADDR(last) + 8) = (int16_t) id;
            }
        }
    }
    GAME_CALL(st, 0x0051F780u, id);
}

void events_tick_44bca0(game_state *st, uint32_t self)
{
    /* @bin 0x0044BCA0 (thiscall on 0x00785D70) unless paused or 0x00784C60 / 0x00784C62 is -1:
     * the first record (count at +0, 0x7C bytes from +4) with byte +2 == 3 for which 0x0044BE30
     * (thiscall on the record) returns non-zero sets +0x48B4 */
    if (S32(st, 0x00605A3Cu) == 1 || S16(st, 0x00784C60u) == -1 || S16(st, 0x00784C62u) == -1) {
        return;
    }
    for (int32_t i = 0; i < (int32_t) P32(st, self); i++) {
        uint32_t rec = self + 4 + 0x7Cu * (uint32_t) i;
        if (P8(st, rec + 2) == 3 && (int8_t) GAME_CALLT0(st, rec, 0x0044BE30u) != 0) {
            P8(st, self + 0x48B4) = 1;
            return;
        }
    }
}

void undo_check_485dc0(game_state *st)
{
    /* @bin 0x00485DC0 every tick while the undo is available (0x00BC9694 and 0x00BC968C == 1):
     * its countdown 0x00BC9688; it is dropped (0x00BC9694 := 0, 0x00E38DCA := 1, buffers
     * cleared) when the countdown ends or 0x0078A624 is set; for most actions (0x00BC9690) also
     * when one of the 0x00BC95F0 saved buildings (ids 0x00BA746C, copies 0x00BA7888 of 0x108
     * bytes) changed */
    if (S32(st, 0x00BC9694u) != 1 || S32(st, 0x00BC968Cu) != 1) {
        return;
    }
    if (S32(st, 0x00BC9688u) <= 0 || U8(st, 0x0078A624u) != 0) {
        S32(st, 0x00BC9694u) = 0;
        U8(st, 0x00E38DCAu) = 1;
        for (uint32_t k = 0; k < 0x21D68; k++) {
            U8(st, 0x00BA7888u + k) = 0;
        }
        for (uint32_t k = 0; k < 0x41A; k++) {
            U8(st, 0x00BA746Cu + k) = 0;
        }
        return;
    }
    S32(st, 0x00BC9688u) -= 1;
    int32_t action = S32(st, 0x00BC9690u);
    if (action == 9 || action == 8 || action == 5 || action == 6 || action == 0xA8 || action == 0xA9 ||
        action == 0x52 || action == 0x53 || action == 0x26 || action == 0x27) {
        return;
    }
    int32_t n = S32(st, 0x00BC95F0u);
    int drop = n <= 0;
    if (!drop && action == 0xA) {
        for (int32_t i = 0; i < n && !drop; i++) {
            int16_t id = S16(st, 0x00BA746Cu + 2u * (uint32_t) i);
            drop = id != 0 && S16(st, BUILDING_ADDR(id) + 0x1C) != 0;
        }
    }
    for (int32_t i = 0; i < n && !drop; i++) {
        int16_t id = S16(st, 0x00BA746Cu + 2u * (uint32_t) i);
        if (id == 0) {
            continue;
        }
        uint32_t b = BUILDING_ADDR(id), copy = 0x00BA7888u + 0x108u * (uint32_t) i;
        uint8_t state = U8(st, b);
        drop = state == 2 || state == 4 || state == 5 || S32(st, b + 0xC) != S32(st, copy + 0xC) ||
               S16(st, b + 0x10) != S16(st, copy + 0x10);
    }
    if (drop) {
        S32(st, 0x00BC9694u) = 0;
        U8(st, 0x00E38DCAu) = 1;
    }
}

#define VOID0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
#define VOID1(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        fn(st, a[0]); \
        return 0; \
    }
VOID0(run_clear_counters, day_clear_counters)
VOID0(run_building_countdown, day_building_countdown)
VOID0(run_step_0x10, day_step_0x10)
VOID1(run_totals_45aae0, day_totals_45aae0)
VOID0(run_step_0x12, day_step_0x12)
VOID1(run_totals_45e6e0, day_totals_45e6e0)
VOID0(run_grid_decay, map_grid_decay_978d9c)
VOID0(run_undo_check, undo_check_485dc0)

static int32_t run_events_tick(game_state *st, const int32_t *a)
{
    (void) a;
    events_tick_44bca0(st, 0x00785D70u);
    return 0;
}
VOID1(run_tile_list_add, figure_tile_list_add)

static int32_t run_sound_stop_stream(game_state *st, const int32_t *a)
{
    sound_stop_stream(st, 0x00E92BF8u, a[0]);
    return 0;
}

const ported_function day_basic_ports[] = {
    {0x0046A060u, "day_clear_counters", 0, 0, 0, run_clear_counters, 0},
    {0x004599A0u, "day_building_countdown", 0, 0, 0, run_building_countdown, 0},
    {0x0045AAA0u, "day_step_0x10", 0, 0, 0, run_step_0x10, 0},
    {0x0045AAE0u, "day_totals_45aae0", 1, 0, 0, run_totals_45aae0, 0},
    {0x0045E6C0u, "day_step_0x12", 0, 0, 0, run_step_0x12, 0},
    {0x0045E6E0u, "day_totals_45e6e0", 1, 0, 0, run_totals_45e6e0, 0},
    {0x0047E290u, "map_grid_decay_978d9c", 0, 0, 0, run_grid_decay, 0},
    {0x004B52F0u, "figure_tile_list_add", 1, 0, 0, run_tile_list_add, 0},
    {0x0044BCA0u, "events_tick_44bca0", 0, 0, 0x00785D70u, run_events_tick, 0},
    {0x00485DC0u, "undo_check_485dc0", 0, 0, 0, run_undo_check, 0},
    {0x0055F561u, "sound_stop_stream", 1, 0, 0x00E92BF8u, run_sound_stop_stream, 0},
};
const int day_basic_ports_count = sizeof(day_basic_ports) / sizeof(day_basic_ports[0]);
