/* Imhotep - the empire map of the scenario editor (windows 0x15, 0x16) (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 0x15 (from 0x19): full redraw 0x0052D4E0 (frame, the mode buttons 0x0052E6F0, the panel of
 * the selected object 0x0052EC50); per frame (inline in 0x004D75D0) the map 0x0052F8B0, the title,
 * 0x0052E6F0, 0x0052EC50. Window 0x16: a number dialog 0x005305E0. The editor's mode is
 * 0x00775928 (1 object picture, 2 object, 0x0E / 0x0F route, others a hint), the object
 * 0x0077591C, the route 0x005D2A30. */
#include "empire.h"

#include "sim/call.h"
#include "state/ported.h"

#define GFX 0x00F55BB0u
#define LAYOUT_DX 0x00E38DA8u
#define LAYOUT_DY 0x00E38DA4u
#define SCR_W 0x00E38DB0u
#define SCR_H 0x00E38DACu
#define TEXT_X 0x00E20778u
#define FONT_SMALL 0x00F56F84u
#define FONT_HEAD 0x00F56F8Cu
#define FONT_TITLE 0x00F56FB4u
#define CLIP_BOTTOM 0x00D3B550u
#define CLIP_TOP 0x00D3B554u
#define CLIP_RIGHT 0x00D3B558u
#define CLIP_LEFT 0x00D3B55Cu
#define IMG_COUNT 0x0126517Cu
#define IMG_ENTRIES 0x010D69F8u
#define MODE 0x00775928u           /* i32 the editor's mode */
#define OBJECT 0x0077591Cu         /* i32 the selected object */
#define OBJECT_IMAGE 0x00775924u   /* i32 the object picture chosen */
#define PICTURE_KIND 0x00775920u   /* i32 0..2 */
#define ROUTE 0x005D2A30u          /* i32 the selected route */
#define HOVER 0x007056C8u
#define ICONS 0x00F571FCu

static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, GFX, 0x0053E590u, id, x, y);
}

static int32_t dx(game_state *st) { return S32(st, LAYOUT_DX); }
static int32_t dy(game_state *st) { return S32(st, LAYOUT_DY); }
static int32_t h(game_state *st) { return S32(st, SCR_H); }

static void centred(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t w, int32_t font, int32_t colour)
{
    GAME_CALL(st, 0x004D6250u, g, i, x, y, w, font, colour, dx(st), dy(st));
}

static void centred_lines(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t w, int32_t font,
                          int32_t colour)
{
    GAME_CALL(st, 0x004D6160u, g, i, x, y, w, font, colour, dx(st), dy(st));
}

static void group(game_state *st, int32_t g, int32_t i, int32_t x, int32_t y, int32_t font, int32_t colour)
{
    GAME_CALL(st, 0x004D62F0u, g, i, x, y, font, colour, dx(st), dy(st));
}

static void raised(game_state *st, int32_t x, int32_t y, int32_t w, int32_t hh)
{
    GAME_CALL(st, 0x004CD930u, x, y, w, hh);
}

static void number_centred(game_state *st, int32_t v, int32_t x, int32_t y, int32_t w, int32_t font)
{
    GAME_CALL(st, 0x004CCE90u, v, 0x40, 0x0057C048, x, y, w, font, 0);
}

static int32_t lit(int cond)
{
    /* @bin 0x0052E701 (-(x != k) & 0xFFFF0800) + 0xF800: red when selected, else 0 */
    return cond ? 0xF800 : 0;
}

static int32_t name_group(game_state *st)
{
    return U8(st, 0x00E39088u) != 0 ? 0x15 : 0xC3;
}

static uint32_t img_entry(game_state *st, int32_t id)
{
    return IMG_ENTRIES + ((uint32_t) (id < S32(st, IMG_COUNT) ? id : 0) << 6);
}

void editor_modes_draw(game_state *st)
{
    /* @bin 0x0052E6F0 the mode buttons' labels (group 0x2C), lit by the mode */
    int32_t y = h(st);
    int32_t m = S32(st, MODE);
    centred(st, 0x2C, 0, 0x0F, y - 0x54, 100, S32(st, FONT_HEAD), lit(m == 1));
    centred(st, 0x2C, 1, 0x0F, y - 0x40, 100, S32(st, FONT_HEAD), lit(S32(st, MODE) == 2));
    centred(st, 0x2C, 2, 0x0F, y - 0x2C, 100, S32(st, FONT_HEAD), lit(S32(st, MODE) == 3));
    centred(st, 0x2C, 3, 0x0F, y - 0x18, 100, S32(st, FONT_HEAD), lit(S32(st, MODE) == 4));
    centred(st, 0x2C, 0xA6, 0x77, y - 0x54, 100, S32(st, FONT_HEAD), lit(S32(st, MODE) == 0x0E));
    centred(st, 0x2C, 0xA7, 0x77, y - 0x40, 100, S32(st, FONT_HEAD), lit(S32(st, MODE) == 0x0F));
    centred(st, 0x2C, 0xDF, 0x77, y - 0x2C, 100, S32(st, FONT_HEAD), 0);
    centred(st, 0x2C, 7, 0x77, y - 0x18, 100, S32(st, FONT_HEAD), 0);
}

