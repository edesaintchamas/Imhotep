/* Imhotep - monument handler object methods and stage redraws (package terr).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The monuments of types 0xE3, 0xE4, 0xE6 have a handler object (0x006FD368, 0x006DE548,
 * 0x006DE650) whose vtable gives, at +0x1c, a part's image (with redraw flags in an output) and,
 * at +0x18, the part's redraw on the map grids. The stage counters of the parts (+0x58) go up
 * through 0x00417640 / 0x004E6BD0 / 0x00401BE0, the sun temple's (0xD5) through 0x00542DD0, the
 * sphinx's (city record +2) through 0x0053D820. 0x0053DD90 (thiscall on the graphics object
 * 0x00F55BB0) loads the image file of a stage into a slot of the image group table: it is called
 * as the original does, through GAME_CALLT. */
#include "building/monument_obj.h"

#include "building/rest_monument.h"
#include "building/rest_redraw.h"

#include "map/map_building.h"
#include "map/terrain_land.h"
#include "map/terrain_veg.h"
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"
#include "state/records.h"

#define B(id) BUILDING_ADDR(id)
#define PART(id) S16(st, B(id) + BLD_PART)
#define LOAD_IMAGES FN_GRAPHICS_LOAD_SLOT /* @bin thiscall (graphics object) (file name, slot) */

/* ---- redraw methods (+0x18) ----------------------------------------------------------------- */

void monument_method_redraw(game_state *st, uint32_t obj, int32_t id)
{
    /* the vtable method +0x18 (ports of package rest: core/building/rest_monument.c) */
    uint32_t m = U32(st, U32(st, obj) + 0x18);
    switch (m) {
    case FN_MONUMENT_REDRAW_4017A0: monument_redraw_4017a0(st, obj, id); break;
    case FN_MONUMENT_REDRAW_4E69C0: monument_redraw_4e69c0(st, obj, id); break;
    case FN_MONUMENT_REDRAW_40DCA0: monument_redraw_40dca0(st, obj, id); break;
    case FN_MONUMENT_REDRAW_5468D0: monument_redraw_5468d0(st, obj, id); break;
    default: GAME_CALLT(st, obj, m, id); break;
    }
}

/* ---- stage counters ------------------------------------------------------------------------- */

void monument_e3_stage_up(game_state *st, uint32_t obj, int32_t id)
{
    /* @bin 0x00417640 the main building's stage +0x58 below 0x13 goes up (0x13: +0x64 |= 1) and
     * its 9 parts are redrawn (method +0x18) */
    int32_t main = id - PART(id);
    int32_t s = S32(st, B(main) + BLD_MONUMENT_STAGE);
    if (s >= 0x13) {
        return;
    }
    S32(st, B(main) + BLD_MONUMENT_STAGE) = s + 1;
    if (s + 1 == 0x13) {
        U32(st, B(main) + BLD_MONUMENT_FLAGS) |= 1;
    }
    for (int32_t i = 0; i < 9; i++) {
        monument_method_redraw(st, obj, main + i);
    }
}

void monument_e4_stage_up(game_state *st, uint32_t obj, int32_t id)
{
    /* @bin 0x004E6BD0 the building's stage +0x58 below 9 goes up (9: +0x64 |= 1), redrawn */
    int32_t s = S32(st, B(id) + BLD_MONUMENT_STAGE);
    if (s >= 9) {
        return;
    }
    S32(st, B(id) + BLD_MONUMENT_STAGE) = s + 1;
    if (s + 1 == 9) {
        U32(st, B(id) + BLD_MONUMENT_FLAGS) |= 1;
    }
    monument_method_redraw(st, obj, id);
}

