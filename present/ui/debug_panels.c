/* Imhotep - the city view's debug panels (roadmap phase 7.6, package frontier7).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 1's per-frame chain (0x004D75D0) ends with four debug panels, each drawn only when its
 * switch is on (the switches are written by the debug keys of the window procedure, inactive in the
 * shipped game: docs/presentation.md 5.3): the city's numbers by player (0x00534610, switch
 * 0x00EA181C), the evolve counters (0x00534DF0, 0x0078D7DC), the building under the mouse
 * (0x00535080, 0x0078D7D8) and the figure under the mouse (0x00536120, 0x0078D7D4). Every line is a
 * label and a number through 0x004D6860 (layout_number_at: value, prefix character, suffix string,
 * x, y in the 640 x 480 layout, font, colour, layout offsets) or a string through 0x004CC770; the
 * text cursor 0x00E20778 (the x after the last string drawn) places the next items. */
#include "ui.h"

#include "sim/call.h"
#include "state/ported.h"

#define DX 0x00E38DA8u          /* i32 (screen width - 640) / 2 */
#define DY 0x00E38DA4u          /* i32 (screen height - 480) / 2 */
#define FONT_SMALL 0x00F56F84u  /* i32 */
#define FONT_PLAIN 0x00F56F8Cu  /* i32 */
#define CURSOR_X 0x00E20778u    /* i32 the text cursor's x */
#define PLAYER 0x00EAAC28u      /* i32 the current player */
#define WHITE 0xFFFF
#define GREEN 0x3EFA
#define RED 0xF800
#define EMPTY 0x0057C048u       /* "" */

/* @bin 0x004D6860 a number at (x, y) of the layout: value, prefix 0x40, suffix */
static void num_p(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, int32_t x, int32_t y, int32_t colour)
{
    GAME_CALL(st, 0x004D6860u, value, prefix, (int32_t) suffix, x, y, S32(st, FONT_SMALL), colour, S32(st, DX),
              S32(st, DY));
}

static void num(game_state *st, int32_t value, uint32_t suffix, int32_t x, int32_t y, int32_t colour)
{
    num_p(st, value, 0x40, suffix, x, y, colour);
}

static void text(game_state *st, uint32_t s, int32_t x, int32_t y, int32_t colour)
{
    GAME_CALL(st, 0x004CC770u, (int32_t) s, x, y, S32(st, FONT_SMALL), colour);
}

static void title(game_state *st, uint32_t s, int32_t y, int32_t font, int32_t colour)
{
    /* @bin 0x004CC720 centred in 0xC0 pixels from x 0 */
    GAME_CALL(st, 0x004CC720u, (int32_t) s, 0, y, 0xC0, S32(st, font), colour);
}

static int32_t cursor(game_state *st)
{
    return S32(st, CURSOR_X);
}

