/* Imhotep - the front end: the scenario / mission list, window 0x3D (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 0x3D (docs/newgame.md 2): the list object 0x00704C00 (core/newgame/flow2.c; its rows
 * 0x00704DB4, the first shown 0x00704DB8, the chosen 0x00704DBC), the list mode 0x00704DB0 (1 the
 * Cleopatra missions, 13 rows; 0 the custom maps or the editor's, 15 rows) and kind 0x00704DAC. The
 * full redraw 0x0041E5C0 draws the background, the rows (0x0041F220), the scroll bar (0x0041E1B0) and
 * the chosen scenario's description: its map's features, read from the previewed scenario
 * (0x0041F670: 0x00784xxx, the event records 0x00785D70) or, for a campaign mission, the mission
 * record 0x007009DC + 0x4C m. Per frame 0x0041F220 and the buttons of 0x0041E100 / 0x0041E130
 * (0x00425730 by the dispatcher); input (0x004D8310): the scroll bar 0x0041F4D0, the buttons, the rows
 * of 0x0041E190 (0x0041F670), the tab 0x0041E120 (0x004261D0), a right click 0x0041F650. */
#include "frontend.h"

#include <string.h>

#include "city/month.h"
#include "city/month_events.h"
#include "draw/text.h"
#include "newgame/flow.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"


int32_t fe_text_expand(game_state *st, uint32_t text, char *out, int32_t size);

static int32_t shown(game_state *st)
{
    /* the rows shown: 13 for the Cleopatra missions, else 15 (neg / sbb / and 2 / add 0xD) */
    return S32(st, G_SCENARIO_LIST_MODE) != 1 ? 15 : 13;
}

uint32_t list_buttons(game_state *st)
{
    /* @bin 0x0041E100 the list's buttons: 0x0057CCC0 (Cleopatra missions) or 0x0057CC30 */
    return S32(st, G_SCENARIO_LIST_MODE) != 1 ? G_SCENARIO_LIST_BUTTONS : G_MISSION_LIST_BUTTONS;
}

uint32_t list_tab(game_state *st)
{
    /* @bin 0x0041E120 the rectangle of the tab of the mission list */
    (void) st;
    return G_SCENARIO_LIST_TAB;
}

uint32_t list_extra_button(game_state *st)
{
    /* @bin 0x0041E130 the difficulty buttons */
    (void) st;
    return G_DIFFICULTY_BUTTONS;
}

int32_t list_has_extra(game_state *st)
{
    /* @bin 0x0041E140 the difficulty buttons are shown: not in the editor, nor on the Cleopatra
     * missions of kind 2 */
    if (S32(st, G_EDITOR) == 1 || (S32(st, G_SCENARIO_LIST_MODE) == 1 && S32(st, G_SCENARIO_KIND) == 2)) {
        return 0;
    }
    return 1;
}

int32_t list_button_count(game_state *st)
{
    /* @bin 0x0041E170 3 buttons when the chosen row is a heading (kind 2), else 4 */
    return (list_row_kind(st, G_LIST_OBJECT, S32(st, G_SCENARIO_LIST_ROW)) != 2) + 3;
}

uint32_t list_rows_table(game_state *st)
{
    /* @bin 0x0041E190 the rows' rectangles: 0x0057CF60 (Cleopatra missions) or 0x0057CDB8 */
    return S32(st, G_SCENARIO_LIST_MODE) != 1 ? G_SCENARIO_LIST_ROWS : G_MISSION_LIST_ROWS;
}

int32_t list_rows_count(game_state *st)
{
    /* @bin 0x0041F210 the rows' rectangles: 0x11 or 0x0F */
    return S32(st, G_SCENARIO_LIST_MODE) != 1 ? 0x0F : 0x11;
}

void list_thumb(game_state *st)
{
    /* @bin 0x0041E1B0 the scroll bar's thumb when there are more rows than shown: on a bar of 16
     * shown - 0x4C pixels at (0x121, 0x115 or 0xF5) */
    int32_t n = shown(st);
    int32_t range = S32(st, G_SCENARIO_ROWS) - n;
    if (range <= 0) {
        return;
    }
    int32_t top = S32(st, G_SCENARIO_LIST_MODE) != 1 ? 0xDC : 0xFC;
    int32_t first = S32(st, G_SCENARIO_FIRST);
    int32_t pct;
    if (first <= 0) {
        pct = 0;
    } else if (n + first < S32(st, G_SCENARIO_ROWS)) {
        pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, first, range);
    } else {
        pct = 100;
    }
    int32_t d = GAME_CALL(st, FN_CALC_PERCENTAGE, shown(st) * 16 - 0x4C, pct);
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_SCROLL_IMAGES) + 0x27, S32(st, G_LAYOUT_DX) + 0x121,
               S32(st, G_LAYOUT_DY) + d + top + 0x19);
}

