/* Imhotep - drawing primitives: text (package blit, roadmap 7.3).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The fonts are image groups (type 20 glyph masks, drawn in the colour [0x00D3B51C] by
 * 0x0053E590); a character's glyph is the font's first image + the character's number - 1
 * (tables 0x005E049C for the drawing, 0x005E047C for the widths, which is the same table offset by
 * 0x20). The text functions keep their state in globals: the text cursor 0x00E20768, the number
 * buffer 0x00E34238 (256 bytes), the line buffer 0x00E3439C (200 bytes), the run of the edit field
 * (0x00E20774 .. 0x00E20799, the edited string 0x00E34338), the font's line height 0x00D3B500. */
#include "text.h"

#include "draw.h"
#include "mem.h"

#include "city/text_lookup.h"
#include "sim/call.h"

#define GLYPH_DRAW_MAP 0x005E049Cu /* u8 per character - 0x20: glyph number (0: none) */
#define GLYPH_WIDTH_MAP 0x005E047Cu
#define GLYPH_LAST 0x00D3B5C0u
#define IMAGE_COUNT 0x0126517Cu
#define IMAGES 0x010D69F8u        /* the graphics object's entries (0x00F55BB0 + 0x180E48) */
#define FONT_LARGE 0x00F56FB4u    /* i32 first image of three fonts */
#define FONT_LARGE2 0x00F56FBCu
#define FONT_SMALL 0x00F56F84u
#define FONT_HEIGHT 0x00D3B500u   /* i32 the font's line height (glyphs taller than it go down) */
#define TEXT_OUTLINE 0x00D3B510u  /* i32 != 0: plain (the other styles off) */
#define TEXT_SHADOW_V 0x00D3B518u /* i32 != 0: black copies one pixel above and below */
#define TEXT_SHADOW 0x00D3B514u   /* i32 != 0: a black copy one pixel right and down */
#define TEXT_FLAG_504 0x00D3B504u
#define NUMBER_BUF 0x00E34238u    /* 256 bytes */
#define EDIT_TEXT 0x00E34338u     /* the edited string */
#define LINE_BUF 0x00E3439Cu      /* 200 bytes */
#define RUN_X 0x00E20778u         /* i32 the width drawn so far (the edit field's caret x) */
#define RUN_Y 0x00E20774u         /* i32 the line's y offset (multi-line text) */
#define RUN_EDIT 0x00E2077Cu      /* i32 != 0: characters come from the edited string, from this index */
#define RUN_HASH 0x00E20780u      /* i32 1: '#' starts the edited string */
#define RUN_CHARS 0x00E20784u     /* i32 characters drawn */
#define CARET_SET 0x00E20788u     /* i32 1: the caret was placed */
#define CARET_W 0x00E2078Cu
#define CARET_X 0x00E20790u
#define CARET_Y 0x00E20794u
#define RUN_SPACES 0x00E20798u    /* u8 != 0: collapse repeated spaces */
#define RUN_NO_COLLAPSE 0x00E20799u
#define CARET_FIELD 0x00E20990u   /* i32 the edit field drawn */
#define CARET_POS 0x00E207ECu     /* i32 per field: the caret's character index */
#define LINE_MAX_W 0x00E2076Cu    /* i32 the widest line of the multi-line text */
#define WORD_CHARS 0x00E20760u    /* i32 characters of the last measured word */
#define LAYOUT_DX 0x00E38DA8u     /* i32 offsets of the 640 x 480 layout on the screen */
#define LAYOUT_DY 0x00E38DA4u

static int32_t image_index(game_state *st, int32_t id)
{
    return id >= S32(st, IMAGE_COUNT) ? 0 : id;
}

