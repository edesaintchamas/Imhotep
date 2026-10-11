/* Imhotep - the game (build/imhotep): SDL2 window, input, sound, video and files around the ported
 * frame loop.
 * Copyright (C) 2026 Imhotep contributors. Licensed under the GNU AGPL-3.0.
 *
 *   build/imhotep [--game DIR] [--scale N] [--fps N] [--hidden]
 *                 [--no-intro] [--no-video] [--no-sound] [--script FILE] [--frames N] [--report FILE]
 *                 [--load SAVE] [-v] [--help] [--version]
 *   tests: [--boot-state BASE] (start from a booted state saved by platform/oracle_boot.py instead of
 *          booting: BASE.state and BASE.heap, the oracle's own boot, to compare with it)
 *          [--det-clock START,STEP] (frame k reads START + k * STEP, window-system events ignored)
 *          [--dump DIR] (back buffer and state after each frame) [--msglog FILE] (the messages
 *          delivered, with the frame they precede: to replay them in the oracle)
 *   --script: one action per line, coordinates in game pixels: wait N (frames) | move X Y |
 *          click X Y | rclick X Y | down X Y | up X Y | key NAME (SDL key name) | text STRING |
 *          shot FILE.bmp (the back buffer) | answer N (the next modal message box, e.g. Esc's "quit
 *          the game?", is answered with its button N, 0 the first: OK / Yes, 1 the second: Cancel /
 *          No, without showing it) | quit | peek ADDR LEN (hex, no frame: state bytes on stdout);
 *          `--script -` reads the lines from stdin and prints "ready FRAME WINDOW" before each
 *          [--dump-window N] (tests: wait while frame k-N's dump is still there: its reader deletes it)
 *          [--dump-delta] (tests: frame_K.sdelta / .rdelta hold the 4 KB blocks of the state file / back
 *          buffer that changed since the previous frame's: "IMDL", u32 size, (u32 offset, u32 length,
 *          bytes)..., u32 0xFFFFFFFF)
 *   --hidden: no window shown; a message box with no scripted answer gets its first button, as the
 *          oracle's (tools/oracle/game.py), and is printed
 *
 * What it does, as WinMain 0x00413ED0 does:
 * - the game directory (--game, IMHOTEP_GAME_DIR, the one remembered from the last start, the usual
 *   install locations: platform/install.h), checked against the reference Pharaoh.exe's SHA-256
 *   (platform/fingerprint.c); the player directory (IMHOTEP_PLAYER_DIR, else the per-user data
 *   directory SDL_GetPrefPath) for saves and progress files (platform/files.c). Nothing depends on
 *   the current directory;
 * - the state: the exe image (platform/exe_image.c) then core's boot (core/boot, game_boot: WinMain's
 *   start-up), with this platform as its host: blocks outside the state come from the platform memory
 *   (platform/memory.h), the screen is a 1024 x 768 one (GetSystemMetrics), the display mode is the
 *   one the game reads from Pharaoh.inf (the player directory's, written at the end of the last run,
 *   else the game's; with none, the original's defaults: 640 x 480 on such a screen). Playing needs
 *   only the game files. Then the platform's own start-up values: the 19 cursor handles (0x004CF960)
 *   and the intro video (0x00413580, as WinMain plays it before its loop). For tests, --boot-state
 *   starts from the oracle's booted state instead (platform/oracle_boot.py writes BASE.state and
 *   BASE.heap; the heap image, what the original allocated outside its segments, is loaded into the
 *   platform memory at the same addresses);
 * - the message loop (0x00414315): pending window messages are delivered to the window procedure's
 *   port (frame_wndproc), translated from SDL events as Windows would deliver them; when none is
 *   pending, one frame (frame_run, the port of 0x004D6B60; through 0x004143C0 when 0x00E39128 is 0
 *   and 0x00E3912C is set); after each, a quit request 0x00D3B43D == 1 becomes WM_CLOSE. The frame
 *   reads the clock itself (platform_clock_ms) and paces the ticks (docs/presentation.md 1.2); the
 *   loop runs one frame per display refresh (vsync; --fps caps it). At 100 % speed the original ticks
 *   once per frame (2 ms minimum), so the game's speed follows the frame rate, as on the original;
 * - the present: 0x004D03F0 (the copy of the back buffer to the screen, a platform request without a
 *   callback yet) is taken over through the call environment: the back buffer (565 or 555 by
 *   0x00E92DFC, 0x00E38DB0 x 0x00E38DAC, pitch 0x00E38D94) goes to an SDL texture shown scaled;
 * - the display mode (core/frame/display.c: the DirectDraw set-up 0x004D0060 at the resolution keys
 *   F7..F9 and F6): set_display makes a new back buffer in the platform memory and resizes the window,
 *   then delivers WM_DISPLAYCHANGE; window_state answers the window procedure's IsIconic /
 *   GetForegroundWindow / GetClientRect (WM_SIZE, WM_MOVE, WM_ACTIVATEAPP, sent from SDL's window
 *   events); memory gives blocks outside the state (the spare screen made again after a mode change);
 *   loaded pixels are converted to RGB 565 when the display is 565 (pixels_load's `convert`);
 * - every call to an original function that has no port is counted (the frontier for play) and
 *   answered 0; --report writes the counts. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL.h>

#include "frame/display.h"
#include "frame/frame.h"
#include "graphics/sg3.h"
#include "present.h"
#include "sim/call.h"
#include "sim/platform.h"
#include "state/ported.h"
#include "state/state.h"

#include "boot/boot.h"

#include "platform/audio.h"
#include "platform/cursors.h"
#include "platform/exe_image.h"
#include "platform/files.h"
#include "platform/fingerprint.h"
#include "platform/install.h"
#include "platform/game_files.h"
#include "platform/memory.h"
#include "platform/video.h"

#define WINDOW_ID 0x00E38DDAu
#define QUIT_REQUEST 0x00D3B43Du
#define HWND 0x00E39140u

#define WM_DESTROY 0x02
#define WM_MOVE 0x03
#define WM_SIZE 0x05
#define WM_ACTIVATE 0x06
#define WM_ACTIVATEAPP 0x1C
#define WM_DISPLAYCHANGE 0x7E
#define SCREEN_W 1024 /* the screen the game starts on (the desktop's mode): GetSystemMetrics */
#define SCREEN_H 768
#define WM_PAINT 0x0F
#define WM_CLOSE 0x10
#define WM_KEYDOWN 0x100
#define WM_KEYUP 0x101
#define WM_CHAR 0x102
#define WM_SYSKEYDOWN 0x104
#define WM_SYSKEYUP 0x105
#define WM_MOUSEMOVE 0x200
#define WM_LBUTTONDOWN 0x201
#define WM_LBUTTONUP 0x202
#define WM_RBUTTONDOWN 0x204
#define WM_RBUTTONUP 0x205

typedef struct {
    uint32_t msg, wparam, lparam;
} message;

static struct {
    game_state st;
    sim_platform plat;
    sim_env env;
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    int tex_fmt;
    int tex_w, tex_h; /* the texture's size: the back buffer's when it was made (a mode change makes another) */
    int w, h;
    int quit;
    int frames;
    message queue[1024];
    int qhead, qcount;
    int mouse_x, mouse_y, buttons;
    uint8_t held[256], pressed[256];
    const char *game;
    /* the unported functions reached (roadmap 8.0): calls to a function without a port, answered 0,
     * and stops inside a ported function at a path it does not port (state_unported); each new one
     * is printed and appended to the player directory's gaps.log */
    const char *player;
    uint32_t un_addr[4096];
    uint64_t un_count[4096];
    int nun;
    const char *report;
    int verbose;
    int no_video;
    int hidden;
    int answers[16]; /* the script's answers to the next message boxes (answer N), in order */
    int nanswers;
    /* testing: a scripted clock (det_step > 0: frame k reads det_start + k * det_step), window-system
     * events ignored, the back buffer dumped after each frame */
    uint32_t det_start, det_step, det_now;
    const char *dump;
    /* the display: the window's scale, the back buffer this platform made (set_display) */
    int scale;
    uint32_t back;
    /* the screen's mode as Windows would have it (what GetSystemMetrics answers): the desktop's (SCREEN_W x
     * SCREEN_H) windowed, the game's mode fullscreen; WM_DISPLAYCHANGE is delivered when it changes */
    int screen_w, screen_h;
} app;

