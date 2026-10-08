/* Imhotep - the display mode and the window: DirectDraw's set-up and release, the resolution, the
 * window procedure's WM_MOVE / WM_SIZE / WM_ACTIVATEAPP, and WinMain's end (package play).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * The original keeps DirectDraw's objects in the state as COM pointers (0x00E39148 the DirectDraw
 * object, 0x00E39178 the primary surface, 0x00E39174 the back buffer, 0x00E39170 the clipper) and the
 * back buffer's memory at 0x00E92E18 (docs/presentation.md 2.1). Here DirectDraw is the platform's:
 * the set-up 0x004D0060 makes the mode change through platform_set_display (core/sim/platform.h),
 * which gives the new back buffer; the COM pointers hold handles of the platform's (non-zero
 * constants below: nothing in the game reads them but DirectDraw calls, which are the platform's).
 * The set-up is a partial port: without a platform that sets display modes (the oracle comparisons)
 * it is replayed from the original's run (its surfaces are the emulator's). Everything around it
 * (release, resolution globals, window rectangle, clearing, the re-layout of the screen buffers) is
 * ported and checked against the original. */
#include <string.h>

#include "frame/display.h"
#include "frame/frame.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define WINDOW 0x00E38DDAu
#define REDRAW 0x00E38DCAu
#define HWND 0x00E39140u
#define FULLSCREEN 0x00E38E5Cu   /* i32 the option: fullscreen (1) or windowed */
#define RESOLUTION 0x00E38E68u   /* i32 1: 640 x 480, 2: 800 x 600, 3: 1024 x 768 */
#define SCREEN_W 0x00E38DB0u     /* the mode's width and height */
#define SCREEN_H 0x00E38DACu
#define DESKTOP_W 0x00E38DB8u    /* the screen's size (GetSystemMetrics, WM_DISPLAYCHANGE) */
#define DESKTOP_H 0x00E38DB4u
#define DD_OBJECT 0x00E39148u
#define DD_CLIPPER 0x00E39170u
#define DD_BACK 0x00E39174u
#define DD_PRIMARY 0x00E39178u
#define BACK_PIXELS 0x00E92E18u
#define FORMAT 0x00E92DFCu       /* 0: RGB 555, 2: RGB 565 (1, 3: 556, 655, never kept) */
#define FORMAT_NAME 0x00E3914Cu  /* 0x555, 0x565, 0x556, 0x655 */
#define DD_READY 0x00E390E0u     /* the set-up succeeded */
#define CLIENT_RECT 0x00E39130u  /* left, top, right, bottom of the client area on the screen */
#define LOG 0x004CE6E0u

/* the platform's DirectDraw objects: handles, not memory (see above) */
#define HANDLE_DDRAW 0x00DD0001u
#define HANDLE_PRIMARY 0x00DD0002u
#define HANDLE_CLIPPER 0x00DD0003u

static void log_msg(game_state *st, uint32_t message, int32_t value)
{
    GAME_CALL(st, LOG, (int32_t) message, 0, value);
}

uint32_t display_memory(game_state *st, uint32_t address, uint32_t size)
{
    /* the original's new / malloc / delete / free of blocks that hold no simulation data (the other
     * heap of the oracle): the platform's memory (sim_platform.memory). A block of the simulation
     * heap is released as the original's free does (state_dealloc) */
    const sim_platform *p = st->platform;
    if (size == 0 && address >= STATE_HEAP_BASE && address - STATE_HEAP_BASE < st->heap_used) {
        state_dealloc(st, address);
        return 0;
    }
    if (!p || !p->memory) {
        return 0;
    }
    return p->memory(p->ctx, address, size);
}

int display_has_memory(game_state *st)
{
    const sim_platform *p = st->platform;
    return p && p->memory;
}

/* ---- the resolution ---- */

void display_resolution_set(game_state *st, int32_t mode)
{
    /* @bin 0x004CFD60 1: 640 x 480, 2: 800 x 600, 3: 1024 x 768 into 0x00E38DB0 / 0x00E38DAC; other
     * values change nothing */
    if (mode == 1) {
        S32(st, SCREEN_W) = 0x280;
        S32(st, SCREEN_H) = 0x1E0;
    } else if (mode == 2) {
        S32(st, SCREEN_W) = 0x320;
        S32(st, SCREEN_H) = 0x258;
    } else if (mode == 3) {
        S32(st, SCREEN_W) = 0x400;
        S32(st, SCREEN_H) = 0x300;
    }
}

