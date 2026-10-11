/* Imhotep - the frame 0x004D6B60 and its non-drawing parts (roadmap phase 7.1).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * docs/presentation.md 1 (the frame loop, pacing, present), 1.4 (what a frame writes into the
 * state), 5.2 (mouse per frame). The drawing, input dispatch, tooltips, top bar, messages and sound
 * functions are called through GAME_CALL in the original's order; they belong to the drawing and
 * sound packages (7.2-7.7). The wall clock (timeGetTime) is platform_clock_ms. */
#include "frame/frame.h"

#include "random/crt_rand.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "sim/tick.h"
#include "state/layout.h"
#include "state/ported.h"


/* ---- clock, timers, pacing ---- */

void frame_timers(game_state *st)
{
    /* @bin 0x0040F270 the 51 flags 0x00605B38 are cleared; timer k (0x00605A6C, i32) fires when the
     * frame clock reaches it + 20 * k (signed compare): flag 1, timer := clock */
    int32_t now = S32(st, G_FRAME_CLOCK);
    for (uint32_t k = 0; k < 51; k++) {
        U8(st, G_ANIM_FLAGS + k) = 0;
    }
    for (uint32_t k = 0; k < 51; k++) {
        uint32_t t = G_ANIM_TIMERS + 4 * k;
        if (now >= (int32_t) (U32(st, t) + 20 * k)) {
            U8(st, G_ANIM_FLAGS + k) = 1;
            S32(st, t) = now;
        }
    }
}

int32_t frame_pacing(game_state *st)
{
    /* @bin 0x004DDCB0 */
    uint32_t now = platform_clock_ms(st); /* timeGetTime */
    int32_t dt = (int32_t) (now - U32(st, G_TICK_LAST_TIME));
    if (dt < 0) {
        dt = 10000; /* @bin 0x004DDCC3 the clock went back */
    }
    /* @bin 0x004DDCCA i = (100 - speed 0x00E38E6C) / 10, truncated (idiv by the 0x66666667 trick) */
    int32_t i = (100 - S32(st, G_GAME_SPEED)) / 10;
    if (i >= 10 || U8(st, G_PAUSED) != 0) { /* @bin 0x00E38E60 the pause flag */
        return 0;
    }
    uint8_t w = U8(st, G_WINDOW);
    if (w == 0x12 || w == 0x23) {
        if (S32(st, G_TIP_WINDOW) != 1) { /* @bin 0x004DDD17 */
            return 0;
        }
    } else if (S32(st, G_WINDOW_STOPS_TIME + 4u * w) != 0) { /* @bin 0x005E1C50 windows that stop time */
        return 0;
    }
    /* @bin 0x004DDD20 a construction drag (0x006059E4) or a scroll stops time */
    if (S32(st, G_CONSTRUCTION_DRAG) != 0 || S32(st, G_SCROLLING) != 0) {
        return 0;
    }
    if (i < 0) {
        i = 0;
    }
    /* @bin 0x005E1DB4 the delay table {0, 20, 35, 55, 80, 110, 160, 240, 350, 500, 700} + 2 ms */
    if (dt < S32(st, G_SPEED_DELAYS + 4u * (uint32_t) i) + 2) {
        return 0;
    }
    U32(st, G_TICK_LAST_TIME) = now;
    return 10000;
}

/* ---- mouse, keys, cursor ---- */

void frame_clear_keys(game_state *st)
{
    /* @bin 0x004CF1F0 */
    S32(st, G_UE38DFC) = 0;
    U8(st, G_LEFT_LATCH) = 0;
    U8(st, G_UE38DF6) = 0;
}

