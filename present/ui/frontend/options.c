/* Imhotep - the options dialogs: display (window 0x0A), sound (0x0B), speed (0x0C), difficulty
 * (0x45) and the hot keys (0x46) (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Each dialog is drawn every frame (0x004D75D0: 0x00531190, 0x00531EF0 + the arrows 0x004256A0 on
 * 0x005C8DF8, 0x00531CC0 + arrows on 0x005C8D98, 0x00531BE0, 0x00537260) and takes its input in
 * 0x004D8310 (0x004261D0 on 0x005C2E20 / 0x005C3070 / 0x005C3038, 0x00425C40 on the arrow tables
 * 0x005C8DF8 / 0x005C8D98 / 0x005C8EE8; a right click closes it). The callbacks below write the
 * settings block (0x00E38E58: Pharaoh.inf): music 0x00E38E64, speech 0x00E38E65, effects 0x00E38E63,
 * city sounds 0x00E38EC0 (u8 flags) and their volumes 0x00E38EC8, 0x00E38ECC, 0x00E38EC4, 0x00E38ED0;
 * game speed 0x00E38E6C, scroll speed 0x00E38E70. The values when the dialog opened are kept in
 * 0x006057A0.. (cancel). Volumes go to the sound driver (0x0055F639 the stream's, 0x0055F38D /
 * 0x0055F32F / 0x0055F3EB the channels' defaults: platform requests, core/sim/platform.h). */
#include "frontend.h"

#include "draw/draw.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define SOUND 0x00E92BF8u        /* `this` of the sound driver's wrappers */
#define MUSIC_ON 0x00E38E64u     /* u8 */
#define SPEECH_ON 0x00E38E65u    /* u8 */
#define EFFECTS_ON 0x00E38E63u   /* u8 */
#define CITY_ON 0x00E38EC0u      /* u8 */
#define MUSIC_VOL 0x00E38EC8u    /* i32 */
#define SPEECH_VOL 0x00E38ECCu   /* i32 */
#define EFFECTS_VOL 0x00E38EC4u  /* i32 */
#define CITY_VOL 0x00E38ED0u     /* i32 */
#define GAME_SPEED 0x00E38E6Cu   /* i32 */
#define SCROLL_SPEED 0x00E38E70u /* i32 */
#define LAYOUT_READY 0x00E94D9Cu
#define LAYOUT_MODE 0x00E94D98u
#define MOUSE_X 0x00E38E34u
#define MOUSE_Y 0x00E38E30u
#define PERCENT 0x005E03FCu      /* the "%" suffix of the volumes */

void fe_small_button(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t width, int32_t plain, int32_t look)
{
    /* @bin 0x00537000 a small button of `width` pieces at (x, y), look 0..: unless plain = 1 only for
     * styles ([0x00F53F10 + 4 kind]) from 0x14, at most style - 0x12 pieces, centred when narrower;
     * pieces 0x28 + 3 look (left), +1 (middle), 3 (look + 0xE) (right) of the buttons' images */
    int32_t n = width;
    if (plain != 1) {
        int32_t style = S32(st, 0x00F53F10u + 4u * (uint32_t) kind);
        if (style < 0x14) {
            return;
        }
        n = style - 0x12;
        if (n > width) {
            n = width;
        } else {
            x += (int32_t) ((uint32_t) (width - n) << 4) / 2;
        }
    }
    for (int32_t i = 0; i < n; i++) {
        int32_t px = i * 16 + x;
        S32(st, 0x00D3B5DCu) = y;
        S32(st, 0x00D3B5E0u) = px;
        int32_t base = S32(st, FE_BUTTON_IMAGES);
        int32_t id;
        if (i == 0) {
            id = look + base + look * 2 + 0x28;
        } else if (i >= n - 1) {
            id = (look + 0x0E) + base + (look + 0x0E) * 2;
        } else {
            id = look + base + look * 2 + 0x29;
        }
        S32(st, 0x00D3B5C0u) = id;
        GAME_CALLT(st, FE_IMAGES, 0x0053E590u, id, px, y);
    }
}