/* ---- the scenario's features (event records) ---- */

int32_t event_true(game_state *st, uint32_t e)
{
    /* @bin 0x004495F0 (thiscall on an event) mov al, 1 */
    (void) st;
    (void) e;
    return 1;
}

int32_t event_is_request(game_state *st, uint32_t e)
{
    /* @bin 0x004496B0 (thiscall) the event's type +2 is 1 or 0x17 */
    uint8_t t = U8(st, e + 2);
    return t == 1 || t == 0x17;
}

int32_t event_is_small_request(game_state *st, uint32_t e)
{
    /* @bin 0x00449600 (thiscall) type 1 with +0x60 in 1..2 */
    if (U8(st, e + 2) != 1) {
        return 0;
    }
    uint8_t v = U8(st, e + 0x60);
    return v != 0 && v <= 2;
}

static uint32_t event_record(game_state *st, int32_t n)
{
    if (n < 0 || n >= S32(st, G_EVENTS)) {
        return 0;
    }
    return G_KINGDOM_EVENTS + 0x7Cu * (uint32_t) n;
}

static void follow(game_state *st, uint32_t e, int32_t link, int32_t level)
{
    int32_t n = S16(st, e + (uint32_t) link);
    if (!(uint8_t) event_link_active(st, e, n)) {
        return;
    }
    uint32_t next = event_record(st, S16(st, e + (uint32_t) link));
    void event_mark_chain(game_state *st, uint32_t e, int32_t level);
    event_mark_chain(st, next, level);
}

void event_mark_chain(game_state *st, uint32_t e, int32_t level)
{
    /* @bin 0x0041E270 the event and those it leads to (+0x24, +0x26, +0x52, and +0x54 as a loop),
     * up to 7 levels: +0x6E := 1 */
    if (level >= 7) {
        return;
    }
    int32_t next_level = level + 1;
    for (;;) {
        if (e == 0) {
            state_unported(st, FN_EVENT_MARK_CHAIN); /* the original writes at address 0x6E */
            return;
        }
        U8(st, e + 0x6E) = 1;
        if ((uint8_t) event_true(st, e)) {
            follow(st, e, 0x24, next_level);
        }
        if ((uint8_t) event_is_request(st, e)) {
            follow(st, e, 0x26, next_level);
        }
        if ((uint8_t) event_is_request(st, e)) {
            follow(st, e, 0x52, next_level);
        }
        if (!(uint8_t) event_is_small_request(st, e)) {
            return;
        }
        int32_t n = S16(st, e + 0x54);
        if (!(uint8_t) event_link_active(st, e, n)) {
            return;
        }
        e = event_record(st, S16(st, e + 0x54));
        next_level++;
        if (next_level >= 8) {
            return;
        }
    }
}

int32_t events_unchained(game_state *st)
{
    /* @bin 0x0041E3B0 the marks +0x6E cleared, the chains of the events with +0x28 bit 4 marked;
     * the number of events (0x0044BB90 on 0x00785D70) left unmarked */
    int32_t count = S32(st, G_EVENTS);
    for (int32_t i = 0; i < count; i++) {
        uint32_t e = event_record(st, i);
        if (e == 0) {
            state_unported(st, FN_EVENTS_UNCHAINED);
            return 0;
        }
        U8(st, e + 0x6E) = 0;
    }
    for (int32_t i = 0; i < count; i++) {
        uint32_t e = event_record(st, i);
        if (e == 0) {
            state_unported(st, FN_EVENTS_UNCHAINED);
            return 0;
        }
        if ((U32(st, e + 0x28) >> 4) & 1) {
            event_mark_chain(st, e, 1);
        }
    }
    int32_t n = 0;
    for (uint32_t e = events_next(st, G_EVENTS, 0, 0); e != 0; e = events_next(st, G_EVENTS, e, 0)) {
        if (U8(st, e + 0x6E) == 0) {
            n++;
        }
    }
    return n;
}