void frame_mouse(game_state *st)
{
    /* @bin 0x004CF210 */
    int32_t px, py;
    frame_cursor_pos(st, &px, &py); /* GetCursorPos */
    S32(st, G_CURSOR_POS_X) = px;
    S32(st, G_CURSOR_POS_Y) = py;
    if (S32(st, G_SCROLLING) != 0) { /* @bin 0x004CF23D the 16-bit position from GetCursorPos */
        S16(st, G_MSG_MOUSE_X) = (int16_t) px;
        S16(st, G_MSG_MOUSE_Y) = (int16_t) py;
    }
    int32_t old_y = S32(st, G_MOUSE_Y);
    int32_t old_x = S32(st, G_MOUSE_X);
    if (S32(st, G_DRAG_STATE) == 1) { /* @bin 0x004CF263 a drag past 50 px from its start */
        int32_t sx = S32(st, G_DRAG_START_X);
        if (old_x > sx + 50) {
            S32(st, G_DRAG_STATE) = 2;
        }
        if (old_x < sx - 50) {
            S32(st, G_DRAG_STATE) = 2;
        }
        int32_t sy = S32(st, G_DRAG_START_Y);
        if (old_y > sy + 50) {
            S32(st, G_DRAG_STATE) = 2;
        }
        if (old_y < sy - 50) {
            S32(st, G_DRAG_STATE) = 2;
        }
    }
    uint8_t old_right = U8(st, G_RIGHT_HELD);
    int32_t now = S32(st, G_FRAME_CLOCK);
    S32(st, G_MOUSE_X_PREV) = old_x;
    U8(st, G_RIGHT_HELD_PREV) = old_right;
    S32(st, G_MOUSE_Y_PREV) = old_y;
    U8(st, G_LEFT_HELD_PREV) = U8(st, G_LEFT_HELD);
    S32(st, G_MOUSE_MOVED) = 0;
    /* @bin 0x004CF2DE the scale: 0x004CDE60(screen size, client size), 0x004CDE40(position, scale) */
    int32_t kx = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, G_DRAW_W), S32(st, G_CLIENT_RECT + 8) - S32(st, G_CLIENT_RECT));
    int32_t ky = GAME_CALL(st, FN_CALC_RATIO_PERCENT, S32(st, G_DRAW_H), S32(st, G_CLIENT_RECT + 12) - S32(st, G_CLIENT_RECT + 4));
    S32(st, G_MOUSE_X) = GAME_CALL(st, FN_CALC_PERCENTAGE, (int16_t) S16(st, G_MSG_MOUSE_X), kx);
    int32_t y = GAME_CALL(st, FN_CALC_PERCENTAGE, (int16_t) S16(st, G_MSG_MOUSE_Y), ky);
    uint8_t buttons = U8(st, G_BUTTONS);
    S32(st, G_MOUSE_Y) = y;
    S32(st, G_BUTTON_CHANGED) = 0;
    U8(st, G_LEFT_HELD) = 0;
    U8(st, G_RIGHT_HELD) = 0;
    U8(st, G_LEFT_PRESSED) = 0;
    U8(st, G_LEFT_RELEASED) = 0;
    U8(st, G_RIGHT_PRESSED) = 0;
    U8(st, G_RIGHT_RELEASED) = 0;
    uint8_t right = 0, left = 0;
    if (buttons & 2) {
        right = 1;
        U8(st, G_RIGHT_HELD) = 1;
    }
    if (buttons & 1) {
        left = 1;
        U8(st, G_LEFT_HELD) = 1;
    }
    if (S32(st, G_MOUSE_X_PREV) != S32(st, G_MOUSE_X)) {
        S32(st, G_MOUSE_MOVED) = 1;
    }
    if (S32(st, G_MOUSE_Y_PREV) != y || S32(st, G_MOUSE_MOVED) != 0) {
        /* @bin 0x004CF3AA moved: the time of the last move 0x00E36C8C */
        S32(st, G_MOUSE_MOVED) = 1;
        S32(st, G_MOUSE_MOVE_TIME) = now;
        S32(st, G_MOUSE_STILL_TIME) = 0;
    } else {
        S32(st, G_MOUSE_STILL_TIME) = now - S32(st, G_MOUSE_MOVE_TIME);
    }
    if (left != U8(st, G_LEFT_HELD_PREV)) {
        S32(st, G_MOUSE_MOVED) = 1;
        S32(st, G_BUTTON_CHANGED) = 1;
        if (left == 1) {
            U8(st, G_LEFT_PRESSED) = 1;
            GAME_CALL(st, FN_UI_SOUND, 0); /* @bin 0x004CF403 the click sound */
            right = U8(st, G_RIGHT_HELD);
        } else {
            U8(st, G_LEFT_RELEASED) = 1;
        }
    }
    if (right != U8(st, G_RIGHT_HELD_PREV)) {
        S32(st, G_MOUSE_MOVED) = 1;
        S32(st, G_BUTTON_CHANGED) = 1;
        if (right == 1) {
            U8(st, G_UE38E06) = 0;
            U8(st, G_RIGHT_PRESSED) = 1;
            return;
        }
        if (right == 0) {
            U8(st, G_RIGHT_RELEASED) = 1;
        }
    }
    U8(st, G_UE38E06) = 0;
}

