/* Imhotep - the boot: the window and DirectDraw set-up (package boot).
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 * WinMain's 0x004CF960 (docs/presentation.md 2.1): the resolution globals, the window class, the
 * cursors, the window, the Bink player's object, then gfxBegin (0x004D0220): DirectDraw, the display
 * mode, the primary surface and the back buffer (a system-memory surface whose pixels the drawing code
 * writes through 0x00E92E18), the pixel format. The window system and DirectDraw are the platform's:
 * their handles and the surfaces' memory come from the boot's host (core/boot/boot.h); the state gets
 * what the original stores. The display mode is the one the settings read (core/boot/settings.c:
 * the resolution 0x00E38E68 and the fullscreen option 0x00E38E5C from Pharaoh.inf, the defaults and the
 * screen's width otherwise); the host's display (boot_display) is the screen before the set-up. Both
 * paths are ported: fullscreen (0x00E38E5C = 1, the default) sets the display mode; windowed (F6 in the
 * game, kept in Pharaoh.inf) keeps the screen's mode, centres the window and attaches a clipper (the
 * window's styles and place are the platform's). */
#include <string.h>

#include "boot/boot_internal.h"
#include "city/text_lookup.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/layout.h"
#include "state/ported.h"


int32_t boot_bink_object(game_state *st, const boot_host *host)
{
    /* @bin 0x004CFD0E the Bink player's object: operator new(0x1C), constructor 0x004133D0(the window
     * 0x00E39140): +8 the window, +4 (byte) 1, +0x10 and +0xC 0; its address in 0x006FDC54 (0 when
     * the allocation fails). The object is the platform's (core/sim/platform.h platform_video_bink) */
    uint32_t p = boot_alloc(st, host, 0x1C, 0x004CFD13u);
    uint8_t *o = p ? state_ptr(st, p, 0x1C) : NULL;
    if (o) {
        uint32_t wnd = U32(st, G_HWND);
        o[8] = (uint8_t) wnd;
        o[9] = (uint8_t) (wnd >> 8);
        o[10] = (uint8_t) (wnd >> 16);
        o[11] = (uint8_t) (wnd >> 24);
        o[4] = 1;
        for (int i = 0xC; i < 0x14; i++) {
            o[i] = 0;
        }
    }
    U32(st, G_BINK_PLAYER) = o ? p : 0;
    return 1;
}

static uint32_t handle(game_state *st, const boot_host *host, int32_t kind, int32_t id)
{
    return host && host->handle ? host->handle(host->ctx, st, kind, id) : 1u;
}

/* WM_DISPLAYCHANGE (0x7E: 16 bits per pixel, height << 16 | width) to the window procedure 0x00414CD0 */
static void display_changed(game_state *st, int32_t w, int32_t h)
{
    GAME_CALL(st, FN_FRAME_WNDPROC, (int32_t) U32(st, G_HWND), 0x7E, 16, (int32_t) ((uint32_t) (uint16_t) h << 16 | (uint16_t) w));
}

