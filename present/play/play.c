/* Imhotep - functions a play session reaches every frame or on common screens (package play).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * - the on-screen messages 0x004E2720 (frame step 13, docs/presentation.md 1.1): the mission goal
 *   hint and the five timed messages of 0x00E94E2A (slots of 0x78 bytes: +0 i16 x centre, +2 i16 y,
 *   +4 i16 shown, +8 i16 width, +0xA u32 the clock when shown, +0x10 the text), each on a small
 *   button of kind 9 + slot, shown 15 s (0x3A98 ms) of the frame clock 0x00E92E0C;
 * - the network info text 0x00536620 (frame step 15), drawn while 0x00F53F68 is set;
 * - the button panels 0x00536DC0 (style by kind 0x00F53F10, as the large buttons 0x00536710);
 * - the layout registry fill 0x004D5D90 (the lists of present/draw/layout.c: the overrides read from
 *   "dialog.loc", the widths of the menu texts);
 * - the briefing's input 0x00418FE0 (windows 0x35, 0x3B: the difficulty arrows, the text scroll);
 * - the sidebar's build menu counts 0x00417A70 / 0x00417AA0 (the tutorial's new-building marks). */
#include <stdlib.h>
#include <string.h>

#include "frame/display.h"
#include "present.h"
#include "present_layout.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"


static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, id, x, y);
}

/* ---- button panels and the on-screen messages ---- */

int32_t play_button_pulse(game_state *st, int32_t kind)
{
    /* @bin 0x00536FD0 0x00F53F28 := 100; for the style 7 0x0047D350(1); returns 1 */
    S32(st, G_UF53F28) = 100;
    if (S32(st, G_BUTTON_STYLE + 4u * (uint32_t) kind) == 7) {
        GAME_CALL(st, FN_RETURN_ZERO, 1);
    }
    return 1;
}

void play_button_panel(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t w, int32_t h, int32_t lit)
{
    /* @bin 0x00536DC0 a panel of w x h pieces of 16 pixels at (x, y) in the style [0x00F53F10 + 4 kind]:
     * below 5 nothing; 5..14 one picture of the table 0x005F81E4 (3 dwords per style: image offset,
     * dx, dy) centred on the panel; from 15 a grid of at most style - 13 pieces each way (corners,
     * edges, middle: 6..14 past [0x00F56494] + 8 lit + lit), centred when smaller; kind 8 drawn
     * tinted (0x0053F4F0, 0xFFFFF863). 0x00D3B5E0 / 0x00D3B5DC: the last piece's place, 0x00D3B5C0
     * its image */
    int32_t style = S32(st, G_BUTTON_STYLE + 4u * (uint32_t) kind);
    if (style < 5) {
        return;
    }
    if (style < 15) {
        uint32_t e = G_BUTTON_PICTURES + 4u * (uint32_t) (style * 3 - 15);
        int32_t nx = (int32_t) ((uint32_t) (w - 2) << 4) / 2 + S32(st, e + 4) + x;
        S32(st, G_BLIT_X) = nx;
        int32_t ny = (int32_t) ((uint32_t) (h - 2) << 4) / 2 + S32(st, e + 8) + y;
        S32(st, G_BLIT_Y) = ny;
        image(st, S32(st, e) + lit * 8 + 0x18 + S32(st, G_SCROLL_IMAGES), nx, ny);
        return;
    }
    int32_t n = style - 0xD;
    int32_t cols = n, rows = n;
    if (n > w) {
        cols = w;
    } else {
        x += (int32_t) ((uint32_t) (w - n) << 4) / 2;
    }
    if (n > h) {
        rows = h;
    } else {
        y += (int32_t) ((uint32_t) (h - n) << 4) / 2;
    }
    for (int32_t r = 0; r < rows; r++) {
        for (int32_t c = 0; c < cols; c++) {
            int32_t px = c * 16 + x, py = r * 16 + y;
            S32(st, G_BLIT_X) = px;
            S32(st, G_BLIT_Y) = py;
            int32_t base = S32(st, G_SCROLL_IMAGES) + lit * 8 + lit;
            int32_t col = c == 0 ? 0 : c < cols - 1 ? 1 : 2;
            int32_t row = r == 0 ? 6 : r < rows - 1 ? 9 : 0xC;
            int32_t id = base + row + col;
            S32(st, G_GLYPH_LAST) = id;
            if (kind == 8) {
                GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW_TINTED, id, px, py, (int32_t) 0xFFFFF863);
            } else {
                image(st, id, px, py);
            }
        }
    }
}

