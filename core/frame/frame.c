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
#include "state/ported.h"

#define WINDOW 0x00E38DDAu       /* u8 the current window id */
#define REDRAW 0x00E38DCAu       /* u8 full redraw of the window's background */
#define PRESENT_NOW 0x00E38DC8u  /* u8 present at the next present call */
#define FRAME_CLOCK 0x00E92E0Cu  /* the frame's timeGetTime */
#define FAST_FORWARD 0x00E3908Cu /* u8 set by the day cascade 0x00452370 */
#define SCROLLING 0x00E38DCCu    /* i32 the map scrolls (0x004CF660) */
#define PLAYER 0x00EAAC28u
#define CITY_SIZE 0x49D8u
#define SOUND 0x00E92BF8u        /* the sound object (thiscall) */
#define FORMATION 0x00BCB9C0u    /* i32 the current formation; records of 0x90 bytes from here */
#define FIGURE_TYPE 0x00BCD630u  /* u8 figure type, records of 0x184 bytes */

/* ---- clock, timers, pacing ---- */

void frame_timers(game_state *st)
{
    /* @bin 0x0040F270 the 51 flags 0x00605B38 are cleared; timer k (0x00605A6C, i32) fires when the
     * frame clock reaches it + 20 * k (signed compare): flag 1, timer := clock */
    int32_t now = S32(st, FRAME_CLOCK);
    for (uint32_t k = 0; k < 51; k++) {
        U8(st, 0x00605B38u + k) = 0;
    }
    for (uint32_t k = 0; k < 51; k++) {
        uint32_t t = 0x00605A6Cu + 4 * k;
        if (now >= (int32_t) (U32(st, t) + 20 * k)) {
            U8(st, 0x00605B38u + k) = 1;
            S32(st, t) = now;
        }
    }
}

int32_t frame_pacing(game_state *st)
{
    /* @bin 0x004DDCB0 */
    uint32_t now = platform_clock_ms(st); /* timeGetTime */
    int32_t dt = (int32_t) (now - U32(st, 0x00E94DB8u));
    if (dt < 0) {
        dt = 10000; /* @bin 0x004DDCC3 the clock went back */
    }
    /* @bin 0x004DDCCA i = (100 - speed 0x00E38E6C) / 10, truncated (idiv by the 0x66666667 trick) */
    int32_t i = (100 - S32(st, 0x00E38E6Cu)) / 10;
    if (i >= 10 || U8(st, 0x00E38E60u) != 0) { /* @bin 0x00E38E60 the pause flag */
        return 0;
    }
    uint8_t w = U8(st, WINDOW);
    if (w == 0x12 || w == 0x23) {
        if (S32(st, 0x00D3A6C4u) != 1) { /* @bin 0x004DDD17 */
            return 0;
        }
    } else if (S32(st, 0x005E1C50u + 4u * w) != 0) { /* @bin 0x005E1C50 windows that stop time */
        return 0;
    }
    /* @bin 0x004DDD20 a construction drag (0x006059E4) or a scroll stops time */
    if (S32(st, 0x006059E4u) != 0 || S32(st, SCROLLING) != 0) {
        return 0;
    }
    if (i < 0) {
        i = 0;
    }
    /* @bin 0x005E1DB4 the delay table {0, 20, 35, 55, 80, 110, 160, 240, 350, 500, 700} + 2 ms */
    if (dt < S32(st, 0x005E1DB4u + 4u * (uint32_t) i) + 2) {
        return 0;
    }
    U32(st, 0x00E94DB8u) = now;
    return 10000;
}

/* ---- mouse, keys, cursor ---- */

void frame_clear_keys(game_state *st)
{
    /* @bin 0x004CF1F0 */
    S32(st, 0x00E38DFCu) = 0;
    U8(st, 0x00E38E02u) = 0;
    U8(st, 0x00E38DF6u) = 0;
}

