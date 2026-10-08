/* Imhotep - the end of a mission: window 0x33 (won or lost) (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 0x33: the full redraw 0x004194C0 (panel 0x00F53DB8 / 0x00F53DB4 at the layout origin +
 * (0x30, 0x10)), per frame 0x00419DB0 (also for window 0x43) the buttons, drawn at (0x30, 0x80).
 * Won (0x00E38DD9 = 1): the city's score (0x0041C380), ratings and monuments (0x004192B0), and the
 * high score's outcome 0x00D38700 (set by 0x004C5A40, window 0x42; highscore.c). Lost: the reason
 * by 0x004EA880 (1 time ran out, 2 deposed, 3 ...). Input (0x004D8310): buttons of 0x004EA8D0 (lost)
 * or a right click 0x00418640 (won). */
#include "frontend.h"

#include <string.h>

#include "draw/text.h"
#include "sim/call.h"
#include "state/ported.h"

#define PANEL_X 0x00F53DB8u
#define PANEL_Y 0x00F53DB4u
#define LIT 0x00E2079Au  /* u8 the last button's lit state */

void fe_button_draw(game_state *st, int32_t kind, int32_t x, int32_t y, int32_t width, int32_t lit);

int32_t mission_end_kind(game_state *st)
{
    /* @bin 0x004EA880 1 when the city's flag 0x00EA1380 is clear; with 0x00EA17FC clear, 0x00EA1380
     * set, the goal 0x00784C48 and the city's +0x47A8 (0x00EA6018) positive: 3; else 2 */
    uint8_t a = U8(st, 0x00EA1380u);
    if (U8(st, 0x00EA17FCu) == 0) {
        if (a == 0) {
            return 1;
        }
        if (S32(st, 0x00784C48u) != 0 && S32(st, 0x00EA6018u + 0x49D8u * U32(st, 0x00EAAC28u)) > 0) {
            return 3;
        }
    }
    return a != 0 ? 2 : 1;
}

uint32_t mission_end_buttons(game_state *st)
{
    /* @bin 0x004EA8D0 the button table of the same cases: 0x005C6628 (1), 0x005C6648 (2), 0x005C6680 (3) */
    int32_t k = mission_end_kind(st);
    return k == 3 ? 0x005C6680u : k == 2 ? 0x005C6648u : 0x005C6628u;
}

static void host_from_state(game_state *st, char *dst, uint32_t src, size_t size)
{
    size_t i = 0;
    for (; i + 1 < size && P8(st, src + (uint32_t) i) != 0; i++) {
        dst[i] = (char) P8(st, src + (uint32_t) i);
    }
    dst[i] = 0;
}

static void state_from_host(game_state *st, uint32_t dst, const char *src)
{
    /* rep movs of the string and its 0 */
    size_t n = strlen(src);
    for (size_t i = 0; i <= n; i++) {
        P8(st, dst + (uint32_t) i) = (uint8_t) src[i];
    }
}

static void state_copy_str(game_state *st, uint32_t dst, uint32_t src)
{
    uint32_t i = 0;
    do {
        P8(st, dst + i) = P8(st, src + i);
    } while (P8(st, src + i++) != 0);
}

static void label_number(game_state *st, int32_t index, int32_t value, int32_t x, int32_t y)
{
    /* @bin 0x00419603 a label (group 0x94) then its number after it (the text cursor 0x00E20778) */
    S32(st, 0x00E20778u) = 0;
    GAME_CALL(st, 0x004CCBB0u, 0x94, index, S32(st, PANEL_X) + x, S32(st, PANEL_Y) + y, S32(st, FE_FONT_LARGE), 0);
    GAME_CALL(st, 0x004CCE00u, value, 0x40, (int32_t) 0x0057C048u, S32(st, 0x00E20778u) + S32(st, PANEL_X) + x,
              S32(st, PANEL_Y) + y, S32(st, FE_FONT_LARGE), 0);
}

