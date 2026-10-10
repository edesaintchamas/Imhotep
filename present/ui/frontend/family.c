/* Imhotep - the front end: the family list (window 0x2F), the new family's name (0x36) and the
 * family's menu (0x30) (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The family list object 0x00EADFA8 (core/newgame/flow2.c: +4 the rows' offsets 0x00EADFAC, +8 the
 * count 0x00EADFB0, +0x10 the names' base 0x00EADFB8; the first row shown 0x00EADFC8, the chosen row
 * 0x00EADFCC). Window 0x2F: the full redraw 0x00516630, per frame the arrows (0x00425730 on
 * 0x005F7EE0, 0x005165B0 of them) and the rows 0x00516720; input (0x004D8310): the scroll bar drag
 * 0x005168E0, the arrows (0x00425990 on 0x005F7EE0: 0x00516890), the rows and buttons (0x004261D0 on
 * 0x005F7F28: rows 0x00516AB0, 0x00516C10 new, 0x00516C60 delete, 0x00516990 done, 0x00402BB0 back),
 * a right click 0x00402BB0. Window 0x36: the full redraw 0x00520B60, per frame 0x00520B90 (the name
 * field 0x00E38E74). Window 0x30: the full redraw 0x004CB4D0, per frame 0x004CB5A0 (table 0x005E0408:
 * 0x004CB800, 0x004CB860, 0x004CB910, 0x004CB940, 0x004CB970). */
#include "frontend.h"

#include <string.h>

#include "draw/text.h"
#include "present_layout.h"
#include "ui/messages/messages.h"
#include "newgame/flow.h"
#include "newgame/flow2.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"


static void image(game_state *st, int32_t id, int32_t x, int32_t y)
{
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, id, x, y);
}

int32_t fe_text_expand(game_state *st, uint32_t text, char *out, int32_t size)
{
    /* @bin 0x00448370 out[0] := 0, then the text with its "[keywords]" expanded (0x004CB980 on the
     * keyword table 0x00784F34) into out[0..size) */
    out[0] = 0;
    return text_expand(st, G_KEYWORDS_THIS, text, out, size);
}

int32_t fe_two(game_state *st)
{
    /* @bin 0x005165B0 the number of arrows of the family list (and other lists): 2 */
    (void) st;
    return 2;
}

static int32_t scroll_percent(game_state *st, int32_t first, int32_t count)
{
    /* the thumb's place: 0 at the top, 100 at the end, else first * 100 / (count - 12) */
    if (first <= 0) {
        return 0;
    }
    if (first + 12 >= count) {
        return 100;
    }
    return GAME_CALL(st, FN_CALC_RATIO_PERCENT, first, count - 12);
}

void family_scroll_thumb(game_state *st)
{
    /* @bin 0x005165C0 the scroll bar's thumb (image 0x27 of the buttons) when more than 12 rows:
     * 0x91 + 0x82 * percent / 100 down */
    int32_t count = S32(st, G_FAMILY_COUNT);
    if (count - 12 <= 0) {
        return;
    }
    int32_t pct = scroll_percent(st, S32(st, G_FAMILY_SCROLL), count);
    int32_t dy = GAME_CALL(st, FN_CALC_PERCENTAGE, 0x82, pct);
    image(st, S32(st, G_SCROLL_IMAGES) + 0x27, S32(st, G_LAYOUT_DX) + 0x1D8, S32(st, G_LAYOUT_DY) + dy + 0x91);
}

