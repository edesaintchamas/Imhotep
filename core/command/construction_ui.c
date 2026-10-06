/* Imhotep - the construction UI handlers of Pharaoh.exe (package ui, docs/commands.md 2.1, 2.3, 2.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Choose a building (0x00405200: type + placement flags, also the map editor's tool menus), mouse
 * down (0x00405C20: construction start and undo snapshot 0x00484F60), drag (0x00405D90: preview
 * 0x0046FEF0), mouse up (0x00405E90: the place function 0x004728A0, package place), cancel
 * (0x00405BA0), undo button (0x00406010: 0x00485230, package clear).
 *
 * 0x00605A48 holds the address of a terrain predicate in the exe (0x00405B40, 0x00405B60 or
 * 0x00405B80): the state keeps the original's value and the callers reach the predicate through
 * sim_call on that address, which runs the ports below (registered at those addresses). */
#include "command/construction_ui.h"

#include <string.h>

#include "sim/call.h"
#include "state/ported.h"
#include "state/records.h"

/* construction globals (docs/commands.md 2.1, 2.3) */
#define FLAG_B4 0x006059B4u
#define FLAG_B8 0x006059B8u
#define FLAG_BC 0x006059BCu
#define FLAG_C0 0x006059C0u
#define FLAG_C4 0x006059C4u
#define FLAG_C8 0x006059C8u
#define FLAG_CC 0x006059CCu
#define START_OFFSET 0x006059D0u
#define Y2 0x006059D4u
#define Y1 0x006059D8u
#define X2 0x006059DCu
#define X1 0x006059E0u
#define IN_PROGRESS 0x006059E4u
#define TYPE 0x006059E8u
#define PREDICATE 0x00605A48u
#define EDITOR 0x00605A3Cu
#define EDITOR_TOOL 0x00605A30u
#define FROZEN 0x00605A5Cu   /* u8: the hovered tile is not updated */
#define DRAGGED 0x00605798u  /* u8 */
#define ROTATION 0x00BC9714u
#define SUBMENU 0x007056ECu
#define ITEM 0x00D3B198u
#define WINDOW 0x00E38DDAu   /* u8 */
#define REDRAW 0x00E38DCAu   /* u8 */
#define GATE 0x00E38E60u     /* u8 */
#define HOVER_OFFSET 0x00BCA300u
#define HOVER_X 0x00BCA304u
#define HOVER_Y 0x00BCA308u
#define MOUSE_OFFSET 0x00BCA2C4u
#define DRAG_OFFSET 0x00BCA2E4u
#define VIEW_ORIENTATION 0x00BCA2E0u
#define MAP_W 0x00BCA2D0u
#define MAP_H 0x00BCA2D4u
#define MAP_START 0x00BCA2D8u
#define FERRY_STATE 0x00BCA310u
#define FERRY_FLAG 0x00BCA315u /* u8 */
#define FERRY_BUILDING 0x00BC9800u
#define MENU_TYPES 0x005CE3B8u /* @bin 0x00405792 i32[submenu * 30 + item] */
#define COST 0x00BC9724u
#define REFUND 0x00BC9718u
#define COUNT 0x00BC971Cu
#define SIZE 0x00BC96D0u
#define BRIDGE_LENGTH 0x00BC96F8u
#define NO_ROOM 0x00605994u

#define GRID_W 228
#define TERRAIN_GRID 0x00AF598Cu  /* u32 per tile */
#define BUILDING_GRID 0x00B4E6FCu /* i16 per tile */
#define ELEVATION_GRID 0x00AB623Cu /* u8 per tile */

/* ---- terrain predicates (0x00605A48) ---- */

int32_t terrain_pred_405b40(uint32_t terrain)
{
    return (terrain & 0x20300002u) == 0x100002u; /* @bin 0x00405B40 */
}

int32_t terrain_pred_405b60(uint32_t terrain)
{
    return (terrain & 0x20300002u) == 2u; /* @bin 0x00405B60 */
}

int32_t terrain_pred_405b80(uint32_t terrain)
{
    return (terrain & 2u) == 2u; /* @bin 0x00405B80 */
}

/* ---- ferry two-step placement ---- */

void ferry_half_delete(game_state *st, int32_t id)
{
    /* @bin 0x00486060 the half-placed ferry: state := 6 (deleted) */
    S32(st, FERRY_STATE) = 0;
    U8(st, BUILDING_ADDR(id)) = 6;
}

void ferry_place_step(game_state *st)
{
    /* @bin 0x00486020 first landing: state 1 and notice 0x43; else 0, and 3 (both landings
     * linked) records the first building for undo (0x00485100) */
    int32_t f = S32(st, FERRY_STATE);
    if (f == 0) {
        S32(st, FERRY_STATE) = 1;
        GAME_CALL(st, 0x004E2970u, 0x43, 0, 0);
        return;
    }
    S32(st, FERRY_STATE) = 0;
    if (f == 3) {
        GAME_CALL(st, 0x00485100u, S32(st, FERRY_BUILDING));
    }
}

/* ---- choose a building ---- */

static void clear_tiles(game_state *st)
{
    /* @bin 0x00405603 / 0x00405AFB common tail */
    S32(st, IN_PROGRESS) = 0;
    S32(st, X2) = 0;
    S32(st, X1) = 0;
    S32(st, Y2) = 0;
    S32(st, Y1) = 0;
    U8(st, REDRAW) = 1;
}

