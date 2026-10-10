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

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"

#define WHITE 0xFFFF
#define GREEN 0x3EFA
#define RED 0xF800

/* @bin 0x004D6860 a number at (x, y) of the layout: value, prefix 0x40, suffix */
static void num_p(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, int32_t x, int32_t y, int32_t colour)
{
    GAME_CALL(st, FN_LAYOUT_NUMBER_AT, value, prefix, (int32_t) suffix, x, y, S32(st, G_FONT_SMALL), colour, S32(st, G_LAYOUT_DX),
              S32(st, G_LAYOUT_DY));
}

static void num(game_state *st, int32_t value, uint32_t suffix, int32_t x, int32_t y, int32_t colour)
{
    num_p(st, value, 0x40, suffix, x, y, colour);
}

static void text(game_state *st, uint32_t s, int32_t x, int32_t y, int32_t colour)
{
    GAME_CALL(st, FN_TEXT_DRAW, (int32_t) s, x, y, S32(st, G_FONT_SMALL), colour);
}

static void title(game_state *st, uint32_t s, int32_t y, int32_t font, int32_t colour)
{
    /* @bin 0x004CC720 centred in 0xC0 pixels from x 0 */
    GAME_CALL(st, FN_TEXT_DRAW_CENTRED, (int32_t) s, 0, y, 0xC0, S32(st, font), colour);
}