void family_rows_draw(game_state *st)
{
    /* @bin 0x00516720 12 rows (rectangles 0x005F7F28..0x005F805C): the name of row first + k - 1 (lit
     * font when hovered, 0x007056C8 = k, or chosen); then the four buttons (frames 0x004CD1A0 lit by
     * the hover 0x0D..0x10, labels 0, 1, 2, 4 of group 0x124 in the large font) */
    int32_t k = 1;
    for (uint32_t e = G_FAMILY_RECTS; e <= G_FAMILY_RECTS + 11u * 0x1Cu; e += 0x1C, k++) {
        int32_t row = S32(st, G_FAMILY_SCROLL) + k - 1;
        if (row < 0 || row >= S32(st, G_FAMILY_COUNT)) {
            continue;
        }
        uint32_t text = P32(st, U32(st, G_FAMILY_ROWS) + 4u * (uint32_t) row) + U32(st, G_FAMILY_NAMES);
        if (text == 0) {
            continue;
        }
        int32_t font = S32(st, G_FONT_9C);
        if (S32(st, G_HOVER) != k && row != S32(st, G_FAMILY_CHOSEN)) {
            font = S32(st, G_FONT_ROW);
        }
        GAME_CALL(st, FN_LAYOUT_TEXT, (int32_t) text, (int32_t) e, font, 0, 0, 0);
    }
    int32_t font = S32(st, G_FONT_LARGE);
    static const struct {
        uint32_t rect;
        int32_t hover, label;
    } buttons[4] = {{G_FAMILY_RECTS + 12u * 0x1Cu, 0x0D, 0}, {G_FAMILY_RECTS + 13u * 0x1Cu, 0x0E, 1},
                       {G_FAMILY_RECTS + 14u * 0x1Cu, 0x0F, 2}, {G_FAMILY_RECTS + 15u * 0x1Cu, 0x10, 4}};
    for (int i = 0; i < 4; i++) {
        GAME_CALL(st, FN_LAYOUT_PANEL, 1, (int32_t) buttons[i].rect, S32(st, G_HOVER) == buttons[i].hover, 0, 0);
        GAME_CALL(st, FN_LAYOUT_TEXT_GROUP_CENTRED, 0x124, buttons[i].label, (int32_t) buttons[i].rect, font, 0, 4, 7);
    }
}

void family_list_draw(game_state *st)
{
    /* @bin 0x00516630 window 0x2F: on a full redraw the screen cleared and the picture 0x00F55C9C;
     * the panel (24 x 23 pieces), the list's inner panel (20 x 13), the arrows, the title (group
     * 0x124, 3), the rows, the scroll bar's inner panel (2 x 10) and its thumb */
    int32_t dx, dy;
    if (U8(st, G_REDRAW) == 1) {
        GAME_CALL0(st, FN_SCREEN_BUFFER_CLEAR);
        image(st, S32(st, G_FAMILY_LIST_IMAGE), S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    }
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    U8(st, G_REDRAW_SIDEBAR) = 1;
    GAME_CALL(st, FN_PANEL_DRAW, dx + 0x80, dy + 0x28, 0x18, 0x17);
    GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_LAYOUT_DX) + 0x90, S32(st, G_LAYOUT_DY) + 0x78, 0x14, 0x0D);
    int32_t n = fe_two(st);
    GAME_CALL(st, FN_BUTTONS_DRAW, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY), G_FAMILY_LIST_BUTTONS, n);
    GAME_CALL(st, FN_TEXT_GROUP_DRAW_CENTRED, 0x124, 3, S32(st, G_LAYOUT_DX) + 0x80, S32(st, G_LAYOUT_DY) + 0x3A, 0x180, S32(st, G_FONT_TITLE), 0);
    family_rows_draw(st);
    GAME_CALL(st, FN_PANEL_DRAW_INNER, S32(st, G_LAYOUT_DX) + 0x1D3, S32(st, G_LAYOUT_DY) + 0x92, 2, 10);
    family_scroll_thumb(st);
}