/* @bin 0x00426B40 DirectDraw opened in the display mode (fullscreen); 1, or 0 on failure */
static int32_t dd_open(game_state *st, const boot_host *host, const boot_display *d)
{
    /* @bin 0x00426B46 GetDeviceCaps: planes * bits per pixel (a 16-bit display: 1 * 16) */
    S32(st, G_OPTIONS) = 16;
    S32(st, G_DESKTOP_W) = d->width;  /* GetSystemMetrics(SM_CXSCREEN): the screen's, before the mode */
    S32(st, G_DESKTOP_H) = d->height; /* GetSystemMetrics(SM_CYSCREEN) */
    /* @bin 0x00426B9C a mode wider than the screen is shown fullscreen (windowed: the notice 0x39 first,
     * 0x004E2970); windowed on a display of another depth than 16 bits too (the depth is 16 here) */
    if (S32(st, G_DESKTOP_W) < S32(st, G_SCREEN_W)) {
        if (S32(st, G_FULLSCREEN) == 0) {
            GAME_CALL(st, FN_NOTICE_ADD_4E2970, 0x39, 0, 0);
        }
        S32(st, G_FULLSCREEN) = 1;
    }
    /* @bin 0x00426BF0 GetPaletteEntries(NULL, 0, 0x100, 0x0070D508) (fullscreen only): no palette in
     * 16-bit modes, nothing written */
    uint32_t dd = U32(st, G_DDRAW);
    if (dd == 0) {
        dd = handle(st, host, BOOT_DIRECTDRAW, 0); /* DirectDrawCreate */
        if (dd == 0) {
            platform_log(st, G_STR_DD_ENABLE_ERROR, 0, -1);
            return 0;
        }
        /* SetCooperativeLevel(window, fullscreen: DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT,
         * windowed: DDSCL_NORMAL) */
        platform_log(st, G_STR_DD_SET_COOPERATIVE, 0, 0);
    }
    if (S32(st, G_FULLSCREEN) == 0) {
        /* @bin 0x00426E09 windowed: the window's styles and size for the mode (SetWindowLongA,
         * AdjustWindowRectEx, SetWindowPos: the platform's), its place centred on the screen
         * (0x00E390E8, 0x00E390E4; signed division by 2), "OK :DD Kept resolution." */
        S32(st, G_WINDOWED_X) = (S32(st, G_DESKTOP_W) - S32(st, G_SCREEN_W)) / 2;
        S32(st, G_WINDOWED_Y) = (S32(st, G_DESKTOP_H) - S32(st, G_SCREEN_H)) / 2;
        platform_log(st, G_STR_DD_KEPT_RESOLUTION, 0, 0);
        U32(st, G_DDRAW) = dd;
        return 1;
    }
    /* @bin 0x00426CA4 SetWindowLongA (the window's style), SetDisplayMode(0x00E38D9C, 0x00E38D98, 16)
     * (0x00426CE1): the platform's. assumed (Windows' behaviour, not the game's): when the mode differs
     * from the screen's, Windows sends WM_DISPLAYCHANGE to the window during the call, whose handler
     * (0x00414CD0) sets the screen's size 0x00E38DB8 / 0x00E38DB4 to the mode; from then on
     * GetSystemMetrics gives the mode (the client rectangle of WM_SIZE / WM_MOVE). With a screen
     * already in the mode (tools/oracle/game.py: GetSystemMetrics gives the display's size) nothing is
     * sent */
    if (S32(st, G_UE38D9C) != d->width || S32(st, G_UE38D98) != d->height) {
        display_changed(st, S32(st, G_UE38D9C), S32(st, G_UE38D98));
    }
    platform_log(st, G_STR_DD_SET_RESOLUTION, 0, 0);
    U32(st, G_DDRAW) = dd;
    return 1;
}

/* @bin 0x00426FF0 the primary surface and the back buffer (0x00427480: an off-screen plain surface in
 * system memory, w x h), locked once: its pixels' address in 0x00E92E18; the pixel format */
