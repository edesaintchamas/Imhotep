/* Imhotep - the three per-window dispatchers: full redraw, per-frame draw, input (roadmap 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window ids and their functions: docs/presentation.md 3.2. The windows' functions are called
 * through GAME_CALL (ported ones dispatch to their port); the input blocks that the original
 * inlines into 0x004D8310 are ported here, whichever window they belong to (window 0x15's, the
 * empire editor's routes, in empire_editor.c). */
#include "ui.h"

#include "draw/draw.h"
#include "present_layout.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define DX() S32(st, G_LAYOUT_DX)
#define DY() S32(st, G_LAYOUT_DY)
#define AX() S32(st, G_ADVISOR_X)
#define AY() S32(st, G_ADVISOR_Y)
#define SW() S32(st, G_SCREEN_W)
#define SH() S32(st, G_SCREEN_H)

/* ---- rectangle tables ---- */

int32_t rect_hit(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    /* 0x00426450 (package msgempire's port): the rectangle under the mouse, 1-based, else 0 */
    return GAME_CALL(st, FN_BUTTONS_HOVERED, x, y, (int32_t) table, count);
}

void rect_layout_apply(game_state *st, uint32_t table, uint32_t list)
{
    /* @bin 0x004D66F0 the layout registry's list of rectangle overrides (nodes: +0 index (1-based),
     * +8 x, +0x0C y, +0x10 w, +0x14 h, +0x18 next) written into the table's entries */
    for (uint32_t n = list; n != 0; n = P32(st, n + 0x18)) {
        uint32_t e = table + (uint32_t) P32(st, n) * 0x1Cu - 0x1Cu;
        P16(st, e) = (int16_t) P32(st, n + 8);
        P16(st, e + 2) = (int16_t) P32(st, n + 0x0C);
        P16(st, e + 4) = (int16_t) ((uint16_t) P32(st, n + 0x10) + (uint16_t) P32(st, n + 8));
        P16(st, e + 6) = (int16_t) ((uint16_t) P32(st, n + 0x14) + (uint16_t) P32(st, n + 0x0C));
    }
}

int32_t rect_highlight(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t hit)
{
    /* @bin 0x004D6740 the first time a window's table is used (flag +0x10 of its registry entry
     * 0x00E94614 + 16 key), the registry's overrides are applied to it and 0 returned; then the hit
     * rectangle `hit` (from 0x00426450) is returned, and in layout mode 2 (0x00E94D98) it is drawn
     * framed over the panel, kept in 0x00E945F8..0x00E94604 / 0x00E94DA0 */
    (void) count;
    uint32_t key = (uint32_t) layout_window_key(st, U8(st, G_WINDOW));
    if (S32(st, G_LAYOUT_RECTS_DONE + key * 16u) == 0) {
        key = (uint32_t) layout_window_key(st, U8(st, G_WINDOW));
        rect_layout_apply(st, table, U32(st, G_LAYOUT_RECT_LISTS + key * 16u));
        key = (uint32_t) layout_window_key(st, U8(st, G_WINDOW));
        S32(st, G_LAYOUT_RECTS_DONE + key * 16u) = 1;
        return 0;
    }
    if (hit == 0) {
        return 0;
    }
    uint32_t e = table + (uint32_t) hit * 0x1Cu - 0x1Cu;
    if (S32(st, G_LAYOUT_MODE) == 2) {
        panel_draw(st, 0, 0, 6, 6);
        S32(st, G_LAYOUT_HIT_ENTRY) = (int32_t) e;
        S32(st, G_LAYOUT_HIT_INDEX) = hit;
        S32(st, G_LAYOUT_PANEL_X) = P16(st, e);
        S32(st, G_LAYOUT_PANEL_Y) = P16(st, e + 2);
        S32(st, G_LAYOUT_PANEL_W) = P16(st, e + 4) - P16(st, e);
        S32(st, G_LAYOUT_PANEL_OX) = hit;
        S32(st, G_LAYOUT_PANEL_H) = P16(st, e + 6) - P16(st, e + 2);
        S32(st, G_LAYOUT_PANEL_OY) = 0;
        int32_t y0 = P16(st, e + 2), x0 = P16(st, e);
        panel_draw_frame(st, 0, x0 + x, y0 + y, P16(st, e + 4) - x0, P16(st, e + 6) - y0, 0xFFFF);
    }
    return hit;
}

static int32_t rect_input(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, uint32_t slot)
{
    /* the inlined sequence of 0x004D8310: hit 0x00426450, highlight 0x004D6740 into `slot` (slot 0:
     * the result is not stored, as for advisor page 11, 0x004DC43A), click 0x004261D0 (its result) */
    int32_t hit = rect_hit(st, x, y, table, count);
    int32_t lit = rect_highlight(st, x, y, table, count, hit);
    if (slot != 0) {
        S32(st, slot) = lit;
    }
    return GAME_CALL(st, FN_BUTTONS_RECT_CLICK, x, y, (int32_t) table, count);
}

static int32_t click(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    return GAME_CALL(st, FN_BUTTONS_CLICK, x, y, (int32_t) table, count); /* buttons_click (sidebar) */
}

static void hover(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t base)
{
    GAME_CALL(st, FN_BUTTONS_HOVER, x, y, (int32_t) table, count, base); /* buttons_hover (sidebar) */
}

static int32_t hover_click(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t base)
{
    hover(st, x, y, table, count, base);
    return click(st, x, y, table, count);
}

static int32_t editor_buttons(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    return GAME_CALL(st, FN_BUTTONS_EDITOR_CLICK, x, y, (int32_t) table, count); /* buttons_editor_click (sidebar) */
}

static void editor_hover(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t base)
{
    GAME_CALL(st, FN_EDITOR_BUTTONS_HOVER, x, y, (int32_t) table, count, base);
}

static void buttons_draw_at(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    GAME_CALL(st, FN_BUTTONS_DRAW, x, y, (int32_t) table, count); /* buttons_draw (sidebar) */
}

static void editor_buttons_draw(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    GAME_CALL(st, FN_EDITOR_BUTTONS_DRAW, x, y, (int32_t) table, count);
}

static void close_to(game_state *st, uint8_t window)
{
    U8(st, G_WINDOW) = window;
    U8(st, G_REDRAW) = 1;
}

/* ---- full redraw 0x004D71C0 ---- */

void ui_redraw(game_state *st, int32_t force)
{
    /* @bin 0x004D71C0 the current window's full redraw, when forced or the redraw flag is set; the
     * window is 0x00D3A6C4 instead while 0x00D3A6E4 is set (help over a window); then the flag is
     * cleared. Switch tables 0x004D7574 (bytes) / 0x004D74B8 */
    if (force != 0) {
        U8(st, G_REDRAW) = 1;
    } else if (U8(st, G_REDRAW) == 0) {
        return;
    }
    uint32_t w = U32(st, G_TIP_ID) != 0 ? U32(st, G_TIP_WINDOW) : U8(st, G_WINDOW);
    if (w > 0x57) {
        U8(st, G_REDRAW) = 0;
        return;
    }
    switch (w) {
    case 0x00: GAME_CALL(st, FN_MAIN_MENU_DRAW, 0); break;
    case 0x01: case 0x03: case 0x16: case 0x18: case 0x19: case 0x1B: case 0x1C: case 0x1E: case 0x1F:
    case 0x20: case 0x25: case 0x27: case 0x46: case 0x49: case 0x4B:
        GAME_CALL0(st, FN_SIDEBAR_DRAW);
        break;
    case 0x04: GAME_CALL0(st, FN_MISSION_PICTURE_DRAW); break;
    case 0x05: GAME_CALL0(st, FN_MESSAGE_WINDOW_REDRAW); break;
    case 0x06: GAME_CALL0(st, FN_BUILD_MENU_REDRAW); break;
    case 0x07: GAME_CALL0(st, FN_ADVISORS_DRAW); break;
    case 0x08: GAME_CALL0(st, FN_OVERLAYS_MENU_REDRAW); break;
    case 0x09: GAME_CALL0(st, FN_SIDEBAR_DRAW); GAME_CALL0(st, FN_INFO_DRAW); break;
    case 0x0D: GAME_CALL0(st, FN_FILE_DIALOG_DRAW); break;
    case 0x0F:
        GAME_CALL(st, FN_POPUP_LIST_DRAW, S32(st, G_LIST_A1), S32(st, G_LIST_A2), S32(st, G_LIST_PER_COLUMN));
        break;
    case 0x10: GAME_CALL0(st, FN_MAP_DIALOG_DRAW); break;
    case 0x11: GAME_CALL0(st, FN_SIDEBAR_DRAW); GAME_CALL0(st, FN_MSGLIST_REDRAW); break;
    case 0x14: GAME_CALL0(st, FN_EMPIRE_DRAW); break;
    case 0x15: GAME_CALL0(st, FN_EDITOR_EMPIRE_DRAW); break;
    case 0x17: GAME_CALL0(st, FN_PERIOD_SCREEN_DRAW); break;
    case 0x23: GAME_CALL0(st, FN_TOOLTIP_DRAW_OVERLAY); break;
    case 0x26: GAME_CALL0(st, 0x00525B90u); break;
    case 0x28:
        GAME_CALL0(st, FN_ADVISORS_DRAW_HALL);
        GAME_CALL0(st, 0x00559E10u);
        GAME_CALL0(st, 0x0055A0E0u);
        GAME_CALL(st, 0x0055AD30u, 1);
        break;
    case 0x2A:
        GAME_CALL0(st, FN_ADVISORS_DRAW_HALL);
        GAME_CALL0(st, 0x00521080u);
        GAME_CALL0(st, 0x005215D0u);
        GAME_CALL0(st, 0x00521800u);
        break;
    case 0x2B: GAME_CALL0(st, FN_ADVISORS_DRAW_HALL); GAME_CALL0(st, 0x00529320u); break;
    case 0x2C: GAME_CALL0(st, FN_ADVISORS_DRAW_HALL); GAME_CALL0(st, 0x00524E20u); break;
    case 0x2D: GAME_CALL0(st, FN_ADVISORS_DRAW_HALL); GAME_CALL0(st, 0x005250F0u); break;
    case 0x2E: GAME_CALL0(st, FN_ADVISORS_DRAW_HALL); GAME_CALL0(st, 0x005255A0u); break;
    case 0x2F: GAME_CALL0(st, FN_FAMILY_LIST_DRAW); break;
    case 0x30: GAME_CALL0(st, FN_FAMILY_MENU_DRAW); break;
    case 0x33: GAME_CALL0(st, FN_MISSION_END_DRAW); break;
    case 0x34: GAME_CALL0(st, FN_MISSION_CHOICE_DRAW); break;
    case 0x35: case 0x3B: GAME_CALL0(st, FN_MISSION_BRIEFING_DRAW); break;
    case 0x36: GAME_CALL0(st, FN_NAME_ENTRY_DRAW); break;
    case 0x37: GAME_CALL0(st, FN_PICTURE_37_DRAW); break;
    case 0x38: GAME_CALL0(st, FN_PICTURE_38_DRAW); break;
    case 0x39: GAME_CALL0(st, FN_PICTURE_39_DRAW); break;
    case 0x3A: GAME_CALL0(st, FN_PICTURE_3A_DRAW); break;
    case 0x3C: GAME_CALL0(st, FN_TITLE_SCREEN_DRAW); break;
    case 0x3D: GAME_CALL0(st, FN_SCENARIO_LIST_DRAW); break;
    case 0x3E: GAME_CALL0(st, FN_RESOLUTION_CHECK_DRAW); break;
    case 0x3F: GAME_CALL0(st, FN_POPUP_DRAW); break;
    case 0x43: GAME_CALL0(st, FN_WINDOW43_DRAW); break;
    case 0x4C: GAME_CALL0(st, FN_ADVISORS_DRAW_HALL); GAME_CALL0(st, FN_ADVISORS_DRAW_GRAVE_GOODS); break;
    case 0x4D: GAME_CALL0(st, 0x00524E20u); break; /* @bin 0x004D735A into case 0x2C, past 0x00520F40 */
    case 0x4F: GAME_CALL0(st, 0x00538050u); break;
    case 0x50: GAME_CALL0(st, FN_ADVISORS_DRAW_RATING_INFO); break;
    case 0x51: GAME_CALL0(st, FN_CHOICE_WINDOW_DRAW); break;
    case 0x52: GAME_CALL0(st, 0x00516080u); break;
    case 0x53: GAME_CALL0(st, FN_HIGHSCORE_DRAW); break;
    case 0x57: GAME_CALL0(st, FN_LOADING_LAST); break;
    default: break;
    }
    U8(st, G_REDRAW) = 0;
}