void frame_mouse(game_state *st)
{
    /* @bin 0x004CF210 */
    int32_t px, py;
    frame_cursor_pos(st, &px, &py); /* GetCursorPos */
    S32(st, 0x00E38DC4u) = px;
    S32(st, 0x00E38DC0u) = py;
    if (S32(st, SCROLLING) != 0) { /* @bin 0x004CF23D the 16-bit position from GetCursorPos */
        S16(st, 0x00E38E0Au) = (int16_t) px;
        S16(st, 0x00E38E08u) = (int16_t) py;
    }
    int32_t old_y = S32(st, 0x00E38E30u);
    int32_t old_x = S32(st, 0x00E38E34u);
    if (S32(st, 0x00E38E14u) == 1) { /* @bin 0x004CF263 a drag past 50 px from its start */
        int32_t sx = S32(st, 0x00E38E1Cu);
        if (old_x > sx + 50) {
            S32(st, 0x00E38E14u) = 2;
        }
        if (old_x < sx - 50) {
            S32(st, 0x00E38E14u) = 2;
        }
        int32_t sy = S32(st, 0x00E38E18u);
        if (old_y > sy + 50) {
            S32(st, 0x00E38E14u) = 2;
        }
        if (old_y < sy - 50) {
            S32(st, 0x00E38E14u) = 2;
        }
    }
    uint8_t old_right = U8(st, 0x00E38DF4u);
    int32_t now = S32(st, FRAME_CLOCK);
    S32(st, 0x00E38E10u) = old_x;
    U8(st, 0x00E38DEDu) = old_right;
    S32(st, 0x00E38E0Cu) = old_y;
    U8(st, 0x00E38DF9u) = U8(st, 0x00E38E00u);
    S32(st, 0x00E38E40u) = 0;
    /* @bin 0x004CF2DE the scale: 0x004CDE60(screen size, client size), 0x004CDE40(position, scale) */
    int32_t kx = GAME_CALL(st, 0x004CDE60u, S32(st, 0x00E38D9Cu), S32(st, 0x00E39138u) - S32(st, 0x00E39130u));
    int32_t ky = GAME_CALL(st, 0x004CDE60u, S32(st, 0x00E38D98u), S32(st, 0x00E3913Cu) - S32(st, 0x00E39134u));
    S32(st, 0x00E38E34u) = GAME_CALL(st, 0x004CDE40u, (int16_t) S16(st, 0x00E38E0Au), kx);
    int32_t y = GAME_CALL(st, 0x004CDE40u, (int16_t) S16(st, 0x00E38E08u), ky);
    uint8_t buttons = U8(st, 0x00E38E07u);
    S32(st, 0x00E38E30u) = y;
    S32(st, 0x00E38E38u) = 0;
    U8(st, 0x00E38E00u) = 0;
    U8(st, 0x00E38DF4u) = 0;
    U8(st, 0x00E38E03u) = 0;
    U8(st, 0x00E38E04u) = 0;
    U8(st, 0x00E38DF7u) = 0;
    U8(st, 0x00E38DF8u) = 0;
    uint8_t right = 0, left = 0;
    if (buttons & 2) {
        right = 1;
        U8(st, 0x00E38DF4u) = 1;
    }
    if (buttons & 1) {
        left = 1;
        U8(st, 0x00E38E00u) = 1;
    }
    if (S32(st, 0x00E38E10u) != S32(st, 0x00E38E34u)) {
        S32(st, 0x00E38E40u) = 1;
    }
    if (S32(st, 0x00E38E0Cu) != y || S32(st, 0x00E38E40u) != 0) {
        /* @bin 0x004CF3AA moved: the time of the last move 0x00E36C8C */
        S32(st, 0x00E38E40u) = 1;
        S32(st, 0x00E36C8Cu) = now;
        S32(st, 0x00E38E3Cu) = 0;
    } else {
        S32(st, 0x00E38E3Cu) = now - S32(st, 0x00E36C8Cu);
    }
    if (left != U8(st, 0x00E38DF9u)) {
        S32(st, 0x00E38E40u) = 1;
        S32(st, 0x00E38E38u) = 1;
        if (left == 1) {
            U8(st, 0x00E38E03u) = 1;
            GAME_CALL(st, 0x0053B090u, 0); /* @bin 0x004CF403 the click sound */
            right = U8(st, 0x00E38DF4u);
        } else {
            U8(st, 0x00E38E04u) = 1;
        }
    }
    if (right != U8(st, 0x00E38DEDu)) {
        S32(st, 0x00E38E40u) = 1;
        S32(st, 0x00E38E38u) = 1;
        if (right == 1) {
            U8(st, 0x00E38E06u) = 0;
            U8(st, 0x00E38DF7u) = 1;
            return;
        }
        if (right == 0) {
            U8(st, 0x00E38DF8u) = 1;
        }
    }
    U8(st, 0x00E38E06u) = 0;
}