void play_network_info(game_state *st)
{
    /* @bin 0x00536620 while 0x00F53F68 is set: a panel (kind 6, 12 x 15 pieces, lit) at
     * (0, 2 * [0x00E38DA4] + 0xDC) and "Network info (F2)" (0x005F8F28) centred on 0xC0 pixels, 6 lower,
     * main font, colour 0xFFFF */
    if (S32(st, G_NETWORK_INFO) == 0) {
        return;
    }
    int32_t y = S32(st, G_LAYOUT_DY) * 2 + 0xDC;
    play_button_panel(st, 6, 0, y, 0xC, 0xF, 1);
    GAME_CALL(st, FN_TEXT_DRAW_CENTRED, G_STR_NETWORK_INFO, 0, y + 6, 0xC0, S32(st, G_FONT_LARGE), 0xFFFF);
}

void play_screen_messages(game_state *st)
{
    /* @bin 0x004E2720 outside the city view (1) and 0x12 the view rows are reset first (0x004E2AB0);
     * the messages go down by 0x46 while paused (0x00E38E60) */
    uint8_t w = U8(st, G_WINDOW);
    if (w != 1 && w != 0x12) {
        GAME_CALL0(st, FN_VIEW_ROWS_RESET);
        w = U8(st, G_WINDOW);
    }
    int32_t yoff = U8(st, G_GATE) != 0 ? 0x46 : 0;
    if ((w == 1 || w == 0x12 || w == 6) && (uint8_t) GAME_CALL0(st, FN_MISSION_GOAL_HINT) != 0) {
        /* @bin 0x004E2773 the mission goal hint (text set by 0x004E2530): small button 9 of 0x1D pieces
         * left of the first slot's centre, two marks, the text centred on 0x1CC pixels; the timed
         * messages go 0x19 lower */
        int32_t y = S16(st, G_NOTICES + NOTICE_Y) + yoff;
        GAME_CALL(st, FN_FE_SMALL_BUTTON, 9, S16(st, G_NOTICES + NOTICE_X) - 0xE5, y, 0x1D, 1, 1);
        image(st, S32(st, G_SIDE_IMAGES) + 0x10, S16(st, G_NOTICES + NOTICE_X) - 0xE4, y + 2);
        image(st, S32(st, G_SIDE_IMAGES) + 0x10, S16(st, G_NOTICES + NOTICE_X) + 0xC8, y + 2);
        GAME_CALL(st, FN_TEXT_DRAW_CENTRED, S32(st, G_TEXT_CURSOR), S16(st, G_NOTICES + NOTICE_X) - 0xE5, y + 4, 0x1CC,
                  S32(st, G_FONT_9C), 0);
        yoff += 0x19;
    }
    S32(st, G_NOTICES_SHOWN) = 0; /* the number of messages shown */
    uint32_t counter = G_UF53F34;
    for (int32_t kind = 9; kind - 9 < 5; kind++, counter += 4) {
        uint32_t m = G_NOTICES + (uint32_t) (kind - 9) * NOTICE_SIZE;
        if (S16(st, m + NOTICE_ACTIVE) == 0) {
            continue;
        }
        S32(st, G_NOTICES_SHOWN) += 1;
        int32_t y = S16(st, m + NOTICE_Y) + yoff;
        int32_t width = S16(st, m + NOTICE_WIDTH);
        int32_t x = S16(st, m + NOTICE_X);
        GAME_CALL(st, FN_FE_SMALL_BUTTON, kind, x - width / 2 + 1, y, width / 16 + 1, 0, 1);
        if (S32(st, counter) > 0x28) {
            /* @bin 0x004E2890 after 0x28 frames of the button's pulse: two marks when narrower than
             * 0x1CC, then the text */
            int16_t w16 = S16(st, m + NOTICE_WIDTH);
            if (w16 < 0x1CC) {
                image(st, S32(st, G_SIDE_IMAGES) + 0xF, x - w16 / 2 + 2, y + 2);
                image(st, S32(st, G_SIDE_IMAGES) + 0xF, w16 / 2 + x - 0x1E, y + 2);
            }
            int32_t tw = S16(st, m + NOTICE_WIDTH);
            GAME_CALL(st, FN_TEXT_DRAW_CENTRED, (int32_t) (m + NOTICE_TEXT), x - tw / 2 + 1, y + 4, tw, S32(st, G_FONT_9C), 0);
        }
        if (play_button_pulse(st, kind) != 0) {
            U8(st, G_PRESENT_NOW) = 1;
            S32(st, counter) += 4;
        }
        if (S32(st, G_FRAME_CLOCK) > (int32_t) (U32(st, m + NOTICE_TIME) + 0x3A98u)) {
            S16(st, m + NOTICE_ACTIVE) = 0;
        }
    }
}