void layout_small_button(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t width, int32_t plain,
                         int32_t look, int32_t dx, int32_t dy)
{
    /* @bin 0x004D6480 a small button (0x00537000) placed through the layout registry (list 3, its
     * width only: present/draw/layout.c); in layout mode 3 with the cursor inside it (16 width x
     * 24) outlined as 0x004D63D0 does */
    if (S32(st, LAYOUT_READY) == 0) {
        GAME_CALL0(st, 0x004D5D90u);
    }
    int32_t ox = x - dx, oy = y - dy;
    int32_t nx = ox, ny = oy;
    uint32_t node = layout_find(st, 3, layout_window_key(st, U8(st, FE_WINDOW)), ox, oy);
    if (node != 0) {
        width = S32(st, node + 0x10);
        nx = S32(st, node + 8);
        ny = S32(st, node + 0x0C);
    }
    int32_t px = dx + nx, py = ny + dy;
    if (S32(st, LAYOUT_MODE) == 3 && px < S32(st, MOUSE_X) && width * 16 + px > S32(st, MOUSE_X) &&
        py < S32(st, MOUSE_Y) && py + 0x18 > S32(st, MOUSE_Y)) {
        layout_mode_panel(st);
        S32(st, 0x00E94600u) = nx;
        S32(st, 0x00E94604u) = ny;
        S32(st, 0x00E94608u) = width;
        S32(st, 0x00E9460Cu) = 0x18;
        S32(st, 0x00E945F8u) = ox;
        S32(st, 0x00E945FCu) = oy;
        panel_draw_frame(st, 1, px, py, width * 16, 0x18, 0xFFFF);
        panel_draw_frame(st, 0, px, py, width * 16, 0x18, 0xFFFF);
    }
    fe_small_button(st, kind, px, py, width, plain, look);
}

void layout_text_group_rect(game_state *st, int32_t group, int32_t index, uint32_t rect, int32_t font, int32_t colour,
                            int32_t ox, int32_t oy, int32_t dx, int32_t dy)
{
    /* @bin 0x004D6380 a string centred in the rectangle's width at its corner + (ox, oy) + (dx, dy),
     * through the layout registry (0x004D6250) */
    int32_t x0 = S16(st, rect);
    GAME_CALL(st, 0x004D6250u, group, index, x0 + ox + dx, S16(st, rect + 2) + oy + dy, S16(st, rect + 4) - x0, font,
              colour, dx, dy);
}

static int32_t hover_colour(game_state *st, int32_t k)
{
    /* the labels of the buttons: 0xFAC1 when hovered, else 0x02BF (neg / sbb / and 0xFFFF07FE /
     * add 0xFAC1) */
    return (int32_t) (((S32(st, FE_HOVER) != k) ? 0xFFFF07FEu : 0u) + 0xFAC1u);
}

static void small(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t w, int32_t k)
{
    int32_t dx = S32(st, FE_DX), dy = S32(st, FE_DY);
    layout_small_button(st, kind, dx + x, dy + y, w, 0, (S32(st, FE_HOVER) != k) + 1, dx, dy);
}

static void centred(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t w, int32_t font,
                    int32_t colour)
{
    int32_t dx = S32(st, FE_DX), dy = S32(st, FE_DY);
    GAME_CALL(st, 0x004D6250u, group, index, dx + x, dy + y, w, font, colour, dx, dy);
}

static void group_at(game_state *st, int32_t group, int32_t index, int32_t x, int32_t y, int32_t font, int32_t colour)
{
    int32_t dx = S32(st, FE_DX), dy = S32(st, FE_DY);
    GAME_CALL(st, 0x004D62F0u, group, index, dx + x, dy + y, font, colour, dx, dy);
}

static void number_at(game_state *st, int32_t value, int32_t x, int32_t y)
{
    int32_t dx = S32(st, FE_DX), dy = S32(st, FE_DY);
    GAME_CALL(st, 0x004D6860u, value, 0x40, (int32_t) PERCENT, dx + x, dy + y, S32(st, FE_FONT_LARGE), 0, dx, dy);
}

/* ---- display (window 0x0A) ---- */