void debug_city_panel(game_state *st)
{
    /* @bin 0x00534610 with the switch 0x00EA181C = 1: for each of the 9 players whose record (0x49D8
     * bytes from 0x00EA627C - 0x32 .. ) is in use (byte -0x32 = 1), from y 0x5E every 0x32 pixels:
     * population, capacity, spare capacity; births, deaths, the people in houses (0x00458F80 of the
     * current player, red when it differs from the population); the docks (0x00705C1C) and the current
     * player's docks list (10 i16 at 0x00EA5CD0); its wine sources; the rotation 0x00BCA2E0, the
     * overlay 0x00E94E0C; natives restless, mission in place; 0x00F55BA0, 0x00F55B9C; the alerts
     * 0x00E951D8, 0x00E951D4; 0x00F535BC; the test values 0x00D3B490 .. 0x00D3B460; the mouse; the
     * sidebar x 0x00F53F64; the construction in progress 0x006059E4 */
    if (S32(st, 0x00EA181Cu) != 1) {
        return;
    }
    uint32_t rec = 0x00EA627Cu;
    for (int32_t y = 0x5E; y <= 0x1BC; y += 0x32, rec += 0x49D8u) {
        int32_t colour = WHITE;
        S32(st, CURSOR_X) = 0;
        if (U8(st, rec - 0x32u) != 1) {
            continue;
        }
        int32_t y0 = y - 0xE;
        S32(st, CURSOR_X) = 0;
        text(st, 0x005F8BC0u, 8, y0, WHITE);
        num(st, S32(st, rec - 0x10u), EMPTY, cursor(st) + 8, y0, WHITE);
        text(st, 0x005F8BB4u, cursor(st) + 8, y0, WHITE);
        num(st, S32(st, rec), EMPTY, cursor(st) + 8, y0, WHITE);
        text(st, 0x005F8BA4u, cursor(st) + 8, y0, WHITE);
        num(st, S32(st, rec + 4), EMPTY, cursor(st) + 8, y0, WHITE);
        S32(st, CURSOR_X) = 0;
        text(st, 0x005F8B98u, 8, y, WHITE);
        num(st, S32(st, rec + 0x26A8u), EMPTY, cursor(st) + 8, y, WHITE);
        text(st, 0x005F8B8Cu, cursor(st) + 8, y, WHITE);
        num(st, S32(st, rec + 0x26ACu), EMPTY, cursor(st) + 8, y, WHITE);
        int32_t housed = GAME_CALL(st, 0x00458F80u, S32(st, PLAYER));
        if (housed != S32(st, rec - 0x10u)) {
            colour = RED;
        }
        text(st, 0x005F8B7Cu, cursor(st) + 8, y, colour);
        num(st, housed, EMPTY, cursor(st) + 8, y, colour);
        int32_t y1 = y + 0xE;
        S32(st, CURSOR_X) = 0;
        text(st, 0x005F8B6Cu, 8, y1, WHITE);
        num(st, S32(st, 0x00705C1Cu), 0x005F8B68u, cursor(st) + 8, y1, WHITE);
        text(st, 0x005F8B58u, cursor(st) + 8, y1, WHITE);
        for (int32_t k = 0; k < 10; k++) {
            int32_t i = S32(st, PLAYER) * 0x24EC + k;
            num(st, S16(st, 0x00EA5CD0u + 2u * (uint32_t) i), 0x005F8B54u, cursor(st) + 4, y1, WHITE);
        }
        uint32_t player = (uint32_t) S32(st, PLAYER) * 0x49D8u;
        S32(st, CURSOR_X) = 0;
        text(st, 0x005F8B40u, 8, y + 0x1C, WHITE);
        num(st, S32(st, 0x00EA5F14u + player), 0x005F8B68u, cursor(st) + 8, y + 0x1C, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x00BCA2E0u), 0x005F8B30u, 8, y + 0x2A, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x00E94E0Cu), 0x005F8B24u, 8, y + 0x38, WHITE);
        S32(st, CURSOR_X) = 0;
        player = (uint32_t) S32(st, PLAYER) * 0x49D8u;
        num(st, S32(st, 0x00EA5E04u + player), 0x005F8B0Cu, 8, y + 0x46, WHITE);
        player = (uint32_t) S32(st, PLAYER) * 0x49D8u;
        num(st, S32(st, 0x00EA5E0Cu + player), 0x005F8AF8u, cursor(st) + 0x6C, y + 0x46, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x00F55BA0u), 0x005F8AECu, 8, y + 0x54, WHITE);
        num(st, S32(st, 0x00F55B9Cu), 0x005F8AE0u, cursor(st) + 0x6C, y + 0x54, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x00E951D8u), 0x005F8AD0u, 8, y + 0x62, WHITE);
        num(st, S32(st, 0x00E951D4u), 0x005F8AB4u, cursor(st) + 0x6C, y + 0x62, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x00F535BCu), 0x005F8A94u, 8, y + 0x70, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x00D3B490u), 0x005F8A84u, 8, y + 0x7E, WHITE);
        num(st, S32(st, 0x00D3B480u), 0x005F8A74u, cursor(st) + 0x6C, y + 0x7E, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x00D3B470u), 0x005F8A64u, 8, y + 0x8C, WHITE);
        num(st, S32(st, 0x00D3B460u), 0x005F8A54u, cursor(st) + 0x6C, y + 0x8C, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x00E38E34u), 0x005F8A48u, 8, y + 0x9A, WHITE);
        num(st, S32(st, 0x00E38E30u), 0x005F8A3Cu, cursor(st) + 0x6C, y + 0x9A, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x00E38E40u), 0x005F8A2Cu, 8, y + 0xA8, WHITE);
        num(st, S32(st, 0x00F53F64u), 0x005F8A1Cu, cursor(st) + 0x6C, y + 0xA8, WHITE);
        S32(st, CURSOR_X) = 0;
        num(st, S32(st, 0x006059E4u), 0x005F8A04u, 8, y + 0xB6, WHITE);
    }
}