void frame_cursor(game_state *st, int32_t n)
{
    /* @bin 0x004CF460 the handle of cursor n (the 19 loaded by 0x004CF960; any other n: cursor 0)
     * becomes the current one 0x00E390DC; SetCursor itself is the platform's (no request yet) */
    static const uint32_t handle[19] = {G_CURSOR_66, G_CURSOR_96, G_CURSOR_82, G_CURSOR_8E, G_CURSOR_UNLOAD,
                                        G_CURSOR_BOARD, G_CURSOR_92, G_CURSOR_93, G_CURSOR_88, G_CURSOR_94,
                                        G_CURSOR_95, G_CURSOR_98, G_CURSOR_99, G_CURSOR_9A, G_CURSOR_9B,
                                        G_CURSOR_9C, G_CURSOR_9D, G_CURSOR_9F, G_CURSOR_A0};
    uint32_t a = (uint32_t) n <= 0x12u ? handle[n] : G_CURSOR_66;
    U32(st, G_CURSOR) = U32(st, a);
}

static uint32_t hovered_offset(game_state *st)
{
    /* the hovered tile's map offset: y * 228 + x + map start 0x00BCA2D8 */
    return (uint32_t) (S32(st, G_MOUSE_TILE_Y) * 228 + S32(st, G_MOUSE_TILE_X) + S32(st, G_MAP_START));
}

static int32_t cursor_attack_target(game_state *st, int32_t f)
{
    /* @bin 0x004D6AA0 a figure that can attack (0x00D05708 set, figure +0xF8 clear) over a tile with
     * an enemy (0x004B5520) */
    if (S32(st, G_TRANSPORT_MODE) == 0 || U8(st, FIGURE_ADDR(f) + FIG_U130) != 0) {
        return 0;
    }
    return GAME_CALL(st, FN_TILE_SOLDIER_FORMATION, (int32_t) hovered_offset(st)) != 0;
}

static int32_t cursor_move_target(game_state *st, int32_t f)
{
    /* @bin 0x004D6B00 a figure that moves (0x00D05708 set, figure +0xF8 set) over a tile whose
     * terrain (0x00AF598C) has no bit 4 */
    if (S32(st, G_TRANSPORT_MODE) == 0 || U8(st, FIGURE_ADDR(f) + FIG_U130) == 0) {
        return 0;
    }
    return (U32(st, G_GRID_TERRAIN + 4 * hovered_offset(st)) & 4) != 4;
}

static int32_t run_attack_target(game_state *st, const int32_t *a) { return cursor_attack_target(st, a[0]); }
static int32_t run_move_target(game_state *st, const int32_t *a) { return cursor_move_target(st, a[0]); }

