/* Imhotep - rich text: the message and help bodies (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * 0x004C8070 lays a text out in lines of a given width, line by line into the buffer 0x00E3439C,
 * with the codes "@P" (new paragraph, indented 0x32), "@L" (new line), "@G<n>" (picture n of the
 * message pictures, centred, the following lines left blank for its height) and "@<n>" (a link to
 * entry n: the next word is drawn in the link font and its rectangle recorded, 0x004C7FD0). It
 * either measures (lines counted in 0x00D3B138) or draws the lines from the scroll position
 * 0x00D3B130 on, at most 0x00D3B13C of them. The texts are often on the caller's stack: the port
 * reads them from a host copy. */
#include "messages.h"

#include "draw/mem.h"
#include "draw/text.h"
#include "model/model_text.h"
#include "sim/call.h"
#include "state/ported.h"

#define LINE_BUF 0x00E3439Cu      /* char[200] the line being laid out */
#define WORD_CHARS 0x00E20760u    /* i32 bytes the last measured word takes */
#define TEXT_X 0x00E20778u        /* i32 text cursor x */
#define TEXT_Y 0x00E20774u        /* i32 the height laid out (+16 per line) */
#define TEXT_MAX_X 0x00E2076Cu    /* i32 widest line */
#define LINES 0x00D3B138u         /* i32 lines counted when measuring */
#define SCROLL 0x00D3B130u        /* i32 first line drawn */
#define VISIBLE 0x00D3B13Cu       /* i32 lines drawn */
#define BOX_W 0x00D3B14Cu         /* i32 the text box's width in pieces */
#define FONT_LINK 0x00D3B164u     /* i32 link font */
#define FONT_TEXT 0x00D3B168u     /* i32 text font */
#define FONT_SMALL 0x00F56F84u
#define GLYPH_COLOUR 0x00D3B51Cu  /* u16 */
#define PIXEL_FORMAT 0x00E92DFCu
#define PICTURES 0x00F55C04u      /* i32 first image of the message pictures */
#define IMGS_W 0x010D6A0Cu         /* i16 image entries of 0x40 bytes: +0 width, +2 height */
#define IMGS_H 0x010D6A0Eu
#define IMG_COUNT 0x0126517Cu
#define GLYPHS 0x005E049Cu        /* u8 glyph number per character - 0x20 */
#define LINKS_COUNT 0x00D3A6ECu
#define LINKS_NEXT 0x00D3A6F0u
#define LINKS 0x00D3A6F8u         /* 50 entries of 5 i32: x1, x2, y1, y2, target */
#define GLYPH_LAST 0x00D3B5C0u
#define GFX 0x00F55BB0u

static int32_t clamp_id(game_state *st, int32_t id)
{
    return id < S32(st, IMG_COUNT) ? id : 0;
}

static int32_t img_w(game_state *st, int32_t id)
{
    return S16(st, IMGS_W + ((uint32_t) clamp_id(st, id) << 6));
}

static int32_t img_h(game_state *st, int32_t id)
{
    return S16(st, IMGS_H + ((uint32_t) clamp_id(st, id) << 6));
}

/* @bin 0x004C8239 (h + (h >> 31 & 15)) >> 4: the height in 16-pixel lines, toward zero */
static int32_t lines_of(int32_t h)
{
    return (h + ((h >> 31) & 0xF)) >> 4;
}