/* ---- the unported calls ---- */

static void gap_logged(uint32_t address, const char *kind)
{
    /* a gap met in play is seen, not silent: once per function, on stderr and in gaps.log */
    fprintf(stderr, "imhotep: gap: 0x%08X %s (window %d, frame %d): the game goes on without it; "
                    "logged in %s/gaps.log\n", address, kind, U8(&app.st, WINDOW_ID), app.frames, app.player);
    char path[2048];
    snprintf(path, sizeof(path), "%s/gaps.log", app.player);
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "0x%08X %s window %d frame %d\n", address, kind, U8(&app.st, WINDOW_ID), app.frames);
        fclose(f);
    }
}

static void count_unported_kind(uint32_t address, const char *kind)
{
    for (int i = 0; i < app.nun; i++) {
        if (app.un_addr[i] == address) {
            app.un_count[i]++;
            return;
        }
    }
    if (app.nun < 4096) {
        app.un_addr[app.nun] = address;
        app.un_count[app.nun++] = 1;
        gap_logged(address, kind);
    }
}

static void count_unported(uint32_t address)
{
    count_unported_kind(address, "not ported, called");
}

static void check_stops(void)
{
    /* a ported function that stopped at a path it does not port (state_unported) */
    if (app.st.unported != 0) {
        count_unported_kind(app.st.unported, "path not ported");
        app.st.unported = 0;
    }
}

static void write_report(void)
{
    if (!app.report) {
        return;
    }
    FILE *f = fopen(app.report, "w");
    if (!f) {
        return;
    }
    /* by count, descending */
    for (int i = 0; i < app.nun; i++) {
        for (int j = i + 1; j < app.nun; j++) {
            if (app.un_count[j] > app.un_count[i]) {
                uint32_t a = app.un_addr[i];
                uint64_t c = app.un_count[i];
                app.un_addr[i] = app.un_addr[j];
                app.un_count[i] = app.un_count[j];
                app.un_addr[j] = a;
                app.un_count[j] = c;
            }
        }
    }
    fprintf(f, "# unported functions reached by build/imhotep (calls answered 0), %d frames\n", app.frames);
    for (int i = 0; i < app.nun; i++) {
        fprintf(f, "0x%08X %llu\n", app.un_addr[i], (unsigned long long) app.un_count[i]);
    }
    fclose(f);
}

/* ---- the screen ---- */

static void present_copy(void);

static void cb_present(void *ctx, uint32_t buffer, int32_t width, int32_t height, int32_t pitch, int32_t rgb565)
{
    (void) ctx;
    (void) buffer;
    (void) width;
    (void) height;
    (void) pitch;
    (void) rgb565;
    present_copy(); /* reads the same values from the state */
}

static void present_copy(void)
{
    /* @bin 0x004D03F0 Blt / BltFast of the back buffer to the primary surface: here to the texture */
    game_state *st = &app.st;
    int w = S32(st, 0x00E38DB0u), h = S32(st, 0x00E38DACu), pitch = S32(st, 0x00E38D94u);
    int fmt = S32(st, 0x00E92DFCu) == 2 ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_RGB555;
    const uint8_t *px = pmem_ptr(U32(st, 0x00E92E18u), (uint32_t) (pitch * h));
    if (!px || w <= 0 || h <= 0 || !app.ren) { /* no renderer yet: the boot's presents, before the window */
        return;
    }
    /* compared with the texture's own size: app.w / app.h already hold the new mode (cb_set_display), so
     * comparing with them kept the old texture after a mode change, which showed the old-size corner of
     * the frame stretched over the window while clicks went to the new layout */
    if (!app.tex || w != app.tex_w || h != app.tex_h || fmt != app.tex_fmt) {
        if (app.tex) {
            SDL_DestroyTexture(app.tex);
        }
        app.tex = SDL_CreateTexture(app.ren, (Uint32) fmt, SDL_TEXTUREACCESS_STREAMING, w, h);
        app.tex_w = w;
        app.tex_h = h;
        app.w = w;
        app.h = h;
        app.tex_fmt = fmt;
        SDL_RenderSetLogicalSize(app.ren, w, h);
        if (app.verbose) {
            fprintf(stderr, "imhotep: texture %dx%d\n", w, h);
        }
    }
    if (app.tex) {
        SDL_UpdateTexture(app.tex, NULL, px, pitch);
    }
}

static void render(void)
{
    SDL_SetRenderDrawColor(app.ren, 0, 0, 0, 255);
    SDL_RenderClear(app.ren);
    if (app.tex) {
        SDL_RenderCopy(app.ren, app.tex, NULL, NULL);
    }
    SDL_RenderPresent(app.ren);
}

static int screenshot(const char *path)
{
    game_state *st = &app.st;
    int w = S32(st, 0x00E38DB0u), h = S32(st, 0x00E38DACu), pitch = S32(st, 0x00E38D94u);
    uint8_t *px = pmem_ptr(U32(st, 0x00E92E18u), (uint32_t) (pitch * h));
    if (!px) {
        return -1;
    }
    Uint32 fmt = S32(st, 0x00E92DFCu) == 2 ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_RGB555;
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(px, w, h, 16, pitch, fmt);
    if (!s) {
        return -1;
    }
    SDL_Surface *c = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGB24, 0);
    int r = c ? SDL_SaveBMP(c, path) : -1;
    SDL_FreeSurface(s);
    if (c) {
        SDL_FreeSurface(c);
    }
    fprintf(stderr, "imhotep: screenshot %s (window %d)\n", path, U8(st, WINDOW_ID));
    return r;
}

static FILE *msglog;
static int dump_window; /* --dump-window N: at most N frames dumped ahead of their reader */

static int dump_delta; /* --dump-delta: each frame's dump holds only the 4 KB blocks that changed */

static void write_delta(const char *path, const uint8_t *buf, size_t len, uint8_t **prev, size_t *prev_len)
{
    /* "IMDL", u32 size, then (u32 offset, u32 length, bytes) for each 4 KB block that differs from the
     * previous dump (all of them the first time), then u32 0xFFFFFFFF. Written to a temporary name
     * and renamed: the reader sees whole files only */
    char tmp[1300];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) {
        return;
    }
    uint32_t n = (uint32_t) len, end = 0xFFFFFFFFu;
    fwrite("IMDL", 1, 4, f);
    fwrite(&n, 4, 1, f);
    for (size_t o = 0; o < len; o += 4096) {
        uint32_t k = (uint32_t) (len - o < 4096 ? len - o : 4096), off = (uint32_t) o;
        if (*prev && *prev_len == len && memcmp(*prev + o, buf + o, k) == 0) {
            continue;
        }
        fwrite(&off, 4, 1, f);
        fwrite(&k, 4, 1, f);
        fwrite(buf + o, 1, k, f);
    }
    fwrite(&end, 4, 1, f);
    fclose(f);
    rename(tmp, path);
    if (*prev_len != len) {
        free(*prev);
        *prev = malloc(len);
        *prev_len = *prev ? len : 0;
    }
    if (*prev) {
        memcpy(*prev, buf, len);
    }
}

/* the state file's bytes in memory (malloc'd): 0, or -1 */
static int state_bytes(const game_state *st, char **buf, size_t *len)
{
#ifdef _WIN32
    /* no open_memstream: through a temporary file */
    FILE *m = tmpfile();
    if (!m) {
        return -1;
    }
    long n = state_write_file(st, m) == 0 ? ftell(m) : -1;
    *buf = n > 0 ? malloc((size_t) n) : NULL;
    rewind(m);
    *len = *buf ? fread(*buf, 1, (size_t) n, m) : 0;
    fclose(m);
    if (!*buf || *len != (size_t) n) {
        free(*buf);
        *buf = NULL;
        return -1;
    }
    return 0;
#else
    FILE *m = open_memstream(buf, len);
    if (!m) {
        return -1;
    }
    state_write_file(st, m);
    fclose(m);
    return *buf ? 0 : -1;
#endif
}