int32_t family_scroll_drag(game_state *st)
{
    /* @bin 0x005168E0 the left button held on the scroll bar (x 0x1D0..0x1F0, y 0x91..0x12C of the
     * layout) of a list of more than 12: the first row := (count - 12) * percent / 100 of the
     * place; a partial redraw (2). 1 when it took the input */
    int32_t count = S32(st, G_FAMILY_COUNT);
    if (count <= 12 || U8(st, G_LEFT_HELD) == 0) {
        return 0;
    }
    int32_t range = count - 12;
    int32_t mx = S32(st, G_MOUSE_X), dx = S32(st, G_LAYOUT_DX);
    if (mx < dx + 0x1D0 || mx > dx + 0x1F0) {
        return 0;
    }
    int32_t my = S32(st, G_MOUSE_Y), dy = S32(st, G_LAYOUT_DY);
    if (my < dy + 0x91 || my > dy + 0x12C) {
        return 0;
    }
    int32_t pos = my - dy - 0x91;
    if (pos > 0x82) {
        pos = 0x82;
    }
    int32_t pct = GAME_CALL(st, FN_CALC_RATIO_PERCENT, pos, 0x82);
    S32(st, G_FAMILY_SCROLL) = GAME_CALL(st, FN_CALC_PERCENTAGE, range, pct);
    U8(st, G_REDRAW) = 2;
    return 1;
}

void family_scroll_click(game_state *st)
{
    /* @bin 0x00516890 the arrows (0x00D3B198: 0 up, 1 down) of a list of more than 12 */
    int32_t count = S32(st, G_FAMILY_COUNT);
    if (count <= 12) {
        return;
    }
    int32_t p = S32(st, G_PARAM1);
    if (p == 0) {
        if (S32(st, G_FAMILY_SCROLL) > 0) {
            U8(st, G_REDRAW) = 2;
            S32(st, G_FAMILY_SCROLL) = S32(st, G_FAMILY_SCROLL) - 1;
        }
        return;
    }
    if (p == 1) {
        if (S32(st, G_FAMILY_SCROLL) + 12 >= count) {
            return;
        }
        S32(st, G_FAMILY_SCROLL) = S32(st, G_FAMILY_SCROLL) + 1;
    }
    U8(st, G_REDRAW) = 2;
}

void fe_back_to_menu(game_state *st)
{
    /* @bin 0x00402BB0 back to the main menu: window 0, a full redraw */
    U8(st, G_WINDOW) = 0;
    U8(st, G_REDRAW) = 1;
}

void family_delete_click(game_state *st)
{
    /* @bin 0x00516C60 the yes / no popup (0x004264D0) of message 0x5A, its answer to 0x00516DD0 (the
     * family's files deleted) */
    GAME_CALL(st, FN_POPUP_CONFIRM_OPEN, 0x5A, FN_FAMILY_DELETE, 2);
}

static void state_cstr(game_state *st, char *dst, uint32_t src, size_t size)
{
    size_t i = 0;
    for (; i + 1 < size && P8(st, src + (uint32_t) i) != 0; i++) {
        dst[i] = (char) P8(st, src + (uint32_t) i);
    }
    dst[i] = 0;
}