/* ---- the layout registry ---- */

static int32_t texts_widest(game_state *st, int32_t group)
{
    /* @bin 0x004D5F80 the widest text of the group (main font 0x00F56F8C), at least 1: texts 1, 2,
     * ... until the text cursor reaches the first text of the next group */
    int32_t widest = 1, i = 1;
    GAME_CALL(st, FN_TEXT_SEEK, group + 1, 0);
    uint32_t end = U32(st, G_TEXT_CURSOR);
    GAME_CALL(st, FN_TEXT_SEEK, group, 1);
    do {
        int32_t w = GAME_CALL(st, FN_TEXT_WIDTH, S32(st, G_TEXT_CURSOR), S32(st, G_FONT_LARGE));
        if (w > widest) {
            widest = w;
        }
        i++;
        GAME_CALL(st, FN_TEXT_SEEK, group, i);
    } while (end != U32(st, G_TEXT_CURSOR));
    return widest;
}

static void menu_widths(game_state *st)
{
    /* @bin 0x004D5DD0 the top menus' widths in 16-pixel pieces (widest text / 16 + 2) of the groups
     * 1, 2, 3, 4, 7, 10 into 0x00E945D0 .. 0x00E945E4 */
    static const int32_t groups[6] = {1, 2, 3, 4, 7, 0xA};
    for (int i = 0; i < 6; i++) {
        S32(st, G_MENU_WIDTHS + 4u * (uint32_t) i) = texts_widest(st, groups[i]) / 16 + 2;
    }
}

static void menu_bar_width(game_state *st)
{
    /* @bin 0x004D5E70 the group 0x1C's widest text in pieces, rounded up, into 0x00E94DB0 (pixels
     * 0x00E94DAC); the x of the 0x1B rectangles 0x005C28AC (stride 0x1C) := (pieces + 7) * 16 */
    int32_t w = texts_widest(st, 0x1C);
    int32_t pieces = w / 16;
    S32(st, G_MENU_BAR_PIECES) = pieces;
    if (w % 16 != 0) {
        pieces++;
        S32(st, G_MENU_BAR_PIECES) = pieces;
    }
    uint16_t x = (uint16_t) ((pieces + 7) << 4);
    for (uint32_t r = G_MENU_BAR_RECTS; r < G_MENU_BAR_RECTS + 0x348u; r += 0x1C) {
        U16(st, r) = x;
    }
    S32(st, G_MENU_BAR_WIDTH) = pieces << 4;
}

