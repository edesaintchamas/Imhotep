/* Imhotep - the frame's simulation side (roadmap phase 7.1, docs/presentation.md 1, 5).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The per-frame function 0x004D6B60 and its non-drawing parts: the frame clock and counter, the
 * animation timers, the pacing (a tick is due or not), the tick loop with the fast-forward, the
 * autosave after a tick, the mouse sampling, the hovered tile, the cursor choice, the present's
 * counters, the per-frame CRT rand draw and the music / ambient requests. The drawing, the input
 * dispatch, the tooltips, the top bar and the sound logic are other packages' functions: the frame
 * calls them through GAME_CALL (replayed in the comparisons until they are ported).
 *
 * The window procedure 0x00414CD0 (core/frame/wndproc.c) is how the platform delivers input: one
 * call per window message, as Windows would, before the frame. */
#ifndef IMHOTEP_FRAME_FRAME_H
#define IMHOTEP_FRAME_FRAME_H

#include <stdint.h>

#include "state/state.h"

/* @bin 0x004D6B60 one frame: clock, timers, ticks, mouse, drawing, input dispatch, present, sound */
void frame_run(game_state *st);

/* @bin 0x0040F270 the animation timers: flag k (0..50) when 20*k ms have passed since its last flag */
void frame_timers(game_state *st);
/* @bin 0x004DDCB0 the pacing: 10000 when a tick is due (the clock is past the speed's delay and the
 * game is not paused, scrolling, dragging or in a window that stops time), else 0 */
int32_t frame_pacing(game_state *st);
/* @bin 0x004CF210 the mouse of this frame: cursor position, scaled position, button edges */
void frame_mouse(game_state *st);
/* @bin 0x004CF1F0 the one-frame key and click flags are cleared */
void frame_clear_keys(game_state *st);
/* @bin 0x004CF460 the cursor shape n (0..18): its handle is kept in 0x00E390DC */
void frame_cursor(game_state *st, int32_t n);
/* @bin 0x00405F80 the hovered tile from the scaled mouse position */
void frame_hover_tile(game_state *st);
/* @bin 0x00430790 the map offset under the scaled mouse position in the city view (0: none) */
int32_t frame_screen_to_tile(game_state *st);
/* @bin 0x004D0280 the present: the back buffer goes to the screen every 50 ms (or when forced) */
void frame_present(game_state *st, int32_t force);

/* @bin 0x004D5C30 the autosave after a tick (once per city flag) */
void frame_autosave(game_state *st);
/* @bin 0x004D4E20 save the game to the player file `path` (a path the game builds): 0, 1 when the
 * file cannot be written */
int32_t frame_save_game(game_state *st, const char *path);

/* @bin 0x00414CD0 the window procedure: the state effects of one window message (msg, wparam,
 * lparam; hwnd: the window, kept by WM_CREATE) as the original's WndProc makes them; returns the
 * message's result (0: DefWindowProcA, as the oracle answers it) */
int32_t frame_wndproc(game_state *st, uint32_t hwnd, uint32_t msg, uint32_t wparam, uint32_t lparam);

/* the inputs the original reads from Windows during a frame (GetCursorPos, GetAsyncKeyState,
 * PeekMessageA): see core/frame/input.c */
void frame_cursor_pos(game_state *st, int32_t *x, int32_t *y);
int16_t frame_key_state(game_state *st, int32_t vk);
int32_t frame_peek_message(game_state *st, uint32_t first, uint32_t last, int32_t remove);

#endif