int32_t rich_word_width(game_state *st, const uint8_t *p)
{
    /* @bin 0x004C8480 the width of the next word (spaces before it 4 each, glyphs by the text font):
     * "@P", "@L", "@G<n>" end it with 0; "@<n>" is skipped; bytes taken in 0x00E20760 */
    int32_t w = 0;
    int seen = 0;
    S32(st, WORD_CHARS) = 0;
    for (int32_t k = 1;; k++) {
        uint8_t c = *p++;
        if (c == 0) {
            return w;
        }
        if (c == 0x40) {
            c = *p++;
            if (!seen) {
                if (c == 0x50 || c == 0x4C) {
                    S32(st, WORD_CHARS) += 2;
                    return 0;
                }
                if (c == 0x47) {
                    S32(st, WORD_CHARS) += 2;
                    p++;
                    c = p[-1];
                    if (c >= 0x30) {
                        while (c <= 0x39) {
                            c = *p++;
                            S32(st, WORD_CHARS) += 1;
                            if (c < 0x30) {
                                break;
                            }
                        }
                    }
                    return 0;
                }
                S32(st, WORD_CHARS) += 1;
                if (c >= 0x30) {
                    while (c <= 0x39) {
                        c = *p++;
                        S32(st, WORD_CHARS) += 1;
                        if (c < 0x30) {
                            break;
                        }
                    }
                }
            }
        }
        if (c == 0x20) {
            if (seen) {
                return w;
            }
            w += 4;
        } else if (c > 0x20) {
            w += glyph_width(st, c, S32(st, FONT_TEXT));
            seen = 1;
        }
        S32(st, WORD_CHARS) += 1;
        if (k + 1 >= 2000) {
            return w;
        }
    }
}

static void link_add(game_state *st, int32_t target, int32_t x1, int32_t x2, int32_t y)
{
    /* @bin 0x004C7FD0 a link's rectangle (at most 50) */
    if (S32(st, LINKS_COUNT) < 0x32) {
        uint32_t e = LINKS + 20u * (uint32_t) S32(st, LINKS_NEXT);
        S32(st, e) = x1 - 2;
        S32(st, e + 4) = x2 + 2;
        S32(st, e + 8) = y - 1;
        S32(st, e + 12) = y + 0x0D;
        S32(st, e + 16) = target;
        S32(st, LINKS_COUNT) += 1;
        S32(st, LINKS_NEXT) += 1;
    }
}

static int32_t glyph_draw(game_state *st, int32_t font, uint8_t ch, int32_t x, int32_t y, int32_t measure,
                          int32_t raised)
{
    /* @bin 0x004C8680 glyph ch (- 0x20) of font at (x, y), lowered by its height - 0x0B (- 0x10 when
     * raised) for characters from 0x80 on but 0xE7 and 0x107: its width */
    uint32_t g = U8(st, GLYPHS + ch);
    S32(st, GLYPH_LAST) = (int32_t) g;
    if (g == 0) {
        return 0;
    }
    int32_t id = (int32_t) g + font - 1;
    int32_t h = raised != 0 ? img_h(st, id) - 0x10 : img_h(st, id) - 0x0B;
    if (h < 0) {
        h = 0;
    }
    if (ch < 0x60) {
        h = 0;
    }
    if (ch == 0xC7 || ch == 0xE7) {
        h = 0;
    }
    if (measure == 0) {
        GAME_CALLT(st, GFX, 0x0053E590u, id, x, y - h);
        g = (uint32_t) S32(st, GLYPH_LAST);
    }
    return img_w(st, (int32_t) g + font - 1);
}

static void line_draw(game_state *st, const uint8_t *p, int32_t x, int32_t y, int32_t measure, int32_t raised)
{
    /* @bin 0x004C8590 a laid-out line: "@<n>" makes the next word a link (link font, rectangle
     * recorded); glyphs without an image advance 6 */
    int32_t link_chars = 0;
    uint8_t c = *p++;
    while (c != 0) {
        if (c == 0x40) {
            int32_t target = model_parse_number(p);
            c = *p++;
            if (c >= 0x30) {
                while (c <= 0x39) {
                    c = *p++;
                    if (c < 0x30) {
                        break;
                    }
                }
            }
            int32_t w = rich_word_width(st, p - 1);
            link_chars = S32(st, WORD_CHARS) + 1;
            link_add(st, target, x, w + x, y);
        }
        if (c >= 0x20) {
            uint8_t ch = (uint8_t) (c - 0x20);
            int32_t font;
            if (link_chars > 0) {
                font = S32(st, FONT_LINK);
                link_chars--;
            } else {
                font = S32(st, FONT_TEXT);
            }
            int32_t adv;
            if (U8(st, GLYPHS + ch) != 0) {
                adv = glyph_draw(st, font, ch, x, y, measure, raised);
                if (font == S32(st, FONT_SMALL)) {
                    adv += 2;
                }
            } else {
                adv = 6;
            }
            x += adv;
        }
        c = *p++;
    }
}