static void dump_frame(void)
{
    game_state *st = &app.st;
    if (msglog) {
        fflush(msglog); /* a reader of the dump also reads the messages of the frames it has */
    }
    int h = S32(st, 0x00E38DACu), pitch = S32(st, 0x00E38D94u);
    const uint8_t *px = pmem_ptr(U32(st, 0x00E92E18u), (uint32_t) (pitch * h));
    char path[1200];
    const char *ext = dump_delta ? "sdelta" : "state";
    if (dump_delta) {
        static uint8_t *prev_raw, *prev_state;
        static size_t prev_raw_len, prev_state_len;
        char *buf = NULL;
        size_t len = 0;
        if (px) {
            snprintf(path, sizeof(path), "%s/frame_%04d.rdelta", app.dump, app.frames);
            write_delta(path, px, (size_t) (pitch * h), &prev_raw, &prev_raw_len);
        }
        if (state_bytes(st, &buf, &len) == 0) {
            snprintf(path, sizeof(path), "%s/frame_%04d.sdelta", app.dump, app.frames);
            write_delta(path, (const uint8_t *) buf, len, &prev_state, &prev_state_len);
            free(buf);
        }
    } else {
        snprintf(path, sizeof(path), "%s/frame_%04d.raw", app.dump, app.frames);
        FILE *f = px ? fopen(path, "wb") : NULL;
        if (f) {
            fwrite(px, 1, (size_t) (pitch * h), f);
            fclose(f);
        }
        snprintf(path, sizeof(path), "%s/frame_%04d.state", app.dump, app.frames);
        f = fopen(path, "wb");
        if (f) {
            state_write_file(st, f);
            fclose(f);
        }
    }
    if (dump_window > 0 && app.frames > dump_window) {
        /* tests (--dump-window): the reader deletes the frames it has compared; wait for it to be
         * at most N frames behind (the clock is scripted, so waiting changes nothing in the game) */
        struct stat sb;
        snprintf(path, sizeof(path), "%s/frame_%04d.%s", app.dump, app.frames - dump_window, ext);
        while (stat(path, &sb) == 0) {
            SDL_Delay(2);
        }
    }
}

/* ---- the call environment: the present, and the unported functions ---- */

static int env_external(const sim_env *env, game_state *st, uint32_t address, const int32_t *args, int nargs,
                        uint32_t this_ptr, int32_t *result)
{
    (void) env;
    (void) this_ptr;
    const ported_function *p = ported_find(address);
    if (p && p->run) {
        return 0;
    }
    if ((address == 0x00561A2Bu || address == 0x00561BDCu) && nargs >= 1) {
        /* operator delete / CRT free (ported inline: core/state/ported.c) called by a port (the
         * external image buffer 0x0053F790): a block of the simulation heap or of the platform memory
         * released (display_memory) */
        if (args[0] != 0) {
            display_memory(st, (uint32_t) args[0], 0);
        }
        *result = 0;
        return 1;
    }
    count_unported(address);
    *result = 0;
    return 1;
}

/* ---- window messages ---- */

static void post(uint32_t msg, uint32_t wparam, uint32_t lparam)
{
    if (app.qcount == (int) (sizeof(app.queue) / sizeof(app.queue[0]))) {
        return;
    }
    app.queue[(app.qhead + app.qcount++) % 1024] = (message){msg, wparam, lparam};
}

/* SDL key -> Windows virtual-key code */
static int vk_of(SDL_Keycode k)
{
    if (k >= SDLK_a && k <= SDLK_z) {
        return 'A' + (k - SDLK_a);
    }
    if (k >= SDLK_0 && k <= SDLK_9) {
        return '0' + (k - SDLK_0);
    }
    if (k >= SDLK_F1 && k <= SDLK_F12) {
        return 0x70 + (k - SDLK_F1);
    }
    if (k >= SDLK_KP_1 && k <= SDLK_KP_9) {
        return 0x61 + (k - SDLK_KP_1);
    }
    switch (k) {
    case SDLK_KP_0: return 0x60;
    case SDLK_BACKSPACE: return 0x08;
    case SDLK_TAB: return 0x09;
    case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0D;
    case SDLK_LSHIFT: case SDLK_RSHIFT: return 0x10;
    case SDLK_LCTRL: case SDLK_RCTRL: case SDLK_LGUI: case SDLK_RGUI: return 0x11; /* Cmd as Ctrl */
    case SDLK_LALT: case SDLK_RALT: return 0x12;
    case SDLK_PAUSE: return 0x13;
    case SDLK_CAPSLOCK: return 0x14;
    case SDLK_ESCAPE: return 0x1B;
    case SDLK_SPACE: return 0x20;
    case SDLK_PAGEUP: return 0x21;
    case SDLK_PAGEDOWN: return 0x22;
    case SDLK_END: return 0x23;
    case SDLK_HOME: return 0x24;
    case SDLK_LEFT: return 0x25;
    case SDLK_UP: return 0x26;
    case SDLK_RIGHT: return 0x27;
    case SDLK_DOWN: return 0x28;
    case SDLK_INSERT: return 0x2D;
    case SDLK_DELETE: return 0x2E;
    case SDLK_KP_MULTIPLY: return 0x6A;
    case SDLK_KP_PLUS: return 0x6B;
    case SDLK_KP_MINUS: return 0x6D;
    case SDLK_KP_PERIOD: return 0x6E;
    case SDLK_KP_DIVIDE: return 0x6F;
    case SDLK_SEMICOLON: return 0xBA;
    case SDLK_EQUALS: return 0xBB;
    case SDLK_COMMA: return 0xBC;
    case SDLK_MINUS: return 0xBD;
    case SDLK_PERIOD: return 0xBE;
    case SDLK_SLASH: return 0xBF;
    case SDLK_BACKQUOTE: return 0xC0;
    case SDLK_LEFTBRACKET: return 0xDB;
    case SDLK_BACKSLASH: return 0xDC;
    case SDLK_RIGHTBRACKET: return 0xDD;
    case SDLK_QUOTE: return 0xDE;
    default: return 0;
    }
}

static uint32_t mouse_lparam(int x, int y)
{
    return (uint32_t) (uint16_t) y << 16 | (uint16_t) x;
}

static uint32_t mk_flags(void)
{
    /* MK_LBUTTON 1, MK_RBUTTON 2, MK_SHIFT 4, MK_CONTROL 8 */
    SDL_Keymod m = SDL_GetModState();
    return (uint32_t) (app.buttons & 3) | (m & KMOD_SHIFT ? 4u : 0u) | (m & (KMOD_CTRL | KMOD_GUI) ? 8u : 0u);
}

static void set_mouse(int x, int y)
{
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (app.w > 0 && x >= app.w) {
        x = app.w - 1;
    }
    if (app.h > 0 && y >= app.h) {
        y = app.h - 1;
    }
    app.mouse_x = x;
    app.mouse_y = y;
}