void editor_hint_draw(game_state *st)
{
    /* @bin 0x0052F850 */
    S32(st, TEXT_X) = 0;
    raised(st, 0xE7, h(st) - 0x22, 0x96, 0x14);
    centred(st, 0x2C, 0xCC, 0xE7, h(st) - 0x1E, 0x78, S32(st, FONT_HEAD), 0);
}

void editor_route_panel_draw(game_state *st)
{
    /* @bin 0x0052E8C0 mode 0x0E / 0x0F: the route number, its kind (army +0x140: 1 0xAB, 2 0xAA,
     * else 0xA9), its two ends' names (+0x13C, +0x13E), its length (points +0x134 times +0) */
    int32_t y = h(st) - 0x68;
    raised(st, 0x189, y, 0xB4, 0x14);
    int32_t colour = S32(st, HOVER) != 1 ? 0 : 0xF800;
    S32(st, TEXT_X) = 0;
    centred_lines(st, 0x2C, 0x20, 0x19D, y + 4, 100, S32(st, FONT_HEAD), colour);
    GAME_CALL(st, 0x004D6860u, S32(st, ROUTE), 0x40, 0x007027D4, S32(st, TEXT_X) + 0x19D, y + 4, S32(st, FONT_HEAD),
              colour, dx(st), dy(st));
    raised(st, 0x189, y + 0x19, 0xB4, 0x14);
    uint32_t army = 0x00776FB0u + 0x144u * (uint32_t) S32(st, ROUTE);
    uint8_t kind = U8(st, army + 0x140);
    int32_t index = kind == 1 ? 0xAB : (kind == 2 ? 0xAA : 0xA9);
    centred_lines(st, 0x2C, index, 0x19D, y + 0x1D, 0xB4, S32(st, FONT_HEAD), 0);
    raised(st, 0x189, y + 0x4B, 0xB4, 0x14);
    S32(st, TEXT_X) = 0;
    centred_lines(st, 0x2C, 0xA8, 0x19D, y + 0x4F, 0xB4, S32(st, FONT_HEAD), 0);
    army = 0x00776FB0u + 0x144u * (uint32_t) S32(st, ROUTE);
    int32_t from = S16(st, army + 0x13C);
    if (from != -1) {
        centred(st, name_group(st), U8(st, 0x0077F761u + 0x62u * (uint32_t) from), 0x251, h(st) - 0x6C, 0xB4,
                S32(st, FONT_HEAD), 0);
    }
    army = 0x00776FB0u + 0x144u * (uint32_t) S32(st, ROUTE);
    int32_t to = S16(st, army + 0x13E);
    if (to != -1) {
        centred(st, name_group(st), U8(st, 0x0077F761u + 0x62u * (uint32_t) to), 0x251, h(st) - 0x58, 0xB4,
                S32(st, FONT_HEAD), 0);
    }
    GAME_CALL(st, 0x004CD9D0u, 0x251, h(st) - 0x44, 0xB4, 0x14, 0xF73B);
    S32(st, TEXT_X) = 0;
    centred_lines(st, 0x2C, 0xAD, 0x251, h(st) - 0x44, 0xB4, S32(st, FONT_HEAD), 0);
    army = 0x00776FB0u + 0x144u * (uint32_t) S32(st, ROUTE);
    int32_t len = (int32_t) ((uint32_t) S32(st, army + 0x134) * (uint32_t) U8(st, army));
    GAME_CALL(st, 0x004D6860u, len, 0x40, 0x007027D4, S32(st, TEXT_X) + 0x251, h(st) - 0x44, S32(st, FONT_HEAD), 0,
              dx(st), dy(st));
}