/* @bin 0x004D6CF0 the cursor of the frame's window; -1: no change */
static int32_t frame_cursor_choice(game_state *st)
{
    uint8_t w = U8(st, G_WINDOW);
    if (w == 0x1E) { /* formation orders */
        int32_t enemy = GAME_CALL(st, FN_TILE_ENEMY_FIGURE, S32(st, G_MOUSE_OFFSET));
        uint32_t rec = G_FORMATION + (uint32_t) S32(st, G_FORMATION) * 0x90u;
        if (enemy != 0) {
            return S16(st, rec + 0x0C) != 0x0C ? 9 : 8;
        }
        /* @bin 0x004D7030 by the formation layout +0x32: 1..4 -> 11..14, 15 -> 15, else 1 */
        switch (S16(st, rec + 0x32)) {
        case 1: return 0x0B;
        case 2: return 0x0C;
        case 3: return 0x0D;
        case 4: return 0x0E;
        case 0x0F: return 0x0F;
        default: return 1;
        }
    }
    if (w == 0x49) {
        return 0x0A;
    }
    if (w == 1) {
        return S32(st, G_CONSTRUCTION_UI_TYPE) == 9 ? 2 : 0;
    }
    if (w == 0x1F) { /* @bin 0x004D6DA2 a ship or a transport chosen (0x00D05700) */
        int32_t f = S32(st, G_SEL_TRANSPORT);
        if (f <= 0) {
            frame_cursor(st, 0);
        }
        /* @bin 0x004D7058 by figure type 9..20 */
        switch (U8(st, FIGURE_ADDR(f) + FIG_ACTION)) {
        case 9:
            if (cursor_attack_target(st, f)) {
                return 5;
            }
            return cursor_move_target(st, f) ? 4 : 0x12;
        case 10: return 0x10;
        case 11:
            if (S32(st, G_TRANSPORT_MODE) != 0) {
                return 0x12;
            }
            if (cursor_attack_target(st, f)) {
                return 5;
            }
            return cursor_move_target(st, f) ? 4 : -1;
        case 12: return cursor_attack_target(st, f) ? 5 : 0x12;
        case 13: return cursor_move_target(st, f) ? 4 : 0x12;
        case 20: return S32(st, G_TRANSPORT_MODE) != 0 ? 6 : -1;
        default: return 3;
        }
    }
    if (w == 0x20) {
        if (GAME_CALL(st, FN_TILE_ENEMY_FIGURE, S32(st, G_MOUSE_OFFSET)) != 0) {
            return 9;
        }
        int32_t f = S32(st, G_SEL_WARSHIP);
        if (f <= 0) {
            frame_cursor(st, 0);
        }
        switch (U8(st, FIGURE_ADDR(f) + FIG_ACTION)) {
        case 0x0A: return 0x11;
        case 0x14: return S32(st, G_WARSHIP_MODE) != 0 ? 7 : -1;
        case 0x17: return 3;
        default: return 0;
        }
    }
    return 0;
}

/* ---- hovered tile ---- */