/* the CRT stream of "dialog.loc" read whole through the platform: fread of 4-byte items, with the
 * FILE flags _IOEOF 0x10 and _IOERR 0x20 (the original's FILE +0xC) */
typedef struct {
    uint8_t *data;
    uint32_t size, pos;
    int flags;
} stream;

static void stream_read4(stream *f, uint8_t out[4])
{
    /* @bin 0x00561CA1 fread(buf, 4, 1, f): the bytes there are, end of file flagged when short */
    uint32_t n = f->size - f->pos < 4 ? f->size - f->pos : 4;
    memcpy(out, f->data + f->pos, n);
    f->pos += n;
    if (n < 4) {
        f->flags |= 0x10;
    }
}

static int32_t le32(const uint8_t b[4])
{
    return (int32_t) ((uint32_t) b[0] | (uint32_t) b[1] << 8 | (uint32_t) b[2] << 16 | (uint32_t) b[3] << 24);
}

static void layout_list_read(game_state *st, stream *f, int32_t count, uint32_t head)
{
    /* @bin 0x004D6900 `count` nodes of 0x1C bytes (operator new), six i32 each, put in front of the
     * list [head]; a node read past the end is deleted. Nothing when the stream has an error */
    if (f->flags & 0x20) {
        return;
    }
    S32(st, head) = 0;
    if (count <= 0) {
        return;
    }
    do {
        uint32_t node = display_memory(st, 0, 0x1C); /* operator new 0x00561A1D (the other heap) */
        if (node == 0) {
            state_unported(st, FN_PLAY_LAYOUT_FILE_READ); /* no platform memory */
            return;
        }
        for (uint32_t k = 0; k < 0x18; k += 4) {
            uint8_t b[4];
            uint8_t *p = state_ptr(st, node + k, 4);
            if (p) {
                memcpy(b, p, 4);
            }
            stream_read4(f, b);
            if (p) {
                memcpy(p, b, 4);
            }
        }
        P32(st, node + 0x18) = U32(st, head);
        if ((f->flags & 0x30) == 0) {
            U32(st, head) = node;
        } else {
            display_memory(st, node, 0); /* operator delete 0x00561A2B */
        }
    } while (--count != 0);
}

static void layout_file_read(game_state *st)
{
    /* @bin 0x004D69B0 fopen("dialog.loc" 0x005E1C44, "rb" 0x005763BC): for each of the 0x78 windows
     * (16 bytes from 0x00E94618) three lists (+4, +0, +8), each a count then its nodes; a non-empty
     * list +4 clears +0xC. At the end of the file the window's lists are emptied (+0xC := 1) and the
     * reading stops. The GOG install has no such file (a developers' layout tool: assumed) */
    int32_t size = platform_file_read(st, "dialog.loc", 0, NULL, 0);
    if (size < 0) {
        return;
    }
    stream f = {NULL, 0, 0, 0};
    f.data = malloc(size > 0 ? (size_t) size : 1);
    if (!f.data) {
        return;
    }
    f.size = (uint32_t) (platform_file_read(st, "dialog.loc", 0, f.data, (uint32_t) size) < 0 ? 0 : size);
    /* the count's stack slot is not initialised before the first fread (a short first read keeps
     * garbage in its high bytes): assumed 0 */
    uint8_t count[4] = {0, 0, 0, 0};
    int32_t k = 0;
    for (uint32_t e = G_LAYOUT_REGISTRY + 4u; e <= G_LAYOUT_REGISTRY + 0x78u * 0x10u + 3u; e += 0x10, k++) {
        stream_read4(&f, count);
        layout_list_read(st, &f, le32(count), e);
        if (U32(st, e) != 0) {
            S32(st, e + 8) = 0;
        }
        stream_read4(&f, count);
        layout_list_read(st, &f, le32(count), e - 4);
        stream_read4(&f, count);
        layout_list_read(st, &f, le32(count), e + 4);
        if (f.flags & 0x30) {
            uint32_t b = G_LAYOUT_REGISTRY + (uint32_t) k * 16u;
            S32(st, b + 4) = 0;
            S32(st, b) = 0;
            S32(st, b + 8) = 0;
            S32(st, b + 0xC) = 1;
            break;
        }
    }
    free(f.data);
}