static int32_t surfaces_create(game_state *st, const boot_host *host, const boot_display *d, uint32_t *front)
{
    int full = S32(st, G_FULLSCREEN) != 0;
    /* @bin 0x00427142 windowed: "OK :DD Creating one Vram and one..." (0x005D1F10), else "... Vram only" */
    platform_log(st, full ? G_STR_DD_VRAM_ONLY : G_STR_DD_VRAM_AND_SYSTEM, 0, 0);
    /* the primary surface: the screen, in the display mode (fullscreen) or the screen's own (windowed:
     * GetSystemMetrics, 0x00E38DB8 x 0x00E38DB4; tools/oracle/game.py surfaces of that size) */
    int32_t sw = full ? S32(st, G_SCREEN_W) : S32(st, G_DESKTOP_W);
    int32_t sh = full ? S32(st, G_SCREEN_H) : S32(st, G_DESKTOP_H);
    uint32_t pixels = host && host->surface ? host->surface(host->ctx, st, sw, sh, 1)
                                            : boot_alloc(st, host, (uint32_t) sw * 2u * (uint32_t) sh, 0x53000000u);
    uint32_t primary = pixels ? handle(st, host, BOOT_SURFACE, 1) : 0;
    U32(st, G_DD_PRIMARY) = primary;
    if (!primary) {
        platform_log(st, G_STR_DD_CREATE_SURFACE_FAILED2, 0, 0);
        return 0;
    }
    S32(st, G_UE92E04) = 0;
    platform_log(st, G_STR_DD_CAN_LOCK_PRIMARY, 0, 0); /* the primary surface locks */
    S32(st, G_UE92E04) = 0;
    uint32_t back_pixels = host && host->surface
                               ? host->surface(host->ctx, st, S32(st, G_UE38D9C), S32(st, G_UE38D98), 0)
                               : boot_alloc(st, host, (uint32_t) S32(st, G_UE38D9C) * 2u * (uint32_t) S32(st, G_UE38D98),
                                            0x53000000u);
    uint32_t back = back_pixels ? handle(st, host, BOOT_SURFACE, 0) : 0;
    U32(st, G_DD_BACK) = back;
    if (!back) {
        platform_log(st, G_STR_DD_CANT_CREATE_BACKBUFFER, 0, 0);
        return 0;
    }
    U32(st, G_BACK_BUFFER) = back_pixels; /* @bin 0x00427257 lpSurface of the lock */
    if (full) {
        platform_log(st, G_STR_DD_SURFACE_CREATED, 0, 0);
    } else {
        /* @bin 0x0042726D windowed: a clipper (CreateClipper into 0x00E39170, SetHWnd(the window),
         * SetClipper on the primary surface), "OK :DD Surfaces created and clipper attached." */
        uint32_t clipper = handle(st, host, BOOT_CLIPPER, 0);
        U32(st, G_DD_CLIPPER) = clipper;
        if (!clipper) {
            platform_log(st, G_STR_DD_CANT_CREATE_CLIPPER, 0, 0);
            return 0;
        }
        platform_log(st, G_STR_DD_CLIPPER_ATTACHED, 0, 0);
    }
    /* @bin 0x004272F3 the format from the primary surface's masks: green 0x07E0: 565 (2), else 555 (0)
     * (red 0xFC00 655 and blue 0x3F 556 are forced to 565; DirectDraw gives the display's format) */
    if (d->format == 565) {
        S32(st, G_UE3914C) = 0x565;
        S32(st, G_PIXEL_FORMAT) = 2;
        platform_log(st, G_STR_DD_RGB565, 0, 0);
    } else {
        S32(st, G_UE3914C) = 0x555;
        S32(st, G_PIXEL_FORMAT) = 0;
        platform_log(st, G_STR_DD_RGB555, 0, 0);
    }
    /* @bin 0x004273C6 a window open and a format other than 0: the format of an earlier set-up kept
     * (the boot has none: 0x00E38DDA is 0) */
    if (U8(st, G_WINDOW) != 0 && S32(st, G_PIXEL_FORMAT) != 0) {
        S32(st, G_PIXEL_FORMAT) = 0;
    }
    *front = pixels;
    return 1;
}

/* @bin 0x004D0480 (thiscall on 0x006DE518, primary, back) both surfaces locked once: their objects
 * (+0x1C, +0x20), pixels (+0x24, +0x28), the back buffer's width (+8) and height (+0xC), +0x10 1 for
 * 565 masks, 0 for 555 */
static void surfaces_keep(game_state *st, uint32_t self, uint32_t primary, uint32_t back, uint32_t primary_pixels,
                          const boot_display *d)
{
    U32(st, self + 0x1C) = primary;
    U32(st, self + 0x20) = back;
    U32(st, self + 0x24) = primary_pixels;
    U32(st, self + 0x28) = U32(st, G_BACK_BUFFER);
    S32(st, self + 0xC) = S32(st, G_UE38D98);
    S32(st, self + 8) = S32(st, G_UE38D9C);
    S32(st, self + 0x10) = d->format == 565 ? 1 : 0;
}