/* ---- per-frame draw 0x004D75D0 ---- */

static void advisor_page_frame(game_state *st)
{
    /* @bin 0x004D7C28 window 7: its buttons, then the page's own per-frame part (0x00605A44) */
    U8(st, G_REDRAW_SIDEBAR) = 1;
    GAME_CALL0(st, FN_ADVISORS_MIN_LINES);
    buttons_draw_at(st, AX(), (S32(st, G_PANEL_H) - 2) * 16 + AY(), G_ADVISOR_HELP_BUTTON, 1);
    switch (S32(st, G_PAGE)) {
    case 1:
        editor_buttons_draw(st, AX(), AY(), G_TAX_ARROWS, 2);
        GAME_CALL0(st, 0x005215D0u);
        break;
    case 2: GAME_CALL0(st, FN_ADVISOR2_FRAME); break;
    case 3: GAME_CALL0(st, FN_ADVISOR3_FRAME); break;
    case 4: GAME_CALL0(st, FN_ADVISOR4_FRAME); break;
    case 5: GAME_CALL0(st, FN_ADVISOR5_FRAME); break;
    case 6: GAME_CALL0(st, 0x0055A0E0u); break;
    case 7: GAME_CALL0(st, FN_ADVISOR7_FRAME); break;
    case 0x0B: GAME_CALL0(st, 0x005292B0u); break;
    case 0x0C: editor_buttons_draw(st, AX(), AY(), G_TAX_BUTTONS, 2); break;
    case 0x0E: GAME_CALL0(st, FN_ADVISOR14_FRAME); break;
    default: break;
    }
}

static void info_frame(game_state *st)
{
    /* @bin 0x004D77EC window 9 (building / figure info): its buttons 0x005C7B10 at the bottom of
     * the panel; the row depends on the building's type for the panels with a list */
    U8(st, G_REDRAW_SIDEBAR) = 1;
    GAME_CALL0(st, FN_INFO_FRAME);
    if (U32(st, G_ORDERS) != 0) {
        uint32_t b = U32(st, G_INFO_BUILDING);
        int16_t type = S16(st, BUILDING_BASE + BLD_TYPE + b * 0x108u);
        int32_t n;
        if (type == 0x48 || type == 0xC1) {
            n = GAME_CALL0(st, FN_STORAGE_ROWS);
        } else if (type == 0x46 || type == 0xBE) {
            n = 0;
            for (uint32_t p = G_GOODS_AVAILABLE; p < G_PANEL_H; p += 4) {
                if (GAME_CALL(st, FN_STORAGE_KIND, S32(st, p)) != 0) {
                    n++;
                }
            }
        } else {
            n = S32(st, G_GOODS_COUNT);
        }
        if (n != 0) {
            int32_t rows = GAME_CALL(st, FN_DIV_ROUND_UP, n * 4, 3);
            buttons_draw_at(st, S32(st, G_INFO_X), (rows + 7) * 16 - 8, G_CLOSE_BUTTONS, 2);
        } else {
            buttons_draw_at(st, S32(st, G_INFO_X), 0x1B0, G_CLOSE_BUTTONS, 2);
        }
        return;
    }
    int32_t y = S32(st, G_INFO_H) * 16 + S32(st, G_INFO_Y) - 0x28;
    int32_t mode = S32(st, G_INFO_KIND);
    buttons_draw_at(st, S32(st, G_INFO_X), y, G_CLOSE_BUTTONS, 2);
    if (mode != 6 && mode != 5 && S32(st, G_INFO_KIND) == 2) {
        GAME_CALL0(st, FN_PLAY_INFO_ORDER_BUTTONS);
    }
}

