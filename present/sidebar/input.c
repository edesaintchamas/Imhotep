/* Imhotep - the sidebar's input: button tables, menu bar, minimap clicks (roadmap phase 7.5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The window input dispatch 0x004D8310 (city view, window 1) runs, in order: the menu bar
 * (0x00426100 / 0x00426030 on 0x005C91F0), the scrolling, the minimap (0x00406190, 0x004061E0), the
 * menu bar's right button (0x00425840), the top bar's figures (0x00406250, right click 0x004072C0),
 * then per sidebar button table its hover (0x00425BC0) and its click (0x00425990, or 0x004261D0 for
 * the rectangle table 0x005C6CB8): expanded 0x005C7300 (2, hover ids from 10), 0x005C7370 (16,
 * from 0x1E) with 0x005C6CB8 (1), 0x005C7980 (2, from 0x32); collapsed 0x005C7348 (1, from 10)
 * and 0x005C75B0 (12, from 0x1E). A click calls the entry's function with its parameters in
 * 0x00D3B198 / 0x00D3B194: the button callbacks below, which lead to the phase-5 commands
 * (build menu 0x00403D70 -> choose building 0x00405200, undo 0x00406010, ...).
 * Mouse globals: the cursor 0x00E38E34 / 0x00E38E30 (copied to 0x00E38E2C / 0x00E38E28 on a hit),
 * left pressed 0x00E38E03, held 0x00E38E00, released 0x00E38E04, the click latch 0x00E38E02, right
 * pressed 0x00E38DF7, right released 0x00E38DF8 (docs/presentation.md 5.2). */
#include "sidebar.h"

#include "present_layout.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"
#include "state/records.h"

static void callback(game_state *st, uint32_t pointer)
{
    /* the entry's function, called through its pointer (a button callback) */
    GAME_CALL0(st, U32(st, pointer));
}

void buttons_hover(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count, int32_t base)
{
    /* @bin 0x00425BC0 the hovered button of a table: 0x007056B8 := base + its index (the hit
     * position kept in 0x00E38E2C / 0x00E38E28); entries of 0x24: +0 x, +2 y, +4 w, +6 h (i16) */
    if (count <= 0) {
        return;
    }
    int32_t mx = S32(st, G_MOUSE_X), my = S32(st, G_MOUSE_Y);
    uint32_t e = table;
    for (int32_t k = 0; k < count; k++, e += 0x24) {
        int32_t bx = P16(st, e) + x;
        int32_t by = P16(st, e + 2) + y;
        if (bx <= mx && P16(st, e + 4) + bx > mx && by <= my && P16(st, e + 6) + by > my) {
            S32(st, G_HIT_X) = mx;
            S32(st, G_HIT_Y) = my;
            S32(st, G_HOVER_BUTTON) = k + base;
            return;
        }
    }
}

static void buttons_release_toggles(game_state *st, uint32_t table, int32_t count)
{
    /* @bin 0x00425B50 clears the pressed state +0x19 of the table's toggle buttons (kind +8 = 2) */
    uint32_t e = table + 0x19;
    for (int32_t k = 0; k < count; k++, e += 0x24) {
        if (P8(st, e) != 0 && P16(st, e - 0x11) == 2) {
            P8(st, e) = 0;
            U8(st, G_REDRAW_SIDEBAR) = 1;
        }
    }
}

static void buttons_count_down(game_state *st, uint32_t table, int32_t count)
{
    /* @bin 0x00425B80 the pressed state +0x19 of push buttons (kinds 4, 6) counts down to 0 */
    uint32_t e = table + 0x19;
    for (int32_t k = 0; k < count; k++, e += 0x24) {
        uint8_t v = P8(st, e);
        if (v != 0) {
            int16_t kind = P16(st, e - 0x11);
            if (kind == 4 || kind == 6) {
                v--;
                P8(st, e) = v;
                if (v == 0) {
                    P8(st, e) = 0;
                    U8(st, G_REDRAW_SIDEBAR) = 1;
                }
            }
        }
    }
}