void monument_e6_step(game_state *st, uint32_t obj, int32_t id)
{
    /* @bin 0x00401BE0 the main building's step +0x54 (below 0x38) in table 0x0056F348 (4 bytes:
     * kind, flag, part, value): flag 0: byte +0x61 + value - 1 of the main building := (kind ==
     * 3); else the stage +0x58 of the part (1: +0xE, 2: +0xB, 3: +0xD, 4: +0xF, 5: +0xC, else the
     * main building) := value. Then the 16 parts get step + 1 and are redrawn (method +0x18). */
    int32_t main = id - PART(id);
    int32_t v = S32(st, B(main) + BLD_U54);
    if (v >= 0x38) {
        return;
    }
    uint32_t e = G_E6_STEPS + 4u * (uint32_t) v;
    int32_t value = U8(st, e + E6STEP_VALUE);
    if (U8(st, e + E6STEP_FLAG) == 0) {
        int32_t k = value - 1;
        U8(st, B(main) + BLD_U61 + (uint32_t) k) = U8(st, e + E6STEP_KIND) == 3;
    } else {
        int32_t target = main;
        switch (U8(st, e + E6STEP_PART)) {
        case 1: target = main + 0xE; break;
        case 2: target = main + 0xB; break;
        case 3: target = main + 0xD; break;
        case 4: target = main + 0xF; break;
        case 5: target = main + 0xC; break;
        default: break;
        }
        S32(st, B(target) + BLD_MONUMENT_STAGE) = value;
    }
    for (int32_t i = main; i < id - PART(id) + 0x10; i++) {
        S32(st, B(i) + BLD_U54) = v + 1;
        monument_method_redraw(st, obj, i);
    }
}

/* ---- sun temple, sphinx ---------------------------------------------------------------------- */

int32_t sun_temple_image(game_state *st, int32_t id, int32_t *out)
{
    /* @bin 0x005414B0 by the part kind (0x00601DE4 + 0x34 * part, 0..12) and the stage +0x58:
     * kind 0: the temple's file (0x00603228, by stage: < 9, < 13, else) into slot 5 / 6 / 7 by
     * +0x54 (1: 6, 2: 7), image group 0x00F56D54 / 5C / 64 + orientation bit; other kinds: the
     * finished image of group 0x00F56D74 at stage 2 (4 for kind 12), else out := 5 (7) with the
     * construction images 0x00F56B54 / 0x00F5736C */
    uint32_t b = B(id);
    *out = 0;
    int32_t kind = S32(st, G_SUN_TEMPLE_PART_KIND + 0x34u * (uint32_t) PART(id));
    int32_t o = S32(st, G_VIEW_ORIENTATION);
    int32_t d74 = S32(st, G_IMAGE_GROUPS + 8 * 568);
    int32_t stage;
    switch (kind) {
    case 0: {
        int32_t s = S32(st, b + BLD_MONUMENT_STAGE);
        int32_t f = s < 9 ? 0 : (s >= 0xD) + 1;
        int32_t v = S32(st, b + BLD_MONUMENT_SLOT);
        uint32_t file = U32(st, G_SUN_TEMPLE_FILES + 4u * (uint32_t) f);
        if (v == 1) {
            GAME_CALLT(st, G_GRAPHICS_GFX, LOAD_IMAGES, (int32_t) file, 6);
            return ((S32(st, G_VIEW_ORIENTATION) / 2) & 1) + S32(st, G_IMAGE_GROUPS + 8 * 565);
        }
        if (v == 2) {
            GAME_CALLT(st, G_GRAPHICS_GFX, LOAD_IMAGES, (int32_t) file, 7);
            return ((S32(st, G_VIEW_ORIENTATION) / 2) & 1) + S32(st, G_IMAGE_GROUPS + 8 * 566);
        }
        GAME_CALLT(st, G_GRAPHICS_GFX, LOAD_IMAGES, (int32_t) file, 5);
        return ((S32(st, G_VIEW_ORIENTATION) / 2) & 1) + S32(st, G_IMAGE_GROUPS + 8 * 564);
    }
    case 1: case 2: case 3: case 4: case 5:
        if (S32(st, b + BLD_MONUMENT_STAGE) == 2) {
            switch (kind) {
            case 1: return ((o / 2) & 1) + d74;
            case 2: return (((8 - o) / 2) & 3) + d74 + 2;
            case 3: return (((8 - o) / 2 + 1) & 3) + d74 + 2;
            case 4: return (((8 - o) / 2 - 2) & 3) + d74 + 2;
            default: return (((8 - o) / 2 - 1) & 3) + d74 + 2;
            }
        }
        *out = 5;
        return S32(st, b + BLD_MONUMENT_STAGE) != 1 ? S32(st, G_IMAGE_GROUPS + 8 * 759) : S32(st, G_PLAIN);
    case 6: case 7: case 10: case 11:
        if (S32(st, b + BLD_MONUMENT_STAGE) == 2) {
            switch (kind) {
            case 6: return ((o / 2) & 1) + d74 + 6;
            case 7: return ((o / 2 - 1) & 1) + d74 + 6;
            case 10: return d74 + 0xA;
            default: return d74 + 0xB;
            }
        }
        *out = 5;
        stage = S32(st, b + BLD_MONUMENT_STAGE);
        return stage == 1 ? S32(st, G_PLAIN) : 0;
    case 8: case 9:
        *out = 5;
        stage = S32(st, b + BLD_MONUMENT_STAGE);
        if (stage == 2) {
            return d74 + kind;
        }
        return stage == 1 ? S32(st, G_PLAIN) : 0;
    case 12:
        if (S32(st, b + BLD_MONUMENT_STAGE) == 4) {
            return d74 + 0xC;
        }
        *out = 5;
        if (S32(st, b + BLD_MONUMENT_STAGE) >= 1) {
            return S32(st, G_PLAIN);
        }
        *out = 7;
        return S32(st, G_IMAGE_GROUPS + 8 * 759);
    default:
        return 0;
    }
}

