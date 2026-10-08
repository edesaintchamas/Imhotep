/* Imhotep - the front end's first screen: the main menu, window 0 (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 0: the full redraw 0x00520AB0 (the background picture), the per-frame 0x004180C0 (the five
 * large buttons of the rectangle table 0x0057BD88 and their labels), the input (0x004D8310: hover
 * 0x00426450 / 0x004D6740, click 0x004261D0 on 0x0057BD88), whose entries all call 0x00417FF0 with
 * their parameter 0..4 in 0x00D3B198. */
#include "frontend.h"

#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define MENU_TABLE 0x0057BD88u       /* 5 rectangles of 0x1C: i16 x0, y0, x1, y1, callback +8, param +0x14 */
#define MENU_TABLE_END 0x0057BE14u
#define MENU_LABELS 0x0057BE14u      /* i32[5] the labels' string indexes in group 0x1E */
#define MENU_BACKGROUND 0x00F55C24u  /* i32 the main menu's picture */
#define BUTTON_STYLE 0x00F53F10u     /* i32[] the large buttons' style by kind */

static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, FE_IMAGES, 0x0053E590u, id, x, y);
}

void fe_button_draw(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t width, int32_t lit)
{
    /* @bin 0x00536710 a large button `width` pieces of 16 pixels wide at (x, y): by the style
     * [0x00F53F10 + 4 kind]: below 5 nothing; 5..14 one picture of the table 0x005F81E4 (3 dwords per
     * style: image offset, dx, dy) centred on the width; from 15 a row of at most style - 13 pieces
     * (left, middle, right; +1 the lit ones), centred when narrower */
    int32_t style = S32(st, BUTTON_STYLE + 4u * (uint32_t) kind);
    if (style < 5) {
        return;
    }
    if (style < 15) {
        uint32_t e = 0x005F81E4u + 4u * (uint32_t) (style * 3 - 15);
        int32_t ny = S32(st, e + 8) + y;
        int32_t nx = (int32_t) ((uint32_t) (width - 2) << 4) / 2 + S32(st, e + 4) + x;
        S32(st, 0x00D3B5E0u) = nx;
        S32(st, 0x00D3B5DCu) = ny;
        image(st, S32(st, e) + lit * 8 + 0x18 + S32(st, FE_BUTTON_IMAGES), nx, ny);
        return;
    }
    int32_t n = style - 13;
    if (n > width) {
        n = width;
    } else {
        x += (int32_t) ((uint32_t) (width - n) << 4) / 2;
    }
    for (int32_t i = 0; i < n; i++) {
        int32_t px = i * 16 + x;
        S32(st, 0x00D3B5DCu) = y;
        S32(st, 0x00D3B5E0u) = px;
        int32_t base = S32(st, FE_BUTTON_IMAGES) + lit * 2;
        int32_t id;
        if (i == 0) {
            id = base + lit;
        } else if (i < n - 1) {
            id = lit + base + 1;
        } else {
            id = lit + base + 2;
        }
        S32(st, 0x00D3B5C0u) = id;
        image(st, id, px, y);
    }
}

void main_menu_draw(game_state *st, int32_t keep)
{
    /* @bin 0x00520AB0 window 0's full redraw: the screen cleared (0x004CF1C0) unless keep, the
     * picture at the layout origin; 0x00F53F28 := 100 */
    if (keep == 0) {
        GAME_CALL0(st, 0x004CF1C0u);
    }
    image(st, S32(st, MENU_BACKGROUND), S32(st, FE_DX), S32(st, FE_DY));
    S32(st, 0x00F53F28u) = 100;
}