void family_delete(game_state *st)
{
    /* @bin 0x00516DD0 the popup's answer 0x00D3B439 = 1 (yes): the progress file (0x00516A20) removed
     * (0x00562BDF); a file list on the stack (0x0041C6D0) filled from the family's directory
     * (0x005169C0) + "\*.sav" (0x005761C4); each file (directory + "\" (0x005761D0) + its name)
     * removed; the directory removed (RemoveDirectoryA: the platform's file_delete on the directory,
     * its result unused); the list destroyed (0x0041C710). Then, whatever the answer, the family list
     * (0x00516BB0) and a full redraw */
    if (U8(st, G_CONFIRM_ANSWER) == 1) {
        char path[600], dir[300];
        state_cstr(st, path, campaign_progress_path(st), sizeof(path));
        crt_remove(st, path);
        state_cstr(st, dir, family_dir_path(st), sizeof(dir));
        char pattern[320];
        strcpy(pattern, dir);
        state_cstr(st, pattern + strlen(pattern), G_STR_SAV_PATTERN, sizeof(pattern) - strlen(pattern));
        static char names[1 << 16];
        int32_t n = file_list_names(st, pattern, names, sizeof(names));
        const char *name = names;
        for (int32_t i = 0; i < n; i++) {
            if (name >= names + sizeof(names) || memchr(name, 0, (size_t) (names + sizeof(names) - name)) == NULL) {
                state_unported(st, FN_FAMILY_DELETE); /* more names than the buffer holds */
                return;
            }
            state_cstr(st, dir, family_dir_path(st), sizeof(dir));
            strcpy(path, dir);
            state_cstr(st, path + strlen(path), G_STR_BACKSLASH, sizeof(path) - strlen(path));
            size_t l = strlen(path); /* strncat(path, name, sizeof(path) - l - 1) */
            for (const char *c = name; *c && l + 1 < sizeof(path); c++) {
                path[l++] = *c;
            }
            path[l] = 0;
            crt_remove(st, path);
            name += strlen(name) + 1;
        }
        state_cstr(st, dir, family_dir_path(st), sizeof(dir));
        platform_file_delete(st, dir);
    }
    family_list_open(st);
    U8(st, G_REDRAW) = 1;
}

/* ---- the new family's name (window 0x36) ---- */

void name_entry_draw(game_state *st)
{
    /* @bin 0x00520B60 window 0x36's full redraw: the screen cleared, the main menu's picture;
     * 0x00F53F28 := 100 */
    GAME_CALL0(st, FN_SCREEN_BUFFER_CLEAR);
    image(st, S32(st, G_BACKGROUND), S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    S32(st, G_UF53F28) = 100;
}

void text_caret_draw(game_state *st, int32_t x, int32_t y, int32_t colour)
{
    /* @bin 0x00537140 the edit field's caret (at 0x00E20790, 0x00E20794 from (x, y)), blinking on
     * the frame clock: hidden 200 ms, shown 400 ms; an underline (a 2-pixel bar of the character's
     * width 0x00E2078C), or in insert mode (0x00E2079B) an I-beam of three lines */
    int32_t now = S32(st, G_FRAME_CLOCK);
    int32_t phase = S32(st, G_CARET_PHASE);
    int32_t since = now - S32(st, G_CARET_TIME);
    if (phase == 0) {
        if (since < 200) {
            return;
        }
        S32(st, G_CARET_TIME) = now;
        S32(st, G_CARET_PHASE) = 1;
    } else if (phase == 1 && since >= 400) {
        S32(st, G_CARET_TIME) = now;
        S32(st, G_CARET_PHASE) = 0;
        return;
    }
    if (U8(st, G_INSERT) == 0) {
        GAME_CALL(st, FN_RECT_FILL, S32(st, G_CARET_X) + x, S32(st, G_CARET_Y) + y + 0x0E, S32(st, G_CARET_W), 2,
                  colour);
        return;
    }
    int32_t cy = S32(st, G_CARET_Y) + y - 3;
    int32_t cx = S32(st, G_CARET_X);
    GAME_CALL(st, FN_LINE_DRAW, cx + x - 3, cy, cx + x + 1, cy, colour);
    cx = S32(st, G_CARET_X);
    int32_t cy2 = S32(st, G_CARET_Y);
    GAME_CALL(st, FN_LINE_DRAW, cx + x - 1, cy2 + y - 3, cx + x - 1, cy2 + y + 0x0D, colour);
    cy = S32(st, G_CARET_Y) + y + 0x0E;
    cx = S32(st, G_CARET_X);
    GAME_CALL(st, FN_LINE_DRAW, cx + x - 3, cy, cx + x + 1, cy, colour);
}

void name_entry_frame(game_state *st)
{
    /* @bin 0x00520B90 the name panel (24 x 8), its title (group 0x1F, 0), the label (group 0xD, 5),
     * the OK frame (25 x 25, lit when hovered) and its text (group 0xD, 8, red 0xF800 when lit), the
     * field's inner panel (20 x 2), the typed name and the caret */
    int32_t dx = S32(st, G_LAYOUT_DX), dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_PANEL_DRAW, dx + 0x80, dy + 0xA0, 0x18, 8);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x1F, 0, dx + 0x80, dy + 0xAC, 0x180, S32(st, G_FONT_TITLE), 0, dx, dy);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_GROUP, 0x0D, 5, dx + 0x18B, dy + 0xFF, S32(st, G_FONT_LARGE), 0, dx, dy);
    int32_t lit = S32(st, G_HOVER) == 1;
    int32_t colour = lit ? 0xF800 : 0;
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_FRAME, 0, dx + 0x160, dy + 0xF6, 0x19, 0x19, lit, dx, dy);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x0D, 8, dx + 0x91, dy + 0x100, 0xB4, S32(st, G_FONT_LARGE), colour, dx, dy);
    dx = S32(st, G_LAYOUT_DX);
    dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_INNER_PANEL, dx + 0xA0, dy + 0xD0, 0x14, 2, dx, dy);
    GAME_CALL0(st, FN_TEXT_RUN_RESET);
    GAME_CALL(st, FN_TEXT_DRAW, (int32_t) G_PLAYER_NAME, S32(st, G_LAYOUT_DX) + 0xB0, S32(st, G_LAYOUT_DY) + 0xD8, S32(st, G_FONT_9C), 0);
    text_caret_draw(st, S32(st, G_LAYOUT_DX) + 0xB0, S32(st, G_LAYOUT_DY) + 0xD9, 0xFFFF);
}