/* @bin 0x004056C8 types whose rotation or placement flag is not set by the first chain */
static void type_flags_2(game_state *st, int32_t t)
{
    if (t >= 0x8C && t <= 0x95) {
        S32(st, ROTATION) = 1;
    } else if (t == 0x38) {
        S32(st, ROTATION) = 1;
    } else if (t == 0x3B || t == 0xAC || t == 0xAD) {
        S32(st, FLAG_BC) = 1;
    } else if (t == 0x8A) {
        S32(st, FLAG_B8) = 1;
    } else if (t == 0x5C || t == 0xB4 || t == 0x4D || t == 0x4E || t == 0x4F || t == 0xBB || t == 0xBC ||
               t == 0xBD) {
        S32(st, FLAG_B4) = 1;
    } else if (t >= 0x41 && t <= 0x45) {
        S32(st, ROTATION) = S32(st, VIEW_ORIENTATION);
    } else if (t >= 0x29 && t <= 0x2B) {
        S32(st, ROTATION) = 0;
    }
    U8(st, WINDOW) = 1;
    clear_tiles(st);
}

/* @bin 0x004057A5 menu entries that open a submenu, then the placement flags of the type */
static void type_flags_1(game_state *st, int32_t t)
{
    int32_t sub = -1;
    switch (t) {
    case 2: sub = 0x14; break;
    case 3: sub = 0x15; break;
    case 4: sub = 0x16; break;
    case 0x30: sub = 0x1C; break;
    case 0x34: sub = 0x22; break;
    case 0x5B: sub = 0x23; break;
    case 0xAE: sub = 0x1E; break;
    case 0xAF: sub = 0x1F; break;
    case 0xB0: sub = 0x20; break;
    case 0x60: sub = 0x17; break;
    case 0x61: /* @bin 0x0040583F i16 CITY+0x4478 != 0 */
        sub = S16(st, CURRENT_CITY(st) + 0x4478u) != 0 ? 0x24 : 0x18;
        break;
    case 0x96: sub = 0x1D; break;
    case 0x39: sub = 0x19; break;
    default: break;
    }
    if (sub >= 0) {
        S32(st, SUBMENU) = sub;
        int32_t r = GAME_CALL0(st, 0x00404120u);
        S32(st, 0x007056F0u) = r;
        S32(st, 0x007056E8u) = S32(st, 0x005CF63Cu + 8u * (uint32_t) r);
        S32(st, 0x007056E4u) = S32(st, 0x005CF640u + 8u * (uint32_t) r);
        S32(st, TYPE) = 0;
        clear_tiles(st);
        return;
    }
    switch (t) {
    case 0x66: case 0x64: case 0x67: case 0x68: case 0x65: case 0xE0: case 0x69: case 0xC4:
        S32(st, FLAG_CC) = 1;
        break;
    case 0xC2:
        S32(st, FLAG_CC) = 0;
        break;
    case 0x6A:
        S32(st, PREDICATE) = 0x00405B80;
        break;
    case 0xDD: case 0x6B: case 0xD8: case 0xA2:
        S32(st, PREDICATE) = 0x00405B60;
        break;
    case 0xA1: case 0xD9:
        S32(st, PREDICATE) = 0x00405B40;
        break;
    case 0x6D:
        S32(st, FLAG_C4) = 1;
        break;
    case 0x3A: case 0xAA: case 0xAB: case 0xC8: case 0xC9: case 0xCA:
        S32(st, ROTATION) = 1;
        break;
    default:
        type_flags_2(st, t);
        return;
    }
    U8(st, WINDOW) = 1;
    clear_tiles(st);
}

/* @bin 0x0040562B the monuments menu (0x1C): the entry of the scenario's list 0x00784DDE gives a
 * record of 0x005D19C8 (16 bytes: +0 pyramid flag, +2 kind, +4 variant) */
static void choose_monument(game_state *st, int32_t item)
{
    uint32_t rec = 0x005D19C8u + 16u * (uint32_t) (int32_t) S16(st, 0x00784DDEu + 2u * (uint32_t) item);
    if (S16(st, rec) == 1) {
        S32(st, TYPE) = 0xB7;
        S32(st, 0x005E6970u) = S16(st, rec + 4);
        type_flags_2(st, 0xB7);
        return;
    }
    int16_t kind = S16(st, rec + 2);
    int32_t t;
    switch (kind) {
    case 1: {
        /* @bin 0x0040566F the Sphinx: its images loaded with CITY+0x433A forced to 1 */
        uint32_t f = CURRENT_CITY(st) + 0x433Au;
        int16_t saved = S16(st, f);
        S32(st, TYPE) = 0xD2;
        S16(st, f) = 1;
        GAME_CALL0(st, 0x0053D850u);
        S16(st, CURRENT_CITY(st) + 0x433Au) = saved;
        type_flags_1(st, S32(st, TYPE));
        return;
    }
    case 3: t = 0xD5; break;
    case 6: t = 0xE1; break;
    case 0xE: t = 0xE6; break;
    case 7: t = 0xE3; break;
    case 0xA: t = 0xE5; break;
    case 0xB: t = 0xEA; break;
    case 0xC: t = 0xEB; break;
    case 0xD: t = 0xEC; break;
    case 5: t = 0xE4; break;
    case 4: t = 0xDE; break;
    case 2:
        /* @bin 0x0040576D obelisks: 0xCF + variant; the item kept in 0x00576144 */
        t = S16(st, rec + 4) + 0xCF;
        S32(st, 0x00576144u) = item;
        S32(st, TYPE) = t;
        type_flags_1(st, t);
        return;
    default:
        type_flags_1(st, S32(st, TYPE));
        return;
    }
    S32(st, TYPE) = t;
    type_flags_2(st, t);
}