void ui_draw_frame(game_state *st)
{
    /* @bin 0x004D75D0 the current window's per-frame drawing; then, for all windows, the buttons
     * flag 0x00E38DC9 cleared, 0x00414C40 when 0x00E3908C is set, and 0x0051F780 */
    uint8_t w = U8(st, G_WINDOW);
    switch (w) {
    case 0x00:
        if (U32(st, G_SOUND_ON) != 0 && GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 0) == 0 &&
            U8(st, G_MUSIC_ON) != 0) {
            GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, G_STR_SETUP_MUSIC, 0, 0); /* the main menu's music */
        }
        GAME_CALL0(st, FN_NOTHING);
        GAME_CALL0(st, FN_MAIN_MENU_FRAME);
        break;
    case 0x17: {
        GAME_CALL0(st, FN_PERIOD_SCREEN_FRAME);
        int32_t n = GAME_CALL0(st, FN_PERIOD_BUTTONS_COUNT);
        if (n != 0) {
            U8(st, G_REDRAW_SIDEBAR) = 1;
            buttons_draw_at(st, DX(), DY(), G_CAMPAIGN_BUTTONS, n);
        }
        break;
    }
    case 0x04: break;
    case 0x3F: GAME_CALL0(st, FN_POPUP_DRAW_BUTTONS); break;
    case 0x01:
        if (S32(st, G_EDITOR) == 0) {
            GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
            GAME_CALL0(st, FN_SIDEBAR_DRAW_BUTTONS);
            GAME_CALL(st, FN_SIDEBAR_DRAW_OVERLAY_NAME, 0, S32(st, G_SIDEBAR_X) + 0x0B);
            GAME_CALL(st, FN_SIDEBAR_DRAW_MINIMAP, 0);
            GAME_CALL0(st, FN_SIDEBAR_DRAW_CITY_TEXTS);
            GAME_CALL0(st, FN_CITY_COST_DRAW);
            GAME_CALL0(st, FN_DEBUG_CITY_PANEL);
            GAME_CALL0(st, FN_DEBUG_EVOLVE_PANEL);
            GAME_CALL0(st, FN_DEBUG_BUILDING_PANEL);
            GAME_CALL0(st, FN_DEBUG_FIGURE_PANEL);
            GAME_CALL0(st, FN_MESSAGE_POPUP);
            GAME_CALL0(st, FN_ANIM_DEBUG_STEP);
        } else {
            if (S32(st, G_U605A38) == 0) {
                GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
            } else {
                GAME_CALL0(st, 0x004138D0u);
                editor_buttons_draw(st, DX() + 0x0C, DY() + 0x28, G_EDITOR_MENU_BUTTONS, 2);
            }
            GAME_CALL(st, FN_SIDEBAR_DRAW_MINIMAP, 0);
            GAME_CALL0(st, 0x00533E70u);
            GAME_CALL0(st, FN_DEBUG_BUILDING_PANEL);
        }
        break;
    case 0x03: GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT); GAME_CALL0(st, FN_MENU_DRAW); break;
    case 0x11: GAME_CALL0(st, FN_MSGLIST_DRAW); break;
    case 0x06: GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT); GAME_CALL0(st, FN_BUILD_MENU_DRAW); break;
    case 0x08: GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT); GAME_CALL0(st, FN_OVERLAYS_MENU_DRAW); break;
    case 0x05: GAME_CALL0(st, FN_MESSAGE_WINDOW_FRAME); break;
    case 0x13:
        GAME_CALL(st, 0x00538FA0u, 0);
        if (U32(st, G_MESSAGE_WINDOW_VIDEO) != 0) {
            GAME_CALL0(st, FN_MESSAGE_WINDOW_FRAME);
        }
        break;
    case 0x4A: {
        uint32_t bink = U32(st, G_BINK_PLAYER);
        if (bink != 0) {
            GAME_CALLT0(st, bink, 0x00413470u);
        }
        break;
    }
    case 0x12: break;
    case 0x23:
        if (S32(st, G_TIP_WINDOW) == 1) {
            GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
        }
        GAME_CALL0(st, FN_TOOLTIP_DRAW_OVERLAY);
        break;
    case 0x09: info_frame(st); break;
    case 0x02: GAME_CALL0(st, FN_SIDEBAR_SLIDE); break;
    case 0x0A: GAME_CALL0(st, FN_DISPLAY_OPTIONS_DRAW); break;
    case 0x56: GAME_CALL0(st, FN_MESSAGE_OPTIONS_FRAME); break;
    case 0x45: GAME_CALL0(st, FN_DIFFICULTY_DIALOG_DRAW); break;
    case 0x0B:
        GAME_CALL0(st, FN_SOUND_OPTIONS_DRAW);
        if (S32(st, G_UF53F1C) > 0x14) {
            editor_buttons_draw(st, DX() + 0xD0, DY() + 0x3C, G_SOUND_ARROWS, 8);
        }
        break;
    case 0x0C:
        GAME_CALL0(st, FN_SPEED_OPTIONS_DRAW);
        if (S32(st, G_UF53F20) > 0x14) {
            editor_buttons_draw(st, DX() + 0x80, DY() + 0x28, G_SPEED_ARROWS, 4);
        }
        break;
    case 0x0D: GAME_CALL0(st, FN_FILE_DIALOG_DRAW); break;
    case 0x10: GAME_CALL0(st, FN_MAP_DIALOG_DRAW); break;
    case 0x16: GAME_CALL0(st, FN_EDITOR_NUMBER_DIALOG_DRAW); break;
    case 0x19: GAME_CALL0(st, 0x00530780u); break;
    case 0x18: GAME_CALL0(st, 0x0044AAF0u); break;
    case 0x1B: GAME_CALL0(st, 0x00532FE0u); break;
    case 0x27: GAME_CALL0(st, 0x00448640u); break;
    case 0x46: GAME_CALL0(st, FN_HOTKEYS_DRAW); break;
    case 0x4B: GAME_CALL0(st, 0x005374A0u); break;
    case 0x1C: GAME_CALL0(st, 0x00533470u); break;
    case 0x25: GAME_CALL0(st, 0x00532920u); break;
    case 0x24: GAME_CALL0(st, 0x00534320u); break;
    case 0x4E: GAME_CALL0(st, 0x00537CF0u); break;
    case 0x14:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        GAME_CALL0(st, FN_EMPIRE_MAP_DRAW);
        GAME_CALL0(st, FN_EMPIRE_DRAW);
        GAME_CALL0(st, FN_EMPIRE_TITLE_DRAW);
        buttons_draw_at(st, 0x14, SH() - 0x2C, G_EMPIRE_HELP_BUTTON, 1);
        buttons_draw_at(st, SW() - 0x2C, SH() - 0x2C, G_EMPIRE_CLOSE_BUTTON, 1);
        buttons_draw_at(st, 0x18, SH() - 0x6E, G_EMPIRE_BUTTON3, 1);
        break;
    case 0x15: {
        GAME_CALL0(st, FN_EMPIRE_MAP_DRAW);
        GAME_CALL0(st, FN_EMPIRE_TITLE_DRAW);
        GAME_CALL0(st, FN_EDITOR_MODES_DRAW);
        GAME_CALL0(st, FN_EDITOR_PANEL_DRAW);
        GAME_CALL0(st, FN_NOTHING);
        int32_t mode = S32(st, G_EDITOR_MODE);
        if (mode == 1) {
            if (S32(st, G_PICTURE_KIND) == 0) {
                editor_buttons_draw(st, 0xDF, SH() - 0x72, G_EMPIRE_EDITOR_BUTTONS, 5);
            }
        } else if (mode == 2) {
            if (U8(st, G_EMPIRE_OBJECTS + U32(st, G_EDITOR_OBJECT) * 0x62u) == 0) {
                editor_buttons_draw(st, 0xDF, SH() - 0x72, G_EMPIRE_EDITOR_BUTTONS, 5);
            }
        }
        break;
    }
    case 0x26:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        buttons_draw_at(st, DX(), DY(), G_W26_BUTTONS, 2);
        break;
    case 0x0E: GAME_CALL(st, FN_NUMPAD_DRAW, S32(st, G_NUMPAD_X), S32(st, G_NUMPAD_Y)); break;
    case 0x0F: case 0x48:
        GAME_CALL(st, FN_POPUP_LIST_DRAW, S32(st, G_LIST_A1), S32(st, G_LIST_A2), S32(st, G_LIST_PER_COLUMN));
        break;
    case 0x07: advisor_page_frame(st); break;
    case 0x28:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        GAME_CALL(st, 0x0055AD30u, 0);
        buttons_draw_at(st, AX(), AY(), G_TRADE_BUTTONS, 2);
        if (S16(st, CITY_BASE + CITY_TRADE_STATUS + (U32(st, G_PLAYER) * 0x24ECu + U32(st, G_TRADE_RESOURCE)) * 2u) == 2) {
            editor_buttons_draw(st, AX(), AY(), G_TRADE_LIMIT_ARROWS, 2);
        }
        if (S16(st, CITY_BASE + CITY_TRADE_STATUS + (U32(st, G_PLAYER) * 0x24ECu + U32(st, G_TRADE_RESOURCE)) * 2u) == 1) {
            editor_buttons_draw(st, AX(), AY(), G_TRADE_LIMIT_ARROWS_B, 2);
        }
        break;
    case 0x2A: GAME_CALL0(st, FN_DIALOG2A_FRAME); break;
    case 0x2B:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        GAME_CALL0(st, 0x00529530u);
        buttons_draw_at(st, AX(), AY(), G_FESTIVAL_BUTTONS, 4);
        break;
    case 0x52:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        GAME_CALL0(st, FN_FESTIVAL_SQUARE_FRAME);
        buttons_draw_at(st, 0x1E, 0x1E, G_W52_BUTTONS, 4);
        break;
    case 0x2C: case 0x4D:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        GAME_CALL0(st, 0x00524E20u);
        break;
    case 0x2D:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        GAME_CALL0(st, FN_DIALOG2D_FRAME);
        editor_buttons_draw(st, AX(), AY(), G_W2D_ARROWS, 2);
        break;
    case 0x2E: U8(st, G_REDRAW_SIDEBAR) = 1; GAME_CALL0(st, FN_DIALOG2E_FRAME); break;
    case 0x4C: U8(st, G_REDRAW_SIDEBAR) = 1; GAME_CALL0(st, FN_DIALOG4C_FRAME); break;
    case 0x43: GAME_CALL0(st, FN_MISSION_END_FRAME); break;
    case 0x33:
        if (U8(st, G_UE38DD9) != 1) {
            GAME_CALL0(st, FN_MISSION_END_FRAME);
        }
        break;
    case 0x35: case 0x3B:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        GAME_CALL0(st, FN_MISSION_BRIEFING_FRAME);
        buttons_draw_at(st, S32(st, G_SCREEN_X) + 0x234, S32(st, G_SCREEN_Y) + 0x19A, G_MISSION_SCREEN_BUTTON, 1);
        buttons_draw_at(st, S32(st, G_SCREEN_X) + 0x6E, S32(st, G_SCREEN_Y) + 0x1A0, G_MISSION_SCREEN_BUTTONS, 2);
        if (w == 0x35 && (uint8_t) GAME_CALLT0(st, G_CAMPAIGN_TABLE, FN_CAMPAIGN_STEP_HAS_CHOICE) != 0) {
            buttons_draw_at(st, S32(st, G_SCREEN_X) + 0x0A, S32(st, G_SCREEN_Y) + 0x19C, G_CAMPAIGN_MAP_BUTTON, 1);
        }
        break;
    case 0x34:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        if (U8(st, G_CHOICE) != 0) {
            buttons_draw_at(st, S32(st, G_SCREEN_X) + 0x244, S32(st, G_SCREEN_Y) + 0x19A, G_MISSION_SCREEN_BUTTON, 1);
        }
        GAME_CALL0(st, FN_MISSION_CHOICE_MARKS);
        break;
    case 0x3D: {
        U8(st, G_REDRAW_SIDEBAR) = 1;
        int32_t n = GAME_CALL0(st, FN_LIST_BUTTON_COUNT);
        int32_t table = GAME_CALL(st, FN_LIST_BUTTONS, n);
        GAME_CALL(st, FN_BUTTONS_DRAW, DX(), DY(), table, n);
        if ((uint8_t) GAME_CALL0(st, FN_LIST_HAS_EXTRA) != 0) {
            table = GAME_CALL(st, FN_LIST_EXTRA_BUTTON, 2);
            GAME_CALL(st, FN_BUTTONS_DRAW, DX(), DY(), table, 2);
        }
        GAME_CALL0(st, FN_LIST_ROWS_DRAW);
        break;
    }
    case 0x2F: {
        U8(st, G_REDRAW_SIDEBAR) = 1;
        int32_t n = GAME_CALL0(st, FN_FE_TWO);
        buttons_draw_at(st, DX(), DY(), G_FAMILY_LIST_BUTTONS, n);
        GAME_CALL0(st, FN_FAMILY_ROWS_DRAW);
        break;
    }
    case 0x30: GAME_CALL0(st, FN_FAMILY_MENU_FRAME); break;
    case 0x3E: GAME_CALL0(st, FN_RESOLUTION_CHECK_FRAME); break;
    case 0x36:
        U8(st, G_REDRAW_SIDEBAR) = 1;
        GAME_CALL0(st, FN_NAME_ENTRY_FRAME);
        buttons_draw_at(st, DX() + 0x1D0, DY() + 0xF9, G_MISSION_SCREEN_BUTTON, 1);
        if (rect_input(st, DX() + 0x160, DY() + 0xF6, G_W36_RECT, 1, G_HOVER) != 0) {
            return; /* @bin 0x004D8178 past the common end */
        }
        if (U8(st, G_RIGHT_RELEASED) != 0) {
            U8(st, G_RIGHT_RELEASED) = 0;
            GAME_CALL0(st, FN_FAMILY_LIST_OPEN);
            return;
        }
        break;
    case 0x51: {
        int32_t x, y;
        int32_t res = S32(st, G_RESOLUTION); /* the resolution: 1 640x480, 2 800x600, 3 1024x768 */
        if (res == 1) {
            x = 0x6E;
            y = 0x1E;
        } else if (res == 2) {
            x = 0xBE;
            y = 0x5A;
        } else {
            x = 0x12E;
            y = 0xAE;
        }
        if (rect_input(st, x, y, G_NAME_ROWS, 0x2B, G_HOVER) != 0) {
            return;
        }
        GAME_CALL0(st, FN_CHOICE_WINDOW_DRAW);
        break;
    }
    case 0x1E: case 0x49: case 0x1F: case 0x20:
        GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
        GAME_CALL(st, FN_SIDEBAR_DRAW_MINIMAP, 0);
        GAME_CALL0(st, FN_SIDEBAR_DRAW_CITY_TEXTS);
        break;
    case 0x53:
        GAME_CALL0(st, FN_HIGHSCORE_DRAW);
        buttons_draw_at(st, DX(), DY(), G_W53_BUTTONS, 2);
        break;
    case 0x55: GAME_CALL0(st, FN_CHEAT_WINDOW_DRAW); break;
    default: break;
    }
    uint8_t refresh = U8(st, G_FAST_FORWARD);
    U8(st, G_REDRAW_SIDEBAR) = 0;
    if (refresh != 0) {
        GAME_CALL0(st, FN_NOTICE_PANEL_DRAW);
    }
    GAME_CALL0(st, FN_NOTHING);
}