void name_entry_back(game_state *st)
{
    /* @bin 0x0040C890 window 0x51, a full redraw */
    U8(st, G_WINDOW) = 0x51;
    U8(st, G_REDRAW) = 1;
}

/* ---- the family's menu (window 0x30) ---- */

static void music_setup(game_state *st)
{
    /* the speech stream stopped (0x0055F561(1)); with sound on (0x00F55B88), no music playing and
     * music on (0x00E38E64): "audio\music\setup.mp3" (0x005761AC) on channel 0 */
    GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, 1);
    if (S32(st, G_SOUND_ON) != 0 && GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 0) == 0 && U8(st, G_MUSIC_ON) != 0) {
        GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, G_STR_SETUP_MUSIC, 0, 0);
    }
}

void family_menu_frame(game_state *st)
{
    /* @bin 0x004CB5A0 the music, then the five buttons of 0x005E0408 (labels of group 0x125; the
     * first is "begin" (7) without a saved game 0x00D3B18B, else "continue" (0); 1 shown when
     * 0x00D3B188, 2 when 0x00D3B18A): text, then frame lit by the hover */
    music_setup(st);
    int32_t font = S32(st, G_FONT_LARGE);
    for (int32_t i = 0; i <= 4; i++) {
        if (i == 1 && U8(st, G_MENU_BUTTON1_SHOWN) == 0) {
            continue;
        }
        if (i == 2 && U8(st, G_MENU_SAVES_FOUND) == 0) {
            continue;
        }
        uint32_t rect = G_FAMILY_MENU_BUTTONS + 0x1Cu * (uint32_t) i;
        int32_t lit;
        if (i == 0 && U8(st, G_MENU_LAST_SAVE_FOUND) == 0) {
            lit = S32(st, G_HOVER) == 1;
            GAME_CALL(st, FN_LAYOUT_TEXT_GROUP_CENTRED, 0x125, 7, (int32_t) rect, font, 0, 4, 7);
        } else {
            lit = S32(st, G_HOVER) == i + 1;
            GAME_CALL(st, FN_LAYOUT_TEXT_GROUP_CENTRED, 0x125, i, (int32_t) rect, font, 0, 4, 7);
        }
        GAME_CALL(st, FN_LAYOUT_PANEL, 0, (int32_t) rect, lit, 0, 0);
    }
}