int32_t frame_screen_to_tile(game_state *st)
{
    /* @bin 0x00430790 the city view's tile under the scaled mouse (0x00E38E34, 0x00E38E30): the
     * view's x, y, tile width / height and the view-to-tile table 0x0070E494 (0xE7 per row);
     * 0 outside the view or on a tile >= 0x0FFF0000 (none). Writes the view column / row 0x0070E404 /
     * 0x0070E408, the table entry 0x0070E3F8 and the edge flags 0x0070E3FC / 0x0070E400 */
    int32_t mx = S32(st, G_MOUSE_X);
    int32_t my = S32(st, G_MOUSE_Y);
    if (!zoom_point(st, &mx, &my)) { /* @imhotep enhanced zoom: the view's own pixels */
        return 0;
    }
    int32_t vx = S32(st, G_VIEW_X);
    if (mx < vx || mx >= (S32(st, G_U70E458) + 2) * S32(st, G_VIEW_COLS) + vx) {
        return 0;
    }
    int32_t vy = S32(st, G_VIEW_Y);
    int32_t th = S32(st, G_TILE_H);
    if (my < vy || my >= th * S32(st, G_VIEW_ROWS) + vy) {
        return 0;
    }
    my -= vy;
    mx -= vx;
    int32_t row = my / th;
    S32(st, G_VIEW_CELL_ROW) = row;
    int32_t tw = S32(st, G_TILE_W) + 1;
    int32_t col = mx / tw;
    int32_t col_odd = col & 1;
    int32_t row_odd = row & 1;
    int32_t parity = (row + col) & 1;
    int32_t half = (mx % tw) / 2;     /* @bin 0x0043084B cdq; sub; sar 1: toward 0 */
    int32_t in_row = my % S32(st, G_TILE_H);
    int32_t x = col / 2;
    S32(st, G_VIEW_CELL_COL) = x;
    if (parity == 0) {
        if (in_row > half) {
            row++;
            S32(st, G_VIEW_CELL_ROW) = row;
        } else if (col_odd != 0 && row_odd != 0) {
            x++;
            S32(st, G_VIEW_CELL_COL) = x;
        }
    } else if (parity == 1) {
        if (in_row + half >= S32(st, G_TILE_H) - 1) {
            row++;
            S32(st, G_VIEW_CELL_ROW) = row;
            if (col_odd != 0 && row_odd == 0) {
                x++;
                S32(st, G_VIEW_CELL_COL) = x;
            }
        }
    }
    int32_t r = S32(st, G_VIEW_FIRST_ROW) + row;
    int32_t odd = row & 1;
    /* @bin 0x004308B7 rows of 0xE7 entries */
    int32_t entry = S32(st, G_VIEW_TABLE + 4u * (uint32_t) (r * 0xE7 + S32(st, G_VIEW_FIRST_COL) + x));
    S32(st, G_VIEW_CELL_ENTRY) = entry;
    if (entry >= 0x0FFF0000) {
        return 0;
    }
    S32(st, G_VIEW_CELL_ROW_EDGE) = 0;
    S32(st, G_VIEW_CELL_COL_EDGE) = 0;
    if (row == 0) {
        S32(st, G_VIEW_CELL_ROW_EDGE) = 2;
    } else if (row >= S32(st, G_VIEW_ROWS)) {
        S32(st, G_VIEW_CELL_ROW_EDGE) = 1;
    }
    if (odd != 0) {
        if (x == 0) {
            S32(st, G_VIEW_CELL_COL_EDGE) = 2;
        } else if (x >= S32(st, G_VIEW_COLS)) {
            S32(st, G_VIEW_CELL_COL_EDGE) = 1;
        }
    }
    return entry;
}

void frame_hover_tile(game_state *st)
{
    /* @bin 0x00405F80 */
    S32(st, G_MOUSE_TILE_Y) = 0;
    S32(st, G_MOUSE_TILE_X) = 0;
    int32_t t = frame_screen_to_tile(st);
    S32(st, G_MOUSE_OFFSET) = t;
    if (t == 0) {
        return;
    }
    int32_t d = t - S32(st, G_MAP_START);
    int32_t x = d % 228;
    int32_t y = d / 228;
    S32(st, G_MOUSE_TILE_X) = x;
    S32(st, G_MOUSE_TILE_Y) = y;
    if (U8(st, G_FROZEN) != 0) { /* @bin 0x00605A5C 'M' freezes the hovered tile */
        return;
    }
    S32(st, G_HOVER_TILE) = t;
    S32(st, G_HOVER_X) = x;
    S32(st, G_HOVER_Y) = y;
    /* @bin 0x00405F60 building type 0xB7 is being placed: 0x004ED330 */
    if (S32(st, G_CONSTRUCTION_UI_TYPE) == 0xB7) {
        GAME_CALL0(st, FN_PYRAMID_NEAREST_4ED330);
    }
}

static int32_t run_hover_0x00405F60(game_state *st, const int32_t *a)
{
    (void) a;
    if (S32(st, G_CONSTRUCTION_UI_TYPE) == 0xB7) {
        GAME_CALL0(st, FN_PYRAMID_NEAREST_4ED330);
    }
    return 0;
}

/* ---- present ---- */

static int32_t present_elapsed(game_state *st)
{
    /* @bin 0x004D0040 ms since the last call (timeGetTime; last 0x00E36C88) */
    uint32_t now = platform_clock_ms(st);
    uint32_t d = now - U32(st, G_PRESENT_LAST_TIME);
    U32(st, G_PRESENT_LAST_TIME) = now;
    return (int32_t) d;
}