/* @bin 0x004D0220 gfxBegin: 0x004D01E0 (nothing open yet), 0x004D0060, the clearing fills 0x00426F60 */
static int32_t gfx_begin(game_state *st, const boot_host *host, const boot_display *d)
{
    U8(st, G_DD_NOT_READY) = 1;
    if (U32(st, G_UE92E00) != 0 || S32(st, G_DD_READY) != 0) {
        state_unported(st, FN_DISPLAY_END); /* a display already open: released first */
        return 0;
    }
    /* @bin 0x004D0060 */
    int32_t ok = dd_open(st, host, d);
    if (ok != 1) {
        platform_log(st, G_STR_GFX_DD_NOT_ENABLED, 0, 0);
        S32(st, G_DD_READY) = 0;
        platform_log(st, G_STR_GFX_BEGIN_FAILED, 0, 0);
        return 0;
    }
    platform_log(st, G_STR_GFX_DD_ENABLED, 0, 0);
    uint32_t primary_pixels = 0;
    if (surfaces_create(st, host, d, &primary_pixels) != 1) {
        state_unported(st, FN_DISPLAY_SURFACES_RELEASE); /* the default display mode tried again */
        return 0;
    }
    surfaces_keep(st, G_SURFACE_LOCK, U32(st, G_DD_PRIMARY), U32(st, G_DD_BACK), primary_pixels, d);
    S32(st, G_RESOLUTION_KEPT) = S32(st, G_RESOLUTION);
    S32(st, G_FULLSCREEN_KEPT) = S32(st, G_FULLSCREEN);
    U8(st, G_U5E06D4) = 0;
    S32(st, G_DD_READY) = 1;
    /* @bin 0x00426F60 UpdateWindow, then colour fills (black) of the back buffer and the primary surface:
     * DirectDraw's, on surfaces that are new (the oracle's fills leave them as they are) */
    U8(st, G_DD_NOT_READY) = 0;
    S32(st, G_APP_ACTIVE) = 1;
    U8(st, G_REDRAW) = 1;
    return 1;
}