void family_menu_draw(game_state *st)
{
    /* @bin 0x004CB4D0 window 0x30: on a full redraw the screen cleared and the picture 0x00F55CAC;
     * the panel (24 x 21), the title (group 0x125, 5, expanded with 0x00784F0C := 0 for the while)
     * centred, then the buttons */
    if (U8(st, G_REDRAW) == 1) {
        GAME_CALL0(st, FN_SCREEN_BUFFER_CLEAR);
        image(st, S32(st, G_FAMILY_MENU_IMAGE), S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    }
    GAME_CALL(st, FN_PANEL_DRAW, S32(st, G_LAYOUT_DX) + 0x80, S32(st, G_LAYOUT_DY) + 0x28, 0x18, 0x15);
    GAME_CALL(st, FN_TEXT_SEEK, 0x125, 5);
    uint32_t text = U32(st, G_TEXT_CURSOR);
    uint32_t saved = U32(st, G_U784F0C);
    char buf[201];
    memset(buf, 0, sizeof(buf));
    U32(st, G_U784F0C) = 0;
    fe_text_expand(st, text, buf, 200);
    U32(st, G_U784F0C) = saved;
    text_draw_centred_host(st, buf, S32(st, G_LAYOUT_DX) + 0x8C, S32(st, G_LAYOUT_DY) + 0x3C, 0x170, S32(st, G_FONT_TITLE), 0);
    family_menu_frame(st);
}

int32_t campaign_any_won(game_state *st, uint32_t self)
{
    /* @bin 0x0041BA80 (thiscall on the campaign table 0x006FDD08) 1 when a step's won flag +0x3C is
     * set, steps of 0x40 bytes, count +0x1900 */
    int32_t n = S32(st, self + 0x1900);
    for (int32_t i = 0; i < n; i++) {
        if (U8(st, self + 0x3C + 0x40u * (uint32_t) i) != 0) {
            return 1;
        }
    }
    return 0;
}

void family_continue_click(game_state *st)
{
    /* @bin 0x004CB800 the first button: the music stopped; with the last game 0x00D3B18B its save
     * 0x00700998 loaded (0x004D5100), the city; else the Pharaoh campaign's period choice (0x17) */
    GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, 0);
    if (U8(st, G_MENU_LAST_SAVE_FOUND) != 0) {
        S32(st, G_EDITOR) = 0;
        U8(st, G_NO_GROUND) = 0;
        GAME_CALL(st, FN_LOADER_LOAD_SAVE, G_LAST_SAVE);
        U8(st, G_WINDOW) = 1;
        U8(st, G_REDRAW) = 1;
        return;
    }
    S32(st, G_CAMPAIGN_PERIOD) = 0;
    S32(st, G_PERIOD_REACHED) = 0;
    S32(st, G_CAMPAIGN_CAMPAIGN_KIND) = 0;
    U8(st, G_WINDOW) = 0x17;
    U8(st, G_REDRAW) = 1;
}