/* @bin 0x00405200 editor tool menus: (tool 0x00605A30, type) per item */
static void editor_tool(game_state *st, int32_t tool, int32_t type)
{
    S32(st, EDITOR_TOOL) = tool;
    S32(st, TYPE) = type;
}

void construction_choose_building(game_state *st)
{
    /* @bin 0x00405200 */
    if (S32(st, FERRY_STATE) == 1) {
        ferry_half_delete(st, S32(st, FERRY_BUILDING));
    }
    if (S32(st, FERRY_STATE) == 2) {
        S32(st, FERRY_STATE) = 0;
    }
    int32_t sub = S32(st, SUBMENU);
    S32(st, FLAG_B4) = 0;
    S32(st, FLAG_B8) = 0;
    S32(st, FLAG_BC) = 0;
    S32(st, FLAG_C4) = 0;
    S32(st, FLAG_C8) = 0;
    S32(st, FLAG_CC) = 0;
    S32(st, FLAG_C0) = 0;
    S32(st, PREDICATE) = 0;
    S32(st, 0x00BC9710u) = S32(st, 0x00E92E0Cu);
    S32(st, ROTATION) = 0;
    S32(st, HOVER_OFFSET) = 0;
    S32(st, MOUSE_OFFSET) = 0;
    U8(st, FROZEN) = 0;
    int32_t item = S32(st, ITEM);
    switch (sub) {
    case 0xE:
        U8(st, WINDOW) = 1;
        S32(st, 0x00605A34u) = item - 1;
        break;
    case 0x25:
        U8(st, WINDOW) = 1;
        S16(st, 0x00784B94u) = (int16_t) (item - 1);
        break;
    case 0x1B:
        /* @bin 0x004052D3 jump table on item - 1 (unsigned, 0..3) */
        if (item == 1) {
            editor_tool(st, 2, 0x76);
        } else if (item == 2) {
            editor_tool(st, 0x1A, 0x86);
        } else if (item == 3) {
            editor_tool(st, 0x1D, 0xC5);
        } else if (item == 4) {
            editor_tool(st, 0x1B, 8);
        }
        U8(st, WINDOW) = 1;
        break;
    case 0xF:
        U8(st, WINDOW) = 1;
        S32(st, 0x00605A2Cu) = item;
        if (item == 1) {
            editor_tool(st, 0xF, 0x7F);
        } else if (item == 2) {
            editor_tool(st, 0x10, 0x80);
        }
        break;
    case 0x12:
        U8(st, WINDOW) = 1;
        if (item == 1) {
            editor_tool(st, 0x12, 0x82);
        } else if (item == 2) {
            editor_tool(st, 0x13, 0x83);
        }
        break;
    case 0x13:
        U8(st, WINDOW) = 1;
        if (item == 1) {
            editor_tool(st, 0x15, 0x58);
        } else if (item == 2) {
            editor_tool(st, 0x16, 0x59);
        } else if (item == 3) {
            editor_tool(st, 0x17, 0x5D);
        }
        break;
    case 0x1A:
        /* @bin 0x00405483 signed compares */
        if (item <= 8) {
            editor_tool(st, 0x18, 0x84);
            S32(st, 0x00605A24u) = item;
        } else if (item <= 0xC) {
            editor_tool(st, 0x19, 0x85);
            S32(st, 0x00605A1Cu) = item - 8;
        } else if (item <= 0x10) {
            editor_tool(st, 0x1C, 0x8B);
            S32(st, 0x00605A1Cu) = item - 0xC;
        } else {
            editor_tool(st, 0x1F, 0xD6);
            S32(st, 0x00605A20u) = item - 0x10;
        }
        U8(st, WINDOW) = 1;
        break;
    case 0x11:
        editor_tool(st, 0xD, 0x7E);
        S32(st, 0x00605A28u) = item;
        U8(st, WINDOW) = 1;
        break;
    case 0x10:
        U8(st, WINDOW) = 1;
        if (item == 1) {
            editor_tool(st, 0xB, 0x7B);
        } else if (item == 2) {
            editor_tool(st, 0xC, 0x7C);
        } else if (item == 3) {
            editor_tool(st, 9, 0x81);
        }
        break;
    case 0: {
        /* @bin 0x004055C5 submenu 0: the type, and the editor tool 0x1E (0xC6), 0x21 (0xDF) or 5 */
        U8(st, WINDOW) = 1;
        int32_t t = S32(st, MENU_TYPES + 4u * (uint32_t) item);
        S32(st, TYPE) = t;
        S32(st, EDITOR_TOOL) = t == 0xC6 ? 0x1E : (t != 0xDF ? 5 : 0x21);
        break;
    }
    case 0x1C:
        choose_monument(st, item);
        return;
    default: {
        /* @bin 0x00405783 */
        int32_t t = S32(st, MENU_TYPES + 4u * (uint32_t) (item + sub * 30));
        S32(st, TYPE) = t;
        type_flags_1(st, t);
        return;
    }
    }
    clear_tiles(st);
}

/* ---- right click ---- */