/* Windows' messages for an SDL event (TranslateMessage included: WM_CHAR) */
static void translate(const SDL_Event *e)
{
    switch (e->type) {
    case SDL_QUIT:
        post(WM_CLOSE, 0, 0);
        break;
    case SDL_WINDOWEVENT:
        if (app.det_step) {
            break;
        }
        /* as Windows sends them: activation of the application and of the window, WM_SIZE with
         * SIZE_MINIMIZED / SIZE_RESTORED and the client size, WM_MOVE (the window procedure asks
         * the rest through window_state) */
        switch (e->window.event) {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            post(WM_ACTIVATEAPP, 1, 0);
            post(WM_ACTIVATE, 1, 0); /* WA_ACTIVE */
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            post(WM_ACTIVATE, 0, 0); /* WA_INACTIVE */
            post(WM_ACTIVATEAPP, 0, 0);
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            post(WM_ACTIVATE, 1u << 16, 0); /* minimised */
            post(WM_SIZE, 1, 0);           /* SIZE_MINIMIZED */
            break;
        case SDL_WINDOWEVENT_RESTORED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            post(WM_SIZE, 0, (uint32_t) (uint16_t) app.h << 16 | (uint16_t) app.w);
            break;
        case SDL_WINDOWEVENT_MOVED:
            post(WM_MOVE, 0, 0);
            break;
        case SDL_WINDOWEVENT_EXPOSED: post(WM_PAINT, 0, 0); break;
        default: break;
        }
        break;
    case SDL_MOUSEMOTION:
        set_mouse(e->motion.x, e->motion.y);
        post(WM_MOUSEMOVE, mk_flags(), mouse_lparam(app.mouse_x, app.mouse_y));
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        int down = e->type == SDL_MOUSEBUTTONDOWN;
        set_mouse(e->button.x, e->button.y);
        int bit = e->button.button == SDL_BUTTON_LEFT ? 1 : e->button.button == SDL_BUTTON_RIGHT ? 2 : 0;
        if (!bit) {
            break; /* the middle button is not handled by the original */
        }
        app.buttons = down ? app.buttons | bit : app.buttons & ~bit;
        app.held[bit] = (uint8_t) down; /* VK_LBUTTON 1, VK_RBUTTON 2 */
        if (down) {
            app.pressed[bit] = 1;
        }
        uint32_t msg = bit == 1 ? (down ? WM_LBUTTONDOWN : WM_LBUTTONUP) : (down ? WM_RBUTTONDOWN : WM_RBUTTONUP);
        post(msg, mk_flags(), mouse_lparam(app.mouse_x, app.mouse_y));
        break;
    }
    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        int down = e->type == SDL_KEYDOWN;
        int vk = vk_of(e->key.keysym.sym);
        if (!vk) {
            break;
        }
        app.held[vk] = (uint8_t) down;
        if (down) {
            app.pressed[vk] = 1;
        }
        int alt = (e->key.keysym.mod & KMOD_ALT) != 0;
        int ctrl = (e->key.keysym.mod & (KMOD_CTRL | KMOD_GUI)) != 0;
        uint32_t lparam = 1u | (e->key.repeat ? 1u << 30 : 0) | (down ? 0 : 3u << 30);
        if (alt) {
            lparam |= 1u << 29;
            post(down ? WM_SYSKEYDOWN : WM_SYSKEYUP, (uint32_t) vk, lparam);
            break;
        }
        post(down ? WM_KEYDOWN : WM_KEYUP, (uint32_t) vk, lparam);
        if (down) {
            /* characters that SDL gives no text event for */
            if (vk == 0x08 || vk == 0x09 || vk == 0x0D || vk == 0x1B) {
                post(WM_CHAR, (uint32_t) vk, lparam);
            } else if (ctrl && vk >= 'A' && vk <= 'Z') {
                post(WM_CHAR, (uint32_t) (vk - 'A' + 1), lparam);
            }
        }
        break;
    }
    case SDL_TEXTINPUT: {
        SDL_Keymod m = SDL_GetModState();
        if (m & (KMOD_CTRL | KMOD_GUI | KMOD_ALT)) {
            break;
        }
        /* UTF-8 -> the game's code page (Windows-1252: code points below 256) */
        const unsigned char *s = (const unsigned char *) e->text.text;
        while (*s) {
            uint32_t c = *s++;
            if (c >= 0xC0 && c < 0xE0 && (*s & 0xC0) == 0x80) {
                c = (c & 0x1F) << 6 | (*s++ & 0x3F);
            } else if (c >= 0x80) {
                while ((*s & 0xC0) == 0x80) {
                    s++;
                }
                continue;
            }
            if (c < 256) {
                post(WM_CHAR, c, 1);
            }
        }
        break;
    }
    default:
        break;
    }
}

static void pump(void)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        translate(&e);
    }
}

static void dispatch(message m)
{
    game_state *st = &app.st;
    if (msglog) { /* tests: the messages and the frame they precede, for a replay in the oracle */
        fprintf(msglog, "%d 0x%X 0x%X 0x%X %d %d\n", app.frames + 1, m.msg, m.wparam, m.lparam, app.mouse_x, app.mouse_y);
    }
    frame_wndproc(st, U32(st, HWND), m.msg, m.wparam, m.lparam);
    if (m.msg == WM_CLOSE) {
        /* DefWindowProcA(WM_CLOSE) destroys the window: WM_DESTROY, whose handler ends with
         * PostQuitMessage (the platform's): GetMessageA returns 0, WinMain leaves its loop */
        frame_wndproc(st, U32(st, HWND), WM_DESTROY, 0, 0);
        app.quit = 1;
    }
}

static void after_message(void)
{
    check_stops();
    /* @bin 0x0041434E a quit request becomes WM_CLOSE (PostMessageA) */
    if (U8(&app.st, QUIT_REQUEST) == 1) {
        post(WM_CLOSE, 0, 0);
        U8(&app.st, QUIT_REQUEST) = 0;
    }
}

/* ---- the sim_platform callbacks ---- */

static char host_path[2048];

static const char *host(const char *gpath)
{
    return gpath && files_resolve(gpath, host_path, sizeof(host_path)) == 0 ? host_path : NULL;
}

static int cb_video_play(void *ctx, const char *name, int32_t a, int32_t b, int32_t c)
{
    (void) ctx;
    /* Smacker: no .smk file is in the GOG install (docs/presentation.md 7); not decoded here */
    fprintf(stderr, "imhotep: Smacker video %s (%d %d %d) not played\n", name, a, b, c);
    return 0;
}

/* the Bink video opened by video_open, played by video_modal (core/sim/platform.c, 0x00413580) */
static video *opened_video;
static const char *opened_name;

static int cb_video_open(void *ctx, const char *name, int32_t *width, int32_t *height)
{
    (void) ctx;
    const char *p = host(name);
    if (!p || app.no_video) {
        fprintf(stderr, "imhotep: video %s: %s\n", name, p ? "skipped (--no-video)" : "no such file");
        return 0;
    }
    int w = 0, h = 0;
    opened_video = video_open_bink(p, &w, &h);
    if (!opened_video) {
        fprintf(stderr, "imhotep: video %s: cannot be opened\n", name);
        return 0;
    }
    opened_name = name;
    *width = w;
    *height = h;
    return 1;
}

static void cb_video_modal(void *ctx, int32_t x, int32_t y)
{
    (void) ctx;
    if (!opened_video) {
        return;
    }
    video_target t = {app.ren, app.w ? app.w : S32(&app.st, 0x00E38DB0u), app.h ? app.h : S32(&app.st, 0x00E38DACu),
                      0, 0, NULL, 0};
    const char *max = getenv("IMHOTEP_VIDEO_TEST_MS"); /* tests: stop the video, save the window */
    t.max_ms = max ? atoi(max) : 0;
    t.shot = getenv("IMHOTEP_VIDEO_SHOT");
    Uint32 t0 = SDL_GetTicks();
    video *v = opened_video;
    opened_video = NULL;
    video_play_opened(v, &t, x, y); /* without a window (renderer) it is closed unseen */
    if (app.verbose) {
        fprintf(stderr, "imhotep: video %s at %d,%d: %d frames shown in %u ms\n", opened_name, x, y, t.frames_shown,
                SDL_GetTicks() - t0);
    }
    if (t.quit) {
        post(WM_CLOSE, 0, 0);
    }
}

static void cb_video_close(void *ctx)
{
    (void) ctx;
}

static int32_t cb_stream_play(void *ctx, const char *name, int32_t channel, int32_t loops, int32_t volume)
{
    (void) ctx;
    const char *p = host(name);
    if (!p) {
        fprintf(stderr, "imhotep: stream %s: no such file\n", name);
        return 0;
    }
    int32_t h = audio_stream_play(p, channel, loops, volume);
    if (app.verbose) {
        fprintf(stderr, "imhotep: stream %d %s loops %d volume %d -> %d\n", channel, name, loops, volume, h);
    }
    return h;
}

static int cb_stream_playing(void *ctx, int32_t channel)
{
    (void) ctx;
    return audio_stream_playing(channel);
}