void sun_temple_redraw(game_state *st, int32_t id)
{
    /* @bin 0x00541920 the image of 0x005414B0 on the part's square (size 0x00601DE0 + 0x34 *
     * part): flags bit 0 clear: 0x0047BF50 (flags 8, the main part; 0x10000008 the others); set:
     * every tile gets terrain 0x10000000, the building, size bits cleared, edge 0x40 and (unless
     * flag bit 1 limits it to the corners) the image (group 0x00F56B54: + random & 7) */
    int32_t flags;
    int32_t image = sun_temple_image(st, id, &flags);
    uint32_t b = B(id);
    int16_t part = PART(id);
    int32_t n = S32(st, G_SUN_TEMPLE_PARTS + 8 + 0x34u * (uint32_t) part);
    if (!(flags & 1)) {
        map_building_tiles(st, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), n, n, image, part != 0 ? 0x10000008u : 8u);
        return;
    }
    S32(st, G_TILE_CURSOR) = S32(st, G_MAP_START) + S16(st, b + BLD_X) + S16(st, b + BLD_Y) * 228;
    for (int32_t j = 0; j < n; j++) {
        for (int32_t i = 0; i < n; i++) {
            int32_t o = S32(st, G_FOOTPRINT_OFFSETS + 0x30u * (uint32_t) j + 8u * (uint32_t) i) + S32(st, G_TILE_CURSOR);
            U32(st, G_GRID_TERRAIN + 4u * (uint32_t) o) |= 0x10000000u;
            S16(st, G_GRID_BUILDING + 2u * (uint32_t) o) = (int16_t) id;
            U8(st, G_GRID_BITFIELDS + (uint32_t) o) &= 0xE0;
            U8(st, G_GRID_EDGE + (uint32_t) o) |= 0x40;
            if ((flags & 2) && !((i == 0 || i == n - 1) && (j == 0 || j == n - 1))) {
                continue;
            }
            if (image == 0) {
                continue;
            }
            if (image == S32(st, G_PLAIN)) {
                S32(st, G_GRID_IMAGE + 4u * (uint32_t) o) = (U8(st, G_GRID_RANDOM + (uint32_t) o) & 7) + image;
            } else {
                S32(st, G_GRID_IMAGE + 4u * (uint32_t) o) = image;
            }
        }
    }
}

void sun_temple_stage_up(game_state *st, int32_t id)
{
    /* @bin 0x00542DD0 the stage +0x58 below 0xD goes up (0xD: +0x64 |= 1), redrawn */
    int32_t s = S32(st, B(id) + BLD_MONUMENT_STAGE);
    if (s >= 0xD) {
        return;
    }
    S32(st, B(id) + BLD_MONUMENT_STAGE) = s + 1;
    if (s + 1 == 0xD) {
        U32(st, B(id) + BLD_MONUMENT_FLAGS) |= 1;
    }
    sun_temple_redraw(st, id);
}