int32_t text_glyph_draw(game_state *st, int32_t font, int32_t ch, int32_t x, int32_t y)
{
    /* @bin 0x004CCA60 the glyph of ch (map 0x005E049C, the low byte) at (x, y - 3), lowered by its
     * height above the font's line height [0x00D3B500] when the low byte is >= 0x60 and not 0xC7 /
     * 0xE7; returns the glyph's width, 0 when the character has none */
    uint8_t c = (uint8_t) ch;
    int32_t glyph = U8(st, GLYPH_DRAW_MAP + c);
    S32(st, GLYPH_LAST) = glyph;
    if (glyph == 0) {
        return 0;
    }
    int32_t id = glyph + font - 1;
    int32_t drop = S16(st, IMAGES + 0x16u + ((uint32_t) image_index(st, id) << 6)) - S32(st, FONT_HEIGHT);
    if (drop < 0) {
        drop = 0;
    }
    if (c < 0x60) {
        drop = 0;
    }
    if (c == 0xC7 || c == 0xE7) {
        drop = 0;
    }
    image_draw(st, id, x, y - 3 - drop);
    id = S32(st, GLYPH_LAST) + font - 1;
    return S16(st, IMAGES + 0x14u + ((uint32_t) image_index(st, id) << 6));
}

int32_t text_edit_next(game_state *st)
{
    /* @bin 0x004CCB00 the next character of the edited string 0x00E34338 (index [0x00E2077C]): at its
     * end the index goes back to 0 and the character is a space. Only al is meaningful. */
    uint32_t i = U32(st, RUN_EDIT);
    uint8_t c = mem_rd8(st, i + EDIT_TEXT);
    if (c == 0) {
        U32(st, RUN_EDIT) = 0;
        return (int32_t) ((i & 0xFFFFFF00u) | 0x20u);
    }
    U32(st, RUN_EDIT) = i + 1;
    return (int32_t) ((i & 0xFFFFFF00u) | c);
}

/* the string's byte i: from the host (a string the port built on its own stack, where the original
 * passes a stack buffer) or from the state */
static uint8_t text_at(game_state *st, uint32_t text, const uint8_t *host, uint32_t i)
{
    return host ? host[i] : mem_rd8(st, text + i);
}

static int32_t text_draw_any(game_state *st, uint32_t text, const uint8_t *host, int32_t x, int32_t y, int32_t font,
                             int32_t colour);

int32_t text_draw(game_state *st, uint32_t text, int32_t x, int32_t y, int32_t font, int32_t colour)
{
    return text_draw_any(st, text, NULL, x, y, font, colour);
}

int32_t text_draw_host(game_state *st, const char *text, int32_t x, int32_t y, int32_t font, int32_t colour)
{
    return text_draw_any(st, 0, (const uint8_t *) text, x, y, font, colour);
}