int32_t construction_cancel(game_state *st)
{
    /* @bin 0x00405BA0 returns 1 when a window other than the city view or a type was active */
    int32_t changed = 0;
    if (U8(st, WINDOW) != 1) {
        changed = 1;
    }
    if (S32(st, TYPE) != 0) {
        changed = 1;
    }
    U8(st, WINDOW) = 1;
    S32(st, TYPE) = 0;
    S32(st, IN_PROGRESS) = 0;
    S32(st, X2) = 0;
    S32(st, X1) = 0;
    S32(st, Y2) = 0;
    S32(st, Y1) = 0;
    if (S32(st, FERRY_STATE) == 1) {
        ferry_half_delete(st, S32(st, FERRY_BUILDING));
    }
    if (S32(st, FERRY_STATE) == 2) {
        S32(st, FERRY_STATE) = 0;
    }
    U8(st, REDRAW) = 1;
    return changed;
}

/* ---- undo ---- */

int32_t construction_undo_snapshot(game_state *st)
{
    /* @bin 0x00484F60 */
    memset(state_ptr(st, 0x00BC95F4u, 0x24 * 4), 0, 0x24 * 4);
    S32(st, 0x00BC968Cu) = 0;
    S32(st, 0x00BC9694u) = 1;
    S32(st, 0x00BC9688u) = 0;
    S32(st, 0x00BC9684u) = 0;
    S32(st, 0x00BC95F0u) = 0;
    S32(st, 0x00BC9690u) = S32(st, TYPE);
    memset(state_ptr(st, 0x00BA7888u, 0x21D68), 0, 0x21D68); /* 0x0040F8F0 */
    memset(state_ptr(st, 0x00BA746Cu, 0x41A), 0, 0x41A);     /* 0x0040F8F0 */
    /* @bin 0x00484FC1 buildings 1..3999: state 2 refuses, state 6 forbids the undo */
    for (int32_t i = 1; i < BUILDING_COUNT; i++) {
        uint8_t s = U8(st, BUILDING_ADDR(i));
        if (s == 2) {
            S32(st, 0x00BC9694u) = 0;
            return 0;
        }
        if (s == 6) {
            S32(st, 0x00BC9694u) = 0;
        }
    }
    /* @bin 0x00484FE8 grids to their backups (0x0040F860) */
    static const uint32_t copies[6][3] = {
        {0x00B7482Cu, 0x00A1126Cu, 0x32C40u}, /* images */
        {0x00AF598Cu, 0x009DE62Cu, 0x32C40u}, /* terrain */
        {0x00AE8E7Cu, 0x009D1B1Cu, 0xCB10u},  /* canals */
        {0x00B285CCu, 0x009B84FCu, 0xCB10u},  /* bitfields */
        {0x00B67D1Cu, 0x009AB9ECu, 0xCB10u},  /* edge */
        {0x00ADC36Cu, 0x009C500Cu, 0xCB10u},  /* sprite */
    };
    for (int i = 0; i < 6; i++) {
        memcpy(state_ptr(st, copies[i][1], copies[i][2]), state_ptr(st, copies[i][0], copies[i][2]), copies[i][2]);
    }
    return 1;
}

void construction_undo_button(game_state *st)
{
    /* @bin 0x00406010 */
    GAME_CALL0(st, 0x00485230u);
    U8(st, REDRAW) = 1;
}

/* ---- mouse ---- */

void construction_mouse_down(game_state *st)
{
    /* @bin 0x00405C20 */
    if (S32(st, HOVER_OFFSET) == 0 || U8(st, GATE) != 0) {
        return;
    }
    int32_t y = S32(st, HOVER_Y), x = S32(st, HOVER_X);
    S32(st, Y1) = y;
    S32(st, Y2) = y;
    S32(st, X1) = x;
    S32(st, X2) = x;
    U8(st, DRAGGED) = 0;
    int32_t off = S32(st, MAP_START) + y * GRID_W + x;
    S32(st, START_OFFSET) = off;
    S32(st, 0x00BC96CCu) = U8(st, ELEVATION_GRID + (uint32_t) off);
    /* @bin 0x00405C83 no undo snapshot for the first landing of a ferry */
    if (S32(st, FERRY_STATE) != 0 || S32(st, TYPE) != 0x88) {
        if (construction_undo_snapshot(st) == 0) {
            return;
        }
    }
    S32(st, IN_PROGRESS) = 1;
    if (S32(st, TYPE) == 5) {
        GAME_CALL(st, 0x0051B680u, 1, S32(st, X1), S32(st, Y1), 0);
    }
    if (S32(st, TYPE) == 6) {
        GAME_CALL(st, 0x0051C790u, 1, S32(st, X1), S32(st, Y1));
    }
    if (S32(st, TYPE) == 0xA8) {
        GAME_CALL(st, 0x0051C790u, 1, S32(st, X1), S32(st, Y1));
    }
    if (S32(st, TYPE) == 0xA9) {
        GAME_CALL(st, 0x0051C790u, 1, S32(st, X1), S32(st, Y1));
    }
    if (S32(st, TYPE) == 8) {
        GAME_CALL(st, 0x0051B680u, 1, S32(st, X1), S32(st, Y1), 1);
    }
    if (S32(st, TYPE) == 7) {
        GAME_CALL(st, 0x0051B680u, 1, S32(st, X1), S32(st, Y1), 1);
    }
}