static void cb_stream_stop(void *ctx, int32_t channel)
{
    (void) ctx;
    audio_stream_stop(channel);
}

static int32_t cb_stream_volume(void *ctx, int32_t channel)
{
    (void) ctx;
    return audio_stream_volume(channel);
}

static void cb_stream_set_volume(void *ctx, int32_t channel, int32_t volume)
{
    (void) ctx;
    audio_stream_set_volume(channel, volume);
}

static void cb_sample_play(void *ctx, int32_t sound, const char *name, int32_t volume, int32_t pan, int32_t loops)
{
    (void) ctx;
    const char *p = host(name);
    if (app.verbose) {
        fprintf(stderr, "imhotep: sample %d %s volume %d pan %d loops %d%s\n", sound, name ? name : "-", volume, pan,
                loops, p ? "" : " (no file)");
    }
    if (p) {
        audio_sample_play(sound, p, volume, pan, loops);
    }
}

static void cb_log(void *ctx, const char *message, const char *text, int32_t value)
{
    (void) ctx;
    if (app.verbose) {
        fprintf(stderr, "imhotep: log: %s %s %d\n", message ? message : "", text ? text : "", value);
    }
}

/* Windows-1252 -> UTF-8 (the game's texts) */
static void to_utf8(const char *s, char *out, size_t size)
{
    size_t k = 0;
    for (; s && *s && k + 3 < size; s++) {
        unsigned char c = (unsigned char) *s;
        if (c < 0x80) {
            out[k++] = (char) c;
        } else {
            out[k++] = (char) (0xC0 | c >> 6);
            out[k++] = (char) (0x80 | (c & 0x3F));
        }
    }
    out[k] = 0;
}

static int cb_message_box(void *ctx, const char *title, const char *text, int32_t kind, int32_t buttons)
{
    (void) ctx;
    /* the original's kinds (0x00538940) are the Win32 MB_ types (core/sim/platform.c) */
    static const char *labels[6][3] = {{"OK"},          {"OK", "Cancel"}, {"Abort", "Retry", "Ignore"},
                                       {"Yes", "No", "Cancel"}, {"Yes", "No"}, {"Retry", "Cancel"}};
    int n = buttons < 1 ? 1 : buttons > 3 ? 3 : buttons;
    if (kind < 0 || kind > 5) {
        kind = 0;
    }
    SDL_MessageBoxButtonData b[3];
    for (int i = 0; i < n; i++) {
        b[i].flags = i == 0 ? SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT : i == n - 1 ? SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT : 0;
        b[i].buttonid = i;
        b[i].text = labels[kind][i] ? labels[kind][i] : "OK";
    }
    char t[512], m[2048];
    to_utf8(title ? title : "Pharaoh", t, sizeof(t));
    to_utf8(text ? text : "", m, sizeof(m));
    SDL_MessageBoxData d = {SDL_MESSAGEBOX_INFORMATION, app.win, t, m, n, b, NULL};
    int pressed = 0;
    if (app.nanswers > 0 || app.hidden) {
        /* scripted (answer N) or hidden: not shown, no wait for a player */
        if (app.nanswers > 0) {
            pressed = app.answers[0];
            memmove(app.answers, app.answers + 1, sizeof(app.answers[0]) * (size_t) --app.nanswers);
        }
        if (pressed < 0 || pressed >= n) {
            pressed = 0;
        }
        fprintf(stderr, "imhotep: message box \"%s\": \"%s\" -> %s\n", t, m, b[pressed].text);
    } else if (SDL_ShowMessageBox(&d, &pressed) != 0 || pressed < 0) {
        pressed = 0;
    }
    /* the keys and button held when the box opened are released by then */
    app.buttons = 0;
    return pressed;
}

static int32_t cb_file_read(void *ctx, const char *path, uint32_t offset, uint8_t *buf, uint32_t size)
{
    (void) ctx;
    return files_read(path, offset, buf, size);
}

static uint32_t cb_pixels_load(void *ctx, const char *path, uint32_t skip, uint32_t size, int32_t convert, int32_t rle)
{
    (void) ctx;
    /* @bin 0x0053F660 the .555 data from `skip` on into a buffer of the platform's memory; its game
     * address is the handle the state keeps. With `convert`, the pixels become RGB 565 when the
     * display is (0x00E92DFC == 2): the RLE stream (rle, 0x004CDEB0) or plain pixels (0x004CDF20),
     * core's ports (core/graphics/sg3.c) on the buffer, which the state maps */
    uint32_t a = pmem_alloc(size);
    uint8_t *p = a ? pmem_ptr(a, size) : NULL;
    if (!p) {
        fprintf(stderr, "imhotep: pixels %s: out of memory (%u bytes)\n", path, size);
        return 0;
    }
    int32_t n = files_read(path, skip, p, size);
    if (n < 0) {
        pmem_free(a);
        return 0;
    }
    if (convert) {
        if (rle) {
            sg3_rle_555_to_565(&app.st, a, (int32_t) size);
        } else {
            sg3_pixels_555_to_565(&app.st, a, (int32_t) size);
        }
    }
    return a;
}

static void cb_pixels_free(void *ctx, uint32_t handle)
{
    (void) ctx;
    pmem_free(handle);
}

static void cb_open_url(void *ctx, const char *url)
{
    (void) ctx;
    SDL_OpenURL(url);
}

static void cb_post_message(void *ctx, uint32_t msg, uint32_t wparam, uint32_t lparam)
{
    (void) ctx;
    post(msg, wparam, lparam);
}

static void cb_quit(void *ctx, int32_t code)
{
    (void) ctx;
    (void) code;
    app.quit = 1;
}

static int32_t cb_file_delete(void *ctx, const char *path)
{
    (void) ctx;
    return files_delete(path);
}

static int cb_cursor_pos(void *ctx, int32_t *x, int32_t *y)
{
    (void) ctx;
    /* GetCursorPos: the game's window covers the screen (fullscreen), so screen = client */
    *x = app.mouse_x;
    *y = app.mouse_y;
    return 1;
}

static int16_t cb_key_state(void *ctx, int32_t vk)
{
    (void) ctx;
    if (vk < 0 || vk > 255) {
        return 0;
    }
    int16_t r = (int16_t) ((app.held[vk] ? 0x8000 : 0) | (app.pressed[vk] ? 1 : 0));
    app.pressed[vk] = 0;
    return r;
}

static int cb_message_pending(void *ctx, uint32_t first, uint32_t last, int32_t remove)
{
    (void) ctx;
    pump();
    for (int i = 0; i < app.qcount; i++) {
        message m = app.queue[(app.qhead + i) % 1024];
        if ((first == 0 && last == 0) || (m.msg >= first && m.msg <= last)) {
            if (remove) {
                for (int j = i; j > 0; j--) {
                    app.queue[(app.qhead + j) % 1024] = app.queue[(app.qhead + j - 1) % 1024];
                }
                app.qhead = (app.qhead + 1) % 1024;
                app.qcount--;
            }
            return 1;
        }
    }
    return 0;
}

static uint32_t cb_clock_ms(void *ctx)
{
    (void) ctx;
    if (app.det_step) {
        return app.det_now;
    }
    return SDL_GetTicks(); /* timeGetTime / GetTickCount */
}

static int32_t cb_file_write(void *ctx, const char *path, const uint8_t *data, uint32_t size)
{
    (void) ctx;
    return files_write(path, data, size);
}

static int32_t cb_file_list(void *ctx, const char *pattern, int32_t index, char *name, uint32_t size)
{
    (void) ctx;
    return files_list(pattern, index, name, size);
}