static int32_t text_draw_any(game_state *st, uint32_t text, const uint8_t *host, int32_t x, int32_t y, int32_t font,
                             int32_t colour)
{
    /* @bin 0x004CC770 the string at (x, y) in the font (space width and line height by font: large
     * 8 / 0x17, large2 8 / 0x18, small 4 / 9 with 1 extra pixel per glyph, others 6 / 0xB) and colour
     * (style by 0x00D3B510 / 0x00D3B518 / 0x00D3B514); '_' and a '#' outside an edit field are
     * spaces, other bytes below 0x20 take no room. It keeps the run of the edit field: the width
     * drawn (0x00E20778, + one space at the end), the caret at the field's caret index. Returns 0. */
    int32_t space, extra;
    if (font == S32(st, FONT_LARGE)) {
        space = 8;
        S32(st, FONT_HEIGHT) = 0x17;
        extra = 0;
    } else if (font == S32(st, FONT_LARGE2)) {
        space = 8;
        S32(st, FONT_HEIGHT) = 0x18;
        extra = 0;
    } else if (font == S32(st, FONT_SMALL)) {
        space = 4;
        S32(st, FONT_HEIGHT) = 9;
        extra = 1;
    } else {
        space = 6;
        S32(st, FONT_HEIGHT) = 0xB;
        extra = 0;
    }
    U16(st, GLYPH_COLOUR) = (uint16_t) colour;
    uint32_t at = 0;
    uint8_t al = text_at(st, text, host, at);
    if (al != 0) {
        /* the character's dword: the low byte replaced in y's stack slot (only al is used) */
        uint32_t yslot = (uint32_t) y;
        do {
            uint8_t prev = al;
            int32_t w;
            int draw = 1;
            if (al == 0x23) {
                if (S32(st, RUN_HASH) == 1) {
                    al = (uint8_t) text_edit_next(st);
                    if (al == 0x5F) {
                        al = 0x20;
                    } else if (al < 0x20) {
                        draw = 0;
                    }
                } else {
                    al = 0x20;
                }
            } else if (al == 0x5F) {
                al = 0x20;
            } else if (al < 0x20) {
                draw = 0;
            }
            if (!draw) {
                w = 0;
            } else {
                uint8_t c = (uint8_t) (al - 0x20);
                yslot = (yslot & 0xFFFFFF00u) | c;
                int32_t ch = (int32_t) yslot;
                if (U8(st, GLYPH_DRAW_MAP + c) == 0) {
                    w = space;
                } else if (S32(st, TEXT_OUTLINE) != 0) {
                    U16(st, GLYPH_COLOUR) = (uint16_t) colour;
                    w = text_glyph_draw(st, font, ch, x, y) + extra;
                } else if (S32(st, TEXT_SHADOW_V) != 0) {
                    U16(st, GLYPH_COLOUR) = 0;
                    text_glyph_draw(st, font, ch, x, y - 1);
                    U16(st, GLYPH_COLOUR) = 0;
                    text_glyph_draw(st, font, ch, x, y + 1);
                    U16(st, GLYPH_COLOUR) = (uint16_t) colour;
                    w = text_glyph_draw(st, font, ch, x, y) + extra;
                } else if (S32(st, TEXT_SHADOW) != 0) {
                    U16(st, GLYPH_COLOUR) = 0;
                    text_glyph_draw(st, font, ch, x + 1, y + 1);
                    U16(st, GLYPH_COLOUR) = (uint16_t) colour;
                    w = text_glyph_draw(st, font, ch, x - 1, y) + extra;
                } else {
                    U16(st, GLYPH_COLOUR) = (uint16_t) colour;
                    w = text_glyph_draw(st, font, ch, x, y) + extra;
                }
                /* @bin 0x004CC941 the caret of the edit field: at its caret index, once */
                if (S32(st, RUN_CHARS) == S32(st, CARET_POS + 4u * U32(st, CARET_FIELD)) && S32(st, CARET_SET) == 0) {
                    S32(st, CARET_W) = w;
                    S32(st, CARET_X) = S32(st, RUN_X);
                    S32(st, CARET_Y) = S32(st, RUN_Y);
                    S32(st, CARET_SET) = 1;
                }
                x += w;
                S32(st, RUN_X) += w;
            }
            if (S32(st, RUN_EDIT) != 0) {
                al = (uint8_t) text_edit_next(st);
            } else {
                at++;
                al = text_at(st, text, host, at);
            }
            S32(st, RUN_CHARS) += 1;
            if (U8(st, RUN_NO_COLLAPSE) == 0 && U8(st, RUN_SPACES) != 0 && prev == al && (al == 0x20 || al == 0x5F)) {
                S32(st, RUN_X) -= w;
                x -= w;
            }
        } while (al != 0);
    }
    if (S32(st, CARET_SET) == 0) {
        S32(st, CARET_W) = 4;
        S32(st, CARET_X) = S32(st, RUN_X);
        S32(st, CARET_Y) = S32(st, RUN_Y);
        S32(st, CARET_SET) = 1;
    }
    S32(st, RUN_HASH) = 0;
    S32(st, RUN_X) += space;
    S32(st, RUN_EDIT) = 0;
    U8(st, RUN_NO_COLLAPSE) = 0;
    S32(st, TEXT_FLAG_504) = 0;
    return 0;
}