static void object_picture(game_state *st, int animate)
{
    /* the chosen picture in its white square, with its animation frame (entry +0x3A: animated,
     * +0x3C frame, +0x22 / +0x24 offset); mode 2 steps the animation (0x0052F790) */
    int32_t id = S32(st, OBJECT_IMAGE);
    GAME_CALL(st, 0x004CD9D0u, 0x125, h(st) - 0x68, 0x5A, 0x5A, 0xFFFF);
    image(st, id, 0x126, h(st) - 0x67);
    uint32_t e = img_entry(st, id);
    if (U8(st, e + 0x3A) == 0) {
        return;
    }
    int32_t ox = S16(st, e + 0x22);
    int32_t oy = S16(st, e + 0x24);
    if (animate) {
        U8(st, e + 0x3C) = (uint8_t) image_anim_step(st, id, U8(st, e + 0x3C));
    }
    image(st, (int32_t) U8(st, e + 0x3C) + id, ox + 0x126, h(st) + oy - 0x67);
}

static void goods_icon(game_state *st, uint8_t res, int32_t x, int32_t y)
{
    int32_t base = S32(st, ICONS);
    int32_t o = GAME_CALL(st, 0x0047D350u, (int32_t) res, 3);
    image(st, o + base + res, x, y);
}

void editor_panel_draw(game_state *st)
{
    /* @bin 0x0052EC50 the panel by the editor's mode */
    int32_t m = S32(st, MODE);
    if (m == 1) {
        S32(st, CLIP_LEFT) = 0x126;
        S32(st, CLIP_TOP) = 0x17E;
        S32(st, CLIP_RIGHT) = h(st) - 0x67;
        S32(st, CLIP_BOTTOM) = h(st) - 0x0F;
        if (S32(st, PICTURE_KIND) == 0) {
            object_picture(st, 0);
        }
        S32(st, CLIP_BOTTOM) = h(st);
        S32(st, CLIP_RIGHT) = S32(st, SCR_W);
        S32(st, CLIP_LEFT) = 0;
        S32(st, CLIP_TOP) = 0;
        GAME_CALL(st, 0x004CD9D0u, 0x183, h(st) - 0x6E, S32(st, SCR_W) - 0x198, 0x62, 0xC618);
        number_centred(st, S32(st, OBJECT_IMAGE) + 1, 0xE1, h(st) - 0x1C, 0x30, S32(st, FONT_HEAD));
        int32_t c = lit(S32(st, PICTURE_KIND) == 0);
        raised(st, 0x189, h(st) - 0x6A, 0xB4, 0x14);
        centred(st, 0x2C, 8, 0x189, h(st) - 0x66, 0xB4, S32(st, FONT_HEAD), c);
        c = lit(S32(st, PICTURE_KIND) == 1);
        raised(st, 0x189, h(st) - 0x51, 0xB4, 0x14);
        centred(st, 0x2C, 9, 0x189, h(st) - 0x4D, 0xB4, S32(st, FONT_HEAD), c);
        c = lit(S32(st, PICTURE_KIND) == 2);
        raised(st, 0x189, h(st) - 0x38, 0xB4, 0x14);
        centred(st, 0x2C, 10, 0x189, h(st) - 0x34, 0xB4, S32(st, FONT_HEAD), c);
        return;
    }
    if (m == 2) {
        S32(st, CLIP_LEFT) = 0x126;
        S32(st, CLIP_TOP) = 0x17E;
        S32(st, CLIP_RIGHT) = h(st) - 0x67;
        S32(st, CLIP_BOTTOM) = h(st) - 0x0F;
        GAME_CALL(st, 0x004CD9D0u, 0xE1, h(st) - 0x36, 0x30, 0x14, 0xF73B);
        number_centred(st, S32(st, OBJECT_IMAGE) + 1, 0xE1, h(st) - 0x34, 0x30, S32(st, FONT_HEAD));
        if (U8(st, 0x0077F748u + 0x62u * (uint32_t) S32(st, OBJECT)) == 0) {
            object_picture(st, 1);
        }
        S32(st, CLIP_RIGHT) = S32(st, SCR_W);
        S32(st, CLIP_BOTTOM) = h(st);
        S32(st, CLIP_LEFT) = 0;
        S32(st, CLIP_TOP) = 0;
        GAME_CALL(st, 0x004CD9D0u, 0x183, h(st) - 0x6E, S32(st, SCR_W) - 0x198, 0x62, 0xF73B);
        number_centred(st, S32(st, OBJECT_IMAGE) + 1, 0xE1, h(st) - 0x1C, 0x30, S32(st, FONT_HEAD));
        S32(st, TEXT_X) = 0;
        uint32_t o = 0x0077F748u + 0x62u * (uint32_t) S32(st, OBJECT);
        uint8_t kind = U8(st, o);
        if (kind == 1) {
            /* a city: its kind, name, picture set, the counts, what it trades */
            raised(st, 0x189, h(st) - 0x70, 0xB4, 0x14);
            centred(st, 0x27, U8(st, o + 0x18), 0x189, h(st) - 0x6C, 0xB4, S32(st, FONT_HEAD), 0);
            raised(st, 0x251, h(st) - 0x70, 0xB4, 0x14);
            centred(st, name_group(st), U8(st, o + 0x19), 0x251, h(st) - 0x6C, 0xB4, S32(st, FONT_HEAD), 0);
            raised(st, 0x125, h(st) - 0x70, 100, 0x14);
            centred(st, 0x56, U8(st, o + 0x13), 0x125, h(st) - 0x6C, 100, S32(st, FONT_HEAD), 0);
            group(st, 0x2C, 0x20, 0x189, h(st) - 0x53, S32(st, FONT_HEAD), 0);
            raised(st, 0x1C5, h(st) - 0x57, 0x32, 0x14);
            number_centred(st, U8(st, o + 0x1A), 0x1C5, h(st) - 0x53, 0x32, S32(st, FONT_HEAD));
            if (U8(st, o + 0x18) != 0) {
                raised(st, 0x20B, h(st) - 0x57, 100, 0x14);
                centred(st, 0x2C, 0x21, 0x20B, h(st) - 0x53, 100, S32(st, FONT_HEAD), 0);
            }
            group(st, 0x2C, 0x22, 0x28D, h(st) - 0x53, S32(st, FONT_HEAD), 0);
            raised(st, 0x2BF, h(st) - 0x57, 0x50, 0x14);
            number_centred(st, S16(st, o + 0x1C), 0x2BF, h(st) - 0x53, 0x3C, S32(st, FONT_HEAD));
            if (U8(st, o + 0x18) == 0) {
                uint32_t p = o + 0x25;
                for (int32_t x = 0x18F; x < 0x24C; x += 0x1B, p++) {
                    GAME_CALL(st, 0x004CD890u, x - 1, h(st) - 0x3E, 0x1A, 0x1A);
                    goods_icon(st, U8(st, p - 7), x, h(st) - 0x3D);
                    GAME_CALL(st, 0x004CD890u, x - 1, h(st) - 0x22, 0x1A, 0x1A);
                    goods_icon(st, U8(st, p), x, h(st) - 0x21);
                }
                return;
            }
            group(st, 0x2C, 0x24, 0x189, h(st) - 0x37, S32(st, FONT_HEAD), 0);
            uint32_t p = o + 0x1E;
            for (int32_t x = 0x20C; x < 0x2FC; x += 0x1E, p++) {
                GAME_CALL(st, 0x004CD890u, x - 1, h(st) - 0x3E, 0x1A, 0x1A);
                goods_icon(st, U8(st, p), x, h(st) - 0x3D);
            }
            group(st, 0x2C, 5, 0x189, h(st) - 0x1B, S32(st, FONT_HEAD), 0);
            p = o + 0x34;
            for (int32_t x = 0x20C; x < 0x2FC; x += 0x1E, p++) {
                GAME_CALL(st, 0x004CD890u, x - 1, h(st) - 0x22, 0x1A, 0x1A);
                goods_icon(st, U8(st, p), x, h(st) - 0x21);
            }
            return;
        }
        if (kind == 3) {
            group(st, 0x2C, 0x45, 0x189, h(st) - 100, S32(st, FONT_TITLE), 0);
            group(st, 0x2C, 0x46, 0x1A7, h(st) - 0x40, S32(st, FONT_HEAD), 0);
            raised(st, 0x20B, h(st) - 0x44, 0x32, 0x14);
            number_centred(st, U8(st, o + 0x3C), 0x20B, h(st) - 0x40, 0x32, S32(st, FONT_HEAD));
            group(st, 0x2C, 0x47, 0x1A7, h(st) - 0x22, S32(st, FONT_HEAD), 0);
            raised(st, 0x20B, h(st) - 0x26, 0x32, 0x14);
            number_centred(st, U8(st, o + 0x3D), 0x20B, h(st) - 0x22, 0x32, S32(st, FONT_HEAD));
            return;
        }
        if (kind == 2) {
            raised(st, 0x189, h(st) - 0x54, 0xB4, 0x14);
            centred(st, 0xC4, U8(st, o + 0x19), 0x189, h(st) - 0x54, 0xB4, S32(st, FONT_SMALL), 0);
        }
        return;
    }
    if (m == 3) {
        return;
    }
    if (m == 4 || m == 9 || m == 0x0A || m == 0x0B || m == 0x0C || m == 0x0D) {
        editor_hint_draw(st);
        return;
    }
    if (m == 0x0E || m == 0x0F) {
        editor_route_panel_draw(st);
    }
}