void family_cleopatra_click(game_state *st)
{
    /* @bin 0x004CB860 the second button (shown with 0x00D3B188): when no mission is won yet a message
     * box (strings 0x8D, 0x8E of group 5, kind 4; 7 = no: nothing); then the Cleopatra periods (0x17,
     * 0x006FDC98 := 1) when 0x00D3B189, else the mission list (0x0041F870, list mode 1, kind 2) */
    if (U8(st, G_MENU_BUTTON1_SHOWN) == 0) {
        return;
    }
    if (!campaign_any_won(st, G_CAMPAIGN_TABLE)) {
        GAME_CALL(st, FN_TEXT_SEEK, 5, 0x8D);
        GAME_CALL(st, FN_TEXT_SEEK2, 5, 0x8E);
        if (GAME_CALL(st, FN_PLATFORM_MESSAGE_BOX, S32(st, G_TEXT_CURSOR), S32(st, G_TEXT_PTR2), 4) == 7) {
            return;
        }
    }
    if (U8(st, G_MENU_PERIOD_DONE) != 0) {
        U8(st, G_WINDOW) = 0x17;
        S32(st, G_CAMPAIGN_CAMPAIGN_KIND) = 1;
        U8(st, G_REDRAW) = 1;
        return;
    }
    S32(st, G_EDITOR) = 0;
    U8(st, G_NO_GROUND) = 0;
    S32(st, G_SCENARIO_KIND) = 2;
    S32(st, G_SCENARIO_LIST_MODE) = 1;
    GAME_CALL0(st, FN_LIST_OPEN);
    U8(st, G_REDRAW) = 1;
}

void family_load_click(game_state *st)
{
    /* @bin 0x004CB910 the third button (shown with 0x00D3B18A): the load dialog 0x00402450, back to
     * window 0x30 (0x00D3B1BC) */
    if (U8(st, G_MENU_SAVES_FOUND) == 0) {
        return;
    }
    S32(st, G_EDITOR) = 0;
    U8(st, G_NO_GROUND) = 0;
    GAME_CALL0(st, FN_LOAD_DIALOG_OPEN);
    S32(st, G_BACK_WINDOW) = 0x30;
}

void family_custom_click(game_state *st)
{
    /* @bin 0x004CB940 the fourth button: the custom scenarios (kind 2, list mode 0, 3), 0x0041F870 */
    S32(st, G_CUSTOM) = 2;
    S32(st, G_EDITOR) = 0;
    U8(st, G_NO_GROUND) = 0;
    S32(st, G_SCENARIO_LIST_MODE) = 0;
    S32(st, G_SCENARIO_KIND) = 3;
    GAME_CALL0(st, FN_LIST_OPEN);
}

void family_change_click(game_state *st)
{
    /* @bin 0x004CB970 the fifth button: jmp 0x00516BB0, the family list */
    GAME_CALL0(st, FN_FAMILY_LIST_OPEN);
}

static int32_t run_expand(game_state *st, const int32_t *a)
{
    /* the output is the caller's buffer (a state address): its bytes are taken, expanded into and
     * put back, so that the bytes the original does not write keep their values */
    static char buf[0x10000];
    int32_t size = a[2];
    uint32_t out = (uint32_t) a[1];
    if (size <= 0 || size > (int32_t) sizeof(buf)) {
        state_unported(st, FN_FE_TEXT_EXPAND);
        return 0;
    }
    for (int32_t i = 0; i < size; i++) {
        buf[i] = (char) P8(st, out + (uint32_t) i);
    }
    int32_t r = fe_text_expand(st, (uint32_t) a[0], buf, size);
    for (int32_t i = 0; i < size; i++) {
        P8(st, out + (uint32_t) i) = (uint8_t) buf[i];
    }
    return r;
}
static int32_t run_two(game_state *st, const int32_t *a) { (void) a; return fe_two(st); }
static int32_t run_thumb(game_state *st, const int32_t *a) { (void) a; family_scroll_thumb(st); return 0; }
static int32_t run_rows(game_state *st, const int32_t *a) { (void) a; family_rows_draw(st); return 0; }
static int32_t run_list(game_state *st, const int32_t *a) { (void) a; family_list_draw(st); return 0; }
static int32_t run_drag(game_state *st, const int32_t *a) { (void) a; return family_scroll_drag(st); }
static int32_t run_arrows(game_state *st, const int32_t *a) { (void) a; family_scroll_click(st); return 0; }
static int32_t run_back(game_state *st, const int32_t *a) { (void) a; fe_back_to_menu(st); return 0; }
static int32_t run_delete(game_state *st, const int32_t *a) { (void) a; family_delete_click(st); return 0; }
static int32_t run_family_delete(game_state *st, const int32_t *a) { (void) a; family_delete(st); return 0; }
static int32_t run_name_draw(game_state *st, const int32_t *a) { (void) a; name_entry_draw(st); return 0; }
static int32_t run_caret(game_state *st, const int32_t *a) { text_caret_draw(st, a[0], a[1], a[2]); return 0; }
static int32_t run_name_frame(game_state *st, const int32_t *a) { (void) a; name_entry_frame(st); return 0; }
static int32_t run_name_back(game_state *st, const int32_t *a) { (void) a; name_entry_back(st); return 0; }
static int32_t run_menu_frame(game_state *st, const int32_t *a) { (void) a; family_menu_frame(st); return 0; }
static int32_t run_menu_draw(game_state *st, const int32_t *a) { (void) a; family_menu_draw(st); return 0; }
static int32_t run_any_won(game_state *st, const int32_t *a) { (void) a; return campaign_any_won(st, G_CAMPAIGN_TABLE); }
static int32_t run_continue(game_state *st, const int32_t *a) { (void) a; family_continue_click(st); return 0; }
static int32_t run_cleo(game_state *st, const int32_t *a) { (void) a; family_cleopatra_click(st); return 0; }
static int32_t run_load(game_state *st, const int32_t *a) { (void) a; family_load_click(st); return 0; }
static int32_t run_custom(game_state *st, const int32_t *a) { (void) a; family_custom_click(st); return 0; }
static int32_t run_change(game_state *st, const int32_t *a) { (void) a; family_change_click(st); return 0; }