int32_t text_width_host(game_state *st, const char *text, int32_t font)
{
    /* @bin 0x004CBF30 text_width (core/city/text_lookup.c) on a host string: spaces 10 / 4 / 6 by
     * font (large and large 2 / small / others), glyph widths + 1 for those three fonts; the last
     * glyph number in 0x00D3B5C0 */
    int32_t space, extra;
    if (font == S32(st, FONT_LARGE) || font == S32(st, FONT_LARGE2)) {
        space = 10;
        extra = 1;
    } else if (font == S32(st, FONT_SMALL)) {
        space = 4;
        extra = 1;
    } else {
        space = 6;
        extra = 0;
    }
    int32_t width = 0;
    const uint8_t *p = (const uint8_t *) text;
    for (int n = 10000; n > 0; n--, p++) {
        if (*p == 0) {
            return width;
        }
        if (*p == 0x20) {
            width += space;
            continue;
        }
        int32_t glyph = U8(st, GLYPH_WIDTH_MAP + *p);
        S32(st, GLYPH_LAST) = glyph;
        if (glyph != 0) {
            width += S16(st, IMAGES + 0x14u + ((uint32_t) image_index(st, glyph - 1 + font) << 6)) + extra;
        }
    }
    return width;
}

int32_t text_draw_centred_host(game_state *st, const char *text, int32_t x, int32_t y, int32_t width, int32_t font,
                               int32_t colour)
{
    /* 0x004CC720 on a host string */
    int32_t d = (width - text_width_host(st, text, font)) / 2;
    S32(st, 0x00E20770u) = d;
    if (d < 0) {
        d = 0;
        S32(st, 0x00E20770u) = 0;
    }
    return text_draw_host(st, text, x + d, y, font, colour);
}

int32_t text_draw_centred(game_state *st, uint32_t text, int32_t x, int32_t y, int32_t width, int32_t font,
                          int32_t colour)
{
    /* @bin 0x004CC720 the string centred in [x, x + width): the offset (cdq / sar: halved towards 0,
     * at least 0) kept in 0x00E20770 */
    int32_t d = (width - text_width(st, text, font)) / 2;
    S32(st, 0x00E20770u) = d;
    if (d < 0) {
        d = 0;
        S32(st, 0x00E20770u) = 0;
    }
    return text_draw(st, text, x + d, y, font, colour);
}

int32_t number_format(game_state *st, int32_t value, int32_t pos, int32_t plus)
{
    /* @bin 0x004CC5C0 the 256 bytes at 0x00E34238 cleared, then from byte pos: '-' for a negative
     * value, '+' when plus is set, and the digits (1 to 9 by the value's size; none from 10^9 on);
     * returns the index after them */
    for (uint32_t i = 0; i < 0x100; i++) {
        U8(st, NUMBER_BUF + i) = 0;
    }
    int32_t v = value;
    if (v < 0) {
        U8(st, NUMBER_BUF + (uint32_t) pos) = 0x2D;
        v = (int32_t) (0u - (uint32_t) v);
        pos++;
    } else if (plus != 0) {
        U8(st, NUMBER_BUF + (uint32_t) pos) = 0x2B;
        pos++;
    }
    int32_t n;
    if (v < 10) {
        n = 1;
    } else if (v < 100) {
        n = 2;
    } else if (v < 1000) {
        n = 3;
    } else if (v < 10000) {
        n = 4;
    } else if (v < 100000) {
        n = 5;
    } else if (v < 1000000) {
        n = 6;
    } else if (v < 10000000) {
        n = 7;
    } else if (v < 100000000) {
        n = 8;
    } else {
        n = v >= 1000000000 ? 0 : 9;
    }
    int32_t end = n + pos;
    for (int32_t i = n - 1; i >= 0; i--) {
        U8(st, NUMBER_BUF + (uint32_t) (pos + i)) = (uint8_t) (v % 10 + 0x30);
        v /= 10;
    }
    return end;
}