void display_mode_set(game_state *st, int32_t mode)
{
    /* @bin 0x004CFDC0 the resolution `mode` and the globals made from it: the drawing size 0x00E38D9C /
     * 0x00E38D98 / 0x00E38D90, the 640 x 480 layout's origin 0x00E38DA8 / 0x00E38DA4 ((size - 640) / 2,
     * (size - 480) / 2), the pitch 0x00E38D94 and buffer size 0x00E38D8C, the clip rectangle
     * 0x00D3B550 .. 0x00D3B55C, the windowed window's place 0x00E390E8 / 0x00E390E4 (centred on the
     * screen 0x00E38DB8 / 0x00E38DB4). The window itself is resized (SetRect, AdjustWindowRectEx,
     * SetWindowPos, UpdateWindow, SetFocus): the platform's, with the display mode */
    display_resolution_set(st, mode);
    S32(st, RESOLUTION) = mode;
    int32_t w = S32(st, SCREEN_W), h = S32(st, SCREEN_H);
    S32(st, 0x00E38D9Cu) = w;
    S32(st, 0x00E38DA8u) = (w - 0x280) / 2;
    S32(st, 0x00E38DA4u) = (h - 0x1E0) / 2;
    S32(st, 0x00E38D94u) = w + w;
    S32(st, 0x00E38D8Cu) = (w + w) * h;
    S32(st, 0x00E38D98u) = h;
    S32(st, 0x00E38D90u) = h;
    S32(st, 0x00D3B55Cu) = 0;
    S32(st, 0x00D3B558u) = w;
    S32(st, 0x00D3B554u) = 0;
    S32(st, 0x00D3B550u) = h;
    S32(st, 0x00E390E8u) = (S32(st, DESKTOP_W) - S32(st, SCREEN_W)) / 2;
    S32(st, 0x00E390E4u) = (S32(st, DESKTOP_H) - S32(st, SCREEN_H)) / 2;
}

void display_buffers_layout(game_state *st)
{
    /* @bin 0x00414BD0 after a resolution change: the sidebar's x (0x0051E930), the external image
     * buffer 0x01265170 released (0x0053F790), the spare screen 0x00E92E1C made again (free, malloc of
     * 2 * width * height bytes, at least 0x124F80), the view's geometry (0x004DDEC0 with the sidebar
     * collapsed 0x00F53F78 == 1, else 0x004DDF20), its bounds 0x004DDF90 and the camera clamp
     * 0x004DE080. The spare screen is the platform's memory (display_memory) */
    if (!display_has_memory(st)) {
        /* the spare screen is a block of the oracle's other heap: replayed (a partial port) */
        state_unported(st, 0x00414BD0u);
        return;
    }
    GAME_CALL0(st, 0x0051E930u);
    GAME_CALL(st, 0x0053F790u, 0x01265170, 1);
    if (U32(st, 0x00E92E1Cu) != 0) {
        display_memory(st, U32(st, 0x00E92E1Cu), 0); /* free 0x00561BDC */
    }
    int32_t n = S32(st, SCREEN_H) * S32(st, SCREEN_W) * 2;
    if (n <= 0x124F80) {
        n = 0x124F80;
    }
    U32(st, 0x00E92E1Cu) = display_memory(st, 0, (uint32_t) n); /* malloc 0x00561B68 */
    if (S32(st, 0x00F53F78u) == 1) {
        GAME_CALL0(st, 0x004DDEC0u);
    } else {
        GAME_CALL0(st, 0x004DDF20u);
    }
    GAME_CALL0(st, 0x004DDF90u);
    GAME_CALL0(st, 0x004DE080u);
}

void display_resolution_check(game_state *st)
{
    /* @bin 0x00402DA0 a resolution above the one confirmed (0x00E38EE4) and above 640 x 480 opens the
     * confirmation window 0x3E (the window before it kept in 0x0070570C, the clock (timeGetTime) in
     * 0x006057E8) */
    int32_t mode = S32(st, RESOLUTION);
    if (mode > S32(st, 0x00E38EE4u) && mode > 1) {
        U32(st, 0x006057E8u) = platform_clock_ms(st);
        U32(st, 0x0070570Cu) = U8(st, WINDOW);
        U8(st, WINDOW) = 0x3E;
        U8(st, REDRAW) = 1;
    }
}