void debug_evolve_panel(game_state *st)
{
    /* @bin 0x00534DF0 with the switch 0x0078D7DC = 1: a panel (0x00536DC0) and the evolve counters
     * (tick, week, month, year, last building, ever, connections - 1, total connections; the evolve
     * error, routings and barbarians in red when the error 0x0078D7E0 is not 0) */
    if (S32(st, 0x0078D7DCu) != 1) {
        return;
    }
    int32_t y = S32(st, DY) * 2 + 0xDC;
    GAME_CALL(st, 0x00536DC0u, 6, 0, y, 0xC, 0xF, 1);
    title(st, 0x005F8C58u, y + 6, FONT_PLAIN, WHITE);
    num(st, S32(st, 0x0078D7CCu), 0x005F8C50u, 8, y + 0x1A, GREEN);
    num(st, S32(st, 0x0078D7C4u), 0x005F8C48u, 0x6E, y + 0x1A, GREEN);
    num(st, S32(st, 0x0078D7C0u), 0x005F8C3Cu, 8, y + 0x28, GREEN);
    num(st, S32(st, 0x0078D7BCu), 0x005F8C34u, 0x6E, y + 0x28, GREEN);
    num(st, S32(st, 0x0078D790u), 0x005F8C20u, 8, y + 0x36, GREEN);
    num(st, S32(st, 0x0078D78Cu), 0x005F8C18u, 0x6E, y + 0x36, GREEN);
    num(st, S32(st, 0x0078D7E8u) - 1, 0x005F8C04u, 8, y + 0x44, GREEN);
    num(st, S32(st, 0x0078D794u), 0x005F8BECu, 8, y + 0x60, GREEN);
    int32_t error = S32(st, 0x0078D7E0u);
    int32_t colour = error != 0 ? RED : GREEN;
    num(st, error, 0x005F8BDCu, 8, y + 0x6E, colour);
    num(st, S32(st, 0x00EADFD4u), 0x005F8BD0u, 8, y + 0x7C, colour);
    num(st, S32(st, 0x00EADFD8u), 0x005F8BC8u, 8, y + 0x8A, colour);
}

#define B(off) ((off) + b)