/* ---- input 0x004D8310 ---- */

static void city_mouse(game_state *st)
{
    /* @bin 0x004D8A44 the city view's mouse: the hovered tile 0x00405F80; off the map nothing;
     * a left press tries the legion / transport / warship under the mouse, then the construction */
    GAME_CALL0(st, FN_FRAME_HOVER_TILE);
    int32_t tile = S32(st, G_MOUSE_OFFSET);
    S32(st, G_NO_ROOM) = 0;
    if (tile == 0) {
        return;
    }
    if (U8(st, G_LEFT_PRESSED) != 0) {
        if (GAME_CALL0(st, FN_CITY_CLICK_LEGION) != 0 || GAME_CALL0(st, FN_CITY_CLICK_TRANSPORT) != 0 ||
            GAME_CALL0(st, FN_CITY_CLICK_WARSHIP) != 0) {
            return;
        }
        GAME_CALL0(st, FN_CONSTRUCTION_MOUSE_DOWN);
    } else if (U8(st, G_LEFT_HELD) != 0) {
        GAME_CALL0(st, FN_CONSTRUCTION_DRAG);
    } else if (U8(st, G_LEFT_RELEASED) != 0) {
        GAME_CALL0(st, FN_CONSTRUCTION_MOUSE_UP);
    } else if (U8(st, G_RIGHT_RELEASED) != 0) {
        if (GAME_CALL0(st, FN_CONSTRUCTION_CANCEL) == 0) {
            GAME_CALL0(st, FN_INFO_OPEN);
        }
    }
}

static int32_t topbar_input(game_state *st)
{
    /* the menu bar's right button and the top bar's figures (right click: their help); 1 when done */
    if ((uint8_t) GAME_CALL0(st, FN_MENUBAR_BUTTON_CLICK) != 0) {
        return 1;
    }
    GAME_CALL0(st, FN_TOPBAR_HOVER);
    if (S32(st, G_TOPBAR_HOVER) != 0 && U8(st, G_RIGHT_RELEASED) != 0) {
        GAME_CALL0(st, FN_TOPBAR_HELP);
        return 1;
    }
    return 0;
}

static void city_input(game_state *st)
{
    /* @bin 0x004D865B window 1: menu bar, scrolling, minimap, sidebar, then the city's mouse */
    S32(st, G_UE94E14) = 0;
    uint32_t menus = S32(st, G_EDITOR) == 1 ? G_EDITOR_MENUBAR : G_MENUBAR;
    S32(st, G_MENUBAR_HOVER) = GAME_CALL(st, FN_MENUBAR_HOVER, (int32_t) menus, 4);
    if (GAME_CALL(st, FN_MENUBAR_CLICK, (int32_t) menus, 4) != 0) {
        return;
    }
    if (S32(st, G_EDITOR) == 0) {
        GAME_CALL0(st, FN_TUTORIAL_CHECKS);
    }
    GAME_CALL(st, FN_CAMERA_SCROLL, GAME_CALL0(st, FN_SCROLL_DIRECTION));
    if (S32(st, G_EDITOR) == 1) {
        int32_t count;
        if (S32(st, G_U605A38) > 0) {
            int32_t hit = rect_hit(st, DX() + 0x0C, DY() + 0x28, G_EDITOR_MENU_RECTS, 8);
            S32(st, G_HOVER) = rect_highlight(st, DX() + 0x0C, DY() + 0x28, G_EDITOR_MENU_RECTS, 8, hit);
        } else {
            hover(st, S32(st, G_SIDEBAR_X), 0x1E, G_EDITOR_SIDEBAR_BUTTONS, 0x0F, 1);
        }
        GAME_CALL0(st, FN_MINIMAP_HOVER);
        if (GAME_CALL(st, FN_MINIMAP_CLICK, 0) != 0 || (uint8_t) GAME_CALL0(st, FN_MENUBAR_BUTTON_CLICK) != 0) {
            return;
        }
        if (S32(st, G_U605A38) == 0) {
            count = 0x0F;
        } else {
            if (U8(st, G_RIGHT_PRESSED) != 0) {
                S32(st, G_U605A38) = 0;
                U8(st, G_REDRAW) = 1;
                return;
            }
            GAME_CALL(st, FN_TEXT_BOX_FOCUS, 5);
            count = 2;
            if (rect_input(st, DX() + 0x0C, DY() + 0x28, G_EDITOR_MENU_RECTS, 0x0A, G_HOVER) != 0 ||
                editor_buttons(st, DX() + 0x0C, DY() + 0x28, G_EDITOR_MENU_BUTTONS, 2) != 0) {
                return;
            }
        }
        if (click(st, S32(st, G_SIDEBAR_X), 0x18, G_EDITOR_SIDEBAR_BUTTONS, count) != 0) {
            U8(st, G_REDRAW_SIDEBAR) = 1;
            return;
        }
        if (S32(st, G_U605A38) == 1) {
            return;
        }
        city_mouse(st);
        return;
    }
    int32_t collapsed = S32(st, G_SIDEBAR_COLLAPSED);
    if (collapsed == 0) {
        GAME_CALL0(st, FN_MINIMAP_HOVER);
        if (GAME_CALL(st, FN_MINIMAP_CLICK, 0) != 0 || topbar_input(st) != 0) {
            return;
        }
        int32_t x = S32(st, G_SIDEBAR_X);
        if (hover_click(st, x, 0x1E, G_SIDEBAR_TOP_BUTTONS, 2, 0x0A) != 0) {
            U8(st, G_REDRAW_SIDEBAR) = 1;
            return;
        }
        hover(st, S32(st, G_SIDEBAR_X), 0x1E, G_SIDEBAR_BUTTONS, 0x10, 0x1E);
        if (GAME_CALL(st, FN_BUTTONS_RECT_CLICK, S32(st, G_SIDEBAR_X), 0x1E, G_SIDEBAR_RECT, 1) != 0) {
            return;
        }
        if (click(st, S32(st, G_SIDEBAR_X), 0x1E, G_SIDEBAR_BUTTONS, 0x10) != 0) {
            U8(st, G_REDRAW_SIDEBAR) = 1;
            return;
        }
        if (hover_click(st, S32(st, G_SIDEBAR_X), 0x1E, G_SIDEBAR_LOW_BUTTONS, 2, 0x32) != 0) {
            U8(st, G_REDRAW_SIDEBAR) = 1;
            return;
        }
    } else if (collapsed == 1) {
        if (topbar_input(st) != 0) {
            return;
        }
        if (hover_click(st, S32(st, G_SIDEBAR_X_COLLAPSED), 0x18, G_SIDEBAR_COLLAPSED_TOP_BUTTONS, 1, 0x0A) != 0 ||
            hover_click(st, S32(st, G_SIDEBAR_X_COLLAPSED), 0x18, G_SIDEBAR_COLLAPSED_BUTTONS, 0x0C, 0x1E) != 0) {
            U8(st, G_REDRAW_SIDEBAR) = 1;
            return;
        }
    }
    city_mouse(st);
}