static void mission_won_draw(game_state *st)
{
    /* @bin 0x00419532 won: the score (0x0041C380 of the months played: |i16 0x007848D8 - 0x0078D7BC| *
     * 12 + 0x0078D7C0); the panel (34 x 27), the title (0x3E, 0), a frame, the mission's victory text
     * (group 0x93, 0x004181A0) in lines; the ratings (culture, prosperity, kingdom), treasury and
     * population (0x94: 0, 1, 3, 5, 4); the monuments done (0x004192B0, their title 0x94, 6); the months
     * (7, 0xF), the difficulty (8, group 0x99), the score (9); the high score's outcome 0x00D38700
     * (0x94: 1 0x10, 2 0xA, 3 0xB, 4 0xC; 5 0xE and 6 0xD expanded (0x00448370) with the record's
     * player as the name 0x00E38E74 for the while: 5 the former holder 0x00D386E0, 6 the holder); the
     * note (0xD, 1) */
    uint32_t c = (uint32_t) S32(st, 0x00EAAC28u) * 0x49D8u;
    int32_t d = S16(st, 0x007848D8u) - S32(st, 0x0078D7BCu);
    if (d < 0) {
        d = -d;
    }
    int32_t months = S32(st, 0x0078D7C0u) + d * 12;
    int32_t score = victory_score(st, months);
    if (st->unported) {
        return;
    }
    GAME_CALL(st, 0x00536820u, S32(st, PANEL_X), S32(st, PANEL_Y), 0x22, 0x1B);
    GAME_CALL(st, 0x004CCB30u, 0x3E, 0, S32(st, PANEL_X), S32(st, PANEL_Y) + 0x10, 0x220, S32(st, 0x00F56FB4u), 0);
    GAME_CALL(st, 0x00536A30u, S32(st, PANEL_X) + 0x10, S32(st, PANEL_Y) + 0x38, 0x20, 7);
    int32_t text = mission_victory_text(st);
    GAME_CALL(st, 0x004CCFD0u, 0x93, text, S32(st, PANEL_X) + 0x20, S32(st, PANEL_Y) + 0x40, 0x1F0,
              S32(st, 0x00F56F9Cu), 0);
    c = (uint32_t) S32(st, 0x00EAAC28u) * 0x49D8u;
    label_number(st, 0, S32(st, 0x00EA5B3Cu + (uint32_t) S32(st, 0x00EAAC28u) * 0x49D8u), 0x28, 0xB4);
    label_number(st, 1, S32(st, 0x00EA5B40u + (uint32_t) S32(st, 0x00EAAC28u) * 0x49D8u), 0x28, 0xC8);
    label_number(st, 3, S32(st, 0x00EA5B48u + (uint32_t) S32(st, 0x00EAAC28u) * 0x49D8u), 0x28, 0xDC);
    label_number(st, 5, S32(st, 0x00EA187Cu + (uint32_t) S32(st, 0x00EAAC28u) * 0x49D8u), 0x28, 0xF0);
    label_number(st, 4, S32(st, 0x00EA1894u + (uint32_t) S32(st, 0x00EAAC28u) * 0x49D8u), 0x28, 0x104);
    (void) c;
    int32_t done = victory_monuments_draw(st);
    if (st->unported) {
        return;
    }
    if (done != 0) {
        GAME_CALL(st, 0x004CCBB0u, 0x94, 6, S32(st, PANEL_X) + 0x113, S32(st, PANEL_Y) + 0xB4, S32(st, FE_FONT_LARGE), 0);
    }
    label_number(st, 7, months, 0x78, 0x118);
    GAME_CALL(st, 0x004CCBB0u, 0x94, 0xF, S32(st, 0x00E20778u) + S32(st, PANEL_X) + 0x78, S32(st, PANEL_Y) + 0x118,
              S32(st, FE_FONT_LARGE), 0);
    S32(st, 0x00E20778u) = 0;
    GAME_CALL(st, 0x004CCBB0u, 0x94, 8, S32(st, PANEL_X) + 0xA0, S32(st, PANEL_Y) + 0x12C, S32(st, FE_FONT_LARGE), 0);
    GAME_CALL(st, 0x004CCBB0u, 0x99, S32(st, 0x00EA6018u + (uint32_t) S32(st, 0x00EAAC28u) * 0x49D8u) + 1,
              S32(st, 0x00E20778u) + S32(st, PANEL_X) + 0xA0, S32(st, PANEL_Y) + 0x12C, S32(st, FE_FONT_LARGE), 0);
    label_number(st, 9, score, 0xC8, 0x140);
    int32_t outcome = S32(st, 0x00D38700u);
    static const int32_t texts[5] = {0x10, 0xA, 0xB, 0xC};
    if (outcome >= 1 && outcome <= 4) {
        GAME_CALL(st, 0x004CCFD0u, 0x94, texts[outcome - 1], S32(st, PANEL_X) + 0x28, S32(st, PANEL_Y) + 0x154, 0x220,
                  S32(st, FE_FONT_LARGE), 0);
    } else if (outcome == 5 || outcome == 6) {
        char saved[64], buf[201];
        memset(buf, 0, sizeof(buf));
        uint32_t holder;
        int32_t from;
        if (outcome == 5) {
            GAME_CALL(st, 0x004CC110u, 0x94, 0xE);
            holder = 0x00D386E0u;
        } else {
            GAME_CALL(st, 0x004CC110u, 0x94, 0xD);
            holder = 0x00D38710u + (uint32_t) (int8_t) U8(st, 0x00E38EA8u) * 0x4Cu;
        }
        from = S32(st, 0x00E20768u);
        host_from_state(st, saved, 0x00E38E74u, sizeof(saved));
        state_copy_str(st, 0x00E38E74u, holder);
        fe_text_expand(st, (uint32_t) from, buf, 200);
        state_from_host(st, 0x00E38E74u, saved);
        text_draw_lines_host(st, buf, S32(st, PANEL_X) + 0x28, S32(st, PANEL_Y) + 0x154, 0x220, S32(st, FE_FONT_LARGE), 0);
    }
    GAME_CALL(st, 0x004CCB30u, 0xD, 1, S32(st, PANEL_X) + 0x10, S32(st, PANEL_Y) + 0x18C, 0x200, S32(st, FE_FONT_LARGE), 0);
}