int32_t scenario_rating(game_state *st)
{
    /* @bin 0x0041E470 the scenario's difficulty points: the goals (culture + 2 (prosperity + 10 x
     * monuments + kingdom), 0x00784C24..0x00784C30), the population goal (above 500: 20 / 50 / 80 /
     * 100), the unchained events (40 / 60 / 80 / 100), the requests of type 1 (indexes count - 1 down
     * to 1; up to 5: 20, then 40 / 60 / 80 / 100) and the invasions' strengths (0x007848E2, 5 i16:
     * 1 one, 2 two points; 20 / 50 / 80 / 100) */
    int32_t r = S32(st, G_GOAL_KINGDOM) + (S32(st, G_GOAL_PROSPERITY) + S32(st, G_GOAL_MONUMENT) * 10 + S32(st, G_GOAL_CULTURE)) * 2;
    if (S32(st, G_GOAL_POPULATION_ON) != 0) {
        int32_t pop = S32(st, G_GOAL_POPULATION);
        if (pop > 500) {
            r += pop <= 2000 ? 0x14 : pop <= 4000 ? 0x32 : pop <= 8000 ? 0x50 : 0x64;
        }
    }
    int32_t free_events = events_unchained(st);
    if (free_events > 0) {
        r += free_events <= 2 ? 0x28 : free_events <= 4 ? 0x3C : free_events <= 10 ? 0x50 : 0x64;
    }
    int32_t count = S32(st, G_EVENTS);
    int32_t requests = 0;
    if (count > 0) {
        for (int32_t i = count; i > 0; i--) {
            uint32_t e = G_KINGDOM_EVENTS + 0x7Cu * (uint32_t) i;
            if (i < count && U8(st, e + 2) == 1) {
                requests++;
            }
        }
        r += requests <= 5 ? 0x14 : requests <= 10 ? 0x28 : requests <= 20 ? 0x3C : requests <= 30 ? 0x50 : 0x64;
    } else {
        r += 0x14;
    }
    int32_t inv = 0;
    for (uint32_t a = G_GOD_MODE; a < G_GOD_MODE + 0x0Au; a += 2) {
        int16_t v = S16(st, a);
        if (v == 1) {
            inv++;
        } else if (v == 2) {
            inv += 2;
        }
    }
    if (inv > 0) {
        r += inv <= 2 ? 0x14 : inv <= 5 ? 0x32 : inv <= 8 ? 0x50 : 0x64;
    }
    return r;
}

/* ---- the rows ---- */