void play_layout_fill(game_state *st)
{
    /* @bin 0x004D5D90 the registry's 0x78 entries emptied (+0xC := 1), marked filled (0x00E94D9C),
     * then the menu widths, the menu bar's width and the overrides of "dialog.loc" */
    for (uint32_t a = G_LAYOUT_REGISTRY + 8u; a < G_LAYOUT_REGISTRY + 0x78u * 0x10u + 8u; a += 0x10) {
        S32(st, a - 4) = 0;
        S32(st, a) = 0;
        S32(st, a - 8) = 0;
        S32(st, a + 4) = 1;
    }
    S32(st, G_LAYOUT_READY) = 1;
    menu_widths(st);
    menu_bar_width(st);
    layout_file_read(st);
}

/* ---- the briefing's input, the build menu counts ---- */

int32_t play_briefing_input(game_state *st)
{
    /* @bin 0x00418FE0 the difficulty arrows (0x005C7DF8, 0x005C7E20: hovered 0xC / 0xD, then clicked)
     * at the briefing's origin (0x006FDCA0 + 0x230, 0x006FDC9C + 0xA8 / + 0x17E), else the text's
     * scroll bar (0x004C7DB0) */
    int32_t x = S32(st, G_SCREEN_X) + 0x230;
    GAME_CALL(st, FN_BUTTONS_HOVER, x, S32(st, G_SCREEN_Y) + 0xA8, G_ARROWS_UP, 1, 0xC);
    int32_t r = GAME_CALL(st, FN_BUTTONS_CLICK, S32(st, G_SCREEN_X) + 0x230, S32(st, G_SCREEN_Y) + 0xA8, G_ARROWS_UP, 1);
    if (r != 0) {
        return r;
    }
    GAME_CALL(st, FN_BUTTONS_HOVER, S32(st, G_SCREEN_X) + 0x230, S32(st, G_SCREEN_Y) + 0x17E, G_ARROWS_DOWN, 1, 0xD);
    r = GAME_CALL(st, FN_BUTTONS_CLICK, S32(st, G_SCREEN_X) + 0x230, S32(st, G_SCREEN_Y) + 0x17E, G_ARROWS_DOWN, 1);
    if (r != 0) {
        return r;
    }
    return GAME_CALL0(st, FN_MESSAGE_WINDOW_SCROLL_DRAG);
}

void play_build_counts_keep(game_state *st)
{
    /* @bin 0x00417A70 for the 12 build menus (0x005C738C, entries of 0x24) their number of buildings
     * (0x00404120 on the menu 0x007056EC) into 0x006FDC58 */
    uint32_t out = G_BUILD_MENU_COUNTS;
    for (uint32_t e = G_BUILD_MENUS; e < G_BUILD_MENUS + 12u * 0x24u; e += 0x24, out += 4) {
        S32(st, G_SUBMENU) = S32(st, e);
        U32(st, out) = (uint32_t) GAME_CALL0(st, FN_BUILD_MENU_COUNT);
    }
}

void play_build_counts_mark(game_state *st)
{
    /* @bin 0x00417AA0 a menu that has more buildings than kept gets its "new" mark (the byte before
     * its entry, 0x005C738B + 0x24 k) */
    uint32_t mark = G_BUILD_MENUS - 1u;
    for (uint32_t kept = G_BUILD_MENU_COUNTS; kept < G_ADVISOR_AVAILABLE; kept += 4, mark += 0x24) {
        S32(st, G_SUBMENU) = S32(st, mark + 1);
        int32_t n = GAME_CALL0(st, FN_BUILD_MENU_COUNT);
        if (S32(st, kept) < n) {
            U8(st, mark) = 1;
        }
    }
}