void frame_present(game_state *st, int32_t force)
{
    /* @bin 0x004D0280 */
    if (S32(st, G_PRESENT_WAIT) == 1) { /* DirectDraw not ready */
        return;
    }
    if (S32(st, G_VIDEO_PLAYING) != 0 && S32(st, G_UE92E04) != 0) { /* a video plays */
        return;
    }
    int32_t shake = S32(st, G_HAIL_TIMER);
    if (U8(st, G_WINDOW) != 5 && shake > 0 && shake % 0x88 == 0) {
        /* @bin 0x004D035E the fade (0x004D05F0, eight steps of 0x004D0650 on the object 0x006DE518),
         * each step presented */
        GAME_CALLT(st, G_SURFACE_LOCK, FN_FADE_START_4D05F0, 1, 0x0F, 0, 1, 0);
        GAME_CALL0(st, FN_PRESENT_COPY);
        for (int i = 0; i < 8; i++) {
            GAME_CALLT(st, G_SURFACE_LOCK, FN_FADE_STEP_4D0650, 1);
            GAME_CALL0(st, FN_PRESENT_COPY);
        }
        return;
    }
    int32_t d = present_elapsed(st);
    S32(st, G_PRESENT_ELAPSED) += d;
    if (force == 1) {
        S32(st, G_PRESENT_ELAPSED) = 50;
    }
    /* @bin 0x004D02F5 every 50 ms, or when 0x00E38DC8 asks */
    if (U8(st, G_PRESENT_NOW) == 0 && S32(st, G_PRESENT_ELAPSED) < 50) {
        return;
    }
    uint8_t w = U8(st, G_WINDOW);
    S32(st, G_PRESENT_ELAPSED) = 0;
    U8(st, G_PRESENT_NOW) = 0;
    /* @bin 0x004D039C the city windows 1, 3, 0x1E, 0x1F, 0x20, 0x49 redraw the sidebar first while
     * 0x0078D7A0 > 0 */
    if (w != 5 && S32(st, G_HAIL_TIMER) > 0 &&
        (w == 1 || w == 3 || w == 0x1E || w == 0x1F || w == 0x20 || w == 0x49)) {
        GAME_CALL0(st, FN_SIDEBAR_DRAW);
        GAME_CALLT(st, G_SURFACE_LOCK, FN_FADE_START_4D05F0, 1, 1, 0, 0, 1);
    }
    GAME_CALL0(st, FN_PRESENT_COPY);
}

static int32_t run_present_copy(game_state *st, const int32_t *a)
{
    /* @bin 0x004D03F0 the copy of the back buffer to the screen (DirectDraw Blt / BltFast): the
     * platform's (platform_present); it writes no game state */
    (void) a;
    platform_present(st);
    return 0;
}

/* ---- the frame ---- */