void construction_drag(game_state *st)
{
    /* @bin 0x00405D90 */
    if (S32(st, IN_PROGRESS) == 0) {
        return;
    }
    int32_t off = S32(st, HOVER_OFFSET);
    if (off == 0) {
        return;
    }
    U8(st, DRAGGED) = 1;
    int32_t x2, y2;
    if (U8(st, FROZEN) == 0) {
        y2 = S32(st, HOVER_Y);
        S32(st, DRAG_OFFSET) = off;
        x2 = S32(st, HOVER_X);
        S32(st, Y2) = y2;
        S32(st, X2) = x2;
    } else {
        x2 = S32(st, X2);
        y2 = S32(st, Y2);
    }
    int32_t t = S32(st, TYPE);
    /* @bin 0x00405DE3 the map editor's drag (0x00478440) except for its tool types 5, 0x58,
     * 0x59, 0x5D */
    if (S32(st, EDITOR) == 0 || t == 5 || t == 0x58 || t == 0x59 || t == 0x5D) {
        construction_preview(st, S32(st, X1), S32(st, Y1), x2, y2, t);
        return;
    }
    GAME_CALL(st, 0x00478440u, x2, y2);
}

void construction_mouse_up(game_state *st)
{
    /* @bin 0x00405E90 */
    if (S32(st, EDITOR) == 1) {
        GAME_CALL0(st, 0x005178B0u);
    }
    int32_t t = S32(st, TYPE);
    if (t == 0x7B || t == 0x7C || t == 0x81) {
        GAME_CALL0(st, 0x004795E0u);
        S32(st, IN_PROGRESS) = 0;
        return;
    }
    if (S32(st, IN_PROGRESS) == 0) {
        return;
    }
    S32(st, IN_PROGRESS) = 0;
    if (S32(st, MOUSE_OFFSET) == 0) {
        /* @bin 0x00405F02 off the map: the last dragged tile, or nothing */
        if (U8(st, DRAGGED) == 0) {
            return;
        }
        S32(st, MOUSE_OFFSET) = S32(st, DRAG_OFFSET);
    }
    if (t > 0) {
        GAME_CALL(st, 0x0047D350u, 4); /* place sound */
        t = S32(st, TYPE);
    }
    GAME_CALL(st, 0x004728A0u, S32(st, CURRENT_PLAYER_ADDR), S32(st, VIEW_ORIENTATION), S32(st, X1), S32(st, Y1),
              S32(st, X2), S32(st, Y2), t);
}

/* ---- drag preview ---- */

/* @bin 0x00486A30 (x, y, x1, y1, size, d, d1, mode, &length) the second landing of a ferry at
 * (x, y) reaches the first one at (x1, y1): both have a road tile next to them (0x004806C0), a
 * path joins them (0x0051DE70, 0x0051E720; its length into *length), (x1, y1) lies within the
 * diamond of 2 * 0x00BC96E4 rows grown from (x, y) in direction d, and d == d1 */
static int32_t ferry_landings_joined(game_state *st, int32_t x, int32_t y, int32_t x1, int32_t y1, int32_t size,
                                     int32_t d, int32_t d1, int32_t mode, int32_t *length)
{
    int32_t found = 0;
    int32_t o = S32(st, VIEW_ORIENTATION);
    int32_t ax = x, ay = y;
    if (o != 0) {
        if (o == 6) {
            ay = y - 1;
        } else {
            ax = x - 1;
            if (o == 4) {
                ay = y - 1;
            }
        }
    }
    if (GAME_CALL(st, 0x004806C0u, ax, ay, size) == 0) {
        return 0;
    }
    int32_t rx0 = S32(st, 0x00D3B4E4u), ry0 = S32(st, 0x00D3B4E0u);
    int32_t bx = x1, by = y1;
    if (S32(st, FERRY_STATE) == 0 && o != 0) {
        if (o == 6) {
            by = y1 - 1;
        } else {
            bx = x1 - 1;
            if (o == 4) {
                by = y1 - 1;
            }
        }
    }
    if (GAME_CALL(st, 0x004806C0u, bx, by, size) == 0) {
        return 0;
    }
    int32_t rx1 = S32(st, 0x00D3B4E4u), ry1 = S32(st, 0x00D3B4E0u);
    GAME_CALL(st, 0x0051DE70u, rx0, ry0, rx1, ry1);
    int32_t len = GAME_CALL(st, 0x0051E720u, 0x3E9, rx0, ry0, rx1, ry1, 0, mode);
    *length = len;
    int32_t rows = S32(st, 0x00BC96E4u) * 2;
    int32_t c = 0;
    if (rows >= 0) {
        int32_t neg = 0;
        for (int32_t i = 0; i <= rows; i++, neg--) {
            for (int32_t a = -c, b = c; a <= c; a++, b--) {
                int32_t dx, dy;
                if (d == 0) {
                    dx = a;
                    dy = neg;
                } else if (d == 2) {
                    dx = i;
                    dy = b;
                } else if (d == 4) {
                    dx = b;
                    dy = i;
                } else {
                    dx = neg;
                    dy = a;
                }
                if (dx + x == x1 && dy + y == y1) {
                    found = 1;
                }
            }
            c++;
            if (c > 5) {
                c = 5;
            }
        }
    }
    return d == d1 && len != 0 && found;
}

static int32_t run_ferry_landings_joined(game_state *st, const int32_t *a)
{
    int32_t len = 0;
    int32_t r = ferry_landings_joined(st, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], &len);
    if (a[8] != 0) {
        P32(st, a[8]) = (uint32_t) len;
    }
    return r;
}