static uint32_t cb_set_display(void *ctx, int32_t width, int32_t height, int32_t rgb565)
{
    /* the display mode (core/frame/display.c, 0x004D0060): a new back buffer of width x height in the
     * platform memory (the previous one this platform made released: the original's surfaces were
     * released first, 0x00426EE0), the window resized to the mode, WM_DISPLAYCHANGE delivered as
     * Windows does after a mode change. The texture follows the state's size and format at the next
     * present (present_copy); the format stays the game's (rgb565 is the one it asks) */
    (void) ctx;
    (void) rgb565;
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        return 0;
    }
    uint32_t a = pmem_alloc((uint32_t) (width * height * 2));
    if (a == 0) {
        return 0;
    }
    if (app.back) {
        pmem_free(app.back);
    }
    app.back = a;
    /* Windows sends WM_DISPLAYCHANGE only when the screen's mode changes (as the boot assumes:
     * core/boot/display.c): fullscreen, DirectDraw's SetDisplayMode sets the game's mode; windowed
     * (0x00E38E5C == 0) the game sets no mode, and leaving the full screen (SetCooperativeLevel normal)
     * gives the desktop's mode back (assumed: Windows' behaviour). The same mode set again (gfxBegin after
     * WM_ACTIVATEAPP) sends none */
    int full = S32(&app.st, 0x00E38E5Cu) != 0;
    int sw = full ? width : SCREEN_W, sh = full ? height : SCREEN_H;
    int changed = sw != app.screen_w || sh != app.screen_h;
    app.screen_w = sw;
    app.screen_h = sh;
    app.w = width;
    app.h = height;
    if (app.win) {
        SDL_SetWindowSize(app.win, width * app.scale, height * app.scale);
        SDL_RenderSetLogicalSize(app.ren, width, height);
    }
    fprintf(stderr, "imhotep: display %dx%d %s\n", width, height, rgb565 ? "565" : "555");
    if (changed) {
        post(WM_DISPLAYCHANGE, 16, (uint32_t) (uint16_t) sh << 16 | (uint16_t) sw);
    }
    return a;
}

static int cb_window_state(void *ctx, int32_t *iconic, int32_t *foreground, int32_t rect[4])
{
    /* IsIconic, GetForegroundWindow, GetClientRect + ClientToScreen: the client area is the mode's
     * size in game pixels (the window shows the back buffer scaled; the cursor is given in game
     * pixels). Tests (--det-clock): shown and in front, whatever the host does */
    (void) ctx;
    Uint32 f = app.win ? SDL_GetWindowFlags(app.win) : 0;
    *iconic = !app.det_step && (f & SDL_WINDOW_MINIMIZED) != 0;
    /* before the SDL window exists (the boot's WM_SIZE / WM_MOVE) the game's window is the one being
     * shown, in front */
    *foreground = app.det_step || !app.win || (f & SDL_WINDOW_INPUT_FOCUS) != 0;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = S32(&app.st, 0x00E38DB0u);
    rect[3] = S32(&app.st, 0x00E38DACu);
    return 1;
}

static uint32_t cb_memory(void *ctx, uint32_t address, uint32_t size)
{
    /* blocks outside the state (the oracle's other heap): the platform memory */
    (void) ctx;
    if (size == 0) {
        pmem_free(address);
        return 0;
    }
    return pmem_alloc(size);
}

/* ---- scripted input (tests): SDL events pushed as a player would make them ---- */

static FILE *script;
static int script_wait;
static struct pending_click {
    int on, x, y, button;
} pending_up, pending_down;
static char pending_text[480];
static int pending_pos;

static void push_mouse(int type, int x, int y, int button)
{
    int wx = x, wy = y;
    float lx = (float) x + 0.5f, ly = (float) y + 0.5f;
    SDL_RenderLogicalToWindow(app.ren, lx, ly, &wx, &wy);
    SDL_Event e = {0};
    e.type = (Uint32) type;
    if (type == SDL_MOUSEMOTION) {
        e.motion.windowID = SDL_GetWindowID(app.win);
        e.motion.x = wx;
        e.motion.y = wy;
    } else {
        e.button.windowID = SDL_GetWindowID(app.win);
        e.button.button = (Uint8) button;
        e.button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
        e.button.clicks = 1;
        e.button.x = wx;
        e.button.y = wy;
    }
    SDL_PushEvent(&e);
}

static void push_key(const char *name, int down)
{
    SDL_Event e = {0};
    e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    e.key.windowID = SDL_GetWindowID(app.win);
    e.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    e.key.keysym.sym = SDL_GetKeyFromName(name);
    e.key.keysym.scancode = SDL_GetScancodeFromKey(e.key.keysym.sym);
    SDL_PushEvent(&e);
}

/* one script line per call when no wait is pending:
 *   wait N | move X Y | click X Y | rclick X Y | down X Y | up X Y | key NAME | text STRING |
 *   shot PATH | quit   (coordinates in game pixels; click = move, 2 frames, down, 2 frames, up) */
static void script_step(void)
{
    if (script_wait > 0) {
        script_wait--;
        return;
    }
    if (pending_down.on) { /* a click's press, two frames after the move */
        push_mouse(SDL_MOUSEBUTTONDOWN, pending_down.x, pending_down.y, pending_down.button);
        pending_down.on = 0;
        script_wait = 2;
        return;
    }
    if (pending_up.on) { /* a click's release, two frames after its press */
        push_mouse(SDL_MOUSEBUTTONUP, pending_up.x, pending_up.y, pending_up.button);
        pending_up.on = 0;
        script_wait = 1;
        return;
    }
    if (pending_text[pending_pos]) { /* typed text: one character every other frame, as a player types */
        SDL_Event e = {0};
        e.type = SDL_TEXTINPUT;
        e.text.windowID = SDL_GetWindowID(app.win);
        e.text.text[0] = pending_text[pending_pos++];
        SDL_PushEvent(&e);
        script_wait = 1;
        return;
    }
    if (!script) {
        return;
    }
    char line[512];
    char cmd[32], arg[480];
    int x, y;
    for (;;) {
        if (script == stdin) {
            /* a driver on stdin (--script -, tests: tools/oracle/fuzz.py): before each line, the frame
             * and the window it can choose from */
            printf("ready %d %d\n", app.frames, U8(&app.st, WINDOW_ID));
            fflush(stdout);
        }
        if (!fgets(line, sizeof(line), script)) {
            if (script != stdin) {
                fclose(script);
            }
            script = NULL;
            return;
        }
        line[strcspn(line, "\r\n")] = 0;
        memset(cmd, 0, sizeof(cmd));
        memset(arg, 0, sizeof(arg));
        x = y = 0;
        sscanf(line, "%31s %479[^\n]", cmd, arg);
        sscanf(arg, "%d %d", &x, &y);
        if (strcmp(cmd, "peek") != 0 && cmd[0] != '#') {
            break;
        }
        if (cmd[0] == 'p') {
            /* peek ADDR LEN: state bytes in hex on stdout; takes no frame (a session reads the same
             * with or without its peeks) */
            unsigned a = 0, n = 0;
            sscanf(arg, "%x %u", &a, &n);
            printf("peek %08X ", a);
            const uint8_t *q = n > 0 && n <= 65536 ? state_ptr(&app.st, a, n) : NULL;
            for (unsigned i = 0; q && i < n; i++) {
                printf("%02X", q[i]);
            }
            printf("\n");
            fflush(stdout);
        }
    }
    if (app.verbose && cmd[0]) {
        fprintf(stderr, "imhotep: script: %s\n", line);
    }
    if (strcmp(cmd, "wait") == 0) {
        script_wait = x;
    } else if (strcmp(cmd, "move") == 0) {
        push_mouse(SDL_MOUSEMOTION, x, y, 0);
        script_wait = 1;
    } else if (strcmp(cmd, "down") == 0 || strcmp(cmd, "up") == 0) {
        push_mouse(SDL_MOUSEMOTION, x, y, 0);
        push_mouse(cmd[0] == 'd' ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP, x, y, SDL_BUTTON_LEFT);
        script_wait = 1;
    } else if (strcmp(cmd, "click") == 0 || strcmp(cmd, "rclick") == 0) {
        int b = cmd[0] == 'r' ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
        push_mouse(SDL_MOUSEMOTION, x, y, 0);
        pending_down = (struct pending_click){1, x, y, b};
        pending_up = (struct pending_click){1, x, y, b};
        script_wait = 2;
    } else if (strcmp(cmd, "key") == 0) {
        push_key(arg, 1);
        push_key(arg, 0);
        script_wait = 1;
    } else if (strcmp(cmd, "text") == 0) {
        snprintf(pending_text, sizeof(pending_text), "%s", arg);
        pending_pos = 0;
    } else if (strcmp(cmd, "shot") == 0) {
        screenshot(arg);
    } else if (strcmp(cmd, "answer") == 0) {
        if (app.nanswers < 16) {
            app.answers[app.nanswers++] = x;
            if (msglog) { /* tests: the answer queued before that frame (not a message: 0xFFFFFFFF) */
                fprintf(msglog, "%d 0xFFFFFFFF 0x%X 0x0 %d %d\n", app.frames + 1, (unsigned) x, app.mouse_x,
                        app.mouse_y);
            }
        }
    } else if (strcmp(cmd, "quit") == 0) {
        post(WM_CLOSE, 0, 0);
    }
}