void debug_building_panel(game_state *st)
{
    /* @bin 0x00535080 with the switch 0x0078D7D8 = 1: a panel (0x00536DC0); the tile under the mouse
     * (0x00430790(0) into 0x00BCA2C4) and its building (i16 grid 0x00B4E6FC). Without one: the
     * pointer, the hovered item's text (group 0x10, 0x00E38E94), the undo state, the alive flags,
     * 8 bytes of the grid 0x00ED4140 around the tile (offsets 0x005F80FC), the redraw flag
     * 0x00E38DC9. With one: its fields (record 0x108 bytes at 0x007999D8; green, the number in red
     * when the building is not in use), by type: aqueduct links, the 7 i16 of 0x47 / 0xBF, the raw
     * materials of the industries; for houses their services and the house level's needs (0x58-byte
     * records 0x0070CDD4 by sub-type), else the shows; then the pointer and the goods */
    if (S32(st, 0x0078D7D8u) != 1) {
        return;
    }
    int32_t y = S32(st, DY) * 2 + 0x9C;
    GAME_CALL(st, 0x00536DC0u, 6, 0, y, 0xC, 0x13, 1);
    int32_t tile = GAME_CALL(st, 0x00430790u, 0);
    S32(st, 0x00BCA2C4u) = tile;
    int32_t id = tile == 0 ? 0 : S16(st, 0x00B4E6FCu + 2u * (uint32_t) tile);
    title(st, 0x005F8E4Cu, y + 6, FONT_PLAIN, WHITE);
    if (id == 0) {
        title(st, 0x005F8E38u, y + 0x78, FONT_SMALL, GREEN);
        num(st, S32(st, 0x00BCA2C4u), 0x005F8E2Cu, 8, y + 0x8C, WHITE);
        GAME_CALL(st, 0x004D6250u, 0x10, S32(st, 0x00E38E94u), 8, y + 0xA0, 0xC0, S32(st, FONT_SMALL), WHITE,
                  S32(st, DX), S32(st, DY));
        num(st, S32(st, 0x00BC968Cu), 0x005F8E1Cu, 8, y + 0xB4, WHITE);
        num(st, S32(st, 0x00BC9694u), 0x005F8E0Cu, 8, y + 0xC8, WHITE);
        num(st, S32(st, 0x00BC95F0u), 0x005F8DF8u, 8, y + 0xDC, WHITE);
        int32_t alive = S16(st, 0x00BA746Cu);
        num(st, U8(st, 0x007999D8u + (uint32_t) (alive * 0x108)), 0x005F8DECu, 8, y + 0xF0, WHITE);
        num(st, (int32_t) (U32(st, 0x00BA7888u) & 0xFF), 0x005F8DECu, 0x64, y + 0xF0, WHITE);
        S32(st, CURSOR_X) = 0;
        for (uint32_t o = 0x005F80FCu; o < 0x005F811Cu; o += 4) {
            int32_t x = cursor(st) + 8;
            int32_t v = (int8_t) U8(st, 0x00ED4140u + (uint32_t) S32(st, o) + (uint32_t) S32(st, 0x00BCA2C4u));
            num(st, v, 0x005E0400u, x, y + 0x104, GREEN);
        }
        num(st, U8(st, 0x00E38DC9u), 0x005F8DE0u, 0x64, y + 0x118, WHITE);
        return;
    }
    uint32_t b = (uint32_t) id * 0x108u;
    num(st, id, 0x005F8DD0u, 8, y + 0x18, U8(st, B(0x007999D8u)) != 1 ? RED : GREEN);
    num(st, S16(st, B(0x007999E8u)), 0x005F8DC8u, 8, y + 0x3C, GREEN);
    num(st, S16(st, B(0x007999EAu)), 0x005F8DBCu, 0x64, y + 0x3C, GREEN);
    int16_t type = S16(st, B(0x007999E8u));
    if (type == 0x5A) {
        S32(st, CURSOR_X) = 0;
        text(st, 0x005F8DB0u, 8, y + 0x48, GREEN);
        for (uint32_t k = 0; k < 4; k++) {
            num(st, S16(st, B(0x00799A76u) + 2 * k), EMPTY, cursor(st) + 8, y + 0x48, GREEN);
        }
    } else if (type == 0x47 || type == 0xBF) {
        S32(st, CURSOR_X) = 0;
        text(st, 0x005F8D98u, 8, y + 0x48, GREEN);
        for (uint32_t k = 0; k < 7; k++) {
            num(st, S16(st, B(0x00799A2Eu) + 2 * k), EMPTY, cursor(st) + 8, y + 0x48, GREEN);
            S32(st, CURSOR_X) = S32(st, CURSOR_X) - 4;
        }
    } else {
        int raw = type >= 0x6E && type <= 0x72;
        switch (type) {
        case 0x20: case 0x2F: case 0x33: case 0x35: case 0x4A: case 0x71: case 0xB1: case 0xB2: case 0xBB:
        case 0xBC: case 0xBD: case 0xC2: case 0xCB: case 0xCC: case 0xCD: case 0xE2: case 0xE7: case 0xE8:
        case 0xE9:
            raw = 1; /* @bin 0x005354BF the jump table 0x00536044 / 0x0053604C */
            break;
        default:
            break;
        }
        if (raw) {
            S32(st, CURSOR_X) = 0;
            text(st, 0x005F8DA0u, 8, y + 0x48, GREEN);
            num(st, S16(st, B(0x00799A14u)), EMPTY, cursor(st) + 8, y + 0x48, GREEN);
        }
    }
    num(st, S16(st, B(0x007999F4u)), 0x005F8D90u, 8, y + 0x54, GREEN);
    num(st, S16(st, B(0x00799A14u)), 0x005F8D84u, 0x64, y + 0x54, GREEN);
    num(st, S16(st, B(0x007999F0u)), 0x005F8D7Cu, 8, y + 0x60, GREEN);
    num(st, S16(st, B(0x007999F2u)), 0x005F8D74u, 0x64, y + 0x60, GREEN);
    num(st, S16(st, B(0x00799A06u)), 0x005F8D64u, 8, y + 0x6C, GREEN);
    if (S16(st, B(0x00799A04u)) != 0) {
        num(st, S16(st, B(0x00799A04u)), 0x005F8D54u, 0x64, y + 0x6C, GREEN);
    } else if (S16(st, B(0x00799A02u)) != 0) {
        num(st, S16(st, B(0x00799A02u)), 0x005F8D48u, 0x64, y + 0x6C, GREEN);
    }
    num(st, S16(st, B(0x007999F8u)), 0x005F8D40u, 8, y + 0x78, GREEN);
    num(st, U8(st, B(0x007999ECu)), 0x005F8D34u, 0x64, y + 0x78, GREEN);
    num(st, S16(st, B(0x007999F6u)), 0x005F8D24u, 8, y + 0x84, GREEN);
    num(st, S16(st, B(0x00799A1Eu)), 0x005F8D18u, 0x64, y + 0x84, GREEN);
    if (U8(st, B(0x00799A0Du)) != 0) {
        num(st, U8(st, B(0x00799A0Du)), 0x005F8D08u, 8, y + 0x90, GREEN);
    } else if (U8(st, B(0x00799A17u)) != 0) {
        num(st, U8(st, B(0x00799A17u)), 0x005F8CFCu, 8, y + 0x90, GREEN);
    } else {
        num(st, 0, 0x005F8CF0u, 8, y + 0x90, GREEN);
    }
    num(st, U8(st, B(0x00799A0Eu)), 0x005F8CE0u, 0x64, y + 0x90, GREEN);
    num(st, S16(st, B(0x007999FCu)), 0x005F8CC8u, 8, y + 0x9C, GREEN);
    num(st, S16(st, B(0x00799A18u)), 0x005F8CB8u, 0x64, y + 0xA8, GREEN);
    num(st, U8(st, B(0x00799A26u)), 0x005F8CACu, 8, y + 0xB4, GREEN);
    num(st, S16(st, B(0x007999FAu)), 0x005F8CA0u, 0x64, y + 0xB4, GREEN);
    if (U8(st, B(0x007999DDu)) != 0) {
        int32_t y1 = y + 0xC0;
        S32(st, CURSOR_X) = 0;
        num(st, U8(st, B(0x00799A44u)), 0x005F8C9Cu, 8, y1, GREEN);
        S32(st, CURSOR_X) = 0;
        num(st, U8(st, B(0x00799A45u)), 0x005E0400u, 0x32, y1, GREEN);
        S32(st, CURSOR_X) = 0;
        num(st, U8(st, B(0x00799A47u)), EMPTY, 0x64, y1, GREEN);
        S32(st, CURSOR_X) = 0;
        num(st, U8(st, B(0x00799A49u)), 0x005F8C98u, 0x96, y1, GREEN);
        S32(st, CURSOR_X) = 0;
        num(st, U8(st, B(0x00799A5Eu)), 0x005F8C98u, 0xC8, y1, GREEN);
        int32_t y2 = y + 0xF0;
        num(st, (int8_t) U8(st, B(0x00799A58u)), 0x005F8C94u, 8, y2, GREEN);
        num(st, (int8_t) U8(st, B(0x00799A59u)), 0x005F8C90u, 0x32, y2, GREEN);
        num(st, (int8_t) U8(st, B(0x00799A5Au)), 0x005F8C8Cu, 0x64, y2, GREEN);
        num(st, (int8_t) U8(st, B(0x00799A5Bu)), 0x005F8C88u, 0x96, y2, GREEN);
        int32_t y3 = y + 0xFC;
        static const uint32_t needs[4] = {0x0070CDD4u, 0x0070CDE0u, 0x0070CDDCu, 0x0070CDF0u};
        static const int32_t xs[4] = {8, 0x32, 0x64, 0x96};
        for (int k = 0; k < 4; k++) {
            int32_t level = S16(st, B(0x007999EAu));
            num_p(st, S32(st, needs[k] + (uint32_t) (level * 0x58)), 0x28, 0x005F7E78u, xs[k], y3, GREEN);
        }
    } else {
        int32_t y1 = y + 0xC0;
        num(st, U8(st, B(0x00799A7Eu)), 0x005F8C80u, 8, y1, GREEN);
        S32(st, CURSOR_X) = 0;
        num(st, U8(st, B(0x00799A7Fu)), 0x005F8C7Cu, 0x6C, y1, GREEN);
        num(st, U8(st, B(0x00799A80u)), 0x005F8C78u, cursor(st) + 0x6C, y1, GREEN);
        num(st, U8(st, B(0x00799A81u)), 0x005F8C78u, cursor(st) + 0x6C, y1, GREEN);
    }
    num(st, S32(st, 0x00BCA2C4u), 0x005F8E2Cu, 8, y + 0xCC, WHITE);
    static const uint32_t house_goods[5] = {0x00799A2Cu, 0x00799A3Eu, 0x00799A42u, 0x00799A40u, 0x00799A3Cu};
    static const uint32_t store_goods[5] = {0x00799A2Eu, 0x00799A40u, 0x00799A44u, 0x00799A42u, 0x00799A3Eu};
    const uint32_t *goods;
    if (U8(st, B(0x007999DDu)) != 0) {
        goods = house_goods;
    } else if (S16(st, B(0x007999E8u)) == 0x46 || S16(st, B(0x007999E8u)) == 0xBE) {
        goods = store_goods;
    } else {
        return;
    }
    int32_t y4 = y + 0xDC;
    S32(st, CURSOR_X) = 0;
    num(st, S16(st, B(goods[0])), 0x005F8C74u, 8, y4, GREEN);
    for (int k = 1; k < 5; k++) {
        num(st, S16(st, B(goods[k])), k == 4 ? 0x005F8C6Cu : 0x005F8C74u, cursor(st) + 8, y4, GREEN);
    }
}

