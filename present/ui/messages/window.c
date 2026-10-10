/* Imhotep - the message display window (window 5 with a message, roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004C5CC0 (window 5's full redraw, package wincore) draws a message with 0x004C5E60, or with
 * 0x004C6C70 when it has a video or a big picture (0x00D3B180). The message type's record
 * (0x00E20970 + 0x50 * [0x00D3B124]) gives the layout: the window's place and size, the title,
 * subtitle and body texts, pictures; a message of the list (0x00D3B184) may replace the texts by
 * expanded strings (0x00448330), which the original keeps on its stack: here host buffers. */
#include "messages.h"

#include "draw/draw.h"
#include "draw/mem.h"
#include "draw/text.h"
#include "present_layout.h"
#include "sim/call.h"
#include "state/ported.h"


/* @bin the message window's globals (0x004C5E60, 0x004C6C70) */


static uint32_t type_rec(game_state *st)
{
    return MESSAGE_TYPES + 0x50u * (uint32_t) S32(st, G_MW_TYPE);
}

static int32_t tf(game_state *st, uint32_t off)
{
    return S16(st, type_rec(st) + off);
}

static uint32_t msg_rec(game_state *st)
{
    return MESSAGE_BASE + 0x30u * (uint32_t) S32(st, G_MESSAGE_CURRENT);
}

/* a text: the state's (Pharaoh_Text.eng) or an expanded copy (the original's stack buffer) */
typedef struct {
    uint32_t addr;
    const char *host;
} text_ref;

static void draw_text(game_state *st, text_ref t, int32_t x, int32_t y, int32_t font, int32_t colour)
{
    if (t.host) {
        text_draw_host(st, t.host, x, y, font, colour);
    } else {
        GAME_CALL(st, FN_TEXT_DRAW, (int32_t) t.addr, x, y, font, colour);
    }
}

static void draw_centred(game_state *st, text_ref t, int32_t x, int32_t y, int32_t w, int32_t font, int32_t colour)
{
    if (t.host) {
        text_draw_centred_host(st, t.host, x, y, w, font, colour);
    } else {
        GAME_CALL(st, FN_TEXT_DRAW_CENTRED, (int32_t) t.addr, x, y, w, font, colour);
    }
}


static int32_t word_width_host(game_state *st, const char *t, int32_t font)
{
    /* 0x004CD120 on a host string (word_width, present/draw/text.c) */
    int seen = 0;
    int32_t w = 0;
    S32(st, G_WORD_CHARS) = 0;
    for (int32_t i = 1; i < 2000; i++) {
        uint8_t c = (uint8_t) *t++;
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
        S32(st, G_WORD_CHARS) += 1;
    }
    return w;
}