/* ---- DirectDraw: release, set-up ---- */

int32_t display_surfaces_release(game_state *st, int32_t all)
{
    /* @bin 0x00426EE0 the clipper, the back buffer and the primary surface released (Release: the
     * platform's), their pointers and the back buffer's memory 0x00E92E18 cleared; with `all` the
     * DirectDraw object too ("OK :DD Released." 0x005D1D00) */
    U32(st, DD_CLIPPER) = 0;
    U32(st, DD_BACK) = 0;
    U32(st, DD_PRIMARY) = 0;
    U32(st, BACK_PIXELS) = 0;
    if (all != 0 && U32(st, DD_OBJECT) != 0) {
        U32(st, DD_OBJECT) = 0;
        log_msg(st, 0x005D1D00u, 0);
    }
    return 1;
}

void display_end(game_state *st, int32_t all)
{
    /* @bin 0x004D01E0 the object 0x00E92E00 released (Release, the platform's; never set in the runs
     * seen), then the surfaces (0x00426EE0) when DirectDraw is set up */
    if (U32(st, 0x00E92E00u) != 0) {
        U32(st, 0x00E92E00u) = 0;
    }
    if (U32(st, DD_READY) != 0) {
        display_surfaces_release(st, all);
        U32(st, DD_READY) = 0;
    }
}

int32_t display_clear(game_state *st)
{
    /* @bin 0x00426F60 UpdateWindow, then the back buffer and the primary surface filled with colour 0
     * (Blt DDBLT_COLORFILL | DDBLT_WAIT, dwFillColor 0): the back buffer's pixels when the state maps
     * them, the screen is the platform's. Blt does not fail here: 1 (the oracle's Blt fills the
     * surfaces the same way: tools/oracle/game.py) */
    int32_t pitch = S32(st, 0x00E38D94u), h = S32(st, 0x00E38D98u);
    uint32_t a = U32(st, BACK_PIXELS);
    if (a != 0 && pitch > 0 && h > 0) {
        uint8_t *p = state_ptr(st, a, (uint32_t) (pitch * h));
        if (p) {
            memset(p, 0, (size_t) pitch * (size_t) h);
        }
    }
    return 1;
}

static int has_display_platform(game_state *st)
{
    const sim_platform *p = st->platform;
    return p && p->set_display;
}

static int32_t dd_create(game_state *st)
{
    /* @bin 0x00426B40 (native with a platform): the display's depth (GetDeviceCaps BITSPIXEL * PLANES:
     * 16, the platform's display) into 0x00E38E58; the screen's size (GetSystemMetrics) is the
     * platform's current mode, which 0x00E38DB8 / 0x00E38DB4 hold (WM_DISPLAYCHANGE); a mode wider than
     * the screen, or a depth other than 16 bits, is shown fullscreen (the notice 0x39 when windowed);
     * GetPaletteEntries(0, ...) fails without writing. The DirectDraw object (DirectDrawCreate,
     * SetCooperativeLevel) is the platform's; SetDisplayMode is done by the back buffer's request
     * (platform_set_display) and does not fail here (the original retries 800 x 600, then 640 x 480,
     * when it fails: 0x004CFDC0, 0x00414BD0, 0x004E2AB0). Windowed: the window's place 0x00E390E8 /
     * 0x00E390E4 centred on the screen, the window calls the platform's ("Kept resolution") */
    S32(st, 0x00E38E58u) = 16;
    if (S32(st, SCREEN_W) > S32(st, DESKTOP_W)) {
        if (S32(st, FULLSCREEN) == 0) {
            GAME_CALL(st, 0x004E2970u, 0x39, 0, 0);
        }
        S32(st, FULLSCREEN) = 1;
    } else if (S32(st, FULLSCREEN) == 0 && S32(st, 0x00E38E58u) != 0x10) {
        GAME_CALL(st, 0x004E2970u, 0x39, 0, 0);
        log_msg(st, 0x005D1CD4u, 0);
        S32(st, FULLSCREEN) = 1;
    }
    int first = U32(st, DD_OBJECT) == 0;
    if (first) {
        log_msg(st, 0x005D1C74u, 0); /* "OK :DD SetCooperative Level." */
    }
    if (S32(st, FULLSCREEN) != 0) {
        log_msg(st, 0x005D1C40u, 0); /* "OK :DD Set resolution." */
    } else {
        S32(st, 0x00E390E8u) = (S32(st, DESKTOP_W) - S32(st, SCREEN_W)) / 2;
        S32(st, 0x00E390E4u) = (S32(st, DESKTOP_H) - S32(st, SCREEN_H)) / 2;
        log_msg(st, 0x005D1C28u, 0); /* "OK :DD Kept resolution." */
    }
    U32(st, DD_OBJECT) = first ? HANDLE_DDRAW : U32(st, DD_OBJECT);
    return 1;
}

