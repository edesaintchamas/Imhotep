/* Imhotep - monuments cleared from the map, and small monument methods (roadmap phase 8.2).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * When a monument's building is destroyed (0x004525F0, core/building/day_state.c) or undone
 * (0x00485230, core/command/undo.c), the monument's handler object clears its footprint
 * (thiscall on the object, argument: the building id): the part's kind is building +0x12 (i16,
 * 0x007999EA + 0x108 * id), its corner +6 / +8 (0x007999DE / 0x007999E0), its size the object's part table
 * (0x34-byte entries) at +0x54 (E1, E3, E4, E6) or at the tomb object's table +8 (E5, EA, EB, EC).
 * Also the vtable methods every monument class shares (+0xC 0x004E52D0) and the tombs' +0x2C /
 * +0x3C that are called directly. */
#include "sim/call.h"
#include "state/layout.h"
#include "state/ported.h"

#define BX(st, id) ((int32_t) S16(st, BUILDING_ADDR(id) + BLD_X))
#define BY(st, id) ((int32_t) S16(st, BUILDING_ADDR(id) + BLD_Y))
#define BKIND(st, id) ((int32_t) S16(st, BUILDING_ADDR(id) + BLD_PART))

static void clear_tiles(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALL(st, FN_BUILDING_CLEAR_TILES, id, x, y); /* building_clear_tiles (core/building/day2_destroy.c) */
}

void terrain_flag_477a40(game_state *st, int32_t x, int32_t y, uint32_t mask)
{
    /* @bin 0x00477A40 the tile's terrain word (0x00AF598C) keeps its bits 0x20300002 = mask: when
     * they already are, 0x00BC9730 := 1; else 0x00BC972C := 1 and the word := (word & 0xDFCFFFFD |
     * mask) & 0xB9FAF682. The tile index goes to 0x00BC9768 */
    int32_t idx = S32(st, G_MAP_START) + y * 0xE4 + x;
    S32(st, G_TILE_CURSOR) = idx;
    uint32_t a = G_GRID_TERRAIN + 4u * (uint32_t) idx;
    uint32_t w = U32(st, a);
    if ((w & 0x20300002u) == mask) {
        S32(st, G_UBC9730) = 1;
        return;
    }
    S32(st, G_UBC972C) = 1;
    U32(st, a) = ((w & 0xDFCFFFFDu) | mask) & 0xB9FAF682u;
}

void monument_e6_clear(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x00401480 (thiscall 0x006DE650) every tile of the part's square (size: object +0x54 +
     * 0x34 * kind), column by column: 0x00BC973C := 0x00BC9738 := 1, the building's tiles cleared
     * (0x0047C480); unless the part's +0x58 is 9, the terrain bits := 0x20000002 (0x00477A40) */
    int32_t x = BX(st, id), y0 = BY(st, id);
    uint32_t part = self + 0x34u * (uint32_t) BKIND(st, id);
    for (int32_t i = 0; i < S32(st, part + MONOBJ_PART_TABLE); i++, x++) {
        int32_t y = y0;
        for (int32_t j = 0; j < S32(st, part + MONOBJ_PART_TABLE); j++, y++) {
            S32(st, G_FOOTPRINT_COLS) = 1;
            S32(st, G_FOOTPRINT_ROWS) = 1;
            clear_tiles(st, id, x, y);
            if (S32(st, part + MONOBJ_PART_KIND) != 9) {
                terrain_flag_477a40(st, x, y, 0x20000002u);
            }
        }
    }
}

void monument_e1_clear(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x0040D890 (thiscall 0x006FD5D8) 0x00BC9738 := 0x00BC973C := the part's size (object
     * +0x54 + 0x34 * kind); the building cleared at its corner (0x0047C480), then every tile of the
     * square, column by column */
    int32_t x = BX(st, id), y = BY(st, id);
    uint32_t size = self + MONOBJ_PART_TABLE + 0x34u * (uint32_t) BKIND(st, id);
    S32(st, G_FOOTPRINT_ROWS) = S32(st, size);
    S32(st, G_FOOTPRINT_COLS) = S32(st, size);
    clear_tiles(st, id, x, y);
    for (int32_t i = 0; i < S32(st, size); i++) {
        for (int32_t j = 0; j < S32(st, size); j++) {
            clear_tiles(st, id, x + i, y + j);
        }
    }
}