int32_t buttons_click(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    /* @bin 0x00425990 a button table's input: pressed states count down (0x00425B80), the hover
     * counter +0x1A counts down and is set to 2 on the enabled (+0x18) button under the mouse;
     * then, on a left press or right release (kinds 2, 4) or a left press or hold (kind 6) -- the
     * kind read past the table's last entry -- the hovered button's function: left +0x10 (marking
     * it pressed, +0x19 = 0x14), right +0x14; toggles (kind 2) release the others first. Returns
     * its index + 1, else whether a hover changed */
    buttons_count_down(st, table, count);
    int32_t changed = 0;
    uint32_t e = table;
    if (count > 0) {
        for (int32_t k = count; k != 0; k--, e += 0x24) {
            uint8_t hover = P8(st, e + 0x1A);
            if (hover != 0) {
                hover--;
                P8(st, e + 0x1A) = hover;
                if (hover == 0) {
                    changed = 1;
                }
            }
            if (P8(st, e + 0x18) == 0) {
                continue;
            }
            int32_t bx = P16(st, e) + x;
            int32_t by = P16(st, e + 2) + y;
            if (bx > S32(st, G_MOUSE_X) || P16(st, e + 4) + bx <= S32(st, G_MOUSE_X) || by > S32(st, G_MOUSE_Y) ||
                P16(st, e + 6) + by <= S32(st, G_MOUSE_Y)) {
                continue;
            }
            S32(st, G_HIT_Y) = S32(st, G_MOUSE_Y);
            S32(st, G_HIT_X) = S32(st, G_MOUSE_X);
            if (P8(st, e + 0x1A) == 0) {
                changed = 1;
            }
            P8(st, e + 0x1A) = 2;
        }
    }
    int16_t kind = P16(st, e + 8);
    uint8_t pressed = U8(st, G_LEFT_PRESSED);
    if (kind == 2 || kind == 4) {
        if (pressed == 0 && U8(st, G_RIGHT_RELEASED) == 0) {
            return changed;
        }
    } else if (kind == 6 && pressed == 0 && U8(st, G_LEFT_HELD) == 0) {
        return changed;
    }
    e = table;
    int32_t k = 0;
    for (;; k++, e += 0x24) {
        if (k >= count) {
            return changed;
        }
        if (P8(st, e + 0x18) != 0 && P8(st, e + 0x1A) != 0) {
            break;
        }
    }
    if (P16(st, e + 8) == 2) {
        buttons_release_toggles(st, table, count);
        pressed = U8(st, G_LEFT_PRESSED);
    }
    S32(st, G_PARAM1) = P32(st, e + 0x1C);
    S32(st, G_PARAM2) = P32(st, e + 0x20);
    if (pressed != 0) {
        GAME_CALL(st, FN_RETURN_ZERO, 3);
        P8(st, e + 0x19) = 0x14;
        P8(st, e + 0x1B) = 0;
        callback(st, e + 0x10);
        return k + 1;
    }
    if (U8(st, G_RIGHT_RELEASED) != 0) {
        callback(st, e + 0x14);
        return k + 1;
    }
    if (P16(st, e + 8) == 6 && U8(st, G_LEFT_HELD) != 0) {
        P8(st, e + 0x19) = 0x14;
        callback(st, e + 0x10);
    }
    return k + 1;
}