void main_menu_frame(game_state *st)
{
    /* @bin 0x004180C0 the five buttons (kind 6, (x1 - x0) / 16 pieces, lit when hovered: 0x007056C8
     * = entry + 1) and their labels (group 0x1E, centred in the rectangle, large font, colour 0,
     * 6 pixels down) */
    uint32_t label = MENU_LABELS;
    int32_t k = 0;
    for (uint32_t e = MENU_TABLE; e < MENU_TABLE_END; e += 0x1C, label += 4) {
        k++;
        int32_t x0 = S16(st, e);
        int32_t lit = S32(st, FE_HOVER) == k;
        int32_t pieces = (S16(st, e + 4) - x0) / 16;
        fe_button_draw(st, 6, x0 + S32(st, FE_DX), S16(st, e + 2) + S32(st, FE_DY), pieces, lit);
        GAME_CALL(st, 0x004CD240u, 0x1E, S32(st, label), (int32_t) e, S32(st, FE_FONT_LARGE), 0, 0, 6);
    }
}

void main_menu_click(game_state *st)
{
    /* @bin 0x00417FF0 a main menu button, 0x00D3B198: 0 the families (0x00516BB0, the sidebar
     * expanded), 1 the publisher's web site (ShowWindow, ShellExecuteA), 2 the scenario
     * editor's map list, 3 window 0x53 after 0x004C5920, 4 quit (PostMessageA Esc).
     * Before the choice: 0x00E38EA8 := 0, 0x005D2A2C := 1 */
    int32_t p = S32(st, FE_PARAM1);
    U8(st, 0x00E38EA8u) = 0;
    S32(st, 0x005D2A2Cu) = 1;
    if ((uint32_t) p > 4) {
        return;
    }
    switch (p) {
    case 0:
        GAME_CALL0(st, 0x00516BB0u);
        S32(st, 0x00F53F78u) = 0;
        return;
    case 2:
        /* @bin 0x00418051 the scenario editor: campaign kind 2, editor mode, no pause, the map list */
        S32(st, 0x00E38EBCu) = 2;
        S32(st, 0x00605A3Cu) = 1;
        U8(st, 0x00E38E60u) = 0;
        S32(st, 0x00F53F78u) = 0;
        S32(st, 0x00704DB0u) = 0;
        S32(st, 0x00704DACu) = 3;
        GAME_CALL0(st, 0x0041F870u);
        return;
    case 3:
        GAME_CALL0(st, 0x004C5920u);
        U8(st, FE_WINDOW) = 0x53;
        return;
    case 1:
        /* @bin 0x00418026 ShowWindow(window 0x00E39140, SW_MINIMIZE), ShellExecuteA(window, "open"
         * (0x005761D8), "www.activision.com" (0x0057BFB0), 0, 0, SW_SHOW): the platform's open_url
         * (minimizing the game's window is the platform's part of the request) */
        platform_open_url(st, "www.activision.com");
        return;
    default:
        /* @bin 0x00418092 4: PostMessageA(window, WM_KEYDOWN, VK_ESCAPE, 0): the quit question comes
         * from the window procedure later */
        platform_post_message(st, 0x100, 0x1B, 0);
        return;
    }
}

static int32_t run_button(game_state *st, const int32_t *a) { fe_button_draw(st, a[0], a[1], a[2], a[3], a[4]); return 0; }
static int32_t run_draw(game_state *st, const int32_t *a) { main_menu_draw(st, a[0]); return 0; }
static int32_t run_frame(game_state *st, const int32_t *a) { (void) a; main_menu_frame(st); return 0; }
static int32_t run_click(game_state *st, const int32_t *a) { (void) a; main_menu_click(st); return 0; }

const ported_function frontend_menu_ports[] = {
    {0x00536710u, "fe_button_draw", 5, 0, 0, run_button, 0},
    {0x00520AB0u, "main_menu_draw", 1, 0, 0, run_draw, 0},
    {0x004180C0u, "main_menu_frame", 0, 0, 0, run_frame, 0},
    {0x00417FF0u, "main_menu_click", 0, 0, 0, run_click, 0},
};
const int frontend_menu_ports_count = sizeof(frontend_menu_ports) / sizeof(frontend_menu_ports[0]);
