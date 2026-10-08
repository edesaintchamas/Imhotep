/* Imhotep - the advisors: pages 12 (0x0052A250) and 13 (0x0052C570), group f.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Both pages draw text groups 0x3C / 0x3D of Pharaoh_Text.eng with the values of the current city
 * record ([0x00EAAC28] * 0x49D8 from the addresses below). The meanings of the record's fields are
 * not established here: the pages only show them. */
#include "f.h"

#include "advisors.h"
#include "draw/draw.h"
#include "draw/text.h"
#include "sidebar/sidebar.h"
#include "sim/call.h"

#define CITY_INDEX 0x00EAAC28u   /* i32 the current city record */
#define CITY_STRIDE 0x49D8u
#define TEXT_CURSOR 0x00E20778u  /* i32 the text cursor (width of the last text drawn) */
#define PANEL_SLIDE 0x00F53F48u  /* i32 the window panel's slide-in step */
#define FONT_GREEN 0x00F56F94u   /* fonts (first image ids) used by the pages */
#define FONT_WHITE 0x00F56F9Cu
#define FONT_RED 0x00F56FA4u

/* the current city record's field at `base` */
static uint32_t city(game_state *st, uint32_t base)
{
    return base + (uint32_t) S32(st, CITY_INDEX) * CITY_STRIDE;
}

static int32_t px(game_state *st, int32_t dx)
{
    return S32(st, ADV_PANEL_X) + dx;
}

static int32_t py(game_state *st, int32_t dy)
{
    return S32(st, ADV_PANEL_Y) + dy;
}

void advisors_draw_strip(game_state *st, int32_t x, int32_t y, int32_t n)
{
    /* @bin 0x00536BA0 the strip: [0x00D3B5DC] := y once; per piece [0x00D3B5E0] := x + 16 i,
     * [0x00D3B5C0] := the image; the draw re-reads y from [0x00D3B5DC] each time */
    int32_t mid = 0;
    S32(st, 0x00D3B5DCu) = y;
    for (int32_t i = 0; i < n; i++) {
        int32_t xi = i * 16 + x;
        int32_t id;
        S32(st, 0x00D3B5E0u) = xi;
        if (i == 0) {
            id = S32(st, 0x00F56844u) + 0x2A;
        } else if (i >= n - 1) {
            id = S32(st, 0x00F56844u) + 0x30;
        } else {
            id = S32(st, 0x00F56844u) + mid + 0x2B;
            mid++;
        }
        S32(st, 0x00D3B5C0u) = id;
        image_draw(st, id, xi, S32(st, 0x00D3B5DCu));
        if (mid >= 5) { /* @bin 0x00536C1A the middle pieces cycle through 5 images */
            mid = 0;
        }
    }
}

/* ---- page 12 ---- */

static void group60(game_state *st, int32_t index, int32_t x, int32_t y, uint32_t font)
{
    layout_group(st, 0x3C, index, x, y, S32(st, font), 0, S32(st, ADV_DX), S32(st, ADV_DY));
}

static void number60(game_state *st, int32_t value, uint32_t suffix, int32_t x, int32_t y, uint32_t font)
{
    layout_number_at(st, value, 0x40, suffix, x, y, S32(st, font), 0, S32(st, ADV_DX), S32(st, ADV_DY));
}

/* a row of two columns: label at x + 0x50, the record's fields at x + 0x122 and x + 0x1AE */
static void row(game_state *st, int32_t label, int32_t y, uint32_t first, uint32_t second)
{
    group60(st, label, px(st, 0x50), py(st, y), ADV_FONT_NORMAL);
    number60(st, S32(st, city(st, first)), 0x0057C048u, px(st, 0x122), py(st, y), ADV_FONT_NORMAL);
    number60(st, S32(st, city(st, second)), 0x0057C048u, px(st, 0x1AE), py(st, y), ADV_FONT_NORMAL);
}

static void rules(game_state *st, int32_t y)
{
    line_draw(st, px(st, 0x118), py(st, y), px(st, 0x15E), py(st, y), 0);
    line_draw(st, px(st, 0x1A4), py(st, y), px(st, 0x1EA), py(st, y), 0);
}