void construction_preview(game_state *st, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t type)
{
    /* @bin 0x0046FEF0 the ghost of the building in the real grids (restored from the backups by
     * the place step), its cost 0x00BC9724 and the ferry state */
    S32(st, COST) = 0;
    S32(st, REFUND) = 0;
    if (type == 0) {
        return;
    }
    if (S32(st, CURRENT_CITY(st) + 0xC) <= -5000) {
        return;
    }
    GAME_CALL(st, 0x0040F920u, 0x00B285CC, 0xCB10, 0xAF);
    S32(st, COST) = S32(st, 0x00705EA4u + 40u * (uint32_t) type); /* @bin 0x0046FF46 model cost */
    int32_t size = S32(st, 0x005D06DCu + 20u * (uint32_t) type);  /* @bin 0x0046FF4D */
    S32(st, SIZE) = size;
    int32_t r;
    switch (type) {
    case 0x1F:
        GAME_CALL(st, 0x00440700u, 1, x1, y1, x2, y2);
        goto by_count;
    case 0x1E:
        GAME_CALL(st, 0x00440850u, 1, x1, y1, x2, y2);
        goto by_count;
    case 0x21:
        GAME_CALL(st, 0x00440B10u, 1, x1, y1, x2, y2);
        goto by_count;
    case 0xD1:
        GAME_CALL(st, 0x00440ED0u, 1, x1, y1, x2, y2);
        goto by_count;
    case 9:
        GAME_CALL(st, 0x004743F0u, 1, x1, y1, x2, y2);
        goto by_count;
    case 7:
        GAME_CALL(st, 0x0047EDA0u, 1, x1, y1, x2, y2);
        S32(st, 0x00BCA328u) = 1;
        GAME_CALL(st, 0x0047BAE0u, 0, 0, S32(st, MAP_W) - 1, S32(st, MAP_H) - 1, 0);
        S32(st, NO_ROOM) = 0;
        goto done;
    case 6:
        GAME_CALL(st, 0x00475220u, 1, x1, y1, x2, y2, 0x4000);
        goto by_count;
    case 0xA8:
        GAME_CALL(st, 0x00475220u, 1, x1, y1, x2, y2, 0x404000);
        goto by_count;
    case 0xA9:
        GAME_CALL(st, 0x00475220u, 1, x1, y1, x2, y2, 0x804000);
        goto by_count;
    case 5:
        GAME_CALL(st, 0x00474F50u, 1, x1, y1, x2, y2);
        goto by_count;
    case 0x26:
        GAME_CALL(st, 0x004754A0u, 1, x1, y1, x2, y2);
        goto by_count;
    case 0x27:
        GAME_CALL(st, 0x004759E0u, 1, x1, y1, x2, y2);
        goto by_count;
    case 8:
        GAME_CALL(st, 0x00475090u, 1, x1, y1, x2, y2);
        goto by_count;
    case 0xA:
        GAME_CALL(st, 0x00474130u, 1, x1, y1, x2, y2);
        goto by_count;
    case 0x52:
    case 0x53:
        /* @bin 0x00470149 bridges: cost by length */
        if (S32(st, BRIDGE_LENGTH) > 1) {
            S32(st, COST) = (int32_t) ((uint32_t) S32(st, BRIDGE_LENGTH) * (uint32_t) S32(st, COST));
        }
        goto done;
    case 0xAA:
    case 0xC8:
    case 0x3A:
    case 0xC9:
        GAME_CALL(st, 0x0047D360u, 2, x2, y2, -0x41, 0, 0);
        goto done;
    case 0xAB:
    case 0xCA:
        GAME_CALL(st, 0x004BFA50u, x2, y2, S32(st, ROTATION));
        goto done;
    case 0x38:
    case 0xCF:
        GAME_CALL(st, 0x0047D360u, 3, x2, y2, -0x41, 0, 0);
        goto done;
    case 0xD0:
        GAME_CALL(st, 0x0047D360u, 5, x2, y2, -0x41, 0, 0);
        goto done;
    case 0x48:
    case 0xC1:
        GAME_CALL(st, 0x0047D360u, 3, x2, y2, -1, 0, 0);
        goto done;
    case 0x28:
    case 0x2C:
    case 0x2D:
        /* @bin 0x0047026B at most 6 forts (0x00BCB9B4) */
        if (S32(st, 0x00BCB9B4u) >= 6) {
            return;
        }
        GAME_CALL(st, 0x0047D530u, 3, x2, y2, -1, 1);
        goto done;
    case 0xD2:
        GAME_CALL(st, 0x0047D530u, 6, x2, y2, -1, 1);
        goto done;
    case 0x41: case 0x42: case 0x43: case 0x44: case 0x45:
    case 0x9C: case 0x9D: case 0x9E: case 0x9F: case 0xA0:
        GAME_CALL(st, 0x0047D530u, 3, x2, y2, -1, 1);
        goto done;
    case 0x4C:
    case 0xB5:
    case 0x5A:
        r = GAME_CALL(st, 0x0047CC70u, x2, y2, 1, 0);
        goto no_room;
    case 0xB6:
        if (GAME_CALL(st, 0x0047D060u, x2, y2, 1) == 0) {
            S32(st, NO_ROOM) = 1;
        }
        /* @bin 0x0047032A at most 6 warship wharves (0x00BCB9B0) */
        if (S32(st, 0x00BCB9B0u) >= 6) {
            return;
        }
        r = GAME_CALL(st, 0x0047D060u, x2, y2, 1);
        goto no_room;
    case 0x4A:
    case 0x4B:
        r = GAME_CALL(st, 0x0047D060u, x2, y2, 1);
        goto no_room;
    case 0x88:
        goto ferry;
    case 0x54: case 0x55: case 0x56: case 0x57: case 0xBB: case 0xBC: case 0xBD:
        /* @bin 0x00470519 i16 CITY+0x4486 */
        if (S16(st, CURRENT_CITY(st) + 0x4486u) != 0) {
            goto done;
        }
        GAME_CALL(st, 0x0047D360u, size, x2, y2, -1, 0, 0);
        goto done;
    case 0x4D:
    case 0x4E:
    case 0x4F:
        /* @bin 0x00470549 i32 CITY+0x499C */
        if (S32(st, CURRENT_CITY(st) + 0x499Cu) != 0) {
            goto done;
        }
        if (S32(st, FLAG_B4) != 0) {
            if (GAME_CALL(st, 0x004829B0u, x2, y2, size, 0, 0x80, S32(st, FLAG_B4)) == 0) {
                return;
            }
            size = S32(st, SIZE);
        }
        GAME_CALL(st, 0x0047D360u, size, x2, y2, -1, 0, 0);
        goto done;
    case 0x5F:
        if (S32(st, 0x00705C6Cu) > 0) {
            goto done;
        }
        GAME_CALL(st, 0x0047D360u, size, x2, y2, -1, 0, 0);
        goto done;
    case 0x5E:
        /* @bin 0x004705B3 i32 CITY+0x49D0 */
        if (S32(st, CURRENT_CITY(st) + 0x49D0u) != 0) {
            goto done;
        }
        GAME_CALL(st, 0x0047D360u, size, x2, y2, -1, 0, 0);
        goto done;
    case 0x32:
        /* @bin 0x004705E3 i32 CITY+0x4600 */
        if (S32(st, CURRENT_CITY(st) + 0x4600u) != 0) {
            goto done;
        }
        GAME_CALL(st, 0x0047D360u, size, x2, y2, -1, 0, 0);
        goto done;
    case 0xD5: case 0xDE: case 0xE1: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xEA: case 0xEB:
    case 0xEC:
        goto done;
    default:
        break;
    }
    {
        /* @bin 0x00470613 other types: the placement flags (docs/commands.md 2.3) */
        int32_t on_roads = (uint8_t) GAME_CALL(st, 0x0046FDE0u, type);
        if (on_roads == 0 && GAME_CALL(st, 0x004828E0u, x2, y2, S32(st, SIZE), 0, 0x10000) != 0) {
            return;
        }
        if (S32(st, FLAG_CC) != 0) {
            r = GAME_CALL(st, 0x00482650u, x2, y2, 3, 1, 0x10800);
        } else if (S32(st, PREDICATE) != 0) {
            r = GAME_CALL(st, 0x00482720u, x2, y2, 2, 1, S32(st, PREDICATE));
        } else if (S32(st, FLAG_C8) != 0) {
            r = GAME_CALL(st, 0x00482650u, x2, y2, 2, 1, 1);
        } else if (S32(st, FLAG_C4) != 0) {
            r = GAME_CALL(st, 0x00482650u, x2, y2, 2, 3, 4);
        } else if (S32(st, FLAG_BC) != 0) {
            r = GAME_CALL(st, 0x004828E0u, x2, y2, 2, 0, 0x4000);
        } else if (S32(st, FLAG_B8) != 0) {
            if (GAME_CALL(st, 0x004828E0u, x2, y2, 1, 0, 0x40) == 0) {
                return;
            }
            GAME_CALL(st, 0x0047D360u, S32(st, SIZE), x2, y2, -0x41, 0, 0);
            goto done;
        } else if (S32(st, FLAG_B4) != 0) {
            r = GAME_CALL(st, 0x004829B0u, x2, y2, S32(st, SIZE), 0, 0x80, S32(st, FLAG_B4));
        } else if (S32(st, FLAG_C0) != 0) {
            r = GAME_CALL(st, 0x00482650u, x2, y2, 2, 1, 0x40000);
        } else {
            if (type != 0xB7) {
                /* @bin 0x00470741 the last argument is y1's stack slot with its low byte
                 * replaced by the 0x0046FDE0 result */
                GAME_CALL(st, 0x0047D360u, S32(st, SIZE), x2, y2, -1, 0,
                          (int32_t) (((uint32_t) y1 & 0xFFFFFF00u) | (uint32_t) on_roads));
            }
            goto done;
        }
        if (r == 0) {
            return;
        }
        goto done;
    }
ferry: {
        /* @bin 0x00470348 */
        if (GAME_CALL(st, 0x0047CC70u, x2, y2, 1, 0) == 0) {
            S32(st, NO_ROOM) = 1;
        }
        int32_t off = y2 * GRID_W + S32(st, MAP_START) + x2;
        if (GAME_CALL(st, 0x00486CF0u, off) == 0) {
            goto done;
        }
        int32_t f = S32(st, FERRY_STATE);
        int32_t b = S16(st, BUILDING_GRID + 2u * (uint32_t) off);
        if (f == 0) {
            U8(st, FERRY_FLAG) = 0;
            if (S32(st, BUILDING_ADDR(b) + 0xC4) < 0) {
                S32(st, FERRY_STATE) = 2;
                U8(st, FERRY_FLAG) = 1;
                S32(st, FERRY_BUILDING) = S16(st, BUILDING_GRID + 2u * (uint32_t) (y2 * GRID_W + S32(st, MAP_START) + x2));
            }
        } else if (f == 1) {
            int32_t b1 = S32(st, FERRY_BUILDING);
            U8(st, FERRY_FLAG) = 0;
            if (S32(st, BUILDING_ADDR(b) + 0xC4) < 0) {
                uint32_t p1 = BUILDING_ADDR(b1);
                int32_t len = 0;
                int32_t ok = ferry_landings_joined(st, x2, y2, S16(st, p1 + 6), S16(st, p1 + 8), U8(st, p1 + 3),
                                                   (int32_t) U8(st, BUILDING_ADDR(b) + 0xAA) << 1,
                                                   ((int32_t) U8(st, p1 + 0xAA) * 2 + 4) & 7, 0, &len);
                if (ok) {
                    int32_t b2 = S16(st, BUILDING_GRID + 2u * (uint32_t) off);
                    b1 = S32(st, FERRY_BUILDING);
                    S32(st, FERRY_STATE) = 3;
                    U8(st, FERRY_FLAG) = 1;
                    S32(st, BUILDING_ADDR(b2) + 0xC4) = b1;
                    S32(st, BUILDING_ADDR(b1) + 0xC4) = b2;
                    ferry_place_step(st);
                }
            }
        }
        goto done;
    }
no_room:
    if (r == 0) {
        S32(st, NO_ROOM) = 1;
    }
    goto done;
by_count:
    /* @bin 0x0047019F cost by count (0x00BC971C) */
    if (S32(st, COUNT) >= 0) {
        S32(st, COST) = (int32_t) ((uint32_t) S32(st, COUNT) * (uint32_t) S32(st, COST));
    }
done:
    S32(st, COST) = (int32_t) ((uint32_t) S32(st, COST) - (uint32_t) S32(st, REFUND));
}