void display_options_draw(game_state *st)
{
    /* @bin 0x00531190 the panel (18 x 12), five buttons (fullscreen / window, 640, 800, 1024, back)
     * and their labels (group 0x2A: 1 or 2 by 0x00E38E5C, 3..6), the clip reset; 0x00F53F18 := 50 */
    int32_t font = S32(st, FE_FONT_LARGE);
    GAME_CALL(st, 0x00536820u, S32(st, FE_DX) + 0x60, S32(st, FE_DY) + 0x50, 0x12, 0x0C);
    for (int32_t k = 1; k <= 5; k++) {
        small(st, 2, 0x80, 0x70 + 0x18 * k, 0x0E, k);
    }
    centred(st, 0x2A, 0, 0x80, 0x5E, 0xE0, S32(st, 0x00F56FB4u), 0);
    int32_t c = hover_colour(st, 1);
    centred(st, 0x2A, S32(st, 0x00E38E5Cu) == 0 ? 1 : 2, 0x80, 0x8C, 0xE0, font, c);
    for (int32_t k = 2; k <= 5; k++) {
        centred(st, 0x2A, k + 1, 0x80, 0x74 + 0x18 * k, 0xE0, font, hover_colour(st, k));
    }
    GAME_CALL0(st, 0x005370D0u);
    S32(st, 0x00F53F18u) = 0x32;
}

void display_fullscreen_click(game_state *st)
{
    /* @bin 0x00402C10 PostMessageA(window 0x00E39140, WM_KEYDOWN, VK_F6, 0) (the window procedure
     * toggles the full screen later), the city, a full redraw */
    platform_post_message(st, 0x100, 0x75, 0);
    U8(st, FE_WINDOW) = 1;
    U8(st, FE_REDRAW) = 1;
}

void display_resolution_click(game_state *st)
{
    /* @bin 0x00402C40 PostMessageA(window 0x00E39140, WM_KEYDOWN, VK_F7 / F8 / F9 for 0x00D3B198 = 1 /
     * 2 / 3, 0) (the window procedure changes the resolution later); whatever the parameter, the city
     * (window 1, no redraw) */
    int32_t p = S32(st, FE_PARAM1);
    if (p == 1) {
        platform_post_message(st, 0x100, 0x76, 0);
    } else if (p == 2) {
        platform_post_message(st, 0x100, 0x77, 0);
    } else if (p == 3) {
        platform_post_message(st, 0x100, 0x78, 0);
    }
    U8(st, FE_WINDOW) = 1;
}

void fe_close_to_city(game_state *st)
{
    /* @bin 0x004EA760 back to the city (window 1), a full redraw */
    U8(st, FE_WINDOW) = 1;
    U8(st, FE_REDRAW) = 1;
}

/* ---- sound (window 0x0B) ---- */

void sound_options_draw(game_state *st)
{
    /* @bin 0x00531EF0 the panel (24 x 18), the four switches and the OK / cancel buttons, the title
     * (group 0x2E, 0), their labels (0xC, 9 coloured by the hover), the column titles (10, 11), each
     * switch's state (1 + on, 3 + on, 5 + on, 7 + on) and its volume (%); the clip reset; 0x00F53F1C
     * := 50 */
    int32_t font = S32(st, FE_FONT_LARGE);
    GAME_CALL(st, 0x00536820u, S32(st, FE_DX) + 0x30, S32(st, FE_DY) + 0x50, 0x18, 0x12);
    small(st, 3, 0x40, 0xA2, 0x0E, 1);
    small(st, 3, 0x40, 0xC0, 0x0E, 2);
    small(st, 3, 0x40, 0xDE, 0x0E, 3);
    small(st, 3, 0x40, 0xFC, 0x0E, 4);
    small(st, 3, 0x90, 0x128, 0x0C, 5);
    small(st, 3, 0x90, 0x146, 0x0C, 6);
    centred(st, 0x2E, 0, 0x60, 0x5C, 0x120, S32(st, 0x00F56FB4u), 0);
    centred(st, 0x2E, 0x0C, 0x80, 0x12C, 0xE0, font, hover_colour(st, 5));
    centred(st, 0x2E, 9, 0x80, 0x14A, 0xE0, font, hover_colour(st, 6));
    group_at(st, 0x2E, 10, 0x70, 0x8E, S32(st, FE_FONT_LARGE), 0);
    group_at(st, 0x2E, 11, 0x150, 0x8E, S32(st, FE_FONT_LARGE), 0);
    centred(st, 0x2E, (U8(st, MUSIC_ON) != 0) + 1, 0x40, 0xA6, 0xE0, font, 0);
    number_at(st, S32(st, MUSIC_VOL), 0x172, 0xA6);
    centred(st, 0x2E, (U8(st, SPEECH_ON) != 0) + 3, 0x40, 0xC4, 0xE0, font, 0);
    number_at(st, S32(st, SPEECH_VOL), 0x172, 0xC4);
    centred(st, 0x2E, (U8(st, EFFECTS_ON) != 0) + 5, 0x40, 0xE2, 0xE0, font, 0);
    number_at(st, S32(st, EFFECTS_VOL), 0x172, 0xE2);
    centred(st, 0x2E, (U8(st, CITY_ON) != 0) + 7, 0x40, 0x100, 0xE0, font, 0);
    number_at(st, S32(st, CITY_VOL), 0x172, 0x100);
    GAME_CALL0(st, 0x005370D0u);
    S32(st, 0x00F53F1Cu) = 0x32;
}