void monument_e3_clear(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x00416FB0 (thiscall 0x006FD368) 0x00BC9738 := 0x00BC973C := 1; every tile of the
     * part's square (object +0x54 + 0x34 * kind) cleared (0x0047C480), column by column */
    int32_t x = BX(st, id), y = BY(st, id);
    uint32_t size = self + MONOBJ_PART_TABLE + 0x34u * (uint32_t) BKIND(st, id);
    S32(st, G_FOOTPRINT_ROWS) = 1;
    S32(st, G_FOOTPRINT_COLS) = 1;
    for (int32_t i = 0; i < S32(st, size); i++) {
        for (int32_t j = 0; j < S32(st, size); j++) {
            clear_tiles(st, id, x + i, y + j);
        }
    }
}

void monument_e4_clear(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x004E6660 (thiscall 0x006DE548) as 0x00401480 without the 0x00BC973C / 0x00BC9738
     * writes; the terrain bits := 2 (0x00477A40) when the part's +0x58 is 0 */
    int32_t x = BX(st, id), y0 = BY(st, id);
    uint32_t part = self + 0x34u * (uint32_t) BKIND(st, id);
    for (int32_t i = 0; i < S32(st, part + MONOBJ_PART_TABLE); i++, x++) {
        int32_t y = y0;
        for (int32_t j = 0; j < S32(st, part + MONOBJ_PART_TABLE); j++, y++) {
            clear_tiles(st, id, x, y);
            if (S32(st, part + MONOBJ_PART_KIND) == 0) {
                terrain_flag_477a40(st, x, y, 2);
            }
        }
    }
}

void tomb_clear_546660(game_state *st, uint32_t self, int32_t id, uint32_t table)
{
    /* @bin 0x00546660 (thiscall on the tomb object; id, its part table; a third argument unused)
     * every tile of the part's square (table + 8 + 0x34 * kind), column by column: the words
     * 0x00B4E6FC and 0x00A9CC1C := 0, the bytes 0x00AE8E7C and 0x00ADC36C := 0, 0x00B285CC &= 0xE0,
     * 0x00B67D1C := (& 0xC0) | 0x40, the terrain word 0x00AF598C := 0x80 for the object's own kind
     * (this +0x50), 0x20000002 for the others */
    int32_t kind = BKIND(st, id);
    int32_t x = BX(st, id), y = BY(st, id);
    uint32_t size = table + 8u + 0x34u * (uint32_t) kind;
    for (int32_t i = 0; i < S32(st, size); i++) {
        int32_t idx = S32(st, G_MAP_START) + y * 0xE4 + i + x;
        for (int32_t j = 0; j < S32(st, size); j++, idx += 0xE4) {
            uint32_t u = (uint32_t) idx;
            uint8_t b = U8(st, G_GRID_BITFIELDS + u);
            U16(st, G_GRID_BUILDING + 2u * u) = 0;
            U32(st, G_GRID_TERRAIN + 4u * u) = 0;
            U8(st, G_GRID_BITFIELDS + u) = (uint8_t) (b & 0xE0);
            b = U8(st, G_GRID_EDGE + u);
            U8(st, G_CANAL + u) = 0;
            U16(st, G_GRID_DAMAGE + 2u * u) = 0;
            U8(st, G_GRID_EDGE + u) = (uint8_t) ((b & 0xC0) | 0x40);
            U8(st, G_SPRITE + u) = 0;
            U32(st, G_GRID_TERRAIN + 4u * u) = kind != S32(st, self + TOMBOBJ_PART) ? 0x20000002u : 0x80u;
        }
    }
}

/* @bin 0x00545720 (E5, 0x006F9320: table +0x1E5C), 0x0054F450 (EC, 0x006E9188: +0x5C88), 0x00555E60
 * (EB, 0x006DE9E0: +0x4EE0), 0x00558BC0 (EA, 0x006F5648: +0x1CBC) the tombs' clear: 0x00546660 on
 * the object's part table (third argument 0xA7, 0x201, 0x1B5, 0x9E: unused) */
static void tomb_clear(game_state *st, uint32_t self, int32_t id, uint32_t off)
{
    tomb_clear_546660(st, self, id, self + off);
}

void tomb_parts_redraw_547b60(game_state *st, uint32_t self, int32_t id)
{
    /* @bin 0x00547B60 (thiscall on a tomb object) from the main building (id - kind +0x12), each
     * of the parts the method +0x38 counts gets the method +0x18 */
    int32_t base = id - BKIND(st, id);
    int32_t n = GAME_CALLT0(st, self, U32(st, U32(st, self) + 0x38));
    int32_t end = n + base;
    for (int32_t p = base; p < end; p++) {
        GAME_CALLT(st, self, U32(st, U32(st, self) + 0x18), p);
    }
}

static uint32_t this_of(const int32_t *a, uint32_t def)
{
    return a[PORT_THIS_ARG] ? (uint32_t) a[PORT_THIS_ARG] : def;
}

