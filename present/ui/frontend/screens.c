/* Imhotep - the front end's other screens: the title picture (window 0x3C) and the resolution check
 * (0x3E) (roadmap phase 7.6).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * Window 0x3C (after the videos of 0x44 -> 0x41 at boot): the full redraw 0x00520E00. Window 0x3E
 * (after a change of resolution, back to 0x0070570C): the full redraw 0x00534540, per frame
 * 0x005345A0; input (0x004D8310): 0x004261D0 on 0x005C68A8 (0x00402E10 keep), after 10 s of the
 * clock (0x006057E8) 0x00402DE0 (back to the old resolution). */
#include "frontend.h"

#include "present_layout.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

void title_screen_draw(game_state *st)
{
    /* @bin 0x00520E00 window 0x3C: the setup music (as the family's menu), the screen cleared, the
     * picture 0x00F55BEC, the text (group 0xD, 7) centred in 320 pixels at the bottom ((0x00E38D9C -
     * 320) / 2, small font 0x00F56F84, white) */
    if (S32(st, G_SOUND_ON) != 0 && GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAYING, 0) == 0 && U8(st, G_MUSIC_ON) != 0) {
        GAME_CALLT(st, G_SOUND, FN_PLATFORM_STREAM_PLAY, G_STR_SETUP_MUSIC, 0, 0);
    }
    GAME_CALL0(st, FN_SCREEN_BUFFER_CLEAR);
    GAME_CALLT(st, G_GRAPHICS_GFX, FN_IMAGE_DRAW, S32(st, G_TITLE_IMAGE), S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
    int32_t dx = S32(st, G_LAYOUT_DX), dy = S32(st, G_LAYOUT_DY);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x0D, 7, (S32(st, G_UE38D9C) - 0x140) / 2, dy + 0x1CE, 0x140, S32(st, G_FONT_SMALL),
              0xFFFF, dx, dy);
}

void resolution_check_draw(game_state *st)
{
    /* @bin 0x00534540 window 0x3E: the screen cleared, the panel (30 x 10), the question (group 0x92,
     * 0, in lines of 0x1A0) */
    int32_t x = S32(st, G_LAYOUT_DX) + 0x50;
    int32_t y = S32(st, G_LAYOUT_DY) + 0x40;
    GAME_CALL0(st, FN_SCREEN_BUFFER_CLEAR);
    GAME_CALL(st, FN_PANEL_DRAW, x, y, 0x1E, 10);
    GAME_CALL(st, FN_LAYOUT_GROUP_LINES, 0x92, 0, x + 0x20, y + 0x1E, 0x1A0, S32(st, G_FONT_LARGE), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

void resolution_check_frame(game_state *st)
{
    /* @bin 0x005345A0 the keep button: a frame (160 x 32, lit when hovered) and its text (group 0x12, 3) */
    int32_t dy0 = S32(st, G_LAYOUT_DY), dx0 = S32(st, G_LAYOUT_DX);
    int32_t lit = S32(st, G_HOVER) == 1;
    int32_t x = dx0 + 0xF0, y = dy0 + 0x40;
    GAME_CALL(st, FN_LAYOUT_FRAME, 0, x, y + 0x70, 0xA0, 0x20, lit, dx0, dy0);
    GAME_CALL(st, FN_LAYOUT_GROUP_CENTRED, 0x12, 3, x, y + 0x7B, 0xA0, S32(st, G_FONT_LARGE), 0, S32(st, G_LAYOUT_DX), S32(st, G_LAYOUT_DY));
}

void resolution_revert(game_state *st)
{
    /* @bin 0x00402DE0 PostMessageA(window 0x00E39140, WM_KEYDOWN, VK_F7, 0) (the window procedure
     * sets the resolution back later), a full redraw, back to the window 0x0070570C */
    platform_post_message(st, 0x100, 0x76, 0);
    U8(st, G_REDRAW) = 1;
    U8(st, G_WINDOW) = U8(st, G_POPUP_BACK);
}

void resolution_keep(game_state *st)
{
    /* @bin 0x00402E10 keep: the resolution 0x00E38E68 becomes the saved one 0x00E38EE4; back to the
     * window 0x0070570C */
    S32(st, G_RESOLUTION_SAVED) = S32(st, G_RESOLUTION);
    U8(st, G_WINDOW) = U8(st, G_POPUP_BACK);
    U8(st, G_REDRAW) = 1;
}

#define RUN0(f) static int32_t run_##f(game_state *st, const int32_t *a) { (void) a; f(st); return 0; }
RUN0(title_screen_draw)
RUN0(resolution_check_draw)
RUN0(resolution_check_frame)
RUN0(resolution_revert)
RUN0(resolution_keep)

const ported_function frontend_screens_ports[] = {
    {0x00520E00u, "title_screen_draw", 0, 0, 0, run_title_screen_draw, 0},
    {0x00534540u, "resolution_check_draw", 0, 0, 0, run_resolution_check_draw, 0},
    {0x005345A0u, "resolution_check_frame", 0, 0, 0, run_resolution_check_frame, 0},
    {0x00402DE0u, "resolution_revert", 0, 0, 0, run_resolution_revert, 0},
    {0x00402E10u, "resolution_keep", 0, 0, 0, run_resolution_keep, 0},
};
const int frontend_screens_ports_count = sizeof(frontend_screens_ports) / sizeof(frontend_screens_ports[0]);