/* the number in the buffer from byte 1, its prefix character (when not 0) at byte 0 and the suffix
 * after them; an empty string when there is no prefix (the suffix overwrites byte 0) */
static void number_with_suffix(game_state *st, int32_t value, int32_t prefix, uint32_t suffix)
{
    number_format(st, value, 1, 0);
    if ((uint8_t) prefix != 0) {
        U8(st, NUMBER_BUF) = (uint8_t) prefix;
    }
    uint32_t n = 0;
    if (U8(st, NUMBER_BUF) != 0) {
        do {
            n++;
        } while (U8(st, NUMBER_BUF + n) != 0);
    }
    for (uint8_t c = mem_rd8(st, suffix); c != 0; c = mem_rd8(st, ++suffix)) {
        U8(st, NUMBER_BUF + n) = c;
        n++;
    }
    U8(st, NUMBER_BUF + n) = 0;
}

int32_t number_draw(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, int32_t x, int32_t y,
                    int32_t font, int32_t colour)
{
    /* @bin 0x004CCE00 */
    number_with_suffix(st, value, prefix, suffix);
    text_draw(st, NUMBER_BUF, x, y, font, colour);
    S32(st, TEXT_FLAG_504) = 0;
    return 0;
}

int32_t number_draw_centred(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, int32_t x, int32_t y,
                            int32_t width, int32_t font, int32_t colour)
{
    /* @bin 0x004CCE90 */
    number_with_suffix(st, value, prefix, suffix);
    text_draw_centred(st, NUMBER_BUF, x, y, width, font, colour);
    S32(st, TEXT_FLAG_504) = 0;
    return 0;
}

int32_t number_draw_between(game_state *st, int32_t value, uint32_t before, uint32_t after, int32_t x, int32_t y,
                            int32_t width, int32_t font, int32_t colour)
{
    /* @bin 0x004CCF20 "before value after" centred: the number written after strlen(before) + 1,
     * then before copied in front, a space on each side of the number */
    uint32_t len = 0;
    if (mem_rd8(st, before) != 0) {
        do {
            len++;
        } while (mem_rd8(st, before + len) != 0);
    }
    int32_t end = number_format(st, value, (int32_t) len + 1, 0);
    uint32_t n = 0;
    for (uint8_t c = mem_rd8(st, before); c != 0; c = mem_rd8(st, before + ++n)) {
        U8(st, NUMBER_BUF + n) = c;
    }
    U8(st, NUMBER_BUF + n) = 0x20;
    U8(st, NUMBER_BUF + (uint32_t) end) = 0x20;
    end++;
    for (uint8_t c = mem_rd8(st, after); c != 0; c = mem_rd8(st, ++after)) {
        U8(st, NUMBER_BUF + (uint32_t) end) = c;
        end++;
    }
    U8(st, NUMBER_BUF + (uint32_t) end) = 0;
    text_draw_centred(st, NUMBER_BUF, x, y, width, font, colour);
    S32(st, TEXT_FLAG_504) = 0;
    return 0;
}

int32_t text_group_draw(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t font,
                        int32_t colour)
{
    /* @bin 0x004CCBB0 string `index` of group `group` of Pharaoh_Text.eng (the seek of 0x004CC110) */
    text_seek(st, group, index);
    text_draw(st, U32(st, TEXT_CURSOR), x, y, font, colour);
    S32(st, TEXT_FLAG_504) = 0;
    return 0;
}

int32_t text_group_draw_centred(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t width,
                                int32_t font, int32_t colour)
{
    /* @bin 0x004CCB30 */
    text_seek(st, group, index);
    text_draw_centred(st, U32(st, TEXT_CURSOR), x, y, width, font, colour);
    S32(st, TEXT_FLAG_504) = 0;
    return 0;
}