void sphinx_stage_up(game_state *st)
{
    /* @bin 0x0053D820 the city's sphinx stage (word +2 of the city record 0x00EA5BA8 + 0x49D8 *
     * [0x00EAAC28]) from 1 to 15 goes up, then 0x0053D850 loads its images */
    uint32_t p = CITY_BASE + CITY_U433A + 0x49D8u * (uint32_t) S32(st, G_PLAYER);
    int16_t v = S16(st, p);
    if (v == 0 || v >= 0x10) {
        return;
    }
    S16(st, p) = (int16_t) (v + 1);
    GAME_CALL0(st, FN_SPHINX_IMAGES_53D850);
}

/* ---- mastaba-like parts, obelisks ------------------------------------------------------------ */

void monument_part_redraw(game_state *st, int32_t id)
{
    /* @bin 0x004DFBB0 the part's layout (0x004DED50 (part, orientation, +0x70)) and image
     * (0x004DF360): flag bit 0 clear: its square by 0x0047BF50 (flags 8); set: every tile gets
     * the building, terrain 0x10000000, size bits cleared, edge 0x40 and the image (ground group
     * 0x00F56B54: + random & 7), terrain & 0xFEFED77E (0xFEFED7FE for the images [0x00F57464],
     * [0x00F5736C]); without an image the square's trees, land and meadow are redrawn
     * (0x00475BF0, 0x004767D0 (.., 0, 1), 0x00477500) */
    uint32_t b = B(id);
    int32_t layout[13];
    monument_part_layout_4ded50(st, layout, PART(id), S32(st, G_VIEW_ORIENTATION), S32(st, b + BLD_LAYOUT_DIRECTION));
    int32_t flags = 0;
    int32_t image = monument_part_image_4df360(st, id, &flags);
    int32_t n = layout[2];
    if (!(flags & 1)) {
        map_building_tiles(st, id, S16(st, b + BLD_X), S16(st, b + BLD_Y), n, n, image, 8);
        return;
    }
    int32_t y = S16(st, b + BLD_Y), x = S16(st, b + BLD_X);
    int32_t c = S32(st, G_MAP_START) + y * 228 + x;
    S32(st, G_TILE_CURSOR) = c;
    for (int32_t j = 0; j < n; j++) {
        for (int32_t i = 0; i < n; i++) {
            int32_t o = S32(st, G_FOOTPRINT_OFFSETS + 0x30u * (uint32_t) j + 8u * (uint32_t) i) + c;
            uint32_t v = U32(st, G_GRID_TERRAIN + 4u * (uint32_t) o) | 0x10000000u;
            S16(st, G_GRID_BUILDING + 2u * (uint32_t) o) = (int16_t) id;
            U8(st, G_GRID_BITFIELDS + (uint32_t) o) &= 0xE0;
            U32(st, G_GRID_TERRAIN + 4u * (uint32_t) o) = v;
            U8(st, G_GRID_EDGE + (uint32_t) o) |= 0x40;
            if (image == 0) {
                S32(st, G_GRID_IMAGE + 4u * (uint32_t) o) = 0;
                continue;
            }
            S32(st, G_GRID_IMAGE + 4u * (uint32_t) o) =
                image == S32(st, G_PLAIN) ? (U8(st, G_GRID_RANDOM + (uint32_t) o) & 7) + image : image;
            U32(st, G_GRID_TERRAIN + 4u * (uint32_t) o) =
                v & (image == S32(st, G_IMAGE_GROUPS + 8 * 790) || image == S32(st, G_IMAGE_GROUPS + 8 * 759) ? 0xFEFED7FEu : 0xFEFED77Eu);
        }
    }
    if (image == 0) {
        trees_rect(st, x, y, x + n - 1, y + n - 1);
        terrain_land(st, x, y, x + n - 1, y + n - 1, 0, 1);
        meadow_rect(st, x, y, x + n - 1, y + n - 1);
    }
}