static void build_menu_input(game_state *st)
{
    /* @bin 0x004D8B30 window 6: right click closes; the menu's items 0x00403E00, then the sidebar */
    if (U8(st, G_RIGHT_RELEASED) != 0) {
        close_to(st, 1);
        return;
    }
    if (GAME_CALL0(st, FN_BUILD_MENU_INPUT) != 0) {
        return;
    }
    int32_t r;
    if (S32(st, G_EDITOR) == 1) {
        if (click(st, S32(st, G_SIDEBAR_X), 0x1E, G_EDITOR_SIDEBAR_BUTTONS, S32(st, G_U605A38) != 0 ? 2 : 0x0F) != 0) {
            U8(st, G_REDRAW_SIDEBAR) = 1;
            return;
        }
    }
    int32_t collapsed = S32(st, G_SIDEBAR_COLLAPSED);
    if (collapsed == 0) {
        if (GAME_CALL(st, FN_BUTTONS_RECT_CLICK, S32(st, G_SIDEBAR_X), 0x1E, G_SIDEBAR_RECT, 1) != 0) {
            return;
        }
        r = click(st, S32(st, G_SIDEBAR_X), 0x1E, G_SIDEBAR_BUTTONS, 0x10);
    } else if (collapsed == 1) {
        r = click(st, S32(st, G_SIDEBAR_X_COLLAPSED), 0x18, G_SIDEBAR_COLLAPSED_BUTTONS, 0x0C);
    } else {
        return;
    }
    if (r != 0) {
        U8(st, G_REDRAW_SIDEBAR) = 1;
    }
}

static void overlays_menu_input(game_state *st)
{
    /* @bin 0x004D8C08 window 8: right press closes; the overlay groups 0x005C2BF0, then the open
     * group's items 0x005C2D08 */
    if (U8(st, G_RIGHT_PRESSED) != 0) {
        close_to(st, 1);
        return;
    }
    if (rect_input(st, S32(st, G_SIDEBAR_X) - 0xAA, S32(st, G_MENU_Y) + 0x20, G_OVL_GROUP_RECTS, 0x0A, G_HOVER) != 0) {
        return;
    }
    GAME_CALL0(st, FN_OVERLAYS_MENU_HOVER);
    if (S32(st, G_OVL_GROUP) == 0) {
        return;
    }
    rect_input(st, S32(st, G_SIDEBAR_X) - 0x15C, S32(st, G_MENU_Y) + S32(st, G_OVL_ROW) * 24 + 0x20,
               G_OVL_ITEM_RECTS, S32(st, G_OVL_ROWS), G_HOVER_2);
}

static void empire_input(game_state *st)
{
    /* @bin 0x004D9A47 window 0x14, the empire map: scrolling, its three buttons, a left press on
     * the map picks the object under the mouse (0x00407A10), right press clears it; a trade city
     * (kind 1, 3, 5, not yet open) shows its open-route button 0x005C3DA0 */
    GAME_CALL(st, FN_EMPIRE_SCROLL_KEY, GAME_CALL0(st, FN_SCROLL_DIRECTION));
    if (hover_click(st, 0x14, SH() - 0x2C, G_EMPIRE_HELP_BUTTON, 1, 0x0A) != 0 ||
        hover_click(st, SW() - 0x2C, SH() - 0x2C, G_EMPIRE_CLOSE_BUTTON, 1, 0x0B) != 0 ||
        hover_click(st, 0x18, SH() - 0x6E, G_EMPIRE_BUTTON3, 1, 0x0C) != 0) {
        return;
    }
    if (U8(st, G_LEFT_PRESSED) != 0) {
        int32_t mx = S32(st, G_MOUSE_X), left = S32(st, G_MAP_X);
        if (mx >= left && mx < S32(st, G_UE38D9C) - 2 * left) {
            int32_t my = S32(st, G_MOUSE_Y), top = S32(st, G_MAP_Y);
            if (my >= top && my < S32(st, G_DRAW_H) - S32(st, G_MAP_BOTTOM) - top) {
                GAME_CALL(st, FN_EMPIRE_PICK, G_SELECTED, G_ARMY_SEL);
                U8(st, G_REDRAW) = 1;
            }
        }
    }
    if (U8(st, G_RIGHT_PRESSED) != 0) {
        S32(st, G_SELECTED) = 0;
        S32(st, G_ARMY_SEL) = -1;
        U8(st, G_REDRAW) = 1;
    } else {
        int32_t obj = S32(st, G_SELECTED);
        if (obj != 0 && U8(st, G_EMPIRE_OBJECTS + (uint32_t) (obj - 1) * 0x62u) == 1) {
            int32_t city = GAME_CALL(st, FN_EMPIRE_CITY_OF_OBJECT, obj - 1);
            S32(st, G_EMPIRE_SELECTED) = city;
            uint32_t c = (uint32_t) city * 0x6Au;
            uint8_t kind = U8(st, G_EMPIRE_CITY_KIND + c);
            if ((kind == 1 || kind == 3 || kind == 5) && U8(st, G_EMPIRE_CITY_OPEN + c) == 0) {
                if (rect_input(st, (SW() - 0x1F4) / 2, SH() - 0x69, G_OPEN_ROUTE_RECT, 1, G_HOVER_2) != 0) {
                    return;
                }
            }
        }
    }
    if (U8(st, G_RIGHT_RELEASED) != 0) {
        GAME_CALL0(st, FN_EMPIRE_CLOSE_409520);
    }
}

static void advisors_input(game_state *st)
{
    /* @bin 0x004DBF4E window 7: the advisor buttons 0x005F8320, the help button, right click back
     * to the city (the page's flag +0x23C of the player cleared), then the page's own tables */
    int32_t ay = S32(st, G_ADVISOR_Y) + 0x1B0;
    int32_t dx = DX();
    hover(st, dx, ay, G_TABS, 0x0E, 0x0A);
    if (click(st, dx, ay, G_TABS, 0x0E) != 0) {
        return;
    }
    hover(st, AX(), (S32(st, G_PANEL_H) - 2) * 16 + AY(), G_ADVISOR_HELP_BUTTON, 1, 0x19);
    if (click(st, AX(), (S32(st, G_PANEL_H) - 2) * 16 + AY(), G_ADVISOR_HELP_BUTTON, 1) != 0) {
        return;
    }
    if (U8(st, G_RIGHT_RELEASED) != 0) {
        uint32_t p = U32(st, G_PLAYER) * 0x49D8u;
        close_to(st, 1);
        U8(st, CITY_BASE + CITY_U49CC + p) = 0;
        return;
    }
    switch (S32(st, G_PAGE)) {
    case 1:
        if (rect_input(st, AX(), AY(), G_ADVISOR1_RECTS, 9, G_HOVER) != 0) {
            return;
        }
        editor_hover(st, AX(), AY(), G_TAX_ARROWS, 2, 0x0A);
        editor_buttons(st, AX(), AY(), G_TAX_ARROWS, 2);
        return;
    case 2: rect_input(st, AX(), AY(), G_ADVISOR2_RECTS, 0x14, G_HOVER); return;
    case 3: rect_input(st, AX(), AY(), G_ADVISOR3_RECTS, 0x14, G_HOVER); return;
    case 4: rect_input(st, AX(), AY(), G_ADVISOR4_RECTS, 8, G_HOVER); return;
    case 5: rect_input(st, AX(), AY(), G_ADVISOR5_RECTS, 4, G_HOVER); return;
    case 6: {
        int32_t n = S32(st, G_STORAGE_LIST);
        if (n > 0x0F) {
            n = 0x0F;
        }
        if (rect_input(st, AX(), AY(), G_ADVISOR6_RECTS, n + 3, G_HOVER) != 0) {
            return;
        }
        GAME_CALL0(st, 0x0055BB90u);
        return;
    }
    case 7: rect_input(st, AX(), AY(), G_ADVISOR7_RECTS, 2, G_HOVER); return;
    case 0x0B:
        if (rect_input(st, AX(), AY(), G_ADVISOR11_RECT, 1, G_HOVER) != 0) {
            return;
        }
        rect_input(st, AX(), AY(), G_RELIGION_ROWS, 5, 0); /* @bin 0x004DC43A the highlight is not stored */
        return;
    case 0x0C:
        editor_hover(st, AX(), AY(), G_TAX_BUTTONS, 2, 0x0A);
        editor_buttons(st, AX(), AY(), G_TAX_BUTTONS, 2);
        return;
    case 0x0E: rect_input(st, AX(), AY(), G_GOAL_RECTS, 8, G_HOVER); return;
    case 0x0D: rect_input(st, AX(), AY(), G_PAGE13_RECTS, 0x0C, G_HOVER); return;
    default: return;
    }
}

static void family_input(game_state *st)
{
    /* @bin 0x004DC56E window 0x28 (the family / kingdom page over the hall) */
    S32(st, G_HOVER) = 0;
    S32(st, G_HOVER_2) = 0;
    S32(st, G_HOVER_3) = 0;
    if (U8(st, G_RIGHT_RELEASED) != 0) {
        close_to(st, 7);
        return;
    }
    if (hover_click(st, AX(), AY(), G_TRADE_BUTTONS, 2, 0x0A) != 0) {
        return;
    }
    int16_t mode = S16(st, CITY_BASE + CITY_TRADE_STATUS + (U32(st, G_PLAYER) * 0x24ECu + U32(st, G_TRADE_RESOURCE)) * 2u);
    if (mode == 2) {
        editor_hover(st, AX(), AY(), G_TRADE_LIMIT_ARROWS, 2, 0x0A);
        if (editor_buttons(st, AX(), AY(), G_TRADE_LIMIT_ARROWS, 2) != 0) {
            return;
        }
    } else if (mode == 1) {
        editor_hover(st, AX(), AY(), G_TRADE_LIMIT_ARROWS_B, 2, 0x0A);
        if (editor_buttons(st, AX(), AY(), G_TRADE_LIMIT_ARROWS_B, 2) != 0) {
            return;
        }
    }
    if (rect_input(st, AX(), AY(), G_TRADE_RECT_A, 1, G_HOVER) != 0 ||
        rect_input(st, AX(), AY(), G_TRADE_RECT_B, 1, G_HOVER_2) != 0) {
        return;
    }
    if (S32(st, G_HOVER_2) == 0) {
        if (rect_input(st, AX(), AY(), G_TRADE_RECT_C, 1, G_HOVER_2) != 0) {
            return;
        }
        if (S32(st, G_HOVER_2) != 0) {
            S32(st, G_HOVER_2)++;
        }
    }
    rect_input(st, AX(), AY(), G_TRADE_STOCKPILE_RECT, 1, G_HOVER_3);
}