int32_t buttons_rect_click(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    /* @bin 0x004261D0 a table of rectangles (entries of 0x1C: +0 x0, +2 y0, +4 x1, +6 y1 (i16), +8
     * left function, +0x0C right function, +0x11 hit, +0x13 kind, +0x14 / +0x18 parameters): the
     * first under the mouse whose kind's buttons are down calls (kind 1 left or right press, 3 on
     * release, 2 press or hold -- with 0x00D3B3AC a hold alone --, 4 left press); returns its index
     * + 1, else 0. The left press and latch are read once, before the scan */
    if (count <= 0) {
        return 0;
    }
    uint8_t pressed = U8(st, G_LEFT_PRESSED);
    uint8_t latch = U8(st, G_LEFT_LATCH);
    uint32_t e = table;
    int32_t k = 0;
    for (;;) {
        int32_t mx = S32(st, G_MOUSE_X);
        int hit = !(P16(st, e) + x > mx || P16(st, e + 4) + x <= mx);
        if (hit) {
            int32_t my = S32(st, G_MOUSE_Y);
            hit = !(P16(st, e + 2) + y > my || P16(st, e + 6) + y <= my);
        }
        int advance = 1;
        if (hit) {
            uint8_t kind = P8(st, e + 0x13);
            if (kind == 1) {
                if (pressed != 0 || U8(st, G_RIGHT_PRESSED) != 0 || latch != 0) {
                    S32(st, G_PARAM1) = P32(st, e + 0x14);
                    P8(st, e + 0x11) = 1;
                    S32(st, G_PARAM2) = P32(st, e + 0x18);
                    if (U8(st, G_LEFT_PRESSED) != 0 || U8(st, G_LEFT_LATCH) != 0) {
                        callback(st, e + 8);
                    } else if (U8(st, G_RIGHT_PRESSED) != 0) {
                        callback(st, e + 0x0C);
                    }
                    return k + 1;
                }
            } else if (kind == 3) {
                if (U8(st, G_LEFT_RELEASED) != 0 || U8(st, G_RIGHT_RELEASED) != 0) {
                    S32(st, G_PARAM1) = P32(st, e + 0x14);
                    P8(st, e + 0x11) = 1;
                    S32(st, G_PARAM2) = P32(st, e + 0x18);
                    if (U8(st, G_LEFT_RELEASED) != 0) {
                        callback(st, e + 8);
                    } else if (U8(st, G_RIGHT_RELEASED) != 0) {
                        callback(st, e + 0x0C);
                    }
                    return k + 1;
                }
            } else if (kind == 2) {
                int call = 0, clear = 0;
                if (U8(st, G_LEFT_HELD) == 0) {
                    if (pressed != 0 || latch != 0) {
                        call = clear = 1;
                    } else if (U8(st, G_RIGHT_HELD) != 0) {
                        goto hold;
                    } else if (U8(st, G_RIGHT_PRESSED) != 0 || U8(st, G_UE38DF6) != 0) {
                        call = clear = 1;
                    }
                } else if (pressed != 0 || latch != 0) {
                    call = clear = 1;
                } else {
                hold:
                    /* @bin 0x004262EC */
                    if (U8(st, G_RIGHT_PRESSED) != 0 || U8(st, G_UE38DF6) != 0) {
                        call = clear = 1;
                    } else if (S32(st, G_UD3B3AC) != 0) {
                        call = 1;
                    } else {
                        advance = 0; /* @bin 0x00426316 the next index on the same entry */
                    }
                }
                if (call) {
                    if (clear) {
                        S32(st, G_UD3B3C8) = 0;
                    }
                    S32(st, G_PARAM1) = P32(st, e + 0x14);
                    P8(st, e + 0x11) = 1;
                    S32(st, G_PARAM2) = P32(st, e + 0x18);
                    if (U8(st, G_LEFT_HELD) != 0) {
                        callback(st, e + 8);
                    } else if (U8(st, G_RIGHT_HELD) != 0) {
                        callback(st, e + 0x0C);
                    }
                    return k + 1;
                }
            } else if (kind == 4) {
                if (pressed != 0 || latch != 0) {
                    S32(st, G_PARAM1) = P32(st, e + 0x14);
                    P8(st, e + 0x11) = 1;
                    S32(st, G_PARAM2) = P32(st, e + 0x18);
                    callback(st, e + 8);
                    return k + 1;
                }
            }
        }
        if (advance) {
            e += 0x1C;
        }
        k++;
        if (k >= count) {
            return 0;
        }
    }
}