static void draw_lines(game_state *st, text_ref t, int32_t x, int32_t y, int32_t width, int32_t font, int32_t colour)
{
    if (!t.host) {
        GAME_CALL(st, FN_TEXT_DRAW_LINES, (int32_t) t.addr, x, y, width, font, colour);
        return;
    }
    /* @bin 0x004CD010 (text_draw_lines, present/draw/text.c) on a host string */
    const char *p = t.host;
    int more = 1;
    for (int32_t line = 1; line < 0x64; line++) {
        for (uint32_t i = 0; i < 200; i++) {
            U8(st, G_LINE_BUF + i) = 0;
        }
        uint32_t n = 0;
        if (more) {
            int32_t w = 0;
            for (;;) {
                if (w >= width) {
                    break;
                }
                w += word_width_host(st, p, font);
                if (w < width) {
                    for (int32_t k = S32(st, G_WORD_CHARS); k > 0; k--) {
                        mem_wr8(st, G_LINE_BUF + n, (uint8_t) *p);
                        p++;
                        n++;
                    }
                    if (*p == 0) {
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
        S32(st, G_RUN_HASH) = 1;
        S32(st, G_TEXT_X) = 0;
        U8(st, G_RUN_NO_COLLAPSE) = 1;
        text_draw(st, G_LINE_BUF, x, y, font, colour);
        if (S32(st, G_TEXT_X) > S32(st, G_LINE_MAX_W)) {
            S32(st, G_LINE_MAX_W) = S32(st, G_TEXT_X);
        }
        int32_t h = S32(st, G_FONT_HEIGHT);
        y += h + 5;
        S32(st, G_TEXT_Y) = S32(st, G_TEXT_Y) + h + 5;
        if (!more) {
            break;
        }
    }
}

static void draw_rich(game_state *st, text_ref t, int32_t x, int32_t y, int32_t w, int32_t measure, int32_t raised)
{
    if (t.host) {
        rich_text_draw(st, (const uint8_t *) t.host, x, y, w, measure, raised);
        return;
    }
    static uint8_t buf[0x10000 + 16];
    uint32_t n = 0;
    while (n < 0x10000) {
        uint8_t c = mem_rd8(st, t.addr + n);
        buf[n++] = c;
        if (c == 0) {
            break;
        }
    }
    for (uint32_t k = 0; k < 16; k++) {
        buf[n + k] = 0;
    }
    rich_text_draw(st, buf, x, y, w, measure, raised);
}

static void draw_image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, id, x, y);
}

void message_reply_buttons(game_state *st)
{
    /* @bin 0x004C5CE0 the reply buttons offered (bits of 0x00D3B186: 4, 1, 2), each a label of group
     * 0x2C (0xCF, 0xD0, 0xD1) in its rectangle (0x005E02F8, 0x005E02C0, 0x005E02DC) and its frame,
     * lit when hovered (0x007056C8 = 3, 1, 2) */
    static const struct {
        uint8_t bit;
        int32_t index;
        uint32_t rect;
        int32_t hover;
    } b[3] = {{4, 0xCF, G_REPLY_RECT_4, 3}, {1, 0xD0, G_REPLY_RECT_1, 1}, {2, 0xD1, G_REPLY_RECT_2, 2}};
    for (int k = 0; k < 3; k++) {
        if ((U8(st, G_MESSAGE_WINDOW_REPLIES) & b[k].bit) == 0) {
            continue;
        }
        GAME_CALL(st, FN_LAYOUT_TEXT_GROUP_CENTRED, 0x2C, b[k].index, (int32_t) b[k].rect, S32(st, G_FONT_LARGE), 0, tf(st, 0x4E) + 4,
                  tf(st, 0x50) + 4);
        GAME_CALL(st, FN_LAYOUT_PANEL, 1, (int32_t) b[k].rect, S32(st, G_HOVER) == b[k].hover, tf(st, 0x4E),
                  tf(st, 0x50));
    }
}

void message_scroll_thumb(game_state *st)
{
    /* @bin 0x004C72B0 the body's scroll thumb when it scrolls */
    int32_t max = S32(st, G_MESSAGE_WINDOW_SCROLL_MAX);
    if (max <= 0) {
        return;
    }
    int32_t s = S32(st, G_SCROLL);
    int32_t pct;
    if (s <= 0) {
        pct = 0;
    } else if (s >= max) {
        pct = 100;
    } else {
        pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, s, max);
    }
    int32_t off = GAME_CALL(st, FN_CALC_PERCENTAGE, (S32(st, G_MESSAGE_WINDOW_BOX_H) << 4) - 0x4D, pct);
    if (S32(st, G_MW_DRAGGING) != 0) {
        off = S32(st, G_MW_THUMB_Y);
    }
    draw_image(st, S32(st, G_SCROLL_IMAGES) + 0x27, (S32(st, G_MESSAGE_WINDOW_BOX_W) << 4) + S32(st, G_MESSAGE_WINDOW_BOX_X) + 6,
               S32(st, G_MESSAGE_WINDOW_BOX_Y) + off + 0x1A);
}

void panel_draw_pattern(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x00536990 w x h pieces of the patterned panel (images 0x0D.. of [0x00F5683C]: 10 per row,
     * the rows shifted by 12, wrapping at 0x78) */
    int32_t row_shift = 0;
    int32_t step = 0;
    if (h <= 0) {
        return;
    }
    int32_t yy = y;
    for (int32_t r = h; r != 0; r--) {
        int32_t col = 0;
        if (w > 0) {
            step = 0x0C;
            int32_t xx = x;
            for (int32_t c = w; c != 0; c--) {
                int32_t id = S32(st, G_PANEL_IMAGES) + col + row_shift + 0x0D;
                S32(st, G_BLIT_X) = xx;
                S32(st, G_BLIT_Y) = yy;
                S32(st, G_GLYPH_LAST) = id;
                col++;
                draw_image(st, id, xx, yy);
                if (col >= 10) {
                    col = 0;
                }
                xx += 0x10;
            }
        }
        row_shift += step;
        if (row_shift >= 0x78) {
            row_shift = 0;
        }
        yy += 0x10;
    }
}

/* the request part of a message (kind 2 of an event): amount, resource, months left */
static int32_t amount_scaled(int32_t res, int32_t amount)
{
    /* @bin 0x004C6A43 jump table 0x004C6C44: amounts in hundreds but for resources 0x18-0x1A, 0x1E,
     * 0x23, 0x1C, 0x0A, 0x24 */
    switch (res) {
    case 0x18: case 0x19: case 0x1A: case 0x1E: case 0x23: case 0x1C: case 0x0A: case 0x24:
        return amount;
    default:
        return amount * 100;
    }
}

static int32_t resource_text_offset(int32_t res, int32_t amount)
{
    /* @bin 0x004C6AFB jump table 0x004C6C58: the plural names (+0x36) of group 0x17 */
    switch (res) {
    case 0x18: case 0x19: case 0x1A: case 0x1E: case 0x23:
        return 0x36;
    case 0x1C: case 0x0A:
        return amount > 1 ? 0x36 : 0;
    default:
        return 0;
    }
}

static int32_t crt_atol(game_state *st, uint32_t p)
{
    /* @bin 0x00561C0B CRT atol (0x00561C96 atoi): spaces skipped (ctype bit 8 of [0x00604C58]),
     * a sign, digits (bit 4), wrapping; the multi-byte path ([0x00604E64] > 1, 0x00565F66) is not
     * ported */
    if (S32(st, G_CRT_MB_CUR_MAX) > 1) {
        state_unported(st, 0x00565F66u);
        return 0;
    }
    uint32_t ctype = U32(st, G_CRT_CTYPE);
    while (mem_rd8(st, ctype + 2u * mem_rd8(st, p)) & 8) {
        p++;
    }
    uint32_t c = mem_rd8(st, p++);
    uint32_t sign = c;
    if (c == 0x2D || c == 0x2B) {
        c = mem_rd8(st, p++);
    }
    uint32_t v = 0;
    while (mem_rd8(st, ctype + 2u * c) & 4) {
        v = c + v * 10u - 0x30;
        c = mem_rd8(st, p++);
    }
    return (int32_t) (sign == 0x2D ? 0u - v : v);
}

void message_window_draw(game_state *st)
{
    /* @bin 0x004C5E60 */
    static char body_buf[1001], title_buf[201], sub_buf[201];
    uint32_t base = U32(st, G_MM_TEXT);
    uint32_t t = type_rec(st);
    text_ref body = {U32(st, t + 0x94) + base, NULL};
    text_ref title = {U32(st, t + 0x8C) + base, NULL};
    text_ref sub = {U32(st, t + 0x90) + base, NULL};
    if (U8(st, G_MW_FROM_LIST) != 0) {
        int16_t k = S16(st, msg_rec(st) + 0x10);
        if (k >= 0) {
            message_text_expand(st, k, body_buf, 1000);
            body.host = body_buf;
        }
        k = S16(st, msg_rec(st) + 0x12);
        if (k >= 0) {
            message_text_expand(st, k, title_buf, 200);
            title.host = title_buf;
        }
        k = S16(st, msg_rec(st) + 0x14);
        if (k >= 0) {
            message_text_expand(st, k, sub_buf, 200);
            sub.host = sub_buf;
        }
    }
    S32(st, G_MESSAGE_WINDOW_FONT_LINK) = S32(st, G_FONT_LINK);
    S32(st, G_MESSAGE_WINDOW_FONT_TEXT) = S32(st, G_FONT_9C);
    S32(st, G_MESSAGE_WINDOW_X) = tf(st, 0x4E) + S32(st, G_LAYOUT_DX);
    S32(st, G_MESSAGE_WINDOW_Y) = tf(st, 0x50) + S32(st, G_LAYOUT_DY);
    if (S32(st, G_MW_NO_CITY) == 0) {
        GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
        if (U8(st, G_REDRAW) == 1) {
            GAME_CALL0(st, FN_SIDEBAR_DRAW);
        }
    }
    S32(st, G_MW_FOOT) = 0x30;
    if (tf(st, 0x48) != 0) {
        S32(st, G_MW_FOOT) = 0x20;
    }
    S32(st, G_MESSAGE_WINDOW_BOX_X) = S32(st, G_MESSAGE_WINDOW_X) + 0x10;
    GAME_CALL(st, FN_PANEL_DRAW, S32(st, G_MESSAGE_WINDOW_X), S32(st, G_MESSAGE_WINDOW_Y), tf(st, 0x52), tf(st, 0x54));
    /* the title */
    if (tf(st, 0x62) == 0) {
        int32_t ty = tf(st, 0x4A) == 4 ? tf(st, 0x64) + S32(st, G_MESSAGE_WINDOW_Y) : S32(st, G_MESSAGE_WINDOW_Y) + 0x0E;
        draw_centred(st, title, S32(st, G_MESSAGE_WINDOW_X), ty, tf(st, 0x52) << 4, S32(st, G_FONT_TITLE), 0);
        S32(st, G_MESSAGE_WINDOW_BOX_Y) = S32(st, G_MESSAGE_WINDOW_Y) + 0x30;
    } else {
        draw_text(st, title, tf(st, 0x62) + S32(st, G_MESSAGE_WINDOW_X), tf(st, 0x64) + S32(st, G_MESSAGE_WINDOW_Y), S32(st, G_FONT_TITLE), 0);
        S32(st, G_MESSAGE_WINDOW_BOX_Y) = S32(st, G_MESSAGE_WINDOW_Y) + 0x20;
    }
    /* the first picture: the type's (0: the god / overseer portrait), from 1001 the second set */
    int16_t pic = (int16_t) tf(st, 0x56);
    if (pic != 0) {
        if (S32(st, G_MW_TYPE) == 0) {
            S16(st, t + 0x58) = 0x10;
            S16(st, t + 0x5A) = 0x10;
            if (S32(st, G_U978D7C) == 2) {
                draw_image(st, S32(st, G_STATUS_IMAGE), S32(st, G_MESSAGE_WINDOW_X) + 0x10, S32(st, G_MESSAGE_WINDOW_Y) + 0x10);
                panel_draw_pattern(st, tf(st, 0x58) + S32(st, G_MESSAGE_WINDOW_X) + 0x1E, tf(st, 0x5A) + S32(st, G_MESSAGE_WINDOW_Y) + 0x28, 2, 1);
            } else {
                draw_image(st, S32(st, G_PORTRAIT_IMAGES), S32(st, G_MESSAGE_WINDOW_X) + 0x10, S32(st, G_MESSAGE_WINDOW_Y) + 0x10);
            }
        } else if (pic < 1000) {
            draw_image(st, pic + S32(st, G_PICTURES) - 1, tf(st, 0x58) + S32(st, G_MESSAGE_WINDOW_X), tf(st, 0x5A) + S32(st, G_MESSAGE_WINDOW_Y));
        } else {
            draw_image(st, pic + S32(st, G_PICTURES2) - 0x3E9, tf(st, 0x58) + S32(st, G_MESSAGE_WINDOW_X), tf(st, 0x5A) + S32(st, G_MESSAGE_WINDOW_Y));
        }
        int32_t bottom = S16(st, G_UD3B5BC) + S32(st, G_MESSAGE_WINDOW_Y) + tf(st, 0x5A) + 8;
        if (bottom > S32(st, G_MESSAGE_WINDOW_BOX_Y)) {
            S32(st, G_MESSAGE_WINDOW_BOX_Y) = bottom;
        }
    }
    int16_t pic2 = (int16_t) tf(st, 0x5C);
    if (pic2 != 0) {
        draw_image(st, pic2 + S32(st, G_PICTURES) - 1, tf(st, 0x5E) + S32(st, G_MESSAGE_WINDOW_X), tf(st, 0x60) + S32(st, G_MESSAGE_WINDOW_Y));
        int32_t bottom = S16(st, G_UD3B5BC) + S32(st, G_MESSAGE_WINDOW_Y) + tf(st, 0x60) + 8;
        if (bottom > S32(st, G_MESSAGE_WINDOW_BOX_Y)) {
            S32(st, G_MESSAGE_WINDOW_BOX_Y) = bottom;
        }
    }
    /* the subtitle */
    int16_t sx = (int16_t) tf(st, 0x66);
    if (sx != 0) {
        int32_t w = (tf(st, 0x52) << 4) - sx - 0x10;
        S32(st, G_TEXT_Y) = 0;
        S32(st, G_MW_SUB_W) = w;
        draw_lines(st, sub, sx + S32(st, G_MESSAGE_WINDOW_X), tf(st, 0x68) + S32(st, G_MESSAGE_WINDOW_Y), w, S32(st, G_FONT_LARGE), 0);
        int32_t bottom = tf(st, 0x68) + S32(st, G_TEXT_Y) + S32(st, G_MESSAGE_WINDOW_Y);
        if (bottom > S32(st, G_MESSAGE_WINDOW_BOX_Y)) {
            S32(st, G_MESSAGE_WINDOW_BOX_Y) = bottom + 4;
        }
    }
    /* the body box: measured once */
    if (S32(st, G_TEXT_LINES) == 0) {
        int32_t used = S32(st, G_MESSAGE_WINDOW_BOX_Y) - S32(st, G_MESSAGE_WINDOW_Y) + S32(st, G_MW_FOOT);
        U8(st, G_ARROW_UP_SHOWN) = 1;
        int32_t h = tf(st, 0x54) - ((used + ((used >> 31) & 0xF)) >> 4);
        U8(st, G_ARROW_DOWN_SHOWN) = 1;
        S32(st, G_MESSAGE_WINDOW_BOX_H) = h - 1;
        S32(st, G_MESSAGE_WINDOW_VISIBLE) = h - 2;
        S32(st, G_MESSAGE_WINDOW_BOX_W) = tf(st, 0x52) - 4;
        draw_rich(st, body, S32(st, G_MESSAGE_WINDOW_BOX_X) + 8, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 6, (S32(st, G_MESSAGE_WINDOW_BOX_W) << 4) - 0x10, 1, 0);
        if (S32(st, G_TEXT_LINES) > S32(st, G_MESSAGE_WINDOW_VISIBLE)) {
            S32(st, G_MESSAGE_WINDOW_SCROLL_MAX) = S32(st, G_TEXT_LINES) - S32(st, G_MESSAGE_WINDOW_VISIBLE);
        } else {
            S32(st, G_MESSAGE_WINDOW_SCROLL_MAX) = 0;
            U8(st, G_ARROW_UP_SHOWN) = 0;
            U8(st, G_ARROW_DOWN_SHOWN) = 0;
            S32(st, G_MESSAGE_WINDOW_BOX_W) = tf(st, 0x52) - 2;
        }
        U8(st, G_REDRAW) = 1;
    }
    if (S32(st, G_TEXT_LINES) > S32(st, G_MESSAGE_WINDOW_VISIBLE)) {
        GAME_CALL(st, FN_PANEL_DRAW_INNER, (tf(st, 0x52) << 4) + S32(st, G_MESSAGE_WINDOW_X) - 0x2E, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x1A, 2,
                  S32(st, G_MESSAGE_WINDOW_BOX_H) - 2);
    }
    if (S32(st, G_MESSAGE_WINDOW_VIDEO) != 0) {
        GAME_CALL(st, FN_RECT_FILL, tf(st, 0x6E) + S32(st, G_MESSAGE_WINDOW_X), tf(st, 0x70) + S32(st, G_MESSAGE_WINDOW_Y), 200, 200, 0xF800);
    }
    GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_MESSAGE_WINDOW_BOX_X), S32(st, G_MESSAGE_WINDOW_BOX_Y), S32(st, G_MESSAGE_WINDOW_BOX_W), S32(st, G_MESSAGE_WINDOW_BOX_H));
    S32(st, G_CLIP_RIGHT) = (S32(st, G_MESSAGE_WINDOW_BOX_W) << 4) + S32(st, G_MESSAGE_WINDOW_BOX_X) - 3;
    S32(st, G_CLIP_LEFT) = S32(st, G_MESSAGE_WINDOW_BOX_X) + 3;
    S32(st, G_CLIP_TOP) = S32(st, G_MESSAGE_WINDOW_BOX_Y) + 3;
    S32(st, G_CLIP_BOTTOM) = (S32(st, G_MESSAGE_WINDOW_BOX_H) << 4) + S32(st, G_MESSAGE_WINDOW_BOX_Y) - 3;
    GAME_CALL0(st, FN_MESSAGE_WINDOW_LINES_CLEAR);
    int32_t width = (S32(st, G_MESSAGE_WINDOW_BOX_W) << 4) - 0x10;
    if (tf(st, 0x48) != 2) {
        draw_rich(st, body, S32(st, G_MESSAGE_WINDOW_BOX_X) + 8, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 6, width, 0, 0);
    } else {
        int32_t font = S32(st, G_FONT_9C);
        if (tf(st, 0x4A) != 4) {
            /* the date, then the city / god / "from" line */
            S32(st, G_TEXT_X) = 0;
            GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x19, S32(st, G_MESSAGE_WINDOW_MONTH), S32(st, G_MESSAGE_WINDOW_BOX_X) + 10, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 6, font, 0);
            GAME_CALL(st, FN_TEXT_MONEY_DRAW, 0, S32(st, G_MESSAGE_WINDOW_YEAR), S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_BOX_X) + 0x0C,
                      S32(st, G_MESSAGE_WINDOW_BOX_Y) + 6, S32(st, G_FONT_9C), 0);
            if (tf(st, 0x4A) == 1 && S32(st, G_MESSAGE_WINDOW_PARAM_A) != 0) {
                if (S32(st, G_MW_TYPE) == 0xFB) {
                    GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 0, S32(st, G_MESSAGE_WINDOW_PARAM_A), S32(st, G_MESSAGE_WINDOW_X) + 0xF0, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 6,
                              S32(st, G_FONT_9C), 0);
                } else {
                    GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x29, S32(st, G_MESSAGE_WINDOW_PARAM_A), S32(st, G_MESSAGE_WINDOW_X) + 0xF0, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 6,
                              S32(st, G_FONT_9C), 0);
                }
            } else {
                GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x3F, 5, S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_BOX_X) + 0x50, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 6,
                          S32(st, G_FONT_9C), 0);
                int32_t x = S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_BOX_X) + 0x50;
                int32_t y = S32(st, G_MESSAGE_WINDOW_BOX_Y) + 6;
                int32_t f = S32(st, G_FONT_9C);
                int32_t s = GAME_CALL(st, FN_ADVISOR_KING_NAME, x, y, f, 0);
                GAME_CALL(st, FN_TEXT_DRAW, s, x, y, f, 0);
            }
        }
        S32(st, G_TEXT_Y) = 0;
        int16_t kind = (int16_t) tf(st, 0x4A);
        if (kind == 1 || kind == 7) {
            GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x0C, 1, S32(st, G_MESSAGE_WINDOW_X) + 0x64, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x2C, S32(st, G_FONT_9C), 0);
            draw_rich(st, body, S32(st, G_MESSAGE_WINDOW_BOX_X) + 8, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x56, (S32(st, G_MESSAGE_WINDOW_BOX_W) << 4) - 0x10, 0, 0);
        } else if (kind == 3) {
            /* @bin 0x004C6667 the city's +0x4540 (rank 1..5) names it */
            int32_t r = S32(st, CITY_BASE + CITY_SENTIMENT_LOW + 0x49D8u * U32(st, CURRENT_PLAYER_ADDR));
            if (r >= 1 && r <= 5) {
                GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x0C, r + 2, S32(st, G_MESSAGE_WINDOW_X) + 0x40, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x2C,
                          S32(st, G_FONT_9C), 0);
            }
            draw_rich(st, body, S32(st, G_MESSAGE_WINDOW_BOX_X) + 8, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x56, (S32(st, G_MESSAGE_WINDOW_BOX_W) << 4) - 0x10, 0, 0);
        } else if (kind == 4) {
            draw_rich(st, body, S32(st, G_MESSAGE_WINDOW_BOX_X) + 8, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 6, (S32(st, G_MESSAGE_WINDOW_BOX_W) << 4) - 0x10, 0, 0);
        } else if (kind == 5 || kind == 6) {
            int32_t iy = S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x28;
            int32_t ix = S32(st, G_MESSAGE_WINDOW_X) + 0x40;
            int32_t icons = S32(st, G_RESOURCE_ICONS);
            int32_t o = GAME_CALL(st, FN_RETURN_ZERO, S32(st, G_MESSAGE_WINDOW_PARAM_B), 3);
            draw_image(st, o + icons + S32(st, G_MESSAGE_WINDOW_PARAM_B), ix, iy);
            if (kind == 5) {
                int32_t group = U8(st, G_EMPIRE_LANGUAGE) != 0 ? 0x15 : 0xC3;
                GAME_CALL(st, FN_TEXT_GROUP_DRAW, group, (int32_t) U8(st, G_EMPIRE_CITY_NAME + 0x6Au * (uint32_t) S32(st, G_MESSAGE_WINDOW_PARAM_A)),
                          S32(st, G_MESSAGE_WINDOW_X) + 0x64, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x2C, S32(st, G_FONT_9C), 0);
            } else {
                GAME_CALL(st, FN_NUMBER_DRAW, S32(st, G_MESSAGE_WINDOW_PARAM_A), 0x40, G_STR_SPACE_DB, S32(st, G_MESSAGE_WINDOW_X) + 0x64,
                          S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x2C, S32(st, G_FONT_9C), 0);
            }
            draw_rich(st, body, S32(st, G_MESSAGE_WINDOW_BOX_X) + 8, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x56, (S32(st, G_MESSAGE_WINDOW_BOX_W) << 4) - 0x10, 0, 0);
        } else {
            draw_rich(st, body, S32(st, G_MESSAGE_WINDOW_BOX_X) + 8, S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x38, (S32(st, G_MESSAGE_WINDOW_BOX_W) << 4) - 0x10, 0, 0);
        }
        /* a request: amount, resource and the months left */
        int32_t ev = S32(st, G_MESSAGE_WINDOW_PARAM_A);
        if (tf(st, 0x4A) == 2 && ev >= 0 && ev < S32(st, G_EVENTS)) {
            uint32_t e = G_EVENT_RECORDS + 0x7Cu * (uint32_t) ev;
            uint32_t m = msg_rec(st);
            int32_t amount = S16(st, m + 0x18);
            int32_t res = S16(st, m + 0x1A);
            int32_t months = S16(st, m + 0x1C);
            if (res != 0x25) {
                S32(st, G_TEXT_X) = 0;
                amount = amount_scaled(res, amount);
                int32_t icon = res == 0x24 ? S32(st, G_RESOURCE_ICONS) + 0x15 : S32(st, G_RESOURCE_ICONS) + res;
                draw_image(st, icon, S32(st, G_MESSAGE_WINDOW_BOX_X) + 8, S32(st, G_TEXT_Y) + S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x51);
                GAME_CALL(st, FN_NUMBER_DRAW, amount, 0x40, G_NUMBER_SUFFIX, S32(st, G_MESSAGE_WINDOW_BOX_X) + 0x14,
                          S32(st, G_TEXT_Y) + S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x56, S32(st, G_FONT_9C), 0);
                GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x17, res + resource_text_offset(res, amount),
                          S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_BOX_X) + 0x14, S32(st, G_TEXT_Y) + S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x56,
                          S32(st, G_FONT_9C), 0);
                if (U8(st, e + 2) == 1 && S16(st, e + 0x30) < 2) {
                    S32(st, G_TEXT_X) = 0;
                    GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 4, months, S32(st, G_MESSAGE_WINDOW_BOX_X) + 0xDC,
                              S32(st, G_TEXT_Y) + S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x56, S32(st, G_FONT_9C), 0);
                    GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x0C, 2, S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_BOX_X) + 0xDC,
                              S32(st, G_TEXT_Y) + S32(st, G_MESSAGE_WINDOW_BOX_Y) + 0x56, S32(st, G_FONT_9C), 0);
                }
            }
        }
        message_reply_buttons(st);
    }
    S32(st, G_CLIP_RIGHT) = S32(st, G_SCREEN_W);
    S32(st, G_CLIP_LEFT) = 0;
    S32(st, G_CLIP_TOP) = 0;
    S32(st, G_CLIP_BOTTOM) = S32(st, G_SCREEN_H);
    message_scroll_thumb(st);
}