int32_t text_group_width(game_state *st, int32_t group, int32_t index, int32_t font)
{
    /* @bin 0x004CC6C0 the width of string `index` of group `group` (the text cursor left on it) */
    text_seek(st, group, index);
    return text_width(st, U32(st, TEXT_CURSOR), font);
}

void text_count_draw(game_state *st, int32_t group, int32_t index, int32_t count, int32_t x, int32_t y, int32_t font,
                     int32_t colour)
{
    /* @bin 0x004CCC30 "count text": the count (prefix ' ', or '-' with its absolute value; suffix
     * the empty string 0x007027D4), then string index (+ 1 unless count is 1 or -1: the plural) */
    int32_t plural = (count == 1 || count == -1) ? 0 : 1;
    S32(st, RUN_X) = 0;
    if (count >= 0) {
        number_draw(st, count, 0x20, 0x007027D4u, x, y, font, colour);
    } else {
        number_draw(st, (int32_t) (0u - (uint32_t) count), 0x2D, 0x007027D4u, x, y, font, colour);
    }
    text_group_draw(st, group, index + plural, S32(st, RUN_X) + x, y, font, colour);
}

void text_money_draw(game_state *st, int32_t kind, int32_t value, int32_t x, int32_t y, int32_t font, int32_t colour)
{
    /* @bin 0x004CCCB0 an amount with string 0 / 1 of group 20: a negative value's (absolute) number
     * first, the string after it (pulled left 8 px for kind 1, 11 for kind 2); otherwise the
     * string first and the number after */
    S32(st, RUN_X) = 0;
    if (value < 0) {
        number_draw(st, (int32_t) (0u - (uint32_t) value), 0x20, 0x0057C048u, x, y, font, colour);
        if (kind == 1) {
            S32(st, RUN_X) -= 8;
        } else if (kind == 2) {
            S32(st, RUN_X) -= 0xB;
        }
        text_group_draw(st, 0x14, 0, S32(st, RUN_X) + x, y, font, colour);
        return;
    }
    text_group_draw(st, 0x14, 1, x, y, font, colour);
    number_draw(st, value, 0x20, 0x0057C048u, S32(st, RUN_X) + x, y, font, colour);
}

void text_pair_draw_centred(game_state *st, uint32_t first, uint32_t second, int32_t x, int32_t y, int32_t width,
                            int32_t font, int32_t colour)
{
    /* @bin 0x004CCD60 "first second" (each when not NULL; at most 255 bytes) centred */
    uint32_t n = 0;
    if (first != 0) {
        uint8_t c = mem_rd8(st, first);
        if (c != 0) {
            uint32_t p = first;
            do {
                if (n >= 0xFF) {
                    break;
                }
                n++;
                U8(st, NUMBER_BUF + (p - first)) = mem_rd8(st, p);
                p++;
            } while (mem_rd8(st, p) != 0);
        }
        U8(st, NUMBER_BUF + n) = 0x20;
        n++;
    }
    if (second != 0 && mem_rd8(st, second) != 0) {
        uint32_t p = second;
        do {
            if (n >= 0xFF) {
                break;
            }
            U8(st, NUMBER_BUF + n) = mem_rd8(st, p);
            n++;
            p++;
        } while (mem_rd8(st, p) != 0);
    }
    U8(st, NUMBER_BUF + n) = 0;
    text_draw_centred(st, NUMBER_BUF, x, y, width, font, colour);
    S32(st, TEXT_FLAG_504) = 0;
}

int32_t glyph_width(game_state *st, int32_t ch, int32_t font)
{
    /* @bin 0x004CC020 the width of character ch (low byte) + 1: 4 for a space, 0 for none */
    uint8_t c = (uint8_t) ch;
    if (c == 0) {
        return 0;
    }
    if (c == 0x20) {
        return 4;
    }
    int32_t glyph = U8(st, GLYPH_WIDTH_MAP + c);
    S32(st, GLYPH_LAST) = glyph;
    if (glyph == 0) {
        return 0;
    }
    int32_t id = glyph + font - 1;
    return S16(st, IMAGES + 0x14u + ((uint32_t) image_index(st, id) << 6)) + 1;
}