static void family_select_input(game_state *st)
{
    /* @bin 0x004DCDF0 window 0x33: the family list (table and count by 0x004EA880 / 0x004EA8D0) */
    int32_t x = S32(st, G_PANEL_X), y = S32(st, G_PANEL_Y) - 0x70;
    int32_t n = GAME_CALL(st, FN_MISSION_END_KIND, DX(), DY());
    int32_t table = GAME_CALL(st, FN_MISSION_END_BUTTONS, n);
    int32_t hit = rect_hit(st, x, y, (uint32_t) table, n);
    n = GAME_CALL0(st, FN_MISSION_END_KIND);
    table = GAME_CALL(st, FN_MISSION_END_BUTTONS, n);
    S32(st, G_HOVER) =
        rect_highlight(st, S32(st, G_PANEL_X), S32(st, G_PANEL_Y) - 0x70, (uint32_t) table, n, hit);
    n = GAME_CALL0(st, FN_MISSION_END_KIND);
    table = GAME_CALL(st, FN_MISSION_END_BUTTONS, n);
    GAME_CALL(st, FN_BUTTONS_RECT_CLICK, S32(st, G_PANEL_X), S32(st, G_PANEL_Y) - 0x70, table, n);
}

static void mission_menu_input(game_state *st)
{
    /* @bin 0x004DCFCA window 0x3D */
    if (GAME_CALL0(st, FN_LIST_SCROLL_DRAG) != 0) {
        return;
    }
    int32_t n = GAME_CALL0(st, FN_LIST_BUTTON_COUNT);
    int32_t table = GAME_CALL(st, FN_LIST_BUTTONS, n);
    if (click(st, DX(), DY(), (uint32_t) table, n) != 0) {
        return;
    }
    if ((uint8_t) GAME_CALL0(st, FN_LIST_HAS_EXTRA) != 0) {
        table = GAME_CALL(st, FN_LIST_EXTRA_BUTTON, 2);
        if (click(st, DX(), DY(), (uint32_t) table, 2) != 0) {
            return;
        }
    }
    if (S32(st, G_SCENARIO_LIST_MODE) == 1) {
        table = GAME_CALL(st, FN_LIST_TAB, 1);
        int32_t hit = rect_hit(st, DX() + 0x168, DY() + 0x19A, (uint32_t) table, 1);
        table = GAME_CALL(st, FN_LIST_TAB, 1);
        S32(st, G_HOVER_2) = rect_highlight(st, DX() + 0x168, DY() + 0x19A, (uint32_t) table, 1, hit);
        table = GAME_CALL(st, FN_LIST_TAB, 1);
        /* @bin 0x004DD0B9 the click's rectangle is 10 pixels left of the hit's (0x15E, not 0x168) */
        if (GAME_CALL(st, FN_BUTTONS_RECT_CLICK, DX() + 0x15E, DY() + 0x19A, table, 1) != 0) {
            return;
        }
    }
    n = GAME_CALL(st, FN_LIST_ROWS_COUNT, DX(), DY());
    table = GAME_CALL(st, FN_LIST_ROWS_TABLE, n);
    int32_t hit = rect_hit(st, DX(), DY(), (uint32_t) table, n);
    n = GAME_CALL0(st, FN_LIST_ROWS_COUNT);
    table = GAME_CALL(st, FN_LIST_ROWS_TABLE, n);
    S32(st, G_HOVER) = rect_highlight(st, DX(), DY(), (uint32_t) table, n, hit);
    n = GAME_CALL0(st, FN_LIST_ROWS_COUNT);
    table = GAME_CALL(st, FN_LIST_ROWS_TABLE, n);
    if (GAME_CALL(st, FN_BUTTONS_RECT_CLICK, DX(), DY(), table, n) != 0) {
        return;
    }
    if (U8(st, G_RIGHT_RELEASED) != 0) {
        GAME_CALL0(st, FN_LIST_BACK_CLICK);
    }
}

static void target_input(game_state *st, uint32_t clear, uint32_t on_click)
{
    /* @bin 0x004DD4B5 windows 0x1E, 0x49, 0x1F, 0x20 (a target for a legion / ships): the hovered
     * tile, the minimap, scrolling; right click back to the city (0x1F, 0x20 clear their figure
     * 0x00D05708 / 0x00D0570C); a left press picks the target */
    GAME_CALL0(st, FN_FRAME_HOVER_TILE);
    if (S32(st, G_SIDEBAR_COLLAPSED) == 0) {
        GAME_CALL0(st, FN_MINIMAP_HOVER);
        if (GAME_CALL(st, FN_MINIMAP_CLICK, 1) != 0) {
            return;
        }
    }
    GAME_CALL(st, FN_CAMERA_SCROLL, GAME_CALL0(st, FN_SCROLL_DIRECTION));
    if (U8(st, G_RIGHT_RELEASED) != 0) {
        U8(st, G_WINDOW) = 1;
        if (clear != 0) {
            S32(st, clear) = 0;
        }
        GAME_CALL0(st, FN_VIEW_ROWS_RESET);
        U8(st, G_REDRAW) = 1;
        return;
    }
    GAME_CALL0(st, FN_FRAME_HOVER_TILE);
    if (U8(st, G_LEFT_PRESSED) != 0) {
        GAME_CALL0(st, on_click);
    }
}

static int32_t right_closes(game_state *st, uint8_t to)
{
    if (U8(st, G_RIGHT_PRESSED) != 0) {
        close_to(st, to);
        return 1;
    }
    return 0;
}

static int32_t right_release_closes(game_state *st, uint8_t to)
{
    if (U8(st, G_RIGHT_RELEASED) != 0) {
        close_to(st, to);
        return 1;
    }
    return 0;
}

static void video_start(game_state *st, uint32_t name, int32_t w, int32_t h, int32_t next)
{
    /* windows 0x44, 0x40, 0x41: the intro videos, centred, then window `next` */
    S32(st, G_VIDEO_STATE) = 2;
    GAME_CALL0(st, FN_SCREEN_BUFFER_CLEAR);
    GAME_CALL(st, FN_UI_VIDEO_OPEN, (int32_t) name, (SW() - w) / 2, (SH() - h) / 2, 1, next);
}