static int32_t dd_surfaces(game_state *st)
{
    /* @bin 0x00426FF0 (native with a platform): the primary surface (a handle of the platform's; its
     * test lock "can lock primary", 0x00E92E04 := 0), the back buffer of 0x00E38D9C x 0x00E38D98
     * (0x00427480: platform_set_display, the buffer's address into 0x00E39174 and 0x00E92E18; the
     * game's format asked), windowed a clipper; then the format from the surface's masks: 565 ->
     * 0x00E3914C 0x565, 0x00E92DFC 2; 555 -> 0x555, 0. Once a window is up (0x00E38DDA != 0) a
     * format other than the one before is logged and the old one kept in 0x00E92DFC (the loaded
     * pixels stay in it) */
    int32_t old = S32(st, FORMAT);
    int full = S32(st, FULLSCREEN) != 0;
    log_msg(st, full ? 0x005D1FBCu : 0x005D1F10u, 0);
    U32(st, DD_PRIMARY) = HANDLE_PRIMARY;
    U32(st, 0x00E92E04u) = 0;
    log_msg(st, 0x005D1F78u, 0);
    U32(st, 0x00E92E04u) = 0;
    /* @bin 0x00427480 the back buffer: an off-screen plain surface in system memory (caps 0x840) */
    int rgb565 = old == 2;
    uint32_t back = platform_set_display(st, S32(st, 0x00E38D9Cu), S32(st, 0x00E38D98u), rgb565);
    U32(st, DD_BACK) = back;
    if (back == 0) {
        log_msg(st, 0x005D1FDCu, 0); /* "ERR:DD ExtraSurface failed." */
        log_msg(st, 0x005D1ECCu, 0);
        return 0;
    }
    U32(st, BACK_PIXELS) = back;
    if (full) {
        log_msg(st, 0x005D1F44u, 0); /* "OK :DD Surface created." */
    } else {
        U32(st, DD_CLIPPER) = HANDLE_CLIPPER;
        log_msg(st, 0x005D1E2Cu, 0); /* "OK :DD Surfaces created and clipper attached." */
    }
    if (rgb565) {
        S32(st, FORMAT_NAME) = 0x565;
        S32(st, FORMAT) = 2;
        log_msg(st, 0x005D1DE4u, 0);
    } else {
        S32(st, FORMAT_NAME) = 0x555;
        S32(st, FORMAT) = 0;
        log_msg(st, 0x005D1D9Cu, 0);
    }
    if (U8(st, WINDOW) != 0 && S32(st, FORMAT) != old) {
        /* @bin 0x004273EF sprintf("ERR: 16-bit surface format changed from %d to %d") into a stack
         * buffer, logged: the format string is logged here */
        log_msg(st, 0x005D1D34u, 0);
        S32(st, FORMAT) = old;
    }
    return 1;
}

static int32_t dd_lock_object(game_state *st, uint32_t self, uint32_t primary, uint32_t back)
{
    /* @bin 0x004D0480 (thiscall, on 0x006DE518; native with a platform) the surfaces kept (+0x1C, +0x20),
     * their memory from a lock (+0x24 the primary's: none, the screen is the platform's; +0x28 the back
     * buffer's), the back buffer's width +8 and height +0xC, +0x10 1 for RGB 565, 0 for 555. The
     * debugger output (OutputDebugStringA) is left out */
    P32(st, self + 0x1C) = primary;
    P32(st, self + 0x20) = back;
    P32(st, self + 0x24) = 0;
    P32(st, self + 0x28) = U32(st, BACK_PIXELS);
    P32(st, self + 0xC) = U32(st, 0x00E38D98u);
    P32(st, self + 8) = U32(st, 0x00E38D9Cu);
    P32(st, self + 0x10) = S32(st, FORMAT) == 2 ? 1u : 0u;
    return 1;
}