int32_t buttons_editor_click(game_state *st, int32_t x, int32_t y, uint32_t table, int32_t count)
{
    /* @bin 0x00425C40 the editor's square buttons (entries of 0x18: +0 x, +2 y, +6 size (i16), +8
     * function, +0x0C on, +0x0D timer, +0x0E repeat, +0x0F kind, +0x10 / +0x14 parameters): timers
     * run down (kind 5 calls when its timer ends), then the button under the mouse: kind 4 calls on
     * a press, and repeats while held (every 30 ms of 0x00705690, the repeat count through the table
     * 0x005C9268, at most 0x2F), kind 2 toggles, kind 5 arms a timer of 10 */
    uint32_t now = platform_clock_ms(st); /* @bin 0x00425C45 timeGetTime */
    int32_t tick;
    if ((int32_t) (now - U32(st, G_BUTTON_REPEAT_CLOCK)) < 0x1E) {
        tick = 0;
    } else {
        tick = 1;
        U32(st, G_BUTTON_REPEAT_CLOCK) = now;
    }
    if (count > 0) {
        uint32_t t = table + 0x0D;
        for (int32_t k = count; k != 0; k--, t += 0x18) {
            uint8_t v = P8(st, t);
            if (v != 0) {
                v--;
                P8(st, t) = v;
                if (v == 0) {
                    P8(st, t - 1) = 1;
                    P8(st, t + 1) = 0;
                    if (P8(st, t + 2) == 5) {
                        S32(st, G_PARAM1) = P32(st, t + 3);
                        S32(st, G_PARAM2) = P32(st, t + 7);
                        callback(st, t - 5);
                    }
                }
            } else {
                P8(st, t + 1) = 0;
            }
            if (P8(st, t + 2) == 3) {
                P8(st, t - 1) = 0;
            }
        }
    }
    uint32_t e = table;
    for (int32_t k = 0; k < count; k++, e += 0x18) {
        int32_t bx = P16(st, e) + x;
        int32_t size = P16(st, e + 6);
        if (bx > S32(st, G_MOUSE_X) || P16(st, e) + size + x <= S32(st, G_MOUSE_X)) {
            continue;
        }
        int32_t by = P16(st, e + 2) + y;
        if (by > S32(st, G_MOUSE_Y) || P16(st, e + 2) + size + y <= S32(st, G_MOUSE_Y)) {
            continue;
        }
        if (S32(st, G_U605A38) > 0 && U8(st, G_WINDOW) == 1 && S32(st, G_EDITOR) == 1) {
            S32(st, G_HOVER_BUTTON) = k + 1;
            if (k > -1) {
                S32(st, G_HOVER_BUTTON) = 0x15;
            }
        }
        uint8_t kind = P8(st, e + 0x0F);
        if (kind == 4) {
            if (U8(st, G_LEFT_PRESSED) != 0 || U8(st, G_LEFT_LATCH) != 0) {
                P8(st, e + 0x0C) = 1;
                P8(st, e + 0x0D) = 3;
                P8(st, e + 0x0E) = 0;
                S32(st, G_PARAM1) = P32(st, e + 0x10);
                S32(st, G_PARAM2) = P32(st, e + 0x14);
                S32(st, G_BUTTON_REPEAT) = 0;
                callback(st, e + 8);
                return k + 1;
            }
            if (U8(st, G_LEFT_HELD) == 0) {
                return 0;
            }
            P8(st, e + 0x0D) = 3;
            if (tick == 0) {
                return 0;
            }
            uint8_t r = (uint8_t) (P8(st, e + 0x0E) + 1);
            P8(st, e + 0x0E) = r;
            if (r >= 0x30) {
                P8(st, e + 0x0E) = 0x2F;
            } else if (r < 8 || U8(st, G_BUTTON_REPEAT_STEPS + r) == 0) {
                return 0;
            }
            S32(st, G_PARAM1) = P32(st, e + 0x10);
            S32(st, G_PARAM2) = P32(st, e + 0x14);
            S32(st, G_BUTTON_REPEAT) = P8(st, e + 0x0E);
            callback(st, e + 8);
            return k + 1;
        }
        if (kind == 2) {
            P8(st, e + 0x0D) = 1;
            if (P8(st, e + 0x0E) == 0) {
                P8(st, e + 0x0C) ^= 1;
                S32(st, G_PARAM1) = P32(st, e + 0x10);
                S32(st, G_PARAM2) = P32(st, e + 0x14);
                callback(st, e + 8);
            }
            return k + 1;
        }
        if (kind == 5) {
            if (U8(st, G_LEFT_PRESSED) == 0 && U8(st, G_LEFT_LATCH) == 0) {
                return 0;
            }
            P8(st, e + 0x0D) = 0x0A;
            P8(st, e + 0x0C) = 1;
            return k + 1;
        }
    }
    return 0;
}

int32_t menubar_hover(game_state *st, uint32_t table, int32_t count)
{
    /* @bin 0x00426100 the menu title under the mouse (entries of 0x10: +0 x0, +2 x1, +4 y; 12 high),
     * 1-based, or 0 */
    int32_t mx = S32(st, G_MOUSE_X), my = S32(st, G_MOUSE_Y);
    uint32_t e = table;
    for (int32_t k = 1; k <= count; k++, e += 0x10) {
        if (P16(st, e) <= mx && P16(st, e + 2) > mx && P16(st, e + 4) <= my && P16(st, e + 4) + 0x0C > my) {
            return k;
        }
    }
    return 0;
}

int32_t menubar_click(game_state *st, int32_t table)
{
    /* @bin 0x00426030 a left press on a menu title (0x007056A8) opens it: window 3, the menu's
     * entries 0x00705724 := table + 0x10 (title - 1), 0x0070572C := 0, the previous window in
     * 0x00705728, top bar redraw */
    if (U8(st, G_LEFT_PRESSED) == 0 || S32(st, G_MENUBAR_HOVER) <= 0) {
        return 0;
    }
    int32_t title = S32(st, G_MENU_TITLE);
    S32(st, G_MENU_HOVER) = 0;
    S32(st, G_MENU_PREV) = U8(st, G_WINDOW);
    U8(st, G_WINDOW) = 3;
    S32(st, G_MENU_OPEN) = (int32_t) ((uint32_t) title * 16u + (uint32_t) table - 0x10u);
    S32(st, G_TOPBAR_REDRAW) = 1;
    U8(st, G_REDRAW) = 1;
    return 1;
}