static int32_t run_477a40(game_state *st, const int32_t *a)
{
    terrain_flag_477a40(st, a[0], a[1], (uint32_t) a[2]);
    return 0;
}

static int32_t run_e6(game_state *st, const int32_t *a)
{
    monument_e6_clear(st, this_of(a, G_MONUMENT_E6), a[0]);
    return 0;
}

static int32_t run_e1(game_state *st, const int32_t *a)
{
    monument_e1_clear(st, this_of(a, G_MONUMENT_E1), a[0]);
    return 0;
}

static int32_t run_e3(game_state *st, const int32_t *a)
{
    monument_e3_clear(st, this_of(a, G_MONUMENT_E3), a[0]);
    return 0;
}

static int32_t run_e4(game_state *st, const int32_t *a)
{
    monument_e4_clear(st, this_of(a, G_MONUMENT_E4), a[0]);
    return 0;
}

static int32_t run_546660(game_state *st, const int32_t *a)
{
    tomb_clear_546660(st, this_of(a, 0), a[0], (uint32_t) a[1]);
    return 0;
}

static int32_t run_e5(game_state *st, const int32_t *a)
{
    tomb_clear(st, this_of(a, G_MONUMENT_E5), a[0], 0x1E5Cu);
    return 0;
}

static int32_t run_ec(game_state *st, const int32_t *a)
{
    tomb_clear(st, this_of(a, G_MONUMENT_EC), a[0], 0x5C88u);
    return 0;
}

static int32_t run_eb(game_state *st, const int32_t *a)
{
    tomb_clear(st, this_of(a, G_MONUMENT_EB), a[0], 0x4EE0u);
    return 0;
}

static int32_t run_ea(game_state *st, const int32_t *a)
{
    tomb_clear(st, this_of(a, G_MONUMENT_EA), a[0], 0x1CBCu);
    return 0;
}

static int32_t run_547b60(game_state *st, const int32_t *a)
{
    tomb_parts_redraw_547b60(st, (uint32_t) a[PORT_THIS_ARG], a[0]);
    return 0;
}

static int32_t run_zero(game_state *st, const int32_t *a)
{
    (void) st;
    (void) a;
    return 0; /* @bin 0x004E52D0 xor eax, eax; ret 8 (the monuments' method +0xC) */
}

static int32_t run_201(game_state *st, const int32_t *a)
{
    (void) st;
    (void) a;
    return 0x201; /* @bin 0x0054FEE0 mov eax, 0x201 (method +0x3C of the class 0x005701E8) */
}

static int32_t run_559110(game_state *st, const int32_t *a)
{
    /* @bin 0x00559110 the tombs' method +0x2C: 0x00547C10 with the same arguments and this */
    return GAME_CALLT(st, (uint32_t) a[PORT_THIS_ARG], FN_MONUMENT_TAKE_547C10, a[0], a[1], a[2]);
}

const ported_function monument_clear_ports[] = {
    {0x00477A40u, "terrain_flag_477a40", 3, 0, 0, run_477a40, 0},
    {0x00401480u, "monument_e6_clear", 1, 0, G_MONUMENT_E6, run_e6, 0},
    {0x0040D890u, "monument_e1_clear", 1, 0, G_MONUMENT_E1, run_e1, 0},
    {0x00416FB0u, "monument_e3_clear", 1, 0, G_MONUMENT_E3, run_e3, 0},
    {0x004E6660u, "monument_e4_clear", 1, 0, G_MONUMENT_E4, run_e4, 0},
    {0x00546660u, "tomb_clear_546660", 3, 0, 0, run_546660, 0},
    {0x00545720u, "tomb_e5_clear", 1, 0, G_MONUMENT_E5, run_e5, 0},
    {0x0054F450u, "tomb_ec_clear", 1, 0, G_MONUMENT_EC, run_ec, 0},
    {0x00555E60u, "tomb_eb_clear", 1, 0, G_MONUMENT_EB, run_eb, 0},
    {0x00558BC0u, "tomb_ea_clear", 1, 0, G_MONUMENT_EA, run_ea, 0},
    {0x00547B60u, "tomb_parts_redraw_547b60", 1, 0, 0, run_547b60, 0},
    {0x004E52D0u, "monument_method_c_zero", 2, 32, 0, run_zero, 0},
    {0x0054FEE0u, "tomb_ec_method_3c", 0, 32, G_MONUMENT_EC, run_201, 0},
    {0x00559110u, "tomb_method_2c", 3, 8, 0, run_559110, 0},
};
const int monument_clear_ports_count = sizeof(monument_clear_ports) / sizeof(monument_clear_ports[0]);
