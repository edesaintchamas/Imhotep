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
 * screen's width otherwise); the host's display (boot_display) is the screen before the set-up. Only the
 * fullscreen path is ported (0x00E38E5C = 1, the default, and what tools/oracle/game.py's display boots
 * read from their Pharaoh.inf); the windowed path (window styles, a clipper) stops as unported. */
#include <string.h>

#include "boot/boot_internal.h"
#include "city/text_lookup.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"

#define FULLSCREEN 0x00E38E5Cu
#define RESOLUTION 0x00E38E68u
#define WINDOW 0x00E39140u
#define DDRAW 0x00E39148u
#define PRIMARY 0x00E39178u
#define BACK 0x00E39174u
#define CLIPPER 0x00E39170u

int32_t boot_bink_object(game_state *st, const boot_host *host)
{
    /* @bin 0x004CFD0E the Bink player's object: operator new(0x1C), constructor 0x004133D0(the window
     * 0x00E39140): +8 the window, +4 (byte) 1, +0x10 and +0xC 0; its address in 0x006FDC54 (0 when
     * the allocation fails). The object is the platform's (core/sim/platform.h platform_video_bink) */
    uint32_t p = boot_alloc(st, host, 0x1C, 0x004CFD13u);
    uint8_t *o = p ? state_ptr(st, p, 0x1C) : NULL;
    if (o) {
        uint32_t wnd = U32(st, 0x00E39140u);
        o[8] = (uint8_t) wnd;
        o[9] = (uint8_t) (wnd >> 8);
        o[10] = (uint8_t) (wnd >> 16);
        o[11] = (uint8_t) (wnd >> 24);
        o[4] = 1;
        for (int i = 0xC; i < 0x14; i++) {
            o[i] = 0;
        }
    }
    U32(st, 0x006FDC54u) = o ? p : 0;
    return 1;
}

static uint32_t handle(game_state *st, const boot_host *host, int32_t kind, int32_t id)
{
    return host && host->handle ? host->handle(host->ctx, st, kind, id) : 1u;
}

/* WM_DISPLAYCHANGE (0x7E: 16 bits per pixel, height << 16 | width) to the window procedure 0x00414CD0 */
static void display_changed(game_state *st, int32_t w, int32_t h)
{
    GAME_CALL(st, 0x00414CD0u, (int32_t) U32(st, WINDOW), 0x7E, 16, (int32_t) ((uint32_t) (uint16_t) h << 16 | (uint16_t) w));
}

/* @bin 0x00426B40 DirectDraw opened in the display mode (fullscreen); 1, or 0 on failure */
static int32_t dd_open(game_state *st, const boot_host *host, const boot_display *d)
{
    /* @bin 0x00426B46 GetDeviceCaps: planes * bits per pixel (a 16-bit display: 1 * 16) */
    S32(st, 0x00E38E58u) = 16;
    S32(st, 0x00E38DB8u) = d->width;  /* GetSystemMetrics(SM_CXSCREEN): the screen's, before the mode */
    S32(st, 0x00E38DB4u) = d->height; /* GetSystemMetrics(SM_CYSCREEN) */
    if (S32(st, 0x00E38DB8u) < S32(st, 0x00E38DB0u)) {
        if (S32(st, FULLSCREEN) == 0) {
            state_unported(st, 0x004E2970u); /* the windowed mode does not fit: a message */
            return 0;
        }
        S32(st, FULLSCREEN) = 1;
    } else if (S32(st, FULLSCREEN) == 0) {
        state_unported(st, 0x00426BC8u); /* windowed */
        return 0;
    }
    /* @bin 0x00426BF0 GetPaletteEntries(NULL, 0, 0x100, 0x0070D508): no palette in 16-bit modes, nothing
     * written */
    uint32_t dd = U32(st, DDRAW);
    if (dd == 0) {
        dd = handle(st, host, BOOT_DIRECTDRAW, 0); /* DirectDrawCreate */
        if (dd == 0) {
            platform_log(st, 0x005D1CB8u, 0, -1);
            return 0;
        }
        /* SetCooperativeLevel(window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT) */
        platform_log(st, 0x005D1C74u, 0, 0);
    }
    /* @bin 0x00426CA4 SetWindowLongA (the window's style), SetDisplayMode(0x00E38D9C, 0x00E38D98, 16)
     * (0x00426CE1): the platform's. assumed (Windows' behaviour, not the game's): when the mode differs
     * from the screen's, Windows sends WM_DISPLAYCHANGE to the window during the call, whose handler
     * (0x00414CD0) sets the screen's size 0x00E38DB8 / 0x00E38DB4 to the mode; from then on
     * GetSystemMetrics gives the mode (the client rectangle of WM_SIZE / WM_MOVE). With a screen
     * already in the mode (tools/oracle/game.py: GetSystemMetrics gives the display's size) nothing is
     * sent */
    if (S32(st, 0x00E38D9Cu) != d->width || S32(st, 0x00E38D98u) != d->height) {
        display_changed(st, S32(st, 0x00E38D9Cu), S32(st, 0x00E38D98u));
    }
    platform_log(st, 0x005D1C40u, 0, 0);
    U32(st, DDRAW) = dd;
    return 1;
}