void frame_cursor(game_state *st, int32_t n)
{
    /* @bin 0x004CF460 the handle of cursor n (the 19 loaded by 0x004CF960; any other n: cursor 0)
     * becomes the current one 0x00E390DC; SetCursor itself is the platform's (no request yet) */
    static const uint32_t handle[19] = {0x00E390D8u, 0x00E390B8u, 0x00E390D0u, 0x00E390CCu, 0x00E390C4u,
                                        0x00E390C8u, 0x00E39094u, 0x00E390BCu, 0x00E390D4u, 0x00E390B4u,
                                        0x00E390B0u, 0x00E390A8u, 0x00E390ACu, 0x00E390A0u, 0x00E390A4u,
                                        0x00E3909Cu, 0x00E39098u, 0x00E390C0u, 0x00E39090u};
    uint32_t a = (uint32_t) n <= 0x12u ? handle[n] : 0x00E390D8u;
    U32(st, 0x00E390DCu) = U32(st, a);
}

static uint32_t hovered_offset(game_state *st)
{
    /* the hovered tile's map offset: y * 228 + x + map start 0x00BCA2D8 */
    return (uint32_t) (S32(st, 0x00BCA2CCu) * 228 + S32(st, 0x00BCA2C8u) + S32(st, 0x00BCA2D8u));
}

static int32_t cursor_attack_target(game_state *st, int32_t f)
{
    /* @bin 0x004D6AA0 a figure that can attack (0x00D05708 set, figure +0xF8 clear) over a tile with
     * an enemy (0x004B5520) */
    if (S32(st, 0x00D05708u) == 0 || U8(st, 0x00BCD728u + (uint32_t) f * 0x184u) != 0) {
        return 0;
    }
    return GAME_CALL(st, 0x004B5520u, (int32_t) hovered_offset(st)) != 0;
}

static int32_t cursor_move_target(game_state *st, int32_t f)
{
    /* @bin 0x004D6B00 a figure that moves (0x00D05708 set, figure +0xF8 set) over a tile whose
     * terrain (0x00AF598C) has no bit 4 */
    if (S32(st, 0x00D05708u) == 0 || U8(st, 0x00BCD728u + (uint32_t) f * 0x184u) == 0) {
        return 0;
    }
    return (U32(st, 0x00AF598Cu + 4 * hovered_offset(st)) & 4) != 4;
}

static int32_t run_attack_target(game_state *st, const int32_t *a) { return cursor_attack_target(st, a[0]); }
static int32_t run_move_target(game_state *st, const int32_t *a) { return cursor_move_target(st, a[0]); }