void list_rows_draw(game_state *st)
{
    /* @bin 0x0041F220 the list's inner panel (16 x shown + 1), the rows' names (lit when hovered or
     * chosen with no hover; headings, kind 2, greyed), for the Cleopatra missions the tabs (the
     * first lit) and, when the list is empty, its note (group 0x126, 0x28), and the tab's frame;
     * 0x0051F780 twice */
    int32_t n = S32(st, G_SCENARIO_LIST_MODE) == 1 ? 0x0D : 0x0F;
    int32_t top = S32(st, G_SCENARIO_LIST_MODE) == 1 ? 0xFC : 0xDC;
    GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_LAYOUT_DX) + 0x0F, S32(st, G_LAYOUT_DY) + top - 0x0F, 0x10, n + 1);
    int32_t y = 0x10;
    for (int32_t k = 1; k <= shown(st); k++, y += 0x10) {
        uint32_t name = list_row_name(st, G_LIST_OBJECT, S32(st, G_SCENARIO_FIRST) - 1 + k);
        if (name == 0) {
            continue;
        }
        int32_t font = S32(st, G_FONT_ROW);
        if (S32(st, G_HOVER) == k) {
            font = S32(st, G_FONT_9C);
        } else if (S32(st, G_HOVER) == 0 && S32(st, G_SCENARIO_FIRST) + k - 1 == S32(st, G_SCENARIO_LIST_ROW)) {
            font = S32(st, G_FONT_9C);
        } else if (list_row_kind(st, G_LIST_OBJECT, S32(st, G_SCENARIO_FIRST) - 1 + k) == 2) {
            font = S32(st, G_UF56FA4);
        }
        int32_t off = S32(st, G_SCENARIO_LIST_MODE) != 1 ? -0x20 : 0;
        GAME_CALL(st, FN_TEXT_DRAW, (int32_t) name, S32(st, G_LAYOUT_DX) + 0x18, S32(st, G_LAYOUT_DY) + y + off + 0xFC - 0x10, font, 0);
    }
    if (S32(st, G_SCENARIO_LIST_MODE) == 1) {
        static const int32_t labels[2] = {0x26, 0x27};
        for (int32_t t = 0; t < 2; t++) {
            int32_t idx = shown(st) + t;
            int32_t lit = S32(st, G_HOVER) == idx + 1;
            if (t == 0) {
                lit = 1;
            }
            uint32_t rect = list_rows_table(st) + 0x1Cu * (uint32_t) idx;
            GAME_CALL(st, FN_LAYOUT_PANEL, 0, (int32_t) rect, lit, 0, 0);
            rect = list_rows_table(st) + 0x1Cu * (uint32_t) idx;
            GAME_CALL(st, FN_LAYOUT_TEXT_GROUP_CENTRED, 0x126, labels[t], (int32_t) rect, S32(st, G_FONT_LARGE), 0, 4, 7);
        }
        if (S32(st, G_SCENARIO_LIST_COUNT) == 0) {
            int32_t off = S32(st, G_SCENARIO_LIST_MODE) != 1 ? -0x20 : 0;
            GAME_CALL(st, FN_TEXT_GROUP_DRAW_LINES, 0x126, 0x28, S32(st, G_LAYOUT_DX) + 0x18, S32(st, G_LAYOUT_DY) + off + 0xFC, 0xEB,
                      S32(st, G_FONT_ROW), 0);
        }
    }
    int32_t lit = S32(st, G_HOVER_2) != 0;
    if (S32(st, G_SCENARIO_LIST_MODE) == 1) {
        GAME_CALL(st, FN_LAYOUT_PANEL, 0, (int32_t) list_tab(st), lit, 0x15E, 0x19A);
    }
    GAME_CALL0(st, FN_NOTHING);
    GAME_CALL0(st, FN_NOTHING);
}

int32_t list_scroll_drag(game_state *st)
{
    /* @bin 0x0041F4D0 the left button held on the scroll bar (x 0x118..0x138, y from top + 0x19 over
     * 16 shown - 0x4C + 0x19 pixels) of a list longer than shown: the first row by the place; a
     * full redraw; 1 when it took the input */
    int32_t n = shown(st);
    if (S32(st, G_SCENARIO_ROWS) <= n) {
        return 0;
    }
    if (U8(st, G_LEFT_HELD) == 0) {
        return 0;
    }
    int32_t range = S32(st, G_SCENARIO_ROWS) - n;
    int32_t bar = n * 16 - 0x4C;
    int32_t top = S32(st, G_SCENARIO_LIST_MODE) != 1 ? 0xDC : 0xFC;
    int32_t mx = S32(st, G_MOUSE_X), dx = S32(st, G_LAYOUT_DX);
    if (mx < dx + 0x118 || mx > dx + 0x138) {
        return 0;
    }
    int32_t dy = S32(st, G_LAYOUT_DY), my = S32(st, G_MOUSE_Y);
    if (my < dy + top + 0x19 || my > dy + top + bar + 0x32) {
        return 0;
    }
    int32_t pos = my - dy - top - 0x19;
    if (pos > bar) {
        pos = bar;
    }
    int32_t pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, pos, bar);
    S32(st, G_SCENARIO_FIRST) = GAME_CALL(st, FN_CALC_PERCENTAGE, range, pct);
    U8(st, G_REDRAW) = 1;
    return 1;
}

/* ---- the description ---- */

static void group_text(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t font)
{
    GAME_CALL(st, FN_TEXT_GROUP_DRAW, group, index, x, y, font, 0);
}

static void number(game_state *st, int32_t value, int32_t x, int32_t y, int32_t font)
{
    GAME_CALL(st, FN_NUMBER_DRAW, value, 0x40, (int32_t) G_NUMBER_SUFFIX, x, y, font, 0);
}

static void goal_line(game_state *st, int32_t value, int32_t label, int32_t x, int32_t y, int32_t font)
{
    /* a goal: the number at x + 0x5A, its label after it (the text cursor 0x00E20778 from 0) */
    S32(st, G_TEXT_X) = 0;
    number(st, value, x + 0x5A, y, font);
    group_text(st, 0x2C, label, S32(st, G_TEXT_X) + x + 0x5A, y, font);
}

