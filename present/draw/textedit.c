/* Imhotep - drawing primitives: the edit field's text and caret (package blit, roadmap 7.3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The edit fields (names, file names, ...) are slots of tables indexed by the current field
 * [0x00E20990]: the string's address (0x00E2079C), the caret's index (0x00E207EC), the end the caret
 * may reach (0x00E208A0), the size limit (0x00E208F0), the width limit in pixels (0x00E20850), the
 * font (0x00E20940), a byte that refuses punctuation (0x00E2083C). The caret is drawn by the text
 * draw 0x004CC770 (0x00E2078C .. 0x00E20794). */
#include "text.h"

#include "mem.h"

#include "city/text_lookup.h"

#define FIELD 0x00E20990u
#define FIELD_RESULT 0x00E20994u /* i32 0, 1 (caret at the end), 2 (field full) */
#define FIELD_TEXT 0x00E2079Cu
#define FIELD_CARET 0x00E207ECu
#define FIELD_END 0x00E208A0u
#define FIELD_MAX 0x00E208F0u
#define FIELD_MAX_W 0x00E20850u
#define FIELD_FONT 0x00E20940u
#define FIELD_NO_PUNCT 0x00E2083Cu
#define FIELD_INSERT 0x00E2079Bu /* u8 1: insert, 0: overwrite */

#define AT(table, f) ((table) + 4u * (uint32_t) (f))

void string_delete_at(game_state *st, uint32_t p, uint32_t end)
{
    /* @bin 0x004CBEE0 the bytes after p moved one left up to end (unsigned compare), *end = 0; or
     * *p = 0 when p >= end */
    if (p < end) {
        uint32_t n = end - p;
        for (uint32_t i = 0; i < n; i++) {
            mem_wr8(st, p + i, mem_rd8(st, p + 1 + i));
        }
        mem_wr8(st, p + n, 0);
        return;
    }
    mem_wr8(st, p, 0);
}

void string_open_at(game_state *st, uint32_t p, uint32_t end)
{
    /* @bin 0x004CBF10 end[1] = 0, then the bytes from p to end - 1 moved one right */
    mem_wr8(st, end + 1, 0);
    while (end > p) {
        mem_wr8(st, end, mem_rd8(st, end - 1));
        end--;
    }
}

void field_check(game_state *st)
{
    /* @bin 0x004CC1B0 the caret kept within the field (result 1 at its end, 2 at the size limit or
     * when the text is as wide as the width limit; at least 0) */
    int32_t f = S32(st, FIELD);
    S32(st, FIELD_RESULT) = 0;
    if (f <= 0) {
        return;
    }
    int32_t w = text_width(st, U32(st, AT(FIELD_TEXT, f)), S32(st, AT(FIELD_FONT, f)));
    int32_t g = S32(st, FIELD);
    uint32_t caret = AT(FIELD_CARET, g);
    int32_t end = S32(st, AT(FIELD_END, g)) + 1;
    if (S32(st, caret) >= end) {
        S32(st, caret) = end;
        S32(st, FIELD_RESULT) = 1;
    }
    int32_t max = S32(st, AT(FIELD_MAX, g));
    if (S32(st, caret) >= max) {
        S32(st, caret) = max;
        S32(st, FIELD_RESULT) = 2;
    }
    if (w >= S32(st, AT(FIELD_MAX_W, g))) {
        S32(st, FIELD_RESULT) = 2;
    }
    if (S32(st, caret) < 0) {
        S32(st, caret) = 0;
    }
}