static void line_draw_buffer(game_state *st, int32_t x, int32_t y, int32_t measure, int32_t raised)
{
    uint8_t buf[201];
    for (uint32_t k = 0; k < 200; k++) {
        buf[k] = U8(st, LINE_BUF + k);
    }
    buf[200] = 0;
    line_draw(st, buf, x, y, measure, raised);
}

void rich_text_draw(game_state *st, const uint8_t *text, int32_t x, int32_t y, int32_t width, int32_t measure,
                    int32_t raised)
{
    /* @bin 0x004C8070 */
    int32_t picture = 0;     /* the picture waiting for its line */
    int32_t delay = 0;       /* lines before it is drawn */
    int32_t new_line = 0;    /* "@L" / "@G" seen (read back nowhere) */
    int32_t paragraph = 0;   /* "@P": the next line indented */
    int32_t more = 1;        /* text left */
    int32_t iterations = 0;
    int32_t line = 0;
    int32_t blank = 0;       /* blank lines left for a picture */
    const uint8_t *p = text;
    (void) new_line;
    if (measure == 1) {
        S32(st, LINES) = 0;
    }
    for (;;) {
        if (more == 0 && blank == 0) {
            return;
        }
        if (++iterations >= 0x3E8) {
            return;
        }
        for (uint32_t k = 0; k < 200; k++) {
            U8(st, LINE_BUF + k) = 0;
        }
        int32_t used, indent;
        if (paragraph != 0) {
            used = 0x32;
            indent = 0x32;
        } else {
            used = 0;
            indent = 0;
        }
        int32_t len = 0;
        int32_t skip_spaces = 1;
        new_line = 0;
        paragraph = 0;
        /* @bin 0x004C8117 words while they fit */
        for (;;) {
            if (!((more != 0 || blank != 0) && used < width)) {
                break;
            }
            if (blank != 0) {
                blank--;
                break;
            }
            int32_t w = rich_word_width(st, p);
            used += w;
            if (used >= width) {
                if (used == 0) {
                    more = 0;
                }
                continue;
            }
            int32_t n = S32(st, WORD_CHARS);
            for (int32_t i = 0; i < n; i++) {
                uint8_t c = *p++;
                if (c == 0x40) {
                    uint8_t d = *p;
                    if (d == 0x50) {
                        used = width;
                        paragraph = 1;
                        p++;
                        break;
                    }
                    if (d == 0x4C) {
                        used = width;
                        new_line = 1;
                        p++;
                        break;
                    }
                    if (d == 0x47) {
                        if (len != 0 && measure == 1) {
                            S32(st, LINES) += 1;
                        }
                        used = width;
                        p++;
                        new_line = 1;
                        int32_t number = model_parse_number(p);
                        uint8_t e = *p++;
                        if (e >= 0x30) {
                            while (e <= 0x39) {
                                e = *p++;
                                if (e < 0x30) {
                                    break;
                                }
                            }
                        }
                        picture = S32(st, PICTURES) + number - 1;
                        blank = lines_of(img_h(st, picture)) + 2;
                        if (line > 0) {
                            delay = 1;
                        }
                        break;
                    }
                }
                if (skip_spaces == 0 || c != 0x20) {
                    skip_spaces = 0;
                    U8(st, LINE_BUF + (uint32_t) len) = c;
                    len++;
                }
            }
            if (*p == 0) {
                more = 0;
            }
        }
        /* @bin 0x004C8277 the line */
        int32_t hidden = 0;
        S32(st, 0x00E20780u) = 1;
        S32(st, TEXT_X) = 0;
        U8(st, 0x00E20799u) = 1;
        int draw = 1;
        if (measure == 0) {
            int32_t scroll = S32(st, SCROLL);
            if (line < scroll) {
                hidden = 1;
            }
            if (line >= S32(st, VISIBLE) + scroll) {
                hidden = 1;
                draw = 0;
            } else if (hidden != 0) {
                draw = 0;
            }
        }
        if (draw) {
            line_draw_buffer(st, indent + x, y, measure, raised);
            if (S32(st, FONT_TEXT) == S32(st, FONT_SMALL)) {
                uint16_t saved = U16(st, GLYPH_COLOUR);
                U16(st, GLYPH_COLOUR) = 0x7F9A;
                if (S32(st, PIXEL_FORMAT) == 2) {
                    U16(st, GLYPH_COLOUR) = 0xFF3A;
                }
                line_draw_buffer(st, x + indent - 1, y - 1, measure, raised);
                U16(st, GLYPH_COLOUR) = saved;
            }
        }
        if (measure == 0 && picture != 0) {
            if (delay != 0) {
                delay--;
            } else {
                blank = lines_of(img_h(st, picture)) + 2;
                int32_t px = (int32_t) ((S32(st, BOX_W) << 4) - img_w(st, picture) - 4) / 2 + x - 4;
                int32_t scroll = S32(st, SCROLL);
                if (line < S32(st, VISIBLE) + scroll) {
                    int32_t py = line < scroll ? ((line - scroll) << 4) + y + 8 : y + 8;
                    GAME_CALLT(st, GFX, 0x0053E590u, picture, px, py);
                }
                picture = 0;
            }
        }
        if (S32(st, TEXT_X) > S32(st, TEXT_MAX_X)) {
            S32(st, TEXT_MAX_X) = S32(st, TEXT_X);
        }
        line++;
        if (measure == 1) {
            S32(st, LINES) += 1;
        }
        if (hidden == 0) {
            y += 0x10;
            S32(st, TEXT_Y) += 0x10;
        }
    }
}