/* @bin 0x00426FF0 the primary surface and the back buffer (0x00427480: an off-screen plain surface in
 * system memory, w x h), locked once: its pixels' address in 0x00E92E18; the pixel format */
static int32_t surfaces_create(game_state *st, const boot_host *host, const boot_display *d, uint32_t *front)
{
    platform_log(st, 0x005D1FBCu, 0, 0);
    /* the primary surface: the screen, in the display mode (fullscreen) */
    int32_t sw = S32(st, 0x00E38DB0u), sh = S32(st, 0x00E38DACu);
    uint32_t pixels = host && host->surface ? host->surface(host->ctx, st, sw, sh, 1)
                                            : boot_alloc(st, host, (uint32_t) sw * 2u * (uint32_t) sh, 0x53000000u);
    uint32_t primary = pixels ? handle(st, host, BOOT_SURFACE, 1) : 0;
    U32(st, PRIMARY) = primary;
    if (!primary) {
        platform_log(st, 0x005D1F9Cu, 0, 0);
        return 0;
    }
    S32(st, 0x00E92E04u) = 0;
    platform_log(st, 0x005D1F78u, 0, 0); /* the primary surface locks */
    S32(st, 0x00E92E04u) = 0;
    uint32_t back_pixels = host && host->surface
                               ? host->surface(host->ctx, st, S32(st, 0x00E38D9Cu), S32(st, 0x00E38D98u), 0)
                               : boot_alloc(st, host, (uint32_t) S32(st, 0x00E38D9Cu) * 2u * (uint32_t) S32(st, 0x00E38D98u),
                                            0x53000000u);
    uint32_t back = back_pixels ? handle(st, host, BOOT_SURFACE, 0) : 0;
    U32(st, BACK) = back;
    if (!back) {
        platform_log(st, 0x005D1ECCu, 0, 0);
        return 0;
    }
    U32(st, 0x00E92E18u) = back_pixels; /* @bin 0x00427257 lpSurface of the lock */
    platform_log(st, 0x005D1F44u, 0, 0);
    /* @bin 0x004272F3 the format from the primary surface's masks: green 0x07E0: 565 (2), else 555 (0)
     * (red 0xFC00 655 and blue 0x3F 556 are forced to 565; DirectDraw gives the display's format) */
    if (d->format == 565) {
        S32(st, 0x00E3914Cu) = 0x565;
        S32(st, 0x00E92DFCu) = 2;
        platform_log(st, 0x005D1DE4u, 0, 0);
    } else {
        S32(st, 0x00E3914Cu) = 0x555;
        S32(st, 0x00E92DFCu) = 0;
        platform_log(st, 0x005D1D9Cu, 0, 0);
    }
    /* @bin 0x004273C6 a window open and a format other than 0: the format of an earlier set-up kept
     * (the boot has none: 0x00E38DDA is 0) */
    if (U8(st, 0x00E38DDAu) != 0 && S32(st, 0x00E92DFCu) != 0) {
        S32(st, 0x00E92DFCu) = 0;
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
    U32(st, self + 0x28) = U32(st, 0x00E92E18u);
    S32(st, self + 0xC) = S32(st, 0x00E38D98u);
    S32(st, self + 8) = S32(st, 0x00E38D9Cu);
    S32(st, self + 0x10) = d->format == 565 ? 1 : 0;
}

/* @bin 0x004D0220 gfxBegin: 0x004D01E0 (nothing open yet), 0x004D0060, the clearing fills 0x00426F60 */
static int32_t gfx_begin(game_state *st, const boot_host *host, const boot_display *d)
{
    U8(st, 0x005E06B0u) = 1;
    if (U32(st, 0x00E92E00u) != 0 || S32(st, 0x00E390E0u) != 0) {
        state_unported(st, 0x004D01E0u); /* a display already open: released first */
        return 0;
    }
    /* @bin 0x004D0060 */
    int32_t ok = dd_open(st, host, d);
    if (ok != 1) {
        platform_log(st, 0x005E07BCu, 0, 0);
        S32(st, 0x00E390E0u) = 0;
        platform_log(st, 0x005E088Cu, 0, 0);
        return 0;
    }
    platform_log(st, 0x005E0870u, 0, 0);
    uint32_t primary_pixels = 0;
    if (surfaces_create(st, host, d, &primary_pixels) != 1) {
        state_unported(st, 0x00426EE0u); /* the default display mode tried again */
        return 0;
    }
    surfaces_keep(st, 0x006DE518u, U32(st, PRIMARY), U32(st, BACK), primary_pixels, d);
    S32(st, 0x00E92E4Cu) = S32(st, RESOLUTION);
    S32(st, 0x005E06D8u) = S32(st, FULLSCREEN);
    U8(st, 0x005E06D4u) = 0;
    S32(st, 0x00E390E0u) = 1;
    /* @bin 0x00426F60 UpdateWindow, then colour fills (black) of the back buffer and the primary surface:
     * DirectDraw's, on surfaces that are new (the oracle's fills leave them as they are) */
    U8(st, 0x005E06B0u) = 0;
    S32(st, 0x00E3912Cu) = 1;
    U8(st, 0x00E38DCAu) = 1;
    return 1;
}

/* @bin 0x004CF960 (hInstance) */
static int32_t display_create(game_state *st, const boot_host *host, const boot_display *d, uint32_t instance)
{
    /* @bin 0x004CF97C _getcwd(0x00E36B94, 100) */
    const char *cwd = host && host->cwd ? host->cwd(host->ctx) : "C:\\";
    size_t n = cwd ? strlen(cwd) : 0;
    if (n + 1 <= 100) {
        memcpy(st->mem + (0x00E36B94u - STATE_BASE), cwd, n + 1);
    }
    S32(st, 0x00D3B384u) = 0x54657687;
    S32(st, 0x00D3B380u) = 0x72641663;
    S32(st, 0x00D3B37Cu) = 0x34518632;
    U8(st, 0x00D3B494u) = 0;
    U8(st, 0x00D3B495u) = 0;
    U8(st, 0x00D3B496u) = 0;
    U8(st, 0x00D3B497u) = 0;
    U8(st, 0x00E38E4Cu) = 0;
    S32(st, 0x00D3B518u) = 0;
    S32(st, 0x00D3B508u) = 0;
    S32(st, 0x00D3B510u) = 0;
    S32(st, 0x00E38E14u) = 0;
    U8(st, 0x00E38DA1u) = 1;
    /* @bin 0x004CF990 the resolution: 2 800x600, 3 1024x768, else 640x480 */
    int32_t w = 640, h = 480;
    if (S32(st, RESOLUTION) == 2) {
        w = 800;
        h = 600;
    } else if (S32(st, RESOLUTION) == 3) {
        w = 1024;
        h = 768;
    }
    S32(st, 0x00E38DB0u) = w;
    S32(st, 0x00E38DA8u) = (w - 640) / 2;
    S32(st, 0x00E38DACu) = h;
    S32(st, 0x00E38DA4u) = (h - 480) / 2;
    S32(st, 0x00E38D94u) = 2 * w;
    S32(st, 0x00E38D9Cu) = w;
    S32(st, 0x00E38D98u) = h;
    S32(st, 0x00E38D90u) = h;
    S32(st, 0x00E38D8Cu) = 2 * w * h;
    S32(st, 0x00D3B55Cu) = 0;
    S32(st, 0x00D3B558u) = w;
    S32(st, 0x00D3B554u) = 0;
    S32(st, 0x00D3B550u) = h;
    S32(st, 0x00E38DB8u) = d->width;  /* GetSystemMetrics */
    S32(st, 0x00E38DB4u) = d->height;
    platform_log(st, 0x005E0730u, 0, 0);
    /* @bin 0x004CFAB2 CoInitialize; the window's title (text 9.0); FindWindowA: no other instance */
    text_seek(st, 9, 0);
    /* @bin 0x004CFB25 RegisterClassA ("WinSJBClass", WndProc 0x00414CD0); the cursors of the exe */
    static const struct {
        int32_t id;
        uint32_t at;
    } CURSORS[19] = {
        {0x66, 0x00E390D8u}, {0x88, 0x00E390D4u}, {0x82, 0x00E390D0u}, {0x93, 0x00E390BCu}, {0x90, 0x00E390C4u},
        {0x91, 0x00E390C8u}, {0x92, 0x00E39094u}, {0x8E, 0x00E390CCu}, {0x96, 0x00E390B8u}, {0x95, 0x00E390B0u},
        {0x94, 0x00E390B4u}, {0x99, 0x00E390ACu}, {0x98, 0x00E390A8u}, {0x9B, 0x00E390A4u}, {0x9A, 0x00E390A0u},
        {0x9C, 0x00E3909Cu}, {0x9D, 0x00E39098u}, {0x9F, 0x00E390C0u}, {0xA0, 0x00E39090u},
    };
    for (int i = 0; i < 19; i++) {
        U32(st, CURSORS[i].at) = handle(st, host, BOOT_CURSOR, CURSORS[i].id);
    }
    U32(st, 0x00E39144u) = instance;
    /* @bin 0x004CFCBD CreateWindowExA (the screen's size) */
    uint32_t wnd = handle(st, host, BOOT_WINDOW, 0);
    U32(st, WINDOW) = wnd;
    if (wnd == 0) {
        return 0;
    }
    U32(st, PRIMARY) = 0;
    U32(st, BACK) = 0;
    U32(st, DDRAW) = 0;
    U32(st, CLIPPER) = 0;
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
    int32_t w = S32(st, 0x00E38DB0u), h = S32(st, 0x00E38DACu);
    GAME_CALL(st, 0x00414CD0u, (int32_t) U32(st, WINDOW), 5, 0, (int32_t) ((uint32_t) (uint16_t) h << 16 | (uint16_t) w));
    GAME_CALL(st, 0x00414CD0u, (int32_t) U32(st, WINDOW), 3, 0, 0);
    return 1;
}