int32_t display_setup(game_state *st)
{
    /* @bin 0x004D0060 DirectDraw's set-up for the current resolution and option: the object and mode
     * (0x00426B40), the surfaces (0x00426FF0); when the surfaces fail, the previous mode (0x00E92E4C,
     * 0x005E06D8) is tried again. Then the lock object 0x006DE518 (0x004D0480); the mode is kept as
     * the last one that worked (0x00E92E4C, 0x005E06D8), 0x005E06D4 := 0. Without a platform that
     * sets display modes the original's run is replayed (a partial port) */
    if (!has_display_platform(st)) {
        state_unported(st, 0x004D0060u);
        return 0;
    }
    if (dd_create(st) != 1) {
        log_msg(st, 0x005E07BCu, 0);
        return 0;
    }
    log_msg(st, 0x005E0870u, 0);
    if (dd_surfaces(st) != 1) {
        log_msg(st, 0x005E0844u, 0);
        log_msg(st, 0x005E0824u, 0);
        display_surfaces_release(st, 1);
        S32(st, RESOLUTION) = S32(st, 0x00E92E4Cu);
        S32(st, FULLSCREEN) = S32(st, 0x005E06D8u);
        display_resolution_set(st, S32(st, 0x00E92E4Cu));
        /* @bin 0x004D00F8 sprintf("Old res=%d,%d; Color mode=%d") into a stack buffer, logged */
        log_msg(st, 0x005E0804u, 0);
        if (dd_create(st) != 1 || dd_surfaces(st) != 1) {
            display_surfaces_release(st, 1);
            return 0;
        }
    }
    dd_lock_object(st, 0x006DE518u, U32(st, DD_PRIMARY), U32(st, DD_BACK));
    S32(st, 0x00E92E4Cu) = S32(st, RESOLUTION);
    S32(st, 0x005E06D8u) = S32(st, FULLSCREEN);
    U8(st, 0x005E06D4u) = 0;
    return 1;
}

int32_t display_begin(game_state *st)
{
    /* @bin 0x004D0220 gfxBegin: 0x005E06B0 := 1 (DirectDraw not ready: the present waits), the old
     * surfaces released (0x004D01E0(0)), the set-up 0x004D0060 (its result in 0x00E390E0; on failure
     * "ERR:gfxBegin failed." and 0), the buffers cleared (0x00426F60); then ready, active
     * (0x00E3912C), a full redraw */
    S32(st, 0x005E06B0u) = 1;
    display_end(st, 0);
    int32_t r = GAME_CALL0(st, 0x004D0060u);
    U32(st, DD_READY) = (uint32_t) r;
    if (r == 0) {
        GAME_CALL(st, LOG, 0x005E088C, r, r);
        return 0;
    }
    display_clear(st);
    S32(st, 0x005E06B0u) = 0;
    S32(st, 0x00E3912Cu) = 1;
    U8(st, REDRAW) = 1;
    return 1;
}

/* ---- the window procedure's window messages ---- */

static void window_state(game_state *st, int32_t *iconic, int32_t *foreground, int32_t rect[4])
{
    const sim_platform *p = st->platform;
    if (p && p->window_state && p->window_state(p->ctx, iconic, foreground, rect)) {
        return;
    }
    *iconic = 0;
    *foreground = 1;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = S32(st, DESKTOP_W);
    rect[3] = S32(st, DESKTOP_H);
}

static void window_active_check(game_state *st, int32_t foreground)
{
    /* @bin 0x004CFFC0 the game runs again (0x00E39128 := 0) when its window is the foreground one */
    if (foreground) {
        S32(st, 0x00E39128u) = 0;
    }
}

static void window_paused(game_state *st)
{
    /* @bin 0x004CFF90 "OK :Paused." (0x005E0764), the frames stop (0x00E39128 := 1); InvalidateRect:
     * the platform's */
    log_msg(st, 0x005E0764u, 0);
    S32(st, 0x00E39128u) = 1;
}

static int32_t surfaces_restore(game_state *st)
{
    /* @bin 0x004CFFE0 Restore of the primary surface and of the back buffer (the platform's surfaces
     * are never lost): 1, or 0 with a log when one is missing */
    if (U32(st, DD_PRIMARY) == 0) {
        log_msg(st, 0x005E0774u, 0);
        return 0;
    }
    if (U32(st, DD_BACK) == 0) {
        log_msg(st, 0x005E0798u, 0);
        return 0;
    }
    return 1;
}