void editor_empire_draw(game_state *st)
{
    /* @bin 0x0052D4E0 window 0x15's full redraw */
    empire_frame_draw(st);
    GAME_CALL(st, 0x004CD9D0u, 10, h(st) - 0x74, S32(st, SCR_W) - 0x14, 0x73, 0xF73B);
    raised(st, 10, h(st) - 0x74, S32(st, SCR_W) - 0x14, 0x73);
    int32_t y = h(st);
    group(st, 0x2C, 0xDE, 0x0F, h(st) - 0x74, S32(st, FONT_TITLE), 0);
    raised(st, 0x0F, y - 0x56, 100, 0x12);
    raised(st, 0x0F, y - 0x42, 100, 0x12);
    raised(st, 0x0F, y - 0x2E, 100, 0x12);
    raised(st, 0x0F, y - 0x1A, 100, 0x12);
    raised(st, 0x77, y - 0x56, 100, 0x12);
    raised(st, 0x77, y - 0x42, 100, 0x12);
    raised(st, 0x77, y - 0x2E, 100, 0x12);
    raised(st, 0x77, y - 0x1A, 100, 0x12);
    editor_modes_draw(st);
    GAME_CALL(st, 0x004CD9D0u, 0xDF, h(st) - 0x72, S32(st, SCR_W) - 0xEE, 0x6D, 0xF73B);
    raised(st, 0xDF, h(st) - 0x72, S32(st, SCR_W) - 0xEE, 0x6D);
    editor_panel_draw(st);
}