/* ---- the military info panel's row of buttons, the empire editor's arrow keys ---- */

void play_info_order_buttons(game_state *st)
{
    /* @bin 0x00515BF0 the order buttons of the info panel (x 0x00EADD90, y 0x00EADD8C, height
     * 0x00EADD84 pieces): for the 15 kinds that are shown (0x00EADCE8 + 4 k) and allowed (0x006FDC88 +
     * k), the button table 0x005F78F0 + 4 k drawn (0x00425730, 1 entry) 0x1B pixels apart, from x + 0x28
     * at the panel's bottom - 0x28; 0x00E38DC9 := 1 first, 0x00E38DC8 := 0 at the end */
    int32_t y = S32(st, G_INFO_H) * 16 - 0x28;
    U8(st, G_REDRAW_SIDEBAR) = 1;
    int32_t dx = 0;
    for (uint32_t k = 0; k < 15; k++) {
        if (S32(st, G_INFO_ADVISORS + 4u * k) != 0 && U8(st, G_ADVISOR_AVAILABLE + k) != 0) {
            GAME_CALL(st, FN_BUTTONS_DRAW, dx + S32(st, G_INFO_X) + 0x28, S32(st, G_INFO_Y) + y,
                      S32(st, G_ADVISOR_BUTTONS + 4u * k), 1);
            dx += 0x1B;
        }
    }
    U8(st, G_PRESENT_NOW) = 0;
}

void play_editor_city_move(game_state *st, int32_t dir, int32_t step)
{
    /* @bin 0x00407C10 in the empire editor (window 0x15) with a city chosen (0x00775928 == 2, the city
     * 0x0077591C, in use: +1 of its record of 0x62 at 0x0077F748): the arrow keys (dir 0 up, 2 right,
     * 4 down, 6 left) move by `step` the city's second position (+0x14 x, +0x16 y; assumed its name's)
     * when 0x00784CB4 is set, else its position (+4 x, +6 y); 0x005D2A2C := 0 */
    if (U8(st, G_WINDOW) != 0x15 || S32(st, G_EDITOR_MODE) != 2) {
        return;
    }
    uint32_t rec = (uint32_t) S32(st, G_EDITOR_OBJECT) * 0x62u;
    if (U8(st, rec + G_EMPIRE_OBJECTS + EMPOBJ_IN_USE) == 0) {
        return;
    }
    int16_t d = (int16_t) step;
    uint32_t x = rec + (S32(st, G_U784CB4) != 0 ? G_EMPIRE_OBJECTS + EMPOBJ_X2 : G_EMPIRE_OBJECTS + EMPOBJ_X), y = x + 2;
    if (dir == 0) {
        S16(st, y) = (int16_t) (S16(st, y) - d);
    } else if (dir == 2) {
        S16(st, x) = (int16_t) (S16(st, x) + d);
    } else if (dir == 4) {
        S16(st, y) = (int16_t) (S16(st, y) + d);
    } else if (dir == 6) {
        S16(st, x) = (int16_t) (S16(st, x) - d);
    }
    S32(st, G_U5D2A2C) = 0;
}

uint8_t play_advisor_next(game_state *st, uint32_t index)
{
    /* @bin 0x004179A0 the advisor [index] if available (0x006FDC88 + i), else the next available one
     * (1 .. 14, wrapping to 1; [index] moved to it): 1, or 0 when none is (back to the start) */
    int32_t i = (int32_t) P32(st, index);
    if (U8(st, G_ADVISOR_AVAILABLE + (uint32_t) i) != 0) {
        return 1;
    }
    int32_t start = i;
    for (;;) {
        i = (int32_t) P32(st, index) + 1;
        P32(st, index) = (uint32_t) i;
        if (i >= 0xF) {
            P32(st, index) = 1;
        }
        i = (int32_t) P32(st, index);
        if (i == start) {
            return 0;
        }
        if (U8(st, G_ADVISOR_AVAILABLE + (uint32_t) i) != 0) {
            return 1;
        }
    }
}