static int32_t cursor(game_state *st)
{
    return S32(st, G_TEXT_X);
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
    if (S32(st, G_DEBUG_CITY_PANEL) != 1) {
        return;
    }
    uint32_t rec = CITY_ADDR(1) + CITY_HOUSE_CAPACITY;
    for (int32_t y = 0x5E; y <= 0x1BC; y += 0x32, rec += 0x49D8u) {
        int32_t colour = WHITE;
        S32(st, G_TEXT_X) = 0;
        if (U8(st, rec - (CITY_HOUSE_CAPACITY - CITY_ACTIVE)) != 1) {
            continue;
        }
        int32_t y0 = y - 0xE;
        S32(st, G_TEXT_X) = 0;
        text(st, G_STR_DBG_POP_IS, 8, y0, WHITE);
        num(st, S32(st, rec - (CITY_HOUSE_CAPACITY - CITY_POPULATION)), G_NUMBER_SUFFIX, cursor(st) + 8, y0, WHITE);
        text(st, G_STR_DBG_TOT_CAP, cursor(st) + 8, y0, WHITE);
        num(st, S32(st, rec), G_NUMBER_SUFFIX, cursor(st) + 8, y0, WHITE);
        text(st, G_STR_DBG_SPARE_CAP, cursor(st) + 8, y0, WHITE);
        num(st, S32(st, rec + (CITY_HOUSE_ROOM_LEFT - CITY_HOUSE_CAPACITY)), G_NUMBER_SUFFIX, cursor(st) + 8, y0, WHITE);
        S32(st, G_TEXT_X) = 0;
        text(st, G_STR_DBG_BIRTHS, 8, y, WHITE);
        num(st, S32(st, rec + (CITY_BIRTHS - CITY_HOUSE_CAPACITY)), G_NUMBER_SUFFIX, cursor(st) + 8, y, WHITE);
        text(st, G_STR_DBG_DEATHS, cursor(st) + 8, y, WHITE);
        num(st, S32(st, rec + (CITY_DEATHS - CITY_HOUSE_CAPACITY)), G_NUMBER_SUFFIX, cursor(st) + 8, y, WHITE);
        int32_t housed = GAME_CALL(st, FN_CITY_SUM_BUILDING_1C, S32(st, G_PLAYER));
        if (housed != S32(st, rec - (CITY_HOUSE_CAPACITY - CITY_POPULATION))) {
            colour = RED;
        }
        text(st, G_STR_DBG_IN_HOUSES, cursor(st) + 8, y, colour);
        num(st, housed, G_NUMBER_SUFFIX, cursor(st) + 8, y, colour);
        int32_t y1 = y + 0xE;
        S32(st, G_TEXT_X) = 0;
        text(st, G_STR_DBG_NOF_DOCKS, 8, y1, WHITE);
        num(st, S32(st, G_DOCK_COUNT), G_STR_DBG_SPACES, cursor(st) + 8, y1, WHITE);
        text(st, G_STR_DBG_DOCKS_LIST, cursor(st) + 8, y1, WHITE);
        for (int32_t k = 0; k < 10; k++) {
            int32_t i = S32(st, G_PLAYER) * 0x24EC + k;
            num(st, S16(st, CITY_BASE + CITY_DOCKS + 2u * (uint32_t) i), G_STR_DBG_COMMA, cursor(st) + 4, y1, WHITE);
        }
        uint32_t player = (uint32_t) S32(st, G_PLAYER) * 0x49D8u;
        S32(st, G_TEXT_X) = 0;
        text(st, G_STR_DBG_WINE_SOURCES, 8, y + 0x1C, WHITE);
        num(st, S32(st, CITY_BASE + CITY_WINE_SOURCES + player), G_STR_DBG_SPACES, cursor(st) + 8, y + 0x1C, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_VIEW_ORIENTATION), G_STR_DBG_MAP_ROTATION, 8, y + 0x2A, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_OVERLAY), G_STR_DBG_MAP_STYLE, 8, y + 0x38, WHITE);
        S32(st, G_TEXT_X) = 0;
        player = (uint32_t) S32(st, G_PLAYER) * 0x49D8u;
        num(st, S32(st, CITY_BASE + CITY_NATIVES_RESTLESS + player), G_STR_DBG_NATIVES_RESTLESS, 8, y + 0x46, WHITE);
        player = (uint32_t) S32(st, G_PLAYER) * 0x49D8u;
        num(st, S32(st, CITY_BASE + CITY_MISSION_IN_PLACE + player), G_STR_DBG_MISSION_IN_PLACE, cursor(st) + 0x6C, y + 0x46, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_WAS_IN_INT), G_STR_DBG_WAS_IN_INT, 8, y + 0x54, WHITE);
        num(st, S32(st, G_WAS_IN_SET), G_STR_DBG_WAS_IN_SET, cursor(st) + 0x6C, y + 0x54, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_UE951D8), G_STR_DBG_ALERT_MESSAGES, 8, y + 0x62, WHITE);
        num(st, S32(st, G_NEXT_MESSAGE_INDEX), G_STR_DBG_LAST_ALERT, cursor(st) + 0x6C, y + 0x62, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_UF535BC), G_STR_DBG_AUTO_REMOVED, 8, y + 0x70, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_TEST_VALUE1A), G_STR_DBG_TEST_VALUE1A, 8, y + 0x7E, WHITE);
        num(st, S32(st, G_TEST_VALUE2A), G_STR_DBG_TEST_VALUE2A, cursor(st) + 0x6C, y + 0x7E, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_TEST_VALUE3A), G_STR_DBG_TEST_VALUE3A, 8, y + 0x8C, WHITE);
        num(st, S32(st, G_TEST_VALUE4A), G_STR_DBG_TEST_VALUE4A, cursor(st) + 0x6C, y + 0x8C, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_MOUSE_X), G_STR_DBG_MOUSE_X, 8, y + 0x9A, WHITE);
        num(st, S32(st, G_MOUSE_Y), G_STR_DBG_MOUSE_Y, cursor(st) + 0x6C, y + 0x9A, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_MOUSE_MOVED), G_STR_DBG_MOUSE_MOVEMENT, 8, y + 0xA8, WHITE);
        num(st, S32(st, G_SIDEBAR_X), G_STR_DBG_SIDE_PANEL_X1, cursor(st) + 0x6C, y + 0xA8, WHITE);
        S32(st, G_TEXT_X) = 0;
        num(st, S32(st, G_CONSTRUCTION_DRAG), G_STR_DBG_BUILDING_IN_PROGRESS, 8, y + 0xB6, WHITE);
    }
}