void editor_number_dialog_draw(game_state *st)
{
    /* @bin 0x005305E0 window 0x16: a panel with its title, the hint, the sign button and the value
     * (0x007848D8, i16) */
    int32_t x0 = S32(st, LAYOUT_DX);
    int32_t y0 = S32(st, LAYOUT_DY);
    GAME_CALL(st, 0x00536820u, x0 + 0x80, y0 + 0x2C, 0x14, 10);
    S32(st, TEXT_X) = 0;
    centred(st, 0x2C, 0x0D, x0 + 0x8A, y0 + 0x38, 0x140, S32(st, FONT_TITLE), 0);
    centred(st, 0x0D, 3, x0 + 0x80, y0 + 0xB2, 0x140, S32(st, FONT_HEAD), 0);
    GAME_CALL(st, 0x004D63D0u, 1, x0 + 0x9E, y0 + 100, 100, 0x1E, S32(st, HOVER) == 1, dx(st), dy(st));
    int16_t v = S16(st, 0x007848D8u);
    group(st, 0x14, v >= 0, x0 + 0xA8, y0 + 0x68, S32(st, FONT_TITLE), 0);
    GAME_CALL(st, 0x004D63D0u, 1, x0 + 0x116, y0 + 100, 0x78, 0x1E, S32(st, HOVER) == 2, dx(st), dy(st));
    v = S16(st, 0x007848D8u);
    GAME_CALL(st, 0x004CCE90u, v < 0 ? -(int32_t) v : (int32_t) v, 0x40, 0x0057C048, x0 + 0x116, y0 + 0x68, 0x78,
              S32(st, FONT_TITLE), 0);
}

#define RUN0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) \
    { \
        (void) a; \
        fn(st); \
        return 0; \
    }
RUN0(run_empire, editor_empire_draw)
RUN0(run_modes, editor_modes_draw)
RUN0(run_panel, editor_panel_draw)
RUN0(run_route, editor_route_panel_draw)
RUN0(run_hint, editor_hint_draw)
RUN0(run_dialog, editor_number_dialog_draw)

const ported_function empire_editor_ports[] = {
    {0x0052D4E0u, "editor_empire_draw", 0, 0, 0, run_empire, 0},
    {0x0052E6F0u, "editor_modes_draw", 0, 0, 0, run_modes, 0},
    {0x0052EC50u, "editor_panel_draw", 0, 0, 0, run_panel, 0},
    {0x0052E8C0u, "editor_route_panel_draw", 0, 0, 0, run_route, 0},
    {0x0052F850u, "editor_hint_draw", 0, 0, 0, run_hint, 0},
    {0x005305E0u, "editor_number_dialog_draw", 0, 0, 0, run_dialog, 0},
};
const int empire_editor_ports_count = sizeof(empire_editor_ports) / sizeof(empire_editor_ports[0]);