/* ---- port table ---- */

static int32_t run_messages(game_state *st, const int32_t *a) { (void) a; play_screen_messages(st); return 0; }
static int32_t run_network(game_state *st, const int32_t *a) { (void) a; play_network_info(st); return 0; }
static int32_t run_panel(game_state *st, const int32_t *a) { play_button_panel(st, a[0], a[1], a[2], a[3], a[4], a[5]); return 0; }
static int32_t run_pulse(game_state *st, const int32_t *a) { return play_button_pulse(st, a[0]); }
static int32_t run_layout(game_state *st, const int32_t *a) { (void) a; play_layout_fill(st); return 0; }
static int32_t run_widest(game_state *st, const int32_t *a) { return texts_widest(st, a[0]); }
static int32_t run_menu_widths(game_state *st, const int32_t *a) { (void) a; menu_widths(st); return 0; }
static int32_t run_bar_width(game_state *st, const int32_t *a) { (void) a; menu_bar_width(st); return 0; }
static int32_t run_file(game_state *st, const int32_t *a) { (void) a; layout_file_read(st); return 0; }
static int32_t run_briefing(game_state *st, const int32_t *a) { (void) a; return play_briefing_input(st); }
static int32_t run_keep(game_state *st, const int32_t *a) { (void) a; play_build_counts_keep(st); return 0; }
static int32_t run_mark(game_state *st, const int32_t *a) { (void) a; play_build_counts_mark(st); return 0; }
static int32_t run_advisor_next(game_state *st, const int32_t *a) { return play_advisor_next(st, (uint32_t) a[0]); }
static int32_t run_orders(game_state *st, const int32_t *a) { (void) a; play_info_order_buttons(st); return 0; }
static int32_t run_city_move(game_state *st, const int32_t *a) { play_editor_city_move(st, a[0], a[1]); return 0; }

const ported_function play_ports[] = {
    {0x004E2720u, "play_screen_messages", 0, 0, 0, run_messages, 0},
    {0x00536620u, "play_network_info", 0, 0, 0, run_network, 0},
    {0x00536DC0u, "play_button_panel", 6, 0, 0, run_panel, 0},
    {0x00536FD0u, "play_button_pulse", 1, 32, 0, run_pulse, 0},
    {0x004D5D90u, "play_layout_fill", 0, 0, 0, run_layout, 0},
    {0x004D5F80u, "play_texts_widest", 1, 32, 0, run_widest, 0},
    {0x004D5DD0u, "play_menu_widths", 0, 0, 0, run_menu_widths, 0},
    {0x004D5E70u, "play_menu_bar_width", 0, 0, 0, run_bar_width, 0},
    {0x004D69B0u, "play_layout_file_read", 0, 0, 0, run_file, 0},
    {0x004D6900u, "play_layout_list_read (in 0x004D69B0)", 3, 0, 0, NULL, 0},
    {0x00418FE0u, "play_briefing_input", 0, 32, 0, run_briefing, 0},
    {0x00417A70u, "play_build_counts_keep", 0, 0, 0, run_keep, 0},
    {0x00417AA0u, "play_build_counts_mark", 0, 0, 0, run_mark, 0},
    {0x00515BF0u, "play_info_order_buttons", 0, 0, 0, run_orders, 0},
    {0x004179A0u, "play_advisor_next", 1, 8, 0, run_advisor_next, 0},
    {0x00407C10u, "play_editor_city_move", 2, 0, 0, run_city_move, 0},
};
const int play_ports_count = sizeof(play_ports) / sizeof(play_ports[0]);