void display_window_moved(game_state *st)
{
    /* @bin 0x00414E8A WM_MOVE / WM_SIZE: minimised -> paused (0x004CFF90), else the game runs again
     * when in front (0x004CFFC0) and is active (0x00E3912C := 1); the client rectangle 0x00E39130 (the
     * screen (0, 0, GetSystemMetrics) when fullscreen, else GetClientRect + ClientToScreen); a forced
     * present (0x004D0280(1)) and a full redraw */
    int32_t iconic, foreground, rect[4];
    window_state(st, &iconic, &foreground, rect);
    if (iconic) {
        window_paused(st);
    } else {
        window_active_check(st, foreground);
        S32(st, 0x00E3912Cu) = 1;
    }
    if (S32(st, FULLSCREEN) != 0) {
        S32(st, CLIENT_RECT) = 0;
        S32(st, CLIENT_RECT + 4) = 0;
        S32(st, CLIENT_RECT + 8) = S32(st, DESKTOP_W);
        S32(st, CLIENT_RECT + 12) = S32(st, DESKTOP_H);
    } else {
        for (int i = 0; i < 4; i++) {
            S32(st, CLIENT_RECT + 4u * (uint32_t) i) = rect[i];
        }
    }
    frame_present(st, 1);
    U8(st, REDRAW) = 1;
    U8(st, 0x00E38DC8u) = 1;
}

void display_window_activated(game_state *st, uint32_t active)
{
    /* @bin 0x00414F86 WM_ACTIVATEAPP: active when wParam is set, the window in front and not
     * minimised (0x00E3912C, "OK :Active." / "OK :Not active."); when the frames were stopped and the
     * game is active again, the surfaces are restored (0x004CFFE0), DirectDraw set up again when
     * that fails and the window is in front ("ERR:Re-initializing direct draw.", 0x004D0220), and the
     * frames go on (0x004CFFC0); a full redraw */
    int32_t iconic, foreground, rect[4];
    window_state(st, &iconic, &foreground, rect);
    if (active != 0 && foreground && !iconic) {
        S32(st, 0x00E3912Cu) = 1;
        log_msg(st, 0x005792E8u, 0);
    } else {
        S32(st, 0x00E3912Cu) = 0;
        log_msg(st, 0x005792D4u, 0);
    }
    if (S32(st, 0x00E39128u) != 0 && S32(st, 0x00E3912Cu) != 0) {
        if (surfaces_restore(st) != 0) {
            window_active_check(st, foreground);
        } else if (foreground) {
            log_msg(st, 0x005792B0u, 0);
            if (display_begin(st) != 0) {
                window_active_check(st, foreground);
            }
        }
    }
    U8(st, REDRAW) = 1;
    U8(st, 0x00E38DC8u) = 1;
}

/* ---- WinMain's end ---- */

static int32_t file_write_whole(game_state *st, const char *path, uint32_t buf, uint32_t size)
{
    /* @bin 0x004CED90 _open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600), _write, _close: 1,
     * or 0 when it cannot be opened */
    const uint8_t *p = state_ptr(st, buf, size);
    return p && platform_file_write(st, path, p, size) >= 0 ? 1 : 0;
}

void display_settings_save(game_state *st)
{
    /* @bin 0x00414750 the options (0x234 bytes from 0x00E38E58) into "Pharaoh.inf" (0x00579274) and
     * the map editor's (0x54 from 0x00BCA2B8) into "Pharaohmap.inf" (0x0057924C): the boot reads
     * them back (0x004149D0, core/boot/settings.c); the log "OK :removed manmades=" with
     * [0x00F535BC] + 1; the spare screen 0x00E92E1C and the block 0x00E36C80 freed, the text blocks
     * 0x00E36C70 / 0x00E36C74 deleted and cleared */
    file_write_whole(st, "Pharaoh.inf", 0x00E38E58u, 0x234);
    file_write_whole(st, "Pharaohmap.inf", 0x00BCA2B8u, 0x54);
    GAME_CALL(st, LOG, 0x0057925C, 0, S32(st, 0x00F535BCu) + 1);
    if (U32(st, 0x00E92E1Cu) != 0) {
        display_memory(st, U32(st, 0x00E92E1Cu), 0);
    }
    if (U32(st, 0x00E36C80u) != 0) {
        display_memory(st, U32(st, 0x00E36C80u), 0);
    }
    if (U32(st, 0x00E36C70u) != 0) {
        display_memory(st, U32(st, 0x00E36C70u), 0);
        U32(st, 0x00E36C70u) = 0;
    }
    if (U32(st, 0x00E36C74u) != 0) {
        display_memory(st, U32(st, 0x00E36C74u), 0);
        U32(st, 0x00E36C74u) = 0;
    }
}

