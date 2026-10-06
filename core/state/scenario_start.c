/* Imhotep - scenario start (Pharaoh.exe 0x004D52A0) and its own steps.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0. */
#include "state/scenario_start.h"

#include <string.h>

#include "map/floodplain.h"
#include "sim/call.h"
#include "state/ported.h"
#include "state/pre_load.h"

#define MAP_WIDTH 0x00BCA2D0u
#define MAP_HEIGHT 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define MAP_ROW_SKIP 0x00BCA2DCu
#define ROW_FIRST 0x00BC9F24u  /* per map row: first column, last column + 1, grid offset */
#define ROW_END 0x00BC9B94u
#define ROW_OFFSET 0x00BC9804u
#define CURRENT_PLAYER 0x00EAAC28u
#define CITY(st) (0x00EA1870u + 0x49D8u * U32(st, CURRENT_PLAYER))
#define BUILDING(id) (0x007999D8u + 0x108u * (uint32_t) (id))
#define FIGURE(id) (0x00BCD5F8u + 0x184u * (uint32_t) (id))
#define EMPIRE_OBJECT(i) (0x0077F748u + 0x62u * (uint32_t) (i))

void scenario_start_map(game_state *st)
{
    /* @bin 0x004D52A0 image grid (chunk 7, 0x32C40 bytes) := 0 (0x0040F8F0) */
    memset(state_ptr(st, 0x00B7482Cu, 0x32C40u), 0, 0x32C40u);
    /* @bin 0x004D52A0 0x00784F10 := (i16) scenario info +0x04, sign-extended (meaning open) */
    S32(st, 0x00784F10u) = S16(st, 0x007848DCu);
    /* @bin 0x004D52A0 width, height, start offset, row skip from scenario info +0x2C..+0x38 */
    S32(st, MAP_WIDTH) = S32(st, 0x00784904u);
    S32(st, MAP_HEIGHT) = S32(st, 0x00784908u);
    S32(st, MAP_START) = S32(st, 0x0078490Cu);
    S32(st, MAP_ROW_SKIP) = S32(st, 0x00784910u);
    map_row_ranges(st);
}

void map_row_ranges(game_state *st)
{
    /* @bin 0x0046F7B0 (C division truncates toward zero, as IDIV) */
    int32_t w = S32(st, MAP_WIDTH), h = S32(st, MAP_HEIGHT);
    int32_t x0 = (0xE4 - w) / 2, y0 = (0xE4 - h) / 2;
    int32_t x1 = x0 - 1 + w, y1 = y0 - 1 + h;
    int32_t cx = (x1 + x0) / 2, cy = (y1 + y0) / 2;
    S32(st, 0x00BCA2F0u) = x1;
    S32(st, 0x00BCA2F4u) = y1;
    S32(st, 0x00BCA2F8u) = cx;
    S32(st, 0x00BCA2FCu) = cy;
    S32(st, 0x00BCA2E8u) = x0;
    S32(st, 0x00BCA2ECu) = y0;
    int32_t a = (cy - y0) + x0;
    for (int32_t y = y0; y <= y1; y++, a--) {
        int32_t lo = y > cy ? (x0 - cy) - 1 + y : a;
        uint32_t k = 4u * (uint32_t) (y - y0);
        S32(st, 0x00BC9F24u + k) = lo - x0;
        S32(st, 0x00BC9B94u + k) = ((cx * 2 - lo) + 2) - x0;
        S32(st, 0x00BC9804u + k) = lo + y * 0xE4;
    }
}

/* ---- the steps of the scenario start ported here ------------------------------------------ */

void terrain_flag10_to_1(game_state *st)
{
    /* @bin 0x004771C0 every map tile: terrain bit 0x10 -> bit 0x01 (height read once) */
    int32_t h = S32(st, MAP_HEIGHT);
    for (int32_t row = 0; row < h; row++) {
        int32_t lo = S32(st, ROW_FIRST + 4u * (uint32_t) row), hi = S32(st, ROW_END + 4u * (uint32_t) row);
        uint32_t o = U32(st, ROW_OFFSET + 4u * (uint32_t) row);
        for (int32_t n = lo; n < hi; n++, o++) {
            uint32_t t = U32(st, 0x00AF598Cu + 4u * o);
            if (t & 0x10) {
                U32(st, 0x00AF598Cu + 4u * o) = (t & ~0x10u) | 1u;
            }
        }
    }
}