void debug_evolve_panel(game_state *st)
{
    /* @bin 0x00534DF0 with the switch 0x0078D7DC = 1: a panel (0x00536DC0) and the evolve counters
     * (tick, week, month, year, last building, ever, connections - 1, total connections; the evolve
     * error, routings and barbarians in red when the error 0x0078D7E0 is not 0) */
    if (S32(st, G_DEBUG_EVOLVE) != 1) {
        return;
    }
    int32_t y = S32(st, G_LAYOUT_DY) * 2 + 0xDC;
    GAME_CALL(st, FN_PLAY_BUTTON_PANEL, 6, 0, y, 0xC, 0xF, 1);
    title(st, G_STR_DBG_EVOLVE_INFO, y + 6, G_FONT_LARGE, WHITE);
    num(st, S32(st, G_TICK), G_STR_DBG_TICK, 8, y + 0x1A, GREEN);
    num(st, S32(st, G_TICK_DAY), G_STR_DBG_WEEK, 0x6E, y + 0x1A, GREEN);
    num(st, S32(st, G_MONTH), G_STR_DBG_MONTH, 8, y + 0x28, GREEN);
    num(st, S32(st, G_YEAR), G_STR_DBG_YEAR, 0x6E, y + 0x28, GREEN);
    num(st, S32(st, G_BUILDING_HIGHEST), G_STR_DBG_LAST_BUILDING, 8, y + 0x36, GREEN);
    num(st, S32(st, G_BUILDING_HIGHEST_EVER), G_STR_DBG_EVER, 0x6E, y + 0x36, GREEN);
    num(st, S32(st, G_CONNECTIONS) - 1, G_STR_DBG_NO_OF_CONNECTS, 8, y + 0x44, GREEN);
    num(st, S32(st, G_TOTAL_CONNECTIONS), G_STR_DBG_TOTAL_CONNECTIONS, 8, y + 0x60, GREEN);
    int32_t error = S32(st, G_U78D7E0);
    int32_t colour = error != 0 ? RED : GREEN;
    num(st, error, G_STR_DBG_EVOLVE_ERROR, 8, y + 0x6E, colour);
    num(st, S32(st, G_ROUTE_SEARCHES), G_STR_DBG_ROUTINGS, 8, y + 0x7C, colour);
    num(st, S32(st, G_ROUTE_SEARCHES_ENEMY), G_STR_DBG_BARB, 8, y + 0x8A, colour);
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
    if (S32(st, G_DEBUG_TILE) != 1) {
        return;
    }
    int32_t y = S32(st, G_LAYOUT_DY) * 2 + 0x9C;
    GAME_CALL(st, FN_PLAY_BUTTON_PANEL, 6, 0, y, 0xC, 0x13, 1);
    int32_t tile = GAME_CALL(st, FN_FRAME_SCREEN_TO_TILE, 0);
    S32(st, G_MOUSE_OFFSET) = tile;
    int32_t id = tile == 0 ? 0 : S16(st, G_GRID_BUILDING + 2u * (uint32_t) tile);
    title(st, G_STR_DBG_BUILDING_INFO, y + 6, G_FONT_LARGE, WHITE);
    if (id == 0) {
        title(st, G_STR_DBG_NOT_OVER_BUILDING, y + 0x78, G_FONT_SMALL, GREEN);
        num(st, S32(st, G_MOUSE_OFFSET), G_STR_DBG_OVER_PTR, 8, y + 0x8C, WHITE);
        GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x10, S32(st, G_DEBUG_GRID), 8, y + 0xA0, 0xC0, S32(st, G_FONT_SMALL), WHITE,
                  S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        num(st, S32(st, G_UNDO_READY), G_STR_DBG_UNDO_READY, 8, y + 0xB4, WHITE);
        num(st, S32(st, G_UNDO_POSSIBLE), G_STR_DBG_UNDO_VALID, 8, y + 0xC8, WHITE);
        num(st, S32(st, G_UNDO_COUNT), G_STR_DBG_UNDO_BUILDINGS, 8, y + 0xDC, WHITE);
        int32_t alive = S16(st, G_UNDO_IDS);
        num(st, U8(st, BUILDING_BASE + BLD_STATE + (uint32_t) (alive * 0x108)), G_STR_DBG_ALIVE, 8, y + 0xF0, WHITE);
        num(st, (int32_t) (U32(st, G_UNDO_RECORDS) & 0xFF), G_STR_DBG_ALIVE, 0x64, y + 0xF0, WHITE);
        S32(st, G_TEXT_X) = 0;
        for (uint32_t o = G_DIRECTION_OFFSETS; o < G_DIRECTION_STEPS; o += 4) {
            int32_t x = cursor(st) + 8;
            int32_t v = (int8_t) U8(st, G_GRID_ROUTING_WATER + (uint32_t) S32(st, o) + (uint32_t) S32(st, G_MOUSE_OFFSET));
            num(st, v, G_STR_COMMA, x, y + 0x104, GREEN);
        }
        num(st, U8(st, G_REDRAW_SIDEBAR), G_STR_DBG_REDRAW, 0x64, y + 0x118, WHITE);
        return;
    }
    uint32_t b = (uint32_t) id * 0x108u;
    num(st, id, G_STR_DBG_BUILDING_NO, 8, y + 0x18, U8(st, B(BUILDING_BASE + BLD_STATE)) != 1 ? RED : GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_TYPE)), G_STR_DBG_TYPE, 8, y + 0x3C, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_SUBTYPE)), G_STR_DBG_SUB_TYPE, 0x64, y + 0x3C, GREEN);
    int16_t type = S16(st, B(BUILDING_BASE + BLD_TYPE));
    if (type == 0x5A) {
        S32(st, G_TEXT_X) = 0;
        text(st, G_STR_DBG_AQUA_LINKS, 8, y + 0x48, GREEN);
        for (uint32_t k = 0; k < 4; k++) {
            num(st, S16(st, B(BUILDING_BASE + BLD_AQUA_LINKS) + 2 * k), G_NUMBER_SUFFIX, cursor(st) + 8, y + 0x48, GREEN);
        }
    } else if (type == 0x47 || type == 0xBF) {
        S32(st, G_TEXT_X) = 0;
        text(st, G_STR_DBG_FOOD, 8, y + 0x48, GREEN);
        for (uint32_t k = 0; k < 7; k++) {
            num(st, S16(st, B(BUILDING_BASE + BLD_INVENTORY) + 2 * k), G_NUMBER_SUFFIX, cursor(st) + 8, y + 0x48, GREEN);
            S32(st, G_TEXT_X) = S32(st, G_TEXT_X) - 4;
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
            S32(st, G_TEXT_X) = 0;
            text(st, G_STR_DBG_RAW_MATERIALS, 8, y + 0x48, GREEN);
            num(st, S16(st, B(BUILDING_BASE + BLD_STORED)), G_NUMBER_SUFFIX, cursor(st) + 8, y + 0x48, GREEN);
        }
    }
    num(st, S16(st, B(BUILDING_BASE + BLD_POPULATION)), G_STR_DBG_POP, 8, y + 0x54, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_STORED)), G_STR_DBG_GOODS2, 0x64, y + 0x54, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_SERVICE)), G_STR_DBG_ACC, 8, y + 0x60, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_LABOUR_PERCENT)), G_STR_DBG_REL, 0x64, y + 0x60, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_IMMIGRANT)), G_STR_DBG_IMMIGRANT, 8, y + 0x6C, GREEN);
    if (S16(st, B(BUILDING_BASE + BLD_WALKER2)) != 0) {
        num(st, S16(st, B(BUILDING_BASE + BLD_WALKER2)), G_STR_DBG_SUB_AGENT, 0x64, y + 0x6C, GREEN);
    } else if (S16(st, B(BUILDING_BASE + BLD_WALKER)) != 0) {
        num(st, S16(st, B(BUILDING_BASE + BLD_WALKER)), G_STR_DBG_AGENT, 0x64, y + 0x6C, GREEN);
    }
    num(st, S16(st, B(BUILDING_BASE + BLD_DISTANCE)), G_STR_DBG_ROME, 8, y + 0x78, GREEN);
    num(st, U8(st, B(BUILDING_BASE + BLD_ROAD_NETWORK)), G_STR_DBG_ROADNET, 0x64, y + 0x78, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_ROOM_LEFT)), G_STR_DBG_SPARE_ROOM, 8, y + 0x84, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_COLLAPSE_RISK)), G_STR_DBG_DAMAGE, 0x64, y + 0x84, GREEN);
    if (U8(st, B(BUILDING_BASE + BLD_WATER_SUPPLY)) != 0) {
        num(st, U8(st, B(BUILDING_BASE + BLD_WATER_SUPPLY)), G_STR_DBG_WATER_SUP, 8, y + 0x90, GREEN);
    } else if (U8(st, B(BUILDING_BASE + BLD_WELL_ACCESS)) != 0) {
        num(st, U8(st, B(BUILDING_BASE + BLD_WELL_ACCESS)), G_STR_DBG_WELL_SUP, 8, y + 0x90, GREEN);
    } else {
        num(st, 0, G_STR_DBG_NO_WATER, 8, y + 0x90, GREEN);
    }
    num(st, U8(st, B(BUILDING_BASE + BLD_WATER_DEP)), G_STR_DBG_WATER_DEP, 0x64, y + 0x90, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_DAYS_CUT_OFF)), G_STR_DBG_NO_CONTACT, 8, y + 0x9C, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_WORKERS)), G_STR_DBG_EMPLOYEES, 0x64, y + 0xA8, GREEN);
    num(st, U8(st, B(BUILDING_BASE + BLD_TAX_COVERAGE)), G_STR_DBG_TAX_CVR, 8, y + 0xB4, GREEN);
    num(st, S16(st, B(BUILDING_BASE + BLD_MAX_POPULATION)), G_STR_DBG_MAX_POP, 0x64, y + 0xB4, GREEN);
    if (U8(st, B(BUILDING_BASE + BLD_HOUSE_SIZE)) != 0) {
        int32_t y1 = y + 0xC0;
        S32(st, G_TEXT_X) = 0;
        num(st, U8(st, B(BUILDING_BASE + BLD_ACCESS_JUGGLER)), G_STR_DBG_TH, 8, y1, GREEN);
        S32(st, G_TEXT_X) = 0;
        num(st, U8(st, B(BUILDING_BASE + BLD_ACCESS_MUSICIAN)), G_STR_COMMA, 0x32, y1, GREEN);
        S32(st, G_TEXT_X) = 0;
        num(st, U8(st, B(BUILDING_BASE + BLD_ACCESS_DANCER)), G_NUMBER_SUFFIX, 0x64, y1, GREEN);
        S32(st, G_TEXT_X) = 0;
        num(st, U8(st, B(BUILDING_BASE + BLD_ACCESS_SENET)), G_STR_DBG_HI, 0x96, y1, GREEN);
        S32(st, G_TEXT_X) = 0;
        num(st, U8(st, B(BUILDING_BASE + BLD_ACCESS_ZOOKEEPER)), G_STR_DBG_HI, 0xC8, y1, GREEN);
        int32_t y2 = y + 0xF0;
        num(st, (int8_t) U8(st, B(BUILDING_BASE + BLD_ENTERTAINMENT)), G_STR_DBG_EN, 8, y2, GREEN);
        num(st, (int8_t) U8(st, B(BUILDING_BASE + BLD_EDUCATION)), G_STR_DBG_ED, 0x32, y2, GREEN);
        num(st, (int8_t) U8(st, B(BUILDING_BASE + BLD_HEALTH)), G_STR_DBG_HE, 0x64, y2, GREEN);
        num(st, (int8_t) U8(st, B(BUILDING_BASE + BLD_RELIGION)), G_STR_DBG_RE, 0x96, y2, GREEN);
        int32_t y3 = y + 0xFC;
        static const uint32_t needs[4] = {G_HOUSE_NEED_ENTERTAINMENT, G_HOUSE_NEED_RELIGION, G_HOUSE_NEED_EDUCATION, G_HOUSE_NEED_SERVICES};
        static const int32_t xs[4] = {8, 0x32, 0x64, 0x96};
        for (int k = 0; k < 4; k++) {
            int32_t level = S16(st, B(BUILDING_BASE + BLD_SUBTYPE));
            num_p(st, S32(st, needs[k] + (uint32_t) (level * 0x58)), 0x28, G_STR_CLOSE_PAREN, xs[k], y3, GREEN);
        }
    } else {
        int32_t y1 = y + 0xC0;
        num(st, U8(st, B(BUILDING_BASE + BLD_SHOWS)), G_STR_DBG_SHOWS, 8, y1, GREEN);
        S32(st, G_TEXT_X) = 0;
        num(st, U8(st, B(BUILDING_BASE + BLD_UA7)), G_STR_DBG_COMMA3, 0x6C, y1, GREEN);
        num(st, U8(st, B(BUILDING_BASE + BLD_UA8)), G_STR_DBG_SPACES2, cursor(st) + 0x6C, y1, GREEN);
        num(st, U8(st, B(BUILDING_BASE + BLD_UA9)), G_STR_DBG_SPACES2, cursor(st) + 0x6C, y1, GREEN);
    }
    num(st, S32(st, G_MOUSE_OFFSET), G_STR_DBG_OVER_PTR, 8, y + 0xCC, WHITE);
    static const uint32_t house_goods[5] = {BUILDING_BASE + BLD_HOUSE_INVENTORY, BUILDING_BASE + BLD_HOUSE_INVENTORY + 2 * 9,
                                              BUILDING_BASE + BLD_HOUSE_INVENTORY + 2 * 11,
                                              BUILDING_BASE + BLD_HOUSE_INVENTORY + 2 * 10,
                                              BUILDING_BASE + BLD_HOUSE_INVENTORY + 2 * 8};
    static const uint32_t store_goods[5] = {BUILDING_BASE + BLD_INVENTORY, BUILDING_BASE + BLD_INVENTORY + 2 * 9,
                                              BUILDING_BASE + BLD_INVENTORY + 2 * 11,
                                              BUILDING_BASE + BLD_INVENTORY + 2 * 10,
                                              BUILDING_BASE + BLD_INVENTORY + 2 * 8};
    const uint32_t *goods;
    if (U8(st, B(BUILDING_BASE + BLD_HOUSE_SIZE)) != 0) {
        goods = house_goods;
    } else if (S16(st, B(BUILDING_BASE + BLD_TYPE)) == 0x46 || S16(st, B(BUILDING_BASE + BLD_TYPE)) == 0xBE) {
        goods = store_goods;
    } else {
        return;
    }
    int32_t y4 = y + 0xDC;
    S32(st, G_TEXT_X) = 0;
    num(st, S16(st, B(goods[0])), G_STR_DBG_COMMA2, 8, y4, GREEN);
    for (int k = 1; k < 5; k++) {
        num(st, S16(st, B(goods[k])), k == 4 ? G_STR_DBG_GOODS : G_STR_DBG_COMMA2, cursor(st) + 8, y4, GREEN);
    }
}