/* ---- table ---- */

static int32_t run_choose(game_state *st, const int32_t *a)
{
    (void) a;
    construction_choose_building(st);
    return 0;
}

static int32_t run_cancel(game_state *st, const int32_t *a)
{
    (void) a;
    return construction_cancel(st);
}

static int32_t run_mouse_down(game_state *st, const int32_t *a)
{
    (void) a;
    construction_mouse_down(st);
    return 0;
}

static int32_t run_drag(game_state *st, const int32_t *a)
{
    (void) a;
    construction_drag(st);
    return 0;
}

static int32_t run_mouse_up(game_state *st, const int32_t *a)
{
    (void) a;
    construction_mouse_up(st);
    return 0;
}

static int32_t run_undo_button(game_state *st, const int32_t *a)
{
    (void) a;
    construction_undo_button(st);
    return 0;
}

static int32_t run_undo_snapshot(game_state *st, const int32_t *a)
{
    (void) a;
    return construction_undo_snapshot(st);
}

static int32_t run_preview(game_state *st, const int32_t *a)
{
    construction_preview(st, a[0], a[1], a[2], a[3], a[4]);
    return 0;
}

static int32_t run_pred_40(game_state *st, const int32_t *a)
{
    (void) st;
    return terrain_pred_405b40((uint32_t) a[0]);
}