int32_t menubar_button_click(game_state *st)
{
    /* @bin 0x00425840 the button at the right of the menu bar (0x005C79C8, one entry): a click
     * picks its third under the mouse (0, 1, 2 from the left: 1, 0, 2) as 0x00D3B198 and
     * 0x005C79E4, hover id 0x14, 0x00705698 := that + 1, and redraws it */
    int32_t saved = S32(st, G_HOVER_BUTTON);
    S32(st, G_MENUBAR_BUTTON_HOVER) = 0;
    if (buttons_click(st, 0, 0, G_MENUBAR_BUTTON, 1) == 0) {
        S32(st, G_HOVER_BUTTON) = saved;
        return 0;
    }
    int32_t w = S16(st, G_MENUBAR_BUTTON + BUTTON_W);
    int32_t x = S16(st, G_MENUBAR_BUTTON);
    int32_t mx = S32(st, G_MOUSE_X);
    S32(st, G_HOVER_BUTTON) = 0x14;
    int32_t part;
    if (mx < w / 3 + x) {
        part = 1;
    } else {
        part = mx >= (2 * w) / 3 + x ? 2 : 0;
    }
    S32(st, G_PARAM1) = part;
    S32(st, G_MENUBAR_BUTTON + BUTTON_PART) = part;
    U8(st, G_REDRAW_SIDEBAR) = 1;
    menubar_draw_button(st);
    S32(st, G_MENUBAR_BUTTON_HOVER) = S32(st, G_PARAM1) + 1;
    return 1;
}

void minimap_hover(game_state *st)
{
    /* @bin 0x00406190 0x007056A0 := 1 when the mouse is on the minimap (its rectangle 0x0070E3D4,
     * 0x0070E3D0 .. 0x0070E3CC, 0x0070E3C8, set by its draw) */
    int32_t mx = S32(st, G_MOUSE_X);
    S32(st, G_MINIMAP_HOVER) = 0;
    if (mx < S32(st, G_MINIMAP_LEFT) || mx >= S32(st, G_MINIMAP_RIGHT)) {
        return;
    }
    int32_t my = S32(st, G_MOUSE_Y);
    if (my < S32(st, G_MINIMAP_TOP) || my >= S32(st, G_MINIMAP_BOTTOM)) {
        return;
    }
    S32(st, G_MINIMAP_HOVER) = 1;
}

int32_t minimap_tile_at_mouse(game_state *st, int32_t x, int32_t y, int32_t w, int32_t h)
{
    /* @bin 0x0043EF10 the grid offset under the mouse on a minimap drawn at (x, y, w, h): the same
     * walk as the draw 0x0043E810 (each tile 2 pixels wide on its row, odd rows one pixel left);
     * 0 when none or off the map */
    int odd = 0;
    minimap_first_tile(st, w, h);
    S32(st, G_MINIMAP_TOP) = y;
    int32_t row_y = y - 4;
    S32(st, G_MINIMAP_LEFT) = x;
    int32_t ty = S32(st, G_MINIMAP_FIRST_ROW) - 4;
    S32(st, G_MINIMAP_RIGHT) = x + w * 2;
    S32(st, G_MINIMAP_BOTTOM) = y + h;
    S32(st, G_VIEW_ROW) = ty;
    if (h + 4 <= -4) {
        return 0;
    }
    for (int32_t row = -4; row < h + 4; row++) {
        int32_t px;
        if (odd == 0) {
            px = x - 8;
            odd = 1;
        } else {
            px = x - 9;
            odd = 0;
        }
        int32_t tx = S32(st, G_MINIMAP_FIRST_COLUMN) - 4;
        S32(st, G_VIEW_COLUMN) = tx;
        if (w > -4) {
            for (int32_t col = -4; col < w; col++) {
                if (tx >= 0 && tx < 0xE7 && ty >= 0 && ty < 0x1C9 && S32(st, G_MOUSE_Y) == row_y &&
                    (S32(st, G_MOUSE_X) == px || S32(st, G_MOUSE_X) == px + 1)) {
                    int32_t v = S32(st, G_VIEW_TABLE + 4u * (uint32_t) (ty * 0xE7 + tx));
                    S32(st, G_VIEW_TILE) = v;
                    return v >= 0x0FFF0000 ? 0 : v;
                }
                px += 2;
                tx++;
                S32(st, G_VIEW_COLUMN) = tx;
            }
        }
        row_y++;
        ty++;
        S32(st, G_VIEW_ROW) = ty;
    }
    return 0;
}

int32_t minimap_click(game_state *st)
{
    /* @bin 0x004061E0 a left or right press on the minimap moves the camera to the tile there
     * (0x00430CD0); returns 1 when it did */
    if (S32(st, G_MINIMAP_HOVER) != 1) {
        return 0;
    }
    if (U8(st, G_LEFT_PRESSED) == 0 && U8(st, G_RIGHT_PRESSED) == 0) {
        return 0;
    }
    int32_t tile = minimap_tile_at_mouse(st, S32(st, G_SIDEBAR_X) + 8, S32(st, G_EDITOR) == 0 ? 0x3B : 0x1E, 0x49, 0x6F);
    if (tile <= 0) {
        return 0;
    }
    GAME_CALL(st, FN_CAMERA_GO_TO, tile);
    return 1;
}