void message_window_draw_video(game_state *st)
{
    /* @bin 0x004C6C70 the video layout: the video (or, for a name starting with '@', big picture n)
     * in a 0x1D2 x 0xEF frame, the subtitle, the date and "from" line, the body, a request's line */
    static char body_buf[1001], title_buf[201], sub_buf[201];
    uint32_t base = U32(st, G_MM_TEXT);
    uint32_t t = type_rec(st);
    text_ref body = {U32(st, t + 0x94) + base, NULL};
    text_ref title = {U32(st, t + 0x8C) + base, NULL};
    int has_video = mem_rd8(st, U32(st, t + 0x84) + base) != 0x40;
    if (U8(st, G_MW_FROM_LIST) != 0) {
        int16_t k = S16(st, msg_rec(st) + 0x10);
        if (k >= 0) {
            message_text_expand(st, k, body_buf, 1000);
            body.host = body_buf;
        }
        k = S16(st, msg_rec(st) + 0x12);
        if (k >= 0) {
            message_text_expand(st, k, title_buf, 200);
            title.host = title_buf;
        }
        k = S16(st, msg_rec(st) + 0x14);
        if (k >= 0) {
            message_text_expand(st, k, sub_buf, 200);
        }
    }
    S32(st, G_MESSAGE_WINDOW_FONT_TEXT) = S32(st, G_FONT_9C);
    S32(st, G_MESSAGE_WINDOW_FONT_LINK) = S32(st, G_FONT_RED);
    S32(st, G_MESSAGE_WINDOW_X) = S32(st, G_LAYOUT_DX);
    S32(st, G_MESSAGE_WINDOW_Y) = S32(st, G_LAYOUT_DY) + 0x1C;
    if (S32(st, G_MW_NO_CITY) == 0) {
        GAME_CALL0(st, FN_CITY_VIEW_DRAW_CURRENT);
        if (U8(st, G_REDRAW) == 1) {
            GAME_CALL0(st, FN_SIDEBAR_DRAW);
        }
    }
    GAME_CALL(st, FN_PANEL_DRAW, S32(st, G_MESSAGE_WINDOW_X), S32(st, G_MESSAGE_WINDOW_Y), 0x1E, 0x1C);
    GAME_CALL(st, FN_RECT_DRAW, S32(st, G_MESSAGE_WINDOW_X) + 7, S32(st, G_MESSAGE_WINDOW_Y) + 9, 0x1D2, 0xEF, 0);
    GAME_CALL0(st, FN_MESSAGE_WINDOW_LINES_CLEAR);
    if (has_video) {
        /* the video: a platform request (0x00538DC0) */
        GAME_CALL(st, FN_UI_VIDEO_OPEN, (int32_t) (U32(st, t + 0x84) + base), S32(st, G_MESSAGE_WINDOW_X) + 8, S32(st, G_MESSAGE_WINDOW_Y) + 8, 0,
                  (int32_t) U8(st, G_WINDOW));
    } else {
        int32_t id = crt_atol(st, U32(st, t + 0x84) + base + 1) + S32(st, G_PICTURES_BIG) - 1;
        int32_t w = 0, h = 0;
        if ((uint8_t) image_size_on(st, G_GRAPHICS_GFX, id, &w, &h) != 0) {
            draw_image(st, id, (0x1D2 - w) / 2 + S32(st, G_MESSAGE_WINDOW_X) + 8, (0xEF - h) / 2 + S32(st, G_MESSAGE_WINDOW_Y) + 8);
        }
    }
    GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_MESSAGE_WINDOW_X) + 8, S32(st, G_MESSAGE_WINDOW_Y) + 0xF5, 0x1D, 10);
    draw_centred(st, title, S32(st, G_MESSAGE_WINDOW_X) + 8, S32(st, G_MESSAGE_WINDOW_Y) + 0x19E, 0x1D0, S32(st, G_FONT_LARGE), 0);
    S32(st, G_TEXT_X) = 0;
    GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x19, S32(st, G_MESSAGE_WINDOW_MONTH), S32(st, G_MESSAGE_WINDOW_X) + 0x10, S32(st, G_MESSAGE_WINDOW_Y) + 0x101, S32(st, G_FONT_9C), 0);
    GAME_CALL(st, FN_TEXT_MONEY_DRAW, 0, S32(st, G_MESSAGE_WINDOW_YEAR), S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_X) + 0x12, S32(st, G_MESSAGE_WINDOW_Y) + 0x101,
              S32(st, G_FONT_9C), 0);
    if (tf(st, 0x48) == 2 && tf(st, 0x4A) == 1 && S32(st, G_MW_TYPE) == 0xFB) {
        GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 0, S32(st, G_MESSAGE_WINDOW_PARAM_A), S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_X) + 0x5A, S32(st, G_MESSAGE_WINDOW_Y) + 0x101,
                  S32(st, G_FONT_9C), 0);
    } else {
        GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x3F, 5, S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_X) + 0x5A, S32(st, G_MESSAGE_WINDOW_Y) + 0x101,
                  S32(st, G_FONT_9C), 0);
        int32_t x = S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_X) + 0x5A;
        int32_t y = S32(st, G_MESSAGE_WINDOW_Y) + 0x101;
        int32_t f = S32(st, G_FONT_9C);
        int32_t s = GAME_CALL(st, FN_ADVISOR_KING_NAME, x, y, f, 0);
        GAME_CALL(st, FN_TEXT_DRAW, s, x, y, f, 0);
    }
    int32_t used = S32(st, G_MW_FOOT) - S32(st, G_MESSAGE_WINDOW_Y) + S32(st, G_MESSAGE_WINDOW_BOX_Y);
    int32_t h = tf(st, 0x54) - ((used + ((used >> 31) & 0xF)) >> 4);
    S32(st, G_TEXT_Y) = 0;
    S32(st, G_MESSAGE_WINDOW_BOX_H) = h - 1;
    S32(st, G_MESSAGE_WINDOW_VISIBLE) = h - 2;
    S32(st, G_MESSAGE_WINDOW_BOX_W) = tf(st, 0x52) - 4;
    draw_rich(st, body, S32(st, G_MESSAGE_WINDOW_X) + 0x10, S32(st, G_MESSAGE_WINDOW_Y) + 0x115, 0x1E0, 0, 0);
    if (tf(st, 0x48) == 2 && tf(st, 0x4A) == 2) {
        int32_t ev = S32(st, G_MESSAGE_WINDOW_PARAM_A);
        uint32_t e = (ev >= 0 && ev < S32(st, G_EVENTS)) ? G_EVENT_RECORDS + 0x7Cu * (uint32_t) ev : 0;
        uint32_t m = msg_rec(st);
        int32_t res = S16(st, m + 0x1A);
        int32_t amount = amount_scaled(res, S16(st, m + 0x18));
        int32_t months = S16(st, m + 0x1C);
        GAME_CALL(st, FN_NUMBER_DRAW, amount, 0x40, G_NUMBER_SUFFIX, S32(st, G_MESSAGE_WINDOW_X) + 8, S32(st, G_MESSAGE_WINDOW_Y) + 0x180,
                  S32(st, G_FONT_9C), 0);
        int32_t icon = res == 0x24 ? S32(st, G_RESOURCE_ICONS) + 0x15 : S32(st, G_RESOURCE_ICONS) + res;
        draw_image(st, icon, S32(st, G_MESSAGE_WINDOW_X) + 0x46, S32(st, G_MESSAGE_WINDOW_Y) + 0x17B);
        GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x17, res + resource_text_offset(res, amount), S32(st, G_MESSAGE_WINDOW_X) + 0x64,
                  S32(st, G_MESSAGE_WINDOW_Y) + 0x180, S32(st, G_FONT_9C), 0);
        /* @bin 0x004C7204 the event's +0x30 (an event out of range: address 0x30, as the original) */
        if ((int16_t) mem_rd16(st, e + 0x30) < 2) {
            S32(st, G_TEXT_X) = 0;
            GAME_CALL(st, FN_TEXT_COUNT_DRAW, 8, 4, months, S32(st, G_MESSAGE_WINDOW_X) + 0xFA, S32(st, G_MESSAGE_WINDOW_Y) + 0x180, S32(st, G_FONT_9C), 0);
            GAME_CALL(st, FN_TEXT_GROUP_DRAW, 0x0C, 2, S32(st, G_TEXT_X) + S32(st, G_MESSAGE_WINDOW_X) + 0xFA, S32(st, G_MESSAGE_WINDOW_Y) + 0x180,
                      S32(st, G_FONT_9C), 0);
        }
    }
    GAME_CALL0(st, FN_MESSAGE_WINDOW_FRAME);
}