void monument_slots_images(game_state *st)
{
    /* @bin 0x004E59A0 0x004E5950(i) for the three monument slots */
    for (int32_t i = 0; i < 3; i++) {
        GAME_CALL(st, 0x004E5950u, i);
    }
}

void buildings_map_add_all(game_state *st)
{
    /* @bin 0x00485F70 every building in use (1..3999): size := 0x005D06DC[type * 5] (also into
     * 0x00BC96D0), building_map_add(player, type, id, x, y, size, 0, 0) */
    for (int32_t id = 1; id < 4000; id++) {
        uint32_t b = BUILDING(id);
        if (U8(st, b) == 0) {
            continue;
        }
        int32_t type = S16(st, b + 0x10);
        int32_t size = S32(st, 0x005D06DCu + 20u * (uint32_t) type);
        S32(st, 0x00BC96D0u) = size;
        GAME_CALL(st, 0x00470990u, S32(st, CURRENT_PLAYER), type, id, S16(st, b + 6), S16(st, b + 8), size, 0, 0);
    }
    /* @bin 0x00485F70 figures 1..1999 in use: the chain of +0x08 links is cut where it comes back
     * to the figure (the link pointing to it := 0) */
    for (int32_t id = 1; id < 2000; id++) {
        uint32_t f = FIGURE(id);
        if (U8(st, f + 0x0E) == 0) {
            continue;
        }
        int32_t prev = id, next = S16(st, f + 0x08);
        while (next != 0) {
            if (next == id) {
                S16(st, FIGURE(prev) + 0x08) = 0;
                break;
            }
            prev = next;
            next = S16(st, FIGURE(next) + 0x08);
        }
    }
}

void buildings_scale_0x54(game_state *st)
{
    /* @bin 0x00489840 buildings 1..3999 of the kind 0x00488030 (this = 0x00BCA350): i16 +0x54
     * (industry progress) *= 10 (the double 10.0 at 0x0056F5A8 through __ftol; 16-bit imul) */
    for (int32_t id = 1; id < 4000; id++) {
        if ((uint8_t) GAME_CALLT(st, 0x00BCA350u, 0x00488030u, id) != 0) {
            S16(st, BUILDING(id) + 0x54) = (int16_t) (S16(st, BUILDING(id) + 0x54) * 10);
        }
    }
}

void figure_slots_check(game_state *st)
{
    /* @bin 0x004B32C0 i16 slots 0x00D04E5A + 2 * (k - 1), k = 1..: a figure id in 1..1999 whose
     * figure is not in use (+0x0E != 1) or whose +0x3A is not k := 0 */
    int32_t k = 1;
    for (uint32_t p = 0x00D04E5Au; p < 0x00D05628u; p += 2, k++) {
        int32_t id = S16(st, p);
        if (id <= 0 || id >= 2000) {
            continue;
        }
        if (U8(st, FIGURE(id) + 0x0E) != 1) {
            S16(st, p) = 0;
        }
        if (S16(st, FIGURE(id) + 0x3A) != k) {
            S16(st, p) = 0;
        }
    }
}

void terrain_images_flag(game_state *st)
{
    /* @bin 0x00476750 tiles whose image id is in [0x00F565F4, +7]: terrain |= 0x20, byte 0x00B285CC
     * &= 0x7F, byte 0x00AE8E7C := 0 */
    int32_t base = S32(st, 0x00F565F4u);
    for (int32_t row = 0; row < S32(st, MAP_HEIGHT); row++) {
        int32_t lo = S32(st, ROW_FIRST + 4u * (uint32_t) row), hi = S32(st, ROW_END + 4u * (uint32_t) row);
        uint32_t o = U32(st, ROW_OFFSET + 4u * (uint32_t) row);
        for (int32_t n = lo; n < hi; n++, o++) {
            int32_t image = S32(st, 0x00B7482Cu + 4u * o);
            if (image >= base && image <= base + 7) {
                U32(st, 0x00AF598Cu + 4u * o) |= 0x20u;
                U8(st, 0x00B285CCu + o) &= 0x7F;
                U8(st, 0x00AE8E7Cu + o) = 0;
            }
        }
    }
}