static void record_line(game_state *st, int32_t value, int32_t label, int32_t x, int32_t y, int32_t font)
{
    /* a mission record's value: "label value / goal" (group 0x12A) */
    S32(st, G_TEXT_X) = 0;
    GAME_CALL(st, FN_TEXT_SEEK, 0x12A, label);
    GAME_CALL(st, FN_NUMBER_DRAW_BETWEEN, value, S32(st, G_TEXT_CURSOR), (int32_t) G_GOAL_SEPARATOR, S32(st, G_TEXT_X) + x + 0x19, y, 0x10E, font, 0);
}

void scenario_list_draw(game_state *st)
{
    /* @bin 0x0041E5C0 */
    int32_t font = S32(st, G_FONT_ROW);
    int32_t dx0 = S32(st, G_LAYOUT_DX), dy0 = S32(st, G_LAYOUT_DY);
    int32_t w = S32(st, G_SCREEN_W), h = S32(st, G_SCREEN_H);
    S32(st, G_PIC_X) = (0x400 - w) / 2;
    S32(st, G_PIC_Y) = (0x300 - h) / 2;
    S32(st, G_PIC_W) = w;
    U8(st, G_RIGHT_RELEASED) = 0;
    U8(st, G_LEFT_RELEASED) = 0;
    int32_t x140 = dx0 + 0x140;
    int32_t y14 = dy0 + 0x14;
    S32(st, G_PIC_H) = h;
    if (U8(st, G_REDRAW) == 1) {
        int32_t pic, px, py;
        if (S32(st, G_EDITOR) == 1) {
            pic = S32(st, G_EDITOR_LIST_IMAGE);
            px = 0;
            py = 0;
        } else {
            pic = S32(st, G_SCENARIO_LIST_MODE) == 0 ? S32(st, G_SCENARIO_LIST_IMAGE) : S32(st, G_PERIOD_SCREEN_IMAGE);
            px = dx0 - 0xC0;
            py = dy0 - 0x90;
        }
        GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, pic, px, py);
    }
    U8(st, G_REDRAW_SIDEBAR) = 1;
    {
        int32_t n = S32(st, G_SCENARIO_LIST_MODE) == 1 ? 0x0D : 0x0F;
        int32_t top = S32(st, G_SCENARIO_LIST_MODE) == 1 ? 0xFC : 0xDC;
        GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_LAYOUT_DX) + 0x11E, S32(st, G_LAYOUT_DY) + top + 0x16, 2, n - 3);
    }
    list_rows_draw(st);
    list_thumb(st);
    /* the period's picture (i16 0x00784B5E: Pharaoh's below 0x13, else Cleopatra's) */
    int32_t py = S32(st, G_SCENARIO_LIST_MODE) == 1 ? dy0 + 0x38 : dy0 + 0x24;
    int16_t period = S16(st, G_SCENARIO_PICTURE);
    int32_t pic = period <= 0x12 ? period + S32(st, G_PERIOD_IMAGES) : period + S32(st, G_PERIOD_IMAGES_B) - 0x13;
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, pic, dx0 + 0x4E, py);
    if (S32(st, G_SCENARIO_LIST_MODE) == 1) {
        GAME_CALL(st, FN_TEXT_SEEK, 0x125, 6);
        uint32_t text = U32(st, G_TEXT_CURSOR);
        uint32_t saved = U32(st, G_U784F0C);
        char buf[201];
        memset(buf, 0, sizeof(buf));
        U32(st, G_U784F0C) = 0;
        fe_text_expand(st, text, buf, 200);
        U32(st, G_U784F0C) = saved;
        text_draw_centred_host(st, buf, S32(st, G_LAYOUT_DX) + 0x14, S32(st, G_LAYOUT_DY) + 0x11, 600, S32(st, G_FONT_TITLE), 0);
    }
    int32_t x = x140 + 0x0F;
    GAME_CALL(st, FN_TEXT_DRAW_CENTRED, G_MAP_NAME, x, y14 + 0x2D, 0x104, S32(st, G_FONT_ROW), 0);
    GAME_CALL(st, FN_TEXT_DRAW_CENTRED, G_SCENARIO_TITLE, x, y14 + 0x41, 0x104, S32(st, G_FONT_9C), 0);
    GAME_CALL(st, FN_TEXT_MONEY_DRAW, 1, (int32_t) S16(st, G_START_YEAR), x140 + 0x5A, y14 + 0x55, S32(st, G_FONT_ROW), 0);
    if (S32(st, G_SCENARIO_KIND) == 3) {
        /* a custom map: its climate, its start, its unchained events, its difficulty */
        GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x2C, (int32_t) (U32(st, G_U784CB0) & 0xFF) + 0x4D, x, y14 + 0x69, 0x104, font, 0);
        int32_t idx;
        switch (S32(st, G_SCENARIO_MAP_WIDTH)) {
        case 0x38: idx = 0x79; break;
        case 0x54: idx = 0x7A; break;
        case 0x70: idx = 0x7B; break;
        case 0x8C: idx = 0x7C; break;
        case 0xAA: idx = 0x7D; break;
        default: idx = 0x7E; break;
        }
        GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x2C, idx, x, y14 + 0x7D, 0x104, font, 0);
        int32_t ev = events_unchained(st);
        idx = ev <= 0 ? 0x70 : ev <= 2 ? 0x71 : ev <= 4 ? 0x72 : ev <= 10 ? 0x73 : 0x74;
        GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x2C, idx, x, y14 + 0x91, 0x104, font, 0);
        int32_t r = scenario_rating(st);
        int32_t level;
        if (r <= 0x64) {
            level = 0;
        } else if (r <= 0xC8) {
            level = 1;
        } else if (r <= 0x190) {
            level = 2;
        } else if (r <= 0x258) {
            level = 3;
        } else if (r <= 0x384) {
            level = 4;
        } else if (r <= 0x514) {
            level = 5;
        } else if (r <= 0x6A4) {
            level = 6;
        } else if (r <= 0x834) {
            level = 7;
        } else {
            level = (r > 0x9C4) + 8;
        }
        GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x20, level + 0x16, x, y14 + 0xA5, 0x104, font, 0);
        if (S16(st, G_U784B60) != 0) {
            GAME_CALL(st, FN_TEXT_GROUP_DRAW_LINES, 0x91, 0, x140 + 0x19, y14 + 0xCD, 0x104, font, 0);
        } else {
            /* the goals */
            GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x2C, 0x7F, x, y14 + 0xC5, 0x104, S32(st, G_FONT_9C), 0);
            if (U8(st, G_GOAL_CULTURE_ON) != 0) {
                goal_line(st, S32(st, G_GOAL_CULTURE), 0x81, x140, y14 + 0xE1, font);
            }
            if (U8(st, G_GOAL_PROSPERITY_ON) != 0) {
                goal_line(st, S32(st, G_GOAL_PROSPERITY), 0x82, x140, y14 + 0xF1, font);
            }
            if (U8(st, G_GOAL_KINGDOM_ON) != 0) {
                goal_line(st, S32(st, G_GOAL_KINGDOM), 0x84, x140, y14 + 0x101, font);
            }
            if (S32(st, G_GOAL_POPULATION_ON) != 0) {
                goal_line(st, S32(st, G_GOAL_POPULATION), 0x85, x140, y14 + 0x111, font);
            }
            if (S32(st, G_GOAL_HOUSES) != 0) {
                S32(st, G_TEXT_X) = 0;
                number(st, S32(st, G_GOAL_HOUSES), x140 + 0x5A, y14 + 0x121, font);
                int32_t plural = S32(st, G_GOAL_HOUSES) > 1 ? 0x14 : 0;
                group_text(st, 0x1D, S32(st, G_GOAL_HOUSE_LEVEL) + plural, S32(st, G_TEXT_X) + x140 + 0x5A, y14 + 0x121, font);
            }
            if (S32(st, G_GOAL_DEADLINE) != 0) {
                goal_line(st, S32(st, G_SURVIVE_YEARS), 0x87, x140, y14 + 0x131, font);
            }
            if (S32(st, G_GOAL_TIME) != 0) {
                goal_line(st, S32(st, G_TIME_LIMIT_YEARS), 0x86, x140, y14 + 0x131, font);
            }
            int32_t ly = y14 + 0x141;
            for (uint32_t a = G_MONUMENT_SLOTS; a < G_MONUMENT_SLOTS + 6u; a += 2, ly += 0x10) {
                S32(st, G_TEXT_X) = 0;
                int16_t v = S16(st, a);
                if (v != 0) {
                    GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0xC6, (int32_t) v, x140 + 0x1E, ly, 0x104, font, 0);
                }
            }
        }
        if (S32(st, G_EDITOR) == 0) {
            int32_t ly = y14 + 0x174;
            S32(st, G_TEXT_X) = 0;
            group_text(st, 0x2C, 0xD8, x140 + 0x5A, ly, font);
            group_text(st, 0x99, S32(st, G_DIFFICULTY) + 1, S32(st, G_TEXT_X) + x140 + 0x5A, ly, font);
        }
    } else {
        /* a campaign mission: its description, or its record when it was played */
        int32_t m = (int8_t) S8(st, G_MISSION);
        uint32_t rec = G_MISSION_RECORDS + 0x4Cu * (uint32_t) m;
        if (S32(st, rec) == 0) {
            GAME_CALL(st, FN_TEXT_GROUP_DRAW_LINES, 0x131, 0, x140 + 0x19, y14 + 0x69, 0x104, font, 0);
        } else {
            GAME_CALL(st, FN_TEXT_GROUP_DRAW_LINES, 0x129, m, x140 + 0x19, y14 + 0x69, 0x10E, font, 0);
            m = (int8_t) S8(st, G_MISSION);
            rec = G_MISSION_RECORDS + 0x4Cu * (uint32_t) m;
            if (U8(st, G_GOAL_CULTURE_ON) != 0) {
                record_line(st, S32(st, rec + 0x28), 0, x140, y14 + 0x109, font);
            }
            if (U8(st, G_GOAL_PROSPERITY_ON) != 0) {
                record_line(st, S32(st, rec + 0x2C), 1, x140, y14 + 0x119, font);
            }
            if (U8(st, G_GOAL_KINGDOM_ON) != 0) {
                record_line(st, S32(st, rec + 0x30), 3, x140, y14 + 0x129, font);
            }
            if (S32(st, G_GOAL_POPULATION_ON) != 0) {
                record_line(st, S32(st, rec + 0x34), 4, x140, y14 + 0x139, font);
            }
            record_line(st, S32(st, rec + 0x38), 5, x140, y14 + 0x149, font);
            S32(st, G_TEXT_X) = 0;
            GAME_CALL(st, FN_TEXT_SEEK, 0x12A, 7);
            GAME_CALL(st, FN_TEXT_SEEK2, 0x99, S32(st, rec + 0x40) + 1);
            GAME_CALL(st, FN_TEXT_PAIR_DRAW_CENTRED, S32(st, G_TEXT_CURSOR), S32(st, G_TEXT_PTR2), S32(st, G_TEXT_X) + x140 + 0x0F, y14 + 0x159,
                      0x10E, font, 0);
            S32(st, G_TEXT_X) = 0;
            GAME_CALL(st, FN_TEXT_SEEK, 0x12A, 6);
            GAME_CALL(st, FN_TEXT_SEEK2, 0x12A, 9);
            GAME_CALL(st, FN_NUMBER_DRAW_BETWEEN, S32(st, rec + 0x3C) / 12, S32(st, G_TEXT_CURSOR), S32(st, G_TEXT_PTR2),
                      S32(st, G_TEXT_X) + x140 + 0x19, y14 + 0x169, 0x10E, font, 0);
            S32(st, G_TEXT_X) = 0;
            GAME_CALL(st, FN_TEXT_SEEK, 0x12A, 8);
            GAME_CALL(st, FN_NUMBER_DRAW_BETWEEN, S32(st, rec), S32(st, G_TEXT_CURSOR), (int32_t) G_GOAL_SEPARATOR, S32(st, G_TEXT_X) + x140 + 0x19,
                      y14 + 0x179, 0x10E, S32(st, G_FONT_9C), 0);
        }
    }
    /* the buttons' labels */
    int32_t ly = y14 + 0x1AA;
    if (list_row_kind(st, G_LIST_OBJECT, S32(st, G_SCENARIO_LIST_ROW)) == 2) {
        group_text(st, 0x2C, 0x15, x140 + 0x20, ly, S32(st, G_FONT_ROW));
        return;
    }
    if (S32(st, G_EDITOR) != 0) {
        group_text(st, 0x2C, 0xD6, x140 + 0xAA, ly, S32(st, G_FONT_LARGE));
        return;
    }
    if (S32(st, G_SCENARIO_LIST_MODE) == 1) {
        group_text(st, 0x2C, 0xD9, x140 + 0x3C, ly, S32(st, G_FONT_LARGE));
        group_text(st, 0x2C, 0xD7, x140 + 0x8C, ly, S32(st, G_FONT_LARGE));
        GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x2C, (S32(st, G_SCENARIO_KIND) != 3) + 0xDC, S32(st, G_LAYOUT_DX) + 0x162, S32(st, G_LAYOUT_DY) + 0x19E, 0xFA,
                  S32(st, G_FONT_ROW), 0);
        return;
    }
    group_text(st, 0x2C, 0x88, x140 + 0xB9, ly, S32(st, G_FONT_LARGE));
}