static int32_t word_width_any(game_state *st, uint32_t text, const uint8_t *host, int32_t font)
{
    /* @bin 0x004CD120 the width of the next word: leading spaces count 4 each, then the word up to a
     * space or '$' (bytes below 0x20 take no room); the bytes taken in 0x00E20760 */
    int seen = 0;
    int32_t w = 0;
    S32(st, WORD_CHARS) = 0;
    for (int32_t i = 1; i < 2000; i++) {
        uint8_t c = text_at(st, text, host, 0);
        text++;
        if (host) {
            host++;
        }
        if (c == 0) {
            break;
        }
        if (c == 0x20) {
            if (seen) {
                break;
            }
            w += 4;
        } else if (c == 0x24) {
            if (seen) {
                break;
            }
        } else if (c >= 0x20) {
            w += glyph_width(st, c, font);
            seen = 1;
        }
        S32(st, WORD_CHARS) += 1;
    }
    return w;
}

int32_t word_width(game_state *st, uint32_t text, int32_t font)
{
    return word_width_any(st, text, NULL, font);
}

static void text_draw_lines_any(game_state *st, uint32_t text, const uint8_t *host, int32_t x, int32_t y,
                                int32_t width, int32_t font, int32_t colour)
{
    /* @bin 0x004CD010 the text in lines of whole words narrower than width (at most 99 lines), each
     * copied to 0x00E3439C (cleared, 200 bytes) and drawn at the next y (+ line height + 5, also
     * added to 0x00E20774); the widest line in 0x00E2076C. A word wider than the width when the line
     * is empty gives empty lines until the count runs out. */
    int more = 1;
    for (int32_t line = 1; line < 0x64; line++) {
        for (uint32_t i = 0; i < 200; i++) {
            U8(st, LINE_BUF + i) = 0;
        }
        uint32_t n = 0;
        if (more) {
            int32_t w = 0;
            for (;;) {
                if (w >= width) {
                    break;
                }
                w += word_width_any(st, text, host, font);
                if (w < width) {
                    for (int32_t k = S32(st, WORD_CHARS); k > 0; k--) {
                        mem_wr8(st, LINE_BUF + n, text_at(st, text, host, 0));
                        text++;
                        if (host) {
                            host++;
                        }
                        n++;
                    }
                    if (text_at(st, text, host, 0) == 0) {
                        more = 0;
                        break;
                    }
                    continue;
                }
                if (w != 0) {
                    continue;
                }
                more = 0;
                break;
            }
        }
        S32(st, RUN_HASH) = 1;
        S32(st, RUN_X) = 0;
        U8(st, RUN_NO_COLLAPSE) = 1;
        text_draw(st, LINE_BUF, x, y, font, colour);
        if (S32(st, RUN_X) > S32(st, LINE_MAX_W)) {
            S32(st, LINE_MAX_W) = S32(st, RUN_X);
        }
        int32_t h = S32(st, FONT_HEIGHT);
        y += h + 5;
        S32(st, RUN_Y) = S32(st, RUN_Y) + h + 5;
        if (!more) {
            break;
        }
    }
}

void text_draw_lines(game_state *st, uint32_t text, int32_t x, int32_t y, int32_t width, int32_t font, int32_t colour)
{
    text_draw_lines_any(st, text, NULL, x, y, width, font, colour);
}

void text_draw_lines_host(game_state *st, const char *text, int32_t x, int32_t y, int32_t width, int32_t font,
                          int32_t colour)
{
    /* 0x004CD010 on a host string (the original's text on its stack) */
    text_draw_lines_any(st, 0, (const uint8_t *) text, x, y, width, font, colour);
}

