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

#include "sim/call.h"
#include "state/ported.h"

#define VALUE 0x0060597Cu   /* i32 */
#define DIGITS 0x00605978u  /* i32 */
#define TYPE 0x0060596Cu    /* i32 1, 2 or 4 */
#define BACK 0x00605980u    /* u8 the window to go back to */

void numpad_draw(game_state *st, int32_t x, int32_t y)
{
    /* @bin 0x005309B0 the panel (8 x 14) around (x, y), the value's box (96 x 30 black) and the value
     * (red, centred) once a digit is typed; the keys 1..9, 0 (25 x 25 rectangles with their digit,
     * colour 0x2BF when hovered, else 0), OK (0x37 wide, group 0x2C, 0x10) and cancel (0x55, 0x11) */
    GAME_CALL(st, 0x00536820u, x - 0x10, y - 0x10, 8, 0x0E);
    GAME_CALL(st, 0x004CD9D0u, x, y, 0x60, 0x1E, 0);
    if (S32(st, DIGITS) > 0) {
        GAME_CALL(st, 0x004CCE90u, S32(st, VALUE), 0x40, 0x0057C048, x - 8, y + 3, 0x60, S32(st, 0x00F56FB4u), 0xF800);
    }
    int32_t cx[3] = {x + 5, x + 0x23, x + 0x41};
    /* @bin 0x005F8A00 .. 0x005F89DC the keys' strings "1" .. "9", "0" */
    for (int32_t k = 1; k <= 10; k++) {
        int32_t kx = cx[(k - 1) % 3];
        int32_t ky = y + 0x23 + 0x1E * ((k - 1) / 3);
        int32_t c = S32(st, FE_HOVER) == k ? 0x2BF : 0;
        GAME_CALL(st, 0x004CD680u, kx, ky, 0x19, 0x19, c);
        GAME_CALL(st, 0x004CC720u, (int32_t) (0x005F8A00u - 4u * (uint32_t) (k - 1)), kx, ky, 0x19, S32(st, 0x00F56FB4u), c);
    }
    int32_t c = S32(st, FE_HOVER) == 11 ? 0x2BF : 0;
    GAME_CALL(st, 0x004CD680u, cx[1], y + 0x7D, 0x37, 0x19, c);
    GAME_CALL(st, 0x004D6250u, 0x2C, 0x10, cx[1], y + 0x83, 0x37, S32(st, FE_FONT_LARGE), c, S32(st, FE_DX), S32(st, FE_DY));
    c = S32(st, FE_HOVER) == 12 ? 0x2BF : 0;
    GAME_CALL(st, 0x004CD680u, cx[0], y + 0x9B, 0x55, 0x19, c);
    GAME_CALL(st, 0x004D6250u, 0x2C, 0x11, cx[0], y + 0xA1, 0x55, S32(st, FE_FONT_LARGE), c, S32(st, FE_DX), S32(st, FE_DY));
}

void numpad_digit(game_state *st)
{
    /* @bin 0x004035A0 a digit (0x00D3B198) appended while fewer than 3 (bytes) or 5 digits; 0x0047D350(4) */
    if (S32(st, DIGITS) >= (S32(st, TYPE) != 1 ? 5 : 3)) {
        return;
    }
    U8(st, FE_REDRAW) = 1;
    S32(st, VALUE) = (int32_t) ((uint32_t) S32(st, FE_PARAM1) + (uint32_t) S32(st, VALUE) * 10u);
    S32(st, DIGITS) = S32(st, DIGITS) + 1;
    GAME_CALL(st, 0x0047D350u, 4);
}