void figures_count_5d_64(game_state *st)
{
    /* @bin 0x004BCA90 city +0x49A0 := figures 1..1999 in use of types 0x5D and 0x64 */
    uint32_t c = CITY(st) + 0x49A0u;
    S32(st, c) = 0;
    for (int32_t id = 1; id < 2000; id++) {
        uint8_t type = U8(st, FIGURE(id) + 0x0A);
        if ((type == 0x5D || type == 0x64) && U8(st, FIGURE(id) + 0x0E) != 0) {
            S32(st, c) += 1;
        }
    }
}

void empire_objects_images(game_state *st)
{
    /* @bin 0x00442660 empire objects in use: kind 1 -> image := 0x004425C0(i); kind 0 -> image :=
     * +0x0E + 0x00F56A0C, at most 0x00F56A14 (unsigned 16-bit compare, signed with the limit) */
    for (int32_t i = 0; i < 200; i++) {
        uint32_t o = EMPIRE_OBJECT(i);
        if (U8(st, o + 1) == 0) {
            continue;
        }
        if (U8(st, o) == 1) {
            U16(st, o + 0x0C) = (uint16_t) GAME_CALL(st, 0x004425C0u, i);
        } else if (U8(st, o) == 0) {
            uint16_t v = (uint16_t) (U16(st, o + 0x0E) + U16(st, 0x00F56A0Cu));
            int32_t limit = S32(st, 0x00F56A14u);
            U16(st, o + 0x0C) = v;
            if ((int32_t) v > limit) {
                U16(st, o + 0x0C) = (uint16_t) limit;
            }
        }
    }
}

void empire_objects_sizes(game_state *st)
{
    /* @bin 0x004426C0 empire objects in use of kind 1: image := 0x004425C0(i), width / height (+0x08,
     * +0x0A) from the image table (0x010D6A0C + 64 * image; image 0 when >= 0x0126517C) */
    for (int32_t i = 0; i < 200; i++) {
        uint32_t o = EMPIRE_OBJECT(i);
        if (U8(st, o + 1) == 0 || U8(st, o) != 1) {
            continue;
        }
        uint32_t image = (uint16_t) GAME_CALL(st, 0x004425C0u, i);
        U16(st, o + 0x0C) = (uint16_t) image;
        int32_t count = S32(st, 0x0126517Cu);
        uint32_t k = (int32_t) image < count ? image : 0;
        U16(st, o + 0x08) = U16(st, 0x010D6A0Cu + 64u * k);
        U16(st, o + 0x0A) = U16(st, 0x010D6A0Eu + 64u * k);
    }
}

int32_t buildings_count_b6(game_state *st)
{
    /* @bin 0x0046F190 buildings 1..[0x0078D790] in use (== 1) of type 0xB6 */
    int32_t n = S32(st, 0x0078D790u), count = 0;
    for (int32_t id = 1; id <= n; id++) {
        if (U8(st, BUILDING(id)) == 1 && S16(st, BUILDING(id) + 0x10) == 0xB6) {
            count++;
        }
    }
    return count;
}

static int16_t storage_clamp(int16_t v)
{
    /* @bin 0x004D5AD0 up to 800, 1600, 2400 or 3200 (signed); above stays */
    if (v <= 800) {
        return 800;
    }
    if (v <= 1600) {
        return 1600;
    }
    if (v <= 2400) {
        return 2400;
    }
    if (v <= 3200) {
        return 3200;
    }
    return v;
}