void mission_end_draw(game_state *st)
{
    /* @bin 0x004194C0 lost: the panel (34 x 16 at y - 7), the title (group 0x3E: 1, or 0x26 for case
     * 3) and the text (0x10 or 0x27) in lines; 0x00E20774 := 0 before it */
    S32(st, PANEL_X) = S32(st, FE_DX) + 0x30;
    S32(st, PANEL_Y) = S32(st, FE_DY) + 0x10;
    int32_t w = S32(st, FE_SCREEN_W), h = S32(st, FE_SCREEN_H);
    S32(st, 0x00D3B56Cu) = (0x400 - w) / 2;
    S32(st, 0x00D3B574u) = w;
    S32(st, 0x00D3B570u) = h;
    S32(st, 0x00D3B568u) = (0x300 - h) / 2;
    if (U8(st, 0x00E38DD9u) == 1) {
        mission_won_draw(st);
        return;
    }
    int32_t k = mission_end_kind(st);
    GAME_CALL(st, 0x00536820u, S32(st, PANEL_X), S32(st, PANEL_Y) - 7, 0x22, 0x10);
    GAME_CALL(st, 0x004CCB30u, 0x3E, k >= 3 ? 0x26 : 1, S32(st, PANEL_X), S32(st, PANEL_Y) + 0x10, 0x220,
              S32(st, 0x00F56FB4u), 0);
    S32(st, 0x00E20774u) = 0;
    GAME_CALL(st, 0x004CCFD0u, 0x3E, k >= 3 ? 0x27 : 0x10, S32(st, PANEL_X) + 0x19, S32(st, PANEL_Y) + 0x30, 0x200,
              S32(st, FE_FONT_LARGE), 0);
}