/* ---- start-up ---- */

static int read_boot_state(const char *base)
{
    char path[1100];
    snprintf(path, sizeof(path), "%s.state", base);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "imhotep: no booted state %s (--boot-state: made by platform/oracle_boot.py)\n", path);
        return -1;
    }
    int r = state_read_file(&app.st, f);
    fclose(f);
    if (r != 0) {
        fprintf(stderr, "imhotep: %s: not a state file\n", path);
        return -1;
    }
    snprintf(path, sizeof(path), "%s.heap", base);
    size_t len;
    uint8_t *heap = read_file(path, &len);
    if (!heap || pmem_preload(heap, (uint32_t) len) != 0) {
        fprintf(stderr, "imhotep: %s: cannot load\n", path);
        free(heap);
        return -1;
    }
    free(heap);
    return 0;
}

/* the boot's host (core/boot/boot.h): its blocks outside the state come from the platform memory */
static uint32_t host_alloc(void *ctx, game_state *st, uint32_t size, uint32_t site)
{
    (void) ctx;
    (void) st;
    (void) site;
    return pmem_alloc(size);
}

static void host_release(void *ctx, game_state *st, uint32_t address)
{
    (void) ctx;
    (void) st;
    pmem_free(address);
}

static uint32_t host_surface(void *ctx, game_state *st, int32_t w, int32_t h, int32_t primary)
{
    (void) ctx;
    (void) st;
    uint32_t a = w > 0 && h > 0 ? pmem_alloc((uint32_t) (w * h * 2)) : 0;
    if (!primary) {
        app.back = a; /* released by the next display mode change (cb_set_display) */
    }
    return a;
}

static int port_boot(int sound)
{
    boot_host host = {.ctx = NULL, .alloc = host_alloc, .release = host_release, .surface = host_surface};
    /* the screen before the game sets its mode (GetSystemMetrics): 1024 x 768, the largest mode (the
     * window then takes the mode's size); 16-bit pixels in RGB 555. The mode is the game's
     * (Pharaoh.inf, read by the boot's settings: core/boot/settings.c) */
    boot_display display = {.width = SCREEN_W, .height = SCREEN_H, .format = 555};
    int n = boot_step_count(&display, sound), r = 0;
    for (int k = 0; k < n && r == 0; k++) {
        int before = app.nun;
        r = boot_step(&app.st, &host, &display, sound, k);
        if (r != 0 || app.nun != before || app.verbose) {
            fprintf(stderr, "imhotep: boot step %d %s: %s%s\n", k, boot_step_name(&display, sound, k),
                    r == 0 ? "ok" : "failed", app.nun != before ? " (unported functions reached)" : "");
        }
    }
    return r;
}

/* a string in the platform memory, for a game call that takes one */
static uint32_t game_string(const char *s)
{
    uint32_t a = pmem_alloc((uint32_t) strlen(s) + 1);
    uint8_t *p = a ? pmem_ptr(a, (uint32_t) strlen(s) + 1) : NULL;
    if (p) {
        memcpy(p, s, strlen(s) + 1);
    }
    return a;
}

static void usage(FILE *out)
{
    fprintf(out,
            "usage: imhotep [options]\n"
            "Imhotep %s, the engine of Pharaoh + Cleopatra; it plays with the files of an installation of the game\n"
            "(GOG version 2.1).\n"
            "\n"
            "  --game DIR       the game's installation folder (the one holding Pharaoh.exe); else IMHOTEP_GAME_DIR,\n"
            "                   else the folder found at the last start, else the usual install locations\n"
            "  --scale N        window size: N times the game's resolution\n"
            "  --fps N          cap the frame rate (the game's speed follows it, as on the original)\n"
            "  --no-intro       skip the intro video\n"
            "  --no-video       play no video\n"
            "  --no-sound       no sound\n"
            "  --load SAVE      start in this saved game (a game path, e.g. \"Save\\Name\\city.sav\")\n"
            "  -v               print the start-up steps\n"
            "  --help, --version\n"
            "\n"
            "Saves and settings are kept in IMHOTEP_PLAYER_DIR, else in the per-user data directory\n"
            "(SDL_GetPrefPath \"Imhotep\"); the game folder is only read.\n"
            "\n"
            "tests (platform/main.c describes them):\n"
            "  --hidden  --frames N  --report FILE  --script FILE|-  --boot-state BASE  --det-clock START,STEP\n"
            "  --dump DIR  --dump-delta  --dump-window N  --msglog FILE\n",
            IMHOTEP_VERSION);
}

