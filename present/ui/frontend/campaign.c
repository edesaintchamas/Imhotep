/* Imhotep - the front end: the campaign (period) choice, window 0x17 (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 0x17 (docs/newgame.md 2): the full redraw 0x0041C2D0 (the background picture), per frame
 * 0x0041BE10 (the chosen period's picture and texts, the nine period buttons of 0x0057BE78, the two
 * tabs of the Cleopatra campaign) and, when the chosen period can start (0x0041BB30), the start /
 * back buttons of 0x0057BE30 (0x00425730 by the dispatcher). Input (0x004D8310): 0x00425990 on
 * 0x0057BE30 (0x0041BB70 start, 0x0041F650 back), 0x004261D0 on 0x0057BE78 (0x0041BB10 a period,
 * 0x0041C310 / 0x0041C350 the tabs), a right click the family's menu 0x004CB6A0.
 * Globals: 0x006FDC98 the campaign (0 Pharaoh, 1 Cleopatra), 0x007027AC the chosen period,
 * 0x007027B0 the first period not reached. */
#include "frontend.h"

#include <string.h>

#include "draw/text.h"
#include "sim/call.h"
#include "state/ported.h"

#define CAMPAIGN_KIND 0x006FDC98u   /* i32 0 Pharaoh, 1 Cleopatra */
#define PERIOD 0x007027ACu          /* i32 the chosen period */
#define PERIOD_REACHED 0x007027B0u  /* i32 */
#define CAMPAIGN_TABLE 0x006FDD08u
#define PERIOD_BUTTONS 0x0057BE78u  /* 11 rectangles of 0x1C: 9 periods, 2 tabs */

int32_t fe_text_expand(game_state *st, uint32_t text, char *out, int32_t size);

static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, FE_IMAGES, 0x0053E590u, id, x, y);
}

int32_t period_valid(game_state *st, int32_t period)
{
    /* @bin 0x0041BA00 (thiscall on the campaign table) 1 for a period 0..8 */
    (void) st;
    return period >= 0 && period < 9;
}

int32_t period_flag(game_state *st, int32_t period)
{
    /* @bin 0x0041BAD0 (thiscall on the campaign table) xor al, al: always 0 (the low byte of the
     * argument cleared; callers test al) */
    (void) st;
    return (int32_t) ((uint32_t) period & 0xFFFFFF00u);
}

int32_t period_buttons_count(game_state *st)
{
    /* @bin 0x0041BB30 how many of the start / back buttons (0x0057BE30) are shown: Pharaoh 1 when the
     * chosen period is the one reached, else 0; Cleopatra 2 (0 when 0x0041BAD0 says so) */
    if (S32(st, CAMPAIGN_KIND) == 0) {
        return S32(st, PERIOD) == S32(st, PERIOD_REACHED);
    }
    uint8_t f = (uint8_t) period_flag(st, S32(st, PERIOD));
    return (f != 0 ? -2 : 0) + 2;
}

void period_screen_draw(game_state *st)
{
    /* @bin 0x0041C2D0 window 0x17's full redraw: the 1024 x 768 picture 0x00F55CBC centred
     * (-0xC0, -0x90 from the layout origin); 0x00F53F28 := 100 */
    image(st, S32(st, 0x00F55CBCu), S32(st, FE_DX) - 0xC0, S32(st, FE_DY) - 0x90);
    S32(st, 0x00F53F28u) = 100;
    U8(st, 0x00E38DC9u) = 1;
}