void storage_yards_check(game_state *st)
{
    /* @bin 0x004D5AD0 buildings 0..3999 in use (== 1) of type 0x47: +0x56 := max(0, 3200 - the sum
     * of the 35 i16 at +0x58); the storage record (+0xC1) +0x36 and +0x7E (35 i16 each) rounded up */
    for (int32_t id = 0; id < 4000; id++) {
        uint32_t b = BUILDING(id);
        if (U8(st, b) != 1 || S16(st, b + 0x10) != 0x47) {
            continue;
        }
        int32_t sum = 0;
        for (uint32_t k = 0; k < 35; k++) {
            sum += S16(st, b + 0x58 + 2 * k);
        }
        int32_t free = 3200 - sum;
        S16(st, b + 0x56) = (int16_t) (free < 0 ? 0 : free);
        uint32_t s = 0x007900B8u + 0xC4u * U8(st, b + 0xC1);
        for (uint32_t k = 0; k < 35; k++) {
            S16(st, s + 0x36 + 2 * k) = storage_clamp(S16(st, s + 0x36 + 2 * k));
            S16(st, s + 0x7E + 2 * k) = storage_clamp(S16(st, s + 0x7E + 2 * k));
        }
    }
}

void city_building_d1_position(game_state *st)
{
    /* @bin 0x004D5BC0 the first building (0..3999) in use of type 0xD1: city +0x47A0, +0x47A4 := its
     * x, y (i16 +0x06, +0x08); none: -1, -1 */
    uint32_t c = CITY(st);
    for (int32_t id = 0; id < 4000; id++) {
        if (U8(st, BUILDING(id)) == 1 && S16(st, BUILDING(id) + 0x10) == 0xD1) {
            S32(st, c + 0x47A0u) = S16(st, BUILDING(id) + 6);
            S32(st, c + 0x47A4u) = S16(st, BUILDING(id) + 8);
            return;
        }
    }
    S32(st, c + 0x47A0u) = -1;
    S32(st, c + 0x47A4u) = -1;
}

void figures_check_kind6(game_state *st)
{
    /* @bin 0x004A3560 figures 1..1999 in use with +0x38 == 6: the first record (0x009784D8, 50 x
     * 0x2C, in use at +0) listing the figure among its 4 ids (+0x04..) must have a figure in use of
     * type 0x4C whose +0x11C is the record's index; else (or no record) +0x0E := 2 */
    for (int32_t id = 1; id < 2000; id++) {
        uint32_t f = FIGURE(id);
        if (U8(st, f + 0x0E) == 0 || U8(st, f + 0x38) != 6) {
            continue;
        }
        int32_t found = -1;
        for (int32_t r = 0; r < 50 && found < 0; r++) {
            uint32_t rec = 0x009784D8u + 0x2Cu * (uint32_t) r;
            if (U8(st, rec) == 0) {
                continue;
            }
            for (uint32_t k = 0; k < 4; k++) {
                if (S32(st, rec + 4 + 4 * k) == id) {
                    found = r;
                    break;
                }
            }
        }
        int keep = 0;
        for (int32_t g = 1; found >= 0 && g < 2000; g++) {
            uint32_t q = FIGURE(g);
            if (U8(st, q + 0x0E) != 0 && U8(st, q + 0x0A) == 0x4C && S32(st, q + 0x11C) == found) {
                keep = 1;
                break;
            }
        }
        if (!keep) {
            U8(st, f + 0x0E) = 2;
        }
    }
}