void obelisk_redraw(game_state *st, int32_t id, int32_t x, int32_t y)
{
    /* @bin 0x004E5840 (id, x, y) the stage file of the obelisk (type 0xCF: 4 stages from
     * 0x005E4588, others 6 from 0x005E4598) by its progress (+0x55 - 0x42) * 3 percent, loaded
     * into slot part + 5; image group 0x00F56D54 / 5C / 64 by part (else 0) on its square */
    uint32_t b = B(id);
    int32_t m = (U8(st, b + BLD_U55) - 0x42) * 3;
    int16_t type = S16(st, b + BLD_TYPE);
    int32_t part = S16(st, b + BLD_OBELISK_SLOT);
    int32_t k = type != 0xCF ? 6 : 4;
    int32_t v = k * (m > 0 ? m : 0) / 100;
    if (v >= k - 1) {
        v = k - 1;
    }
    GAME_CALLT(st, G_GRAPHICS_GFX, LOAD_IMAGES, S32(st, G_OBELISK_FILES + 4u * (uint32_t) ((type != 0xCF ? 4 : 0) + v)),
               part + 5);
    int32_t image;
    switch (S16(st, B(id) + BLD_OBELISK_SLOT)) {
    case 0: image = S32(st, G_IMAGE_GROUPS + 8 * 564); break;
    case 1: image = S32(st, G_IMAGE_GROUPS + 8 * 565); break;
    case 2: image = S32(st, G_IMAGE_GROUPS + 8 * 566); break;
    default: image = 0; break;
    }
    map_building_tiles(st, id, x, y, U8(st, b + BLD_SIZE), U8(st, b + BLD_SIZE), image, 8);
}

/* ---- registration -------------------------------------------------------------------------- */

#define THIS(a) ((uint32_t) (a)[PORT_THIS_ARG])

static int32_t image_out(game_state *st, const int32_t *a, int32_t r, int32_t out)
{
    /* the output pointer is written when it is in the state (a test passes a global) */
    if (state_ptr(st, (uint32_t) a[1], 4)) {
        S32(st, (uint32_t) a[1]) = out;
    }
    return r;
}

static int32_t run_sun_image(game_state *st, const int32_t *a)
{
    int32_t out = 0;
    int32_t r = sun_temple_image(st, a[0], &out);
    return image_out(st, a, r, out);
}

static int32_t run_e3_up(game_state *st, const int32_t *a) { monument_e3_stage_up(st, THIS(a), a[0]); return 0; }
static int32_t run_e4_up(game_state *st, const int32_t *a) { monument_e4_stage_up(st, THIS(a), a[0]); return 0; }
static int32_t run_e6_step(game_state *st, const int32_t *a) { monument_e6_step(st, THIS(a), a[0]); return 0; }
static int32_t run_sun_redraw(game_state *st, const int32_t *a) { sun_temple_redraw(st, a[0]); return 0; }
static int32_t run_sun_up(game_state *st, const int32_t *a) { sun_temple_stage_up(st, a[0]); return 0; }
static int32_t run_part_redraw(game_state *st, const int32_t *a) { monument_part_redraw(st, a[0]); return 0; }
static int32_t run_obelisk_redraw(game_state *st, const int32_t *a) { obelisk_redraw(st, a[0], a[1], a[2]); return 0; }
static int32_t run_sphinx_up(game_state *st, const int32_t *a) { (void) a; sphinx_stage_up(st); return 0; }

static int32_t run_nothing2(game_state *st, const int32_t *a)
{
    (void) st;
    (void) a;
    return 0; /* @bin 0x00401D30 ret 8 */
}

const ported_function terr_monument_ports[] = {
    {0x00417640u, "monument_e3_stage_up", 1, 0, 0, run_e3_up, 0},
    {0x004E6BD0u, "monument_e4_stage_up", 1, 0, 0, run_e4_up, 0},
    {0x00401BE0u, "monument_e6_step", 1, 0, 0, run_e6_step, 0},
    {0x00401D30u, "monument_e6_nothing", 2, 0, 0, run_nothing2, 0},
    {0x005414B0u, "sun_temple_image", 2, 32, 0, run_sun_image, 0},
    {0x00541920u, "sun_temple_redraw", 1, 0, 0, run_sun_redraw, 0},
    {0x00542DD0u, "sun_temple_stage_up", 1, 0, 0, run_sun_up, 0},
    {0x0053D820u, "sphinx_stage_up", 0, 0, 0, run_sphinx_up, 0},
    {0x004DFBB0u, "monument_part_redraw", 1, 0, 0, run_part_redraw, 0},
    {0x004E5840u, "obelisk_redraw", 3, 0, 0, run_obelisk_redraw, 0},
};
const int terr_monument_ports_count = sizeof(terr_monument_ports) / sizeof(terr_monument_ports[0]);