void period_screen_frame(game_state *st)
{
    /* @bin 0x0041BE10 */
    int32_t w = S32(st, FE_SCREEN_W), h = S32(st, FE_SCREEN_H);
    /* the clip of the 1024 x 768 picture: its part on the screen */
    S32(st, 0x00D3B56Cu) = (0x400 - w) / 2;
    S32(st, 0x00D3B574u) = w;
    S32(st, 0x00D3B568u) = (0x300 - h) / 2;
    S32(st, 0x00D3B570u) = h;
    U8(st, 0x00E38DC9u) = 1;
    int32_t kind = S32(st, CAMPAIGN_KIND);
    int32_t y = S32(st, FE_DY) + (kind == 0 ? 0x3C : 0x38);
    int32_t period = S32(st, PERIOD);
    int32_t pic;
    /* the period's picture: Pharaoh's 0x00F55C94 + period, Cleopatra's of 0x00F57314 (+4, 2, 5, 7) */
    if (period < 5) {
        pic = S32(st, 0x00F55C94u) + period;
    } else if (period == 5) {
        pic = S32(st, 0x00F57314u) + 4;
    } else if (period == 6) {
        pic = S32(st, 0x00F57314u) + 2;
    } else if (period == 7) {
        pic = S32(st, 0x00F57314u) + 5;
    } else {
        pic = S32(st, 0x00F57314u) + 7;
    }
    image(st, pic, S32(st, FE_DX) + 0x4E, y);
    /* the title: string 5 (Pharaoh) or 6 of group 0x125, expanded */
    GAME_CALL(st, 0x004CC110u, 0x125, S32(st, CAMPAIGN_KIND) == 0 ? 5 : 6);
    uint32_t text = U32(st, 0x00E20768u);
    uint32_t saved = U32(st, 0x00784F0Cu);
    U32(st, 0x00784F0Cu) = 0;
    char buf[201];
    memset(buf, 0, sizeof(buf));
    fe_text_expand(st, text, buf, 200);
    U32(st, 0x00784F0Cu) = saved;
    text_draw_centred_host(st, buf, S32(st, FE_DX) + 0x14, S32(st, FE_DY) + 0x11, 600, S32(st, 0x00F56FB4u), 0);
    if ((uint8_t) period_valid(st, S32(st, PERIOD))) {
        /* the period's name (group 0x126, 4 period) and its description (4 period + 1 played, 2
         * current, 3 not reached; Cleopatra 2, or 3 by 0x0041BAD0) */
        GAME_CALL(st, 0x004CCB30u, 0x126, S32(st, PERIOD) * 4, S32(st, FE_DX) + 0x153, S32(st, FE_DY) + 0x3C, 0x11B,
                  S32(st, 0x00F56FBCu), 0);
        int32_t d;
        if (S32(st, CAMPAIGN_KIND) == 0) {
            int32_t p = S32(st, PERIOD), r = S32(st, PERIOD_REACHED);
            d = p < r ? 1 : (p != r) + 2;
        } else {
            d = ((uint8_t) period_flag(st, S32(st, PERIOD)) != 0) + 2;
        }
        GAME_CALL(st, 0x004CCFD0u, 0x126, d + S32(st, PERIOD) * 4, S32(st, FE_DX) + 0x15B, S32(st, FE_DY) + 0x74, 0x10D,
                  S32(st, 0x00F56F94u), 0);
    }
    /* the nine period buttons: closed ones (Pharaoh: not the period reached; Cleopatra: by
     * 0x0041BA00 / 0x0041BAD0 on the entry's parameter) greyed (font 0x00F56FA4, never lit) */
    int32_t start = 0;
    int32_t i = 0;
    for (uint32_t e = PERIOD_BUTTONS; e < 0x0057BF74u; e += 0x1C) {
        int32_t x0 = S16(st, e);
        int32_t bx = x0 + S32(st, FE_DX);
        int32_t by = S16(st, e + 2) + S32(st, FE_DY);
        int32_t width = S16(st, e + 4) - x0;
        int32_t lit = S32(st, FE_HOVER) == i + 1;
        int32_t param = S32(st, e + 0x14);
        int32_t closed;
        if (S32(st, CAMPAIGN_KIND) == 0) {
            closed = i != S32(st, PERIOD_REACHED);
        } else {
            closed = (uint8_t) period_valid(st, param) && (uint8_t) period_flag(st, param);
        }
        if (i == S32(st, PERIOD)) {
            start = !closed;
        }
        int32_t font = S32(st, FE_FONT_LARGE);
        if (closed) {
            lit = 0;
            font = S32(st, 0x00F56FA4u);
        }
        fe_button_draw(st, 6, bx, by, width / 16, lit);
        GAME_CALL(st, 0x004CD240u, 0x1B, i, (int32_t) e, font, 0, 0, 6);
        i++;
    }
    int32_t k = S32(st, CAMPAIGN_KIND);
    if (k == 1 || k == 0) {
        GAME_CALL(st, 0x004CCFD0u, 0x126, 0x29, S32(st, FE_DX) + 0x1E, S32(st, FE_DY) + 0x10A, 0x90, S32(st, FE_FONT_LARGE), 0);
        GAME_CALL(st, 0x004CCFD0u, 0x126, 0x2A, S32(st, FE_DX) + 0x1E, S32(st, FE_DY) + 0x16E, 0x90, S32(st, FE_FONT_LARGE), 0);
        k = S32(st, CAMPAIGN_KIND);
    }
    if (k == 1) {
        /* the tabs (entries 9, 10: the mission list, the periods; the second always lit) */
        static const int32_t labels[2] = {0x26, 0x27};
        for (int32_t t = 0; t < 2; t++) {
            int32_t lit = S32(st, FE_HOVER) == t + 10;
            if (t == 1) {
                lit = 1;
            }
            uint32_t rect = PERIOD_BUTTONS + 0x1Cu * (uint32_t) (t + 9);
            GAME_CALL(st, 0x004CD1A0u, 0, (int32_t) rect, lit, 0, 0);
            GAME_CALL(st, 0x004CD240u, 0x126, labels[t], (int32_t) rect, S32(st, FE_FONT_LARGE), 0, 4, 7);
        }
        k = S32(st, CAMPAIGN_KIND);
    }
    if (start) {
        /* the start button's labels */
        if (k == 0) {
            GAME_CALL(st, 0x004CCBB0u, 0x126, 0x24, S32(st, FE_DX) + 0x1A4, S32(st, FE_DY) + 0x1BD, S32(st, FE_FONT_LARGE), 0);
            U8(st, FE_REDRAW) = 1;
            return;
        }
        GAME_CALL(st, 0x004CCBB0u, 0x126, 0x25, S32(st, FE_DX) + 0x226, S32(st, FE_DY) + 0x1BD, S32(st, FE_FONT_LARGE), 0);
        GAME_CALL(st, 0x004CCBB0u, 0x2C, 0xD9, S32(st, FE_DX) + 0x17C, S32(st, FE_DY) + 0x1BD, S32(st, FE_FONT_LARGE), 0);
    }
    U8(st, FE_REDRAW) = 1;
}