void ui_input(game_state *st)
{
    /* @bin 0x004D8310 the current window's input: the text field, the hover globals 0x007056A8 /
     * 0x007056B0 / 0x007056B8 cleared and the mouse kept in 0x00E38E2C / 0x00E38E28; a finished drag
     * (0x00E38E14 = 2) ends and redraws; then the window's handlers in the original's order */
    int32_t drag = S32(st, G_DRAG_STATE);
    int32_t mx = S32(st, G_MOUSE_X), my = S32(st, G_MOUSE_Y);
    S32(st, G_CARET_FIELD) = 0;
    S32(st, G_HOVER_BUTTON) = 0;
    S32(st, G_SQUARE_BUTTON_HOVER) = 0;
    S32(st, G_MENUBAR_HOVER) = 0;
    S32(st, G_HIT_X) = mx;
    S32(st, G_HIT_Y) = my;
    if (drag == 2) {
        S32(st, G_DRAG_STATE) = 0;
        U8(st, G_REDRAW) = 1;
    }
    uint8_t w = U8(st, G_WINDOW);
    switch (w) {
    case 0x00: rect_input(st, DX(), DY(), G_MENU_TABLE, 5, G_HOVER); return;
    case 0x17: {
        int32_t n = GAME_CALL0(st, FN_PERIOD_BUTTONS_COUNT);
        if (n > 0 && click(st, DX(), DY(), G_CAMPAIGN_BUTTONS, n) != 0) {
            return;
        }
        if (rect_input(st, DX(), DY(), G_PERIOD_BUTTONS, 0x0B, G_HOVER) != 0 || U8(st, G_RIGHT_RELEASED) == 0) {
            return;
        }
        if (S32(st, G_CAMPAIGN_CAMPAIGN_KIND) == 1 || S32(st, G_PERIOD_REACHED) == 0) {
            GAME_CALL0(st, FN_MAIN_MENU_OPEN);
        }
        return;
    }
    case 0x04: GAME_CALL0(st, FN_MISSION_PICTURE_INPUT); return;
    case 0x3F: GAME_CALL0(st, FN_POPUP_INPUT); return;
    case 0x44: video_start(st, G_STR_SMK_LOGO, 0x1F4, 0x124, 0x40); return;
    case 0x40: video_start(st, G_STR_SMK_INTRO, 0x1F4, 0x124, 0x41); return;
    case 0x41: video_start(st, G_STR_SMK_CREDITS, 0x280, 0x1E0, 0x3C); return;
    case 0x42:
        S32(st, G_U7028F4) = 0;
        GAME_CALL0(st, FN_HIGHSCORE_UPDATE);
        GAME_CALL(st, FN_FADE_START, 2, 0x33, 0x3E8);
        return;
    case 0x43: {
        int32_t n = 1;
        if (S8(st, G_MISSION) >= 2 || S32(st, G_CUSTOM) != 0 || U8(st, G_FLOW2_PERIOD) != 0) {
            n = 3;
        }
        rect_input(st, S32(st, G_PANEL_X), S32(st, G_PANEL_Y), G_W43_RECTS, n, G_HOVER);
        return;
    }
    case 0x01: city_input(st); return;
    case 0x12:
        if (S32(st, G_EDITOR) == 1 || S32(st, G_SIDEBAR_COLLAPSED) != 0) {
            return;
        }
        GAME_CALL(st, FN_BUTTONS_RECT_CLICK, S32(st, G_SIDEBAR_X), 0x18, G_SIDEBAR_RECT, 1);
        return;
    case 0x06: build_menu_input(st); return;
    case 0x08: overlays_menu_input(st); return;
    case 0x05: GAME_CALL0(st, FN_MESSAGE_WINDOW_INPUT); return;
    case 0x13:
        if (S32(st, G_MESSAGE_WINDOW_VIDEO) != 0) {
            GAME_CALL0(st, FN_MESSAGE_WINDOW_INPUT);
            return;
        }
        if (U8(st, G_RIGHT_RELEASED) != 0) {
            GAME_CALL0(st, FN_PLATFORM_VIDEO_CLOSE);
        }
        if (U8(st, G_LEFT_RELEASED) != 0) {
            GAME_CALL0(st, FN_PLATFORM_VIDEO_CLOSE);
        }
        return;
    case 0x4A: {
        if (S32(st, G_MESSAGE_WINDOW_VIDEO) != 0) {
            GAME_CALL0(st, FN_MESSAGE_WINDOW_INPUT);
            return;
        }
        uint32_t bink = U32(st, G_BINK_PLAYER);
        if (bink != 0 && (U8(st, G_RIGHT_RELEASED) != 0 || U8(st, G_LEFT_RELEASED) != 0)) {
            GAME_CALLT0(st, bink, FN_BINK_END);
        }
        return;
    }
    case 0x09: GAME_CALL0(st, FN_INFO_INPUT); return;
    case 0x03: {
        uint32_t menus = S32(st, G_EDITOR) == 1 ? G_EDITOR_MENUBAR : G_MENUBAR;
        if (GAME_CALL(st, FN_MENU_INPUT, (int32_t) menus, 4) != 0) {
            return;
        }
        if (U8(st, G_RIGHT_RELEASED) != 0) {
            GAME_CALL0(st, FN_MENU_CLOSE);
        }
        return;
    }
    case 0x11: GAME_CALL0(st, FN_MSGLIST_INPUT); return;
    case 0x02: GAME_CALL(st, FN_CAMERA_SCROLL, GAME_CALL0(st, FN_SCROLL_DIRECTION)); return;
    case 0x0A:
        if (!right_release_closes(st, 1)) {
            rect_input(st, DX(), DY(), G_DISPLAY_OPTIONS_RECTS, 5, G_HOVER);
        }
        return;
    case 0x56:
        if (!right_release_closes(st, 1)) {
            rect_input(st, DX(), DY(), G_MESSAGE_OPTIONS_RECTS, 0x0E, G_HOVER);
        }
        return;
    case 0x45:
        if (U8(st, G_RIGHT_RELEASED) != 0) {
            uint32_t p = U32(st, G_PLAYER) * 0x49D8u;
            int32_t limit = S32(st, G_DIFFICULTY);
            close_to(st, 1);
            if (S32(st, CITY_BASE + CITY_DIFFICULTY_COPY + p) > limit) {
                S32(st, CITY_BASE + CITY_DIFFICULTY_COPY + p) = limit;
            }
            return;
        }
        editor_buttons(st, DX() + 0x120, DY() + 0x70, G_DIFFICULTY_ARROWS, 2);
        return;
    case 0x0B:
        if (right_release_closes(st, 1) || rect_input(st, DX(), DY(), G_SOUND_OPTIONS_RECTS, 6, G_HOVER) != 0) {
            return;
        }
        editor_buttons(st, DX() + 0xD0, DY() + 0x3C, G_SOUND_ARROWS, 8);
        return;
    case 0x0C:
        if (right_release_closes(st, 1) || rect_input(st, DX(), DY(), G_SPEED_OPTIONS_RECTS, 2, G_HOVER) != 0) {
            return;
        }
        editor_buttons(st, DX() + 0x80, DY() + 0x28, G_SPEED_ARROWS, 4);
        return;
    case 0x0D:
    case 0x10:
        GAME_CALL(st, FN_TEXT_BOX_FOCUS, w == 0x0D ? 2 : 4);
        if (S32(st, G_UE20998) != 0) {
            S32(st, G_UE20998) = 0;
            S32(st, G_PARAM1) = 1;
            GAME_CALL0(st, w == 0x0D ? FN_FILE_DIALOG_OK : FN_MAP_DIALOG_OK);
        }
        if (w == 0x0D) {
            if (U8(st, G_RIGHT_RELEASED) != 0) {
                U8(st, G_RIGHT_RELEASED) = 0;
                close_to(st, U8(st, G_BACK_WINDOW));
                return;
            }
        } else if (right_closes(st, 1)) {
            return;
        }
        if (rect_input(st, DX(), DY(), G_FILE_ROWS_RECTS, 0x0C, G_HOVER) != 0 ||
            hover_click(st, DX(), DY(), w == 0x0D ? G_FILE_DIALOG_BUTTONS : G_MAP_DIALOG_BUTTONS, 4, 0x0A) != 0) {
            return;
        }
        GAME_CALL0(st, FN_FILE_DIALOG_DRAG);
        return;
    case 0x16:
        if (!right_closes(st, 0x25)) {
            rect_input(st, DX() + 0xA0, DY() + 0x2C, G_W16_RECTS, 2, G_HOVER);
        }
        return;
    case 0x19:
        if (!right_closes(st, 1)) {
            rect_input(st, DX(), DY() + 0x50, G_W19_RECTS, 3, G_HOVER);
        }
        return;
    case 0x18:
        if (U8(st, G_RIGHT_PRESSED) != 0) {
            GAME_CALL0(st, 0x00448CF0u);
            return;
        }
        rect_input(st, DX(), DY(), G_EVENT_RECTS, 0x1E, G_HOVER);
        return;
    case 0x1B:
        if (!right_closes(st, 1)) {
            rect_input(st, DX() + 0x10, DY() + 0x20, G_W1B_RECTS, 0x2F, G_HOVER);
        }
        return;
    case 0x46:
        if (!right_closes(st, 1)) {
            rect_input(st, DX(), DY(), G_HOTKEY_RECTS_FIRST, 0x14, G_HOVER);
        }
        return;
    case 0x4B:
        if (!right_closes(st, 0x1C)) {
            rect_input(st, DX(), DY(), G_W4B_RECTS, 0x13, G_HOVER);
        }
        return;
    case 0x27:
        if (right_closes(st, 1) || rect_input(st, DX(), DY(), G_EVENT_LIST_RECTS, 0x0E, G_HOVER) != 0 ||
            hover_click(st, DX(), DY(), G_EVENT_LIST_BUTTONS, 2, 0x0A) != 0) {
            return;
        }
        GAME_CALL0(st, 0x004483F0u);
        return;
    case 0x1C:
        if (!right_closes(st, 1)) {
            rect_input(st, DX() + 0x10, DY() + 0x20, G_W1C_RECTS, 0x12, G_HOVER);
        }
        return;
    case 0x25:
        if (!right_closes(st, 1)) {
            rect_input(st, DX() + 0x0C, DY() + 0x0C, G_W25_RECTS, 0x0C, G_HOVER);
        }
        return;
    case 0x24:
        if (!right_closes(st, 0x15)) {
            rect_input(st, DX() + 0x10, DY() + 0x20, G_W24_RECTS, 0x10, G_HOVER);
        }
        return;
    case 0x4E:
        if (!right_closes(st, 0x15)) {
            rect_input(st, DX() + 0x10, DY(), G_W4E_RECTS, 0x45, G_HOVER);
        }
        return;
    case 0x47: right_closes(st, 0x15); return;
    case 0x14: empire_input(st); return;
    case 0x15:
        empire_editor_input(st, mx, my);
        return;
    case 0x26: hover_click(st, DX(), DY(), G_W26_BUTTONS, 2, 0x0A); return;
    case 0x0E:
        if (U8(st, G_RIGHT_PRESSED) != 0) {
            close_to(st, U8(st, G_NUMPAD_BACK));
            return;
        }
        rect_input(st, S32(st, G_NUMPAD_X), S32(st, G_NUMPAD_Y), G_NUMPAD_RECTS, 0x0C, G_HOVER);
        return;
    case 0x0F:
    case 0x48:
        if (U8(st, G_RIGHT_PRESSED) != 0) {
            uint8_t back = U8(st, G_LIST_PREV_WINDOW);
            S32(st, G_LIST_EXTRA_ON) = 0;
            close_to(st, back);
            return;
        }
        rect_input(st, S32(st, G_LIST_A1), S32(st, G_LIST_A2), G_LIST_BUTTONS, S32(st, G_LIST_COUNT), G_HOVER);
        return;
    case 0x07: advisors_input(st); return;
    case 0x28: family_input(st); return;
    case 0x2A:
        if (!right_release_closes(st, 7)) {
            rect_input(st, AX(), AY(), G_PRIORITY_RECTS, S32(st, G_PRIORITY_CHOICES) + 1, G_HOVER);
        }
        return;
    case 0x2B:
        if (right_release_closes(st, 7) || hover_click(st, AX(), AY(), G_FESTIVAL_BUTTONS, 4, 0x0A) != 0) {
            return;
        }
        rect_input(st, AX(), AY(), G_GOD_RECTS, 8, G_HOVER);
        return;
    case 0x52:
        if (right_release_closes(st, 9) || hover_click(st, 0x1E, 0x1E, G_W52_BUTTONS, 4, 0x0A) != 0) {
            return;
        }
        rect_input(st, 0x1E, 0x1E, G_W52_RECTS, 8, G_HOVER);
        return;
    case 0x2C:
        if (!right_release_closes(st, 7)) {
            rect_input(st, AX(), AY(), G_SALARY_RECTS, 0x0C, G_HOVER);
        }
        return;
    case 0x4D:
        if (!right_release_closes(st, 9)) {
            rect_input(st, AX(), AY(), G_SALARY_RECTS, 0x0C, G_HOVER);
        }
        return;
    case 0x2D:
        if (right_release_closes(st, 7) || rect_input(st, AX(), AY(), G_W2D_RECTS, 7, G_HOVER) != 0) {
            return;
        }
        editor_hover(st, AX(), AY(), G_W2D_ARROWS, 2, 0x0A);
        editor_buttons(st, AX(), AY(), G_W2D_ARROWS, 2);
        return;
    case 0x2E:
        if (!right_release_closes(st, 7)) {
            rect_input(st, AX(), AY(), G_W2E_RECTS, 5, G_HOVER);
        }
        return;
    case 0x4C:
        if (right_release_closes(st, 7) || rect_input(st, AX(), AY(), G_W4C_RECTS, 3, G_HOVER) != 0) {
            return;
        }
        editor_hover(st, AX() + 0x130, AY() + 0xE9, G_W4C_ARROWS, 2, 0x0A);
        editor_buttons(st, AX() + 0x130, AY() + 0xE9, G_W4C_ARROWS, 2);
        editor_buttons_draw(st, AX() + 0x130, AY() + 0xE9, G_W4C_ARROWS, 2);
        return;
    case 0x33:
        if (U8(st, G_UE38DD9) == 1) {
            if (U8(st, G_RIGHT_RELEASED) != 0) {
                GAME_CALL(st, FN_NOTHING, 0);
                GAME_CALL(st, FN_NOTHING, 1);
                GAME_CALL0(st, FN_NEWGAME_NEXT_MISSION);
            }
            return;
        }
        family_select_input(st);
        return;
    case 0x35:
        if (click(st, S32(st, G_SCREEN_X) + 0x234, S32(st, G_SCREEN_Y) + 0x19A, G_MISSION_SCREEN_BUTTON, 1) != 0 ||
            click(st, S32(st, G_SCREEN_X) + 0x6E, S32(st, G_SCREEN_Y) + 0x1A0, G_MISSION_SCREEN_BUTTONS, 2) != 0) {
            return;
        }
        if ((uint8_t) GAME_CALLT0(st, G_CAMPAIGN_TABLE, FN_CAMPAIGN_STEP_HAS_CHOICE) != 0 &&
            click(st, S32(st, G_SCREEN_X) + 0x0A, S32(st, G_SCREEN_Y) + 0x19C, G_CAMPAIGN_MAP_BUTTON, 1) != 0) {
            return;
        }
        GAME_CALL0(st, FN_PLAY_BRIEFING_INPUT);
        return;
    case 0x34:
        if (U8(st, G_CHOICE) != 0 &&
            click(st, S32(st, G_SCREEN_X) + 0x244, S32(st, G_SCREEN_Y) + 0x19A, G_MISSION_SCREEN_BUTTON, 1) != 0) {
            return;
        }
        GAME_CALL0(st, FN_MISSION_CHOICE_CLICK);
        return;
    case 0x3B:
        if (click(st, S32(st, G_SCREEN_X) + 0x234, S32(st, G_SCREEN_Y) + 0x19A, G_MISSION_SCREEN_BUTTON, 1) != 0 ||
            click(st, S32(st, G_SCREEN_X) + 0x6E, S32(st, G_SCREEN_Y) + 0x1A0, G_MISSION_SCREEN_BUTTONS, 2) != 0) {
            return;
        }
        GAME_CALL0(st, FN_PLAY_BRIEFING_INPUT);
        return;
    case 0x3D: mission_menu_input(st); return;
    case 0x2F: {
        if (GAME_CALL0(st, FN_FAMILY_SCROLL_DRAG) != 0) {
            return;
        }
        int32_t n = GAME_CALL0(st, FN_FE_TWO);
        if (click(st, DX(), DY(), G_FAMILY_LIST_BUTTONS, n) != 0 ||
            rect_input(st, DX(), DY(), G_FAMILY_RECTS, 0x10, G_HOVER) != 0) {
            return;
        }
        if (U8(st, G_RIGHT_RELEASED) != 0) {
            GAME_CALL0(st, FN_FE_BACK_TO_MENU);
        }
        return;
    }
    case 0x30:
        if (rect_input(st, DX(), DY(), G_FAMILY_MENU_BUTTONS, 5, G_HOVER) != 0) {
            return;
        }
        if (U8(st, G_RIGHT_RELEASED) != 0) {
            GAME_CALL0(st, FN_FAMILY_CHANGE_CLICK);
        }
        return;
    case 0x3E:
        if (rect_input(st, DX() + 0x50, DY() + 0x40, G_W3E_RECT, 1, G_HOVER) != 0) {
            return;
        }
        /* @bin 0x004DD31E timeGetTime, compared signed with the screen's start 0x006057E8 + 10 s */
        if ((int32_t) platform_clock_ms(st) > (int32_t) (U32(st, G_RESOLUTION_ASK_TIME) + 0x2710u)) {
            GAME_CALL0(st, FN_RESOLUTION_REVERT);
        }
        return;
    case 0x36:
        GAME_CALL(st, FN_TEXT_BOX_FOCUS, 1);
        if (click(st, DX() + 0x1D0, DY() + 0xF9, G_MISSION_SCREEN_BUTTON, 1) != 0 || S32(st, G_UE20998) == 0) {
            return;
        }
        GAME_CALL0(st, FN_FLOW_CONTINUE);
        return;
    case 0x51:
        if (U8(st, G_RIGHT_RELEASED) != 0) {
            S32(st, G_UE20998) = 0;
            close_to(st, 0x36);
        }
        return;
    case 0x3C:
        if (U8(st, G_RIGHT_RELEASED) != 0 || U8(st, G_LEFT_RELEASED) != 0) {
            close_to(st, 0);
        }
        return;
    case 0x37:
        if (U8(st, G_RIGHT_RELEASED) != 0 || U8(st, G_LEFT_PRESSED) != 0) {
            close_to(st, 0x38);
        }
        return;
    case 0x38:
    case 0x39:
        if (U8(st, G_RIGHT_RELEASED) != 0 || U8(st, G_LEFT_PRESSED) != 0) {
            U8(st, G_QUIT_REQUEST) = 1; /* the quit request */
            U8(st, G_REDRAW) = 1;
        }
        return;
    case 0x3A:
        if (U8(st, G_RIGHT_RELEASED) != 0 || U8(st, G_LEFT_PRESSED) != 0) {
            close_to(st, 0);
        }
        return;
    case 0x1E: target_input(st, 0, FN_LEGION_CLICK_TARGET); return;
    case 0x49: target_input(st, 0, FN_LEGION_CLICK_TARGET_0X49); return;
    case 0x1F: target_input(st, G_TRANSPORT_MODE, FN_TRANSPORT_CLICK_TARGET); return;
    case 0x20: target_input(st, G_WARSHIP_MODE, FN_WARSHIP_CLICK_TARGET); return;
    case 0x4F: {
        if (right_release_closes(st, 7)) {
            return;
        }
        int32_t k = S32(st, CITY_BASE + CITY_EXPLAIN_PAGE + U32(st, G_PLAYER) * 0x49D8u);
        if (U8(st, G_ADVISOR_AVAILABLE + (uint32_t) k) == 0) {
            return;
        }
        hover(st, AX() + 0x20, AY() + 0x110, U32(st, G_ADVISOR_BUTTONS + (uint32_t) k * 4u), 1, k + 0x28);
        k = S32(st, CITY_BASE + CITY_EXPLAIN_PAGE + U32(st, G_PLAYER) * 0x49D8u);
        click(st, AX() + 0x20, AY() + 0x110, U32(st, G_ADVISOR_BUTTONS + (uint32_t) k * 4u), 1);
        return;
    }
    case 0x50: right_release_closes(st, 7); return;
    case 0x53:
        if (right_release_closes(st, 0) || GAME_CALL0(st, FN_HIGHSCORE_DRAG) != 0) {
            return;
        }
        click(st, DX(), DY(), G_W53_BUTTONS, 2);
        return;
    case 0x54:
        rect_input(st, 0, 0, G_MESSAGE_BOX_RECTS, S32(st, G_MESSAGE_BOX_COUNT), G_HOVER);
        return;
    case 0x55: GAME_CALL0(st, FN_CHEAT_WINDOW_INPUT); return;
    default: return;
    }
}