void scenario_start(game_state *st)
{
    /* @bin 0x004D52A0, in order (edi = 0 and ebx = 1 throughout) */
    scenario_start_map(st);
    GAME_CALL0(st, 0x00540C90u); /* building model of the difficulty */
    GAME_CALL0(st, 0x00540ED0u); /* figure model */
    GAME_CALL0(st, 0x00541160u); /* tax and sentiment model */
    GAME_CALL0(st, 0x004F4520u); /* graphics slots for the monuments (and the pyramid kind) */
    GAME_CALL0(st, 0x0053D970u);
    GAME_CALL0(st, 0x004BC720u); /* enemy models */
    terrain_flag10_to_1(st);     /* 0x004771C0 */
    /* @bin 0x004D52A0 16 map points (x at 0x00784BC0, y at 0x00784BE0, i16, scenario info): a point
     * outside the map (0x0046F930) becomes (-1, -1); the test runs twice */
    for (uint32_t i = 0; i < 16; i++) {
        for (int k = 0; k < 2; k++) {
            int32_t x = S32(st, 0x00BCA2E8u) + S16(st, 0x00784BC0u + 2 * i);
            int32_t y = S32(st, 0x00BCA2ECu) + S16(st, 0x00784BE0u + 2 * i);
            if (!map_tile_inside(st, x, y)) {
                S16(st, 0x00784BC0u + 2 * i) = -1;
                S16(st, 0x00784BE0u + 2 * i) = -1;
            }
        }
    }
    GAME_CALL0(st, 0x004E7E20u); /* city flag 0x00EA1820 */
    GAME_CALL0(st, 0x004677C0u); /* industry and building counts */
    GAME_CALL0(st, 0x00468170u);
    /* @bin 0x004D52A0 view orientation 0..6, else 0; 0x00BCA2E0 := it */
    if (S32(st, 0x00E94E04u) < 0 || S32(st, 0x00E94E04u) > 6) {
        S32(st, 0x00E94E04u) = 0;
    }
    S32(st, 0x00BCA2E0u) = S32(st, 0x00E94E04u);
    monument_slots_images(st);   /* 0x004E59A0 */
    buildings_map_add_all(st);   /* 0x00485F70 */
    if (S32(st, 0x00E945C0u) < 0x8D) { /* @bin 0x004D52A0 file versions below 0x8D */
        U8(st, 0x00BCA30Cu) = 1;
        GAME_CALL0(st, 0x00486E70u);
    }
    if (S32(st, 0x00E945C0u) < 0x8E) {
        buildings_scale_0x54(st); /* 0x00489840 */
    }
    GAME_CALL0(st, 0x004DEA50u); /* terrain rebuild */
    GAME_CALLT(st, 0x00D05720u, 0x004BE160u, 0, 0, S32(st, MAP_WIDTH) - 1, S32(st, MAP_HEIGHT) - 1); /* floodplain */
    GAME_CALL(st, 0x00430530u, S32(st, 0x00E94E04u), 1); /* view tables */
    GAME_CALL0(st, 0x004DDF90u);
    GAME_CALL0(st, 0x004DE080u);
    GAME_CALL0(st, 0x00517A20u); /* routing grids */
    GAME_CALL0(st, 0x005178B0u);
    GAME_CALL0(st, 0x00517370u);
    GAME_CALL0(st, 0x00517660u);
    GAME_CALL0(st, 0x005178E0u);
    figure_slots_check(st);      /* 0x004B32C0 */
    GAME_CALL0(st, 0x00468860u);
    GAME_CALL0(st, 0x00457AF0u);
    GAME_CALL0(st, 0x0045E040u);
    GAME_CALL0(st, 0x00404350u);
    GAME_CALL0(st, 0x004051A0u);
    GAME_CALL0(st, 0x004E1830u); /* message timer (reads the clock) */
    GAME_CALL0(st, 0x0041D730u); /* ambient sounds */
    GAME_CALL0(st, 0x0041DF60u);
    S32(st, 0x00BC9694u) = 0;
    S32(st, 0x00E94E0Cu) = 0;
    S32(st, 0x00E94E10u) = 0;
    S32(st, 0x007027A8u) = 1; /* @bin 0x00417E70 (dword store) */
    uint32_t city = CITY(st);
    S32(st, city + 0x4724u) = 1; /* 0x00EA5F94 */
    S32(st, city + 0x4720u) = 1; /* 0x00EA5F90 */
    terrain_images_flag(st);     /* 0x00476750 */
    GAME_CALL0(st, 0x0046D840u);
    S32(st, 0x00E9511Cu) = 0;
    S32(st, 0x00E95120u) = 0;
    U8(st, 0x00E38E60u) = 0;
    GAME_CALL0(st, 0x004BC7A0u);
    figures_count_5d_64(st);     /* 0x004BCA90 */
    U8(st, 0x00EAAC2Cu) = 1;
    empire_objects_images(st);   /* 0x00442660 */
    empire_objects_sizes(st);    /* 0x004426C0 */
    S32(st, 0x00BCB9B0u) = buildings_count_b6(st); /* 0x0046F190 */
    storage_yards_check(st);     /* 0x004D5AD0 */
    city_building_d1_position(st); /* 0x004D5BC0 */
    if (S32(st, 0x00E38EBCu) != 0) {
        S32(st, 0x0078D828u) = 0;
    }
    S32(st, 0x00E945C0u) = 0xA0;
    GAME_CALL0(st, 0x00443330u);
    GAME_CALL0(st, 0x005596D0u); /* goods available */
    GAME_CALL0(st, 0x0045AA30u);
    GAME_CALL0(st, 0x004F61E0u);
    figures_check_kind6(st);     /* 0x004A3560 */
}