static void object_delete(game_state *st, uint32_t holder)
{
    /* @bin 0x0055D960 the object [holder]: destroyed (0x0055D8B0), deleted, its pointer cleared */
    uint32_t o = U32(st, holder);
    if (o != 0) {
        GAME_CALLT0(st, o, 0x0055D8B0u);
        display_memory(st, o, 0); /* operator delete 0x00561A2B */
        U32(st, holder) = 0;
    }
}

void display_quit_cleanup(game_state *st)
{
    /* @bin 0x004E63B0 the objects 0x00EA136C and 0x00EA1370 deleted through their vtables (scalar
     * deleting destructor, 1) and cleared, the object 0x00EA1368 (0x0055D960), the images' reset
     * (0x0053DB60 on 0x00F55BB0) */
    for (uint32_t h = 0x00EA136Cu; h <= 0x00EA1370u; h += 4) {
        uint32_t o = U32(st, h);
        if (o != 0) {
            GAME_CALLT(st, o, P32(st, P32(st, o)), 1);
            U32(st, h) = 0;
        }
    }
    object_delete(st, 0x00EA1368u);
    GAME_CALLT0(st, 0x00F55BB0u, 0x0053DB60u);
}

static void bink_close(game_state *st, uint32_t self)
{
    /* @bin 0x004133F0 the Bink player's handles closed (BinkClose +0x10, +0xC: the platform's; the
     * platform plays videos to their end and keeps none) and cleared */
    if (P32(st, self + 0x10) != 0) {
        P32(st, self + 0x10) = 0;
    }
    if (P32(st, self + 0xC) != 0) {
        P32(st, self + 0xC) = 0;
    }
}

void display_bink_release(game_state *st)
{
    /* @bin 0x004CFF50 the Bink player's object 0x006FDC54 closed (0x004133F0), deleted, cleared;
     * CoUninitialize (the platform's); "OK :Exited Normally" (0x005E074C) */
    uint32_t b = U32(st, 0x006FDC54u);
    if (b != 0) {
        bink_close(st, b);
        display_memory(st, b, 0); /* operator delete 0x00561A2B */
        U32(st, 0x006FDC54u) = 0;
    }
    log_msg(st, 0x005E074Cu, 0);
}

void display_video_stop(game_state *st)
{
    /* @bin 0x005391E0 a Smacker video open (0x00F54054): the CD path restored (0x004CE100 on
     * 0x00F5400C) unless 0x00F5404C, SmackClose of 0x00F54114 (the platform's; none is ever open:
     * platform_video_play) and cleared, the working directory (0x004CE2D0) */
    if (U32(st, 0x00F54054u) == 0) {
        return;
    }
    if (U32(st, 0x00F5404Cu) == 0) {
        GAME_CALL(st, 0x004CE100u, 0x00F5400C);
    }
    if (U32(st, 0x00F54114u) != 0) {
        U32(st, 0x00F54114u) = 0;
    }
    GAME_CALL0(st, 0x004CE2D0u);
}

/* ---- port table ---- */