void debug_figure_panel(game_state *st)
{
    /* @bin 0x00536120 with the switch 0x0078D7D4 = 1: a panel; the watched figure 0x00F535CC becomes
     * the one on the tile under the mouse (i16 grid 0x00B350DC; kept when the tile has none, 0 off the
     * map). Without one: the figure counts and the free route slots (0x004B3320). With one: its
     * fields (record 0x184 bytes at 0x00BCD5F0; the number in red when the figure is not in use) and
     * the dockers (type 0x26 in use) counted over the figures */
    if (S32(st, G_DEBUG_FIGURE) != 1) {
        return;
    }
    int32_t y = S32(st, G_LAYOUT_DY) * 2 + 0xDC;
    GAME_CALL(st, FN_PLAY_BUTTON_PANEL, 6, 0, y, 0xC, 0xF, 1);
    int32_t tile = GAME_CALL(st, FN_FRAME_SCREEN_TO_TILE, 0);
    S32(st, G_MOUSE_OFFSET) = tile;
    if (tile == 0) {
        S32(st, G_DEBUG_WATCHED_FIGURE) = 0;
    } else if (S16(st, G_FIGURE_GRID + 2u * (uint32_t) tile) != 0) {
        S32(st, G_DEBUG_WATCHED_FIGURE) = S16(st, G_FIGURE_GRID + 2u * (uint32_t) tile);
    }
    title(st, G_STR_DBG_FIGURE_INFO, y + 6, G_FONT_LARGE, WHITE);
    if (S32(st, G_DEBUG_WATCHED_FIGURE) == 0) {
        title(st, G_STR_DBG_NOT_WATCHING, y + 0x18, G_FONT_LARGE, WHITE);
        num(st, S32(st, G_FIGURES_UPDATED), G_STR_DBG_FIGS, 8, y + 0x30, GREEN);
        num(st, S32(st, G_FIGURE_HIGHEST), G_STR_DBG_EVER, 0x6E, y + 0x30, GREEN);
        int32_t slots = GAME_CALL0(st, 0x004B3320u);
        num(st, slots, G_STR_DBG_FREE_ROUTE_SLOTS, 8, y + 0x3C, GREEN);
        return;
    }
#define F(off) ((off) + (uint32_t) S32(st, G_DEBUG_WATCHED_FIGURE) * 0x184u)
    int32_t fig = S32(st, G_DEBUG_WATCHED_FIGURE);
    num(st, fig, G_STR_DBG_FIGURE_NO, 8, y + 0x18, U8(st, F(FIGURE_BASE + FIG_STATE)) != 1 ? RED : GREEN);
    num(st, U8(st, F(FIGURE_BASE + FIG_OWNER)), G_STR_DBG_RULER, 0x6E, y + 0x18, GREEN);
    S32(st, G_TEXT_X) = 0;
    num(st, S16(st, F(FIGURE_BASE + FIG_CC_X)), G_STR_COMMA, 8, y + 0x30, GREEN);
    num(st, S16(st, F(FIGURE_BASE + FIG_CC_Y)), G_STR_DBG_XY, cursor(st) + 8, y + 0x30, GREEN);
    S32(st, G_TEXT_X) = 0;
    num(st, S16(st, F(FIGURE_BASE + FIG_X)), G_STR_COMMA, 0x64, y + 0x30, GREEN);
    num(st, S16(st, F(FIGURE_BASE + FIG_Y)), G_STR_DBG_MAP_XY, cursor(st) + 0x64, y + 0x30, GREEN);
    S32(st, G_TEXT_X) = 0;
    num(st, U8(st, F(FIGURE_BASE + FIG_CC_X)) & 0xF, G_STR_COMMA, 8, y + 0x48, GREEN);
    num(st, U8(st, F(FIGURE_BASE + FIG_CC_Y)) & 0xF, G_STR_DBG_XY_BITS, cursor(st) + 8, y + 0x48, GREEN);
    S32(st, G_TEXT_X) = 0;
    num(st, S16(st, F(FIGURE_BASE + FIG_HOME)), G_STR_DBG_AGENT_FOR, 8, y + 0x54, GREEN);
    num(st, S16(st, F(FIGURE_BASE + FIG_TARGET_HOUSE)), G_STR_DBG_IMMIGRANT_TO, cursor(st) + 8, y + 0x54, GREEN);
    S32(st, G_TEXT_X) = 0;
    num(st, U8(st, F(FIGURE_BASE + FIG_MIGRANT_PEOPLE)), G_STR_DBG_PEOPLE, 8, y + 0x60, GREEN);
    num(st, U8(st, F(FIGURE_BASE + FIG_INVISIBLE)), G_STR_DBG_GHOST, cursor(st) + 8, y + 0x60, GREEN);
#undef F
    int32_t dockers = 0;
    for (uint32_t a = FIGURE_ADDR(1) + FIG_TYPE; a < G_FIGURES_END; a += 0x184u) {
        if (U8(st, a + 4) == 1 && U8(st, a) == 0x26) {
            dockers++;
        }
    }
    S32(st, G_TEXT_X) = 0;
    num(st, dockers, G_STR_DBG_DOCKERS, 8, y + 0x6C, GREEN);
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