void debug_figure_panel(game_state *st)
{
    /* @bin 0x00536120 with the switch 0x0078D7D4 = 1: a panel; the watched figure 0x00F535CC becomes
     * the one on the tile under the mouse (i16 grid 0x00B350DC; kept when the tile has none, 0 off the
     * map). Without one: the figure counts and the free route slots (0x004B3320). With one: its
     * fields (record 0x184 bytes at 0x00BCD5F0; the number in red when the figure is not in use) and
     * the dockers (type 0x26 in use) counted over the figures */
    if (S32(st, 0x0078D7D4u) != 1) {
        return;
    }
    int32_t y = S32(st, DY) * 2 + 0xDC;
    GAME_CALL(st, 0x00536DC0u, 6, 0, y, 0xC, 0xF, 1);
    int32_t tile = GAME_CALL(st, 0x00430790u, 0);
    S32(st, 0x00BCA2C4u) = tile;
    if (tile == 0) {
        S32(st, 0x00F535CCu) = 0;
    } else if (S16(st, 0x00B350DCu + 2u * (uint32_t) tile) != 0) {
        S32(st, 0x00F535CCu) = S16(st, 0x00B350DCu + 2u * (uint32_t) tile);
    }
    title(st, 0x005F8F14u, y + 6, FONT_PLAIN, WHITE);
    if (S32(st, 0x00F535CCu) == 0) {
        title(st, 0x005F8EFCu, y + 0x18, FONT_PLAIN, WHITE);
        num(st, S32(st, 0x00D056F4u), 0x005F8EF4u, 8, y + 0x30, GREEN);
        num(st, S32(st, 0x00BCD5F0u), 0x005F8C18u, 0x6E, y + 0x30, GREEN);
        int32_t slots = GAME_CALL0(st, 0x004B3320u);
        num(st, slots, 0x005F8EE0u, 8, y + 0x3C, GREEN);
        return;
    }
#define F(off) ((off) + (uint32_t) S32(st, 0x00F535CCu) * 0x184u)
    int32_t fig = S32(st, 0x00F535CCu);
    num(st, fig, 0x005F8ED0u, 8, y + 0x18, U8(st, F(0x00BCD606u)) != 1 ? RED : GREEN);
    num(st, U8(st, F(0x00BCD607u)), 0x005F8EC4u, 0x6E, y + 0x18, GREEN);
    S32(st, CURSOR_X) = 0;
    num(st, S16(st, F(0x00BCD642u)), 0x005E0400u, 8, y + 0x30, GREEN);
    num(st, S16(st, F(0x00BCD644u)), 0x005F8EC0u, cursor(st) + 8, y + 0x30, GREEN);
    S32(st, CURSOR_X) = 0;
    num(st, S16(st, F(0x00BCD60Cu)), 0x005E0400u, 0x64, y + 0x30, GREEN);
    num(st, S16(st, F(0x00BCD60Eu)), 0x005F8EB4u, cursor(st) + 0x64, y + 0x30, GREEN);
    S32(st, CURSOR_X) = 0;
    num(st, U8(st, F(0x00BCD642u)) & 0xF, 0x005E0400u, 8, y + 0x48, GREEN);
    num(st, U8(st, F(0x00BCD644u)) & 0xF, 0x005F8EA8u, cursor(st) + 8, y + 0x48, GREEN);
    S32(st, CURSOR_X) = 0;
    num(st, S16(st, F(0x00BCD652u)), 0x005F8E98u, 8, y + 0x54, GREEN);
    num(st, S16(st, F(0x00BCD654u)), 0x005F8E84u, cursor(st) + 8, y + 0x54, GREEN);
    S32(st, CURSOR_X) = 0;
    num(st, U8(st, F(0x00BCD65Cu)), 0x005F8E78u, 8, y + 0x60, GREEN);
    num(st, U8(st, F(0x00BCD65Du)), 0x005F8E6Cu, cursor(st) + 8, y + 0x60, GREEN);
#undef F
    int32_t dockers = 0;
    for (uint32_t a = 0x00BCD786u; a < 0x00C8AD42u; a += 0x184u) {
        if (U8(st, a + 4) == 1 && U8(st, a) == 0x26) {
            dockers++;
        }
    }
    S32(st, CURSOR_X) = 0;
    num(st, dockers, 0x005F8E60u, 8, y + 0x6C, GREEN);
}

#define RUN0(f) static int32_t run_##f(game_state *st, const int32_t *a) { (void) a; f(st); return 0; }
RUN0(debug_city_panel)
RUN0(debug_evolve_panel)
RUN0(debug_building_panel)
RUN0(debug_figure_panel)

const ported_function ui_debug_panel_ports[] = {
    {0x00534610u, "debug_city_panel", 0, 0, 0, run_debug_city_panel, 0},
    {0x00534DF0u, "debug_evolve_panel", 0, 0, 0, run_debug_evolve_panel, 0},
    {0x00535080u, "debug_building_panel", 0, 0, 0, run_debug_building_panel, 0},
    {0x00536120u, "debug_figure_panel", 0, 0, 0, run_debug_figure_panel, 0},
};
const int ui_debug_panel_ports_count = sizeof(ui_debug_panel_ports) / sizeof(ui_debug_panel_ports[0]);