/* @bin 0x004CF960 (hInstance) */
static int32_t display_create(game_state *st, const boot_host *host, const boot_display *d, uint32_t instance)
{
    /* @bin 0x004CF97C _getcwd(0x00E36B94, 100) */
    const char *cwd = host && host->cwd ? host->cwd(host->ctx) : "C:\\";
    size_t n = cwd ? strlen(cwd) : 0;
    if (n + 1 <= 100) {
        memcpy(st->mem + (G_GAME_DIR - STATE_BASE), cwd, n + 1);
    }
    S32(st, G_R_IV1) = 0x54657687;
    S32(st, G_R_IV2) = 0x72641663;
    S32(st, G_UD3B37C) = 0x34518632;
    U8(st, G_UD3B494) = 0;
    U8(st, G_UD3B494 + 1u) = 0;
    U8(st, G_UD3B494 + 2u) = 0;
    U8(st, G_UD3B494 + 3u) = 0;
    U8(st, G_UE38E4C) = 0;
    S32(st, G_TEXT_SHADOW_V) = 0;
    S32(st, G_UD3B508) = 0;
    S32(st, G_TEXT_OUTLINE) = 0;
    S32(st, G_DRAG_STATE) = 0;
    U8(st, G_UE38DA1) = 1;
    /* @bin 0x004CF990 the resolution: 2 800x600, 3 1024x768, else 640x480 */
    int32_t w = 640, h = 480;
    if (S32(st, G_RESOLUTION) == 2) {
        w = 800;
        h = 600;
    } else if (S32(st, G_RESOLUTION) == 3) {
        w = 1024;
        h = 768;
    }
    S32(st, G_SCREEN_W) = w;
    S32(st, G_LAYOUT_DX) = (w - 640) / 2;
    S32(st, G_SCREEN_H) = h;
    S32(st, G_LAYOUT_DY) = (h - 480) / 2;
    S32(st, G_SCREEN_PITCH) = 2 * w;
    S32(st, G_UE38D9C) = w;
    S32(st, G_UE38D98) = h;
    S32(st, G_UE38D90) = h;
    S32(st, G_SCREEN_BYTES) = 2 * w * h;
    S32(st, G_CLIP_LEFT) = 0;
    S32(st, G_CLIP_RIGHT) = w;
    S32(st, G_CLIP_TOP) = 0;
    S32(st, G_CLIP_BOTTOM) = h;
    S32(st, G_DESKTOP_W) = d->width;  /* GetSystemMetrics */
    S32(st, G_DESKTOP_H) = d->height;
    platform_log(st, G_STR_DEBUG_REPORT, 0, 0);
    /* @bin 0x004CFAB2 CoInitialize; the window's title (text 9.0); FindWindowA: no other instance */
    text_seek(st, 9, 0);
    /* @bin 0x004CFB25 RegisterClassA ("WinSJBClass", WndProc 0x00414CD0); the cursors of the exe */
    static const struct {
        int32_t id;
        uint32_t at;
    } CURSORS[19] = {
        {0x66, G_CURSOR_66}, {0x88, G_CURSOR_88}, {0x82, G_CURSOR_82}, {0x93, G_CURSOR_93}, {0x90, G_CURSOR_UNLOAD},
        {0x91, G_CURSOR_BOARD}, {0x92, G_CURSOR_92}, {0x8E, G_CURSOR_8E}, {0x96, G_CURSOR_96}, {0x95, G_CURSOR_95},
        {0x94, G_CURSOR_94}, {0x99, G_CURSOR_99}, {0x98, G_CURSOR_98}, {0x9B, G_CURSOR_9B}, {0x9A, G_CURSOR_9A},
        {0x9C, G_CURSOR_9C}, {0x9D, G_CURSOR_9D}, {0x9F, G_CURSOR_9F}, {0xA0, G_CURSOR_A0},
    };
    for (int i = 0; i < 19; i++) {
        U32(st, CURSORS[i].at) = handle(st, host, BOOT_CURSOR, CURSORS[i].id);
    }
    U32(st, G_HINSTANCE) = instance;
    /* @bin 0x004CFCBD CreateWindowExA (the screen's size) */
    uint32_t wnd = handle(st, host, BOOT_WINDOW, 0);
    U32(st, G_HWND) = wnd;
    if (wnd == 0) {
        return 0;
    }
    U32(st, G_DD_PRIMARY) = 0;
    U32(st, G_DD_BACK) = 0;
    U32(st, G_DDRAW) = 0;
    U32(st, G_DD_CLIPPER) = 0;
    boot_bink_object(st, host); /* @bin 0x004CFD0E */
    return gfx_begin(st, host, d);
}

int32_t boot_display_setup(game_state *st, const boot_host *host, const boot_display *display)
{
    /* @bin 0x0041401E 0x004CF960(hInstance: the exe's image base), in the mode the settings read
     * (0x00E38E68, 0x00E38E5C: Pharaoh.inf, core/boot/settings.c) */
    int32_t r = display_create(st, host, display, 0x00400000u);
    if (r == 0) {
        return 0;
    }
    /* the window shown: Windows sends WM_SIZE (the client size) and WM_MOVE (assumed order, as
     * tools/oracle/game.py) to the window procedure 0x00414CD0 (core/frame/wndproc.c: the client
     * rectangle 0x00E39130, a present, a redraw; core/frame/display.c display_window_moved) */
    int32_t w = S32(st, G_SCREEN_W), h = S32(st, G_SCREEN_H);
    GAME_CALL(st, FN_FRAME_WNDPROC, (int32_t) U32(st, G_HWND), 5, 0, (int32_t) ((uint32_t) (uint16_t) h << 16 | (uint16_t) w));
    GAME_CALL(st, FN_FRAME_WNDPROC, (int32_t) U32(st, G_HWND), 3, 0, 0);
    return 1;
}