void sound_switch_click(game_state *st)
{
    /* @bin 0x00403060 a switch (0x00D3B198): 1 music, 2 speech, 4 city sounds (turned off: the
     * channel's stream 0, 1, 2 stopped, 0x0055F561), 3 effects */
    if (S32(st, FE_PARAM1) == 1) {
        if (U8(st, MUSIC_ON) == 0) {
            U8(st, MUSIC_ON) = 1;
            U8(st, FE_REDRAW) = 1;
            return;
        }
        U8(st, MUSIC_ON) = 0;
        GAME_CALLT(st, SOUND, 0x0055F561u, 0);
    }
    if (S32(st, FE_PARAM1) == 2) {
        if (U8(st, SPEECH_ON) == 0) {
            U8(st, SPEECH_ON) = 1;
            U8(st, FE_REDRAW) = 1;
            return;
        }
        U8(st, SPEECH_ON) = 0;
        GAME_CALLT(st, SOUND, 0x0055F561u, 1);
    }
    int32_t p = S32(st, FE_PARAM1);
    if (p == 3) {
        U8(st, FE_REDRAW) = 1;
        U8(st, EFFECTS_ON) = U8(st, EFFECTS_ON) == 0;
        return;
    }
    if (p == 4) {
        if (U8(st, CITY_ON) == 0) {
            U8(st, CITY_ON) = 1;
            U8(st, FE_REDRAW) = 1;
            return;
        }
        U8(st, CITY_ON) = 0;
        GAME_CALLT(st, SOUND, 0x0055F561u, 2);
    }
    U8(st, FE_REDRAW) = 1;
}

void sound_cancel_click(game_state *st)
{
    /* @bin 0x00402F70 cancel: the settings as they were (0x006057A4 .. 0x006057C0), music stopped when
     * it was off, the volumes given to the driver again; the city */
    S32(st, MUSIC_VOL) = S32(st, 0x006057BCu);
    U8(st, SPEECH_ON) = U8(st, 0x006057B8u);
    S32(st, SPEECH_VOL) = S32(st, 0x006057B4u);
    U8(st, EFFECTS_ON) = U8(st, 0x006057B0u);
    S32(st, EFFECTS_VOL) = S32(st, 0x006057ACu);
    uint8_t music = U8(st, 0x006057C0u);
    U8(st, MUSIC_ON) = music;
    U8(st, CITY_ON) = U8(st, 0x006057A8u);
    S32(st, CITY_VOL) = S32(st, 0x006057A4u);
    if (music == 0) {
        GAME_CALLT(st, SOUND, 0x0055F561u, 0);
    }
    GAME_CALLT(st, SOUND, 0x0055F639u, 0, S32(st, MUSIC_VOL));
    GAME_CALLT(st, SOUND, 0x0055F38Du, S32(st, MUSIC_VOL));
    GAME_CALLT(st, SOUND, 0x0055F639u, 2, S32(st, CITY_VOL));
    GAME_CALLT(st, SOUND, 0x0055F3EBu, S32(st, CITY_VOL));
    GAME_CALLT(st, SOUND, 0x0055F639u, 1, S32(st, SPEECH_VOL));
    GAME_CALLT(st, SOUND, 0x0055F32Fu, S32(st, SPEECH_VOL));
    U8(st, FE_WINDOW) = 1;
    U8(st, FE_REDRAW) = 1;
}