static void button(game_state *st, int32_t hover, int32_t x, int32_t y, int32_t width, int32_t label, int32_t text_width,
                   int32_t font)
{
    int32_t lit = S32(st, FE_HOVER) == hover;
    U8(st, LIT) = (uint8_t) lit;
    fe_button_draw(st, 6, x, y, width, lit);
    GAME_CALL(st, 0x004CCB30u, 0x3E, label, x, y + 6, text_width, font, 0);
}

void mission_end_frame(game_state *st)
{
    /* @bin 0x00419DB0 the buttons (0x00536710 kind 6, lit by the hover 0x007056C8 = n, kept in
     * 0x00E2079A) and their labels (group 0x3E): won: continue (3) and, past the first two missions
     * of a campaign or in a custom one, 4 and 5; lost: by 0x004EA880 */
    int32_t font = S32(st, FE_FONT_LARGE);
    int32_t x = S32(st, FE_DX) + 0x30;
    int32_t y = S32(st, FE_DY) + 0x80;
    S32(st, PANEL_X) = x;
    S32(st, PANEL_Y) = y;
    if (U8(st, 0x00E38DD9u) == 1) {
        button(st, 1, x + 0x20, y + 0x70, 0x1E, 3, 0x1E0, font);
        if ((int8_t) S8(st, 0x00E38EA8u) >= 2 || S32(st, 0x00E38EBCu) != 0 || U8(st, 0x00E38EA9u) != 0) {
            button(st, 2, S32(st, PANEL_X) + 0x20, S32(st, PANEL_Y) + 0x90, 0x1E, 4, 0x1E0, font);
            button(st, 3, S32(st, PANEL_X) + 0x20, S32(st, PANEL_Y) + 0xB0, 0x1E, 5, 0x1E0, font);
        }
        return;
    }
    if (mission_end_kind(st) == 1) {
        button(st, 1, x + 0x40, y + 0x60, 0x1A, 6, 0x1A0, S32(st, FE_FONT_LARGE));
        return;
    }
    if (mission_end_kind(st) == 2) {
        button(st, 1, S32(st, PANEL_X) + 0x40, S32(st, PANEL_Y) + 0x60, 9, 6, 0x90, S32(st, FE_FONT_LARGE));
        button(st, 2, S32(st, PANEL_X) + 0x160, S32(st, PANEL_Y) + 0x60, 9, 0x25, 0x90, S32(st, FE_FONT_LARGE));
        return;
    }
    button(st, 1, S32(st, PANEL_X) + 0x20, S32(st, PANEL_Y) + 0x60, 9, 0x28, 0x90, S32(st, FE_FONT_LARGE));
    button(st, 2, S32(st, PANEL_X) + 0xD0, S32(st, PANEL_Y) + 0x60, 9, 6, 0x90, S32(st, FE_FONT_LARGE));
    button(st, 3, S32(st, PANEL_X) + 0x180, S32(st, PANEL_Y) + 0x60, 9, 0x25, 0x90, S32(st, FE_FONT_LARGE));
}

static int32_t run_kind(game_state *st, const int32_t *a) { (void) a; return mission_end_kind(st); }
static int32_t run_buttons(game_state *st, const int32_t *a) { (void) a; return (int32_t) mission_end_buttons(st); }
static int32_t run_draw(game_state *st, const int32_t *a) { (void) a; mission_end_draw(st); return 0; }
static int32_t run_frame(game_state *st, const int32_t *a) { (void) a; mission_end_frame(st); return 0; }

const ported_function frontend_victory_ports[] = {
    {0x004EA880u, "mission_end_kind", 0, 32, 0, run_kind, 0},
    {0x004EA8D0u, "mission_end_buttons", 0, 32, 0, run_buttons, 0},
    {0x004194C0u, "mission_end_draw", 0, 0, 0, run_draw, 0},
    {0x00419DB0u, "mission_end_frame", 0, 0, 0, run_frame, 0},
};
const int frontend_victory_ports_count = sizeof(frontend_victory_ports) / sizeof(frontend_victory_ports[0]);