void frame_run(game_state *st)
{
    /* @bin 0x004D6B60 */
    S32(st, G_FRAME_COUNTER) += 1;                       /* the frame counter */
    U32(st, G_FRAME_CLOCK) = platform_clock_ms(st);  /* timeGetTime */
    frame_timers(st);
    int32_t n = frame_pacing(st);
    for (int32_t i = 0; i < n; i++) {
        S32(st, G_TICK_COUNTER) += 1;
        game_tick(st, st->env);
        /* @bin 0x004D6BA5 fast-forward: up to n ticks this frame, until a right button up is
         * pending (PeekMessageA, PM_NOREMOVE); then the pending mouse and key messages are dropped */
        if (U8(st, G_FAST_FORWARD) == 0) {
            n = 0;
        } else if (frame_peek_message(st, 0x205, 0x205, 0)) {
            while (frame_peek_message(st, 0x200, 0x209, 1)) {
            }
            while (frame_peek_message(st, 0x100, 0x108, 1)) {
            }
            U8(st, G_FAST_FORWARD) = 0;
            n = 0;
        } else if (U8(st, G_FAST_FORWARD) == 0) {
            n = 0;
        }
        frame_autosave(st);
    }
    frame_mouse(st);
    GAME_CALL(st, FN_UI_REDRAW, 0);   /* the window's background, when REDRAW */
    if (S32(st, G_LAYOUT_MODE) != 0) { /* @bin 0x004D6070 */
        U8(st, G_REDRAW) = 1;
    }
    GAME_CALL0(st, FN_UI_DRAW_FRAME);     /* the window's per-frame draw */
    GAME_CALL0(st, FN_UI_INPUT);     /* the window's input */
    frame_clear_keys(st);
    GAME_CALL0(st, FN_TOOLTIP_UPDATE);     /* tooltips */
    GAME_CALL0(st, FN_TOOLTIP_OVERLAY_TEXT);
    GAME_CALL(st, FN_TOPBAR_DRAW, 0);   /* the top menu bar */
    GAME_CALL(st, FN_SIDEBAR_DRAW_MESSAGES, 1);
    GAME_CALL0(st, FN_PLAY_SCREEN_MESSAGES);     /* on-screen messages */
    if (S32(st, G_TIP_ID) != 0) {
        GAME_CALL0(st, FN_TOOLTIP_DRAW);
        if (S32(st, G_TIP_KIND) == 0x23) {
            frame_hover_tile(st);
        }
        GAME_CALL0(st, FN_TOOLTIP_END);
    }
    GAME_CALL0(st, FN_PLAY_NETWORK_INFO);
    /* @bin 0x004D6CB7 a Bink video (0x006FDC40) is played instead of the present */
    if (S32(st, G_MOVIE) != 0) {
        if ((uint8_t) GAME_CALL(st, 0x005605A0u, S32(st, G_U6FDC38)) != 0) {
            GAME_CALL0(st, 0x00416BC0u);
        }
    }
    if (S32(st, G_MOVIE) == 0) {
        frame_present(st, 0);
    }
    GAME_CALL0(st, FN_AMBIENT_STEP);     /* the ambient-sound slots */
    int32_t c = frame_cursor_choice(st);
    if (c >= 0) {
        frame_cursor(st, c);
    }
    /* @bin 0x004D6ECF music: battle music while the city has more than 15 enemies (city +0x634) or
     * more than 4 (+0x9A0) */
    uint32_t city = CURRENT_CITY(st);
    if (S32(st, city + CITY_U4634) <= 0x0F && S32(st, city + CITY_U49A0) <= 4) {
        if (S32(st, G_BATTLE_MUSIC) == 1) {
            S32(st, G_STREAM_FADE0) = 1;
            S32(st, G_BATTLE_MUSIC) = 0;
        }
        if (GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 0) == 0 && S32(st, G_UF55B94) == 0) {
            if (U8(st, G_WINDOW) != 1) {
                goto ambient;
            }
            GAME_CALLT0(st, G_MUSIC, FN_MUSIC_STEP); /* the next tune */
        }
    } else if (S32(st, G_SOUND_ON) != 0 && S32(st, G_BATTLE_MUSIC) == 0 && U8(st, G_MUSIC_ON) != 0) {
        GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, (int32_t) G_STR_BATTLE_MP3, 0, 0); /* "audio\music\battle.mp3" */
        S32(st, G_BATTLE_MUSIC) = 1;
    }
    if (U8(st, G_WINDOW) == 1) {
        /* @bin 0x004D6F7C the per-frame CRT rand draw of the city view: a random city sound when
         * rand() * 50 / 32768 == 1 */
        int32_t r = crt_rand(st);
        if ((r * 50) / 32768 == 1) {
            GAME_CALL0(st, FN_CITY_RANDOM_SOUND);
        }
    }