static int32_t volume_step(game_state *st, uint32_t a, int32_t step)
{
    /* 0x00D3B198: 1 down, 0 up; clamped to 0..100 */
    int32_t p = S32(st, FE_PARAM1);
    if (p == 1) {
        S32(st, a) = S32(st, a) - step;
    } else if (p == 0) {
        S32(st, a) = S32(st, a) + step;
    }
    int32_t v = S32(st, a);
    if (v < 0) {
        S32(st, a) = 0;
    } else if (v > 100) {
        S32(st, a) = 100;
    }
    return S32(st, a);
}

void music_volume_click(game_state *st)
{
    /* @bin 0x00403130 the music volume +-1: to the stream and the channel's default */
    int32_t v = volume_step(st, MUSIC_VOL, 1);
    GAME_CALLT(st, SOUND, 0x0055F639u, 0, v);
    GAME_CALLT(st, SOUND, 0x0055F38Du, S32(st, MUSIC_VOL));
    U8(st, FE_REDRAW) = 1;
}

void speech_volume_click(game_state *st)
{
    /* @bin 0x004031A0 the speech volume +-1 */
    int32_t v = volume_step(st, SPEECH_VOL, 1);
    GAME_CALLT(st, SOUND, 0x0055F639u, 1, v);
    GAME_CALLT(st, SOUND, 0x0055F32Fu, S32(st, SPEECH_VOL));
    U8(st, FE_REDRAW) = 1;
}

void effects_volume_click(game_state *st)
{
    /* @bin 0x00403210 the effects volume +-1 */
    volume_step(st, EFFECTS_VOL, 1);
    U8(st, FE_REDRAW) = 1;
}

void city_volume_click(game_state *st)
{
    /* @bin 0x00403270 the city sounds' volume +-1: the channel's default */
    volume_step(st, CITY_VOL, 1);
    GAME_CALLT(st, SOUND, 0x0055F3EBu, S32(st, CITY_VOL));
    U8(st, FE_REDRAW) = 1;
}

/* ---- speed (window 0x0C) ---- */

void speed_options_draw(game_state *st)
{
    /* @bin 0x00531CC0 the panel (18 x 14), OK / cancel, the title (group 0x2D, 0) and labels (4, 1),
     * game speed (2) and scroll speed (3) with their values (%); clip reset; 0x00F53F20 := 50 */
    int32_t font = S32(st, FE_FONT_LARGE);
    GAME_CALL(st, 0x00536820u, S32(st, FE_DX) + 0x60, S32(st, FE_DY) + 0x50, 0x12, 0x0E);
    small(st, 4, 0x90, 0xE8, 0x0C, 1);
    small(st, 4, 0x90, 0x106, 0x0C, 2);
    centred(st, 0x2D, 0, 0x80, 0x5C, 0xE0, S32(st, 0x00F56FB4u), 0);
    centred(st, 0x2D, 4, 0x80, 0xEC, 0xE0, font, hover_colour(st, 1));
    centred(st, 0x2D, 1, 0x80, 0x10A, 0xE0, font, hover_colour(st, 2));
    group_at(st, 0x2D, 2, 0x90, 0x92, S32(st, FE_FONT_LARGE), 0);
    number_at(st, S32(st, GAME_SPEED), 0x128, 0x92);
    group_at(st, 0x2D, 3, 0x90, 0xB6, S32(st, FE_FONT_LARGE), 0);
    number_at(st, S32(st, SCROLL_SPEED), 0x128, 0xB6);
    GAME_CALL0(st, 0x005370D0u);
    S32(st, 0x00F53F20u) = 0x32;
}