void numpad_ok(game_state *st)
{
    /* @bin 0x004035F0 the value into its field (by the type; a field that was negative stays
     * negative), then: 0x00605968 = 1 the map's period 0x00605A14 - 1 (0..0x21: 0x00BCA2B8, 0x004DE160,
     * the camera at 0x6D, 0xDA), 2 0x00444270(0x00605A10 - 1); the callback 0x00605A4C (cleared); back
     * to 0x00605980 */
    int32_t type = S32(st, TYPE);
    uint32_t p16 = U32(st, 0x00605960u), p32 = U32(st, 0x0060595Cu);
    int32_t sign = 1;
    int stored = 0;
    if (type == 2 && S16(st, p16) < 0) {
        sign = -1;
        S16(st, p16) = (int16_t) S32(st, VALUE);
        stored = 1;
    } else if (type == 4 && S32(st, p32) < 0) {
        sign = -1;
        S32(st, p32) = S32(st, VALUE);
        stored = 1;
    }
    if (!stored) {
        if (type == 1) {
            P8(st, U32(st, 0x00605964u)) = (uint8_t) S32(st, VALUE);
        } else if (type == 2) {
            S16(st, p16) = (int16_t) S32(st, VALUE);
        } else if (type == 4) {
            S32(st, p32) = S32(st, VALUE);
        }
    }
    type = S32(st, TYPE);
    if (type == 1) {
        uint32_t p8 = U32(st, 0x00605964u);
        uint8_t v = P8(st, p8);
        if (v == 0 || sign != -1) {
            goto after;
        }
        P8(st, p8) = (uint8_t) -v;
        type = S32(st, TYPE);
    }
    if (type == 2) {
        p16 = U32(st, 0x00605960u);
        int16_t v = S16(st, p16);
        if ((v < 0 && sign == 1) || (v > 0 && sign == -1)) {
            S16(st, p16) = (int16_t) -v;
            type = S32(st, TYPE);
        } else {
            goto after;
        }
    }
    if (type == 4) {
        p32 = U32(st, 0x0060595Cu);
        int32_t v = S32(st, p32);
        if ((v < 0 && sign == 1) || (v > 0 && sign == -1)) {
            S32(st, p32) = (int32_t) (0u - (uint32_t) v);
        }
    }
after:
    if (S32(st, 0x00605968u) == 1) {
        int32_t v = S32(st, 0x00605A14u) - 1;
        S32(st, 0x00605A14u) = v;
        if (v < 0 || v > 0x21) {
            goto callback;
        }
        S32(st, 0x00BCA2B8u) = v;
        GAME_CALL(st, 0x004DE160u, v);
        S32(st, 0x00BCA2BCu) = 0x6D;
        S32(st, 0x00BCA2C0u) = 0xDA;
    }
    if (S32(st, 0x00605968u) == 2) {
        GAME_CALL(st, 0x00444270u, S32(st, 0x00605A10u) - 1);
    }
callback:
    if (U32(st, 0x00605A4Cu) != 0) {
        GAME_CALL0(st, U32(st, 0x00605A4Cu));
        U32(st, 0x00605A4Cu) = 0;
        U8(st, FE_WINDOW) = U8(st, BACK);
        U8(st, FE_REDRAW) = 1;
        return;
    }
    U8(st, FE_REDRAW) = 1;
    U8(st, FE_WINDOW) = U8(st, BACK);
}

void numpad_cancel(game_state *st)
{
    /* @bin 0x00403770 back to 0x00605980 */
    U8(st, FE_REDRAW) = 1;
    U8(st, FE_WINDOW) = U8(st, BACK);
}