static int32_t run_draw(game_state *st, const int32_t *a) { (void) a; message_window_draw(st); return 0; }
static int32_t run_video(game_state *st, const int32_t *a) { (void) a; message_window_draw_video(st); return 0; }
static int32_t run_replies(game_state *st, const int32_t *a) { (void) a; message_reply_buttons(st); return 0; }
static int32_t run_thumb(game_state *st, const int32_t *a) { (void) a; message_scroll_thumb(st); return 0; }
static int32_t run_pattern(game_state *st, const int32_t *a)
{
    panel_draw_pattern(st, a[0], a[1], a[2], a[3]);
    return 0;
}

const ported_function msgwindow_ports[] = {
    {0x004C5E60u, "message_window_draw", 0, 0, 0, run_draw, 0},
    {0x004C6C70u, "message_window_draw_video", 0, 0, 0, run_video, 0},
    {0x004C5CE0u, "message_reply_buttons", 0, 0, 0, run_replies, 0},
    {0x004C72B0u, "message_scroll_thumb", 0, 0, 0, run_thumb, 0},
    {0x00536990u, "panel_draw_pattern", 4, 0, 0, run_pattern, 0},
};
const int msgwindow_ports_count = sizeof(msgwindow_ports) / sizeof(msgwindow_ports[0]);