/* ---- registered ports ---------------------------------------------------------------------- */

#define RUN0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
RUN0(run_scenario_start, scenario_start)
RUN0(run_terrain_flag10_to_1, terrain_flag10_to_1)
RUN0(run_monument_slots_images, monument_slots_images)
RUN0(run_buildings_map_add_all, buildings_map_add_all)
RUN0(run_buildings_scale_0x54, buildings_scale_0x54)
RUN0(run_figure_slots_check, figure_slots_check)
RUN0(run_terrain_images_flag, terrain_images_flag)
RUN0(run_figures_count_5d_64, figures_count_5d_64)
RUN0(run_empire_objects_images, empire_objects_images)
RUN0(run_empire_objects_sizes, empire_objects_sizes)
RUN0(run_storage_yards_check, storage_yards_check)
RUN0(run_city_building_d1_position, city_building_d1_position)
RUN0(run_figures_check_kind6, figures_check_kind6)

static int32_t run_flag_7027a8(game_state *st, const int32_t *a)
{
    (void) a;
    S32(st, 0x007027A8u) = 1; /* @bin 0x00417E70 */
    return 0;
}

static int32_t run_buildings_count_b6(game_state *st, const int32_t *a)
{
    (void) a;
    return buildings_count_b6(st);
}

static int32_t run_floodplain_object_create(game_state *st, const int32_t *a)
{
    /* @bin 0x004BD740 (this = a depth list of the floodplain manager), 5 stack arguments */
    return (int32_t) floodplain_object_create(st, (uint32_t) a[PORT_THIS_ARG], a[0], a[1], a[2], a[3], (uint8_t) a[4]);
}

const ported_function scenario_ports[] = {
    {0x004D52A0u, "scenario_start", 0, 0, 0, run_scenario_start, 0},
    {0x004771C0u, "terrain_flag10_to_1", 0, 0, 0, run_terrain_flag10_to_1, 0},
    {0x004E59A0u, "monument_slots_images", 0, 0, 0, run_monument_slots_images, 0},
    {0x00485F70u, "buildings_map_add_all", 0, 0, 0, run_buildings_map_add_all, 0},
    {0x00489840u, "buildings_scale_0x54", 0, 0, 0, run_buildings_scale_0x54, 0},
    {0x004B32C0u, "figure_slots_check", 0, 0, 0, run_figure_slots_check, 0},
    {0x00417E70u, "flag_7027a8_set", 0, 0, 0, run_flag_7027a8, 0},
    {0x00476750u, "terrain_images_flag", 0, 0, 0, run_terrain_images_flag, 0},
    {0x004BCA90u, "figures_count_5d_64", 0, 0, 0, run_figures_count_5d_64, 0},
    {0x00442660u, "empire_objects_images", 0, 0, 0, run_empire_objects_images, 0},
    {0x004426C0u, "empire_objects_sizes", 0, 0, 0, run_empire_objects_sizes, 0},
    {0x0046F190u, "buildings_count_b6", 0, 32, 0, run_buildings_count_b6, 0},
    {0x004D5AD0u, "storage_yards_check", 0, 0, 0, run_storage_yards_check, 0},
    {0x004D5BC0u, "city_building_d1_position", 0, 0, 0, run_city_building_d1_position, 0},
    {0x004A3560u, "figures_check_kind6", 0, 0, 0, run_figures_check_kind6, 0},
    {0x004BD740u, "floodplain_object_create (map/floodplain.c)", 5, 32, 0, run_floodplain_object_create, 0},
    {0x004BD610u, "floodplain_object_init (in floodplain_object_create)", 6, 32, 0, NULL, 0},
};
const int scenario_ports_count = sizeof(scenario_ports) / sizeof(scenario_ports[0]);