void topbar_hover(game_state *st)
{
    /* @bin 0x00406250 the top bar's figure under the mouse (0x0070569C: 1 treasury, 2 population, 3
     * date: 128 pixels from their anchors 0x00F53DC4, 0x00F53DC0, 0x00F53DBC; rows 4..17) */
    int32_t my = S32(st, G_MOUSE_Y);
    S32(st, G_TOPBAR_HOVER) = 0;
    if (my < 4 || my >= 0x12) {
        return;
    }
    int32_t mx = S32(st, G_MOUSE_X);
    int32_t a = S32(st, G_TOPBAR_TREASURY_X);
    if (mx > a && mx < a + 0x80) {
        S32(st, G_TOPBAR_HOVER) = 1;
    }
    a = S32(st, G_TOPBAR_POPULATION_X);
    if (mx > a && mx < a + 0x80) {
        S32(st, G_TOPBAR_HOVER) = 2;
    }
    a = S32(st, G_TOPBAR_DATE_X);
    if (mx > a && mx < a + 0x80) {
        S32(st, G_TOPBAR_HOVER) = 3;
    }
}

void topbar_help(game_state *st)
{
    /* @bin 0x004072C0 right click on a top bar figure: its help message (0x0F, 0x10, 0x11) */
    int32_t v = S32(st, G_TOPBAR_HOVER);
    if (v == 1) {
        GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 0x0F, 0, 0);
    } else if (v == 2) {
        GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 0x10, 0, 0);
    } else if (v == 3) {
        GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, 0x11, 0, 0);
    }
    U8(st, G_REDRAW) = 1;
}

/* ---- the button callbacks of the sidebar tables ---- */

void sidebar_collapse_click(game_state *st)
{
    /* @bin 0x00409D10 (0x005C7300 / 0x005C7348 entry 0) start sliding the sidebar: window 2 */
    S32(st, G_SIDEBAR_SLIDE_STEP) = 0;
    U8(st, G_WINDOW) = 2;
    GAME_CALL0(st, FN_VIEW_GEOMETRY_A);
    GAME_CALL0(st, FN_CAMERA_CLAMP);
    GAME_CALL(st, FN_RETURN_ZERO, 2);
    GAME_CALL(st, FN_UI_SOUND, 6);
}

void sidebar_overlays_click(game_state *st)
{
    /* @bin 0x00409D40 (0x005C7300 entry 1) the overlays menu: window 8 and its layout */
    U8(st, G_WINDOW) = 8;
    S32(st, G_MENU_COUNT) = 0x14;
    S32(st, G_MENU_Y) = 0x28;
    S32(st, G_U7056E4) = 0x0C;
    S32(st, G_U7056E0) = 0;
    S32(st, G_OVL_GROUP) = 0;
    S32(st, G_OVL_ROWS) = 0;
    S32(st, G_U7056D4) = 0;
    S32(st, G_OVL_ROW) = 0x28;
    S32(st, G_U7056CC) = 0x0C;
    U8(st, G_REDRAW) = 1;
}

void sidebar_build_click(game_state *st)
{
    /* @bin 0x00403D70 (0x005C7370 / 0x005C75B0 entries 0..11) a build menu, 0x00D3B198: window 6,
     * menu 0x007056EC (menu 1 is 0x0D when 0x00978D70 = 1), its item count (0x00404120) and the
     * layout 0x005CF63C / 0x005CF640 by count; menus 1, 2, 3 have one building: chosen at once
     * (0x00405200 with item 1) */
    if (U8(st, G_WINDOW) == 6 && S32(st, G_EDITOR) == 1) {
        return;
    }
    U8(st, G_WINDOW) = 6;
    int32_t menu = S32(st, G_PARAM1);
    S32(st, G_SUBMENU) = menu;
    if (menu == 1 && S32(st, G_U978D70) == 1) {
        S32(st, G_SUBMENU) = 0x0D;
    }
    int32_t n = GAME_CALL0(st, FN_BUILD_MENU_COUNT);
    S32(st, G_MENU_COUNT) = n;
    S32(st, G_MENU_Y) = S32(st, G_MENU_LAYOUT + 8u * (uint32_t) n);
    S32(st, G_U7056E4) = S32(st, G_MENU_LAYOUT + 4u + 8u * (uint32_t) n);
    menu = S32(st, G_SUBMENU);
    if (menu == 2 || menu == 3 || menu == 1) {
        S32(st, G_PARAM1) = 1;
        GAME_CALL0(st, FN_CONSTRUCTION_CHOOSE_BUILDING);
    }
    U8(st, G_REDRAW) = 1;
}