int main(int argc, char **argv)
{
    const char *boot = NULL, *script_path = NULL, *load = NULL, *game_arg = NULL;
    int fps = 0;
    int scale = 1, hidden = 0, intro = 1, sound = 1, max_frames = 0;
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *v = i + 1 < argc ? argv[i + 1] : NULL;
        if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
            usage(stdout);
            return 0;
        } else if (strcmp(a, "--version") == 0) {
            SDL_version linked;
            SDL_GetVersion(&linked);
            printf("imhotep %s (SDL %d.%d.%d)\nthe engine of Pharaoh + Cleopatra, checked against GOG Pharaoh + "
                   "Cleopatra 2.1 (Pharaoh.exe SHA-256 %s)\n",
                   IMHOTEP_VERSION, linked.major, linked.minor, linked.patch, FINGERPRINT_EXE_SHA256);
            return 0;
        } else if (strcmp(a, "--game") == 0 && v) {
            game_arg = v, i++;
        } else if (strcmp(a, "--boot-state") == 0 && v) {
            boot = v, i++;
        } else if (strcmp(a, "--scale") == 0 && v) {
            scale = atoi(v), i++;
        } else if (strcmp(a, "--script") == 0 && v) {
            script_path = v, i++;
        } else if (strcmp(a, "--frames") == 0 && v) {
            max_frames = atoi(v), i++;
        } else if (strcmp(a, "--report") == 0 && v) {
            app.report = v, i++;
        } else if (strcmp(a, "--load") == 0 && v) {
            load = v, i++;
        } else if (strcmp(a, "--hidden") == 0) {
            hidden = 1;
            app.hidden = 1;
        } else if (strcmp(a, "--no-intro") == 0) {
            intro = 0;
        } else if (strcmp(a, "--det-clock") == 0 && v) {
            sscanf(v, "%u,%u", &app.det_start, &app.det_step), i++;
            app.det_now = app.det_start;
        } else if (strcmp(a, "--fps") == 0 && v) {
            fps = atoi(v), i++;
        } else if (strcmp(a, "--msglog") == 0 && v) {
            msglog = fopen(v, "w"), i++;
        } else if (strcmp(a, "--dump-delta") == 0) {
            dump_delta = 1;
        } else if (strcmp(a, "--dump-window") == 0 && v) {
            dump_window = atoi(v), i++;
        } else if (strcmp(a, "--dump") == 0 && v) {
            app.dump = v, i++;
        } else if (strcmp(a, "--no-video") == 0) {
            app.no_video = 1;
        } else if (strcmp(a, "--no-sound") == 0) {
            sound = 0;
        } else if (strcmp(a, "-v") == 0) {
            app.verbose = 1;
        } else {
            fprintf(stderr, "imhotep: unknown option or missing value: %s\n", a);
            usage(stderr);
            return 2;
        }
    }
    if (scale < 1) {
        scale = 1;
    }
    app.scale = scale;

    /* the game directory, checked against the reference; the player directory (platform/install.h) */
    static char game_path[1024], player_path[1024];
    if (install_game_dir(game_arg, hidden, app.verbose, game_path, sizeof(game_path)) != 0) {
        return 1;
    }
    if (install_player_dir(player_path, sizeof(player_path)) != 0) {
        fprintf(stderr, "imhotep: no player directory (IMHOTEP_PLAYER_DIR or the per-user data directory)\n");
        return 1;
    }
    app.game = game_path;
    app.player = player_path;
    if (app.verbose) {
        fprintf(stderr, "imhotep: game directory %s, player directory %s\n", app.game, app.player);
    }
    files_init(app.game, app.player);

    /* the state: the exe image, then the boot (core/boot; --boot-state: a saved booted state) */
    present_register();
    if (state_create(&app.st) != 0 || pmem_init() != 0) {
        fprintf(stderr, "imhotep: out of memory\n");
        return 1;
    }
    if (exe_image_load(app.game, &app.st) != 0) {
        fprintf(stderr, "imhotep: cannot read %s/Pharaoh.exe\n", app.game);
        return 1;
    }
    app.env.external = env_external;
    app.st.env = &app.env;
    app.plat = (sim_platform){
        .ctx = NULL,
        .video_play = cb_video_play,
        .stream_play = cb_stream_play,
        .stream_playing = cb_stream_playing,
        .stream_stop = cb_stream_stop,
        .stream_volume = cb_stream_volume,
        .stream_set_volume = cb_stream_set_volume,
        .sample_play = cb_sample_play,
        .log = cb_log,
        .message_box = cb_message_box,
        .file_read = cb_file_read,
        .pixels_load = cb_pixels_load,
        .present = cb_present,
        .video_close = cb_video_close,
        .open_url = cb_open_url,
        .post_message = cb_post_message,
        .quit = cb_quit,
        .file_delete = cb_file_delete,
        .cursor_pos = cb_cursor_pos,
        .key_state = cb_key_state,
        .message_pending = cb_message_pending,
        .clock_ms = cb_clock_ms,
        .pixels_free = cb_pixels_free,
        .file_write = cb_file_write,
        .file_list = cb_file_list,
        .video_open = cb_video_open,
        .video_modal = cb_video_modal,
        .set_display = cb_set_display,
        .window_state = cb_window_state,
        .memory = cb_memory,
    };
    app.st.platform = &app.plat;
    pmem_map(&app.st);
    if (!boot) {
        if (port_boot(sound) != 0) {
            return 1;
        }
    } else if (read_boot_state(boot) != 0) {
        return 1;
    }
    /* the screen's mode after the boot: the size it keeps in 0x00E38DB8 / 0x00E38DB4 (the desktop's, or the
     * game's mode once its boot set it fullscreen) */
    app.screen_w = S32(&app.st, 0x00E38DB8u);
    app.screen_h = S32(&app.st, 0x00E38DB4u);


    /* SDL: the window shows the back buffer scaled */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "imhotep: SDL: %s\n", SDL_GetError());
        return 1;
    }
    int w = S32(&app.st, 0x00E38DB0u), h = S32(&app.st, 0x00E38DACu);
    if (w <= 0 || h <= 0) {
        w = 1024, h = 768;
    }
    app.win = SDL_CreateWindow("Imhotep", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w * scale, h * scale,
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | (hidden ? SDL_WINDOW_HIDDEN : 0));
    app.ren = app.win ? SDL_CreateRenderer(app.win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : NULL;
    if (!app.ren && app.win) {
        app.ren = SDL_CreateRenderer(app.win, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!app.ren) {
        fprintf(stderr, "imhotep: SDL window: %s\n", SDL_GetError());
        return 1;
    }
    SDL_RenderSetLogicalSize(app.ren, w, h);
    app.w = w;
    app.h = h;
    if (sound && audio_init() != 0) {
        fprintf(stderr, "imhotep: no sound output (%s)\n", SDL_GetError());
    }
    if (cursors_load(app.game, scale) != 0) {
        fprintf(stderr, "imhotep: the cursors of Pharaoh.exe could not all be read\n");
    }
    cursors_install(&app.st);
    SDL_StartTextInput();
    if (script_path && strcmp(script_path, "-") == 0) {
        script = stdin;
    } else if (script_path && !(script = fopen(script_path, "r"))) {
        fprintf(stderr, "imhotep: cannot read %s\n", script_path);
    }

    /* @bin 0x004142E1 WinMain: the window rectangle; the intro video (0x00413580, before the loop) */
    U8(&app.st, 0x00E38DCAu) = 1;
    if (load) {
        /* debugging aid, not the original's flow: the saved game loaded with the loader's port
         * (0x004D5100), the city view shown */
        int32_t r = GAME_CALL(&app.st, 0x004D5100u, (int32_t) game_string(load));
        fprintf(stderr, "imhotep: load %s -> %d\n", load, r);
        U8(&app.st, WINDOW_ID) = 1;
        U8(&app.st, 0x00E38DCAu) = 1;
        intro = 0;
    }
    if (intro && U32(&app.st, 0x006FDC54u) != 0) {
        int32_t x = (S32(&app.st, 0x00E38DB0u) - 0x1E0) / 2, y = (S32(&app.st, 0x00E38DACu) - 0x11E) / 2;
        /* @bin 0x004142F2 Binks\low\intro.bik when 0x00E38EDC < 0xE9, else Binks\high\intro_big.bik */
        const char *name = U32(&app.st, 0x00E38EDCu) < 0xE9 ? "Binks\\low\\intro.bik" : "Binks\\high\\intro_big.bik";
        GAME_CALLT(&app.st, U32(&app.st, 0x006FDC54u), 0x00413580u, (int32_t) game_string(name), x, y);
    }

    /* the message loop */
    Uint32 start_ms = SDL_GetTicks();
    while (!app.quit) {
        pump();
        while (app.qcount > 0 && !app.quit) {
            message m = app.queue[app.qhead];
            app.qhead = (app.qhead + 1) % 1024;
            app.qcount--;
            dispatch(m);
            after_message();
        }
        if (app.quit) {
            break;
        }
        if (U32(&app.st, 0x00E39128u) == 0 && U32(&app.st, 0x00E3912Cu) != 0) {
            if (app.det_step) {
                app.det_now = app.det_start + (uint32_t) (app.frames + 1) * app.det_step;
            }
            frame_run(&app.st); /* @bin 0x004143C0 */
            app.frames++;
            if (app.dump) {
                dump_frame();
            }
            after_message();
            cursors_show(U32(&app.st, 0x00E390DCu));
        }
        audio_update();
        script_step();
        render();
        if (fps > 0) { /* a frame-rate cap on top of the display's refresh (vsync) */
            static Uint32 last;
            Uint32 now = SDL_GetTicks(), period = 1000u / (Uint32) fps;
            if (now - last < period) {
                SDL_Delay(period - (now - last));
            }
            last = SDL_GetTicks();
        }
        if (max_frames && app.frames >= max_frames) {
            break;
        }
    }

    /* @bin 0x00414082 WinMain's end */
    GAME_CALL0(&app.st, 0x004E63B0u);
    GAME_CALL0(&app.st, 0x00414750u);
    GAME_CALL0(&app.st, 0x004CFF50u);
    write_report();
    if (msglog) {
        fclose(msglog);
    }
    double secs = (SDL_GetTicks() - start_ms) / 1000.0;
    fprintf(stderr, "imhotep: %d frames in %.1f s (%.1f fps), %d ticks, %d unported functions reached, platform memory %u KB\n",
            app.frames, secs, secs > 0 ? app.frames / secs : 0.0, S32(&app.st, 0x00E94DBCu), app.nun, pmem_used() / 1024);
    if (app.verbose) {
        fprintf(stderr, "imhotep: sound output %s, %lld frames mixed, peak %d\n", audio_ok() ? "open" : "none",
                (long long) audio_mixed(), audio_peak());
    }
    audio_quit();
    cursors_free();
    SDL_Quit();
    return 0;
}