void period_click(game_state *st)
{
    /* @bin 0x0041BB10 a period button: 0x007027AC := its parameter, a full redraw */
    int32_t p = S32(st, FE_PARAM1);
    U8(st, FE_REDRAW) = 1;
    S32(st, PERIOD) = p;
}

void period_missions_tab(game_state *st)
{
    /* @bin 0x0041C310 the Cleopatra campaign's mission list tab: in window 0x17 of the Cleopatra
     * campaign, the list (0x0041F870, mode 1, kind 2) */
    if (U8(st, FE_WINDOW) != 0x17 || S32(st, CAMPAIGN_KIND) != 1) {
        return;
    }
    S32(st, 0x00704DB0u) = 1;
    S32(st, 0x00605A3Cu) = 0;
    U8(st, 0x006DE515u) = 0;
    S32(st, 0x00704DACu) = 2;
    GAME_CALL0(st, 0x0041F870u);
    U8(st, FE_REDRAW) = 1;
}

void period_periods_tab(game_state *st)
{
    /* @bin 0x0041C350 the periods tab: window 0x17 of the Cleopatra campaign */
    if (U8(st, FE_WINDOW) == 0x17) {
        return;
    }
    U8(st, FE_WINDOW) = 0x17;
    S32(st, CAMPAIGN_KIND) = 1;
    U8(st, FE_REDRAW) = 1;
}

void list_back_click(game_state *st)
{
    /* @bin 0x0041F650 back: in the editor (0x00605A3C) the main menu (window 0), else the family's
     * menu (jmp 0x004CB6A0) */
    if (S32(st, 0x00605A3Cu) != 0) {
        U8(st, FE_WINDOW) = 0;
        U8(st, FE_REDRAW) = 1;
        return;
    }
    GAME_CALL0(st, 0x004CB6A0u);
}

static int32_t run_valid(game_state *st, const int32_t *a) { return period_valid(st, a[0]); }
static int32_t run_flag(game_state *st, const int32_t *a) { return period_flag(st, a[0]); }
static int32_t run_count(game_state *st, const int32_t *a) { (void) a; return period_buttons_count(st); }
static int32_t run_draw(game_state *st, const int32_t *a) { (void) a; period_screen_draw(st); return 0; }
static int32_t run_frame(game_state *st, const int32_t *a) { (void) a; period_screen_frame(st); return 0; }
static int32_t run_click(game_state *st, const int32_t *a) { (void) a; period_click(st); return 0; }
static int32_t run_tab1(game_state *st, const int32_t *a) { (void) a; period_missions_tab(st); return 0; }
static int32_t run_tab2(game_state *st, const int32_t *a) { (void) a; period_periods_tab(st); return 0; }
static int32_t run_back(game_state *st, const int32_t *a) { (void) a; list_back_click(st); return 0; }

const ported_function frontend_campaign_ports[] = {
    {0x0041BA00u, "period_valid", 1, 8, CAMPAIGN_TABLE, run_valid, 0},
    {0x0041BAD0u, "period_flag", 1, 8, CAMPAIGN_TABLE, run_flag, 0},
    {0x0041BB30u, "period_buttons_count", 0, 32, 0, run_count, 0},
    {0x0041C2D0u, "period_screen_draw", 0, 0, 0, run_draw, 0},
    {0x0041BE10u, "period_screen_frame", 0, 0, 0, run_frame, 0},
    {0x0041BB10u, "period_click", 0, 0, 0, run_click, 0},
    {0x0041C310u, "period_missions_tab", 0, 0, 0, run_tab1, 0},
    {0x0041C350u, "period_periods_tab", 0, 0, 0, run_tab2, 0},
    {0x0041F650u, "list_back_click", 0, 0, 0, run_back, 0},
};
const int frontend_campaign_ports_count = sizeof(frontend_campaign_ports) / sizeof(frontend_campaign_ports[0]);