void messages_open(game_state *st)
{
    /* @bin 0x004E1620 the message list (sorted, 0x004E1640): window 0x11 */
    GAME_CALL0(st, FN_MESSAGE_SORT);
    U8(st, G_WINDOW) = 0x11;
    U8(st, G_REDRAW) = 1;
}

void sidebar_messages_click(game_state *st)
{
    /* @bin 0x00406020 (0x005C7370 entry 13) jmp 0x004E1620 */
    messages_open(st);
}

void sidebar_next_message_click(game_state *st)
{
    /* @bin 0x00406030 (0x005C7370 entry 14, 0x005C6CB8) the next important message (0x004E23E0),
     * with 0x00D3B198 set; the cycle restarts (0x00E951D4 := 0) after 3 s of wall clock since the
     * last use (0x00E951D0) */
    if (S32(st, G_PARAM1) == 0) {
        return;
    }
    uint32_t now = platform_clock_ms(st); /* @bin 0x00406039 timeGetTime */
    if ((int32_t) (now - U32(st, G_NEXT_MESSAGE_TIME)) > 0xBB8) {
        S32(st, G_NEXT_MESSAGE_INDEX) = 0;
    }
    U32(st, G_NEXT_MESSAGE_TIME) = now;
    GAME_CALL0(st, FN_MESSAGE_NEXT_IMPORTANT);
    U8(st, G_REDRAW) = 1;
}

void sidebar_mission_click(game_state *st)
{
    /* @bin 0x00418390 (0x005C7370 entry 15) the mission briefing: a fade to window 0x3B */
    if (S32(st, G_CUSTOM) != 0) {
        return;
    }
    GAME_CALL(st, FN_FADE_START, 0, 0x3B, 1000);
    S32(st, G_SCROLL) = 0;
    S32(st, G_TEXT_LINES) = 0;
    S32(st, G_TIP_ID) = 0;
    S32(st, G_MW_THUMB_Y) = 0;
    U8(st, G_REDRAW) = 1;
}

void sidebar_advisors_click(game_state *st)
{
    /* @bin 0x0040B520 (0x005C7980 entry 0) the advisors (window 7) at the last advisor 0x00E38EA4,
     * unless not available (0x004179A0) outside the tutorial 0x00E38EBC: a notice 0x0D */
    if (S32(st, G_CUSTOM) == 0 && (GAME_CALL(st, FN_PLAY_ADVISOR_NEXT, G_LAST_ADVISOR) & 0xFF) == 0) {
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0x0D, 0, 0);
        return;
    }
    U8(st, G_WINDOW) = 7;
    S32(st, G_PAGE) = S32(st, G_LAST_ADVISOR);
    U8(st, CURRENT_CITY(st) + CITY_U49CC) = 0;
    GAME_CALL0(st, FN_ADVISORS_REFRESH);
    U8(st, G_REDRAW) = 1;
    S32(st, G_ADVISOR_SLIDE) = 6;
}

void sidebar_empire_click(game_state *st)
{
    /* @bin 0x0040B590 (0x005C7980 entry 1) the empire map (0x00409FA0 from the city, else
     * 0x0040A090); outside the editor and the tutorial, with 0x0078D828 set, a notice 0x0D */
    if (S32(st, G_EDITOR) != 1 && S32(st, G_CUSTOM) == 0 && S32(st, G_U78D828) != 0) {
        GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0x0D, 0, 0);
        return;
    }
    if (U8(st, G_WINDOW) == 1) {
        GAME_CALL0(st, FN_EMPIRE_WINDOW_OPEN);
        return;
    }
    GAME_CALL0(st, FN_EMPIRE_CLOSE_40A090);
}

void sidebar_help_click(game_state *st)
{
    /* @bin 0x00407320 the right-click help of a button: the message 0x00D3B194 */
    GAME_CALL(st, FN_MESSAGE_WINDOW_OPEN, S32(st, G_PARAM2), 0, 0);
    U8(st, G_REDRAW) = 1;
}

/* ---- port table ---- */