/* the window panel's slide-in (both pages): 1 when the page is drawn in full */
static int panel_open(game_state *st, int32_t h)
{
    int32_t dx = S32(st, ADV_DX);
    int32_t dy = S32(st, ADV_DY);
    int32_t k = 5 - S32(st, PANEL_SLIDE);
    S32(st, ADV_PANEL_X) = dx;
    S32(st, ADV_PANEL_Y) = dy;
    S32(st, ADV_PANEL_W) = 0x28;
    S32(st, ADV_PANEL_H) = h;
    if (k >= 5) {
        return 0;
    }
    if (k > 0) {
        /* @bin 0x0052A2D4 sliding in: an outline k * 16 px inside the panel */
        rect_draw(st, k * 16 + dx, dy + k * 8, (0x28 - 2 * k) * 16, (h - 2 * k) * 16, 0);
        return 0;
    }
    panel_draw(st, dx, dy, 0x28, h);
    return 1;
}

void advisors_draw_page12(game_state *st)
{
    /* @bin 0x0052A250 advisor page 12: text group 0x3C, the city record's values */
    int32_t v = S32(st, 0x00784DDCu);
    if (U16(st, city(st, 0x00EA5CE8u)) != 0
        && GAME_CALL(st, 0x004C5120u, S32(st, city(st, 0x00EA5F9Cu))) == 1) {
        v -= 5; /* @bin 0x0052A283 less 5, not below 0 */
        if (v < 0) {
            v = 0;
        }
    }
    if (!panel_open(st, 0x1B)) {
        return;
    }
    image_draw(st, S32(st, 0x00F5681Cu) + 10, px(st, 10), py(st, 10));
    group60(st, 0, px(st, 0x3C), py(st, 0x0C), ADV_FONT_LARGE);
    layout_inner_panel(st, px(st, 0x40), py(st, 0x30), 0x22, 4, S32(st, ADV_DX), S32(st, ADV_DY));
    advisors_draw_strip(st, px(st, 0x40), py(st, 0x68), 0x22);

    S32(st, TEXT_CURSOR) = 0;
    /* @bin 0x0052A3D2 signed: a negative value shown as its opposite in red */
    if (S32(st, city(st, 0x00EA187Cu)) >= 0) {
        group60(st, 2, px(st, 0x46), py(st, 0x35), FONT_WHITE);
        text_count_draw(st, 8, 0, S32(st, city(st, 0x00EA187Cu)), S32(st, TEXT_CURSOR) + px(st, 0x48), py(st, 0x35),
                        S32(st, FONT_WHITE), 0);
    } else {
        group60(st, 3, px(st, 0x46), py(st, 0x35), FONT_RED);
        text_count_draw(st, 8, 0, -S32(st, city(st, 0x00EA187Cu)), S32(st, TEXT_CURSOR) + px(st, 0x48),
                        py(st, 0x35), S32(st, FONT_RED), 0);
    }
    group60(st, 1, px(st, 0x46), py(st, 0x4C), FONT_WHITE);
    S32(st, TEXT_CURSOR) = 0;
    number60(st, S32(st, city(st, 0x00EA1878u)), 0x005E03FCu, px(st, 0xF0), py(st, 0x4C), FONT_WHITE);
    group60(st, 4, S32(st, TEXT_CURSOR) + px(st, 0xF0), py(st, 0x4C), FONT_WHITE);
    text_count_draw(st, 8, 0, S32(st, city(st, 0x00EA5E5Cu)), S32(st, TEXT_CURSOR) + px(st, 0xF0), py(st, 0x4C),
                    S32(st, FONT_WHITE), 0);
    S32(st, TEXT_CURSOR) = 0;
    number60(st, S32(st, city(st, 0x00EA44A8u)), 0x005E03FCu, px(st, 0x46), py(st, 0x62), FONT_WHITE);
    group60(st, 5, S32(st, TEXT_CURSOR) + px(st, 0x46), py(st, 0x62), FONT_WHITE);
    S32(st, TEXT_CURSOR) = 0;
    /* @bin 0x0052A638 the sum of two fields */
    number60(st, S32(st, city(st, 0x00EA44B8u)) + S32(st, city(st, 0x00EA44B4u)), 0x005F89D8u, px(st, 0x16D),
             py(st, 0x62), FONT_WHITE);
    group60(st, 0x17, S32(st, TEXT_CURSOR) + px(st, 0x16D), py(st, 0x62), FONT_WHITE);
    group60(st, 6, px(st, 0x10E), py(st, 0x7D), ADV_FONT_NORMAL);
    group60(st, 7, px(st, 0x190), py(st, 0x7D), ADV_FONT_NORMAL);

    row(st, 8, 0x8C, 0x00EA44C0u, 0x00EA44BCu);
    row(st, 9, 0x9B, 0x00EA44D8u, 0x00EA44D4u);
    row(st, 0x14, 0xAA, 0x00EA5CC4u, 0x00EA5CC8u);
    row(st, 0x18, 0xB9, 0x00EA601Cu, 0x00EA6020u);
    rules(st, 0xC6);
    row(st, 10, 0xCB, 0x00EA4514u, 0x00EA4518u);
    row(st, 0x0B, 0xE3, 0x00EA44E0u, 0x00EA44DCu);
    row(st, 0x0C, 0xF2, 0x00EA448Cu, 0x00EA4488u);
    row(st, 0x0D, 0x101, 0x00EA44F8u, 0x00EA44FCu);
    S32(st, TEXT_CURSOR) = 0;
    group60(st, 0x0E, px(st, 0x50), py(st, 0x110), ADV_FONT_NORMAL);
    number60(st, v, 0x005E03FCu, S32(st, TEXT_CURSOR) + px(st, 0x50), py(st, 0x110), ADV_FONT_NORMAL);
    number60(st, S32(st, city(st, 0x00EA44E8u)), 0x0057C048u, px(st, 0x122), py(st, 0x110), ADV_FONT_NORMAL);
    number60(st, S32(st, city(st, 0x00EA44ECu)), 0x0057C048u, px(st, 0x1AE), py(st, 0x110), ADV_FONT_NORMAL);
    row(st, 0x0F, 0x11F, 0x00EA4500u, 0x00EA4504u);
    row(st, 0x10, 0x12E, 0x00EA44F0u, 0x00EA44F4u);
    row(st, 0x15, 0x13D, 0x00EA5D54u, 0x00EA5D50u);
    row(st, 0x16, 0x14C, 0x00EA5FC4u, 0x00EA5FC8u);
    rules(st, 0x159);
    row(st, 0x11, 0x15E, 0x00EA451Cu, 0x00EA4520u);
    row(st, 0x12, 0x175, 0x00EA4524u, 0x00EA4528u);
    row(st, 0x13, 0x18C, 0x00EA452Cu, 0x00EA4530u);
}