const ported_function frontend_family_ports[] = {
    {0x00448370u, "fe_text_expand", 3, 0, 0, run_expand, 0},
    {0x005165B0u, "fe_two", 0, 32, 0, run_two, 0},
    {0x005165C0u, "family_scroll_thumb", 0, 0, 0, run_thumb, 0},
    {0x00516720u, "family_rows_draw", 0, 0, 0, run_rows, 0},
    {0x00516630u, "family_list_draw", 0, 0, 0, run_list, 0},
    {0x005168E0u, "family_scroll_drag", 0, 32, 0, run_drag, 0},
    {0x00516890u, "family_scroll_click", 0, 0, 0, run_arrows, 0},
    {0x00402BB0u, "fe_back_to_menu", 0, 0, 0, run_back, 0},
    {0x00516C60u, "family_delete_click", 0, 0, 0, run_delete, 0},
    {0x00516DD0u, "family_delete", 0, 0, 0, run_family_delete, 0},
    {0x00520B60u, "name_entry_draw", 0, 0, 0, run_name_draw, 0},
    {0x00537140u, "text_caret_draw", 3, 0, 0, run_caret, 0},
    {0x00520B90u, "name_entry_frame", 0, 0, 0, run_name_frame, 0},
    {0x0040C890u, "name_entry_back", 0, 0, 0, run_name_back, 0},
    {0x004CB5A0u, "family_menu_frame", 0, 0, 0, run_menu_frame, 0},
    {0x004CB4D0u, "family_menu_draw", 0, 0, 0, run_menu_draw, 0},
    {0x0041BA80u, "campaign_any_won", 0, 8, G_CAMPAIGN_TABLE, run_any_won, 0},
    {0x004CB800u, "family_continue_click", 0, 0, 0, run_continue, 0},
    {0x004CB860u, "family_cleopatra_click", 0, 0, 0, run_cleo, 0},
    {0x004CB910u, "family_load_click", 0, 0, 0, run_load, 0},
    {0x004CB940u, "family_custom_click", 0, 0, 0, run_custom, 0},
    {0x004CB970u, "family_change_click", 0, 0, 0, run_change, 0},
};
const int frontend_family_ports_count = sizeof(frontend_family_ports) / sizeof(frontend_family_ports[0]);