void speed_cancel_click(game_state *st)
{
    /* @bin 0x004032F0 cancel: the speeds as they were (0x006057A0, 0x0060579C), the city */
    S32(st, GAME_SPEED) = S32(st, 0x006057A0u);
    S32(st, SCROLL_SPEED) = S32(st, 0x0060579Cu);
    U8(st, FE_WINDOW) = 1;
    U8(st, FE_REDRAW) = 1;
}

void scroll_speed_click(game_state *st)
{
    /* @bin 0x00403390 the scroll speed +-10, within 0..100 */
    volume_step(st, SCROLL_SPEED, 10);
    U8(st, FE_REDRAW) = 1;
}

/* ---- difficulty (window 0x45) and hot keys (0x46) ---- */

void difficulty_dialog_draw(game_state *st)
{
    /* @bin 0x00531BE0 the panel (24 x 12), the title (group 0x99, 0), the difficulty's name (1 +
     * 0x00E39080), its arrows (0x004256A0 on 0x005C8EE8) and the note (8) */
    GAME_CALL(st, 0x00536820u, S32(st, FE_DX) + 0x30, S32(st, FE_DY) + 0x50, 0x18, 0x0C);
    centred(st, 0x99, 0, 0x30, 0x5E, 0x180, S32(st, 0x00F56FB4u), 0);
    centred(st, 0x99, S32(st, 0x00E39080u) + 1, 0x50, 0xAE, 0xE0, S32(st, FE_FONT_LARGE), 0);
    GAME_CALL(st, 0x004256A0u, S32(st, FE_DX) + 0x120, S32(st, FE_DY) + 0x70, 0x005C8EE8, 2);
    centred(st, 0x99, 8, 0x30, 0xF6, 0x180, S32(st, FE_FONT_LARGE), 0);
}

void hotkeys_draw(game_state *st)
{
    /* @bin 0x00537260 the panel (21 x 14), title (group 0x2C, 0xA4), note (0xD, 3), the column titles
     * (0x2C: 0xAE, 0xAF, 0xB0), then five rows: the name (group 0x9D), two key boxes (rectangles of
     * 0x005C6CD8 and 0x005C6DF0, lit by the hover k + 1 / k + 0x0B) with their keys (group 0xBB of
     * the i16 0x007848E2 + 2k, group 0x12 of 0x00784DB0 + 2k) */
    int32_t x0 = S32(st, FE_DX);
    int32_t y0 = S32(st, FE_DY);
    GAME_CALL(st, 0x00536820u, x0, y0, 0x15, 0x0E);
    GAME_CALL(st, 0x004D62F0u, 0x2C, 0xA4, x0 + 10, y0 + 10, S32(st, 0x00F56FB4u), 0, S32(st, FE_DX), S32(st, FE_DY));
    GAME_CALL(st, 0x004D6250u, 0x0D, 3, x0, y0 + 200, 0x150, S32(st, FE_FONT_LARGE), 0, S32(st, FE_DX), S32(st, FE_DY));
    int32_t ty = y0 + 0x2E;
    GAME_CALL(st, 0x004D62F0u, 0x2C, 0xAE, x0 + 0x14, ty, S32(st, FE_FONT_LARGE), 0, S32(st, FE_DX), S32(st, FE_DY));
    GAME_CALL(st, 0x004D62F0u, 0x2C, 0xAF, x0 + 0x84, ty, S32(st, FE_FONT_LARGE), 0, S32(st, FE_DX), S32(st, FE_DY));
    GAME_CALL(st, 0x004D62F0u, 0x2C, 0xB0, x0 + 0xC5, ty, S32(st, FE_FONT_LARGE), 0, S32(st, FE_DX), S32(st, FE_DY));
    int32_t y = y0 + 0x46;
    uint32_t key = 0x00784DB0u;
    int32_t i = 0;
    for (uint32_t rect = 0x005C6DF0u; rect < 0x005C6E7Cu; rect += 0x1C, key += 2, i++, y += 0x18) {
        int32_t lit = S32(st, FE_HOVER) == i + 1;
        int32_t font = S32(st, FE_FONT_LARGE);
        GAME_CALL(st, 0x004D62F0u, 0x9D, i, x0 + 0x14, y, S32(st, 0x00F56F9Cu), 0, S32(st, FE_DX), S32(st, FE_DY));
        GAME_CALL(st, 0x004CD1A0u, 1, (int32_t) (rect - 0x118), lit, 0, 0);
        layout_text_group_rect(st, 0xBB, S16(st, key - 0x4CE), rect - 0x118, font, 0, 4, 4, S32(st, FE_DX), S32(st, FE_DY));
        GAME_CALL(st, 0x004CD1A0u, 1, (int32_t) rect, S32(st, FE_HOVER) == i + 0x0B, 0, 0);
        layout_text_group_rect(st, 0x12, S16(st, key), rect, font, 0, 4, 4, S32(st, FE_DX), S32(st, FE_DY));
    }
}