/* @bin 0x004D6CF0 the cursor of the frame's window; -1: no change */
static int32_t frame_cursor_choice(game_state *st)
{
    uint8_t w = U8(st, WINDOW);
    if (w == 0x1E) { /* formation orders */
        int32_t enemy = GAME_CALL(st, 0x004B5640u, S32(st, 0x00BCA2C4u));
        uint32_t rec = FORMATION + (uint32_t) S32(st, FORMATION) * 0x90u;
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
        return S32(st, 0x006059E8u) == 9 ? 2 : 0;
    }
    if (w == 0x1F) { /* @bin 0x004D6DA2 a ship or a transport chosen (0x00D05700) */
        int32_t f = S32(st, 0x00D05700u);
        if (f <= 0) {
            frame_cursor(st, 0);
        }
        /* @bin 0x004D7058 by figure type 9..20 */
        switch (U8(st, FIGURE_TYPE + (uint32_t) f * 0x184u)) {
        case 9:
            if (cursor_attack_target(st, f)) {
                return 5;
            }
            return cursor_move_target(st, f) ? 4 : 0x12;
        case 10: return 0x10;
        case 11:
            if (S32(st, 0x00D05708u) != 0) {
                return 0x12;
            }
            if (cursor_attack_target(st, f)) {
                return 5;
            }
            return cursor_move_target(st, f) ? 4 : -1;
        case 12: return cursor_attack_target(st, f) ? 5 : 0x12;
        case 13: return cursor_move_target(st, f) ? 4 : 0x12;
        case 20: return S32(st, 0x00D05708u) != 0 ? 6 : -1;
        default: return 3;
        }
    }
    if (w == 0x20) {
        if (GAME_CALL(st, 0x004B5640u, S32(st, 0x00BCA2C4u)) != 0) {
            return 9;
        }
        int32_t f = S32(st, 0x00D05704u);
        if (f <= 0) {
            frame_cursor(st, 0);
        }
        switch (U8(st, FIGURE_TYPE + (uint32_t) f * 0x184u)) {
        case 0x0A: return 0x11;
        case 0x14: return S32(st, 0x00D0570Cu) != 0 ? 7 : -1;
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
    int32_t mx = S32(st, 0x00E38E34u);
    int32_t vx = S32(st, 0x0070E490u);
    if (mx < vx || mx >= (S32(st, 0x0070E458u) + 2) * S32(st, 0x0070E464u) + vx) {
        return 0;
    }
    int32_t my = S32(st, 0x00E38E30u);
    int32_t vy = S32(st, 0x0070E48Cu);
    int32_t th = S32(st, 0x0070E44Cu);
    if (my < vy || my >= th * S32(st, 0x0070E460u) + vy) {
        return 0;
    }
    my -= vy;
    mx -= vx;
    int32_t row = my / th;
    S32(st, 0x0070E404u) = row;
    int32_t tw = S32(st, 0x0070E45Cu) + 1;
    int32_t col = mx / tw;
    int32_t col_odd = col & 1;
    int32_t row_odd = row & 1;
    int32_t parity = (row + col) & 1;
    int32_t half = (mx % tw) / 2;     /* @bin 0x0043084B cdq; sub; sar 1: toward 0 */
    int32_t in_row = my % S32(st, 0x0070E44Cu);
    int32_t x = col / 2;
    S32(st, 0x0070E408u) = x;
    if (parity == 0) {
        if (in_row > half) {
            row++;
            S32(st, 0x0070E404u) = row;
        } else if (col_odd != 0 && row_odd != 0) {
            x++;
            S32(st, 0x0070E408u) = x;
        }
    } else if (parity == 1) {
        if (in_row + half >= S32(st, 0x0070E44Cu) - 1) {
            row++;
            S32(st, 0x0070E404u) = row;
            if (col_odd != 0 && row_odd == 0) {
                x++;
                S32(st, 0x0070E408u) = x;
            }
        }
    }
    int32_t r = S32(st, 0x0070E474u) + row;
    int32_t odd = row & 1;
    /* @bin 0x004308B7 rows of 0xE7 entries */
    int32_t entry = S32(st, 0x0070E494u + 4u * (uint32_t) (r * 0xE7 + S32(st, 0x0070E478u) + x));
    S32(st, 0x0070E3F8u) = entry;
    if (entry >= 0x0FFF0000) {
        return 0;
    }
    S32(st, 0x0070E3FCu) = 0;
    S32(st, 0x0070E400u) = 0;
    if (row == 0) {
        S32(st, 0x0070E3FCu) = 2;
    } else if (row >= S32(st, 0x0070E460u)) {
        S32(st, 0x0070E3FCu) = 1;
    }
    if (odd != 0) {
        if (x == 0) {
            S32(st, 0x0070E400u) = 2;
        } else if (x >= S32(st, 0x0070E464u)) {
            S32(st, 0x0070E400u) = 1;
        }
    }
    return entry;
}

void frame_hover_tile(game_state *st)
{
    /* @bin 0x00405F80 */
    S32(st, 0x00BCA2CCu) = 0;
    S32(st, 0x00BCA2C8u) = 0;
    int32_t t = frame_screen_to_tile(st);
    S32(st, 0x00BCA2C4u) = t;
    if (t == 0) {
        return;
    }
    int32_t d = t - S32(st, 0x00BCA2D8u);
    int32_t x = d % 228;
    int32_t y = d / 228;
    S32(st, 0x00BCA2C8u) = x;
    S32(st, 0x00BCA2CCu) = y;
    if (U8(st, 0x00605A5Cu) != 0) { /* @bin 0x00605A5C 'M' freezes the hovered tile */
        return;
    }
    S32(st, 0x00BCA300u) = t;
    S32(st, 0x00BCA304u) = x;
    S32(st, 0x00BCA308u) = y;
    /* @bin 0x00405F60 building type 0xB7 is being placed: 0x004ED330 */
    if (S32(st, 0x006059E8u) == 0xB7) {
        GAME_CALL0(st, 0x004ED330u);
    }
}

static int32_t run_hover_0x00405F60(game_state *st, const int32_t *a)
{
    (void) a;
    if (S32(st, 0x006059E8u) == 0xB7) {
        GAME_CALL0(st, 0x004ED330u);
    }
    return 0;
}

/* ---- present ---- */

static int32_t present_elapsed(game_state *st)
{
    /* @bin 0x004D0040 ms since the last call (timeGetTime; last 0x00E36C88) */
    uint32_t now = platform_clock_ms(st);
    uint32_t d = now - U32(st, 0x00E36C88u);
    U32(st, 0x00E36C88u) = now;
    return (int32_t) d;
}

void frame_present(game_state *st, int32_t force)
{
    /* @bin 0x004D0280 */
    if (S32(st, 0x005E06B0u) == 1) { /* DirectDraw not ready */
        return;
    }
    if (S32(st, 0x00F54054u) != 0 && S32(st, 0x00E92E04u) != 0) { /* a video plays */
        return;
    }
    int32_t shake = S32(st, 0x0078D7A0u);
    if (U8(st, WINDOW) != 5 && shake > 0 && shake % 0x88 == 0) {
        /* @bin 0x004D035E the fade (0x004D05F0, eight steps of 0x004D0650 on the object 0x006DE518),
         * each step presented */
        GAME_CALLT(st, 0x006DE518u, 0x004D05F0u, 1, 0x0F, 0, 1, 0);
        GAME_CALL0(st, 0x004D03F0u);
        for (int i = 0; i < 8; i++) {
            GAME_CALLT(st, 0x006DE518u, 0x004D0650u, 1);
            GAME_CALL0(st, 0x004D03F0u);
        }
        return;
    }
    int32_t d = present_elapsed(st);
    S32(st, 0x00E92E08u) += d;
    if (force == 1) {
        S32(st, 0x00E92E08u) = 50;
    }
    /* @bin 0x004D02F5 every 50 ms, or when 0x00E38DC8 asks */
    if (U8(st, PRESENT_NOW) == 0 && S32(st, 0x00E92E08u) < 50) {
        return;
    }
    uint8_t w = U8(st, WINDOW);
    S32(st, 0x00E92E08u) = 0;
    U8(st, PRESENT_NOW) = 0;
    /* @bin 0x004D039C the city windows 1, 3, 0x1E, 0x1F, 0x20, 0x49 redraw the sidebar first while
     * 0x0078D7A0 > 0 */
    if (w != 5 && S32(st, 0x0078D7A0u) > 0 &&
        (w == 1 || w == 3 || w == 0x1E || w == 0x1F || w == 0x20 || w == 0x49)) {
        GAME_CALL0(st, 0x0051E990u);
        GAME_CALLT(st, 0x006DE518u, 0x004D05F0u, 1, 1, 0, 0, 1);
    }
    GAME_CALL0(st, 0x004D03F0u);
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
    S32(st, 0x00E390ECu) += 1;                       /* the frame counter */
    U32(st, FRAME_CLOCK) = platform_clock_ms(st);  /* timeGetTime */
    frame_timers(st);
    int32_t n = frame_pacing(st);
    for (int32_t i = 0; i < n; i++) {
        S32(st, 0x00E94DBCu) += 1;
        game_tick(st, st->env);
        /* @bin 0x004D6BA5 fast-forward: up to n ticks this frame, until a right button up is
         * pending (PeekMessageA, PM_NOREMOVE); then the pending mouse and key messages are dropped */
        if (U8(st, FAST_FORWARD) == 0) {
            n = 0;
        } else if (frame_peek_message(st, 0x205, 0x205, 0)) {
            while (frame_peek_message(st, 0x200, 0x209, 1)) {
            }
            while (frame_peek_message(st, 0x100, 0x108, 1)) {
            }
            U8(st, FAST_FORWARD) = 0;
            n = 0;
        } else if (U8(st, FAST_FORWARD) == 0) {
            n = 0;
        }
        frame_autosave(st);
    }
    frame_mouse(st);
    GAME_CALL(st, 0x004D71C0u, 0);   /* the window's background, when REDRAW */
    if (S32(st, 0x00E94D98u) != 0) { /* @bin 0x004D6070 */
        U8(st, REDRAW) = 1;
    }
    GAME_CALL0(st, 0x004D75D0u);     /* the window's per-frame draw */
    GAME_CALL0(st, 0x004D8310u);     /* the window's input */
    frame_clear_keys(st);
    GAME_CALL0(st, 0x004C8750u);     /* tooltips */
    GAME_CALL0(st, 0x004C9750u);
    GAME_CALL(st, 0x0051F0A0u, 0);   /* the top menu bar */
    GAME_CALL(st, 0x0051F680u, 1);
    GAME_CALL0(st, 0x004E2720u);     /* on-screen messages */
    if (S32(st, 0x00D3A6E4u) != 0) {
        GAME_CALL0(st, 0x004CABD0u);
        if (S32(st, 0x00D3A6C0u) == 0x23) {
            frame_hover_tile(st);
        }
        GAME_CALL0(st, 0x004CA930u);
    }
    GAME_CALL0(st, 0x00536620u);
    /* @bin 0x004D6CB7 a Bink video (0x006FDC40) is played instead of the present */
    if (S32(st, 0x006FDC40u) != 0) {
        if ((uint8_t) GAME_CALL(st, 0x005605A0u, S32(st, 0x006FDC38u)) != 0) {
            GAME_CALL0(st, 0x00416BC0u);
        }
    }
    if (S32(st, 0x006FDC40u) == 0) {
        frame_present(st, 0);
    }
    GAME_CALL0(st, 0x0041D560u);     /* the ambient-sound slots */
    int32_t c = frame_cursor_choice(st);
    if (c >= 0) {
        frame_cursor(st, c);
    }
    /* @bin 0x004D6ECF music: battle music while the city has more than 15 enemies (city +0x634) or
     * more than 4 (+0x9A0) */
    uint32_t city = U32(st, PLAYER) * CITY_SIZE;
    if (S32(st, city + 0x00EA5EA4u) <= 0x0F && S32(st, city + 0x00EA6210u) <= 4) {
        if (S32(st, 0x00E92BF0u) == 1) {
            S32(st, 0x00F55B8Cu) = 1;
            S32(st, 0x00E92BF0u) = 0;
        }
        if (GAME_CALLT(st, SOUND, 0x0055F5A1u, 0) == 0 && S32(st, 0x00F55B94u) == 0) {
            if (U8(st, WINDOW) != 1) {
                goto ambient;
            }
            GAME_CALLT0(st, 0x00E39180u, 0x00539820u); /* the next tune */
        }
    } else if (S32(st, 0x00F55B88u) != 0 && S32(st, 0x00E92BF0u) == 0 && U8(st, 0x00E38E64u) != 0) {
        GAME_CALLT(st, SOUND, 0x0055F449u, (int32_t) 0x005E1DE0u, 0, 0); /* "audio\music\battle.mp3" */
        S32(st, 0x00E92BF0u) = 1;
    }
    if (U8(st, WINDOW) == 1) {
        /* @bin 0x004D6F7C the per-frame CRT rand draw of the city view: a random city sound when
         * rand() * 50 / 32768 == 1 */
        int32_t r = crt_rand(st);
        if ((r * 50) / 32768 == 1) {
            GAME_CALL0(st, 0x0053ACF0u);
        }
    }
ambient:
    if (S32(st, 0x0078D79Cu) == 0 && S32(st, 0x0078D7A0u) == 0 && S32(st, 0x0078D7A8u) == 0) {
        if (S32(st, SCROLLING) != 0) {
            GAME_CALL(st, 0x0053B6F0u, 2);
        } else {
            uint8_t w = U8(st, WINDOW);
            if (w == 1 || w == 9 || w == 5) {
                GAME_CALL0(st, 0x0053B960u);
            } else if (GAME_CALLT(st, SOUND, 0x0055F5A1u, 2) != 0) {
                GAME_CALLT(st, SOUND, 0x0055F561u, 2);
            }
        }
    }
    if (S32(st, 0x00F55B8Cu) != 0) {
        GAME_CALL(st, 0x0053B6F0u, 0);
    }
    if (S32(st, 0x00F55B90u) != 0) {
        GAME_CALL(st, 0x0053B6F0u, 1);
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
    if (S32(st, 0x00E94D98u) != 0) {
        U8(st, REDRAW) = 1;
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