/* ---- page 13 ---- */

/* 0x004D6200 (the parent's helper): string `index` of `group` in the layout rectangle `rect` */
static void line61(game_state *st, int32_t group, int32_t index, uint32_t rect, int32_t font, int32_t x)
{
    GAME_CALL(st, 0x004D6200u, group, index, (int32_t) rect, font, 0, x, 0, S32(st, ADV_DX), S32(st, ADV_DY));
}

static void title61(game_state *st, int32_t index, uint32_t rect)
{
    line61(st, 0x3D, index, rect, S32(st, FONT_WHITE), 0);
}

static void number61(game_state *st, int32_t value, uint32_t suffix, uint32_t rect, int32_t font)
{
    layout_number(st, value, 0x40, suffix, rect, font, 0, S32(st, TEXT_CURSOR) + 0x8C, 0);
}

void advisors_draw_page13(game_state *st)
{
    /* @bin 0x0052C570 advisor page 13: text group 0x3D, eleven lines of a title and a verdict
     * chosen from the city record (the layout rectangles 0x005C5878 + 0x1C k) */
    if (!panel_open(st, 0x18)) {
        return;
    }
    image_draw(st, S32(st, 0x00F5681Cu) + 0x0B, px(st, 10), py(st, 10));
    layout_group(st, 0x3D, 0, px(st, 0x3C), py(st, 0x0C), S32(st, ADV_FONT_LARGE), 0, S32(st, ADV_DX),
                 S32(st, ADV_DY));
    layout_group(st, 0x3D, 0xDA, px(st, 0x3C), py(st, 0x15E), S32(st, ADV_FONT_NORMAL), 0, S32(st, ADV_DX),
                 S32(st, ADV_DY));
    layout_inner_panel(st, px(st, 0x20), py(st, 0x3C), 0x24, 0x11, S32(st, ADV_DX), S32(st, ADV_DY));
    for (int32_t i = 0; i < 0xF0; i += 0x14) {
        image_draw(st, S32(st, 0x00F5690Cu), px(st, 0x28), i + py(st, 0x43));
    }

    /* 1: rect 0x005C5878 */
    title61(st, 1, 0x005C5878u);
    S32(st, TEXT_CURSOR) = 0;
    {
        int32_t v = S32(st, city(st, 0x00EA1880u));
        if (v <= 0) {
            line61(st, 0x3D, 0x14, 0x005C5878u, S32(st, FONT_RED), 0x8C);
            int32_t w = S32(st, city(st, 0x00EA5DB0u));
            if (w != 0) {
                line61(st, 0x3D, w + 0x20, 0x005C5878u, S32(st, FONT_RED), S32(st, TEXT_CURSOR) + 0x8C);
            }
        } else if (v >= 0x64) {
            line61(st, 0x3D, 0x1F, 0x005C5878u, S32(st, FONT_GREEN), 0x8C);
        } else {
            int32_t font;
            if (v < 0x1E) {
                font = S32(st, FONT_RED);
            } else {
                font = S32(st, FONT_WHITE);
                if (v >= 0x32) {
                    font = S32(st, FONT_GREEN);
                }
            }
            line61(st, 0x3D, v / 10 + 0x15, 0x005C5878u, font, 0x8C);
            if (S32(st, city(st, 0x00EA1880u)) < 0x32) {
                int32_t w = S32(st, city(st, 0x00EA5DB0u));
                if (w != 0) {
                    line61(st, 0x3D, w + 0x1F, 0x005C5878u, font, S32(st, TEXT_CURSOR) + 0x8C);
                }
            }
        }
    }

    /* 2: rect 0x005C5894 */
    title61(st, 2, 0x005C5894u);
    {
        int32_t font = S32(st, FONT_GREEN);
        int32_t index;
        uint32_t c = (uint32_t) S32(st, CITY_INDEX) * CITY_STRIDE;
        S32(st, TEXT_CURSOR) = 0;
        int32_t mode = S32(st, c + 0x00EA3FB0u);
        if (mode == 0) {
            if (S32(st, c + 0x00EA5EB4u) + S32(st, c + 0x00EA5EA4u) > 3) {
                font = S32(st, FONT_RED);
                index = 0x2B;
            } else if (S32(st, c + 0x00EA3FACu) >= 5) {
                index = 0x2C;
            } else if (S32(st, c + 0x00EA3F98u) == 0 && S32(st, c + 0x00EA18A8u) > 0) {
                if (S32(st, c + 0x00EA3F9Cu) >= 0x50) {
                    index = 0x2C;
                } else {
                    index = S32(st, c + 0x00EA5C80u) + 0x2E;
                    font = S32(st, FONT_RED);
                }
            } else {
                font = S32(st, FONT_RED);
                index = 0x2D;
            }
        } else if (mode == 1) {
            font = S32(st, FONT_RED);
            if (S32(st, c + 0x00EA3F98u) == 0 && S32(st, c + 0x00EA18A8u) > 0) {
                index = S32(st, c + 0x00EA5C80u) + 0x35;
            } else {
                index = 0x34;
            }
        } else {
            index = 0x3B;
        }
        line61(st, 0x3D, index, 0x005C5894u, font, 0x8C);
    }

    /* 3: rect 0x005C58B0 */
    title61(st, 3, 0x005C58B0u);
    {
        int32_t green = S32(st, FONT_GREEN);
        int32_t font = green;
        int32_t index = 0; /* unset in the original when both values are <= 0: then unused */
        S32(st, TEXT_CURSOR) = 0;
        int32_t a = S32(st, city(st, 0x00EA446Cu));
        if (a > 0x11) {
            font = S32(st, FONT_RED);
            index = 0x4C;
        } else if (a > 0x0A) {
            font = S32(st, FONT_WHITE);
            index = 0x4D;
        } else if (a > 5) {
            index = 0x4E;
        } else if (a > 0) {
            index = 0x4F;
        } else {
            int32_t b = S32(st, city(st, 0x00EA4474u));
            if (b > 0x14) {
                font = S32(st, FONT_RED);
                index = 0x50;
            } else if (b > 0x0A) {
                font = S32(st, FONT_WHITE);
                index = 0x51;
            } else if (b > 5) {
                index = 0x52;
            } else if (b > 0) {
                index = 0x53;
            }
        }
        if (a <= 5 && S32(st, city(st, 0x00EA4474u)) <= 5) {
            line61(st, 0x3D, 0x54, 0x005C58B0u, green, 0x8C);
        } else {
            line61(st, 0x3D, index, 0x005C58B0u, font, 0x8C);
            if (a > 0) {
                number61(st, S32(st, city(st, 0x00EA446Cu)), 0x005E03FCu, 0x005C58B0u, font);
            } else {
                int32_t b = S32(st, city(st, 0x00EA4474u));
                if (b > 0) {
                    number61(st, b, 0x0057C048u, 0x005C58B0u, font);
                    line61(st, 8, 0x0D, 0x005C58B0u, font, S32(st, TEXT_CURSOR) + 0x8C);
                }
            }
        }
    }

    /* 4: rect 0x005C58CC */
    title61(st, 4, 0x005C58CCu);
    S32(st, TEXT_CURSOR) = 0;
    if (S32(st, 0x00784BBCu) != 0) {
        line61(st, 0x3D, 0x5E, 0x005C58CCu, S32(st, FONT_GREEN), 0x8C);
    } else {
        int32_t n = S32(st, city(st, 0x00EA4388u));
        if (n <= 0) {
            line61(st, 0x3D, 0x5F, 0x005C58CCu, S32(st, FONT_RED), 0x8C);
        } else if (S32(st, city(st, 0x00EA41DCu)) * 100 / S32(st, city(st, 0x00EA5E1Cu)) < 0x32) {
            /* @bin 0x0052CBFD signed idiv */
            line61(st, 0x3D, 0x60, 0x005C58CCu, S32(st, FONT_WHITE), 0x8C);
        } else if (n < 1) {
            line61(st, 0x3D, 0x61, 0x005C58CCu, S32(st, FONT_GREEN), 0x8C);
        } else {
            line61(st, 0x3D, 0x62, 0x005C58CCu, S32(st, FONT_GREEN), 0x8C);
            number61(st, S32(st, city(st, 0x00EA4388u)), 0x0057C048u, 0x005C58CCu, S32(st, FONT_GREEN));
            line61(st, 8, 5, 0x005C58CCu, S32(st, FONT_GREEN), S32(st, TEXT_CURSOR) + 0x8C);
        }
    }

    /* 5: rect 0x005C58E8 */
    {
        int32_t font = S32(st, FONT_GREEN);
        int32_t index;
        title61(st, 5, 0x005C58E8u);
        S32(st, TEXT_CURSOR) = 0;
        index = -1;
        if (S32(st, city(st, 0x00EA1894u)) != 0) {
            int32_t r = GAME_CALL0(st, 0x00447530u);
            int32_t pct = r * 100 / S32(st, city(st, 0x00EA1894u)); /* @bin 0x0052CD37 signed idiv */
            if (pct > 0x96) {
                index = 0x0D;
            } else if (pct > 0x6E) {
                index = 0x0E;
            } else if (pct > 0x5F) {
                index = 0x0F;
            } else if (pct > 0x55) {
                index = 0x10;
            } else if (pct > 0x46) {
                font = S32(st, FONT_WHITE);
                index = 0x11;
            }
        }
        if (index < 0) {
            font = S32(st, FONT_RED);
            index = 0x12;
        }
        line61(st, 0x3D, index, 0x005C58E8u, font, S32(st, TEXT_CURSOR) + 0x8C);
    }

    /* 6: rect 0x005C5904 */
    title61(st, 6, 0x005C5904u);
    S32(st, TEXT_CURSOR) = 0;
    {
        int32_t v = S32(st, city(st, 0x00EA1888u));
        int32_t font;
        if (v < 0x14) {
            font = S32(st, FONT_RED);
        } else {
            font = S32(st, FONT_WHITE);
            if (v >= 0x1E) {
                font = S32(st, FONT_GREEN);
            }
        }
        line61(st, 0x3D, v / 10 + 0x67, 0x005C5904u, font, 0x8C);
    }

    /* 7: rect 0x005C5920, the five signed bytes at 0x00EA5C00 */
    title61(st, 7, 0x005C5920u);
    {
        int32_t font = S32(st, FONT_GREEN);
        uint32_t c = city(st, 0x00EA5C00u);
        int32_t low = 0;
        int32_t least = 0x64;
        int32_t high = 0;
        int32_t index = 0; /* every case sets it */
        S32(st, TEXT_CURSOR) = 0;
        for (uint32_t k = 0; k < 5; k++) {
            int32_t b = (int8_t) U8(st, c + k);
            if (b < 0x1E) {
                low++;
            }
            if (b < least) {
                least = b;
            }
        }
        for (uint32_t k = 0; k < 5; k++) {
            if ((int8_t) U8(st, c + k) > 0x46) {
                high++;
            }
        }
        if (low == 0) {
            index = high == 0 ? 0x85 : high == 1 ? 0x86 : high == 2 ? 0x87 : 0x88 + (high != 3);
        } else if (low > 3) {
            index = least < 0x14 ? 0x7E : 0x7D;
        } else if (low == 3) {
            index = least < 0x14 ? 0x80 : 0x7F;
        } else if (low == 2) {
            index = least < 0x14 ? 0x82 : 0x81;
        } else if (low == 1) {
            index = least < 0x14 ? 0x84 : 0x83;
        }
        if (low != 0 && least < 0x14) {
            font = S32(st, FONT_WHITE);
        }
        line61(st, 0x3D, index, 0x005C5920u, font, 0x8C);
    }

    /* 8: rect 0x005C593C */
    title61(st, 8, 0x005C593Cu);
    {
        int32_t font = S32(st, FONT_GREEN);
        int32_t value = -1;
        int32_t index;
        uint32_t c = (uint32_t) S32(st, CITY_INDEX) * CITY_STRIDE;
        int32_t sum = S32(st, c + 0x00EA44B8u) + S32(st, c + 0x00EA44B4u);
        S32(st, TEXT_CURSOR) = 0;
        if (sum != 0 && (S32(st, c + 0x00EA44B0u) + S32(st, c + 0x00EA44ACu)) / sum < 1) {
            font = S32(st, FONT_WHITE);
            index = 0x97;
        } else {
            int32_t a = S32(st, c + 0x00EA187Cu);
            int32_t b = S32(st, c + 0x00EA452Cu);
            if (a > b) {
                value = a - b;
                index = 0x98;
            } else if (a < b) {
                font = S32(st, FONT_WHITE);
                value = b - a;
                index = 0x9A;
            } else {
                index = 0x99;
            }
        }
        line61(st, 0x3D, index, 0x005C593Cu, font, 0x8C);
        if (value != -1) {
            number61(st, value, 0x005F89CCu, 0x005C593Cu, font);
        }
    }

    /* 9: rect 0x005C5958 */
    title61(st, 9, 0x005C5958u);
    {
        int32_t font = S32(st, FONT_GREEN);
        int32_t value = -1;
        int32_t index;
        uint32_t c = (uint32_t) S32(st, CITY_INDEX) * CITY_STRIDE;
        int32_t n = S32(st, c + 0x00EA5FD0u);
        S32(st, TEXT_CURSOR) = 0;
        if (n >= 10) {
            value = S32(st, c + 0x00EA5FD4u);
            font = S32(st, FONT_RED);
            index = 0x9F;
        } else if (n == 0) {
            index = 0xA3;
        } else if (n < 2) {
            value = S32(st, c + 0x00EA5FD4u);
            index = 0xA2;
        } else if (n < 5) {
            value = S32(st, c + 0x00EA5FD4u);
            index = 0xA1;
        } else {
            value = S32(st, c + 0x00EA5FD4u);
            font = S32(st, FONT_WHITE);
            index = 0xA0;
        }
        line61(st, 0x3D, index, 0x005C5958u, font, 0x8C);
        if (value != -1) {
            number61(st, value, 0x0057C048u, 0x005C5958u, font);
            line61(st, 0x3D, 0xA4, 0x005C5958u, font, S32(st, TEXT_CURSOR) + 0x8C);
        }
    }

    /* the active events (0x0044BAF0 on the list 0x00785D70): of type 0x25, running, kind +0x30 */
    {
        int running = 0, kind5 = 0, kind67 = 0;
        for (uint32_t e = (uint32_t) GAME_CALLT(st, 0x00785D70u, 0x0044BAF0u, 0); e != 0;
             e = (uint32_t) GAME_CALLT(st, 0x00785D70u, 0x0044BAF0u, (int32_t) e)) {
            if (U16(st, e + 4) == 0x25) {
                if ((uint8_t) GAME_CALLT0(st, e, 0x0044D4C0u) != 0) {
                    running = 1;
                }
                uint16_t k = U16(st, e + 0x30);
                if (k == 5) {
                    kind5 = 1;
                }
                if (k == 6 || k == 7) {
                    kind67 = 1;
                }
            }
        }

        /* 10: rect 0x005C5974 */
        title61(st, 10, 0x005C5974u);
        int32_t font = S32(st, FONT_GREEN);
        int32_t index;
        S32(st, TEXT_CURSOR) = 0;
        if (S32(st, city(st, 0x00EA5EB4u)) != 0) {
            font = S32(st, FONT_RED);
            index = 0xAE;
        } else if (S32(st, city(st, 0x00EA5EA4u)) != 0) {
            font = S32(st, FONT_RED);
            index = 0xAD;
        } else if (GAME_CALL0(st, 0x00446610u) != 0) {
            font = S32(st, FONT_RED);
            index = 0xAC;
        } else if (kind67 || kind5) {
            index = 0xB0;
        } else if (running) {
            index = 0xAF;
        } else {
            index = 0xAA + (S32(st, city(st, 0x00EA5EE8u)) >= 0);
        }
        line61(st, 0x3D, index, 0x005C5974u, font, S32(st, TEXT_CURSOR) + 0x8C);
    }

    /* 11: rect 0x005C5990, the running events with a type (at most 5 counted) */
    {
        int32_t count = 0;
        for (uint32_t e = (uint32_t) GAME_CALLT(st, 0x00785D70u, 0x0044BAF0u, 0); e != 0;
             e = (uint32_t) GAME_CALLT(st, 0x00785D70u, 0x0044BAF0u, (int32_t) e)) {
            if ((uint8_t) GAME_CALLT0(st, e, 0x0044D4C0u) != 0) {
                if (count >= 5) {
                    break;
                }
                if (U16(st, e + 4) != 0) {
                    count++;
                }
            }
        }
        int32_t font = S32(st, FONT_GREEN);
        title61(st, 0x0B, 0x005C5990u);
        S32(st, TEXT_CURSOR) = 0;
        int32_t index = count == 0 ? 0xBB : count <= 2 ? 0xBA : 0xB8 + (count <= 4);
        line61(st, 0x3D, index, 0x005C5990u, font, 0x8C);
    }

    /* 12: rect 0x005C59AC, the flood (0x004BD590 on 0x00D38360) */
    title61(st, 0x0C, 0x005C59ACu);
    S32(st, TEXT_CURSOR) = 0;
    {
        int32_t f = (GAME_CALLT0(st, 0x00D38360u, 0x004BD590u) + 0x13) / 0x14; /* @bin 0x0052D40A signed /20 */
        int32_t font;
        if (f == 0) {
            font = S32(st, FONT_RED);
        } else {
            font = S32(st, FONT_WHITE);
            if (f != 1) {
                font = S32(st, FONT_GREEN);
            }
        }
        line61(st, 0x3D, f + 0xC0, 0x005C59ACu, font, 0x8C);
        if (f != 0) {
            /* @bin 0x0052D483 ([0x00D38360] % 30) / 15 + 0xC2 + 2 [0x00D386D0], signed */
            int32_t index = S32(st, 0x00D38360u) % 0x1E / 0x0F + 0xC2 + S32(st, 0x00D386D0u) * 2;
            line61(st, 0x3D, index, 0x005C59C8u, S32(st, FONT_GREEN), 0x8C);
        }
    }
}
