/* Imhotep - the Windows inputs the frame and the window procedure read (GetCursorPos,
 * GetAsyncKeyState, PeekMessageA).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * They are platform inputs (core/sim/platform.h: platform_cursor_pos, platform_key_state,
 * platform_message_pending). Without a platform, or when it does not give them, they answer:
 * - the cursor is where the last mouse message put it (i16 0x00E38E0A / 0x00E38E08, written by the
 *   window procedure from WM_MOUSEMOVE and the button messages). In the fullscreen game (window at
 *   0,0) GetCursorPos returns that same point, as the oracle's frames deliver a WM_MOUSEMOVE for
 *   every cursor move (tools/oracle/framestep.py); @unverified for a windowed game, where
 *   GetCursorPos is in screen coordinates;
 * - no key is held (GetAsyncKeyState 0);
 * - no message is pending (PeekMessageA 0), as in the oracle (tools/oracle/game.py). */
#include "frame/frame.h"

#include "sim/platform.h"
#include "state/layout.h"

void frame_cursor_pos(game_state *st, int32_t *x, int32_t *y)
{
    if (platform_cursor_pos(st, x, y)) {
        return;
    }
    /* @unverified fallback: the last mouse message's point (see above) */
    *x = (int16_t) S16(st, G_MSG_MOUSE_X);
    *y = (int16_t) S16(st, G_MSG_MOUSE_Y);
}

int16_t frame_key_state(game_state *st, int32_t vk)
{
    return platform_key_state(st, vk);
}

int32_t frame_peek_message(game_state *st, uint32_t first, uint32_t last, int32_t remove)
{
    return platform_message_pending(st, first, last, remove);
}