/* ---- port table ---- */

static int32_t run_redraw(game_state *st, const int32_t *a) { ui_redraw(st, a[0]); return 0; }
static int32_t run_frame(game_state *st, const int32_t *a) { (void) a; ui_draw_frame(st); return 0; }
static int32_t run_input(game_state *st, const int32_t *a) { (void) a; ui_input(st); return 0; }
static int32_t run_rect_highlight(game_state *st, const int32_t *a)
{
    return rect_highlight(st, a[0], a[1], (uint32_t) a[2], a[3], a[4]);
}
static int32_t run_layout_apply(game_state *st, const int32_t *a)
{
    rect_layout_apply(st, (uint32_t) a[0], (uint32_t) a[1]);
    return 0;
}

const ported_function ui_dispatch_ports[] = {
    {0x004D71C0u, "ui_redraw", 1, 0, 0, run_redraw, 0},
    {0x004D75D0u, "ui_draw_frame", 0, 0, 0, run_frame, 0},
    {0x004D8310u, "ui_input", 0, 0, 0, run_input, 0},
    {0x004D6740u, "rect_highlight", 7, 32, 0, run_rect_highlight, 0},
    {0x004D66F0u, "rect_layout_apply", 2, 0, 0, run_layout_apply, 0},
};
const int ui_dispatch_ports_count = sizeof(ui_dispatch_ports) / sizeof(ui_dispatch_ports[0]);