void text_group_draw_lines(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t width,
                           int32_t font, int32_t colour)
{
    /* @bin 0x004CCFD0 */
    text_seek(st, group, index);
    text_draw_lines(st, U32(st, TEXT_CURSOR), x, y, width, font, colour);
}

/* ---- in a 640 x 480 layout: a rectangle (i16 left, top, right, bottom) + the layout offset ---- */

static int32_t rect_field(game_state *st, uint32_t rect, int k)
{
    return (int16_t) mem_rd16(st, rect + 2u * (uint32_t) k);
}

void layout_panel(game_state *st, int32_t a1, uint32_t rect, int32_t a3, int32_t dx, int32_t dy)
{
    /* @bin 0x004CD1A0 the panel 0x00536C30 over the rectangle */
    int32_t l = rect_field(st, rect, 0), t = rect_field(st, rect, 1);
    int32_t r = rect_field(st, rect, 2), b = rect_field(st, rect, 3);
    GAME_CALL(st, 0x00536C30u, a1, l + S32(st, LAYOUT_DX) + dx, t + S32(st, LAYOUT_DY) + dy, r - l, b - t, a3);
}

void layout_text_group(game_state *st, int32_t group, int32_t index, uint32_t rect, int32_t font, int32_t colour,
                       int32_t dx, int32_t dy)
{
    /* @bin 0x004CD1F0 */
    text_group_draw(st, group, index, rect_field(st, rect, 0) + S32(st, LAYOUT_DX) + dx,
                    rect_field(st, rect, 1) + S32(st, LAYOUT_DY) + dy, font, colour);
}

void layout_text_group_centred(game_state *st, int32_t group, int32_t index, uint32_t rect, int32_t font,
                               int32_t colour, int32_t dx, int32_t dy)
{
    /* @bin 0x004CD240 centred in the rectangle's width */
    int32_t l = rect_field(st, rect, 0);
    text_group_draw_centred(st, group, index, l + S32(st, LAYOUT_DX) + dx, rect_field(st, rect, 1) + S32(st, LAYOUT_DY) + dy,
                            rect_field(st, rect, 2) - l, font, colour);
}

void layout_number(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, uint32_t rect, int32_t font,
                   int32_t colour, int32_t dx, int32_t dy)
{
    /* @bin 0x004CD290 */
    number_draw(st, value, prefix, suffix, rect_field(st, rect, 0) + S32(st, LAYOUT_DX) + dx,
                rect_field(st, rect, 1) + S32(st, LAYOUT_DY) + dy, font, colour);
}

void layout_number_centred(game_state *st, int32_t value, int32_t prefix, uint32_t suffix, uint32_t rect,
                           int32_t font, int32_t colour, int32_t dx, int32_t dy)
{
    /* @bin 0x004CD2E0 */
    int32_t l = rect_field(st, rect, 0);
    number_draw_centred(st, value, prefix, suffix, l + S32(st, LAYOUT_DX) + dx,
                        rect_field(st, rect, 1) + S32(st, LAYOUT_DY) + dy, rect_field(st, rect, 2) - l, font, colour);
}

void layout_money(game_state *st, int32_t kind, int32_t value, uint32_t rect, int32_t font, int32_t colour,
                  int32_t dx, int32_t dy)
{
    /* @bin 0x004CD340 */
    text_money_draw(st, kind, value, rect_field(st, rect, 0) + S32(st, LAYOUT_DX) + dx,
                    rect_field(st, rect, 1) + S32(st, LAYOUT_DY) + dy, font, colour);
}

void layout_text(game_state *st, uint32_t text, uint32_t rect, int32_t font, int32_t colour, int32_t dx, int32_t dy)
{
    /* @bin 0x004CD390 */
    text_draw(st, text, rect_field(st, rect, 0) + S32(st, LAYOUT_DX) + dx, rect_field(st, rect, 1) + S32(st, LAYOUT_DY) + dy,
              font, colour);
}