static int32_t run_ret(game_state *st, const int32_t *a, uint32_t (*f)(game_state *)) { (void) a; return (int32_t) f(st); }
static int32_t run_buttons(game_state *st, const int32_t *a) { return run_ret(st, a, list_buttons); }
static int32_t run_tab(game_state *st, const int32_t *a) { return run_ret(st, a, list_tab); }
static int32_t run_extra(game_state *st, const int32_t *a) { return run_ret(st, a, list_extra_button); }
static int32_t run_rows_table(game_state *st, const int32_t *a) { return run_ret(st, a, list_rows_table); }
static int32_t run_has_extra(game_state *st, const int32_t *a) { (void) a; return list_has_extra(st); }
static int32_t run_count(game_state *st, const int32_t *a) { (void) a; return list_button_count(st); }
static int32_t run_rows_count(game_state *st, const int32_t *a) { (void) a; return list_rows_count(st); }
static int32_t run_thumb(game_state *st, const int32_t *a) { (void) a; list_thumb(st); return 0; }
static int32_t run_true(game_state *st, const int32_t *a) { return event_true(st, (uint32_t) a[PORT_THIS_ARG]); }
static int32_t run_request(game_state *st, const int32_t *a) { return event_is_request(st, (uint32_t) a[PORT_THIS_ARG]); }
static int32_t run_small(game_state *st, const int32_t *a) { return event_is_small_request(st, (uint32_t) a[PORT_THIS_ARG]); }
static int32_t run_chain(game_state *st, const int32_t *a) { event_mark_chain(st, (uint32_t) a[0], a[1]); return 0; }
static int32_t run_unchained(game_state *st, const int32_t *a) { (void) a; return events_unchained(st); }
static int32_t run_rating(game_state *st, const int32_t *a) { (void) a; return scenario_rating(st); }
static int32_t run_rows(game_state *st, const int32_t *a) { (void) a; list_rows_draw(st); return 0; }
static int32_t run_drag(game_state *st, const int32_t *a) { (void) a; return list_scroll_drag(st); }
static int32_t run_draw(game_state *st, const int32_t *a) { (void) a; scenario_list_draw(st); return 0; }