void field_insert(game_state *st, int32_t ch)
{
    /* @bin 0x004CC400 the character at the caret (inserted when [0x00E2079B], else written over with
     * a NUL after it), the caret moved right; nothing when the field is full */
    if (S32(st, FIELD) <= 0) {
        return;
    }
    field_check(st);
    if (S32(st, FIELD_RESULT) == 2) {
        return;
    }
    int32_t f = S32(st, FIELD);
    if (U8(st, FIELD_INSERT) != 0) {
        int32_t end = S32(st, AT(FIELD_END, f));
        if (end >= S32(st, AT(FIELD_MAX, f))) {
            return;
        }
        uint32_t text = U32(st, AT(FIELD_TEXT, f));
        string_open_at(st, U32(st, AT(FIELD_CARET, f)) + text, text + (uint32_t) end + 1u);
        f = S32(st, FIELD);
        mem_wr8(st, U32(st, AT(FIELD_CARET, f)) + U32(st, AT(FIELD_TEXT, f)), (uint8_t) ch);
        f = S32(st, FIELD);
        S32(st, AT(FIELD_CARET, f)) += 1;
        return;
    }
    mem_wr8(st, U32(st, AT(FIELD_CARET, f)) + U32(st, AT(FIELD_TEXT, f)), (uint8_t) ch);
    f = S32(st, FIELD);
    uint32_t caret = U32(st, AT(FIELD_CARET, f)) + 1u;
    U32(st, AT(FIELD_CARET, f)) = caret;
    mem_wr8(st, U32(st, AT(FIELD_TEXT, f)) + caret, 0);
}

void field_type(game_state *st, int32_t ch)
{
    /* @bin 0x004CC250 a typed character: space, '-', digits, letters, 0x80..0x9A, 0xA0..0xA7, 0xE1, and
     * , . ? ! unless the field refuses them; then the caret check 0x004CC1B0 */
    uint8_t c = (uint8_t) ch;
    int take = 0;
    if (c == 0x20 || c == 0x2D) {
        take = 1;
    } else if (c == 0x2C || c == 0x2E || c == 0x3F || c == 0x21) {
        take = U8(st, FIELD_NO_PUNCT + U32(st, FIELD)) == 0;
    } else if ((c >= 0x30 && c <= 0x39) || (c >= 0x61 && c <= 0x7A) || (c >= 0x41 && c <= 0x5A) ||
               (c >= 0x80 && c <= 0x9A) || (c >= 0xA0 && c <= 0xA7) || c == 0xE1) {
        take = 1;
    }
    if (take) {
        field_insert(st, ch);
    }
    field_check(st);
}

void field_delete(game_state *st)
{
    /* @bin 0x004CC4C0 the character at the caret removed (up to the field's end) */
    int32_t f = S32(st, FIELD);
    if (f <= 0) {
        return;
    }
    uint32_t text = U32(st, AT(FIELD_TEXT, f));
    string_delete_at(st, U32(st, AT(FIELD_CARET, f)) + text, U32(st, AT(FIELD_END, f)) + text);
    field_check(st);
}

void field_backspace(game_state *st)
{
    /* @bin 0x004CC330 */
    int32_t f = S32(st, FIELD);
    if (S32(st, AT(FIELD_CARET, f)) > 0) {
        S32(st, AT(FIELD_CARET, f)) -= 1;
        field_delete(st);
    }
}

void field_left(game_state *st)
{
    /* @bin 0x004CC370 */
    int32_t f = S32(st, FIELD);
    if (S32(st, AT(FIELD_CARET, f)) > 0) {
        S32(st, AT(FIELD_CARET, f)) -= 1;
    }
}

void field_right(game_state *st)
{
    /* @bin 0x004CC390 */
    int32_t f = S32(st, FIELD);
    if (S32(st, AT(FIELD_CARET, f)) < S32(st, AT(FIELD_END, f))) {
        S32(st, AT(FIELD_CARET, f)) += 1;
    }
}

void field_toggle_insert(game_state *st)
{
    /* @bin 0x004CC360 */
    U8(st, FIELD_INSERT) ^= 1;
}

void field_done(game_state *st)
{
    /* @bin 0x004CC320 [0x00E20998] = 1 */
    S32(st, 0x00E20998u) = 1;
}

void text_run_reset(game_state *st)
{
    /* @bin 0x004CC540 the run of the text draw: caret placed, characters, caret width, x, y */
    S32(st, 0x00E20788u) = 0;
    S32(st, 0x00E20784u) = 0;
    S32(st, 0x00E2078Cu) = 0;
    S32(st, 0x00E20778u) = 0;
    S32(st, 0x00E20774u) = 0;
}
