/* Imhotep - the number pad (window 0x0E) and the pop-up list (windows 0x0F, 0x48) (roadmap 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 0x0E: a value typed with a pad of digits (the scenario editor's fields): per frame
 * 0x005309B0(0x00605974, 0x00605970); input (0x004D8310) 0x004261D0 on 0x005C4208 (0x004035A0 a digit,
 * 0x004035F0 OK, 0x00403770 cancel), a right click back to 0x00605980. The value 0x0060597C, its
 * digits 0x00605978, its type 0x0060596C (1 byte at [0x00605964], 2 i16 at [0x00605960], 4 i32 at
 * [0x0060595C]). Windows 0x0F / 0x48: a list of choices drawn and taken by 0x00530D70(0x00605954,
 * 0x0060594C, 0x00605950) on the rectangles 0x00704DC8 (0x00605948 of them). */
#include "frontend.h"

#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"


void numpad_draw(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x005309B0 the panel (8 x 14) around (x, y), the value's box (96 x 30 black) and the value
     * (red, centred) once a digit is typed; the keys 1..9, 0 (25 x 25 rectangles with their digit,
     * colour 0x2BF when hovered, else 0), OK (0x37 wide, group 0x2C, 0x10) and cancel (0x55, 0x11) */
    GAME_CALL(st, FN_PANEL_DRAW, x - 0x10, y - 0x10, 8, 0x0E);
    GAME_CALL(st, FN_RECT_FILL, x, y, 0x60, 0x1E, 0);
    if (S32(st, G_DIGITS) > 0) {
        GAME_CALL(st, FN_NUMBER_DRAW_CENTRED, S32(st, G_NUMPAD_VALUE), 0x40, G_NUMBER_SUFFIX, x - 8, y + 3, 0x60, S32(st, G_FONT_TITLE), 0xF800);
    }
    int32_t cx[3] = {x + 5, x + 0x23, x + 0x41};
    /* @bin 0x005F8A00 .. 0x005F89DC the keys' strings "1" .. "9", "0" */
    for (int32_t k = 1; k <= 10; k++) {
        int32_t kx = cx[(k - 1) % 3];
        int32_t ky = y + 0x23 + 0x1E * ((k - 1) / 3);
        int32_t c = S32(st, G_HOVER) == k ? 0x2BF : 0;
        GAME_CALL(st, FN_RECT_DRAW, kx, ky, 0x19, 0x19, c);
        GAME_CALL(st, FN_TEXT_DRAW_CENTRED, (int32_t) (G_STR_KEY_1 - 4u * (uint32_t) (k - 1)), kx, ky, 0x19, S32(st, G_FONT_TITLE), c);
    }
    int32_t c = S32(st, G_HOVER) == 11 ? 0x2BF : 0;
    GAME_CALL(st, FN_RECT_DRAW, cx[1], y + 0x7D, 0x37, 0x19, c);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x2C, 0x10, cx[1], y + 0x83, 0x37, S32(st, G_FONT_LARGE), c, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    c = S32(st, G_HOVER) == 12 ? 0x2BF : 0;
    GAME_CALL(st, FN_RECT_DRAW, cx[0], y + 0x9B, 0x55, 0x19, c);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x2C, 0x11, cx[0], y + 0xA1, 0x55, S32(st, G_FONT_LARGE), c, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

void numpad_digit(game_state *st)
{
    /* @bin 0x004035A0 a digit (0x00D3B198) appended while fewer than 3 (bytes) or 5 digits; 0x0047D350(4) */
    if (S32(st, G_DIGITS) >= (S32(st, G_NUMPAD_TYPE) != 1 ? 5 : 3)) {
        return;
    }
    U8(st, G_REDRAW) = 1;
    S32(st, G_NUMPAD_VALUE) = (int32_t) ((uint32_t) S32(st, G_PARAM1) + (uint32_t) S32(st, G_NUMPAD_VALUE) * 10u);
    S32(st, G_DIGITS) = S32(st, G_DIGITS) + 1;
    GAME_CALL(st, FN_RETURN_ZERO, 4);
}

void numpad_ok(game_state *st)
{
    /* @bin 0x004035F0 the value into its field (by the type; a field that was negative stays
     * negative), then: 0x00605968 = 1 the map's period 0x00605A14 - 1 (0..0x21: 0x00BCA2B8, 0x004DE160,
     * the camera at 0x6D, 0xDA), 2 0x00444270(0x00605A10 - 1); the callback 0x00605A4C (cleared); back
     * to 0x00605980 */
    int32_t type = S32(st, G_NUMPAD_TYPE);
    uint32_t p16 = U32(st, G_NUMPAD_FIELD16), p32 = U32(st, G_NUMPAD_FIELD32);
    int32_t sign = 1;
    int stored = 0;
    if (type == 2 && S16(st, p16) < 0) {
        sign = -1;
        S16(st, p16) = (int16_t) S32(st, G_NUMPAD_VALUE);
        stored = 1;
    } else if (type == 4 && S32(st, p32) < 0) {
        sign = -1;
        S32(st, p32) = S32(st, G_NUMPAD_VALUE);
        stored = 1;
    }
    if (!stored) {
        if (type == 1) {
            P8(st, U32(st, G_NUMPAD_FIELD8)) = (uint8_t) S32(st, G_NUMPAD_VALUE);
        } else if (type == 2) {
            S16(st, p16) = (int16_t) S32(st, G_NUMPAD_VALUE);
        } else if (type == 4) {
            S32(st, p32) = S32(st, G_NUMPAD_VALUE);
        }
    }
    type = S32(st, G_NUMPAD_TYPE);
    if (type == 1) {
        uint32_t p8 = U32(st, G_NUMPAD_FIELD8);
        uint8_t v = P8(st, p8);
        if (v == 0 || sign != -1) {
            goto after;
        }
        P8(st, p8) = (uint8_t) -v;
        type = S32(st, G_NUMPAD_TYPE);
    }
    if (type == 2) {
        p16 = U32(st, G_NUMPAD_FIELD16);
        int16_t v = S16(st, p16);
        if ((v < 0 && sign == 1) || (v > 0 && sign == -1)) {
            S16(st, p16) = (int16_t) -v;
            type = S32(st, G_NUMPAD_TYPE);
        } else {
            goto after;
        }
    }
    if (type == 4) {
        p32 = U32(st, G_NUMPAD_FIELD32);
        int32_t v = S32(st, p32);
        if ((v < 0 && sign == 1) || (v > 0 && sign == -1)) {
            S32(st, p32) = (int32_t) (0u - (uint32_t) v);
        }
    }
after:
    if (S32(st, G_NUMPAD_ACTION) == 1) {
        int32_t v = S32(st, G_NUMPAD_PERIOD) - 1;
        S32(st, G_NUMPAD_PERIOD) = v;
        if (v < 0 || v > 0x21) {
            goto callback;
        }
        S32(st, G_MAP_EDITOR_SETTINGS) = v;
        GAME_CALL(st, 0x004DE160u, v);
        S32(st, G_CAMERA_X) = 0x6D;
        S32(st, G_CAMERA_Y) = 0xDA;
    }
    if (S32(st, G_NUMPAD_ACTION) == 2) {
        GAME_CALL(st, 0x00444270u, S32(st, G_U605A10) - 1);
    }
callback:
    if (U32(st, G_NUMPAD_CALLBACK) != 0) {
        GAME_CALL0(st, U32(st, G_NUMPAD_CALLBACK));
        U32(st, G_NUMPAD_CALLBACK) = 0;
        U8(st, G_WINDOW) = U8(st, G_NUMPAD_BACK);
        U8(st, G_REDRAW) = 1;
        return;
    }
    U8(st, G_REDRAW) = 1;
    U8(st, G_WINDOW) = U8(st, G_NUMPAD_BACK);
}

void numpad_cancel(game_state *st)
{
    /* @bin 0x00403770 back to 0x00605980 */
    U8(st, G_REDRAW) = 1;
    U8(st, G_WINDOW) = U8(st, G_NUMPAD_BACK);
}

static int32_t choice_value(game_state *st, uint32_t entry)
{
    /* the string index of a choice: [0x006057F4 + 4 k] + 0x00605940; in the editor's group 0x17
     * the entry 0x13 is the name of the chosen empire city's ... (0x005D2A3C + 8 [0x0077F761 +
     * 0x62 [0x0077591C]]) */
    if (S32(st, G_EDITOR) == 1 && S32(st, G_LIST_TEXT_GROUP) == 0x17) {
        if (S32(st, entry) == 0x13) {
            uint8_t b = U8(st, G_EMPIRE_OBJECTS + 0x19u + 0x62u * (uint32_t) S32(st, G_EDITOR_OBJECT));
            return S32(st, G_CITY_KIND_TEXTS + 8u * b);
        }
        return S32(st, entry) + S32(st, G_LIST_TEXT_FIRST);
    }
    return S32(st, G_LIST_TEXT_FIRST) + S32(st, entry);
}

void popup_list_draw(game_state *st, int32_t x, int32_t y, int32_t split)
{
    /* @bin 0x00530D70 the list's panel (0x006057EC x 13 pieces wide, rows of 20 pixels; one column of
     * 13 when all rows are in the first column, split >= count); with 0x00605A58 the extra button
     * (rectangle 0x005C5E78 set below the first column, callback 0x00409300, or 0x004092B0 without
     * 0x00605A54; its hover / highlight / click taken here, a click ends the call) and its label
     * (group 0x130, 0x00605A54); then the choices (0x004D6380 group 0x00605944, on the rectangles
     * 0x00704DC8 offset by (x, y) - layout origin), coloured 0x2BF when hovered unless 0x006057F0 and
     * the hover is on the first entry of a column */
    int32_t count = S32(st, G_LIST_COUNT);
    int32_t city = S32(st, G_EDITOR_OBJECT);
    (void) city;
    if (count > split) {
        GAME_CALL(st, FN_PANEL_DRAW, x, y, S32(st, G_LIST_COLUMNS) * 13, (split * 20 + 0x18) / 16);
    } else {
        GAME_CALL(st, FN_PANEL_DRAW, x, y, 0x0D, (count * 21 + 0x18) / 16);
    }
    if (S32(st, G_LIST_EXTRA_ON) != 0) {
        int32_t by = count > split ? y + split * 20 - 0x19 : y + count * 20 - 0x19;
        S16(st, G_LIST_EXTRA_BUTTON + 2u) = (int16_t) by;
        U32(st, G_LIST_EXTRA_BUTTON + 8u) = 0x00409300u;
        S16(st, G_LIST_EXTRA_BUTTON + 6u) = (int16_t) (by + 0x1E);
        int32_t bx = S32(st, G_LIST_COLUMNS) * 0xD0 + x - 0xDA;
        S16(st, G_LIST_EXTRA_BUTTON) = (int16_t) bx;
        S16(st, G_LIST_EXTRA_BUTTON + 4u) = (int16_t) (bx + 0xC8);
        if (S32(st, G_LIST_EXTRA_LABEL) == 0) {
            U32(st, G_LIST_EXTRA_BUTTON + 8u) = 0x004092B0u;
        }
        int32_t h = GAME_CALL(st, FN_BUTTONS_HOVERED, 0, 0, G_LIST_EXTRA_BUTTON, 1, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        int32_t lit = GAME_CALL(st, FN_RECT_HIGHLIGHT, 0, 0, G_LIST_EXTRA_BUTTON, 1, h);
        if (GAME_CALL(st, FN_BUTTONS_RECT_CLICK, 0, 0, G_LIST_EXTRA_BUTTON, 1) != 0) {
            return;
        }
        GAME_CALL(st, FN_LAYOUT_FRAME, 1, bx, by, 0xC8, 0x1E, lit == 1, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x130, S32(st, G_LIST_EXTRA_LABEL), bx, by + 8, 0xC8, S32(st, G_FONT_LARGE), 0,
                  S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    }
    int32_t ox = x - S32(st, G_LAYOUT_DX);
    int32_t oy = y - S32(st, G_LAYOUT_DY);
    void layout_text_group_rect(game_state *, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                                int32_t);
    if (count > split) {
        for (int32_t k = 0; k < split; k++) {
            int32_t c = S32(st, G_HOVER) == k + 1 ? 0x2BF : 0;
            if (U8(st, G_LIST_A15) != 0 && S32(st, G_HOVER) == 1) {
                c = 0;
            }
            int32_t v = choice_value(st, G_LIST_ORDER + 4u * (uint32_t) k);
            layout_text_group_rect(st, S32(st, G_LIST_TEXT_GROUP), v, G_LIST_BUTTONS + 0x1Cu * (uint32_t) k, S32(st, G_FONT_LARGE),
                                   c, ox, oy, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        }
        for (int32_t j = 0; j < S32(st, G_LIST_COUNT) - split; j++) {
            int32_t k = split + j;
            int32_t c = S32(st, G_HOVER) == k + 1 ? 0x2BF : 0;
            if (U8(st, G_LIST_A15) != 0 && S32(st, G_HOVER) == split + 1) {
                c = 0;
            }
            int32_t v = choice_value(st, G_LIST_ORDER + 4u * (uint32_t) k);
            layout_text_group_rect(st, S32(st, G_LIST_TEXT_GROUP), v, G_LIST_BUTTONS + 0x1Cu * (uint32_t) k, S32(st, G_FONT_LARGE),
                                   c, ox, oy, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
        }
        return;
    }
    for (int32_t k = 0; k < S32(st, G_LIST_COUNT); k++) {
        int32_t c = S32(st, G_HOVER) == k + 1 ? 0x2BF : 0;
        if (U8(st, G_LIST_A15) != 0 && (S32(st, G_HOVER) == 1 || S32(st, G_HOVER) == split)) {
            c = 0;
        }
        layout_text_group_rect(st, S32(st, G_LIST_TEXT_GROUP), S32(st, G_LIST_ORDER + 4u * (uint32_t) k) + S32(st, G_LIST_TEXT_FIRST),
                               G_LIST_BUTTONS + 0x1Cu * (uint32_t) k, S32(st, G_FONT_LARGE), c, ox, oy, S32(st, G_LAYOUT_DX),
                               S32(st, G_LAYOUT_DY));
    }
}

static int32_t run_draw(game_state *st, const int32_t *a) { numpad_draw(st, a[0], a[1]); return 0; }
static int32_t run_digit(game_state *st, const int32_t *a) { (void) a; numpad_digit(st); return 0; }
static int32_t run_ok(game_state *st, const int32_t *a) { (void) a; numpad_ok(st); return 0; }
static int32_t run_cancel(game_state *st, const int32_t *a) { (void) a; numpad_cancel(st); return 0; }
static int32_t run_popup(game_state *st, const int32_t *a) { popup_list_draw(st, a[0], a[1], a[2]); return 0; }

const ported_function frontend_numpad_ports[] = {
    {0x005309B0u, "numpad_draw", 2, 0, 0, run_draw, 0},
    {0x004035A0u, "numpad_digit", 0, 0, 0, run_digit, 0},
    {0x004035F0u, "numpad_ok", 0, 0, 0, run_ok, 0},
    {0x00403770u, "numpad_cancel", 0, 0, 0, run_cancel, 0},
    {0x00530D70u, "popup_list_draw", 3, 0, 0, run_popup, 0},
};
const int frontend_numpad_ports_count = sizeof(frontend_numpad_ports) / sizeof(frontend_numpad_ports[0]);