/* ---- port table: texts are state addresses on the original's side ---- */

static uint8_t *host_text(game_state *st, uint32_t a)
{
    static uint8_t buf[0x10000 + 16];
    uint32_t n = 0;
    while (n < 0x10000) {
        uint8_t c = mem_rd8(st, a + n);
        buf[n++] = c;
        if (c == 0) {
            break;
        }
    }
    for (uint32_t k = 0; k < 16; k++) {
        buf[n + k] = 0;
    }
    return buf;
}

static int32_t run_rich(game_state *st, const int32_t *a)
{
    rich_text_draw(st, host_text(st, (uint32_t) a[0]), a[1], a[2], a[3], a[4], a[5]);
    return 0;
}

static int32_t run_word(game_state *st, const int32_t *a)
{
    return rich_word_width(st, host_text(st, (uint32_t) a[0]));
}

static int32_t run_line(game_state *st, const int32_t *a)
{
    line_draw(st, host_text(st, (uint32_t) a[0]), a[1], a[2], a[3], a[4]);
    return 0;
}

static int32_t run_glyph(game_state *st, const int32_t *a)
{
    return glyph_draw(st, a[0], (uint8_t) a[1], a[2], a[3], a[4], a[5]);
}

static int32_t run_link(game_state *st, const int32_t *a)
{
    link_add(st, a[0], a[1], a[2], a[3]);
    return 0;
}

const ported_function richtext_ports[] = {
    {0x004C8070u, "rich_text_draw", 6, 0, 0, run_rich, 0},
    {0x004C8480u, "rich_word_width", 1, 32, 0, run_word, 0},
    {0x004C8590u, "rich_line_draw", 5, 0, 0, run_line, 0},
    {0x004C8680u, "rich_glyph_draw", 6, 32, 0, run_glyph, 0},
    {0x004C7FD0u, "rich_link_add", 4, 0, 0, run_link, 0},
};
const int richtext_ports_count = sizeof(richtext_ports) / sizeof(richtext_ports[0]);