const ported_function frontend_scenario_ports[] = {
    {0x0041E100u, "list_buttons", 0, 32, 0, run_buttons, 0},
    {0x0041E120u, "list_tab", 0, 32, 0, run_tab, 0},
    {0x0041E130u, "list_extra_button", 0, 32, 0, run_extra, 0},
    {0x0041E140u, "list_has_extra", 0, 8, 0, run_has_extra, 0},
    {0x0041E170u, "list_button_count", 0, 8, 0, run_count, 0},
    {0x0041E190u, "list_rows_table", 0, 32, 0, run_rows_table, 0},
    {0x0041F210u, "list_rows_count", 0, 32, 0, run_rows_count, 0},
    {0x0041E1B0u, "list_thumb", 0, 0, 0, run_thumb, 0},
    {0x004495F0u, "event_true", 0, 8, 0, run_true, 0},
    {0x004496B0u, "event_is_request", 0, 8, 0, run_request, 0},
    {0x00449600u, "event_is_small_request", 0, 8, 0, run_small, 0},
    {0x0041E270u, "event_mark_chain", 2, 0, 0, run_chain, 0},
    {0x0041E3B0u, "events_unchained", 0, 32, 0, run_unchained, 0},
    {0x0041E470u, "scenario_rating", 0, 32, 0, run_rating, 0},
    {0x0041F220u, "list_rows_draw", 0, 0, 0, run_rows, 0},
    {0x0041F4D0u, "list_scroll_drag", 0, 32, 0, run_drag, 0},
    {0x0041E5C0u, "scenario_list_draw", 0, 0, 0, run_draw, 0},
};
const int frontend_scenario_ports_count = sizeof(frontend_scenario_ports) / sizeof(frontend_scenario_ports[0]);