static int32_t run_hover(game_state *st, const int32_t *a) { buttons_hover(st, a[0], a[1], (uint32_t) a[2], a[3], a[4]); return 0; }
static int32_t run_click(game_state *st, const int32_t *a) { return buttons_click(st, a[0], a[1], (uint32_t) a[2], a[3]); }
static int32_t run_rect(game_state *st, const int32_t *a) { return buttons_rect_click(st, a[0], a[1], (uint32_t) a[2], a[3]); }
static int32_t run_editor(game_state *st, const int32_t *a) { return buttons_editor_click(st, a[0], a[1], (uint32_t) a[2], a[3]); }
static int32_t run_release(game_state *st, const int32_t *a) { buttons_release_toggles(st, (uint32_t) a[0], a[1]); return 0; }
static int32_t run_countdown(game_state *st, const int32_t *a) { buttons_count_down(st, (uint32_t) a[0], a[1]); return 0; }
static int32_t run_mb_hover(game_state *st, const int32_t *a) { return menubar_hover(st, (uint32_t) a[0], a[1]); }
static int32_t run_mb_click(game_state *st, const int32_t *a) { return menubar_click(st, a[0]); }
static int32_t run_mb_button(game_state *st, const int32_t *a) { (void) a; return menubar_button_click(st); }
static int32_t run_mm_hover(game_state *st, const int32_t *a) { (void) a; minimap_hover(st); return 0; }
static int32_t run_mm_tile(game_state *st, const int32_t *a) { return minimap_tile_at_mouse(st, a[0], a[1], a[2], a[3]); }
static int32_t run_mm_click(game_state *st, const int32_t *a) { (void) a; return minimap_click(st); }
static int32_t run_tb_hover(game_state *st, const int32_t *a) { (void) a; topbar_hover(st); return 0; }
static int32_t run_tb_help(game_state *st, const int32_t *a) { (void) a; topbar_help(st); return 0; }

#define RUN0(name, fn) \
    static int32_t name(game_state *st, const int32_t *a) { (void) a; fn(st); return 0; }
RUN0(run_collapse, sidebar_collapse_click)
RUN0(run_overlays, sidebar_overlays_click)
RUN0(run_build, sidebar_build_click)
RUN0(run_messages_open, messages_open)
RUN0(run_messages, sidebar_messages_click)
RUN0(run_next_message, sidebar_next_message_click)
RUN0(run_mission, sidebar_mission_click)
RUN0(run_advisors, sidebar_advisors_click)
RUN0(run_empire, sidebar_empire_click)
RUN0(run_help, sidebar_help_click)

const ported_function sidebar_input_ports[] = {
    {0x00425BC0u, "buttons_hover", 5, 0, 0, run_hover, 0},
    {0x00425990u, "buttons_click", 4, 32, 0, run_click, 0},
    {0x004261D0u, "buttons_rect_click", 4, 32, 0, run_rect, 0},
    {0x00425C40u, "buttons_editor_click", 4, 32, 0, run_editor, 0},
    {0x00425B50u, "buttons_release_toggles", 2, 0, 0, run_release, 0},
    {0x00425B80u, "buttons_count_down", 2, 0, 0, run_countdown, 0},
    {0x00426100u, "menubar_hover", 2, 32, 0, run_mb_hover, 0},
    {0x00426030u, "menubar_click", 1, 32, 0, run_mb_click, 0},
    {0x00425840u, "menubar_button_click", 0, 8, 0, run_mb_button, 0},
    {0x00406190u, "minimap_hover", 0, 0, 0, run_mm_hover, 0},
    {0x0043EF10u, "minimap_tile_at_mouse", 4, 32, 0, run_mm_tile, 0},
    {0x004061E0u, "minimap_click", 0, 32, 0, run_mm_click, 0},
    {0x00406250u, "topbar_hover", 0, 0, 0, run_tb_hover, 0},
    {0x004072C0u, "topbar_help", 0, 0, 0, run_tb_help, 0},
    {0x00409D10u, "sidebar_collapse_click", 0, 0, 0, run_collapse, 0},
    {0x00409D40u, "sidebar_overlays_click", 0, 0, 0, run_overlays, 0},
    {0x00403D70u, "sidebar_build_click", 0, 0, 0, run_build, 0},
    {0x004E1620u, "messages_open", 0, 0, 0, run_messages_open, 0},
    {0x00406020u, "sidebar_messages_click", 0, 0, 0, run_messages, 0},
    {0x00406030u, "sidebar_next_message_click", 0, 0, 0, run_next_message, 0},
    {0x00418390u, "sidebar_mission_click", 0, 0, 0, run_mission, 0},
    {0x0040B520u, "sidebar_advisors_click", 0, 0, 0, run_advisors, 0},
    {0x0040B590u, "sidebar_empire_click", 0, 0, 0, run_empire, 0},
    {0x00407320u, "sidebar_help_click", 0, 0, 0, run_help, 0},
};
const int sidebar_input_ports_count = sizeof(sidebar_input_ports) / sizeof(sidebar_input_ports[0]);