static int32_t run_pred_60(game_state *st, const int32_t *a)
{
    (void) st;
    return terrain_pred_405b60((uint32_t) a[0]);
}

static int32_t run_pred_80(game_state *st, const int32_t *a)
{
    (void) st;
    return terrain_pred_405b80((uint32_t) a[0]);
}

static int32_t run_ferry_half_delete(game_state *st, const int32_t *a)
{
    ferry_half_delete(st, a[0]);
    return 0;
}

static int32_t run_ferry_place_step(game_state *st, const int32_t *a)
{
    (void) a;
    ferry_place_step(st);
    return 0;
}

const ported_function ui_ports[] = {
    {0x00405200u, "construction_choose_building", 0, 0, 0, run_choose, 0},
    {0x00405BA0u, "construction_cancel", 0, 32, 0, run_cancel, 0},
    {0x00405C20u, "construction_mouse_down", 0, 0, 0, run_mouse_down, 0},
    {0x00405D90u, "construction_drag", 0, 0, 0, run_drag, 0},
    {0x00405E90u, "construction_mouse_up", 0, 0, 0, run_mouse_up, 0},
    {0x00406010u, "construction_undo_button", 0, 0, 0, run_undo_button, 0},
    {0x00484F60u, "construction_undo_snapshot", 0, 32, 0, run_undo_snapshot, 0},
    {0x0046FEF0u, "construction_preview", 5, 0, 0, run_preview, 0},
    {0x00405B40u, "terrain_pred_405b40", 1, 8, 0, run_pred_40, 0},
    {0x00405B60u, "terrain_pred_405b60", 1, 8, 0, run_pred_60, 0},
    {0x00405B80u, "terrain_pred_405b80", 1, 8, 0, run_pred_80, 0},
    {0x00486060u, "ferry_half_delete", 1, 0, 0, run_ferry_half_delete, 0},
    {0x00486020u, "ferry_place_step", 0, 0, 0, run_ferry_place_step, 0},
    {0x00486A30u, "ferry_landings_joined", 9, 32, 0, run_ferry_landings_joined, 0},
};
const int ui_ports_count = sizeof(ui_ports) / sizeof(ui_ports[0]);