static int32_t run_res_set(game_state *st, const int32_t *a) { display_resolution_set(st, a[0]); return 0; }
static int32_t run_mode_set(game_state *st, const int32_t *a) { display_mode_set(st, a[0]); return 0; }
static int32_t run_layout(game_state *st, const int32_t *a) { (void) a; display_buffers_layout(st); return 0; }
static int32_t run_res_check(game_state *st, const int32_t *a) { (void) a; display_resolution_check(st); return 0; }
static int32_t run_release(game_state *st, const int32_t *a) { return display_surfaces_release(st, a[0]); }
static int32_t run_end(game_state *st, const int32_t *a) { display_end(st, a[0]); return 0; }
static int32_t run_clear(game_state *st, const int32_t *a) { (void) a; return display_clear(st); }
static int32_t run_setup(game_state *st, const int32_t *a) { (void) a; return display_setup(st); }
static int32_t run_begin(game_state *st, const int32_t *a) { (void) a; return display_begin(st); }
static int32_t run_settings(game_state *st, const int32_t *a) { (void) a; display_settings_save(st); return 0; }
static int32_t run_cleanup(game_state *st, const int32_t *a) { (void) a; display_quit_cleanup(st); return 0; }
static int32_t run_bink(game_state *st, const int32_t *a) { (void) a; display_bink_release(st); return 0; }
static int32_t run_video_stop(game_state *st, const int32_t *a) { (void) a; display_video_stop(st); return 0; }
static int32_t run_file_write(game_state *st, const int32_t *a)
{
    const char *path = (const char *) state_ptr(st, (uint32_t) a[0], 1);
    char name[260];
    size_t n = 0;
    for (; path && n + 1 < sizeof(name); n++) {
        const uint8_t *c = state_ptr(st, (uint32_t) a[0] + (uint32_t) n, 1);
        if (!c || *c == 0) {
            break;
        }
        name[n] = (char) *c;
    }
    name[n] = 0;
    return path ? file_write_whole(st, name, (uint32_t) a[1], (uint32_t) a[2]) : 0;
}
static int32_t run_object_delete(game_state *st, const int32_t *a) { object_delete(st, (uint32_t) a[0]); return 0; }
static int32_t run_bink_close(game_state *st, const int32_t *a) { bink_close(st, (uint32_t) a[PORT_THIS_ARG]); return 0; }
static int32_t run_restore(game_state *st, const int32_t *a) { (void) a; return surfaces_restore(st); }
static int32_t run_paused(game_state *st, const int32_t *a) { (void) a; window_paused(st); return 0; }
static int32_t run_active_check(game_state *st, const int32_t *a)
{
    (void) a;
    int32_t iconic, foreground, rect[4];
    window_state(st, &iconic, &foreground, rect);
    window_active_check(st, foreground);
    return 0;
}

const ported_function display_ports[] = {
    {0x004CFD60u, "display_resolution_set", 1, 0, 0, run_res_set, 0},
    {0x004CFDC0u, "display_mode_set", 1, 0, 0, run_mode_set, 0},
    {0x00414BD0u, "display_buffers_layout (platform memory)", 0, 0, 0, run_layout, PORT_PARTIAL},
    {0x00402DA0u, "display_resolution_check", 0, 0, 0, run_res_check, 0},
    {0x00426EE0u, "display_surfaces_release", 1, 32, 0, run_release, 0},
    {0x004D01E0u, "display_end", 1, 0, 0, run_end, 0},
    {0x00426F60u, "display_clear", 0, 32, 0, run_clear, 0},
    {0x004D0060u, "display_setup (platform: set_display)", 0, 32, 0, run_setup, PORT_PARTIAL},
    {0x00426B40u, "dd_create (in 0x004D0060)", 0, 32, 0, NULL, 0},
    {0x00426FF0u, "dd_surfaces (in 0x004D0060)", 0, 32, 0, NULL, 0},
    {0x00427480u, "dd_surface_back (in 0x004D0060)", 4, 32, 0, NULL, 0},
    {0x004D0480u, "dd_lock_object (in 0x004D0060)", 2, 32, 0, NULL, 0},
    {0x004D0220u, "display_begin", 0, 32, 0, run_begin, 0},
    {0x004CFF90u, "window_paused", 0, 0, 0, run_paused, 0},
    {0x004CFFC0u, "window_active_check", 0, 0, 0, run_active_check, 0},
    {0x004CFFE0u, "surfaces_restore", 0, 32, 0, run_restore, 0},
    {0x00414750u, "display_settings_save", 0, 0, 0, run_settings, 0},
    {0x004CED90u, "file_write_whole", 3, 32, 0, run_file_write, 0},
    {0x004E63B0u, "display_quit_cleanup", 0, 0, 0, run_cleanup, 0},
    {0x0055D960u, "object_delete", 1, 0, 0, run_object_delete, 0},
    {0x004CFF50u, "display_bink_release", 0, 0, 0, run_bink, 0},
    {0x004133F0u, "bink_close", 0, 0, 0, run_bink_close, 0},
    {0x005391E0u, "display_video_stop", 0, 0, 0, run_video_stop, 0},
};
const int display_ports_count = sizeof(display_ports) / sizeof(display_ports[0]);