static int32_t run_small(game_state *st, const int32_t *a) { fe_small_button(st, a[0], a[1], a[2], a[3], a[4], a[5]); return 0; }
static int32_t run_lsmall(game_state *st, const int32_t *a) { layout_small_button(st, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]); return 0; }
static int32_t run_lrect(game_state *st, const int32_t *a)
{
    layout_text_group_rect(st, a[0], a[1], (uint32_t) a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
    return 0;
}
#define RUN0(f) static int32_t run_##f(game_state *st, const int32_t *a) { (void) a; f(st); return 0; }
RUN0(display_options_draw)
RUN0(display_fullscreen_click)
RUN0(display_resolution_click)
RUN0(fe_close_to_city)
RUN0(sound_options_draw)
RUN0(sound_switch_click)
RUN0(sound_cancel_click)
RUN0(music_volume_click)
RUN0(speech_volume_click)
RUN0(effects_volume_click)
RUN0(city_volume_click)
RUN0(speed_options_draw)
RUN0(speed_cancel_click)
RUN0(scroll_speed_click)
RUN0(difficulty_dialog_draw)
RUN0(hotkeys_draw)

const ported_function frontend_options_ports[] = {
    {0x00537000u, "fe_small_button", 6, 0, 0, run_small, 0},
    {0x004D6480u, "layout_small_button", 8, 0, 0, run_lsmall, 0},
    {0x004D6380u, "layout_text_group_rect", 9, 0, 0, run_lrect, 0},
    {0x00531190u, "display_options_draw", 0, 0, 0, run_display_options_draw, 0},
    {0x00402C10u, "display_fullscreen_click", 0, 0, 0, run_display_fullscreen_click, 0},
    {0x00402C40u, "display_resolution_click", 0, 0, 0, run_display_resolution_click, 0},
    {0x004EA760u, "fe_close_to_city", 0, 0, 0, run_fe_close_to_city, 0},
    {0x00531EF0u, "sound_options_draw", 0, 0, 0, run_sound_options_draw, 0},
    {0x00403060u, "sound_switch_click", 0, 0, 0, run_sound_switch_click, 0},
    {0x00402F70u, "sound_cancel_click", 0, 0, 0, run_sound_cancel_click, 0},
    {0x00403130u, "music_volume_click", 0, 0, 0, run_music_volume_click, 0},
    {0x004031A0u, "speech_volume_click", 0, 0, 0, run_speech_volume_click, 0},
    {0x00403210u, "effects_volume_click", 0, 0, 0, run_effects_volume_click, 0},
    {0x00403270u, "city_volume_click", 0, 0, 0, run_city_volume_click, 0},
    {0x00531CC0u, "speed_options_draw", 0, 0, 0, run_speed_options_draw, 0},
    {0x004032F0u, "speed_cancel_click", 0, 0, 0, run_speed_cancel_click, 0},
    {0x00403390u, "scroll_speed_click", 0, 0, 0, run_scroll_speed_click, 0},
    {0x00531BE0u, "difficulty_dialog_draw", 0, 0, 0, run_difficulty_dialog_draw, 0},
    {0x00537260u, "hotkeys_draw", 0, 0, 0, run_hotkeys_draw, 0},
};
const int frontend_options_ports_count = sizeof(frontend_options_ports) / sizeof(frontend_options_ports[0]);