static int32_t choice_value(game_state *st, uint32_t entry)
{
    /* the string index of a choice: [0x006057F4 + 4 k] + 0x00605940; in the editor's group 0x17
     * the entry 0x13 is the name of the chosen empire city's ... (0x005D2A3C + 8 [0x0077F761 +
     * 0x62 [0x0077591C]]) */
    if (S32(st, 0x00605A3Cu) == 1 && S32(st, 0x00605944u) == 0x17) {
        if (S32(st, entry) == 0x13) {
            uint8_t b = U8(st, 0x0077F761u + 0x62u * (uint32_t) S32(st, 0x0077591Cu));
            return S32(st, 0x005D2A3Cu + 8u * b);
        }
        return S32(st, entry) + S32(st, 0x00605940u);
    }
    return S32(st, 0x00605940u) + S32(st, entry);
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
    int32_t count = S32(st, 0x00605948u);
    int32_t city = S32(st, 0x0077591Cu);
    (void) city;
    if (count > split) {
        GAME_CALL(st, 0x00536820u, x, y, S32(st, 0x006057ECu) * 13, (split * 20 + 0x18) / 16);
    } else {
        GAME_CALL(st, 0x00536820u, x, y, 0x0D, (count * 21 + 0x18) / 16);
    }
    if (S32(st, 0x00605A58u) != 0) {
        int32_t by = count > split ? y + split * 20 - 0x19 : y + count * 20 - 0x19;
        S16(st, 0x005C5E7Au) = (int16_t) by;
        U32(st, 0x005C5E80u) = 0x00409300u;
        S16(st, 0x005C5E7Eu) = (int16_t) (by + 0x1E);
        int32_t bx = S32(st, 0x006057ECu) * 0xD0 + x - 0xDA;
        S16(st, 0x005C5E78u) = (int16_t) bx;
        S16(st, 0x005C5E7Cu) = (int16_t) (bx + 0xC8);
        if (S32(st, 0x00605A54u) == 0) {
            U32(st, 0x005C5E80u) = 0x004092B0u;
        }
        int32_t h = GAME_CALL(st, 0x00426450u, 0, 0, 0x005C5E78, 1, S32(st, FE_DX), S32(st, FE_DY));
        int32_t lit = GAME_CALL(st, 0x004D6740u, 0, 0, 0x005C5E78, 1, h);
        if (GAME_CALL(st, 0x004261D0u, 0, 0, 0x005C5E78, 1) != 0) {
            return;
        }
        GAME_CALL(st, 0x004D63D0u, 1, bx, by, 0xC8, 0x1E, lit == 1, S32(st, FE_DX), S32(st, FE_DY));
        GAME_CALL(st, 0x004D6250u, 0x130, S32(st, 0x00605A54u), bx, by + 8, 0xC8, S32(st, FE_FONT_LARGE), 0,
                  S32(st, FE_DX), S32(st, FE_DY));
    }
    int32_t ox = x - S32(st, FE_DX);
    int32_t oy = y - S32(st, FE_DY);
    void layout_text_group_rect(game_state *, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                                int32_t);
    if (count > split) {
        for (int32_t k = 0; k < split; k++) {
            int32_t c = S32(st, FE_HOVER) == k + 1 ? 0x2BF : 0;
            if (U8(st, 0x006057F0u) != 0 && S32(st, FE_HOVER) == 1) {
                c = 0;
            }
            int32_t v = choice_value(st, 0x006057F4u + 4u * (uint32_t) k);
            layout_text_group_rect(st, S32(st, 0x00605944u), v, 0x00704DC8u + 0x1Cu * (uint32_t) k, S32(st, FE_FONT_LARGE),
                                   c, ox, oy, S32(st, FE_DX), S32(st, FE_DY));
        }
        for (int32_t j = 0; j < S32(st, 0x00605948u) - split; j++) {
            int32_t k = split + j;
            int32_t c = S32(st, FE_HOVER) == k + 1 ? 0x2BF : 0;
            if (U8(st, 0x006057F0u) != 0 && S32(st, FE_HOVER) == split + 1) {
                c = 0;
            }
            int32_t v = choice_value(st, 0x006057F4u + 4u * (uint32_t) k);
            layout_text_group_rect(st, S32(st, 0x00605944u), v, 0x00704DC8u + 0x1Cu * (uint32_t) k, S32(st, FE_FONT_LARGE),
                                   c, ox, oy, S32(st, FE_DX), S32(st, FE_DY));
        }
        return;
    }
    for (int32_t k = 0; k < S32(st, 0x00605948u); k++) {
        int32_t c = S32(st, FE_HOVER) == k + 1 ? 0x2BF : 0;
        if (U8(st, 0x006057F0u) != 0 && (S32(st, FE_HOVER) == 1 || S32(st, FE_HOVER) == split)) {
            c = 0;
        }
        layout_text_group_rect(st, S32(st, 0x00605944u), S32(st, 0x006057F4u + 4u * (uint32_t) k) + S32(st, 0x00605940u),
                               0x00704DC8u + 0x1Cu * (uint32_t) k, S32(st, FE_FONT_LARGE), c, ox, oy, S32(st, FE_DX),
                               S32(st, FE_DY));
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