ambient:
    if (S32(st, G_LOCUST_TIMER) == 0 && S32(st, G_HAIL_TIMER) == 0 && S32(st, G_FROGS_COUNTDOWN) == 0) {
        if (S32(st, G_SCROLLING) != 0) {
            GAME_CALL(st, FN_STREAM_FADE_OUT, 2);
        } else {
            uint8_t w = U8(st, G_WINDOW);
            if (w == 1 || w == 9 || w == 5) {
                GAME_CALL0(st, FN_AMBIENT_STREAM);
            } else if (GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 2) != 0) {
                GAME_CALLT(st, G_SOUND, FN_SOUND_STOP_STREAM, 2);
            }
        }
    }
    if (S32(st, G_STREAM_FADE0) != 0) {
        GAME_CALL(st, FN_STREAM_FADE_OUT, 0);
    }
    if (S32(st, G_STREAM_FADE1) != 0) {
        GAME_CALL(st, FN_STREAM_FADE_OUT, 1);
    }
}

/* ---- the port table ---- */

static int32_t run_frame(game_state *st, const int32_t *a)
{
    (void) a;
    frame_run(st);
    return 0;
}

static int32_t run_timers(game_state *st, const int32_t *a)
{
    (void) a;
    frame_timers(st);
    return 0;
}

static int32_t run_pacing(game_state *st, const int32_t *a)
{
    (void) a;
    return frame_pacing(st);
}

static int32_t run_mouse(game_state *st, const int32_t *a)
{
    (void) a;
    frame_mouse(st);
    return 0;
}

static int32_t run_clear_keys(game_state *st, const int32_t *a)
{
    (void) a;
    frame_clear_keys(st);
    return 0;
}

static int32_t run_redraw_check(game_state *st, const int32_t *a)
{
    (void) a;
    if (S32(st, G_LAYOUT_MODE) != 0) {
        U8(st, G_REDRAW) = 1;
    }
    return 0;
}

static int32_t run_cursor(game_state *st, const int32_t *a)
{
    frame_cursor(st, a[0]);
    return 0;
}

static int32_t run_hover(game_state *st, const int32_t *a)
{
    (void) a;
    frame_hover_tile(st);
    return 0;
}

static int32_t run_screen_to_tile(game_state *st, const int32_t *a)
{
    (void) a;
    return frame_screen_to_tile(st);
}

static int32_t run_present(game_state *st, const int32_t *a)
{
    frame_present(st, a[0]);
    return 0;
}

static int32_t run_present_elapsed(game_state *st, const int32_t *a)
{
    (void) a;
    return present_elapsed(st);
}

const ported_function frame_ports[] = {
    {0x004D6B60u, "frame_run", 0, 0, 0, run_frame, 0},
    {0x0040F270u, "frame_timers", 0, 0, 0, run_timers, 0},
    {0x004DDCB0u, "frame_pacing", 0, 32, 0, run_pacing, 0},
    {0x004CF210u, "frame_mouse", 0, 0, 0, run_mouse, 0},
    {0x004CF1F0u, "frame_clear_keys", 0, 0, 0, run_clear_keys, 0},
    {0x004D6070u, "frame_redraw_check", 0, 0, 0, run_redraw_check, 0},
    {0x004CF460u, "frame_cursor", 1, 0, 0, run_cursor, 0},
    {0x004D6AA0u, "cursor_attack_target", 1, 32, 0, run_attack_target, 0},
    {0x004D6B00u, "cursor_move_target", 1, 32, 0, run_move_target, 0},
    {0x00405F80u, "frame_hover_tile", 0, 0, 0, run_hover, 0},
    {0x00405F60u, "hover_place_0xB7", 0, 0, 0, run_hover_0x00405F60, 0},
    {0x00430790u, "frame_screen_to_tile", 1, 32, 0, run_screen_to_tile, 0},
    {0x004D0280u, "frame_present", 1, 0, 0, run_present, 0},
    {0x004D0040u, "present_elapsed", 0, 32, 0, run_present_elapsed, 0},
    {0x004D03F0u, "present_copy (platform)", 0, 0, 0, run_present_copy, PORT_PLATFORM},
};
const int frame_ports_count = sizeof(frame_ports) / sizeof(frame_ports[0]);
